/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsettlersft.h"

#include "Algos/xsettlersftdecoder.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 SETTLERS_TABLE_OFFSET = 8;
const qint64 SETTLERS_ENTRY_SIZE = 8;
// U3 refuses a member larger than this, and so does this reader.
const qint64 SETTLERS_MAX_ENTRY_SIZE = 0x1000000;
// 8 bytes of header plus the entry table have to fit in a qint32-addressable
// buffer; the real archives declare 4000 entries.
const qint32 SETTLERS_MAX_ENTRIES = 0x100000;
const qint32 SETTLERS_NAME_DIGITS = 4;

QString settlersMemberName(qint32 nIndex, qint32 nKind)
{
    const QString sNumber =
        QStringLiteral("%1").arg(nIndex, SETTLERS_NAME_DIGITS, 10,
                                 QLatin1Char('0'));
    switch (nKind) {
        case XSettlersFTDecoder::KIND_XMI:
            return QStringLiteral("XMIDI/") + sNumber +
                   QStringLiteral(".xmi");
        case XSettlersFTDecoder::KIND_BITMAP:
            return QStringLiteral("Bitmaps/") + sNumber +
                   QStringLiteral(".bmp");
        case XSettlersFTDecoder::KIND_MASK:
            return QStringLiteral("Masks/") + sNumber +
                   QStringLiteral(".bmp");
        case XSettlersFTDecoder::KIND_PALETTE:
            return QStringLiteral("Palettes/") + sNumber +
                   QStringLiteral(".pal");
        default:
            return sNumber + QStringLiteral(".bin");
    }
}

QString settlersKindName(qint32 nKind)
{
    switch (nKind) {
        case XSettlersFTDecoder::KIND_XMI: return QStringLiteral("XMIDI");
        case XSettlersFTDecoder::KIND_BITMAP: return QStringLiteral("Bitmap");
        case XSettlersFTDecoder::KIND_MASK: return QStringLiteral("Sprite RLE");
        case XSettlersFTDecoder::KIND_PALETTE: return QStringLiteral("Palette");
        default: return QStringLiteral("Resource");
    }
}
}  // namespace

XSettlersFT::XSettlersFT(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSettlersFT::~XSettlersFT()
{
}

bool XSettlersFT::parseContext(CONTEXT *pContext, bool bClassify,
                               PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSettlersFT> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // Two entries are the minimum the identifying arithmetic below needs.
    if (context.nInputSize < SETTLERS_TABLE_OFFSET + 2 * SETTLERS_ENTRY_SIZE) {
        return false;
    }

    const QByteArray baHead =
        read_array_process(0, SETTLERS_TABLE_OFFSET, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baHead.size() != SETTLERS_TABLE_OFFSET)) {
        return false;
    }
    const uchar *pHead = reinterpret_cast<const uchar *>(baHead.constData());
    // The only anchor a format with no magic offers: the declared size must be
    // the real file size.
    if (static_cast<qint64>(qFromLittleEndian<quint32>(pHead)) !=
        context.nInputSize) {
        return false;
    }
    const qint64 nCount =
        static_cast<qint64>(qFromLittleEndian<quint32>(pHead + 4));
    if ((nCount < 2) || (nCount > SETTLERS_MAX_ENTRIES)) return false;

    context.nEntryCount = static_cast<qint32>(nCount);
    context.nTableSize = nCount * SETTLERS_ENTRY_SIZE;
    if (SETTLERS_TABLE_OFFSET + context.nTableSize > context.nInputSize) {
        return false;
    }

    const QByteArray baTable = read_array_process(
        SETTLERS_TABLE_OFFSET, context.nTableSize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baTable.size() != context.nTableSize)) {
        return false;
    }
    const uchar *pTable = reinterpret_cast<const uchar *>(baTable.constData());

    // U3's identifying predicate, spelled out: the first member starts right
    // behind the table, the second starts right behind the first, and both are
    // non-empty.  Together with the size anchor this is what stands in for a
    // magic number.
    const qint64 nSize0 =
        static_cast<qint32>(qFromLittleEndian<quint32>(pTable));
    const qint64 nOffset0 =
        static_cast<qint32>(qFromLittleEndian<quint32>(pTable + 4));
    const qint64 nSize1 =
        static_cast<qint32>(qFromLittleEndian<quint32>(pTable + 8));
    const qint64 nOffset1 =
        static_cast<qint32>(qFromLittleEndian<quint32>(pTable + 12));
    if ((nOffset0 != SETTLERS_TABLE_OFFSET + context.nTableSize) ||
        (nSize0 <= 0) || (nSize1 <= 0) || (nOffset1 != nOffset0 + nSize0)) {
        return false;
    }

    qint64 nPaletteOffset = -1;
    for (qint32 i = 0; i < context.nEntryCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nSize = static_cast<qint32>(
            qFromLittleEndian<quint32>(pTable + i * SETTLERS_ENTRY_SIZE));
        const qint64 nOffset = static_cast<qint32>(
            qFromLittleEndian<quint32>(pTable + i * SETTLERS_ENTRY_SIZE + 4));

        // The palette is looked up the way U3 does it - the first entry whose
        // size is exactly 768, whether or not the slot is in use.
        if ((nPaletteOffset < 0) &&
            (nSize == XSettlersFTDecoder::PALETTE_SIZE)) {
            nPaletteOffset = nOffset;
        }
        if (nOffset == 0) continue;  // unused slot
        if ((nOffset < 0) || (nSize < 0) || (nSize > SETTLERS_MAX_ENTRY_SIZE) ||
            (nOffset > context.nInputSize) ||
            (nSize > context.nInputSize - nOffset)) {
            return false;
        }

        MEMBER member = {};
        member.nIndex = i + 1;
        member.nDataOffset = nOffset;
        member.nDataSize = nSize;
        member.nUncompressedSize = nSize;
        member.nKind = XSettlersFTDecoder::KIND_BIN;
        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;
    // Every archive of this family carries its palette; without it the image
    // members could not be produced, and U3 gives up as well.
    if ((nPaletteOffset < 0) ||
        (nPaletteOffset >
         context.nInputSize - XSettlersFTDecoder::PALETTE_SIZE)) {
        return false;
    }
    context.baPalette = read_array_process(
        nPaletteOffset, XSettlersFTDecoder::PALETTE_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (context.baPalette.size() != XSettlersFTDecoder::PALETTE_SIZE)) {
        return false;
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        MEMBER &member = context.listMembers[i];
        if (bClassify) {
            const QByteArray baEntry = read_array_process(
                member.nDataOffset, member.nDataSize, pPdStruct);
            if (!guardedThis || !guardedSource ||
                (baEntry.size() != member.nDataSize)) {
                return false;
            }
            qint64 nOutputSize = member.nDataSize;
            member.nKind = static_cast<qint32>(
                XSettlersFTDecoder::classify(baEntry, &nOutputSize));
            member.nUncompressedSize = nOutputSize;
        }
        member.sFileName = settlersMemberName(member.nIndex, member.nKind);
    }

    context.nArchiveSize = context.nInputSize;
    if (!isPdStructNotCanceled(pPdStruct)) return false;
    *pContext = context;
    return true;
}

void XSettlersFT::fillMemberProperties(const CONTEXT &context,
                                       const MEMBER &member,
                                       QMap<FPART_PROP, QVariant> *pmap)
{
    if (!pmap) return;
    const bool bImage =
        (member.nKind == XSettlersFTDecoder::KIND_BITMAP) ||
        (member.nKind == XSettlersFTDecoder::KIND_MASK);
    pmap->insert(FPART_PROP_COMPRESSEDSIZE, member.nDataSize);
    pmap->insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    pmap->insert(FPART_PROP_HANDLEMETHOD,
                 bImage ? HANDLE_METHOD_SETTLERS_FT : HANDLE_METHOD_STORE);
    pmap->insert(FPART_PROP_REPORTEDMETHOD, settlersKindName(member.nKind));
    if (bImage) {
        // The decoder cannot colour a sprite without the archive's own
        // palette, so it travels with the member.
        pmap->insert(FPART_PROP_COMPRESSPROPERTIES, context.baPalette);
    }
}

bool XSettlersFT::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XSettlersFT::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSettlersFT archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSettlersFT::createInstance(QIODevice *pDevice, bool bIsImage,
                                     XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSettlersFT(pDevice);
}

XBinary::FT XSettlersFT::getFileType()
{
    return FT_SETTLERS_FT;
}

XBinary::MODE XSettlersFT::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSettlersFT::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSettlersFT::getArch()
{
    return QString();
}

QString XSettlersFT::getFileFormatExt()
{
    return QStringLiteral("pa");
}

QString XSettlersFT::getFileFormatExtsString()
{
    return QStringLiteral("Settlers data archive (*.pa)");
}

QString XSettlersFT::getMIMEString()
{
    return QStringLiteral("application/x-settlers-pa");
}

QString XSettlersFT::getVersion()
{
    // The container has no version field at all.
    return QString();
}

qint64 XSettlersFT::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSettlersFT::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSettlersFT::getMemoryMap(MAPMODE mapMode,
                                               PDSTRUCT *pPdStruct)
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

bool XSettlersFT::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSettlersFT::getFileParts(quint32 nFileParts,
                                                qint32 nLimit,
                                                PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    const bool bNeedKinds =
        (nFileParts & (FILEPART_STREAM | FILEPART_REGION)) != 0;
    if (!parseContext(&context, bNeedKinds, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = SETTLERS_TABLE_OFFSET + context.nTableSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Table");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct)) break;
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            fillMemberProperties(context, member, &part.mapProperties);
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
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
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XSettlersFT::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSettlersFT::initUnpack(UNPACK_STATE *pState,
                             const QMap<UNPACK_PROP, QVariant> &mapProperties,
                             PDSTRUCT *pPdStruct)
{
    QPointer<XSettlersFT> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() ||
        !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, true, pPdStruct) || !guardedThis ||
        !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO, tr("Settlers data archive; palette, sprites and raw "
                            "resources"));
    pState->nCurrentOffset = pContext->listMembers.first().nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
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

XBinary::ARCHIVERECORD XSettlersFT::infoCurrent(UNPACK_STATE *pState,
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
    if (pState->nCurrentOffset != member.nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    fillMemberProperties(*pContext, member, &result.mapProperties);
    return result;
}

bool XSettlersFT::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
            pContext->listMembers.at(pState->nCurrentIndex).nDataOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XSettlersFT::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
