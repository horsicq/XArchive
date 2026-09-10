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
#include "xc64wraptorarchive.h"

#include "Algos/xc64wraptordecoder.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const quint32 C64_MEMBER_MAGIC = 0xff4c42ff;
const quint32 C64_END_MARKER = 0x1a1a1a1a;
const qint64 C64_MAGIC_SIZE = 4;
const qint64 C64_CHECKSUM_SIZE = 2;
const qint32 C64_MAX_MEMBERS = 100000;
const qint32 C64_MAX_NAME_SIZE = 4096;
const qint64 C64_MAX_OUTPUT_SIZE = 0x4000000;
// The chain can only be walked in memory, and a C64 archive that needs more
// than this is not one.
const qint64 C64_MAX_INPUT_SIZE = 0x4000000;
}  // namespace

XC64WraptorArchive::XC64WraptorArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XC64WraptorArchive::~XC64WraptorArchive()
{
}

bool XC64WraptorArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XC64WraptorArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < C64_MAGIC_SIZE) || (context.nInputSize > C64_MAX_INPUT_SIZE)) return false;

    const QByteArray baMagic = read_array_process(0, C64_MAGIC_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baMagic.size() != C64_MAGIC_SIZE)) return false;
    if (qFromLittleEndian<quint32>((const uchar *)baMagic.constData()) != C64_MEMBER_MAGIC) return false;

    const QByteArray baFile = read_array_process(0, context.nInputSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baFile.size() != context.nInputSize)) return false;

    const quint8 *pData = (const quint8 *)baFile.constData();
    const qint64 nSize = baFile.size();
    qint64 nPosition = 0;
    bool bEndMarker = false;

    while ((nPosition + C64_MAGIC_SIZE) <= nSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= C64_MAX_MEMBERS) return false;

        const quint32 nTag = qFromLittleEndian<quint32>(pData + nPosition);
        if (nTag == C64_END_MARKER) {
            bEndMarker = true;
            break;
        }
        if (nTag != C64_MEMBER_MAGIC) return false;

        const qint64 nHeaderOffset = nPosition;
        nPosition += C64_MAGIC_SIZE;

        qint64 nNameEnd = nPosition;
        while ((nNameEnd < nSize) && (pData[nNameEnd] != 0)) {
            if ((nNameEnd - nPosition) > C64_MAX_NAME_SIZE) return false;
            ++nNameEnd;
        }
        if (nNameEnd >= nSize) return false;
        const QByteArray baName((const char *)(pData + nPosition), (qint32)(nNameEnd - nPosition));

        // the NUL and the flags byte that follows it
        const qint64 nFlagsOffset = nNameEnd + 1;
        if (nFlagsOffset >= nSize) return false;
        const quint8 nFlags = pData[nFlagsOffset];
        const qint64 nDataOffset = nFlagsOffset + 1;
        if (nDataOffset > nSize) return false;

        qint64 nConsumed = 0;
        qint64 nRawSize = 0;
        if (!XC64WraptorDecoder::scan(pData, nSize, nDataOffset, C64_MAX_OUTPUT_SIZE, &nConsumed, &nRawSize, pPdStruct)) return false;

        MEMBER member = {};
        member.nHeaderOffset = nHeaderOffset;
        member.nHeaderSize = nDataOffset - nHeaderOffset;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nConsumed;
        member.nUncompressedSize = nRawSize;
        member.nFlags = nFlags;
        member.sFileName = QString::fromLatin1(baName);
        context.listMembers.append(member);

        nPosition = nDataOffset + nConsumed + C64_CHECKSUM_SIZE;
        if (nPosition > nSize) {
            // the checksum of the last member may itself be truncated; the walk
            // simply ends there, exactly as the reference's does
            nPosition = nSize;
            break;
        }
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = qMin(bEndMarker ? (nPosition + C64_MAGIC_SIZE) : nPosition, context.nInputSize);
    *pContext = context;

    return true;
}

bool XC64WraptorArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XC64WraptorArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XC64WraptorArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XC64WraptorArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XC64WraptorArchive(pDevice);
}

QList<QString> XC64WraptorArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("ff424cff");
}

XBinary::FT XC64WraptorArchive::getFileType()
{
    return FT_C64WRAPTOR;
}

XBinary::MODE XC64WraptorArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XC64WraptorArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XC64WraptorArchive::getArch()
{
    return QString();
}

qint32 XC64WraptorArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XC64WraptorArchive::getFileFormatExt()
{
    return QStringLiteral("wra");
}

QString XC64WraptorArchive::getFileFormatExtsString()
{
    return QStringLiteral("C64 Wraptor (*.wra *.wr3)");
}

QString XC64WraptorArchive::getMIMEString()
{
    return QStringLiteral("application/x-c64-wraptor");
}

QString XC64WraptorArchive::getVersion()
{
    return QString();
}

qint64 XC64WraptorArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XC64WraptorArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XC64WraptorArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XC64WraptorArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XC64WraptorArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_C64WRAPTOR);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Wraptor LZSS"));
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_OVERLAY) && (context.nArchiveSize < context.nInputSize) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XC64WraptorArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XC64WraptorArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XC64WraptorArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = pContext->listMembers.at(0).nHeaderOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedThis || !guardedSource || !bFinalized) {
        if (!guardedThis) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XC64WraptorArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_C64WRAPTOR);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Wraptor LZSS"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XC64WraptorArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;

    return false;
}

bool XC64WraptorArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}

QList<XBinary::FPART_PROP> XC64WraptorArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
