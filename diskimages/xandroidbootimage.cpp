/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xandroidbootimage.h"

#include <QBuffer>
#include <QPointer>
#include <QtEndian>

#include <memory>
#include <new>

// Layout facts from AOSP system/tools/mkbootimg/include/bootimg/bootimg.h
// (public specification).  Every field is little-endian; header_version sits at
// offset 40 in every version.
namespace {
const qint64 BOOT_MAGIC_SIZE = 8;
const qint64 BOOT_HDR_V0_SIZE = 1632;
const qint64 BOOT_HDR_V1_SIZE = 1648;
const qint64 BOOT_HDR_V2_SIZE = 1660;
const qint64 BOOT_HDR_V3_SIZE = 1580;
const qint64 BOOT_HDR_V4_SIZE = 1584;
const qint64 BOOT_HDR_READ_SIZE = BOOT_HDR_V2_SIZE;
const qint64 BOOT_V3_PAGE_SIZE = 4096;
const qint64 BOOT_MIN_PAGE_SIZE = 2048;
const qint64 BOOT_MAX_PAGE_SIZE = 65536;
const qint64 BOOT_COPY_CHUNK = 1 << 20;

const qint64 OFF_HEADER_VERSION = 40;
// v0-v2
const qint64 OFF_V0_KERNEL_SIZE = 8;
const qint64 OFF_V0_RAMDISK_SIZE = 16;
const qint64 OFF_V0_SECOND_SIZE = 24;
const qint64 OFF_V0_PAGE_SIZE = 36;
const qint64 OFF_V0_NAME = 48;
const qint64 V0_NAME_SIZE = 16;
const qint64 OFF_V0_CMDLINE = 64;
const qint64 V0_CMDLINE_SIZE = 512;
const qint64 OFF_V0_EXTRA_CMDLINE = 608;
const qint64 V0_EXTRA_CMDLINE_SIZE = 1024;
const qint64 OFF_V1_RECOVERY_DTBO_SIZE = 1632;
const qint64 OFF_V1_RECOVERY_DTBO_OFFSET = 1636;
const qint64 OFF_V1_HEADER_SIZE = 1644;
const qint64 OFF_V2_DTB_SIZE = 1648;
// v3-v4
const qint64 OFF_V3_KERNEL_SIZE = 8;
const qint64 OFF_V3_RAMDISK_SIZE = 12;
const qint64 OFF_V3_HEADER_SIZE = 20;
const qint64 OFF_V3_CMDLINE = 44;
const qint64 V3_CMDLINE_SIZE = 1536;
const qint64 OFF_V4_SIGNATURE_SIZE = 1580;

quint32 readLE32(const QByteArray &baData, qint64 nOffset)
{
    if ((nOffset < 0) || (nOffset + 4 > baData.size())) return 0;
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData()) + nOffset);
}

quint64 readLE64(const QByteArray &baData, qint64 nOffset)
{
    if ((nOffset < 0) || (nOffset + 8 > baData.size())) return 0;
    return qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(baData.constData()) + nOffset);
}

// A NUL-padded ASCII field: printable bytes up to the first NUL, NUL after it.
bool readTextField(const QByteArray &baData, qint64 nOffset, qint64 nSize, QByteArray *pText)
{
    if (!pText || (nOffset < 0) || (nSize <= 0) || (nOffset + nSize > baData.size())) return false;
    qint64 nLength = -1;
    for (qint64 i = 0; i < nSize; ++i) {
        const quint8 nValue = static_cast<quint8>(baData.at(static_cast<int>(nOffset + i)));
        if (nLength < 0) {
            if (nValue == 0U) {
                nLength = i;
            } else if ((nValue < 0x20U) || (nValue > 0x7eU)) {
                return false;
            }
        } else if (nValue != 0U) {
            return false;
        }
    }
    if (nLength < 0) nLength = nSize;
    *pText = baData.mid(static_cast<int>(nOffset), static_cast<int>(nLength));
    return true;
}

qint64 alignUp(qint64 nValue, qint64 nPage)
{
    return ((nValue + nPage - 1) / nPage) * nPage;
}
}  // namespace

XAndroidBootImage::XAndroidBootImage(QIODevice *pDevice) : XArchive(pDevice)
{
}

XAndroidBootImage::~XAndroidBootImage()
{
}

bool XAndroidBootImage::parseImage(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    QPointer<XAndroidBootImage> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pContext || !guardedThis || !guardedSource || !guardedSource->isOpen() || !guardedSource->isReadable() ||
        guardedSource->isSequential() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    CONTEXT context = {};
    context.nFileSize = getSize();
    if (!guardedThis || !guardedSource || (context.nFileSize < BOOT_HDR_V3_SIZE)) return false;

    const qint64 nHeaderRead = qMin<qint64>(BOOT_HDR_READ_SIZE, context.nFileSize);
    const QByteArray baHeader = read_array_process(0, nHeaderRead, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != nHeaderRead)) return false;
    if (baHeader.left(static_cast<int>(BOOT_MAGIC_SIZE)) != QByteArray("ANDROID!", 8)) return false;

    context.nHeaderVersion = readLE32(baHeader, OFF_HEADER_VERSION);
    QByteArray baCmdline;
    QList<MEMBER> listMembers;

    if (context.nHeaderVersion <= 2) {
        if (baHeader.size() < BOOT_HDR_V0_SIZE) return false;
        context.nPageSize = readLE32(baHeader, OFF_V0_PAGE_SIZE);
        if ((context.nPageSize < BOOT_MIN_PAGE_SIZE) || (context.nPageSize > BOOT_MAX_PAGE_SIZE) ||
            ((context.nPageSize & (context.nPageSize - 1)) != 0)) {
            return false;
        }
        QByteArray baName;
        QByteArray baExtra;
        if (!readTextField(baHeader, OFF_V0_NAME, V0_NAME_SIZE, &baName) || !readTextField(baHeader, OFF_V0_CMDLINE, V0_CMDLINE_SIZE, &baCmdline) ||
            !readTextField(baHeader, OFF_V0_EXTRA_CMDLINE, V0_EXTRA_CMDLINE_SIZE, &baExtra)) {
            return false;
        }
        baCmdline += baExtra;

        qint64 nHeaderSize = BOOT_HDR_V0_SIZE;
        qint64 nRecoveryDtboSize = 0;
        quint64 nRecoveryDtboOffset = 0;
        qint64 nDtbSize = 0;
        if (context.nHeaderVersion >= 1) {
            if (baHeader.size() < BOOT_HDR_V1_SIZE) return false;
            nHeaderSize = readLE32(baHeader, OFF_V1_HEADER_SIZE);
            if (nHeaderSize != ((context.nHeaderVersion == 1) ? BOOT_HDR_V1_SIZE : BOOT_HDR_V2_SIZE)) return false;
            nRecoveryDtboSize = readLE32(baHeader, OFF_V1_RECOVERY_DTBO_SIZE);
            nRecoveryDtboOffset = readLE64(baHeader, OFF_V1_RECOVERY_DTBO_OFFSET);
        }
        if (context.nHeaderVersion >= 2) {
            if (baHeader.size() < BOOT_HDR_V2_SIZE) return false;
            nDtbSize = readLE32(baHeader, OFF_V2_DTB_SIZE);
        }
        if (nHeaderSize > context.nPageSize) return false;

        const qint64 nSizes[5] = {readLE32(baHeader, OFF_V0_KERNEL_SIZE), readLE32(baHeader, OFF_V0_RAMDISK_SIZE), readLE32(baHeader, OFF_V0_SECOND_SIZE),
                                  nRecoveryDtboSize, nDtbSize};
        const char *pNames[5] = {"kernel", "ramdisk", "second", "recovery_dtbo", "dtb"};
        qint64 nPosition = context.nPageSize;
        for (qint32 i = 0; i < 5; ++i) {
            if (nSizes[i] <= 0) continue;
            if (nPosition + nSizes[i] > context.nFileSize) return false;
            if ((i == 3) && (nRecoveryDtboOffset != static_cast<quint64>(nPosition))) return false;
            MEMBER member = {};
            member.sName = QString::fromLatin1(pNames[i]);
            member.nOffset = nPosition;
            member.nSize = nSizes[i];
            member.bSynthesized = false;
            listMembers.append(member);
            nPosition += alignUp(nSizes[i], context.nPageSize);
        }
    } else if ((context.nHeaderVersion == 3) || (context.nHeaderVersion == 4)) {
        context.nPageSize = BOOT_V3_PAGE_SIZE;
        const qint64 nExpectedHeader = (context.nHeaderVersion == 3) ? BOOT_HDR_V3_SIZE : BOOT_HDR_V4_SIZE;
        if ((baHeader.size() < nExpectedHeader) || (readLE32(baHeader, OFF_V3_HEADER_SIZE) != static_cast<quint32>(nExpectedHeader))) return false;
        if (!readTextField(baHeader, OFF_V3_CMDLINE, V3_CMDLINE_SIZE, &baCmdline)) return false;

        const qint64 nSizes[3] = {readLE32(baHeader, OFF_V3_KERNEL_SIZE), readLE32(baHeader, OFF_V3_RAMDISK_SIZE),
                                  (context.nHeaderVersion == 4) ? static_cast<qint64>(readLE32(baHeader, OFF_V4_SIGNATURE_SIZE)) : 0};
        const char *pNames[3] = {"kernel", "ramdisk", "signature"};
        qint64 nPosition = context.nPageSize;
        for (qint32 i = 0; i < 3; ++i) {
            if (nSizes[i] <= 0) continue;
            if (nPosition + nSizes[i] > context.nFileSize) return false;
            MEMBER member = {};
            member.sName = QString::fromLatin1(pNames[i]);
            member.nOffset = nPosition;
            member.nSize = nSizes[i];
            member.bSynthesized = false;
            listMembers.append(member);
            nPosition += alignUp(nSizes[i], context.nPageSize);
        }
    } else {
        return false;
    }

    if (listMembers.isEmpty() || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    MEMBER cmdline = {};
    cmdline.sName = QStringLiteral("cmdline.txt");
    cmdline.nOffset = (context.nHeaderVersion <= 2) ? OFF_V0_CMDLINE : OFF_V3_CMDLINE;
    cmdline.nSize = baCmdline.size();
    cmdline.bSynthesized = true;
    cmdline.baSynthesized = baCmdline;
    listMembers.append(cmdline);

    context.listMembers = listMembers;
    *pContext = context;
    return true;
}

bool XAndroidBootImage::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseImage(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);
    return bResult;
}

bool XAndroidBootImage::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XAndroidBootImage image(pDevice);
    return image.isValid(pPdStruct);
}

XBinary *XAndroidBootImage::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XAndroidBootImage(pDevice);
}

QList<QString> XAndroidBootImage::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'ANDROID!'");
}

XBinary::FT XAndroidBootImage::getFileType()
{
    return FT_ANDROID_BOOT;
}

XBinary::MODE XAndroidBootImage::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XAndroidBootImage::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XAndroidBootImage::getArch()
{
    return QString();
}

qint32 XAndroidBootImage::getType()
{
    return TYPE_ARCHIVE;
}

QString XAndroidBootImage::getFileFormatExt()
{
    return QStringLiteral("img");
}

QString XAndroidBootImage::getFileFormatExtsString()
{
    return QStringLiteral("Android boot image (*.img)");
}

QString XAndroidBootImage::getMIMEString()
{
    return QStringLiteral("application/x-android-boot-image");
}

QString XAndroidBootImage::getVersion()
{
    CONTEXT context = {};
    if (!parseImage(&context, nullptr)) return QString();
    return QString::number(context.nHeaderVersion);
}

QMap<XBinary::UNPACK_PROP, QVariant> XAndroidBootImage::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XAndroidBootImage::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XAndroidBootImage> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedThis || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) goto failed;
    if (!parseImage(pContext, pPdStruct) || !guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) goto failed;

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.count();
    pState->nCurrentOffset = pContext->listMembers.constFirst().nOffset;
    pState->nTotalSize = pContext->nFileSize;
    pState->pContext = pContext;
    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) goto failed;
    return true;

failed:
    if (guardedThis) releaseUnpackSource(pState);
    delete pContext;
    *pState = UNPACK_STATE();
    return false;
}

XBinary::ARCHIVERECORD XAndroidBootImage::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XAndroidBootImage> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext || !guardedThis || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return ARCHIVERECORD();
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->nFileSize) || (pState->nNumberOfRecords != pContext->listMembers.count()) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }

    const MEMBER member = pContext->listMembers.at(pState->nCurrentIndex);
    if (member.sName.isEmpty() || (member.nSize < 0)) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nOffset;
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, member.bSynthesized ? QStringLiteral("Header field") : QStringLiteral("Store"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();
    return result;
}

bool XAndroidBootImage::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XAndroidBootImage> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(getDevice());
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !guardedThis || !guardedOutput || !guardedSource ||
        !isUnpackOutputSupported(guardedOutput.data()) || devicesAlias(guardedSource.data(), guardedOutput.data()) ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->nFileSize) || (pState->nNumberOfRecords != pContext->listMembers.count()) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    const MEMBER member = pContext->listMembers.at(pState->nCurrentIndex);
    if ((member.nSize < 0) || !isUnpackOutputSizeAllowed(pState->mapUnpackProperties, member.nSize)) {
        XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
        return false;
    }

    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, member.sName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(member.nSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(createFileBuffer(member.nSize, pPdStruct));
    if (!pStage || !pStage->seek(0) || !guardedThis || !guardedOutput || !guardedSource) return false;

    if (member.bSynthesized) {
        if (member.baSynthesized.size() != member.nSize) return false;
        if (member.nSize > 0) {
            const qint64 nWrite = pStage->write(member.baSynthesized.constData(), member.nSize);
            if (nWrite != member.nSize) return false;
        }
    } else {
        if ((member.nOffset < 0) || (member.nOffset + member.nSize > pContext->nFileSize)) return false;
        qint64 nDone = 0;
        while (nDone < member.nSize) {
            if (!guardedThis || !guardedOutput || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            const qint64 nChunk = qMin<qint64>(BOOT_COPY_CHUNK, member.nSize - nDone);
            const QByteArray baChunk = read_array_process(member.nOffset + nDone, nChunk, pPdStruct);
            if (!guardedThis || !guardedOutput || !guardedSource || (baChunk.size() != nChunk) || !isUnpackSourceCurrent(pState, pPdStruct)) return false;
            const qint64 nWrite = pStage->write(baChunk.constData(), nChunk);
            if (nWrite != nChunk) return false;
            nDone += nChunk;
        }
    }

    if ((pStage->size() != member.nSize) || !pStage->seek(0) || !guardedThis || !guardedOutput || !guardedSource ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const bool bResult = publishUnpackOutput(pStage.get(), guardedOutput.data(), pState, pPdStruct);
    if (bResult && guardedThis) pState->nCurrentOffset = member.nOffset + member.nSize;
    return bResult && guardedThis;
}

bool XAndroidBootImage::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XAndroidBootImage> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !guardedThis || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->nFileSize) || (pState->nNumberOfRecords != pContext->listMembers.count()) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nFileSize;
    return false;
}

bool XAndroidBootImage::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}

QList<XBinary::FPART_PROP> XAndroidBootImage::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_REPORTEDMETHOD
                               << FPART_PROP_ISFOLDER;
}
