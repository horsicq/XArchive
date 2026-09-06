/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xhlb.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 HLB_HEADER_SIZE = 8;
const qint64 HLB_ENTRY_SIZE = 18;
const qint64 HLB_NAME_SIZE = 14;
const qint64 HLB_NAME_FIELD_OFFSET = 4;
const qint64 HLB_BACKPOINTER_SIZE = 4;
const quint16 HLB_MAGIC = 0x04D2U;  // 1234
// The count field is 16 bit, so 65535 entries (~1.1 MiB of directory) is the
// hard producer limit; nothing larger can be a real archive.
const qint32 HLB_MAX_ENTRIES = 65535;

bool hlbRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 && nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

// The 14-byte name field is NUL padded.  The reference extractor stops at the
// first NUL and keeps everything before it verbatim; a NUL followed by a
// non-NUL never occurs in the 4230 members of the reference corpus, and
// rejecting that shape is one of the structural rules that keeps random data
// from parsing as a directory.
bool hlbIsValidRawName(const char *pName)
{
    if (pName[0] == 0) return false;
    bool bPadding = false;
    for (qint64 i = 0; i < HLB_NAME_SIZE; i++) {
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

XHlb::XHlb(QIODevice *pDevice) : XArchive(pDevice)
{
}

XHlb::~XHlb()
{
}

// Names are plain DOS 8.3 identifiers in the whole reference corpus, but the
// field is raw bytes, so path separators and the Windows reserved punctuation
// are escaped as %XX rather than folded to '_': escaping is reversible and
// cannot collapse two distinct members onto one output file.
QString XHlb::rawNameToString(const char *pRawName, qint32 nIndex)
{
    qint32 nLength = 0;
    while ((nLength < static_cast<qint32>(HLB_NAME_SIZE)) && (pRawName[nLength] != '\0')) nLength++;
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

bool XHlb::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XHlb> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // 8 header bytes + at least one payload byte + one entry + the back-pointer.
    if (context.nInputSize < HLB_HEADER_SIZE + 1 + HLB_ENTRY_SIZE + HLB_BACKPOINTER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, HLB_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != HLB_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    if (qFromLittleEndian<quint16>(pHeader) != HLB_MAGIC) return false;
    const qint32 nNumberOfEntries = static_cast<qint32>(qFromLittleEndian<quint16>(pHeader + 2));
    if ((nNumberOfEntries < 1) || (nNumberOfEntries > HLB_MAX_ENTRIES)) return false;
    context.nNumberOfEntries = nNumberOfEntries;

    context.nDirectoryOffset = static_cast<qint64>(qFromLittleEndian<quint32>(pHeader + 4));
    if (context.nDirectoryOffset <= HLB_HEADER_SIZE) return false;

    const qint64 nDirectorySize = static_cast<qint64>(nNumberOfEntries) * HLB_ENTRY_SIZE;
    // Header, payloads, directory and back-pointer must tile the file exactly.
    // This is the check that makes a two-byte magic safe to detect on: it rules
    // out both a coincidental 0x04D2 and an appended overlay.
    if (context.nDirectoryOffset + nDirectorySize + HLB_BACKPOINTER_SIZE != context.nInputSize) return false;

    const QByteArray baDirectory = read_array_process(context.nDirectoryOffset, nDirectorySize + HLB_BACKPOINTER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baDirectory.size() != nDirectorySize + HLB_BACKPOINTER_SIZE)) return false;
    const char *pDirectory = baDirectory.constData();

    // The directory repeats its own offset behind the last entry; the reference
    // extractor refuses the archive when the two disagree.
    if (static_cast<qint64>(qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pDirectory) + nDirectorySize)) != context.nDirectoryOffset) {
        return false;
    }

    qint64 nPreviousOffset = HLB_HEADER_SIZE - 1;
    for (qint32 i = 0; i < nNumberOfEntries; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const char *pEntry = pDirectory + (HLB_ENTRY_SIZE * i);
        const qint64 nOffset = static_cast<qint64>(qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pEntry)));
        if (nOffset < HLB_HEADER_SIZE) return false;
        // Sizes are implicit (next offset, or the directory offset for the last
        // member), so strictly ascending offsets are what guarantees every
        // member has a positive, non-overlapping extent.
        if (nOffset <= nPreviousOffset) return false;
        nPreviousOffset = nOffset;
        if (!hlbIsValidRawName(pEntry + HLB_NAME_FIELD_OFFSET)) return false;

        MEMBER member = {};
        member.nDataOffset = nOffset;
        member.sFileName = rawNameToString(pEntry + HLB_NAME_FIELD_OFFSET, i);
        context.listMembers.append(member);
    }
    if (nPreviousOffset >= context.nDirectoryOffset) return false;

    for (qint32 i = 0; i < nNumberOfEntries; i++) {
        const qint64 nEnd = (i + 1 < nNumberOfEntries) ? context.listMembers.at(i + 1).nDataOffset : context.nDirectoryOffset;
        MEMBER &member = context.listMembers[i];
        member.nSize = nEnd - member.nDataOffset;
        if ((member.nSize <= 0) || !hlbRangeWithin(context.nInputSize, member.nDataOffset, member.nSize)) return false;
    }

    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XHlb::isValid(PDSTRUCT *pPdStruct)
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

bool XHlb::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XHlb archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XHlb::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XHlb(pDevice);
}

QList<QString> XHlb::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("D204"));  // 0x04D2 little endian at offset 0
    return listResult;
}

XBinary::FT XHlb::getFileType()
{
    return FT_HLB;
}

XBinary::MODE XHlb::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XHlb::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XHlb::getArch()
{
    return QString();
}

QString XHlb::getFileFormatExt()
{
    return QStringLiteral("hlb");
}

QString XHlb::getFileFormatExtsString()
{
    return QStringLiteral("HLB library (*.hlb)");
}

QString XHlb::getMIMEString()
{
    return QStringLiteral("application/x-hlb");
}

QString XHlb::getVersion()
{
    return QString();
}

qint64 XHlb::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XHlb::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XHlb::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XHlb::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XHlb::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = HLB_HEADER_SIZE;
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
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = context.nDirectoryOffset;
        part.nFileSize = context.nInputSize - context.nDirectoryOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Directory");
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

QMap<XBinary::UNPACK_PROP, QVariant> XHlb::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XHlb::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XHlb> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("HLB library"));
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

XBinary::ARCHIVERECORD XHlb::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    // Nothing in the container is compressed; the .SON / .AGD payloads that the
    // members carry are inner formats and must stay untouched here.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XHlb::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->nDirectoryOffset;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XHlb::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
