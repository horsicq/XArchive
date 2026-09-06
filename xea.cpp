/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xea.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
// Every field of the 48-byte header is accounted for; +0x21..+0x2f is zero
// padding on all 212 members of the corpus.
const qint64 EA_HEADER_SIZE = 48;
const qint32 EA_NAME_OFFSET = 3;
const qint32 EA_NAME_SIZE = 12;
const qint32 EA_METHOD_OFFSET = 0x14;
const qint32 EA_USIZE_OFFSET = 0x15;
const qint32 EA_CSIZE_OFFSET = 0x19;
const qint32 EA_TAG_OFFSET = 0x1d;
const qint32 EA_STAMP_OFFSET = 0x10;
const quint32 EA_TAG_VALUE = 0x130U;
const quint8 EA_METHOD_STORED = 0U;
const quint8 EA_METHOD_LZW = 1U;
const qint32 EA_MAX_MEMBERS = 100000;
const qint64 EA_MAX_UNCOMPRESSED_SIZE = 0x10000000;  // 256 MB sanity cap

bool eaRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

bool eaIsKnownMethod(quint8 nMethod)
{
    return (nMethod == EA_METHOD_STORED) || (nMethod == EA_METHOD_LZW);
}

// The name is a NUL-padded 8.3 DOS name.  Everything after the first NUL must
// also be NUL - a stale tail would mean this is not an EA header at all.
bool eaDecodeName(const QByteArray &baHeader, QString *psName)
{
    const QByteArray baField = baHeader.mid(EA_NAME_OFFSET, EA_NAME_SIZE);
    if (baField.size() != EA_NAME_SIZE) return false;

    qint32 nLength = 0;
    while ((nLength < EA_NAME_SIZE) && (baField.at(nLength) != '\0')) {
        ++nLength;
    }
    if (nLength == 0) return false;
    for (qint32 i = nLength; i < EA_NAME_SIZE; ++i) {
        if (baField.at(i) != '\0') return false;
    }
    for (qint32 i = 0; i < nLength; ++i) {
        const quint8 nCharacter = static_cast<quint8>(baField.at(i));
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
        if ((nCharacter == '/') || (nCharacter == '\\') ||
            (nCharacter == ':')) {
            return false;
        }
    }
    *psName = QString::fromLatin1(baField.constData(), nLength);
    return true;
}
}  // namespace

XEA::XEA(QIODevice *pDevice) : XArchive(pDevice)
{
}

XEA::~XEA()
{
}

bool XEA::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XEA> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < EA_HEADER_SIZE) return false;

    qint64 nOffset = 0;
    while (context.listMembers.size() < EA_MAX_MEMBERS &&
           isPdStructNotCanceled(pPdStruct)) {
        if (!eaRangeWithin(context.nInputSize, nOffset, EA_HEADER_SIZE)) {
            return false;
        }
        const QByteArray baHeader =
            read_array_process(nOffset, EA_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baHeader.size() != EA_HEADER_SIZE) {
            return false;
        }
        const uchar *pHeader =
            reinterpret_cast<const uchar *>(baHeader.constData());

        if (pHeader[0] != 0x1a) return false;
        if ((pHeader[1] != 'E') || (pHeader[2] != 'A')) return false;
        if (qFromLittleEndian<quint32>(pHeader + EA_TAG_OFFSET) !=
            EA_TAG_VALUE) {
            return false;
        }

        const quint8 nMethod = pHeader[EA_METHOD_OFFSET];
        if (!eaIsKnownMethod(nMethod)) return false;

        // Both size words are read as SIGNED and U3 rejects a negative one;
        // that is the only bound the format itself gives.
        const qint32 nUncompressed = static_cast<qint32>(
            qFromLittleEndian<quint32>(pHeader + EA_USIZE_OFFSET));
        const qint32 nCompressed = static_cast<qint32>(
            qFromLittleEndian<quint32>(pHeader + EA_CSIZE_OFFSET));
        if ((nUncompressed < 0) || (nCompressed < 0)) return false;
        if (nUncompressed > EA_MAX_UNCOMPRESSED_SIZE) return false;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nOffset + EA_HEADER_SIZE;
        member.nMethod = nMethod;
        member.nCompressedSize = nCompressed;
        member.nUncompressedSize = nUncompressed;
        member.nStamp = qFromLittleEndian<quint32>(pHeader + EA_STAMP_OFFSET);
        if (!eaDecodeName(baHeader, &member.sFileName)) return false;
        if (!eaRangeWithin(context.nInputSize, member.nDataOffset,
                           member.nCompressedSize)) {
            return false;
        }
        // A stored member is the structural anchor of the method gate: both
        // size fields have to agree, exactly as U3's stored branch requires.
        if ((nMethod == EA_METHOD_STORED) &&
            (member.nCompressedSize != member.nUncompressedSize)) {
            return false;
        }

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;

        // No terminator record: the chain ends by landing exactly on EOF.
        if (nOffset == context.nInputSize) {
            context.nArchiveSize = nOffset;
            context.nFirstMemberOffset =
                context.listMembers.first().nHeaderOffset;
            *pContext = context;
            return guardedThis && guardedSource &&
                   isPdStructNotCanceled(pPdStruct);
        }
    }

    return false;
}

bool XEA::isValid(PDSTRUCT *pPdStruct)
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

bool XEA::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XEA archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XEA::createInstance(QIODevice *pDevice, bool bIsImage,
                             XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XEA(pDevice);
}

QList<QString> XEA::getSearchSignatures()
{
    return {QStringLiteral("1A'EA'")};
}

XBinary::FT XEA::getFileType()
{
    return FT_EA;
}

XBinary::MODE XEA::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XEA::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XEA::getArch()
{
    return QString();
}

QString XEA::getFileFormatExt()
{
    return QStringLiteral("pea");
}

QString XEA::getFileFormatExtsString()
{
    return QStringLiteral("Electronic Arts archive (*.pea)");
}

QString XEA::getMIMEString()
{
    return QStringLiteral("application/x-ea-pea");
}

QString XEA::getVersion()
{
    // +0x1d is a fixed 0x130 on every member of every known archive and the
    // parser rejects anything else, so it is reported as the format tag.
    return QStringLiteral("0x130");
}

qint64 XEA::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XEA::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XEA::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(
            FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XEA::methodToString(quint8 nMethod)
{
    if (nMethod == EA_METHOD_STORED) return QStringLiteral("Stored");
    if (nMethod == EA_METHOD_LZW) return QStringLiteral("LZW (12-bit)");
    return QStringLiteral("Unknown 0x%1").arg(nMethod, 2, 16, QLatin1Char('0'));
}

XBinary::HANDLE_METHOD XEA::methodToHandleMethod(quint8 nMethod)
{
    if (nMethod == EA_METHOD_STORED) return HANDLE_METHOD_STORE;
    if (nMethod == EA_METHOD_LZW) return HANDLE_METHOD_EA;
    return HANDLE_METHOD_UNKNOWN;
}

bool XEA::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XEA::getFileParts(quint32 nFileParts, qint32 nLimit,
                                        PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = EA_HEADER_SIZE;
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
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      methodToHandleMethod(member.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      methodToString(member.nMethod));
            part.mapProperties.insert(FPART_PROP_TYPE,
                                      static_cast<quint32>(member.nMethod));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = EA_HEADER_SIZE + member.nCompressedSize;
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
        (context.nArchiveSize < context.nInputSize) &&
        canAppendPart(nLimit, result.size())) {
        // parseContext() only accepts a chain that lands on EOF, so this
        // branch cannot fire today; it is kept so the part list stays correct
        // if the acceptance rule is ever relaxed.
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

QMap<XBinary::UNPACK_PROP, QVariant> XEA::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XEA::initUnpack(UNPACK_STATE *pState,
                     const QMap<UNPACK_PROP, QVariant> &mapProperties,
                     PDSTRUCT *pPdStruct)
{
    QPointer<XEA> guardedThis(this);
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
        tr("Electronic Arts DOS archive; stored and 12-bit LZW members"));
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
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

XBinary::ARCHIVERECORD XEA::infoCurrent(UNPACK_STATE *pState,
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
                                methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                methodToString(member.nMethod));
    result.mapProperties.insert(FPART_PROP_TYPE,
                                static_cast<quint32>(member.nMethod));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // +0x10 is not a DOS date - the high word decodes to years far outside
    // 1980..2107 on most members - so it is published raw and no MTIME is set.
    result.mapProperties.insert(FPART_PROP_UID,
                                static_cast<quint32>(member.nStamp));
    return result;
}

bool XEA::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
            pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XEA::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
