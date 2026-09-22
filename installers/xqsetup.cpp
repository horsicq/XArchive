/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xqsetup.h"

#include <QBuffer>
#include <QDateTime>
#include <QSet>
#include <QStringList>

#include <cstring>
#include <new>

#include "Algos/xdeflatedecoder.h"
#include "xpe.h"

namespace {
// The container always sits AT the PE overlay; nothing is searched for.
const qint64 QSETUP_MIN_CONTAINER = 0x20;
// A QSetup installer is a desktop setup program. The ceilings only exist so a
// corrupt carrier cannot ask for an unbounded read or an unbounded inflate.
const qint64 QSETUP_MAX_CONTAINER = Q_INT64_C(0x40000000);
const qint64 QSETUP_MAX_FILE_SIZE = Q_INT64_C(0x20000000);
const qint32 QSETUP_MAX_FILE_COUNT = 65536;
const qint32 QSETUP_MAX_HEADER_LINE = 4096;
// The second string is a '|'-joined product list and opens with "|http:".
const qint64 QSETUP_MIN_STRING2 = 7;
// The record trailer: 0x4a bytes, 4A3B2C1D at +0x0c and 0x4a at +0x46.
const qint64 QSETUP_TRAILER_SIZE = 0x4a;
const quint32 QSETUP_TRAILER_MAGIC = 0x4a3b2c1d;
// A zlib member is at least a 2-byte header, one block and a 4-byte footer.
const qint64 QSETUP_MIN_RECORD = 8;
// The record timestamps count seconds from 1980-01-01, not from the Unix
// epoch: read as Unix seconds, three carriers place members eight years BEFORE
// the build stamp in those members' own PE headers; read from 1980 every one
// of them lands after it.
const qint64 QSETUP_EPOCH_1980 = Q_INT64_C(315532800);

quint32 qsRd32(const quint8 *p)
{
    return (quint32)(p[0] | ((quint32)p[1] << 8) | ((quint32)p[2] << 16) | ((quint32)p[3] << 24));
}

// A QSetup builder stores each payload member under an index key, "NNNNN#"
// in front of the installed file name: the project script inside the
// container lists SET_COPY_FILES(00005#blotestw32vccrel.exe) for the file it
// installs as <Application Folder>lotestw32vccrel.exe.  The key is not part
// of the name.  It does carry information, though - the same name appears
// under two keys in two different install groups - so it is kept as the
// collision fallback rather than discarded.
QString qsStripIndexKey(const QString &sName)
{
    const qint32 nHash = sName.indexOf(QLatin1Char('#'));
    if (nHash != 5) return sName;
    for (qint32 i = 0; i < 5; i++) {
        if (!sName.at(i).isDigit()) return sName;
    }

    return sName.mid(6);
}

// A member name is a bare file name. Anything that is not one is refused and
// replaced by a generated name rather than written where it points.
bool qsIsSafeBaseName(const QString &sName)
{
    if (sName.isEmpty() || (sName.size() > 255) || (sName == QLatin1String(".")) || (sName == QLatin1String("..")) || sName.endsWith(QLatin1Char(' ')) ||
        sName.endsWith(QLatin1Char('.'))) {
        return false;
    }

    const QString sForbidden = QStringLiteral("<>:\"/\\|?*");
    for (qint32 i = 0; i < sName.size(); i++) {
        const QChar character = sName.at(i);
        if (sForbidden.contains(character) || !character.isPrint()) return false;
    }
    return true;
}

// zlib's own decoder reads the output device back to check the Adler footer, so
// the decoded stream has to be kept. The cap turns a decompression bomb into a
// write error instead of an allocation.
class QSetupBoundedBuffer : public QBuffer {
public:
    explicit QSetupBoundedBuffer(QByteArray *pBuffer, qint64 nLimit) : QBuffer(pBuffer), m_nLimit(nLimit)
    {
    }

protected:
    qint64 writeData(const char *pData, qint64 nSize) override
    {
        if ((nSize < 0) || (nSize > m_nLimit - size())) return -1;
        return QBuffer::writeData(pData, nSize);
    }

private:
    qint64 m_nLimit;
};
}  // namespace

XQSetup::XQSetup(QIODevice *pDevice) : XArchive(pDevice)
{
}

XQSetup::~XQSetup()
{
}

// Detection is deliberately cheap: the two declared strings at the overlay, the
// "|http:" opening of the second one, and the first record's size word plus a
// readable zlib header behind it. The record walk itself costs a full inflate
// and belongs in parseContext(), not here.
bool XQSetup::locateContainer(qint64 *pnContainerOffset, qint64 *pnRecordsOffset, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pnContainerOffset || !pnRecordsOffset || guardedSource->isSequential()) return false;

    const qint64 nInputSize = guardedSource->size();
    if (nInputSize < QSETUP_MIN_CONTAINER) return false;

    XPE pe(getDevice());
    if (!pe.isValid(pPdStruct)) return false;
    const qint64 nContainerOffset = pe.getOverlayOffset(pPdStruct);
    if ((nContainerOffset <= 0) || (nContainerOffset >= nInputSize)) return false;
    const qint64 nContainerSize = nInputSize - nContainerOffset;
    if ((nContainerSize < QSETUP_MIN_CONTAINER) || (nContainerSize > QSETUP_MAX_CONTAINER)) return false;

    QByteArray baHead = read_array_process(nContainerOffset, 32, pPdStruct);
    if ((baHead.size() != 32)) return false;
    const quint8 *pHead = (const quint8 *)baHead.constData();

    const qint64 nLength1 = (qint64)qsRd32(pHead);
    if ((nLength1 != 1) && (nLength1 != 2)) return false;
    for (qint64 i = 0; i < nLength1; i++) {
        if (pHead[4 + i] != (quint8)'|') return false;
    }

    const qint64 nLength2Offset = 4 + nLength1;
    const qint64 nLength2 = (qint64)qsRd32(pHead + nLength2Offset);
    if ((nLength2 < QSETUP_MIN_STRING2) || (nLength2 > nContainerSize - (nLength2Offset + 4))) return false;
    if (memcmp(pHead + nLength2Offset + 4, "|http:", 6) != 0) return false;

    const qint64 nRecordsOffset = nContainerOffset + nLength2Offset + 4 + nLength2;
    if (nRecordsOffset > nInputSize - (4 + QSETUP_MIN_RECORD)) return false;

    QByteArray baRecord = read_array_process(nRecordsOffset, 8, pPdStruct);
    if ((baRecord.size() != 8)) return false;
    const quint8 *pRecord = (const quint8 *)baRecord.constData();
    const qint64 nRecordSize = (qint64)qsRd32(pRecord);
    if ((nRecordSize < QSETUP_MIN_RECORD) || (nRecordSize > nInputSize - (nRecordsOffset + 4))) return false;
    // RFC 1950 header, no preset dictionary.
    const quint8 nCMF = pRecord[4];
    const quint8 nFLG = pRecord[5];
    if (((nCMF & 0x0F) != 8) || ((nCMF >> 4) > 7) || (((((quint32)nCMF << 8) | (quint32)nFLG) % 31) != 0) || (nFLG & 0x20)) return false;

    *pnContainerOffset = nContainerOffset;
    *pnRecordsOffset = nRecordsOffset;
    return true;
}

// Inflate one record with the decoder this reader publishes the member to, and
// split the decoded stream into its header line and its body.
bool XQSetup::readMember(MEMBER *pMember, qint32 nIndex, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pMember || (pMember->nStreamSize < QSETUP_MIN_RECORD)) return false;

    QByteArray baDecoded;
    QSetupBoundedBuffer output(&baDecoded, QSETUP_MAX_FILE_SIZE);
    if (!output.open(QIODevice::ReadWrite)) return false;

    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = guardedSource;
    state.pDeviceOutput = &output;
    state.nInputOffset = pMember->nStreamOffset;
    state.nInputLimit = pMember->nStreamSize;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;

    const bool bDecoded = XDeflateDecoder::decompress_zlib(&state, pPdStruct);
    output.close();
    if (!bDecoded) return false;
    if ((state.nCountInput != pMember->nStreamSize) || (state.nCountOutput != (qint64)baDecoded.size())) return false;

    const qint32 nTerminator = baDecoded.indexOf('\0');
    if ((nTerminator <= 0) || (nTerminator > QSETUP_MAX_HEADER_LINE)) return false;

    const QString sLine = QString::fromLatin1(baDecoded.constData(), nTerminator);
    const QStringList listFields = sLine.split(QLatin1Char('|'));
    // "|<name>|<seconds>|" splits into exactly four fields, the first and the
    // last of them empty.
    if ((listFields.size() != 4) || !listFields.at(0).isEmpty() || !listFields.at(3).isEmpty()) return false;

    QString sName = listFields.at(1);
    // The builder marks "run this after the install" with a trailing '*'. The
    // mark is not part of the name.
    if (sName.endsWith(QLatin1Char('*'))) sName.chop(1);
    QString sPlainName = qsStripIndexKey(sName);
    if (!qsIsSafeBaseName(sName)) sName = QString("file_%1").arg(nIndex, 4, 10, QChar('0'));
    if (!qsIsSafeBaseName(sPlainName)) sPlainName = sName;

    bool bSecondsOk = false;
    const quint32 nSeconds = listFields.at(2).toUInt(&bSecondsOk);
    if (!bSecondsOk) return false;

    pMember->sFileName = sPlainName;
    pMember->sKeyedName = sName;
    pMember->nTotalUnpackedSize = (qint64)baDecoded.size();
    pMember->nSubstreamOffset = (qint64)nTerminator + 1;
    pMember->nUncompressedSize = pMember->nTotalUnpackedSize - pMember->nSubstreamOffset;
    pMember->nSeconds1980 = nSeconds;

    return (pMember->nUncompressedSize >= 0) && isPdStructNotCanceled(pPdStruct);
}

bool XQSetup::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (!locateContainer(&context.nContainerOffset, &context.nRecordsOffset, pPdStruct)) return false;

    qint64 nPosition = context.nRecordsOffset;
    bool bTrailerFound = false;

    while (!bTrailerFound) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (QSETUP_TRAILER_SIZE > context.nInputSize - nPosition) return false;

        const QByteArray baHeader = read_array_process(nPosition, QSETUP_TRAILER_SIZE, pPdStruct);
        if ((baHeader.size() != QSETUP_TRAILER_SIZE)) return false;
        const quint8 *pHeader = (const quint8 *)baHeader.constData();

        if ((qsRd32(pHeader + 0x0c) == QSETUP_TRAILER_MAGIC) && ((qint64)qsRd32(pHeader + 0x46) == QSETUP_TRAILER_SIZE)) {
            // The trailer repeats what the walk just produced. A disagreement
            // means the walk desynchronised, and publishing its members would
            // publish garbage.
            if ((qint64)qsRd32(pHeader + 4) != context.nContainerOffset) return false;
            if ((qint64)qsRd32(pHeader + 8) != (qint64)context.listMembers.size()) return false;
            context.nArchiveSize = nPosition + QSETUP_TRAILER_SIZE;
            bTrailerFound = true;
            break;
        }

        const qint64 nRecordSize = (qint64)qsRd32(pHeader);
        if ((nRecordSize < QSETUP_MIN_RECORD) || (nRecordSize > context.nInputSize - (nPosition + 4))) return false;
        if (context.listMembers.size() >= QSETUP_MAX_FILE_COUNT) return false;

        MEMBER member = {};
        member.nStreamOffset = nPosition + 4;
        member.nStreamSize = nRecordSize;
        if (!readMember(&member, context.listMembers.size(), pPdStruct)) return false;

        context.listMembers.append(member);
        nPosition += 4 + nRecordSize;
    }

    // Two members may install the same file name into two different groups
    // (the PocketPC carrier ships Setup.exe three times).  The container's
    // index key is what tells them apart, so fall back to it rather than let
    // the second record silently overwrite the first.
    QSet<QString> setUsedNames;
    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        QString sName = context.listMembers.at(i).sFileName;
        if (setUsedNames.contains(sName.toCaseFolded())) sName = context.listMembers.at(i).sKeyedName;
        if (setUsedNames.contains(sName.toCaseFolded())) sName = QString("file_%1").arg(i, 4, 10, QChar('0'));
        if (setUsedNames.contains(sName.toCaseFolded())) return false;
        setUsedNames.insert(sName.toCaseFolded());
        context.listMembers[i].sFileName = sName;
    }

    if (context.listMembers.isEmpty()) return false;

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

void XQSetup::fillMemberProperties(const CONTEXT &context, qint32 nIndex, QMap<FPART_PROP, QVariant> *pMapProperties)
{
    const MEMBER &member = context.listMembers.at(nIndex);

    pMapProperties->insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    pMapProperties->insert(FPART_PROP_COMPRESSEDSIZE, member.nStreamSize);
    pMapProperties->insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    pMapProperties->insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZLIB);
    pMapProperties->insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Deflate"));
    pMapProperties->insert(FPART_PROP_ISFOLDER, false);
    // One member per folder: the record's own header line occupies the bytes in
    // front of the substream, so the whole stream has to be decoded before the
    // body can be emitted.
    pMapProperties->insert(FPART_PROP_ISSOLID, true);
    pMapProperties->insert(FPART_PROP_SUBSTREAMOFFSET, member.nSubstreamOffset);
    pMapProperties->insert(FPART_PROP_STREAMUNPACKEDSIZE, member.nTotalUnpackedSize);
    pMapProperties->insert(FPART_PROP_SOLIDFOLDERINDEX, (qint64)nIndex);

    if (member.nSeconds1980 > 0) {
        pMapProperties->insert(FPART_PROP_DATETIME, QDateTime::fromMSecsSinceEpoch(((qint64)member.nSeconds1980 + QSETUP_EPOCH_1980) * 1000, Qt::UTC));
    }
}

bool XQSetup::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    qint64 nContainerOffset = 0;
    qint64 nRecordsOffset = 0;
    const bool bResult = locateContainer(&nContainerOffset, &nRecordsOffset, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XQSetup::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XQSetup archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XQSetup::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XQSetup(pDevice);
}

XBinary::FT XQSetup::getFileType()
{
    return FT_QSETUP;
}

XBinary::MODE XQSetup::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XQSetup::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XQSetup::getArch()
{
    return QString();
}

QString XQSetup::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XQSetup::getFileFormatExtsString()
{
    return QStringLiteral("QSetup installer (*.exe)");
}

QString XQSetup::getMIMEString()
{
    return QStringLiteral("application/x-qsetup-installer");
}

QString XQSetup::getVersion()
{
    return QString();
}

qint64 XQSetup::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XQSetup::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XQSetup::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XQSetup::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XQSetup::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nRecordsOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = context.listMembers.at(i).nStreamOffset;
            part.nFileSize = context.listMembers.at(i).nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = context.listMembers.at(i).sFileName;
            fillMemberProperties(context, i, &part.mapProperties);
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

QMap<XBinary::UNPACK_PROP, QVariant> XQSetup::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XQSetup::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("QSetup installer"));
    pState->nCurrentOffset = pContext->listMembers.at(0).nStreamOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XQSetup::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const qint32 nIndex = pState->nCurrentIndex;
    if (pState->nCurrentOffset != pContext->listMembers.at(nIndex).nStreamOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->listMembers.at(nIndex).nStreamOffset;
    result.nStreamSize = pContext->listMembers.at(nIndex).nStreamSize;
    fillMemberProperties(*pContext, nIndex, &result.mapProperties);
    return result;
}

bool XQSetup::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nStreamOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XQSetup::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
