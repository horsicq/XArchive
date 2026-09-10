/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xpalmdatabase.h"

#include <QIODevice>
#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
// Every parse runs on a device the caller still owns, so the read cursor has
// to be put back exactly where it was found - detection must never depend on
// where the previous probe happened to leave it.
class PalmPositionKeeper {
public:
    explicit PalmPositionKeeper(QIODevice *pDevice)
        : m_guardedDevice(pDevice),
          m_nPosition(pDevice ? pDevice->pos() : -1)
    {
    }
    ~PalmPositionKeeper()
    {
        if (m_guardedDevice && (m_nPosition >= 0)) {
            m_guardedDevice->seek(m_nPosition);
        }
    }

private:
    QPointer<QIODevice> m_guardedDevice;
    qint64 m_nPosition;
};

// PalmOS DatabaseHdrType.  Every field is big-endian.
//   +0x00 name[32]           NUL-terminated, 8-bit (the corpus carries CP1255
//                            Hebrew titles, so a 7-bit gate would reject 170
//                            of 206 real files)
//   +0x20 attributes u16     bit0 = resource database (.prc)
//   +0x22 version u16
//   +0x24 creationDate u32
//   +0x28 modificationDate u32
//   +0x2c lastBackupDate u32
//   +0x30 modificationNumber u32
//   +0x34 appInfoID u32      file offset or 0
//   +0x38 sortInfoID u32     file offset or 0
//   +0x3c type 4CC
//   +0x40 creator 4CC
//   +0x44 uniqueIDSeed u32
//   +0x48 nextRecordListID u32
//   +0x4c numRecords u16
//   +0x4e entry list
const qint64 PALMDB_HEADER_SIZE = 78;
const qint32 PALMDB_NAME_SIZE = 32;
const qint64 PALMDB_RECORD_ENTRY_SIZE = 8;
const qint64 PALMDB_RESOURCE_ENTRY_SIZE = 10;
const quint32 PALMDB_ATTR_RESOURCE = 0x0001U;
// PalmOS defines bits 0..11, 14 and 15; 0x7000 is reserved and is always
// clear in real databases, so a set bit there is a cheap reject.
const quint32 PALMDB_ATTR_RESERVED_MASK = 0x7000U;
const qint32 PALMDB_MAX_ENTRIES = 65535;
// PalmOS aligns the first block to an even offset, so writers emit either
// nothing or a two-byte filler between the entry list and the first block.
const qint64 PALMDB_MAX_FILLER = 2;

bool palmRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

// The 32-byte name buffer is read at its FIELD size, never size-1: a name
// that fills the buffer completely puts its terminator in the last byte, and
// reading one short would reject the record.
bool palmDecodeName(const uchar *pName, QString *psName)
{
    if (pName[0] == 0) return false;

    qint32 nTerminator = -1;
    for (qint32 i = 0; i < PALMDB_NAME_SIZE; i++) {
        if (pName[i] == 0) {
            nTerminator = i;
            break;
        }
        // Control characters never appear in a PalmOS database name; high
        // bytes do (national character sets).
        if (pName[i] < 0x20) return false;
    }

    const qint32 nLength =
        (nTerminator < 0) ? PALMDB_NAME_SIZE : nTerminator;
    if (nTerminator >= 0) {
        // Everything past the terminator must be padding.  Stale bytes there
        // would mean this is not a PalmOS header at all.
        for (qint32 i = nTerminator; i < PALMDB_NAME_SIZE; i++) {
            if (pName[i] != 0) return false;
        }
    }

    *psName = QString::fromLatin1(reinterpret_cast<const char *>(pName),
                                  nLength);
    return true;
}

bool palmIsPrintable4CC(quint32 nValue)
{
    for (qint32 i = 0; i < 4; i++) {
        const quint8 nCharacter =
            static_cast<quint8>((nValue >> (8 * (3 - i))) & 0xffU);
        if (nCharacter < 0x20 || nCharacter > 0x7e) return false;
    }
    return true;
}
}  // namespace

XPalmDatabase::XPalmDatabase(QIODevice *pDevice) : XArchive(pDevice)
{
}

XPalmDatabase::~XPalmDatabase()
{
}

QString XPalmDatabase::sanitizeToken(const QString &sToken)
{
    QString sResult;
    for (qint32 i = 0; i < sToken.size(); i++) {
        const QChar character = sToken.at(i);
        if (character.isLetterOrNumber() || (character == QLatin1Char('_')) ||
            (character == QLatin1Char('-'))) {
            sResult.append(character);
        } else {
            sResult.append(QLatin1Char('_'));
        }
    }
    if (sResult.isEmpty()) sResult = QStringLiteral("res");
    return sResult;
}

QString XPalmDatabase::fourCCToString(quint32 nValue)
{
    QString sResult;
    for (qint32 i = 0; i < 4; i++) {
        sResult.append(QLatin1Char(
            static_cast<char>((nValue >> (8 * (3 - i))) & 0xffU)));
    }
    return sResult;
}

bool XPalmDatabase::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XPalmDatabase> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;
    PalmPositionKeeper positionKeeper(guardedSource);

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < PALMDB_HEADER_SIZE + 1) return false;

    const QByteArray baHeader =
        read_array_process(0, PALMDB_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        baHeader.size() != PALMDB_HEADER_SIZE) {
        return false;
    }
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());

    if (!palmDecodeName(pHeader, &context.sDatabaseName)) return false;

    context.nAttributes = qFromBigEndian<quint16>(pHeader + 0x20);
    context.nVersion = qFromBigEndian<quint16>(pHeader + 0x22);
    if (context.nAttributes & PALMDB_ATTR_RESERVED_MASK) return false;

    const qint64 nAppInfoOffset =
        static_cast<qint64>(qFromBigEndian<quint32>(pHeader + 0x34));
    const qint64 nSortInfoOffset =
        static_cast<qint64>(qFromBigEndian<quint32>(pHeader + 0x38));
    const quint32 nType = qFromBigEndian<quint32>(pHeader + 0x3c);
    const quint32 nCreator = qFromBigEndian<quint32>(pHeader + 0x40);
    // Both 4CCs are registered ASCII identifiers on PalmOS; nothing legitimate
    // stores binary there, and requiring them is what keeps this permissive
    // "header + offset table" shape from matching unrelated files.
    if (!palmIsPrintable4CC(nType) || !palmIsPrintable4CC(nCreator)) {
        return false;
    }
    context.sType = fourCCToString(nType);
    context.sCreator = fourCCToString(nCreator);
    context.bIsResourceDatabase =
        (context.nAttributes & PALMDB_ATTR_RESOURCE) != 0;
    context.bIsISilo = (context.sType == QLatin1String("ToGo")) &&
                       (context.sCreator == QLatin1String("ToGo"));

    const qint32 nNumberOfEntries =
        static_cast<qint32>(qFromBigEndian<quint16>(pHeader + 0x4c));
    if ((nNumberOfEntries < 1) || (nNumberOfEntries > PALMDB_MAX_ENTRIES)) {
        return false;
    }

    const qint64 nEntrySize = context.bIsResourceDatabase
                                  ? PALMDB_RESOURCE_ENTRY_SIZE
                                  : PALMDB_RECORD_ENTRY_SIZE;
    const qint64 nEntryTableSize =
        nEntrySize * static_cast<qint64>(nNumberOfEntries);
    if (!palmRangeWithin(context.nInputSize, PALMDB_HEADER_SIZE,
                         nEntryTableSize)) {
        return false;
    }
    context.nHeaderSize = PALMDB_HEADER_SIZE + nEntryTableSize;

    const QByteArray baEntries =
        read_array_process(PALMDB_HEADER_SIZE, nEntryTableSize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        baEntries.size() != nEntryTableSize) {
        return false;
    }
    const uchar *pEntries =
        reinterpret_cast<const uchar *>(baEntries.constData());

    // Block offsets are collected in file order: appInfo, sortInfo, then the
    // records/resources.  Sizes are implicit - each block runs to the next
    // offset and the last one runs to EOF - so the walk has to be monotonic
    // to make any sense at all.
    QList<ENTRY> listEntries;

    if (nAppInfoOffset != 0) {
        ENTRY entry = {};
        entry.entryKind = EK_APPINFO;
        entry.nOffset = nAppInfoOffset;
        entry.sName = QStringLiteral("appinfo.bin");
        entry.sInfo = tr("Application info block");
        listEntries.append(entry);
    }
    if (nSortInfoOffset != 0) {
        ENTRY entry = {};
        entry.entryKind = EK_SORTINFO;
        entry.nOffset = nSortInfoOffset;
        entry.sName = QStringLiteral("sortinfo.bin");
        entry.sInfo = tr("Sort info block");
        listEntries.append(entry);
    }

    for (qint32 i = 0; i < nNumberOfEntries; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const uchar *pEntry = pEntries + nEntrySize * i;
        ENTRY entry = {};
        if (context.bIsResourceDatabase) {
            const quint32 nResourceType = qFromBigEndian<quint32>(pEntry);
            const quint32 nResourceId = qFromBigEndian<quint16>(pEntry + 4);
            if (!palmIsPrintable4CC(nResourceType)) return false;
            entry.entryKind = EK_RESOURCE;
            entry.nOffset =
                static_cast<qint64>(qFromBigEndian<quint32>(pEntry + 6));
            entry.nId = nResourceType;
            entry.nAttributes = nResourceId;
            entry.sName =
                QStringLiteral("%1_%2.bin")
                    .arg(sanitizeToken(fourCCToString(nResourceType)))
                    .arg(nResourceId, 5, 10, QLatin1Char('0'));
            entry.sInfo = QStringLiteral("%1 #%2")
                              .arg(fourCCToString(nResourceType))
                              .arg(nResourceId);
        } else {
            entry.entryKind = EK_RECORD;
            entry.nOffset =
                static_cast<qint64>(qFromBigEndian<quint32>(pEntry));
            entry.nAttributes = static_cast<quint32>(pEntry[4]);
            entry.nId = (static_cast<quint32>(pEntry[5]) << 16) |
                        (static_cast<quint32>(pEntry[6]) << 8) |
                        static_cast<quint32>(pEntry[7]);
            entry.sName = QStringLiteral("record_%1.bin")
                              .arg(i, 5, 10, QLatin1Char('0'));
            entry.sInfo = tr("Record");
        }
        listEntries.append(entry);
    }

    // Monotonic, in-range, starting immediately after the entry table (with
    // at most the two-byte alignment filler PalmOS writers emit).
    const qint64 nFirstOffset = listEntries.first().nOffset;
    const qint64 nFiller = nFirstOffset - context.nHeaderSize;
    if ((nFiller < 0) || (nFiller > PALMDB_MAX_FILLER)) return false;

    qint64 nPrevious = context.nHeaderSize;
    for (qint32 i = 0; i < listEntries.size(); i++) {
        const qint64 nOffset = listEntries.at(i).nOffset;
        if ((nOffset < nPrevious) || (nOffset > context.nInputSize)) {
            return false;
        }
        nPrevious = nOffset;
    }
    // The final block must actually contain something; a database whose last
    // entry points at EOF is truncated, not merely empty.
    if (nPrevious >= context.nInputSize) return false;

    for (qint32 i = 0; i < listEntries.size(); i++) {
        const qint64 nNext = (i + 1 < listEntries.size())
                                 ? listEntries.at(i + 1).nOffset
                                 : context.nInputSize;
        listEntries[i].nSize = nNext - listEntries.at(i).nOffset;
        if (context.bIsISilo && (listEntries.at(i).entryKind == EK_RECORD)) {
            listEntries[i].sInfo =
                tr("iSilo record (proprietary compression, stored verbatim)");
        }
    }

    context.nArchiveSize = context.nInputSize;
    context.listEntries = listEntries;
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XPalmDatabase::isValid(PDSTRUCT *pPdStruct)
{
    // Detection probes a device the caller still owns: snapshot and restore.
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XPalmDatabase::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPalmDatabase archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XPalmDatabase::createInstance(QIODevice *pDevice, bool bIsImage,
                                       XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPalmDatabase(pDevice);
}

QList<QString> XPalmDatabase::getSearchSignatures()
{
    // A PalmOS database opens with a free-form 32-byte name, so there is no
    // magic to scan for; detection is the structural walk in parseContext().
    return {};
}

XBinary::FT XPalmDatabase::getFileType()
{
    return FT_PALM_PDB;
}

XBinary::MODE XPalmDatabase::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XPalmDatabase::getEndian()
{
    return ENDIAN_BIG;
}

QString XPalmDatabase::getArch()
{
    return QString();
}

QString XPalmDatabase::getFileFormatExt()
{
    return QStringLiteral("pdb");
}

QString XPalmDatabase::getFileFormatExtsString()
{
    return QStringLiteral("Palm OS database (*.pdb *.prc)");
}

QString XPalmDatabase::getMIMEString()
{
    return QStringLiteral("application/vnd.palm");
}

QString XPalmDatabase::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return QString::number(context.nVersion);
}

QString XPalmDatabase::getInfo(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return QString();

    QString sResult = QStringLiteral("%1 '%2' %3/%4")
                          .arg(context.bIsResourceDatabase
                                   ? QStringLiteral("PRC")
                                   : QStringLiteral("PDB"),
                               context.sDatabaseName, context.sType,
                               context.sCreator);
    if (context.bIsISilo) {
        sResult.append(QStringLiteral(" (iSilo document)"));
    }
    return sResult;
}

qint64 XPalmDatabase::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XPalmDatabase::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XPalmDatabase::getMemoryMap(MAPMODE mapMode,
                                                 PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM |
                                 FILEPART_OVERLAY,
                             pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XPalmDatabase::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XPalmDatabase::getFileParts(quint32 nFileParts,
                                                  qint32 nLimit,
                                                  PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nHeaderSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    for (const ENTRY &entry : context.listEntries) {
        if (!isPdStructNotCanceled(pPdStruct)) break;
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = entry.nOffset;
            part.nFileSize = entry.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = entry.sName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, entry.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      entry.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("Stored"));
            part.mapProperties.insert(FPART_PROP_INFO, entry.sInfo);
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = entry.nOffset;
            part.nFileSize = entry.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = entry.sName;
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
        // parseContext() runs the last block to EOF, so there is never an
        // overlay today; kept so the part list stays correct if that ever
        // changes.
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

QMap<XBinary::UNPACK_PROP, QVariant> XPalmDatabase::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XPalmDatabase::initUnpack(UNPACK_STATE *pState,
                               const QMap<UNPACK_PROP, QVariant> &mapProperties,
                               PDSTRUCT *pPdStruct)
{
    QPointer<XPalmDatabase> guardedThis(this);
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
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedThis ||
        !guardedSource || pContext->listEntries.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        pContext->bIsISilo
            ? tr("Palm OS database holding an iSilo document; the records are "
                 "exposed verbatim (the iSilo text codec is not implemented)")
            : tr("Palm OS database; records exposed as stored members"));
    pState->nCurrentOffset = pContext->listEntries.first().nOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listEntries.size();
    pState->pContext = pContext;

    const bool bFinalized =
        guardedThis->validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
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

XBinary::ARCHIVERECORD XPalmDatabase::infoCurrent(UNPACK_STATE *pState,
                                                  PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pState->nCurrentIndex >= pContext->listEntries.size()) {
        return ARCHIVERECORD();
    }
    const ENTRY &entry = pContext->listEntries.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != entry.nOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = entry.nOffset;
    result.nStreamSize = entry.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, entry.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, entry.nSize);
    // Every block is emitted byte-for-byte.  Palm-application payload codecs
    // (iSilo, PalmDoc, ...) live inside the records and are deliberately not
    // claimed here.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_INFO, entry.sInfo);
    result.mapProperties.insert(FPART_PROP_TYPE,
                                static_cast<quint32>(entry.entryKind));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XPalmDatabase::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pState->nCurrentIndex >= pContext->listEntries.size()) {
        return false;
    }
    // Must advance PAST the last entry: guarding on size() - 1 would make the
    // walk stop one short and list nothing in the GUI and CLI.
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listEntries.size()) {
        pState->nCurrentOffset =
            pContext->listEntries.at(pState->nCurrentIndex).nOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listEntries.size());
}

bool XPalmDatabase::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
