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
#include "xbigafarchive.h"

#include <QSet>
#include <QtEndian>

#include <new>

namespace {
const qint64 BIGAF_HEADER_SIZE = 128;
const qint64 BIGAF_MEMBER_HEADER = 112;

// every numeric field is decimal ASCII, blank padded; an all-blank field is a
// legitimate zero
bool bigafNumber(const QByteArray &baBuffer, qint32 nOffset, qint32 nSize, qint64 *pnValue)
{
    const QByteArray baField = baBuffer.mid(nOffset, nSize).trimmed();
    if (baField.isEmpty()) {
        *pnValue = 0;
        return true;
    }
    bool bOk = false;
    const qint64 nValue = baField.toLongLong(&bOk);
    if (!bOk) return false;
    *pnValue = nValue;
    return true;
}
const qint32 BIGAF_MAX_MEMBERS = 100000;
const qint32 BIGAF_MAX_NAME_SIZE = 4096;

bool bigafRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

// The name is a fixed-length field, not a C string; only control bytes make it
// implausible.  Trailing NULs do occur and are trimmed.
bool bigafIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (qint32 i = 0; i < baName.size(); ++i) {
        const quint8 nCharacter = (quint8)baName.at(i);
        if ((nCharacter < 0x20) && (nCharacter != 0)) return false;
    }
    return true;
}
}  // namespace

XBigafArchive::XBigafArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBigafArchive::~XBigafArchive()
{
}

bool XBigafArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < BIGAF_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, BIGAF_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != BIGAF_HEADER_SIZE)) return false;
    if (baHeader.left(8) != QByteArray("<bigaf>\n", 8)) return false;

    qint64 nFirst = 0;
    qint64 nLast = 0;
    if (!bigafNumber(baHeader, 68, 20, &nFirst)) return false;
    if (!bigafNumber(baHeader, 88, 20, &nLast)) return false;
    if ((nFirst <= 0) || (nFirst >= context.nInputSize)) return false;

    QSet<qint64> stSeen;
    qint64 nOffset = nFirst;
    while ((nOffset > 0) && !stSeen.contains(nOffset)) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= BIGAF_MAX_MEMBERS) break;
        stSeen.insert(nOffset);
        if (!bigafRangeWithin(context.nInputSize, nOffset, BIGAF_MEMBER_HEADER)) return false;
        const QByteArray baEntry = read_array_process(nOffset, BIGAF_MEMBER_HEADER, pPdStruct);
        if ((baEntry.size() != BIGAF_MEMBER_HEADER)) return false;

        qint64 nSize = 0;
        qint64 nNext = 0;
        qint64 nNameLength = 0;
        if (!bigafNumber(baEntry, 0, 20, &nSize)) return false;
        if (!bigafNumber(baEntry, 20, 20, &nNext)) return false;
        if (!bigafNumber(baEntry, 108, 4, &nNameLength)) return false;
        if ((nSize < 0) || (nNameLength < 0) || (nNameLength > BIGAF_MAX_NAME_SIZE)) return false;
        if (!bigafRangeWithin(context.nInputSize, nOffset + BIGAF_MEMBER_HEADER, nNameLength)) return false;

        const QByteArray baName = read_array_process(nOffset + BIGAF_MEMBER_HEADER, nNameLength, pPdStruct);
        if ((baName.size() != nNameLength)) return false;
        if (!bigafIsValidName(baName)) return false;

        // the name is followed by a two-byte terminator and the data starts on
        // an even offset
        qint64 nDataOffset = nOffset + BIGAF_MEMBER_HEADER + nNameLength + 2;
        if (nDataOffset & 1) ++nDataOffset;
        if (!bigafRangeWithin(context.nInputSize, nDataOffset, nSize)) return false;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nHeaderSize = nDataOffset - nOffset;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nSize;
        member.nUncompressedSize = nSize;
        member.sFileName = QString::fromLatin1(baName);
        context.listMembers.append(member);

        if (nOffset == nLast) break;
        nOffset = nNext;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = context.nInputSize;
    *pContext = context;

    return true;
}

bool XBigafArchive::isValid(PDSTRUCT *pPdStruct)
{
    // getRecords-style probing displaces the caller's cursor, so snapshot it.
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XBigafArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBigafArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XBigafArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XBigafArchive(pDevice);
}

QList<QString> XBigafArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'<bigaf>'0A");
}

XBinary::FT XBigafArchive::getFileType()
{
    return FT_AIX_BIGAF;
}

XBinary::MODE XBigafArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XBigafArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XBigafArchive::getArch()
{
    return QString();
}

qint32 XBigafArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XBigafArchive::getFileFormatExt()
{
    return QStringLiteral("a");
}

QString XBigafArchive::getFileFormatExtsString()
{
    return QStringLiteral("AIX big archive (*.a)");
}

QString XBigafArchive::getMIMEString()
{
    return QStringLiteral("application/x-archive");
}

QString XBigafArchive::getVersion()
{
    return QString();
}

qint64 XBigafArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XBigafArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XBigafArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XBigafArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XBigafArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
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

QMap<XBinary::UNPACK_PROP, QVariant> XBigafArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XBigafArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
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

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XBigafArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XBigafArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XBigafArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XBigafArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
