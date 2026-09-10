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
#include "xtarnextsteparchive.h"

#include <QPointer>

#include <new>

namespace {
const qint64 TARNS_BLOCK_SIZE = 0x200;
const qint64 TARNS_NAME_SIZE = 0xe1;      // 225 - the whole point of the variant
const qint64 TARNS_SIZE_OFFSET = 0xf9;    // 12 octal digits
const qint64 TARNS_MTIME_OFFSET = 0x105;  // 12 octal digits
const qint64 TARNS_CHKSUM_OFFSET = 0x111;
const qint64 TARNS_CHKSUM_SIZE = 8;
const qint64 TARNS_TYPEFLAG_OFFSET = 0x119;
const qint32 TARNS_MAX_MEMBERS = 1000000;

const char TARNS_TYPE_REGULAR_OLD = 0x00;
const char TARNS_TYPE_REGULAR = '0';
const char TARNS_TYPE_LINK = '1';
const char TARNS_TYPE_SYMLINK = '2';
const char TARNS_TYPE_DIRECTORY = '5';

bool tarnsRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

// An octal tar field: NUL terminates it, ASCII whitespace surrounds it, and an
// empty field means zero.  Anything else in the digits makes the field
// unusable rather than zero, which is what keeps a wrong-layout block from
// looking like a size-zero member.
bool tarnsOctal(const QByteArray &baField, qint64 *pnValue)
{
    if (!pnValue) return false;
    *pnValue = 0;

    qint32 nEnd = baField.indexOf((char)0);
    if (nEnd < 0) nEnd = baField.size();
    qint32 nStart = 0;
    while ((nStart < nEnd) && (((quint8)baField.at(nStart) == 0x20) || (((quint8)baField.at(nStart) >= 0x09) && ((quint8)baField.at(nStart) <= 0x0d)))) {
        ++nStart;
    }
    while ((nEnd > nStart) && (((quint8)baField.at(nEnd - 1) == 0x20) || (((quint8)baField.at(nEnd - 1) >= 0x09) && ((quint8)baField.at(nEnd - 1) <= 0x0d)))) {
        --nEnd;
    }
    if (nStart == nEnd) return true;

    qint64 nValue = 0;
    for (qint32 i = nStart; i < nEnd; ++i) {
        const char cDigit = baField.at(i);
        if ((cDigit < '0') || (cDigit > '7')) return false;
        if (nValue > ((qint64)0x0fffffffffffffff)) return false;
        nValue = (nValue << 3) | (qint64)(cDigit - '0');
    }
    *pnValue = nValue;

    return true;
}

// The classic tar checksum computed at the SHIFTED offset: the eight checksum
// bytes count as spaces, i.e. 8 * 0x20 = 0x100.
quint32 tarnsChecksum(const QByteArray &baBlock)
{
    quint32 nSum = 0x100;
    const quint8 *pBlock = (const quint8 *)baBlock.constData();
    for (qint64 i = 0; i < TARNS_CHKSUM_OFFSET; ++i) nSum += pBlock[i];
    for (qint64 i = TARNS_CHKSUM_OFFSET + TARNS_CHKSUM_SIZE; i < TARNS_BLOCK_SIZE; ++i) nSum += pBlock[i];

    return nSum;
}

bool tarnsIsZeroBlock(const QByteArray &baBlock)
{
    const quint8 *pBlock = (const quint8 *)baBlock.constData();
    for (qint64 i = 0; i < TARNS_BLOCK_SIZE; ++i) {
        if (pBlock[i] != 0) return false;
    }
    return true;
}

QString tarnsName(const QByteArray &baField)
{
    const qint32 nZero = baField.indexOf((char)0);
    return QString::fromLatin1((nZero >= 0) ? baField.left(nZero) : baField);
}

}  // namespace

XTarNextStepArchive::XTarNextStepArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XTarNextStepArchive::~XTarNextStepArchive()
{
}

bool XTarNextStepArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XTarNextStepArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < TARNS_BLOCK_SIZE) return false;

    qint64 nOffset = 0;
    bool bFirst = true;

    while ((nOffset + TARNS_BLOCK_SIZE) <= context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= TARNS_MAX_MEMBERS) break;

        const QByteArray baBlock = read_array_process(nOffset, TARNS_BLOCK_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baBlock.size() != TARNS_BLOCK_SIZE)) return false;
        if (tarnsIsZeroBlock(baBlock)) break;

        qint64 nStoredChecksum = 0;
        if (!tarnsOctal(baBlock.mid((qint32)TARNS_CHKSUM_OFFSET, (qint32)TARNS_CHKSUM_SIZE), &nStoredChecksum) || (nStoredChecksum <= 0) ||
            ((quint32)nStoredChecksum != tarnsChecksum(baBlock))) {
            // The first block IS the detector; a later mismatch only ends the walk.
            if (bFirst) return false;
            break;
        }
        bFirst = false;

        qint64 nDataSize = 0;
        if (!tarnsOctal(baBlock.mid((qint32)TARNS_SIZE_OFFSET, 12), &nDataSize)) break;
        qint64 nMTime = 0;
        tarnsOctal(baBlock.mid((qint32)TARNS_MTIME_OFFSET, 12), &nMTime);

        const QString sName = tarnsName(baBlock.left((qint32)TARNS_NAME_SIZE));
        const char cTypeFlag = baBlock.at((qint32)TARNS_TYPEFLAG_OFFSET);
        const qint64 nHeaderOffset = nOffset;
        nOffset += TARNS_BLOCK_SIZE;

        // Links carry no payload; forgetting this walks the cursor into the
        // next member's header and the rest of the archive reads as corrupt.
        if ((cTypeFlag == TARNS_TYPE_LINK) || (cTypeFlag == TARNS_TYPE_SYMLINK)) nDataSize = 0;

        const bool bIsFolder = (cTypeFlag == TARNS_TYPE_DIRECTORY) ||
                               (((cTypeFlag == TARNS_TYPE_REGULAR_OLD) || (cTypeFlag == TARNS_TYPE_REGULAR)) && sName.endsWith(QChar('/')));

        if (!bIsFolder && !sName.isEmpty()) {
            if (!tarnsRangeWithin(context.nInputSize, nOffset, nDataSize)) break;
            MEMBER member = {};
            member.nHeaderOffset = nHeaderOffset;
            member.nDataOffset = nOffset;
            member.nDataSize = nDataSize;
            member.nMTime = (quint32)nMTime;
            member.sFileName = sName;
            context.listMembers.append(member);
        }

        const qint64 nPadded = ((nDataSize + TARNS_BLOCK_SIZE - 1) / TARNS_BLOCK_SIZE) * TARNS_BLOCK_SIZE;
        if (nPadded > (context.nInputSize - nOffset)) {
            nOffset = context.nInputSize;
            break;
        }
        nOffset += nPadded;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = (nOffset < context.nInputSize) ? nOffset : context.nInputSize;
    *pContext = context;

    return true;
}

bool XTarNextStepArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XTarNextStepArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XTarNextStepArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XTarNextStepArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XTarNextStepArchive(pDevice);
}

QList<QString> XTarNextStepArchive::getSearchSignatures()
{
    // No magic exists; the checksum at +0x111 is the identification.
    return QList<QString>();
}

XBinary::FT XTarNextStepArchive::getFileType()
{
    return FT_TARNEXTSTEP;
}

XBinary::MODE XTarNextStepArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XTarNextStepArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XTarNextStepArchive::getArch()
{
    return QString();
}

qint32 XTarNextStepArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XTarNextStepArchive::getFileFormatExt()
{
    return QStringLiteral("tar");
}

QString XTarNextStepArchive::getFileFormatExtsString()
{
    return QStringLiteral("TAR NextStep (*.tar)");
}

QString XTarNextStepArchive::getMIMEString()
{
    return QStringLiteral("application/x-tar");
}

QString XTarNextStepArchive::getVersion()
{
    return QString();
}

qint64 XTarNextStepArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XTarNextStepArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XTarNextStepArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XTarNextStepArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XTarNextStepArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
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
            part.nFileSize = member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nDataSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nDataSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XTarNextStepArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XTarNextStepArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XTarNextStepArchive> guardedThis(this);
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

XBinary::ARCHIVERECORD XTarNextStepArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nDataSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nDataSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XTarNextStepArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XTarNextStepArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XTarNextStepArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
