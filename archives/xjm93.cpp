/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xjm93.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 JM93_HEADER_SIZE = 0x49;
const qint64 JM93_NAME_OFFSET = 0x05;
const qint64 JM93_NAME_SIZE = 60;
// Nothing this tool ever produced comes close; the cap only keeps a corrupt
// header from asking for an absurd allocation.
const qint64 JM93_MAX_SIZE = qint64(1) << 32;

// The 60-byte name field is NUL padded.  A NUL followed by a non-NUL never
// happens in a real header, and refusing that shape is what keeps the 4-byte
// magic from firing on unrelated data.
bool jm93IsValidRawName(const char *pName)
{
    if (pName[0] == 0) return false;
    bool bPadding = false;
    for (qint64 i = 0; i < JM93_NAME_SIZE; i++) {
        const quint8 nCharacter = static_cast<quint8>(pName[i]);
        if (nCharacter == 0) {
            bPadding = true;
        } else if (bPadding) {
            return false;
        } else if ((nCharacter < 0x20) || (nCharacter > 0x7e)) {
            return false;
        }
    }
    return true;
}
}  // namespace

XJM93::XJM93(QIODevice *pDevice) : XArchive(pDevice)
{
}

XJM93::~XJM93()
{
}

// The stored name is a DOS 8.3 name in the whole reference corpus, but the
// field is raw bytes, so path separators and the Windows reserved punctuation
// are escaped as %XX rather than folded to '_': escaping is reversible and
// cannot collapse two distinct names onto one output file.
QString XJM93::rawNameToString(const char *pRawName)
{
    qint32 nLength = 0;
    while ((nLength < static_cast<qint32>(JM93_NAME_SIZE)) && (pRawName[nLength] != '\0')) nLength++;
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
        sResult = QStringLiteral("jm93_data");
    }
    return sResult;
}

bool XJM93::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // Header plus the two-byte implode preamble plus at least one byte of code.
    if (context.nInputSize < JM93_HEADER_SIZE + 3) return false;

    const QByteArray baHeader = read_array_process(0, JM93_HEADER_SIZE + 2, pPdStruct);
    if ((baHeader.size() != JM93_HEADER_SIZE + 2)) return false;
    const char *pHeader = baHeader.constData();
    const uchar *pRaw = reinterpret_cast<const uchar *>(pHeader);

    if (memcmp(pHeader, "JM93", 4) != 0) return false;
    if (pHeader[4] != 0) return false;
    if (!jm93IsValidRawName(pHeader + JM93_NAME_OFFSET)) return false;

    const quint32 nCompressedSize = qFromLittleEndian<quint32>(pRaw + 0x41);
    const quint32 nUncompressedSize = qFromLittleEndian<quint32>(pRaw + 0x45);
    if ((nCompressedSize == 0) || (nUncompressedSize == 0)) return false;
    if ((qint64(nCompressedSize) > JM93_MAX_SIZE) || (qint64(nUncompressedSize) > JM93_MAX_SIZE)) return false;
    if (qint64(nCompressedSize) > context.nInputSize - JM93_HEADER_SIZE) return false;
    // A stream this short cannot even carry the preamble and the end code.
    if (nCompressedSize < 3) return false;

    // The payload is a PKWARE DCL implode stream, and its first two bytes are a
    // plain-text preamble: literal mode 0 or 1, then the dictionary order,
    // which the reference decoder only accepts as 4, 5 or 6.  Checking it here
    // costs two bytes and rules out a container that would decode to garbage.
    const quint8 nLiteralMode = static_cast<quint8>(pHeader[JM93_HEADER_SIZE]);
    const quint8 nDictionaryBits = static_cast<quint8>(pHeader[JM93_HEADER_SIZE + 1]);
    if (nLiteralMode > 1) return false;
    if ((nDictionaryBits < 4) || (nDictionaryBits > 6)) return false;

    context.nDataOffset = JM93_HEADER_SIZE;
    context.nCompressedSize = nCompressedSize;
    context.nUncompressedSize = nUncompressedSize;
    context.nArchiveSize = JM93_HEADER_SIZE + qint64(nCompressedSize);
    context.sFileName = rawNameToString(pHeader + JM93_NAME_OFFSET);

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XJM93::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if ((nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XJM93::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XJM93 archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XJM93::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XJM93(pDevice);
}

QList<QString> XJM93::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'JM93'00"));
    return listResult;
}

XBinary::FT XJM93::getFileType()
{
    return FT_JM93;
}

XBinary::MODE XJM93::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XJM93::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XJM93::getArch()
{
    return QString();
}

QString XJM93::getFileFormatExt()
{
    return QStringLiteral("cmp");
}

QString XJM93::getFileFormatExtsString()
{
    return QStringLiteral("JM93 compressed file (*.cmp)");
}

QString XJM93::getMIMEString()
{
    return QStringLiteral("application/x-jm93");
}

QString XJM93::getVersion()
{
    return QString();
}

qint64 XJM93::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XJM93::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XJM93::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XJM93::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XJM93::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = JM93_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nDataOffset;
        part.nFileSize = context.nCompressedSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nCompressedSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKWARE_DCL_IMPLODE);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PKWARE DCL implode"));
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = context.nDataOffset;
        part.nFileSize = context.nCompressedSize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XJM93::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XJM93::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    if (!parseContext(pContext, pPdStruct)) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("JM93 compressed file"));
    pState->nCurrentOffset = pContext->nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XJM93::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex != 0)) return ARCHIVERECORD();
    if (pState->nCurrentOffset != pContext->nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nDataOffset;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKWARE_DCL_IMPLODE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PKWARE DCL implode"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XJM93::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XJM93::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
