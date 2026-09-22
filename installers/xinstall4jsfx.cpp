/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xinstall4jsfx.h"

#include <new>

#include "xpe.h"

namespace {
// The container always sits AT the PE overlay; nothing is searched for.
const qint64 I4J_MIN_CONTAINER = 0x20;
// An install4j/exe4j launcher is a desktop program.  The ceiling only exists so
// a corrupt carrier cannot ask for an unbounded read.
const qint64 I4J_MAX_CONTAINER = Q_INT64_C(0x40000000);
// Stored D5 13 E4 E8 in front of the variable table...
const quint32 I4J_MAGIC_HEAD = 0xE8E413D5U;
// ...and the same four bytes reversed, E8 E4 13 D5, in front of the trailer.
const quint32 I4J_MAGIC_TAIL = 0xD513E4E8U;
// The first variable is the product name.
const qint32 I4J_KEY_PRODUCTNAME = 101;
// The variable that carries the ';'-terminated member name list.
const qint32 I4J_KEY_FILELIST = 2003;
// Both bounds are the reference implementation's own: the variable count and
// the first variable's length are each required to be inside (0, 0x400).
const qint32 I4J_MAX_COUNT = 0x400;
const qint32 I4J_MAX_FIRSTLENGTH = 0x400;
// ...and the name list itself is capped at 64 KiB there.
const qint32 I4J_MAX_NAMELIST = 0x10000;
// Ceilings this reader adds so a desynchronised walk stops instead of running
// the whole file.
const qint32 I4J_MAX_EXTRACOUNT = 0x10000;
const qint32 I4J_MAX_FILECOUNT = 65536;
const qint32 I4J_MAX_NAMELENGTH = 1024;

quint32 i4jRd32(const quint8 *pData)
{
    return (quint32)(pData[0] | ((quint32)pData[1] << 8) | ((quint32)pData[2] << 16) | ((quint32)pData[3] << 24));
}

quint32 i4jRd32BE(const quint8 *pData)
{
    return (quint32)(((quint32)pData[0] << 24) | ((quint32)pData[1] << 16) | ((quint32)pData[2] << 8) | pData[3]);
}

quint64 i4jRd64BE(const quint8 *pData)
{
    return (((quint64)i4jRd32BE(pData)) << 32) | (quint64)i4jRd32BE(pData + 4);
}

// A name is taken from the container byte for byte.  Only the characters that
// cannot survive a file system at all are refused, and a refused name falls
// back to the member's index - which is what the reference implementation
// already does for an empty one.
bool i4jIsUsableName(const QString &sName)
{
    if (sName.isEmpty() || (sName.length() > I4J_MAX_NAMELENGTH)) return false;

    for (qint32 i = 0; i < sName.length(); i++) {
        const ushort nCode = sName.at(i).unicode();
        if (nCode < 0x20) return false;
        if ((nCode == '*') || (nCode == '?') || (nCode == '<') || (nCode == '>') || (nCode == '|') || (nCode == '"') || (nCode == ':')) return false;
    }
    if (sName.startsWith(QLatin1Char('/')) || sName.startsWith(QLatin1Char('\\'))) return false;
    if (sName.contains(QLatin1String(".."))) return false;

    return true;
}
}  // namespace

XInstall4jSFX::XInstall4jSFX(QIODevice *pDevice) : XArchive(pDevice)
{
}

XInstall4jSFX::~XInstall4jSFX()
{
}

// Detection is the reference implementation's own predicate, no wider: the
// magic at the overlay, a bounded variable count, the first variable being key
// 101 and its length bounded too.  Measured over F:\ARC and F:\tests (73,838
// files) those sixteen bytes appear at an overlay in the 18 install4j/exe4j
// launchers of the corpus and nowhere else.
bool XInstall4jSFX::locateContainer(qint64 *pnContainerOffset, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pnContainerOffset || guardedSource->isSequential()) return false;

    const qint64 nInputSize = guardedSource->size();
    if (nInputSize < I4J_MIN_CONTAINER) return false;

    XPE pe(getDevice());
    if (!pe.isValid(pPdStruct)) return false;
    const qint64 nContainerOffset = pe.getOverlayOffset(pPdStruct);
    if ((nContainerOffset <= 0) || (nContainerOffset >= nInputSize)) return false;

    const qint64 nContainerSize = nInputSize - nContainerOffset;
    if ((nContainerSize < I4J_MIN_CONTAINER) || (nContainerSize > I4J_MAX_CONTAINER)) return false;

    const QByteArray baHead = read_array_process(nContainerOffset, 16, pPdStruct);
    if ((baHead.size() != 16)) return false;
    const quint8 *pHead = (const quint8 *)baHead.constData();

    if (i4jRd32(pHead) != I4J_MAGIC_HEAD) return false;

    const qint32 nVariableCount = (qint32)i4jRd32(pHead + 4);
    if ((nVariableCount <= 0) || (nVariableCount >= I4J_MAX_COUNT)) return false;

    if ((qint32)i4jRd32(pHead + 8) != I4J_KEY_PRODUCTNAME) return false;

    const qint32 nFirstLength = (qint32)i4jRd32(pHead + 12);
    if ((nFirstLength <= 0) || (nFirstLength >= I4J_MAX_FIRSTLENGTH)) return false;

    *pnContainerOffset = nContainerOffset;
    return true;
}

// Walk the two variable tables and keep the two variables that say anything
// about the payload: key 101 names the product, key 2003 lists the members.
bool XInstall4jSFX::parseVariables(CONTEXT *pContext, QByteArray *pbaNameList, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pContext || !pbaNameList) return false;

    qint64 nPosition = pContext->nContainerOffset + 4;
    qint32 nCount = 0;
    bool bNameListFound = false;

    for (qint32 nTable = 0; nTable < 2; nTable++) {
        if (nPosition > pContext->nInputSize - 4) return false;
        const QByteArray baCount = read_array_process(nPosition, 4, pPdStruct);
        if ((baCount.size() != 4)) return false;
        nCount = (qint32)i4jRd32((const quint8 *)baCount.constData());
        nPosition += 4;

        // The first table's count was already bounded by the detector; the
        // second one is bounded here so a desynchronised walk cannot spin.
        if (nCount < 0) return false;
        if ((nTable == 1) && (nCount > I4J_MAX_EXTRACOUNT)) return false;

        for (qint32 i = 0; i < nCount; i++) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            if (nPosition > pContext->nInputSize - 8) return false;

            const QByteArray baEntry = read_array_process(nPosition, 8, pPdStruct);
            if ((baEntry.size() != 8)) return false;
            const quint8 *pEntry = (const quint8 *)baEntry.constData();

            const qint32 nKey = (qint32)i4jRd32(pEntry);
            const qint32 nLength = (qint32)i4jRd32(pEntry + 4);
            if ((nKey < 0) || (nLength < 0)) return false;
            nPosition += 8;
            if ((qint64)nLength > pContext->nInputSize - nPosition) return false;

            if ((nTable == 0) && (nKey == I4J_KEY_PRODUCTNAME) && pContext->sProductName.isEmpty() && (nLength > 0)) {
                const QByteArray baValue = read_array_process(nPosition, nLength, pPdStruct);
                if ((baValue.size() != nLength)) return false;
                pContext->sProductName = QString::fromLatin1(baValue.constData(), baValue.size());
            } else if ((nTable == 0) && (nKey == I4J_KEY_FILELIST)) {
                if (nLength > I4J_MAX_NAMELIST) return false;
                // Two key 2003 variables would mean the walk is not reading the
                // table the format describes.
                if (bNameListFound) return false;
                *pbaNameList = read_array_process(nPosition, nLength, pPdStruct);
                if ((pbaNameList->size() != nLength)) return false;
                bNameListFound = true;
            }

            nPosition += nLength;
        }
    }

    if (!bNameListFound) return false;

    pContext->nRecordsOffset = nPosition;
    return true;
}

// One member per name in the key 2003 list, in that order.
bool XInstall4jSFX::parseMembers(CONTEXT *pContext, const QByteArray &baNameList, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pContext) return false;

    QList<QByteArray> listNames = baNameList.split(';');
    // The list is ';'-TERMINATED, so the split leaves one empty element behind
    // the last name.  An empty element anywhere else is a member the builder
    // wrote without a name, which the reference implementation replaces by the
    // member's index - not something to drop.
    if (!listNames.isEmpty() && listNames.last().isEmpty()) listNames.removeLast();
    if (listNames.isEmpty() || (listNames.size() > I4J_MAX_FILECOUNT)) return false;

    qint64 nPosition = pContext->nRecordsOffset;
    bool bHasTrailer = false;

    for (qint32 i = 0; i < listNames.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        // The size word plus the dword behind it: the reference implementation
        // reads both before it knows where the member's data starts, so a
        // member with fewer than eight bytes of container behind its size word
        // is outside the format.
        if (nPosition > pContext->nInputSize - 8) return false;

        const QByteArray baHeader = read_array_process(nPosition, 8, pPdStruct);
        if ((baHeader.size() != 8)) return false;
        const quint8 *pHeader = (const quint8 *)baHeader.constData();

        const qint64 nSize = (qint64)i4jRd32(pHeader);
        if (nSize > (qint64)0x7fffffff) return false;
        nPosition += 4;

        // Zero here is the later builds' pad word, and it is also what says a
        // trailer follows the last member.  Anything else is already the
        // member's first four bytes.
        if (i4jRd32(pHeader + 4) == 0) {
            bHasTrailer = true;
            nPosition += 4;
        }

        if (nSize > pContext->nInputSize - nPosition) return false;

        MEMBER member = {};
        member.nStreamOffset = nPosition;
        member.nStreamSize = nSize;
        member.bStored = false;

        const QByteArray baName = listNames.at(i);
        member.sFileName = QString::fromLatin1(baName.constData(), baName.size());
        if (!i4jIsUsableName(member.sFileName)) member.sFileName = QString::number(i);

        pContext->listMembers.append(member);
        nPosition += nSize;
    }

    if (bHasTrailer) {
        if (!parseTrailer(pContext, &nPosition, pPdStruct)) return false;
    }

    pContext->nArchiveSize = nPosition;
    return true;
}

// The big-endian trailer behind the last member.  Every carrier in the
// reference corpus declares a count of zero, so this walk exists to keep a
// non-empty trailer from being dropped silently; it refuses whatever it cannot
// bound.
bool XInstall4jSFX::parseTrailer(CONTEXT *pContext, qint64 *pnPosition, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pContext || !pnPosition) return false;

    qint64 nPosition = *pnPosition;
    if (nPosition > pContext->nInputSize - 8) return false;

    const QByteArray baHead = read_array_process(nPosition, 8, pPdStruct);
    if ((baHead.size() != 8)) return false;
    const quint8 *pHead = (const quint8 *)baHead.constData();

    if (i4jRd32(pHead) != I4J_MAGIC_TAIL) return false;

    const quint32 nCount = i4jRd32BE(pHead + 4);
    if (nCount > (quint32)I4J_MAX_FILECOUNT) return false;
    if ((qint64)nCount + (qint64)pContext->listMembers.size() > (qint64)I4J_MAX_FILECOUNT) return false;
    nPosition += 8;

    for (quint32 i = 0; i < nCount; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nPosition > pContext->nInputSize - 2) return false;

        QByteArray baLength = read_array_process(nPosition, 2, pPdStruct);
        if ((baLength.size() != 2)) return false;
        const qint32 nNameLength = (qint32)((((quint32)(quint8)baLength.at(0)) << 8) | (quint32)(quint8)baLength.at(1));
        if ((nNameLength <= 0) || (nNameLength > I4J_MAX_NAMELENGTH)) return false;
        nPosition += 2;

        if ((qint64)nNameLength > pContext->nInputSize - nPosition) return false;
        const QByteArray baName = read_array_process(nPosition, nNameLength, pPdStruct);
        if ((baName.size() != nNameLength)) return false;
        nPosition += nNameLength;

        if (nPosition > pContext->nInputSize - 8) return false;
        const QByteArray baSize = read_array_process(nPosition, 8, pPdStruct);
        if ((baSize.size() != 8)) return false;
        const quint64 nRawSize = i4jRd64BE((const quint8 *)baSize.constData());
        if (nRawSize > (quint64)Q_INT64_C(0x7fffffffffffffff)) return false;
        const qint64 nSize = (qint64)nRawSize;
        nPosition += 8;
        if (nSize > pContext->nInputSize - nPosition) return false;

        MEMBER member = {};
        member.nStreamOffset = nPosition;
        member.nStreamSize = nSize;
        member.bStored = true;
        member.sFileName = QString::fromLatin1(baName.constData(), baName.size());
        if (!i4jIsUsableName(member.sFileName)) member.sFileName = QString::number(pContext->listMembers.size());

        pContext->listMembers.append(member);
        nPosition += nSize;
    }

    *pnPosition = nPosition;
    return true;
}

bool XInstall4jSFX::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (!locateContainer(&context.nContainerOffset, pPdStruct)) return false;

    QByteArray baNameList;
    if (!parseVariables(&context, &baNameList, pPdStruct)) return false;
    if (!parseMembers(&context, baNameList, pPdStruct)) return false;
    if (context.listMembers.isEmpty()) return false;

    if (context.nArchiveSize > context.nInputSize) context.nArchiveSize = context.nInputSize;

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

void XInstall4jSFX::fillMemberProperties(const CONTEXT &context, qint32 nIndex, QMap<FPART_PROP, QVariant> *pMapProperties)
{
    const MEMBER &member = context.listMembers.at(nIndex);

    pMapProperties->insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    // The XOR is length preserving and the trailer members are stored, so the
    // member's one declared size is both of its sizes.
    pMapProperties->insert(FPART_PROP_COMPRESSEDSIZE, member.nStreamSize);
    pMapProperties->insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nStreamSize);
    if (member.bStored) {
        pMapProperties->insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        pMapProperties->insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    } else {
        pMapProperties->insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_XOR_88);
        pMapProperties->insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("XOR 0x88"));
    }
    // The container has no directory record and no attribute word: every entry
    // in the key 2003 list is a file the launcher writes.
    pMapProperties->insert(FPART_PROP_ISFOLDER, false);
}

bool XInstall4jSFX::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    qint64 nContainerOffset = 0;
    const bool bResult = locateContainer(&nContainerOffset, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XInstall4jSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XInstall4jSFX archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XInstall4jSFX::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XInstall4jSFX(pDevice);
}

XBinary::FT XInstall4jSFX::getFileType()
{
    return FT_INSTALL4J_SFX;
}

XBinary::MODE XInstall4jSFX::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XInstall4jSFX::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XInstall4jSFX::getArch()
{
    return QString();
}

QString XInstall4jSFX::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XInstall4jSFX::getFileFormatExtsString()
{
    return QStringLiteral("install4j/exe4j launcher (*.exe)");
}

QString XInstall4jSFX::getMIMEString()
{
    return QStringLiteral("application/x-install4j-launcher");
}

QString XInstall4jSFX::getVersion()
{
    return QString();
}

qint64 XInstall4jSFX::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XInstall4jSFX::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XInstall4jSFX::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XInstall4jSFX::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XInstall4jSFX::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nRecordsOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = context.listMembers.at(i).nStreamOffset;
            part.nFileSize = context.listMembers.at(i).nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = context.listMembers.at(i).sFileName;
            fillMemberProperties(context, i, &part.mapProperties);
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

QMap<XBinary::UNPACK_PROP, QVariant> XInstall4jSFX::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XInstall4jSFX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    if (!parseContext(pContext, pPdStruct) || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("install4j/exe4j launcher"));
    pState->nCurrentOffset = pContext->listMembers.at(0).nStreamOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XInstall4jSFX::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const qint32 nIndex = pState->nCurrentIndex;
    if (pState->nCurrentOffset != pContext->listMembers.at(nIndex).nStreamOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->listMembers.at(nIndex).nStreamOffset;
    result.nStreamSize = pContext->listMembers.at(nIndex).nStreamSize;
    fillMemberProperties(*pContext, nIndex, &result.mapProperties);
    return result;
}

bool XInstall4jSFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nStreamOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XInstall4jSFX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
