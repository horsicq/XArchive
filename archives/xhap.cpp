/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xhap.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 HAP_HEADER_SIZE = 15;
const qint64 HAP_ENTRY_SIZE = 40;
const qint64 HAP_NAME_OFFSET = 26;
const qint64 HAP_NAME_SIZE = 13;
const quint8 HAP_METHOD_STORED = 0x15;
const quint8 HAP_METHOD_PPM = 0x16;
// No real archive holds a million members; the bound only stops a corrupt chain
// from spinning.
const qint32 HAP_MAX_ENTRIES = 1000000;

const uchar HAP_ARCHIVE_MAGIC[4] = {0x91, 0x33, 0x48, 0x46};
const uchar HAP_ENTRY_MAGIC[4] = {0x8e, 0x68, 0x4a, 0x57};

bool hapRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XHAP::XHAP(QIODevice *pDevice) : XArchive(pDevice)
{
}

XHAP::~XHAP()
{
}

// Exactly the checks the reference extractor makes on a member header, plus the
// method byte: the four-byte signature, a non-negative compressed size, the
// zero flag byte at offset 16 and a non-negative uncompressed size.
bool XHAP::isValidEntryHeader(const uchar *pHeader)
{
    for (qint32 i = 0; i < 4; i++) {
        if (pHeader[i] != HAP_ENTRY_MAGIC[i]) return false;
    }
    if (pHeader[16] != 0) return false;
    if (static_cast<qint32>(qFromLittleEndian<quint32>(pHeader + 4)) < 0) return false;
    if (static_cast<qint32>(qFromLittleEndian<quint32>(pHeader + 22)) < 0) return false;
    const quint8 nMethod = pHeader[39];
    if ((nMethod != HAP_METHOD_STORED) && (nMethod != HAP_METHOD_PPM)) return false;
    return true;
}

// The name field is NUL padded.  Bytes that are legal in the field but not in a
// file name are escaped as %XX: escaping is reversible and cannot collapse two
// distinct members onto one output file.
QString XHAP::rawNameToString(const char *pRawName, qint32 nIndex)
{
    qint32 nLength = 0;
    while ((nLength < static_cast<qint32>(HAP_NAME_SIZE) - 1) && (pRawName[nLength] != '\0')) nLength++;
    while ((nLength > 0) && (pRawName[nLength - 1] == ' ')) nLength--;

    QString sResult;
    for (qint32 i = 0; i < nLength; i++) {
        const quint8 nCharacter = static_cast<quint8>(pRawName[i]);
        const bool bSafe = (nCharacter > 0x20) && (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != '/') && (nCharacter != '\\') &&
                           (nCharacter != ':') && (nCharacter != '*') && (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') &&
                           (nCharacter != '>') && (nCharacter != '|');
        if (bSafe) {
            sResult.append(QLatin1Char(static_cast<char>(nCharacter)));
        } else {
            QString sHex = QString::number(nCharacter, 16).toUpper();
            if (sHex.size() < 2) sHex.prepend(QLatin1Char('0'));
            sResult.append(QLatin1Char('%'));
            sResult.append(sHex);
        }
    }
    if (sResult.isEmpty()) sResult = QStringLiteral("record%1").arg(nIndex);
    return sResult;
}

XBinary::HANDLE_METHOD XHAP::methodToHandleMethod(quint8 nMethod)
{
    return (nMethod == HAP_METHOD_PPM) ? HANDLE_METHOD_HAP : HANDLE_METHOD_STORE;
}

bool XHAP::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XHAP> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < HAP_HEADER_SIZE + HAP_ENTRY_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, HAP_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != HAP_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    for (qint32 i = 0; i < 4; i++) {
        if (pHeader[i] != HAP_ARCHIVE_MAGIC[i]) return false;
    }
    // The eleven bytes behind the signature are all zero in a real archive; the
    // reference detector checks every one of them, and so must this class - the
    // signature alone is only four bytes.
    for (qint32 i = 4; i < static_cast<qint32>(HAP_HEADER_SIZE); i++) {
        if (pHeader[i] != 0) return false;
    }

    qint64 nPosition = HAP_HEADER_SIZE;
    while (isPdStructNotCanceled(pPdStruct)) {
        if (nPosition >= context.nInputSize) break;
        if (!hapRangeWithin(context.nInputSize, nPosition, HAP_ENTRY_SIZE)) return false;
        const QByteArray baEntry = read_array_process(nPosition, HAP_ENTRY_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baEntry.size() != HAP_ENTRY_SIZE)) return false;
        const uchar *pEntry = reinterpret_cast<const uchar *>(baEntry.constData());
        if (!isValidEntryHeader(pEntry)) return false;

        MEMBER member = {};
        member.nRecordOffset = nPosition;
        member.nDataOffset = nPosition + HAP_ENTRY_SIZE;
        member.nCompressedSize = static_cast<qint64>(static_cast<qint32>(qFromLittleEndian<quint32>(pEntry + 4)));
        member.nUncompressedSize = static_cast<qint64>(static_cast<qint32>(qFromLittleEndian<quint32>(pEntry + 22)));
        member.nCrc = qFromLittleEndian<quint32>(pEntry + 8);
        member.nTime = qFromLittleEndian<quint16>(pEntry + 18);
        member.nDate = qFromLittleEndian<quint16>(pEntry + 20);
        member.nMethod = pEntry[39];
        member.sFileName = rawNameToString(baEntry.constData() + HAP_NAME_OFFSET, context.listMembers.size());
        if (!hapRangeWithin(context.nInputSize, member.nDataOffset, member.nCompressedSize)) return false;
        // A stored member is a verbatim copy, so the two sizes must agree; the
        // reference extractor refuses it otherwise.
        if ((member.nMethod == HAP_METHOD_STORED) && (member.nCompressedSize != member.nUncompressedSize)) return false;

        context.listMembers.append(member);
        if (context.listMembers.size() > HAP_MAX_ENTRIES) return false;
        nPosition = member.nDataOffset + member.nCompressedSize;
        if (nPosition <= member.nRecordOffset) return false;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = nPosition;
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XHAP::isValid(PDSTRUCT *pPdStruct)
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

bool XHAP::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XHAP archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XHAP::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XHAP(pDevice);
}

QList<QString> XHAP::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("91334846"));
    return listResult;
}

XBinary::FT XHAP::getFileType()
{
    return FT_HAP;
}

XBinary::MODE XHAP::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XHAP::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XHAP::getArch()
{
    return QString();
}

QString XHAP::getFileFormatExt()
{
    return QStringLiteral("hap");
}

QString XHAP::getFileFormatExtsString()
{
    return QStringLiteral("HAP archive (*.hap)");
}

QString XHAP::getMIMEString()
{
    return QStringLiteral("application/x-hap");
}

QString XHAP::getVersion()
{
    return QString();
}

qint64 XHAP::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XHAP::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XHAP::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XHAP::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XHAP::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = HAP_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      (member.nMethod == HAP_METHOD_PPM) ? QStringLiteral("HAP PPM") : QStringLiteral("Stored"));
            const QDateTime dtMTime = dosDateTimeToQDateTime(member.nDate, member.nTime);
            if (dtMTime.isValid()) part.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nRecordOffset;
            part.nFileSize = HAP_ENTRY_SIZE + member.nCompressedSize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XHAP::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XHAP::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XHAP> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("HAP archive"));
    pState->nCurrentOffset = pContext->listMembers.first().nRecordOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
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

XBinary::ARCHIVERECORD XHAP::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nRecordOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, (member.nMethod == HAP_METHOD_PPM) ? QStringLiteral("HAP PPM") : QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // The header has a checksum slot at offset 8, but it is zero in all 236
    // members of the reference corpus, so which algorithm fills it could not be
    // determined; it is deliberately neither reported nor enforced rather than
    // guessed at.
    const QDateTime dtMTime = dosDateTimeToQDateTime(member.nDate, member.nTime);
    if (dtMTime.isValid()) result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
    return result;
}

bool XHAP::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nRecordOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XHAP::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
