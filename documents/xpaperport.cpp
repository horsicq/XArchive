/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xpaperport.h"

#include <QtEndian>

#include <new>

#include "Algos/xpaperportdecoder.h"

namespace {
const qint64 PAPERPORT_FILE_HEADER_SIZE = 200;
const qint64 PAPERPORT_ROOT_POINTER_OFFSET = 164;
const qint64 PAPERPORT_CHUNK_HEADER_SIZE = 32;
const qint64 PAPERPORT_ROOT_ENTRY_SIZE = 12;
const qint64 PAPERPORT_ROOT_PREFIX_SIZE = 22;
const qint64 PAPERPORT_ITEM_RECORD_SIZE = 62;
const qint64 PAPERPORT_OBJECT_HEADER_SIZE = 0x88;
const qint64 PAPERPORT_TILE_HEADER_SIZE = 14;
const quint16 PAPERPORT_CHUNK_ROOT = 0x8000;
const quint16 PAPERPORT_CHUNK_ITEM = 0x4000;
const quint16 PAPERPORT_CHUNK_IMAGE = 0x1000;
// The root payload has to be bigger than 0x35 for the count block to fit; the
// reference extractor enforces exactly that.
const qint64 PAPERPORT_MIN_ROOT_PAYLOAD = 0x36;
// The container's page count is a quint16.
const qint32 PAPERPORT_MAX_PAGES = 65535;

bool paperportRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XPaperPort::XPaperPort(QIODevice *pDevice) : XArchive(pDevice)
{
}

XPaperPort::~XPaperPort()
{
}

bool XPaperPort::readChunk(qint64 nOffset, qint64 *pnSize, quint16 *pnType, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource || !pnSize || !pnType) return false;
    if (!paperportRangeWithin(guardedSource->size(), nOffset, PAPERPORT_CHUNK_HEADER_SIZE)) return false;

    const QByteArray baChunk = read_array_process(nOffset, PAPERPORT_CHUNK_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baChunk.size() != PAPERPORT_CHUNK_HEADER_SIZE)) return false;
    const uchar *p = reinterpret_cast<const uchar *>(baChunk.constData());
    if ((p[0] != 'V') || (p[1] != 'Z')) return false;
    const qint32 nSize = qFromLittleEndian<qint32>(p + 2);
    if (nSize < static_cast<qint32>(PAPERPORT_CHUNK_HEADER_SIZE)) return false;
    *pnSize = nSize;
    *pnType = qFromLittleEndian<quint16>(p + 26);
    return true;
}

bool XPaperPort::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < PAPERPORT_FILE_HEADER_SIZE + PAPERPORT_CHUNK_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, PAPERPORT_FILE_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != PAPERPORT_FILE_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    if ((pHeader[0] != 'V') || (pHeader[1] != 'i') || (pHeader[2] != 'G')) return false;
    const quint8 nVariant = pHeader[3];
    if ((nVariant != 'C') && (nVariant != 'D') && (nVariant != 'E') && (nVariant != 'F')) return false;
    if (pHeader[5] != 0x1a) return false;
    context.nVariant = nVariant;

    const qint64 nRootOffset = static_cast<qint64>(qFromLittleEndian<qint32>(pHeader + PAPERPORT_ROOT_POINTER_OFFSET));
    qint64 nRootSize = 0;
    quint16 nRootType = 0;
    if (!readChunk(nRootOffset, &nRootSize, &nRootType, pPdStruct)) return false;
    if ((nRootType != PAPERPORT_CHUNK_ROOT) || (nRootSize < PAPERPORT_MIN_ROOT_PAYLOAD)) return false;

    const qint64 nRootBase = nRootOffset + PAPERPORT_CHUNK_HEADER_SIZE;
    if (!paperportRangeWithin(context.nInputSize, nRootBase, PAPERPORT_ROOT_PREFIX_SIZE)) return false;
    const QByteArray baRoot = read_array_process(nRootBase, PAPERPORT_ROOT_PREFIX_SIZE, pPdStruct);
    if (!guardedSource || (baRoot.size() != PAPERPORT_ROOT_PREFIX_SIZE)) return false;
    const uchar *pRoot = reinterpret_cast<const uchar *>(baRoot.constData());
    const qint32 nCount = static_cast<qint32>(qFromLittleEndian<quint16>(pRoot));
    if ((nCount < 1) || (nCount > PAPERPORT_MAX_PAGES)) return false;
    // The count is stored twice; the reference extractor refuses the file when
    // the two copies disagree, and that is what makes the six-byte signature
    // safe to detect on.
    if (static_cast<qint32>(qFromLittleEndian<quint16>(pRoot + 10)) != nCount) return false;

    qint64 nEntry = nRootBase + PAPERPORT_ROOT_PREFIX_SIZE;
    for (qint32 i = 0; i < nCount; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (!paperportRangeWithin(context.nInputSize, nEntry, PAPERPORT_ROOT_ENTRY_SIZE)) return false;
        const QByteArray baEntry = read_array_process(nEntry, PAPERPORT_ROOT_ENTRY_SIZE, pPdStruct);
        if (!guardedSource || (baEntry.size() != PAPERPORT_ROOT_ENTRY_SIZE)) return false;
        const qint64 nItemOffset = static_cast<qint64>(qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(baEntry.constData()) + 4));
        nEntry += PAPERPORT_ROOT_ENTRY_SIZE;

        qint64 nItemSize = 0;
        quint16 nItemType = 0;
        if (!readChunk(nItemOffset, &nItemSize, &nItemType, pPdStruct)) return false;
        if (nItemType != PAPERPORT_CHUNK_ITEM) return false;

        const qint64 nItemBase = nItemOffset + PAPERPORT_CHUNK_HEADER_SIZE;
        if (!paperportRangeWithin(context.nInputSize, nItemBase, PAPERPORT_ITEM_RECORD_SIZE)) return false;
        const QByteArray baItem = read_array_process(nItemBase, PAPERPORT_ITEM_RECORD_SIZE, pPdStruct);
        if (!guardedSource || (baItem.size() != PAPERPORT_ITEM_RECORD_SIZE)) return false;
        const qint64 nImageOffset = static_cast<qint64>(qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(baItem.constData()) + 34));

        qint64 nImageSize = 0;
        quint16 nImageType = 0;
        if (!readChunk(nImageOffset, &nImageSize, &nImageType, pPdStruct)) return false;
        if (nImageType != PAPERPORT_CHUNK_IMAGE) return false;

        const qint64 nObjectOffset = nImageOffset + PAPERPORT_CHUNK_HEADER_SIZE;
        if (!paperportRangeWithin(context.nInputSize, nObjectOffset, nImageSize)) return false;
        if (nImageSize < PAPERPORT_OBJECT_HEADER_SIZE) return false;

        const QByteArray baObjectHeader = read_array_process(nObjectOffset, PAPERPORT_OBJECT_HEADER_SIZE, pPdStruct);
        if (!guardedSource || (baObjectHeader.size() != PAPERPORT_OBJECT_HEADER_SIZE)) return false;
        qint32 nTileHeaderOffset = 0;
        if (!XPaperPortDecoder::probeObjectHeader(baObjectHeader, &nTileHeaderOffset)) return false;
        if (static_cast<qint64>(nTileHeaderOffset) > nImageSize - PAPERPORT_TILE_HEADER_SIZE) return false;

        const QByteArray baTileHeader = read_array_process(nObjectOffset + nTileHeaderOffset, PAPERPORT_TILE_HEADER_SIZE, pPdStruct);
        if (!guardedSource || (baTileHeader.size() != PAPERPORT_TILE_HEADER_SIZE)) return false;
        XPaperPortDecoder::IMAGEINFO info = {};
        if (!XPaperPortDecoder::probeGeometry(baObjectHeader, baTileHeader, &info)) return false;

        MEMBER member = {};
        member.nObjectOffset = nObjectOffset;
        member.nObjectSize = nImageSize;
        member.nOutputSize = info.nOutputSize;
        member.nWidth = info.nWidth;
        member.nHeight = info.nHeight;
        member.nBitsPerPixel = info.nBitsPerPixel;
        member.sFileName = QStringLiteral("%1.bmp").arg(i + 1);
        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XPaperPort::isValid(PDSTRUCT *pPdStruct)
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

bool XPaperPort::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPaperPort archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XPaperPort::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPaperPort(pDevice);
}

QList<QString> XPaperPort::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'ViG'"));  // "ViG" at offset 0
    return listResult;
}

XBinary::FT XPaperPort::getFileType()
{
    return FT_PAPERPORT;
}

XBinary::MODE XPaperPort::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XPaperPort::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XPaperPort::getArch()
{
    return QString();
}

QString XPaperPort::getFileFormatExt()
{
    return QStringLiteral("max");
}

QString XPaperPort::getFileFormatExtsString()
{
    return QStringLiteral("PaperPort document (*.max)");
}

QString XPaperPort::getMIMEString()
{
    return QStringLiteral("image/x-paperport-max");
}

QString XPaperPort::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return QString(QLatin1Char(static_cast<char>(context.nVariant)));
}

qint64 XPaperPort::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XPaperPort::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XPaperPort::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XPaperPort::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XPaperPort::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = PAPERPORT_FILE_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        const bool bRenderable = (member.nBitsPerPixel == 1);
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nObjectOffset;
            part.nFileSize = member.nObjectSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nObjectSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nOutputSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, bRenderable ? HANDLE_METHOD_PAPERPORT : HANDLE_METHOD_UNKNOWN);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      bRenderable ? QStringLiteral("PaperPort tiles") : QStringLiteral("PaperPort JPEG tiles"));
            part.mapProperties.insert(FPART_PROP_WIDTH, member.nWidth);
            part.mapProperties.insert(FPART_PROP_HEIGHT, member.nHeight);
            part.mapProperties.insert(FPART_PROP_BITSPERCOMPONENT, member.nBitsPerPixel);
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nObjectOffset - PAPERPORT_CHUNK_HEADER_SIZE;
            part.nFileSize = member.nObjectSize + PAPERPORT_CHUNK_HEADER_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
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
    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XPaperPort::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XPaperPort::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    if (!parseContext(pContext, pPdStruct) || !guardedSource || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("PaperPort document"));
    pState->nCurrentOffset = pContext->listMembers.first().nObjectOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
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

XBinary::ARCHIVERECORD XPaperPort::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nObjectOffset) return ARCHIVERECORD();
    const bool bRenderable = (member.nBitsPerPixel == 1);

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nObjectOffset;
    result.nStreamSize = member.nObjectSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nObjectSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nOutputSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, bRenderable ? HANDLE_METHOD_PAPERPORT : HANDLE_METHOD_UNKNOWN);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, bRenderable ? QStringLiteral("PaperPort tiles") : QStringLiteral("PaperPort JPEG tiles"));
    result.mapProperties.insert(FPART_PROP_WIDTH, member.nWidth);
    result.mapProperties.insert(FPART_PROP_HEIGHT, member.nHeight);
    result.mapProperties.insert(FPART_PROP_BITSPERCOMPONENT, member.nBitsPerPixel);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XPaperPort::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nObjectOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XPaperPort::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
