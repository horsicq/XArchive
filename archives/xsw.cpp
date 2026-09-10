/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsw.h"

#include <QPointer>

#include <new>

namespace {
const qint64 SW_HEADER_SIZE = 13;
const qint64 SW_MAGIC_SIZE = 12;
const qint64 SW_SCAN_BLOCK = 0x10000;
const qint64 SW_SCAN_OVERLAP = 0x100;
const qint32 SW_MIN_NAME = 5;    // strictly greater than 4
const qint32 SW_MAX_NAME = 255;  // strictly less than 0x100
// A single image never carries more members than this in the reference corpus
// (the largest holds 2896); the cap only keeps a pathological input from
// building an unbounded list.
const qint32 SW_MAX_MEMBERS = 1000000;

// A member name always starts at one of the eight IRIX top level directories
// the reference scanner recognises.  This is what makes the boundary recovery
// safe: without it a 16 bit length that happens to fall in 5..255 would split
// the payload at a random offset.
bool swIsKnownPrefix(const char *pName)
{
    static const char *const s_prefixes[] = {"usr/", "var/", "dev/", "etc/", "lib/", "tmp/", "sbin", "stan"};
    for (qint32 i = 0; i < 8; i++) {
        if ((pName[0] == s_prefixes[i][0]) && (pName[1] == s_prefixes[i][1]) && (pName[2] == s_prefixes[i][2]) && (pName[3] == s_prefixes[i][3])) {
            return true;
        }
    }
    return false;
}

// The reference scanner rejects a candidate whose name holds a control
// character or any of " * < > ? \ | - the characters that cannot appear in a
// filename.  '/' and ':' are allowed because they are path punctuation.
bool swIsValidNameByte(quint8 nCharacter)
{
    if (nCharacter < 0x20) return false;
    if ((nCharacter == '"') || (nCharacter == '*') || (nCharacter == '<') || (nCharacter == '>') || (nCharacter == '?') || (nCharacter == '\\') ||
        (nCharacter == '|')) {
        return false;
    }
    return true;
}

bool swIsDigit(char cCharacter)
{
    return (cCharacter >= '0') && (cCharacter <= '9');
}

// Names are IRIX paths; they are emitted with '/' separators and stripped of
// anything that could escape the destination directory.
QString swRawNameToString(const QByteArray &baName, qint32 nIndex)
{
    QString sResult;
    for (qint32 i = 0; i < baName.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baName.at(i));
        if (nCharacter == ':') {
            sResult.append(QLatin1Char('_'));
        } else if (nCharacter < 0x20) {
            sResult.append(QLatin1Char('_'));
        } else {
            sResult.append(QLatin1Char(static_cast<char>(nCharacter)));
        }
    }
    while (sResult.startsWith(QLatin1Char('/'))) sResult.remove(0, 1);
    sResult.replace(QLatin1String("../"), QLatin1String("__/"));
    if (sResult.isEmpty()) sResult = QStringLiteral("record%1").arg(nIndex);
    return sResult;
}
}  // namespace

XSW::XSW(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSW::~XSW()
{
}

bool XSW::parseContext(CONTEXT *pContext, bool bHeaderOnly, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSW> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // Header, one length field and at least a five byte name have to fit.
    if (context.nInputSize < SW_HEADER_SIZE + 2 + SW_MIN_NAME) return false;

    const QByteArray baHeader = read_array_process(0, SW_HEADER_SIZE + 2, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != SW_HEADER_SIZE + 2)) return false;
    const char *pHeader = baHeader.constData();

    if (memcmp(pHeader, "im001V", 6) != 0) return false;
    if (!swIsDigit(pHeader[6]) || !swIsDigit(pHeader[7]) || !swIsDigit(pHeader[8])) return false;
    if (pHeader[9] != 'P') return false;
    if (!swIsDigit(pHeader[10]) || !swIsDigit(pHeader[11])) return false;
    // Byte 12 closes the header, byte 13 is the high half of the first member's
    // name length (always zero because names are shorter than 256 bytes) and
    // byte 14 is that length, which can never be zero.
    if (pHeader[12] != 0) return false;
    if (pHeader[13] != 0) return false;
    if (pHeader[14] == 0) return false;

    context.sVersion = QString::fromLatin1(pHeader, static_cast<int>(SW_MAGIC_SIZE));
    context.nArchiveSize = context.nInputSize;

    if (bHeaderOnly) {
        *pContext = context;
        return true;
    }

    // Boundary recovery, exactly as the reference extractor does it: a sliding
    // 64 KiB window with a 256 byte overlap, restarted seven bytes past every
    // hit (two length bytes plus the shortest accepted name).
    QList<qint64> listOffsets;
    qint64 nPosition = SW_HEADER_SIZE;
    while (isPdStructNotCanceled(pPdStruct)) {
        const QByteArray baBlock = read_array_process(nPosition, SW_SCAN_BLOCK, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        const qint64 nBlockSize = baBlock.size();
        if (nBlockSize <= 5) break;
        const bool bLast = (nBlockSize < SW_SCAN_BLOCK);
        const uchar *pBlock = reinterpret_cast<const uchar *>(baBlock.constData());
        const qint64 nLimit = bLast ? (nBlockSize - 6) : (nBlockSize - SW_SCAN_OVERLAP - 1);

        qint64 nFound = -1;
        bool bAbort = false;
        for (qint64 i = 0; (i <= nLimit) && !bAbort; i++) {
            const qint32 nNameLength = (static_cast<qint32>(pBlock[i]) << 8) | static_cast<qint32>(pBlock[i + 1]);
            if ((nNameLength < SW_MIN_NAME) || (nNameLength > SW_MAX_NAME)) continue;
            if (!swIsKnownPrefix(reinterpret_cast<const char *>(pBlock + i + 2))) continue;
            // In the final block a name that would run past EOF ends the walk;
            // in every other block the 256 byte tail guarantees it fits.
            if (bLast && ((nBlockSize - i - 2) < nNameLength)) {
                bAbort = true;
                break;
            }
            bool bNameOk = true;
            for (qint32 j = 0; j < nNameLength; j++) {
                if (!swIsValidNameByte(pBlock[i + 2 + j])) {
                    bNameOk = false;
                    break;
                }
            }
            if (bNameOk) {
                nFound = i;
                break;
            }
        }

        if (bAbort) break;
        if (nFound < 0) {
            if (bLast) break;
            nPosition += (nBlockSize - SW_SCAN_OVERLAP);
            continue;
        }
        nPosition += nFound;
        listOffsets.append(nPosition);
        if (listOffsets.size() > SW_MAX_MEMBERS) return false;
        nPosition += 7;
    }
    if (!isPdStructNotCanceled(pPdStruct)) return false;
    if (listOffsets.isEmpty()) return false;

    for (qint32 i = 0; i < listOffsets.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nOffset = listOffsets.at(i);
        const QByteArray baLength = read_array_process(nOffset, 2, pPdStruct);
        if (!guardedThis || !guardedSource || (baLength.size() != 2)) return false;
        const qint32 nNameLength = (static_cast<qint32>(static_cast<quint8>(baLength.at(0))) << 8) | static_cast<qint32>(static_cast<quint8>(baLength.at(1)));
        if ((nNameLength <= 0) || (nNameLength > SW_MAX_NAME)) break;
        const QByteArray baName = read_array_process(nOffset + 2, nNameLength, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        if (baName.size() != nNameLength) break;

        const qint64 nEnd = (i + 1 < listOffsets.size()) ? listOffsets.at(i + 1) : context.nInputSize;
        const qint64 nDataOffset = nOffset + 2 + nNameLength;
        const qint64 nSize = nEnd - nDataOffset;
        if (nSize < 0) break;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nDataOffset;
        member.nSize = nSize;
        member.sFileName = swRawNameToString(baName, i);
        context.listMembers.append(member);
    }
    if (context.listMembers.isEmpty()) return false;

    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XSW::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    // The 12 byte ASCII magic plus the two structural zero bytes is specific
    // enough on its own; the (potentially long) member scan is not needed to
    // answer "is this an inst image?".
    const bool bResult = parseContext(&context, true, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XSW::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSW archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSW::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSW(pDevice);
}

QList<QString> XSW::getSearchSignatures()
{
    return {QStringLiteral("'im001'")};
}

XBinary::FT XSW::getFileType()
{
    return FT_SW;
}

XBinary::MODE XSW::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSW::getEndian()
{
    return ENDIAN_BIG;
}

QString XSW::getArch()
{
    return QString();
}

QString XSW::getFileFormatExt()
{
    return QStringLiteral("sw");
}

QString XSW::getFileFormatExtsString()
{
    return QStringLiteral("IRIX software distribution (*.sw *.books *.man *.src *.idb)");
}

QString XSW::getMIMEString()
{
    return QStringLiteral("application/x-sgi-inst");
}

QString XSW::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, true, nullptr)) return QString();
    return context.sVersion;
}

qint64 XSW::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, true, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSW::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSW::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XSW::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSW::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, false, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = SW_HEADER_SIZE;
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
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = (member.nDataOffset - member.nHeaderOffset) + member.nSize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XSW::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSW::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSW> guardedThis(this);
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
    if (!parseContext(pContext, false, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("IRIX software distribution image"));
    pState->mapArchiveProperties.insert(FPART_PROP_VERSION, pContext->sVersion);
    pState->nCurrentOffset = pContext->listMembers.first().nHeaderOffset;
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

XBinary::ARCHIVERECORD XSW::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    // The container never compresses; a member that holds a compress(1) or
    // pack(1) stream keeps it, and that is what the reference extractor writes.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XSW::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
    } else {
        pState->nCurrentOffset = pContext->nInputSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XSW::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
