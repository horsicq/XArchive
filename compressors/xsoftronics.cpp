/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsoftronics.h"

#include <QPointer>

#include <new>
#include <string.h>

namespace {
// "Softronics Compressed File\0Version 2.00\0" - 40 bytes starting at offset 2.
const char SOFTRONICS_SIGNATURE[] = "Softronics Compressed File\0Version 2.00";
const qint64 SOFTRONICS_SIGNATURE_OFFSET = 2;
const qint64 SOFTRONICS_SIGNATURE_SIZE = 40;  // 26 + NUL + 12 + NUL
// Offset of the compressed size, and of the name that follows it.
const qint64 SOFTRONICS_COMPRESSEDSIZE_OFFSET = 0x2a;
const qint64 SOFTRONICS_NAME_OFFSET = 0x2e;
// A DOS 8.3 name, so 12 characters plus the terminator; the byte at offset 0
// caps the name at exactly this because it has to fit in a quint8.
const qint64 SOFTRONICS_MAX_NAME = 12;
// What follows the name: uncompressed size and the packed DOS time stamp.
const qint64 SOFTRONICS_TAIL_SIZE = 8;
// The largest header the layout can produce, which is also the largest probe
// the checks below ever need.
const qint64 SOFTRONICS_MAX_HEADER = SOFTRONICS_NAME_OFFSET + SOFTRONICS_MAX_NAME + 1 + SOFTRONICS_TAIL_SIZE;

quint32 softronicsReadLe32(const QByteArray &baData, qint64 nOffset)
{
    const quint8 *pData = (const quint8 *)baData.constData() + nOffset;
    return (quint32)pData[0] | ((quint32)pData[1] << 8) | ((quint32)pData[2] << 16) | ((quint32)pData[3] << 24);
}
}  // namespace

XSoftronics::XSoftronics(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSoftronics::~XSoftronics()
{
}

QString XSoftronics::sanitizeName(const QByteArray &baRawName)
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

// Two independent identities decide this format, and a file that is not one of
// these cannot satisfy both: the byte at offset 0 has to equal the header length
// the stored name implies, AND that same byte plus the stored compressed size has
// to equal the file size.  The 40-byte banner on its own is already decisive, but
// the two identities are what keeps a banner that happens to appear inside some
// other carrier from being read as a container.
bool XSoftronics::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSoftronics> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nUncompressedSize = -1;
    context.nArchiveSize = guardedSource->size();
    // The smallest possible container: header with a one-character name plus at
    // least one byte of code stream.
    if (context.nArchiveSize < SOFTRONICS_NAME_OFFSET + 1 + 1 + SOFTRONICS_TAIL_SIZE + 1) return false;
    if (context.nArchiveSize > 0x7fffffff) return false;

    // The banner is checked on its own first: this runs during type detection on
    // every file, and only a file carrying it is worth the full header read.
    const QByteArray baBanner = read_array_process(SOFTRONICS_SIGNATURE_OFFSET, SOFTRONICS_SIGNATURE_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baBanner.size() != SOFTRONICS_SIGNATURE_SIZE)) return false;
    if (memcmp(baBanner.constData(), SOFTRONICS_SIGNATURE, (size_t)SOFTRONICS_SIGNATURE_SIZE) != 0) return false;

    const qint64 nProbeSize = qMin(context.nArchiveSize, SOFTRONICS_MAX_HEADER);
    const QByteArray baProbe = read_array_process(0, nProbeSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baProbe.size() != nProbeSize)) return false;

    if ((quint8)baProbe.at(1) != 0) return false;

    if (nProbeSize < SOFTRONICS_COMPRESSEDSIZE_OFFSET + 4) return false;
    const quint32 nCompressedSize = softronicsReadLe32(baProbe, SOFTRONICS_COMPRESSEDSIZE_OFFSET);
    if ((nCompressedSize == 0) || (nCompressedSize > 0x7fffffff)) return false;

    qint64 nNameLength = 0;
    while ((SOFTRONICS_NAME_OFFSET + nNameLength < nProbeSize) && (nNameLength < SOFTRONICS_MAX_NAME) &&
           ((quint8)baProbe.at((qint32)(SOFTRONICS_NAME_OFFSET + nNameLength)) != 0)) {
        const quint8 nCharacter = (quint8)baProbe.at((qint32)(SOFTRONICS_NAME_OFFSET + nNameLength));
        if ((nCharacter < 0x20) || (nCharacter == 0x7f)) return false;
        nNameLength++;
    }
    if (nNameLength < 1) return false;
    if (SOFTRONICS_NAME_OFFSET + nNameLength >= nProbeSize) return false;
    if ((quint8)baProbe.at((qint32)(SOFTRONICS_NAME_OFFSET + nNameLength)) != 0) return false;

    const qint64 nHeaderSize = SOFTRONICS_NAME_OFFSET + nNameLength + 1 + SOFTRONICS_TAIL_SIZE;
    if ((quint8)baProbe.at(0) != (quint8)nHeaderSize) return false;
    if (nHeaderSize > nProbeSize) return false;
    if (nHeaderSize + (qint64)nCompressedSize != context.nArchiveSize) return false;

    const qint64 nTailOffset = SOFTRONICS_NAME_OFFSET + nNameLength + 1;
    const quint32 nUncompressedSize = softronicsReadLe32(baProbe, nTailOffset);
    if ((nUncompressedSize == 0) || (nUncompressedSize > 0x7fffffff)) return false;

    context.nHeaderSize = nHeaderSize;
    context.nStreamOffset = nHeaderSize;
    context.nStreamSize = (qint64)nCompressedSize;
    context.nUncompressedSize = (qint64)nUncompressedSize;
    context.nDosDateTime = softronicsReadLe32(baProbe, nTailOffset + 4);
    context.sFileName = sanitizeName(baProbe.mid((qint32)SOFTRONICS_NAME_OFFSET, (qint32)nNameLength));
    if (context.sFileName.isEmpty()) return false;

    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XSoftronics::isValid(PDSTRUCT *pPdStruct)
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

bool XSoftronics::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSoftronics archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSoftronics::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSoftronics(pDevice);
}

QList<QString> XSoftronics::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("..00'Softronics Compressed File'00'Version 2.00'00"));
    return listResult;
}

XBinary::FT XSoftronics::getFileType()
{
    return FT_SOFTRONICS;
}

XBinary::MODE XSoftronics::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSoftronics::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSoftronics::getArch()
{
    return QString();
}

QString XSoftronics::getFileFormatExt()
{
    return QString();
}

QString XSoftronics::getFileFormatExtsString()
{
    return QStringLiteral("Softronics compressed file (*.*)");
}

QString XSoftronics::getMIMEString()
{
    return QStringLiteral("application/x-softronics-compressed");
}

QString XSoftronics::getVersion()
{
    return QStringLiteral("2.00");
}

qint64 XSoftronics::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSoftronics::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSoftronics::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XSoftronics::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSoftronics::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nHeaderSize;
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
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SOFTRONICS_LZW);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Softronics LZW"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XSoftronics::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSoftronics::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSoftronics> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || (pContext->nUncompressedSize < 0)) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Softronics compressed file"));
    pState->nCurrentOffset = pContext->nStreamOffset;
    pState->nTotalSize = pContext->nArchiveSize;
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

XBinary::ARCHIVERECORD XSoftronics::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SOFTRONICS_LZW);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Softronics LZW"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    const QDateTime dtModified = dosDateTimeToQDateTime((quint16)(pContext->nDosDateTime >> 16), (quint16)(pContext->nDosDateTime & 0xffff));
    if (dtModified.isValid()) result.mapProperties.insert(FPART_PROP_DATETIME, dtModified);
    return result;
}

bool XSoftronics::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XSoftronics::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
