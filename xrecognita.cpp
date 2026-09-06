/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xrecognita.h"

#include <QDateTime>
#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 RECOGNITA_HEADER_SIZE = 25;
const qint64 RECOGNITA_NAME_SIZE = 13;
// The implode payload always announces binary literals and a 4 KiB dictionary.
const quint8 RECOGNITA_DCL_LITERAL = 0x00;
const quint8 RECOGNITA_DCL_DICT = 0x06;
// The whole reference corpus tops out at 57 members; 65536 only bounds a
// corrupt chain.
const qint32 RECOGNITA_MAX_MEMBERS = 65536;

bool recognitaRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XRecognita::XRecognita(QIODevice *pDevice) : XArchive(pDevice)
{
}

XRecognita::~XRecognita()
{
}

// The reference extractor's own name test, reproduced exactly: the field is a
// strict DOS 8.3 name - printable ASCII, at least one character before the
// single dot and exactly three after it.  This is the main thing standing
// between a headerless container and a false positive, so it stays strict.
bool XRecognita::isValidRawName(const char *pRawName)
{
    qint32 nBeforeDot = 0;
    qint32 nDots = 0;
    qint32 nAfterDot = 0;
    for (qint32 i = 0; i < RECOGNITA_NAME_SIZE; i++) {
        const quint8 nCharacter = static_cast<quint8>(pRawName[i]);
        if (i == 12) {
            // A 12-character name would leave no room for the terminator.
            if (nCharacter != 0) return false;
            break;
        }
        if (nCharacter == 0) break;
        if ((nCharacter < 0x20) || (nCharacter > 0x7f)) return false;
        if (nCharacter == '.') {
            nDots++;
        } else if (nDots == 0) {
            nBeforeDot++;
        } else {
            nAfterDot++;
        }
    }
    return (nBeforeDot >= 1) && (nDots == 1) && (nAfterDot == 3);
}

bool XRecognita::isValidDosDateTime(quint16 nDosDate, quint16 nDosTime)
{
    if ((nDosDate == 0) && (nDosTime == 0)) return false;
    const quint32 nHour = static_cast<quint32>(nDosTime >> 11);
    const quint32 nMinute = static_cast<quint32>((nDosTime >> 5) & 0x3f);
    const quint32 nSecond = static_cast<quint32>((nDosTime & 0x1f) * 2);
    if ((nHour >= 24) || (nMinute >= 60) || (nSecond >= 60)) return false;

    const quint32 nMonth = static_cast<quint32>((nDosDate >> 5) & 0x0f);
    const quint32 nDay = static_cast<quint32>(nDosDate & 0x1f);
    if ((nMonth < 1) || (nMonth > 12) || (nDay < 1) || (nDay > 31)) return false;

    const quint32 nYear = static_cast<quint32>(nDosDate >> 9) + 1980;
    static const quint32 nDaysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    quint32 nLimit = nDaysInMonth[nMonth - 1];
    if (nMonth == 2) {
        const bool bLeap = ((nYear % 4) == 0) && (((nYear % 100) != 0) || ((nYear % 400) == 0));
        if (bLeap) nLimit = 29;
    }
    return nDay <= nLimit;
}

QString XRecognita::rawNameToString(const char *pRawName, qint32 nIndex)
{
    qint32 nLength = 0;
    while ((nLength < static_cast<qint32>(RECOGNITA_NAME_SIZE)) && (pRawName[nLength] != '\0')) nLength++;
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
    if (sResult.isEmpty()) {
        sResult = QStringLiteral("record%1").arg(nIndex);
    }
    return sResult;
}

bool XRecognita::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XRecognita> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // Header plus the two DCL parameter bytes is the shortest possible archive.
    if (context.nInputSize < RECOGNITA_HEADER_SIZE + 2) return false;

    qint64 nOffset = 0;
    while (nOffset < context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= RECOGNITA_MAX_MEMBERS) return false;
        if (!recognitaRangeWithin(context.nInputSize, nOffset, RECOGNITA_HEADER_SIZE + 2)) return false;

        const QByteArray baHeader = read_array_process(nOffset, RECOGNITA_HEADER_SIZE + 2, pPdStruct);
        if (!guardedThis || !guardedSource || (baHeader.size() != RECOGNITA_HEADER_SIZE + 2)) return false;
        const char *pRaw = baHeader.constData();
        const uchar *pHeader = reinterpret_cast<const uchar *>(pRaw);

        if (!isValidRawName(pRaw)) return false;
        const qint32 nUncompressedSize = qFromLittleEndian<qint32>(pHeader + 13);
        const quint16 nDosTime = qFromLittleEndian<quint16>(pHeader + 17);
        const quint16 nDosDate = qFromLittleEndian<quint16>(pHeader + 19);
        const qint32 nNextOffset = qFromLittleEndian<qint32>(pHeader + 21);
        if (nUncompressedSize <= 0) return false;
        if (!isValidDosDateTime(nDosDate, nDosTime)) return false;

        const qint64 nDataOffset = nOffset + RECOGNITA_HEADER_SIZE;
        if (static_cast<qint64>(nNextOffset) <= nDataOffset) return false;
        if (static_cast<qint64>(nNextOffset) > context.nInputSize) return false;
        // The DCL parameter pair is fixed by the producer, and checking it is
        // what makes this headerless container safe to detect on.
        if ((static_cast<quint8>(pHeader[25]) != RECOGNITA_DCL_LITERAL) || (static_cast<quint8>(pHeader[26]) != RECOGNITA_DCL_DICT)) return false;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nDataOffset;
        member.nPackedSize = static_cast<qint64>(nNextOffset) - nDataOffset;
        member.nUncompressedSize = nUncompressedSize;
        member.nDosTime = nDosTime;
        member.nDosDate = nDosDate;
        member.sFileName = rawNameToString(pRaw, context.listMembers.size());
        context.listMembers.append(member);

        nOffset = nNextOffset;
    }

    // The chain has to end exactly at end of file.
    if (context.listMembers.isEmpty() || (nOffset != context.nInputSize)) return false;

    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XRecognita::isValid(PDSTRUCT *pPdStruct)
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

bool XRecognita::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRecognita archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XRecognita::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XRecognita(pDevice);
}

XBinary::FT XRecognita::getFileType()
{
    return FT_RECOGNITA;
}

XBinary::MODE XRecognita::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XRecognita::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XRecognita::getArch()
{
    return QString();
}

QString XRecognita::getFileFormatExt()
{
    return QStringLiteral("cmp");
}

QString XRecognita::getFileFormatExtsString()
{
    return QStringLiteral("Recognita archive (*.cmp)");
}

QString XRecognita::getMIMEString()
{
    return QStringLiteral("application/x-recognita-cmp");
}

QString XRecognita::getVersion()
{
    return QString();
}

qint64 XRecognita::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XRecognita::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XRecognita::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XRecognita::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XRecognita::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = RECOGNITA_HEADER_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Header");
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nPackedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKWARE_DCL_IMPLODE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PKWARE DCL implode"));
            const QDateTime dtMTime = dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
            if (dtMTime.isValid()) {
                part.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
            }
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = (member.nDataOffset + member.nPackedSize) - member.nHeaderOffset;
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

QMap<XBinary::UNPACK_PROP, QVariant> XRecognita::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XRecognita::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XRecognita> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Recognita archive"));
    pState->nCurrentOffset = pContext->listMembers.first().nDataOffset;
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

XBinary::ARCHIVERECORD XRecognita::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nPackedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKWARE_DCL_IMPLODE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PKWARE DCL implode"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    const QDateTime dtMTime = dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
    if (dtMTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
    }
    return result;
}

bool XRecognita::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nDataOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XRecognita::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
