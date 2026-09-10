/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xswag.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
// Fixed part of the level-0 header: everything up to and including the name
// length byte at +0xBA.
const qint64 SWAG_FIXED_HEADER_SIZE = 0xbb;
// The name length byte sits at the very end of that fixed part.
const qint32 SWAG_NAMELENGTH_OFFSET = 0xba;
// SWAG names are plain MS-DOS 8.3, which is also the bound the reference implementation enforces.
const qint32 SWAG_MAX_NAME_SIZE = 12;
// ShortString[60] copyright + ShortString[65] title + u16 member count.
const qint64 SWAG_FOOTER_SIZE = 0x81;
const qint32 SWAG_FOOTER_TITLE_OFFSET = 0x3d;
const qint32 SWAG_FOOTER_COUNT_OFFSET = 0x7f;
const qint32 SWAG_FOOTER_COPYRIGHT_CAPACITY = 60;
const qint32 SWAG_FOOTER_TITLE_CAPACITY = 65;
// SWAG metadata block inside the header.
const qint32 SWAG_CRC32_OFFSET = 0x15;
const qint32 SWAG_SOURCEARCHIVE_OFFSET = 0x19;
const qint32 SWAG_SOURCEARCHIVE_CAPACITY = 0x28;
// A collection never comes close to this; it only keeps a corrupt count from
// making the walk unbounded.
const qint32 SWAG_MAX_MEMBERS = 65535;
const qint64 SWAG_MAX_UNCOMPRESSED_SIZE = 0x4000000;  // 64 MB sanity cap

bool swagIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty() || (baName.size() > SWAG_MAX_NAME_SIZE)) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
        if ((c == '"') || (c == '*') || (c == '<') || (c == '>') ||
            (c == '?') || (c == '|') || (c == ':') || (c == '/') ||
            (c == '\\')) {
            return false;
        }
    }
    return true;
}
}  // namespace

XSWAG::XSWAG(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSWAG::~XSWAG()
{
}

QString XSWAG::readShortString(const QByteArray &baBlock, qint32 nOffset,
                               qint32 nCapacity)
{
    // Turbo Pascal ShortString: one length byte followed by a fixed-size body.
    if ((nOffset < 0) || (nCapacity < 0) ||
        (nOffset >= baBlock.size()) ||
        (baBlock.size() - nOffset - 1 < nCapacity)) {
        return QString();
    }
    const qint32 nLength =
        static_cast<quint8>(baBlock.at(nOffset)) & 0xff;
    if ((nLength == 0) || (nLength > nCapacity)) return QString();
    const QByteArray baText = baBlock.mid(nOffset + 1, nLength);
    for (char c : baText) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return QString();
    }
    return QString::fromLatin1(baText).trimmed();
}

bool XSWAG::parseHeader(const QByteArray &baHeader, MEMBER *pMember)
{
    if (!pMember) return false;
    if (baHeader.size() < SWAG_FIXED_HEADER_SIZE) return false;

    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    const qint32 nHeaderSize = pHeader[0];
    const qint32 nNameSize = pHeader[SWAG_NAMELENGTH_OFFSET];

    if (baHeader.size() != nHeaderSize + 2) return false;
    if ((nNameSize <= 0) || (nNameSize > SWAG_MAX_NAME_SIZE)) return false;
    // The whole point of the "-sw1-" layout: the header is exactly the fixed
    // 0xBB-byte block plus the name, so this equality is a hard structural rule
    // and not a heuristic.
    if (nHeaderSize != nNameSize + SWAG_FIXED_HEADER_SIZE) return false;
    if (memcmp(pHeader + 2, "-sw1-", 5) != 0) return false;

    // LHA level-0 checksum: the low byte of the sum of the nHeaderSize bytes
    // that follow the size and checksum fields.  195 summed bytes make a false
    // positive on random data a 1-in-256 event on top of the literal match.
    quint32 nSum = 0;
    for (qint32 i = 0; i < nHeaderSize; ++i) nSum += pHeader[2 + i];
    if (static_cast<quint8>(nSum & 0xff) != pHeader[1]) return false;

    const qint32 nCompressedSize =
        static_cast<qint32>(qFromLittleEndian<quint32>(pHeader + 7));
    const qint32 nUncompressedSize =
        static_cast<qint32>(qFromLittleEndian<quint32>(pHeader + 0x0b));
    if ((nCompressedSize < 0) || (nUncompressedSize < 0)) return false;
    if (nUncompressedSize > SWAG_MAX_UNCOMPRESSED_SIZE) return false;

    const QByteArray baName = baHeader.mid(
        static_cast<qint32>(SWAG_FIXED_HEADER_SIZE), nNameSize);
    if (!swagIsValidName(baName)) return false;

    pMember->nCompressedSize = nCompressedSize;
    pMember->nUncompressedSize = nUncompressedSize;
    pMember->nDosTime = qFromLittleEndian<quint16>(pHeader + 0x0f);
    pMember->nDosDate = qFromLittleEndian<quint16>(pHeader + 0x11);
    pMember->nAttributes = qFromLittleEndian<quint16>(pHeader + 0x13);
    // The stored word is the CRC-32 register before its final inversion.
    pMember->nCrc32 =
        qFromLittleEndian<quint32>(pHeader + SWAG_CRC32_OFFSET) ^ 0xffffffffU;
    pMember->nCrc16 = qFromLittleEndian<quint16>(pHeader + nHeaderSize);
    pMember->sFileName = QString::fromLatin1(baName);
    pMember->sSourceArchive = readShortString(baHeader,
                                              SWAG_SOURCEARCHIVE_OFFSET,
                                              SWAG_SOURCEARCHIVE_CAPACITY);
    return true;
}

bool XSWAG::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSWAG> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < SWAG_FIXED_HEADER_SIZE + 3 + SWAG_FOOTER_SIZE) {
        return false;
    }

    context.nFooterOffset = context.nInputSize - SWAG_FOOTER_SIZE;
    const QByteArray baFooter =
        read_array_process(context.nFooterOffset, SWAG_FOOTER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baFooter.size() != SWAG_FOOTER_SIZE)) {
        return false;
    }
    context.nDeclaredCount = static_cast<qint32>(qFromLittleEndian<quint16>(
        reinterpret_cast<const uchar *>(baFooter.constData()) +
        SWAG_FOOTER_COUNT_OFFSET));
    if ((context.nDeclaredCount <= 0) ||
        (context.nDeclaredCount > SWAG_MAX_MEMBERS)) {
        return false;
    }
    context.sCopyright =
        readShortString(baFooter, 0, SWAG_FOOTER_COPYRIGHT_CAPACITY);
    context.sTitle = readShortString(baFooter, SWAG_FOOTER_TITLE_OFFSET,
                                     SWAG_FOOTER_TITLE_CAPACITY);

    qint64 nOffset = 0;
    for (qint32 i = 0; i < context.nDeclaredCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nOffset + SWAG_FIXED_HEADER_SIZE > context.nInputSize) break;

        const QByteArray baFixed =
            read_array_process(nOffset, SWAG_FIXED_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            (baFixed.size() != SWAG_FIXED_HEADER_SIZE)) {
            return false;
        }
        const qint64 nHeaderSize =
            static_cast<quint8>(baFixed.at(0)) & 0xff;
        if (nOffset + nHeaderSize + 2 > context.nInputSize) break;

        const QByteArray baHeader =
            read_array_process(nOffset, nHeaderSize + 2, pPdStruct);
        if (!guardedThis || !guardedSource ||
            (baHeader.size() != nHeaderSize + 2)) {
            return false;
        }

        MEMBER member = {};
        if (!parseHeader(baHeader, &member)) break;
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nOffset + nHeaderSize + 2;
        if (member.nCompressedSize >
            context.nInputSize - member.nDataOffset) {
            // Truncated tail member: two of the reference archives end this
            // way and the reference implementation keeps everything before it, so stop rather than
            // failing the whole file.
            break;
        }

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;
    }

    if (context.listMembers.isEmpty()) return false;
    if (!guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return true;
}

bool XSWAG::isValid(PDSTRUCT *pPdStruct)
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

bool XSWAG::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSWAG archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSWAG::createInstance(QIODevice *pDevice, bool bIsImage,
                               XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSWAG(pDevice);
}

QList<QString> XSWAG::getSearchSignatures()
{
    // Header size and checksum first, then the method tag; the structural walk
    // in isValid() does the real deciding.
    return {QStringLiteral("....'-sw1-'")};
}

XBinary::FT XSWAG::getFileType()
{
    return FT_SWAG;
}

XBinary::MODE XSWAG::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSWAG::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSWAG::getArch()
{
    return QString();
}

QString XSWAG::getFileFormatExt()
{
    return QStringLiteral("swg");
}

QString XSWAG::getFileFormatExtsString()
{
    return QStringLiteral("SWAG collection (*.swg)");
}

QString XSWAG::getMIMEString()
{
    return QStringLiteral("application/x-swag");
}

QString XSWAG::getVersion()
{
    // The method tag is the only version-like field the container carries.
    return QStringLiteral("sw1");
}

qint64 XSWAG::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSWAG::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSWAG::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(
            FILEPART_HEADER | FILEPART_STREAM | FILEPART_FOOTER, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XSWAG::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSWAG::getFileParts(quint32 nFileParts, qint32 nLimit,
                                          PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct)) break;
        if ((nFileParts & FILEPART_HEADER) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nDataOffset - member.nHeaderOffset;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
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
            part.mapProperties.insert(
                FPART_PROP_HANDLEMETHOD,
                (member.nUncompressedSize == 0) ? HANDLE_METHOD_STORE
                                                : HANDLE_METHOD_LZH1);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("-sw1-"));
            part.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_CRC16);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDCRC,
                                      static_cast<quint32>(member.nCrc16));
            const QDateTime dtMTime =
                dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
            if (dtMTime.isValid()) {
                part.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
            }
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = (member.nDataOffset - member.nHeaderOffset) +
                             member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_FOOTER) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_FOOTER;
        part.nFileOffset = context.nFooterOffset;
        part.nFileSize = SWAG_FOOTER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sTitle.isEmpty() ? tr("Footer") : context.sTitle;
        result.append(part);
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
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XSWAG::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSWAG::initUnpack(UNPACK_STATE *pState,
                       const QMap<UNPACK_PROP, QVariant> &mapProperties,
                       PDSTRUCT *pPdStruct)
{
    QPointer<XSWAG> guardedThis(this);
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
    if (!operationGuard.isAcquired() ||
        !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedThis ||
        !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        pContext->sTitle.isEmpty()
            ? tr("SWAG collection; LZHUF (-sw1-) members")
            : pContext->sTitle);
    pState->nCurrentOffset = pContext->listMembers.first().nHeaderOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
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

XBinary::ARCHIVERECORD XSWAG::infoCurrent(UNPACK_STATE *pState,
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
    result.mapProperties.insert(
        FPART_PROP_HANDLEMETHOD,
        (member.nUncompressedSize == 0) ? HANDLE_METHOD_STORE
                                        : HANDLE_METHOD_LZH1);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("-sw1-"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_CRC16);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDCRC,
                                static_cast<quint32>(member.nCrc16));
    if (!member.sSourceArchive.isEmpty()) {
        result.mapProperties.insert(FPART_PROP_INFO, member.sSourceArchive);
    }
    const QDateTime dtMTime =
        dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
    if (dtMTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
    }
    return result;
}

bool XSWAG::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XSWAG::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
