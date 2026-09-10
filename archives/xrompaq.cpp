/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xrompaq.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
// Header layout, recovered from the handler (detector 0x0064E2A0, worker
// 0x0064E610).  Every offset below is checked by that detector.
const qint64 ROMPAQ_HEADER_SIZE = 0x48;

const qint64 ROMPAQ_OFFSET_SIZE = 0x00;       // int32, whole ROM image size
const qint64 ROMPAQ_OFFSET_CHECKSUM = 0x04;   // uint32 over the packed bytes
const qint64 ROMPAQ_OFFSET_ROMID = 0x08;      // uint16, low 12 bits = extension
const qint64 ROMPAQ_OFFSET_VERSION = 0x0a;    // uint16, 0x0100 or 0x0101
const qint64 ROMPAQ_OFFSET_NAME = 0x0c;       // 7 alphanumeric bytes
const qint64 ROMPAQ_NAME_SIZE = 7;
const qint64 ROMPAQ_OFFSET_NAME_TERMINATOR = 0x13;  // must be NUL
const qint64 ROMPAQ_OFFSET_DATE = 0x14;       // ASCII date, NUL padded
const qint64 ROMPAQ_OFFSET_DATE_END = 0x3b;   // must be NUL
const qint64 ROMPAQ_OFFSET_METHOD = 0x3c;     // 1 = stored, 2 = PKWARE DCL
const qint64 ROMPAQ_OFFSET_PARTCOUNT = 0x3d;  // uint16, 0 = single part
const qint64 ROMPAQ_OFFSET_PARTSIZE = 0x3f;   // int32, packed bytes of part 1
const qint64 ROMPAQ_OFFSET_RESERVED1 = 0x43;  // uint32, must be 0
const qint64 ROMPAQ_OFFSET_RESERVED2 = 0x47;  // uint8, must be 0

const quint16 ROMPAQ_VERSION_100 = 0x0100;
const quint16 ROMPAQ_VERSION_101 = 0x0101;

const quint8 ROMPAQ_METHOD_STORED = 1;
const quint8 ROMPAQ_METHOD_IMPLODE = 2;

// The ROM image never exceeds a handful of megabytes in practice; the ceiling
// only bounds what a corrupt size field may ask the decode chain to allocate.
const qint64 ROMPAQ_MAX_IMAGE_SIZE = 0x4000000;

bool rompaqIsNameCharacter(quint8 nCharacter)
{
    // Exactly the class the reference implementation tests through its 0x0064E290 bitmap: 0-9 A-Z a-z.
    return ((nCharacter >= '0') && (nCharacter <= '9')) ||
           ((nCharacter >= 'A') && (nCharacter <= 'Z')) ||
           ((nCharacter >= 'a') && (nCharacter <= 'z'));
}
}  // namespace

XRomPaq::XRomPaq(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XRomPaq::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

bool XRomPaq::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext) return false;

    QPointer<XRomPaq> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential() ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (!guardedThis) return false;
    // A single-part image needs the header plus the two-byte probe the worker
    // performs before the PKWARE DCL stream.
    if (context.nInputSize < ROMPAQ_HEADER_SIZE + 2) return false;

    const QByteArray baHeader = read_array(0, ROMPAQ_HEADER_SIZE + 2);
    if (!guardedThis || !guardedSource ||
        (baHeader.size() != (ROMPAQ_HEADER_SIZE + 2))) {
        return false;
    }
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    const qint32 nImageSize = static_cast<qint32>(
        qFromLittleEndian<quint32>(pHeader + ROMPAQ_OFFSET_SIZE));
    if ((nImageSize <= 0) || (nImageSize > ROMPAQ_MAX_IMAGE_SIZE)) return false;

    context.nVersion = qFromLittleEndian<quint16>(pHeader + ROMPAQ_OFFSET_VERSION);
    if ((context.nVersion != ROMPAQ_VERSION_100) &&
        (context.nVersion != ROMPAQ_VERSION_101)) {
        return false;
    }

    context.nMethod = pHeader[ROMPAQ_OFFSET_METHOD];
    if ((context.nMethod != ROMPAQ_METHOD_STORED) &&
        (context.nMethod != ROMPAQ_METHOD_IMPLODE)) {
        return false;
    }

    if (pHeader[ROMPAQ_OFFSET_NAME_TERMINATOR] != 0) return false;
    if (pHeader[ROMPAQ_OFFSET_DATE_END] != 0) return false;
    if (pHeader[ROMPAQ_OFFSET_RESERVED2] != 0) return false;
    if (qFromLittleEndian<quint32>(pHeader + ROMPAQ_OFFSET_RESERVED1) != 0) {
        return false;
    }

    // The name field is fixed width: all seven bytes carry name characters and
    // the terminator lives outside it, so the whole field is read.
    QString sName;
    for (qint64 i = 0; i < ROMPAQ_NAME_SIZE; ++i) {
        const quint8 nCharacter = pHeader[ROMPAQ_OFFSET_NAME + i];
        if (!rompaqIsNameCharacter(nCharacter)) return false;
        sName.append(QChar::fromLatin1(static_cast<char>(nCharacter)));
    }
    context.sName = sName;

    context.nPartCount = qFromLittleEndian<quint16>(pHeader + ROMPAQ_OFFSET_PARTCOUNT);
    const qint32 nPartSize = static_cast<qint32>(
        qFromLittleEndian<quint32>(pHeader + ROMPAQ_OFFSET_PARTSIZE));
    // Part count and part size are set together or not at all: a chained image
    // declares both, a single-part image declares neither.
    if (context.nPartCount != 0) {
        if (nPartSize <= 0) return false;
    } else if (nPartSize != 0) {
        return false;
    }

    context.nHeaderChecksum =
        qFromLittleEndian<quint32>(pHeader + ROMPAQ_OFFSET_CHECKSUM);
    context.nRomId = qFromLittleEndian<quint16>(pHeader + ROMPAQ_OFFSET_ROMID);
    context.nUncompressedSize = nImageSize;

    QString sDate;
    for (qint64 i = ROMPAQ_OFFSET_DATE; i < ROMPAQ_OFFSET_DATE_END; ++i) {
        const quint8 nCharacter = pHeader[i];
        if (nCharacter == 0) break;
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) {
            sDate.clear();
            break;
        }
        sDate.append(QChar::fromLatin1(static_cast<char>(nCharacter)));
    }
    context.sDate = sDate;

    if (context.nPartCount != 0) {
        // The chain restarts at offset 0: the container header IS the first
        // bank header, so the whole file is handed to the chain decoder.
        if (static_cast<qint64>(nPartSize) >
            context.nInputSize - (ROMPAQ_HEADER_SIZE + 2)) {
            return false;
        }
        context.nStreamOffset = 0;
        context.nStreamSize = context.nInputSize;
        context.nHandleMethod = HANDLE_METHOD_ROMPAQ;
    } else if (context.nMethod == ROMPAQ_METHOD_IMPLODE) {
        qint64 nDataOffset = ROMPAQ_HEADER_SIZE;
        // A zero word here is padding; a non-zero word is already the PKWARE
        // DCL selector pair and must stay in the stream.
        if (qFromLittleEndian<quint16>(pHeader + ROMPAQ_HEADER_SIZE) == 0) {
            nDataOffset += 2;
        }
        if (context.nInputSize - nDataOffset < 3) return false;

        const QByteArray baSelector = read_array(nDataOffset, 2);
        if (!guardedThis || !guardedSource || (baSelector.size() != 2)) {
            return false;
        }
        const quint8 nLiteralMode = static_cast<quint8>(baSelector.at(0));
        const quint8 nDictionaryBits = static_cast<quint8>(baSelector.at(1));
        if ((nLiteralMode > 1) || (nDictionaryBits < 4) || (nDictionaryBits > 6)) {
            return false;
        }

        context.nStreamOffset = nDataOffset;
        context.nStreamSize = context.nInputSize - nDataOffset;
        context.nHandleMethod = HANDLE_METHOD_PKWARE_DCL_IMPLODE;
    } else {
        if (context.nInputSize - ROMPAQ_HEADER_SIZE < context.nUncompressedSize) {
            return false;
        }
        context.nStreamOffset = ROMPAQ_HEADER_SIZE;
        context.nStreamSize = context.nUncompressedSize;
        context.nHandleMethod = HANDLE_METHOD_STORE;
    }

    *pContext = context;

    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XRomPaq::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XRomPaq::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRomPaq archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XRomPaq::createInstance(QIODevice *pDevice, bool bIsImage,
                                 XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XRomPaq(pDevice);
}

QList<QString> XRomPaq::getSearchSignatures()
{
    // No magic exists: the only fixed bytes are a version word two thirds of
    // the way into a field of otherwise free-form data.  Publishing that as a
    // carving signature would match constantly, so the format is identified by
    // isValid() alone.
    return QList<QString>();
}

XBinary::FT XRomPaq::getFileType()
{
    return FT_ROMPAQ;
}

XBinary::MODE XRomPaq::getMode()
{
    return MODE_DATA;
}

qint32 XRomPaq::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

XBinary::ENDIAN XRomPaq::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XRomPaq::getArch()
{
    return QString();
}

QString XRomPaq::getFileFormatExt()
{
    // Compaq names the distributed file after the ROM id: the low twelve bits
    // of the id word printed as three hex digits are literally the extension
    // (id 0x0B39 -> CPQ15010.B39, id 0x3184 -> HC2490A3.184).
    QPointer<XRomPaq> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential() ||
        (guardedSource->size() < ROMPAQ_HEADER_SIZE)) {
        return QStringLiteral("rompaq");
    }
    const qint64 nSavedPosition = guardedSource->pos();
    const QByteArray baHeader = read_array(ROMPAQ_OFFSET_ROMID, 2);
    if (guardedSource && (nSavedPosition >= 0)) guardedSource->seek(nSavedPosition);
    if (!guardedThis || (baHeader.size() != 2)) return QStringLiteral("rompaq");

    const quint16 nRomId =
        qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baHeader.constData()));

    return QString("%1").arg(nRomId & 0x0fffU, 3, 16, QChar('0')).toUpper();
}

QString XRomPaq::getFileFormatExtsString()
{
    return QStringLiteral("Compaq ROMPAQ firmware image (*.*)");
}

QString XRomPaq::getMIMEString()
{
    return QStringLiteral("application/x-rompaq");
}

QString XRomPaq::getVersion()
{
    QPointer<XRomPaq> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential() ||
        (guardedSource->size() < ROMPAQ_HEADER_SIZE)) {
        return QString();
    }
    const qint64 nSavedPosition = guardedSource->pos();
    const QByteArray baHeader = read_array(ROMPAQ_OFFSET_VERSION, 2);
    if (guardedSource && (nSavedPosition >= 0)) guardedSource->seek(nSavedPosition);
    if (!guardedThis || (baHeader.size() != 2)) return QString();

    const quint16 nVersion =
        qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baHeader.constData()));
    if ((nVersion != ROMPAQ_VERSION_100) && (nVersion != ROMPAQ_VERSION_101)) {
        return QString();
    }

    return QString("%1.%2").arg(nVersion >> 8).arg(nVersion & 0xffU);
}

qint64 XRomPaq::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return 0;
    return context.nInputSize;
}

QList<XBinary::MAPMODE> XRomPaq::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XRomPaq::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY,
                             pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QList<XBinary::FPART> XRomPaq::getFileParts(quint32 nFileParts, qint32 nLimit,
                                            PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = ROMPAQ_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nStreamOffset;
        part.nFileSize = context.nStreamSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nStreamSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                  context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, context.nHandleMethod);
        part.mapProperties.insert(
            FPART_PROP_REPORTEDMETHOD,
            (context.nMethod == ROMPAQ_METHOD_STORED)
                ? QStringLiteral("Stored")
                : (context.nPartCount ? QStringLiteral("PKWARE DCL implode banks")
                                      : QStringLiteral("PKWARE DCL implode")));
        result.append(part);
    }

    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = ROMPAQ_HEADER_SIZE;
        part.nFileSize = context.nInputSize - ROMPAQ_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Compressed Data");
        result.append(part);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }

    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XRomPaq::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XRomPaq::initUnpack(UNPACK_STATE *pState,
                         const QMap<UNPACK_PROP, QVariant> &mapProperties,
                         PDSTRUCT *pPdStruct)
{
    QPointer<XRomPaq> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("Compaq ROMPAQ image '%1', ROM id %2, %3")
            .arg(pContext->sName)
            .arg(QString("%1").arg(pContext->nRomId, 4, 16, QChar('0')).toUpper())
            .arg(pContext->nPartCount
                     ? tr("%1 banks").arg(pContext->nPartCount)
                     : tr("single part")));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
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

XBinary::ARCHIVERECORD XRomPaq::infoCurrent(UNPACK_STATE *pState,
                                            PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nStreamOffset;
    result.nStreamSize = pContext->nStreamSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sName);
    result.mapProperties.insert(FPART_PROP_STREAMOFFSET, pContext->nStreamOffset);
    result.mapProperties.insert(FPART_PROP_STREAMSIZE, pContext->nStreamSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, pContext->nHandleMethod);
    result.mapProperties.insert(
        FPART_PROP_REPORTEDMETHOD,
        (pContext->nMethod == ROMPAQ_METHOD_STORED)
            ? QStringLiteral("Stored")
            : (pContext->nPartCount ? QStringLiteral("PKWARE DCL implode banks")
                                    : QStringLiteral("PKWARE DCL implode")));
    result.mapProperties.insert(FPART_PROP_HEADER_OFFSET, static_cast<qint64>(0));
    result.mapProperties.insert(FPART_PROP_HEADER_SIZE, ROMPAQ_HEADER_SIZE);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (!pContext->sDate.isEmpty()) {
        result.mapProperties.insert(FPART_PROP_INFO,
                                    tr("ROM date %1").arg(pContext->sDate));
    }

    return result;
}

bool XRomPaq::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        return true;
    }
    pState->nCurrentOffset = pContext->nInputSize;

    return false;
}

bool XRomPaq::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
