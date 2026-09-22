/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xoraclesqueeze.h"
#include <new>

namespace {
const qint64 ORASQ_HEADER_SIZE = 8;  // magic, decoded length, checksum
// The name is a bare DOS file name in every known producer; the cap keeps the
// scan that looks for its terminator bounded on non-Squeeze input.
const qint64 ORASQ_MAX_NAME = 255;
// 256 byte values plus the end marker share 257 leaves, so a well-formed tree
// never holds more than 256 internal nodes.
const qint64 ORASQ_MAX_NODES = 256;
const qint64 ORASQ_LEAF_EOF = 256;
// Everything the header check can possibly need: 8 + name + NUL + count + tree.
const qint64 ORASQ_PROBE_SIZE = ORASQ_HEADER_SIZE + ORASQ_MAX_NAME + 1 + 2 + (ORASQ_MAX_NODES * 4);

quint16 orasqReadLe16(const QByteArray &baData, qint64 nOffset)
{
    return (quint16)((quint8)baData.at((qint32)nOffset)) | (quint16)(((quint8)baData.at((qint32)nOffset + 1)) << 8);
}

quint32 orasqReadLe32(const QByteArray &baData, qint64 nOffset)
{
    return (quint32)orasqReadLe16(baData, nOffset) | ((quint32)orasqReadLe16(baData, nOffset + 2) << 16);
}

bool orasqIsMagic(const QByteArray &baData)
{
    return (baData.size() >= 2) && ((quint8)baData.at(0) == 0x76) && ((quint8)baData.at(1) == 0xFF);
}
}  // namespace

XOracleSqueeze::XOracleSqueeze(QIODevice *pDevice) : XArchive(pDevice)
{
}

XOracleSqueeze::~XOracleSqueeze()
{
}

// The stored name is a bare DOS file name, but it is raw bytes: '\' and '/' are
// escaped rather than treated as separators, because a single-member container
// must never be able to write outside the extraction root.
QString XOracleSqueeze::sanitizeName(const QByteArray &baRawName)
{
    QString sResult;
    for (qint32 i = 0; i < baRawName.size(); i++) {
        const quint8 nCharacter = (quint8)baRawName.at(i);
        const bool bSafe = (nCharacter > 0x20) && (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != '/') && (nCharacter != '\\') &&
                           (nCharacter != ':') && (nCharacter != '*') && (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') &&
                           (nCharacter != '>') && (nCharacter != '|');
        if (bSafe) {
            sResult.append(QLatin1Char((char)nCharacter));
        } else if (nCharacter == 0x20) {
            sResult.append(QLatin1Char(' '));
        } else {
            QString sHex = QString::number(nCharacter, 16).toUpper();
            if (sHex.size() < 2) sHex.prepend(QLatin1Char('0'));
            sResult.append(QLatin1Char('%'));
            sResult.append(sHex);
        }
    }
    while (sResult.endsWith(QLatin1Char(' '))) sResult.chop(1);
    return sResult;
}

qint64 XOracleSqueeze::scanName(const QByteArray &baProbe, qint64 nOffset, QString *pName)
{
    const qint64 nProbeSize = baProbe.size();
    qint64 nLength = 0;
    while ((nOffset + nLength < nProbeSize) && (nLength <= ORASQ_MAX_NAME) && ((quint8)baProbe.at((qint32)(nOffset + nLength)) != 0)) {
        // Control bytes and DEL are what keep a random 76 FF from matching; every
        // producer of this container stores a printable DOS name.
        const quint8 nCharacter = (quint8)baProbe.at((qint32)(nOffset + nLength));
        if ((nCharacter < 0x20) || (nCharacter == 0x7f)) return -1;
        nLength++;
    }
    if (nLength < 1) return -1;                                             // no name at all
    if (nOffset + nLength >= nProbeSize) return -1;                         // ran out before the terminator
    if ((quint8)baProbe.at((qint32)(nOffset + nLength)) != 0) return -1;    // hit the length cap instead
    if (pName) *pName = sanitizeName(baProbe.mid((qint32)nOffset, (qint32)nLength));
    return nOffset + nLength + 1;
}

// The decoding tree is a flat array of node pairs: a non-negative child is the
// next node index, a negative child is the leaf value -(child + 1), and leaf 256
// is the end marker.  Checking that every child is in range is what turns "the
// magic happens to match" into a decision, and it costs at most 1 KiB.
qint64 XOracleSqueeze::scanTree(const QByteArray &baProbe, qint64 nOffset, qint64 nInputSize)
{
    if (nOffset < 0) return -1;
    if (nOffset + 2 > (qint64)baProbe.size()) return -1;

    const qint64 nNumberOfNodes = (qint64)orasqReadLe16(baProbe, nOffset);
    if ((nNumberOfNodes < 1) || (nNumberOfNodes > ORASQ_MAX_NODES)) return -1;

    const qint64 nTreeOffset = nOffset + 2;
    const qint64 nTreeSize = nNumberOfNodes * 4;
    if (nTreeOffset + nTreeSize > (qint64)baProbe.size()) return -1;
    // At least one byte of bit stream has to follow the tree.
    if (nTreeOffset + nTreeSize + 1 > nInputSize) return -1;

    for (qint64 i = 0; i < nNumberOfNodes * 2; i++) {
        const qint32 nChild = (qint32)(qint16)orasqReadLe16(baProbe, nTreeOffset + (i * 2));
        if (nChild >= nNumberOfNodes) return -1;                            // forward index out of range
        if ((nChild < 0) && ((-(nChild + 1)) > ORASQ_LEAF_EOF)) return -1;  // leaf out of range
    }

    // The root of a Squeeze tree is built last and always points at the two
    // subtrees created just before it, so only these three shapes occur.
    const qint32 nLeft = (qint32)(qint16)orasqReadLe16(baProbe, nTreeOffset);
    const qint32 nRight = (qint32)(qint16)orasqReadLe16(baProbe, nTreeOffset + 2);
    const bool bRoot = ((nLeft == 1) && (nRight == 2)) || ((nLeft == 2) && (nRight == 1)) || ((nLeft == -1) && (nRight == 1));
    if (!bRoot) return -1;

    return nTreeOffset + nTreeSize;
}

// A plain CP/M Squeeze has its name at 0x04 and no decoded-length field.  Both
// layouts describe the same payload, so the only honest way to separate them is
// to hand anything that already parses as the classic header to the classic
// reader and claim only what does not.
bool XOracleSqueeze::looksLikeClassicSqueeze(const QByteArray &baProbe, qint64 nInputSize)
{
    const qint64 nAfterName = scanName(baProbe, 4, nullptr);
    if (nAfterName < 0) return false;
    return (scanTree(baProbe, nAfterName, nInputSize) >= 0);
}

bool XOracleSqueeze::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nUncompressedSize = -1;
    context.nInputSize = guardedSource->size();
    // header + a one-character name + NUL + node count + one node + one payload byte.
    if (context.nInputSize < ORASQ_HEADER_SIZE + 2 + 2 + 4 + 1) return false;

    // The magic is checked on its own first: this runs during type detection on
    // every file, and only a 76 FF file is worth the 1 KiB probe read below.
    const QByteArray baMagic = read_array_process(0, 2, pPdStruct);
    if (!guardedSource || (baMagic.size() != 2) || !orasqIsMagic(baMagic)) return false;

    const qint64 nProbeSize = qMin(context.nInputSize, ORASQ_PROBE_SIZE);
    const QByteArray baProbe = read_array_process(0, nProbeSize, pPdStruct);
    if (!guardedSource || (baProbe.size() != nProbeSize)) return false;

    if (looksLikeClassicSqueeze(baProbe, context.nInputSize)) return false;

    const qint64 nUncompressedSize = (qint64)orasqReadLe32(baProbe, 2);
    if (nUncompressedSize <= 0) return false;

    QString sFileName;
    const qint64 nAfterName = scanName(baProbe, ORASQ_HEADER_SIZE, &sFileName);
    if (nAfterName < 0) return false;
    if (sFileName.isEmpty()) return false;

    const qint64 nBitStreamOffset = scanTree(baProbe, nAfterName, context.nInputSize);
    if (nBitStreamOffset < 0) return false;

    // The bit stream starts at the node count: that is exactly what the shared
    // ARC method 4 decoder expects to see first.
    context.nStreamOffset = nAfterName;
    context.nStreamSize = context.nInputSize - context.nStreamOffset;
    context.nArchiveSize = context.nInputSize;
    context.nUncompressedSize = nUncompressedSize;
    context.nChecksum = orasqReadLe16(baProbe, 6);
    context.sFileName = sFileName;
    if (context.nStreamSize <= 0) return false;

    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XOracleSqueeze::isValid(PDSTRUCT *pPdStruct)
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

bool XOracleSqueeze::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XOracleSqueeze archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XOracleSqueeze::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XOracleSqueeze(pDevice);
}

QList<QString> XOracleSqueeze::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("76FF"));
    return listResult;
}

XBinary::FT XOracleSqueeze::getFileType()
{
    return FT_ORACLE_SQUEEZE;
}

XBinary::MODE XOracleSqueeze::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XOracleSqueeze::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XOracleSqueeze::getArch()
{
    return QString();
}

QString XOracleSqueeze::getFileFormatExt()
{
    return QStringLiteral("sq");
}

QString XOracleSqueeze::getFileFormatExtsString()
{
    return QStringLiteral("Oracle squeezed file (*.sq *.msg *.hlp)");
}

QString XOracleSqueeze::getMIMEString()
{
    return QStringLiteral("application/x-oracle-squeeze");
}

QString XOracleSqueeze::getVersion()
{
    return QString();
}

qint64 XOracleSqueeze::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XOracleSqueeze::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XOracleSqueeze::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XOracleSqueeze::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XOracleSqueeze::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nStreamOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nStreamOffset;
        part.nFileSize = context.nStreamSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nStreamSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ARC_SQUEEZE);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Squeezed (RLE + Huffman)"));
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = context.nStreamOffset;
        part.nFileSize = context.nStreamSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        listResult.append(part);
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
    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XOracleSqueeze::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XOracleSqueeze::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedSource || (pContext->nUncompressedSize < 0)) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Oracle squeezed file"));
    pState->nCurrentOffset = pContext->nStreamOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XOracleSqueeze::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex != 0) || (pContext->nUncompressedSize < 0)) return ARCHIVERECORD();
    if (pState->nCurrentOffset != pContext->nStreamOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nStreamOffset;
    result.nStreamSize = pContext->nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ARC_SQUEEZE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Squeezed (RLE + Huffman)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XOracleSqueeze::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    ++pState->nCurrentIndex;
    pState->nCurrentOffset = pContext->nArchiveSize;
    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XOracleSqueeze::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
