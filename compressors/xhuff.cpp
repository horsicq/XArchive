/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xhuff.h"
#include <QtEndian>

#include <algorithm>
#include <new>

#include "Algos/xhuffdecoder.h"

namespace {
const qint64 HUFF_HEADER_SIZE = 10;
const quint16 HUFF_MAGIC = 0x01BDU;
const qint64 HUFF_RECORD_SIZE = 13;
const qint32 HUFF_MAX_MEMBERS = 65535;
const qint64 HUFF_MAX_UNCOMPRESSED_SIZE = 0x40000000;  // 1 GB sanity cap

bool huffRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

bool huffIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (qint32 i = 0; i < baName.size(); ++i) {
        const quint8 nCharacter = quint8(baName.at(i));
        // Member names are plain DOS/CP-M file names; anything outside the
        // printable range means the tree decoded something that is not a name.
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
        const char c = baName.at(i);
        if ((c == '"') || (c == '*') || (c == '<') || (c == '>') ||
            (c == '?') || (c == '|') || (c == ':')) {
            return false;
        }
    }
    return true;
}
}  // namespace

XHUFF::XHUFF(QIODevice *pDevice) : XArchive(pDevice)
{
}

XHUFF::~XHUFF()
{
}

bool XHUFF::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (HUFF_HEADER_SIZE + HUFF_RECORD_SIZE)) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, HUFF_HEADER_SIZE, pPdStruct);
    if (!guardedSource ||
        (baHeader.size() != HUFF_HEADER_SIZE)) {
        return false;
    }
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    if (qFromLittleEndian<quint16>(pHeader) != HUFF_MAGIC) return false;
    const qint32 nMemberCount = qint32(qFromLittleEndian<quint16>(pHeader + 2));
    const qint32 nSymbolCount = qint32(qFromLittleEndian<quint16>(pHeader + 4));
    const qint64 nDirectoryOffset =
        qint64(qFromLittleEndian<quint32>(pHeader + 6));
    if ((nMemberCount <= 0) || (nMemberCount > HUFF_MAX_MEMBERS)) return false;
    if ((nSymbolCount <= 0) || (nSymbolCount > XHuffDecoder::MAX_SYMBOLS)) {
        return false;
    }
    if ((nDirectoryOffset <= 9) || (nDirectoryOffset >= context.nInputSize)) {
        return false;
    }
    const qint64 nDirectorySize = qint64(nMemberCount) * HUFF_RECORD_SIZE;
    if (!huffRangeWithin(context.nInputSize, nDirectoryOffset,
                         nDirectorySize)) {
        return false;
    }

    context.nSymbolCount = nSymbolCount;
    context.nDirectoryOffset = nDirectoryOffset;
    context.nDirectorySize = nDirectorySize;
    context.nTreeOffset = HUFF_HEADER_SIZE + nSymbolCount;
    if (context.nTreeOffset >= nDirectoryOffset) return false;
    context.nTreeSize = nDirectoryOffset - context.nTreeOffset;

    const QByteArray baSymbols =
        read_array_process(HUFF_HEADER_SIZE, nSymbolCount, pPdStruct);
    if (!guardedSource || (baSymbols.size() != nSymbolCount)) {
        return false;
    }
    const QByteArray baTreeBits = read_array_process(
        context.nTreeOffset, context.nTreeSize, pPdStruct);
    if (!guardedSource ||
        (baTreeBits.size() != context.nTreeSize)) {
        return false;
    }

    QVector<XHuffDecoder::NODE> listNodes;
    qint64 nTreeBytesUsed = 0;
    if (!XHuffDecoder::buildTree(baTreeBits, baSymbols, &listNodes,
                                 &nTreeBytesUsed)) {
        return false;
    }
    // The tree ends exactly where the directory begins in every genuine
    // archive; requiring that is what keeps a random BD 01 file out.
    if (nTreeBytesUsed != context.nTreeSize) return false;

    context.baProperty = XHuffDecoder::packTree(baSymbols, baTreeBits);
    if (context.baProperty.isEmpty()) return false;

    const QByteArray baDirectory =
        read_array_process(nDirectoryOffset, nDirectorySize, pPdStruct);
    if (!guardedSource ||
        (baDirectory.size() != nDirectorySize)) {
        return false;
    }

    // Streams have no length field, so the end of one is the start of the next
    // thing in the file; collect every stream start to find that.
    QList<qint64> listStarts;
    for (qint32 i = 0; i < nMemberCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const uchar *pRecord =
            reinterpret_cast<const uchar *>(baDirectory.constData()) +
            (qint64(i) * HUFF_RECORD_SIZE);
        MEMBER member = {};
        member.nIndexOffset = nDirectoryOffset + (qint64(i) * HUFF_RECORD_SIZE);
        member.nNameOffset = qint64(qFromLittleEndian<quint32>(pRecord));
        member.nUncompressedSize =
            qint64(qFromLittleEndian<quint32>(pRecord + 4));
        member.nDataOffset = qint64(qFromLittleEndian<quint32>(pRecord + 8));
        member.nFlags = quint8(pRecord[12]);

        if ((member.nUncompressedSize < 0) ||
            (member.nUncompressedSize > HUFF_MAX_UNCOMPRESSED_SIZE)) {
            return false;
        }
        // Both streams sit past the directory in every specimen, and a stream
        // that started inside the header would be nonsense in any case.
        if ((member.nNameOffset < nDirectoryOffset + nDirectorySize) ||
            (member.nNameOffset >= context.nInputSize)) {
            return false;
        }
        if ((member.nDataOffset < nDirectoryOffset + nDirectorySize) ||
            (member.nDataOffset >= context.nInputSize)) {
            return false;
        }
        listStarts.append(member.nNameOffset);
        listStarts.append(member.nDataOffset);
        context.listMembers.append(member);
    }

    std::sort(listStarts.begin(), listStarts.end());

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        MEMBER &member = context.listMembers[i];

        // The name is coded with the shared tree just like the data is.  A
        // 256-symbol name cannot need more than 256 * 256 bits even with a
        // fully degenerate tree, so this window is a hard upper bound.
        const qint64 nNameWindow =
            qMin<qint64>(context.nInputSize - member.nNameOffset, 8448);
        const QByteArray baNameStream =
            read_array_process(member.nNameOffset, nNameWindow, pPdStruct);
        if (!guardedSource || baNameStream.isEmpty()) {
            return false;
        }
        QByteArray baName;
        if (!XHuffDecoder::decodeSymbols(listNodes, baNameStream, -1,
                                         XHuffDecoder::MAX_NAME_SIZE,
                                         &baName)) {
            return false;
        }
        if (!huffIsValidName(baName)) return false;
        member.sFileName = QString::fromLatin1(baName)
                               .replace(QLatin1Char('\\'), QLatin1Char('/'));

        qint64 nEnd = context.nInputSize;
        for (qint32 j = 0; j < listStarts.size(); ++j) {
            if (listStarts.at(j) > member.nDataOffset) {
                nEnd = listStarts.at(j);
                break;
            }
        }
        member.nStreamSize = nEnd - member.nDataOffset;
        if (member.nStreamSize <= 0) return false;
    }

    if (context.listMembers.isEmpty()) return false;
    if (!guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return true;
}

bool XHUFF::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XHUFF::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XHUFF archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XHUFF::createInstance(QIODevice *pDevice, bool bIsImage,
                               XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XHUFF(pDevice);
}

XBinary::FT XHUFF::getFileType()
{
    return FT_HUFF;
}

XBinary::MODE XHUFF::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XHUFF::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XHUFF::getArch()
{
    return QString();
}

QString XHUFF::getFileFormatExt()
{
    return QStringLiteral("huf");
}

QString XHUFF::getFileFormatExtsString()
{
    return QStringLiteral("HUF archive (*.huf)");
}

QString XHUFF::getMIMEString()
{
    return QStringLiteral("application/x-huf");
}

QString XHUFF::getVersion()
{
    // The u16 magic is the only version-like field the container has.
    return QStringLiteral("1");
}

qint64 XHUFF::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XHUFF::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XHUFF::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(
            FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XHUFF::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XHUFF::getFileParts(quint32 nFileParts, qint32 nLimit,
                                          PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nDirectoryOffset + context.nDirectorySize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        const MEMBER &member = context.listMembers.at(i);
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nStreamSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nUncompressedSize);
            part.mapProperties.insert(
                FPART_PROP_HANDLEMETHOD,
                (member.nUncompressedSize == 0) ? HANDLE_METHOD_STORE
                                                : HANDLE_METHOD_HUFF);
            if (member.nUncompressedSize != 0) {
                part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES,
                                          context.baProperty);
            }
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("Huffman (shared tree)"));
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    if ((nFileParts & FILEPART_OVERLAY) &&
        (context.nArchiveSize < context.nInputSize) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        result.append(part);
    }
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XHUFF::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XHUFF::initUnpack(UNPACK_STATE *pState,
                       const QMap<UNPACK_PROP, QVariant> &mapProperties,
                       PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedSource ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedSource ||
        pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("HUF archive; Huffman members sharing one archive-wide tree"));
    pState->nCurrentOffset = pContext->listMembers.first().nIndexOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized =
        validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XHUFF::infoCurrent(UNPACK_STATE *pState,
                                          PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nIndexOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nUncompressedSize);
    result.mapProperties.insert(
        FPART_PROP_HANDLEMETHOD,
        (member.nUncompressedSize == 0) ? HANDLE_METHOD_STORE
                                        : HANDLE_METHOD_HUFF);
    if (member.nUncompressedSize != 0) {
        // The tree is archive-wide, not per member; without it the member's
        // bytes cannot be decoded at all.
        result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES,
                                    pContext->baProperty);
    }
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("Huffman (shared tree)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // The container carries no CRC and no timestamps.
    return result;
}

bool XHUFF::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nIndexOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XHUFF::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
