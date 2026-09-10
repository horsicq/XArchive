/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 *
 * The container layout below was recovered from the 2537-file NetWare
 * installation-disk corpus: every one of them walks end to end with no
 * leftover bytes, and the decimal size text, the u32 size in the directory
 * entry and the u32 size in front of the compressed stream agree in all of
 * them.  The PackedData codec (format 0x01 / method 0x0A) is the same LZ77 +
 * three-Huffman-tree stream as the Personal NetWare "Packed File " format, so
 * extraction rides HANDLE_METHOD_NETWARE_PACK through the generic decode chain.
 */

#include "xnetwareinstallfile.h"

#include <QPointer>
#include <QStringList>

#include <cstring>
#include <limits>
#include <new>
#include <QTimeZone>

namespace {

// A container needs the three mandatory chunks; the smallest real sample is
// 185 bytes, so this floor only rejects obvious noise.
const qint64 NWIF_MIN_FILE_SIZE = 64;
const qint64 NWIF_MAX_FILE_SIZE = Q_INT64_C(1) * 1024 * 1024 * 1024;

// "NetWareFileInfo" chunk: u32 0x23, u8 0x10, the 15-character key, then a
// fixed 15-byte value.  The value is byte-identical in every known sample and
// is the only thing in the format that behaves like a magic number.
const qint64 NWIF_SIGNATURE_CHUNK_SIZE = 0x23;
const qint32 NWIF_SIGNATURE_KEY_LENGTH = 0x10;
const qint32 NWIF_SIGNATURE_VALUE_SIZE = 15;
const quint8 NWIF_SIGNATURE_VALUE[NWIF_SIGNATURE_VALUE_SIZE] = {0x0aU, 0x0aU, 0x1aU, 0x5fU, 0x5fU, 0x6fU, 0x55U, 0x0aU,
                                                                0x0aU, 0x1aU, 0x00U, 0x56U, 0x55U, 0x75U, 0x55U};

const qint32 NWIF_MIN_CHUNK_SIZE = 6;
const qint32 NWIF_MAX_CHUNK_KEY = 64;
const qint32 NWIF_MAX_CHUNKS = 64;

// NetWareFile value: u8 record version, u8 sub-version, 16 stamp characters,
// 10 decimal size digits, 24 hex digest characters, u32 size, u32 native
// timestamp, u8 name length, name, NUL, then a 12-byte trailer.
const qint32 NWIF_REC_VERSION = 0x04;
const qint32 NWIF_STAMP_SIZE = 16;
const qint32 NWIF_DECIMAL_SIZE = 10;
const qint32 NWIF_DIGEST_SIZE = 24;
const qint32 NWIF_REC_FIXED_PREFIX = 2 + NWIF_STAMP_SIZE + NWIF_DECIMAL_SIZE + NWIF_DIGEST_SIZE + 4 + 4;
const qint32 NWIF_REC_TRAILER_SIZE = 12;
const qint32 NWIF_MAX_NAME_SIZE = 64;

// PackedData value: u8 format version, u8 method, u32 uncompressed size, then
// the bit stream.
const quint8 NWIF_PACK_VERSION = 0x01;
const quint8 NWIF_PACK_METHOD_LZH = 0x0aU;
const qint32 NWIF_PACK_HEADER_SIZE = 6;

const qint64 NWIF_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(4) * 1024 * 1024 * 1024 - 1;

bool nwifIsHexDigit(char cCharacter)
{
    return ((cCharacter >= '0') && (cCharacter <= '9')) || ((cCharacter >= 'A') && (cCharacter <= 'F'));
}

bool nwifIsKeyCharacter(quint8 nCharacter)
{
    if ((nCharacter >= 'A') && (nCharacter <= 'Z')) return true;
    if ((nCharacter >= 'a') && (nCharacter <= 'z')) return true;
    if ((nCharacter >= '0') && (nCharacter <= '9')) return true;
    return (nCharacter == '=') || (nCharacter == '_');
}

quint32 nwifReadUInt32(const QByteArray &baData, qint32 nOffset)
{
    return (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset)))) |
           (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 1))) << 8) |
           (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 2))) << 16) |
           (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 3))) << 24);
}

}  // namespace

XNetWareInstallFile::XNetWareInstallFile(QIODevice *pDevice) : XArchive(pDevice)
{
}

XNetWareInstallFile::~XNetWareInstallFile()
{
}

// The member name is handed to the extractor as an output file name, so every
// separator and traversal shape has to die here.
bool XNetWareInstallFile::isValidMemberName(const QByteArray &baName)
{
    const qint32 nSize = baName.size();
    if ((nSize < 1) || (nSize > NWIF_MAX_NAME_SIZE)) return false;
    if ((baName == ".") || (baName == "..")) return false;

    for (qint32 i = 0; i < nSize; i++) {
        const quint8 nCharacter = static_cast<quint8>(baName.at(i));
        if ((nCharacter < 0x20U) || (nCharacter >= 0x7fU)) return false;
        if ((nCharacter == '/') || (nCharacter == '\\') || (nCharacter == ':') || (nCharacter == '*') || (nCharacter == '?') || (nCharacter == '"') ||
            (nCharacter == '<') || (nCharacter == '>') || (nCharacter == '|')) {
            return false;
        }
    }
    return true;
}

// "MM-DD-YYHH:MM:SS", the only human-readable stamp the record carries.  The
// two-digit year is windowed the way the installers themselves did.
QDateTime XNetWareInstallFile::parseStamp(const QByteArray &baStamp)
{
    if (baStamp.size() != NWIF_STAMP_SIZE) return QDateTime();
    for (qint32 i = 0; i < NWIF_STAMP_SIZE; i++) {
        const char cCharacter = baStamp.at(i);
        const bool bSeparator = ((i == 2) || (i == 5)) ? (cCharacter == '-') : (((i == 10) || (i == 13)) ? (cCharacter == ':') : false);
        if (!bSeparator && ((cCharacter < '0') || (cCharacter > '9'))) return QDateTime();
    }

    bool bOk = false;
    const qint32 nMonth = baStamp.mid(0, 2).toInt(&bOk);
    if (!bOk) return QDateTime();
    const qint32 nDay = baStamp.mid(3, 2).toInt(&bOk);
    if (!bOk) return QDateTime();
    const qint32 nYear = baStamp.mid(6, 2).toInt(&bOk);
    if (!bOk) return QDateTime();
    const qint32 nHour = baStamp.mid(8, 2).toInt(&bOk);
    if (!bOk) return QDateTime();
    const qint32 nMinute = baStamp.mid(11, 2).toInt(&bOk);
    if (!bOk) return QDateTime();
    const qint32 nSecond = baStamp.mid(14, 2).toInt(&bOk);
    if (!bOk) return QDateTime();

    const QDate date(((nYear >= 80) ? (1900 + nYear) : (2000 + nYear)), nMonth, nDay);
    const QTime time(nHour, nMinute, nSecond);
    if (!date.isValid() || !time.isValid()) return QDateTime();

    return QDateTime(date, time, X_UTC_TZ);
}

bool XNetWareInstallFile::parseFileRecord(const QByteArray &baValue, CONTEXT *pContext)
{
    if (!pContext) return false;
    if (baValue.size() < (NWIF_REC_FIXED_PREFIX + 1 + 1 + NWIF_REC_TRAILER_SIZE)) return false;
    if (static_cast<quint8>(baValue.at(0)) != NWIF_REC_VERSION) return false;

    qint32 nOffset = 2;
    const QByteArray baStamp = baValue.mid(nOffset, NWIF_STAMP_SIZE);
    nOffset += NWIF_STAMP_SIZE;
    const QByteArray baDecimal = baValue.mid(nOffset, NWIF_DECIMAL_SIZE);
    nOffset += NWIF_DECIMAL_SIZE;
    const QByteArray baDigest = baValue.mid(nOffset, NWIF_DIGEST_SIZE);
    nOffset += NWIF_DIGEST_SIZE;

    for (qint32 i = 0; i < NWIF_DECIMAL_SIZE; i++) {
        const char cCharacter = baDecimal.at(i);
        if ((cCharacter < '0') || (cCharacter > '9')) return false;
    }
    for (qint32 i = 0; i < NWIF_DIGEST_SIZE; i++) {
        if (!nwifIsHexDigit(baDigest.at(i))) return false;
    }

    const qint64 nRecordSize = static_cast<qint64>(nwifReadUInt32(baValue, nOffset));
    nOffset += 4;
    // The second u32 is a native timestamp; it is not consistent enough across
    // the corpus to be published, so it is read past rather than trusted.
    nOffset += 4;

    bool bOk = false;
    const qint64 nDecimalSize = baDecimal.toLongLong(&bOk);
    // Both size fields describe the same member.  A container where they
    // disagree is not one of these, and guessing which one is right is exactly
    // how a truncated member gets published as a whole one.
    if (!bOk || (nDecimalSize != nRecordSize)) return false;

    const qint32 nNameLength = static_cast<quint8>(baValue.at(nOffset));
    nOffset++;
    if ((nNameLength < 1) || (nNameLength > NWIF_MAX_NAME_SIZE)) return false;
    if ((nOffset + nNameLength + 1 + NWIF_REC_TRAILER_SIZE) != baValue.size()) return false;

    const QByteArray baName = baValue.mid(nOffset, nNameLength);
    if (!isValidMemberName(baName)) return false;
    if (static_cast<quint8>(baValue.at(nOffset + nNameLength)) != 0x00U) return false;

    pContext->nRecordVersion = static_cast<quint8>(baValue.at(0));
    pContext->nRecordSubVersion = static_cast<quint8>(baValue.at(1));
    pContext->nUncompressedSize = nRecordSize;
    pContext->sFileName = QString::fromLatin1(baName);
    pContext->sDigest = QString::fromLatin1(baDigest);
    pContext->dtStamp = parseStamp(baStamp);

    return true;
}

bool XNetWareInstallFile::parseChunkChain(const QByteArray &baSource, CONTEXT *pContext)
{
    if (!pContext) return false;

    const qint64 nSourceSize = baSource.size();
    if (nSourceSize < NWIF_MIN_FILE_SIZE) return false;

    bool bHasSignature = false;
    bool bHasRecord = false;
    bool bHasPacked = false;
    QStringList listInfo;

    qint64 nOffset = 0;
    qint32 nChunkCount = 0;

    while (nOffset < nSourceSize) {
        if (++nChunkCount > NWIF_MAX_CHUNKS) return false;
        if ((nSourceSize - nOffset) < NWIF_MIN_CHUNK_SIZE) return false;

        const qint64 nChunkSize = static_cast<qint64>(nwifReadUInt32(baSource, static_cast<qint32>(nOffset)));
        if ((nChunkSize < NWIF_MIN_CHUNK_SIZE) || (nChunkSize > (nSourceSize - nOffset))) return false;

        const qint32 nKeyLength = static_cast<quint8>(baSource.at(static_cast<qint32>(nOffset) + 4));
        // nKeyLength counts one byte past the key text; the byte in that slot
        // is the first byte of the value, not a terminator.
        if ((nKeyLength < 2) || (nKeyLength > NWIF_MAX_CHUNK_KEY)) return false;
        if ((5 + (nKeyLength - 1)) >= nChunkSize) return false;

        const QByteArray baKey = baSource.mid(static_cast<qint32>(nOffset) + 5, nKeyLength - 1);
        for (qint32 i = 0; i < baKey.size(); i++) {
            if (!nwifIsKeyCharacter(static_cast<quint8>(baKey.at(i)))) return false;
        }

        const qint64 nValueOffset = nOffset + 5 + (nKeyLength - 1);
        const qint64 nValueSize = nChunkSize - 5 - (nKeyLength - 1);
        const QByteArray baValue = baSource.mid(static_cast<qint32>(nValueOffset), static_cast<qint32>(nValueSize));
        if (baValue.size() != nValueSize) return false;

        if (baKey == "NetWareFileInfo") {
            // Must be the first chunk, and must be the exact signature block.
            if (bHasSignature || (nOffset != 0)) return false;
            if ((nChunkSize != NWIF_SIGNATURE_CHUNK_SIZE) || (nKeyLength != NWIF_SIGNATURE_KEY_LENGTH)) return false;
            if (nValueSize != NWIF_SIGNATURE_VALUE_SIZE) return false;
            if (memcmp(baValue.constData(), NWIF_SIGNATURE_VALUE, NWIF_SIGNATURE_VALUE_SIZE) != 0) return false;
            bHasSignature = true;
        } else if (baKey == "NetWareFile") {
            if (bHasSignature == false) return false;
            if (bHasRecord || bHasPacked) return false;
            if (!parseFileRecord(baValue, pContext)) return false;
            bHasRecord = true;
        } else if (baKey == "PackedData") {
            if (!bHasSignature || !bHasRecord || bHasPacked) return false;
            if (nValueSize < (NWIF_PACK_HEADER_SIZE + 1)) return false;
            pContext->nFormatVersion = static_cast<quint8>(baValue.at(0));
            pContext->nMethod = static_cast<quint8>(baValue.at(1));
            if (pContext->nFormatVersion != NWIF_PACK_VERSION) return false;
            const qint64 nPackedSize = static_cast<qint64>(nwifReadUInt32(baValue, 2));
            if (nPackedSize != pContext->nUncompressedSize) return false;
            pContext->nStreamOffset = nValueOffset + NWIF_PACK_HEADER_SIZE;
            pContext->nStreamSize = nValueSize - NWIF_PACK_HEADER_SIZE;
            bHasPacked = true;
        } else {
            // Optional metadata ("VeRsIoN=", "CoPyRiGhT=").  nKeyLength stops
            // one byte short of the value, so baValue already starts at the
            // first character of the text.
            if (!bHasSignature) return false;
            QByteArray baText = baValue;
            baText.replace('\0', "");
            listInfo.append(QString::fromLatin1(baKey) + QString::fromLatin1(baText).trimmed());
        }

        nOffset += nChunkSize;
    }

    // The chain has to consume the file exactly; a short walk means a
    // truncated volume, and a long one is not this format.
    if (nOffset != nSourceSize) return false;
    if (!bHasSignature || !bHasRecord || !bHasPacked) return false;
    if ((pContext->nUncompressedSize < 0) || (pContext->nUncompressedSize > NWIF_MAX_UNCOMPRESSED_SIZE)) return false;
    if (pContext->nStreamSize <= 0) return false;

    pContext->nInputSize = nSourceSize;
    pContext->sInfo = listInfo.join(QStringLiteral("; "));

    return true;
}

bool XNetWareInstallFile::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XNetWareInstallFile> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    const qint64 nSize = getSize();
    if (!guardedThis || !guardedSource) return false;
    if ((nSize < NWIF_MIN_FILE_SIZE) || (nSize > NWIF_MAX_FILE_SIZE) || (nSize > static_cast<qint64>((std::numeric_limits<int>::max)()))) {
        return false;
    }

    const QByteArray baSource = read_array_process(0, nSize, pPdStruct);
    if (!guardedThis || !guardedSource || (static_cast<qint64>(baSource.size()) != nSize) || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    CONTEXT context;
    if (!parseChunkChain(baSource, &context)) return false;

    *pContext = context;

    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XNetWareInstallFile::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    // Detection probes a device the caller still owns.
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context;
    const bool bResult = parseContext(&context, pPdStruct);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    return bResult;
}

bool XNetWareInstallFile::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XNetWareInstallFile archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XNetWareInstallFile::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XNetWareInstallFile(pDevice);
}

QList<QString> XNetWareInstallFile::getSearchSignatures()
{
    // u32 0x23 | u8 0x10 | "NetWareFileInfo" | the fixed 15-byte value.
    return {"23000000104E65745761726546696C65496E666F0A0A1A5F5F6F550A0A1A0056557555"};
}

XBinary::FT XNetWareInstallFile::getFileType()
{
    return FT_NETWARE_PACK2;
}

XBinary::MODE XNetWareInstallFile::getMode()
{
    return MODE_DATA;
}

qint32 XNetWareInstallFile::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XNetWareInstallFile::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XNetWareInstallFile::getArch()
{
    return QString();
}

QString XNetWareInstallFile::getFileFormatExt()
{
    return QStringLiteral("_");
}

QString XNetWareInstallFile::getFileFormatExtsString()
{
    return QStringLiteral("NetWare installation packed file (*._ *.001 *.002)");
}

QString XNetWareInstallFile::getMIMEString()
{
    return QStringLiteral("application/octet-stream");
}

QString XNetWareInstallFile::getVersion()
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context;
    const bool bResult = parseContext(&context, nullptr);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    if (!bResult) return QString();

    // PackedData format version.  The method byte is member metadata, not a
    // container version, and is published through FPART_PROP_REPORTEDMETHOD.
    return QString::number(static_cast<qint32>(context.nFormatVersion));
}

qint64 XNetWareInstallFile::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    // The chunk chain covers the whole file; there is no trailer or overlay.
    return isValid(pPdStruct) ? getSize() : 0;
}

bool XNetWareInstallFile::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XNetWareInstallFile> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    // Binding only stages the source; validateAndFinalizeUnpackSource() below
    // is what makes the session usable.
    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;

    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    if (!parseContext(&pContext->context, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->context.nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
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

XBinary::ARCHIVERECORD XNetWareInstallFile::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    QPointer<XNetWareInstallFile> guardedThis(this);

    ARCHIVERECORD result = {};

    if (!operationGuard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) || (pState->nNumberOfRecords != 1)) {
        return result;
    }

    const UNPACK_CONTEXT *pContext = static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    if (pContext->context.sFileName.isEmpty() || (pContext->context.nStreamSize <= 0)) return result;

    result.nStreamOffset = pContext->context.nStreamOffset;
    result.nStreamSize = pContext->context.nStreamSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->context.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->context.nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->context.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_NETWARE_PACK);

    QString sMethod = QString("NetWare PackedData v%1 method 0x%2")
                          .arg(static_cast<qint32>(pContext->context.nFormatVersion))
                          .arg(static_cast<qint32>(pContext->context.nMethod), 2, 16, QChar('0'));
    if (pContext->context.nMethod == NWIF_PACK_METHOD_LZH) {
        sMethod += QStringLiteral(" (LZ77+Huffman)");
    }
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, sMethod);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    if (pContext->context.dtStamp.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, pContext->context.dtStamp);
    }
    if (!pContext->context.sInfo.isEmpty()) {
        result.mapProperties.insert(FPART_PROP_INFO, pContext->context.sInfo);
    }

    return result;
}

bool XNetWareInstallFile::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XNetWareInstallFile> guardedThis(this);

    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    // Advance past the last record before reporting.  Guarding on
    // nNumberOfRecords - 1 here leaves the caller's cursor pinned to record 0
    // and the listing comes back empty.
    ++pState->nCurrentIndex;
    pState->nCurrentOffset = (pState->nCurrentIndex >= pState->nNumberOfRecords) ? pState->nTotalSize : 0;

    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XNetWareInstallFile::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}

QList<XBinary::FPART_PROP> XNetWareInstallFile::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD,
            FPART_PROP_REPORTEDMETHOD, FPART_PROP_ISFOLDER,     FPART_PROP_DATETIME,         FPART_PROP_INFO};
}
