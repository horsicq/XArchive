/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xquarterdeckqp.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
// File header: "QP" | u16 member count | u32 index size | u16 version |
// 6 zero bytes.  Every one of the 232 known samples has version 2 and an
// all-zero tail.
const qint64 QP_HEADER_SIZE = 16;
const qint64 QP_INDEX_ENTRY_SIZE = 16;
const qint32 QP_INDEX_NAME_SIZE = 12;
const quint16 QP_VERSION_2 = 2U;

// Record header: "QD" | u16 record kind.  Kind 0 is a member, kind 1 is an
// install-destination directory.
const qint64 QP_RECORD_MAGIC_SIZE = 4;
const quint16 QP_RECORD_FILE = 0U;
const quint16 QP_RECORD_PATH = 1U;

// Member header: the four magic/kind bytes plus u32 packed size, u16
// sequence, u32 CRC32, u8 attributes, u16 DOS time, u16 DOS date, u32
// unpacked size and a fixed 13-byte name buffer.  4+4+2+4+1+2+2+4+13 = 36
// accounts for every byte before the payload.
const qint64 QP_FILE_HEADER_SIZE = 36;
const qint32 QP_FILE_NAME_OFFSET = 23;
// 8.3 plus the terminator.  A name that fills all twelve characters
// ("PRINTMAN.EXE") puts its NUL in the thirteenth byte, so the field must be
// read at its full width - reading twelve would reject the record.
const qint32 QP_FILE_NAME_FIELD = 13;

// Directory record: the four magic/kind bytes plus u32 name-blob length,
// u16 sequence and u32 reserved, then the NUL-terminated path.
const qint64 QP_PATH_HEADER_SIZE = 14;
const qint32 QP_PATH_NAME_MAX = 256;

const qint32 QP_MAX_MEMBERS = 65535;  // the count field is a u16
const qint64 QP_MAX_UNCOMPRESSED_SIZE = 0x10000000;  // 256 MB sanity cap

// The payload is a complete PKWARE DCL Implode stream and carries its own
// prelude: literal mode (0 or 1) then dictionary-size bits (4..6).  Every
// corpus member uses 0/6.
const qint64 QP_MIN_PAYLOAD_SIZE = 3;
const quint8 QP_DCL_MAX_LITERAL_MODE = 1U;
const quint8 QP_DCL_MIN_DICT_BITS = 4U;
const quint8 QP_DCL_MAX_DICT_BITS = 6U;

bool qpRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

// Reads a fixed-width NUL-padded field: the name is everything up to the
// first terminator, and the bytes after it are stale buffer content that
// must be ignored (index entries carry visible garbage there).
QByteArray qpFixedName(const QByteArray &baField)
{
    const qint32 nIndex = baField.indexOf('\0');
    return (nIndex < 0) ? baField : baField.left(nIndex);
}

bool qpIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty() || baName.size() > 12) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if (nCharacter < 0x20 || nCharacter > 0x7e) return false;
        if ((c == '/') || (c == '\\') || (c == ':')) return false;
    }
    return true;
}

bool qpIsValidPath(const QByteArray &baPath)
{
    for (char c : baPath) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if (nCharacter < 0x20 || nCharacter > 0x7e) return false;
    }
    return true;
}
}  // namespace

XQuarterdeckQP::XQuarterdeckQP(QIODevice *pDevice) : XArchive(pDevice)
{
}

XQuarterdeckQP::~XQuarterdeckQP()
{
}

bool XQuarterdeckQP::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XQuarterdeckQP> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize <
        QP_HEADER_SIZE + QP_INDEX_ENTRY_SIZE + QP_FILE_HEADER_SIZE +
            QP_MIN_PAYLOAD_SIZE) {
        return false;
    }

    const QByteArray baHeader = read_array_process(0, QP_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || baHeader.size() != QP_HEADER_SIZE) {
        return false;
    }
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    if (std::memcmp(pHeader, "QP", 2) != 0) return false;

    const qint32 nCount = static_cast<qint32>(qFromLittleEndian<quint16>(pHeader + 2));
    const qint64 nIndexSize =
        static_cast<qint64>(qFromLittleEndian<quint32>(pHeader + 4));
    const quint16 nVersion = qFromLittleEndian<quint16>(pHeader + 8);
    if (nVersion != QP_VERSION_2) return false;
    // The six trailing header bytes are zero on every known sample; keeping
    // them in the gate is what stops a random "QP" pair from matching.
    for (qint32 i = 10; i < QP_HEADER_SIZE; ++i) {
        if (pHeader[i] != 0) return false;
    }
    if (nCount < 1 || nCount > QP_MAX_MEMBERS) return false;
    // Fixed-stride index: the size field is fully determined by the count.
    if (nIndexSize != static_cast<qint64>(nCount) * QP_INDEX_ENTRY_SIZE) {
        return false;
    }
    if (!qpRangeWithin(context.nInputSize, QP_HEADER_SIZE, nIndexSize)) {
        return false;
    }

    context.nVersion = static_cast<qint32>(nVersion);
    context.nIndexOffset = QP_HEADER_SIZE;
    context.nIndexSize = nIndexSize;

    const QByteArray baIndex =
        read_array_process(QP_HEADER_SIZE, nIndexSize, pPdStruct);
    if (!guardedThis || !guardedSource || baIndex.size() != nIndexSize) {
        return false;
    }

    QString sCurrentPath;
    qint64 nOffset = QP_HEADER_SIZE + nIndexSize;

    while (isPdStructNotCanceled(pPdStruct)) {
        if (nOffset == context.nInputSize) break;
        if (!qpRangeWithin(context.nInputSize, nOffset, QP_RECORD_MAGIC_SIZE)) {
            return false;
        }
        const QByteArray baKind =
            read_array_process(nOffset, QP_RECORD_MAGIC_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baKind.size() != QP_RECORD_MAGIC_SIZE) {
            return false;
        }
        if (std::memcmp(baKind.constData(), "QD", 2) != 0) return false;
        const quint16 nKind = qFromLittleEndian<quint16>(
            reinterpret_cast<const uchar *>(baKind.constData()) + 2);

        if (nKind == QP_RECORD_PATH) {
            if (!qpRangeWithin(context.nInputSize, nOffset,
                               QP_PATH_HEADER_SIZE)) {
                return false;
            }
            const QByteArray baPathHeader =
                read_array_process(nOffset, QP_PATH_HEADER_SIZE, pPdStruct);
            if (!guardedThis || !guardedSource ||
                baPathHeader.size() != QP_PATH_HEADER_SIZE) {
                return false;
            }
            const qint64 nNameSize = static_cast<qint64>(qFromLittleEndian<quint32>(
                reinterpret_cast<const uchar *>(baPathHeader.constData()) + 4));
            if (nNameSize < 1 || nNameSize > QP_PATH_NAME_MAX) return false;
            if (!qpRangeWithin(context.nInputSize,
                               nOffset + QP_PATH_HEADER_SIZE, nNameSize)) {
                return false;
            }
            const QByteArray baPath = read_array_process(
                nOffset + QP_PATH_HEADER_SIZE, nNameSize, pPdStruct);
            if (!guardedThis || !guardedSource || baPath.size() != nNameSize) {
                return false;
            }
            const QByteArray baPathName = qpFixedName(baPath);
            if (!qpIsValidPath(baPathName)) return false;
            sCurrentPath = QString::fromLatin1(baPathName)
                               .replace(QLatin1Char('\\'), QLatin1Char('/'));
            nOffset += QP_PATH_HEADER_SIZE + nNameSize;
            continue;
        }

        if (nKind != QP_RECORD_FILE) return false;

        if (context.listMembers.size() >= nCount) return false;
        if (!qpRangeWithin(context.nInputSize, nOffset, QP_FILE_HEADER_SIZE)) {
            return false;
        }
        const QByteArray baRecord =
            read_array_process(nOffset, QP_FILE_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baRecord.size() != QP_FILE_HEADER_SIZE) {
            return false;
        }
        const uchar *pRecord =
            reinterpret_cast<const uchar *>(baRecord.constData());

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nOffset + QP_FILE_HEADER_SIZE;
        member.nCompressedSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(pRecord + 4));
        member.nSequence = qFromLittleEndian<quint16>(pRecord + 8);
        member.nCRC32 = qFromLittleEndian<quint32>(pRecord + 10);
        member.nAttributes = pRecord[14];
        // The time/date pair straddles an odd offset, so it must be read
        // byte-wise rather than through an aligned load.
        member.nDosTime = qFromLittleEndian<quint16>(pRecord + 15);
        member.nDosDate = qFromLittleEndian<quint16>(pRecord + 17);
        member.nUncompressedSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(pRecord + 19));
        member.sPath = sCurrentPath;

        const QByteArray baName = qpFixedName(
            baRecord.mid(QP_FILE_NAME_OFFSET, QP_FILE_NAME_FIELD));
        if (!qpIsValidName(baName)) return false;
        member.sFileName = QString::fromLatin1(baName);

        if (member.nCompressedSize < QP_MIN_PAYLOAD_SIZE) return false;
        if (member.nUncompressedSize > QP_MAX_UNCOMPRESSED_SIZE) return false;
        if (!qpRangeWithin(context.nInputSize, member.nDataOffset,
                           member.nCompressedSize)) {
            return false;
        }

        // Cross-check the DCL prelude.  The container itself has no method
        // field, so this is the only place the payload encoding is asserted.
        const QByteArray baPrelude =
            read_array_process(member.nDataOffset, 2, pPdStruct);
        if (!guardedThis || !guardedSource || baPrelude.size() != 2) {
            return false;
        }
        if (static_cast<quint8>(baPrelude.at(0)) > QP_DCL_MAX_LITERAL_MODE) {
            return false;
        }
        const quint8 nDictBits = static_cast<quint8>(baPrelude.at(1));
        if (nDictBits < QP_DCL_MIN_DICT_BITS ||
            nDictBits > QP_DCL_MAX_DICT_BITS) {
            return false;
        }

        // Every index entry must name the record it points at, in order.
        const qint64 nIndexEntry =
            static_cast<qint64>(context.listMembers.size()) *
            QP_INDEX_ENTRY_SIZE;
        const qint64 nIndexedOffset =
            static_cast<qint64>(qFromLittleEndian<quint32>(
                reinterpret_cast<const uchar *>(baIndex.constData()) +
                nIndexEntry));
        if (nIndexedOffset != member.nHeaderOffset) return false;
        const QByteArray baIndexName = qpFixedName(
            baIndex.mid(static_cast<qint32>(nIndexEntry) + 4,
                        QP_INDEX_NAME_SIZE));
        if (baIndexName != baName) return false;

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;
    }

    if (!isPdStructNotCanceled(pPdStruct)) return false;
    // The chain has no terminator: it ends by landing exactly on EOF with
    // every indexed member accounted for.
    if (nOffset != context.nInputSize) return false;
    if (context.listMembers.size() != nCount) return false;

    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return guardedThis && guardedSource;
}

bool XQuarterdeckQP::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XQuarterdeckQP::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XQuarterdeckQP archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XQuarterdeckQP::createInstance(QIODevice *pDevice, bool bIsImage,
                                        XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XQuarterdeckQP(pDevice);
}

QList<QString> XQuarterdeckQP::getSearchSignatures()
{
    // "QP" alone is far too weak; pin the version word and the six zero
    // bytes that follow it.
    return {QStringLiteral("'QP'............0200000000000000")};
}

XBinary::FT XQuarterdeckQP::getFileType()
{
    return FT_QDECK_QIP;
}

XBinary::MODE XQuarterdeckQP::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XQuarterdeckQP::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XQuarterdeckQP::getArch()
{
    return QString();
}

QString XQuarterdeckQP::getFileFormatExt()
{
    return QStringLiteral("qip");
}

QString XQuarterdeckQP::getFileFormatExtsString()
{
    return QStringLiteral("Quarterdeck QIP package (*.qip *.qif)");
}

QString XQuarterdeckQP::getMIMEString()
{
    return QStringLiteral("application/x-quarterdeck-qip");
}

QString XQuarterdeckQP::getVersion()
{
    // parseContext() rejects anything but 2 in the version word, so this is
    // a constant here and needs no device access (which would have to
    // snapshot and restore the caller's cursor).
    return QStringLiteral("2");
}

qint64 XQuarterdeckQP::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XQuarterdeckQP::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XQuarterdeckQP::getMemoryMap(MAPMODE mapMode,
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

bool XQuarterdeckQP::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XQuarterdeckQP::getFileParts(quint32 nFileParts,
                                                   qint32 nLimit,
                                                   PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = QP_HEADER_SIZE + context.nIndexSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = QP_FILE_HEADER_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME,
                                      member.sFileName);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_PKWARE_DCL_IMPLODE);
            part.mapProperties.insert(
                FPART_PROP_REPORTEDMETHOD,
                QStringLiteral("PKWARE DCL Implode"));
            // Verified against the corpus: the stored value is a plain
            // CRC-32 (EDB88320) over the UNPACKED member.
            part.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
            part.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                      CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
            part.mapProperties.insert(
                FPART_PROP_DATETIME,
                dosDateTimeToQDateTime(member.nDosDate, member.nDosTime));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = QP_FILE_HEADER_SIZE + member.nCompressedSize;
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
    if ((nFileParts & FILEPART_OVERLAY) &&
        context.nArchiveSize < context.nInputSize &&
        canAppendPart(nLimit, result.size())) {
        // parseContext() only accepts a chain that lands on EOF, so this
        // cannot fire today; it is kept so the part list stays correct if
        // the acceptance rule is ever relaxed.
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

QMap<XBinary::UNPACK_PROP, QVariant> XQuarterdeckQP::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XQuarterdeckQP::initUnpack(UNPACK_STATE *pState,
                                const QMap<UNPACK_PROP, QVariant> &mapProperties,
                                PDSTRUCT *pPdStruct)
{
    QPointer<XQuarterdeckQP> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource ||
        pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("Quarterdeck QIP package; PKWARE DCL Implode members"));
    pState->nCurrentOffset = pContext->listMembers.first().nHeaderOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized =
        guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
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

XBinary::ARCHIVERECORD XQuarterdeckQP::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext || pState->nCurrentIndex >= pContext->listMembers.size()) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                HANDLE_METHOD_PKWARE_DCL_IMPLODE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("PKWARE DCL Implode"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
    result.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    result.mapProperties.insert(
        FPART_PROP_DATETIME,
        dosDateTimeToQDateTime(member.nDosDate, member.nDosTime));
    result.mapProperties.insert(FPART_PROP_FILEMODE,
                                static_cast<quint32>(member.nAttributes));
    // The preceding directory record is the install destination, not part
    // of the member name; it is published for information only so that the
    // extracted layout matches the reference extractor's flat output.
    if (!member.sPath.isEmpty()) {
        result.mapProperties.insert(FPART_PROP_PREFIX, member.sPath);
    }
    return result;
}

bool XQuarterdeckQP::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pState->nCurrentIndex >= pContext->listMembers.size()) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XQuarterdeckQP::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
