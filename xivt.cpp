/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xivt.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const quint32 IVT_MAGIC = 0x01045F3FU;
const quint16 IVT_BTREE_MAGIC = 0x293BU;
const quint16 IVT_PAGE_SIZE = 0x2000U;
const qint64 IVT_FILE_HEADER_SIZE = 8;
const qint64 IVT_BTREE_HEADER_SIZE = 48;
const qint64 IVT_NODE_HEADER_SIZE = 12;
const qint32 IVT_MAX_ENTRIES = 500000;
const qint32 IVT_MAX_PAGES = 65536;
const qint64 IVT_MSZIP_HEADER_SIZE = 12;
const qint64 IVT_MAX_MEMBER_SIZE = 0x40000000;  // 1 GB sanity cap

// Internal files that carry the title's own indexes rather than content.
const char *const IVT_SYSTEM_NAMES[] = {"AlinkInfo",   "AlinkList", "AlinkLookup", "GRPINF",  "KeywordInfo", "KeywordList",
                                        "KeywordLookup", "STRINGS", "TOCIDX",      "TOPICS",  "TitleInformation", "URLTREE",
                                        "charmap",     "ftindex",   "source.toc",  "stoplist"};
const qint32 IVT_SYSTEM_NAME_COUNT = qint32(sizeof(IVT_SYSTEM_NAMES) / sizeof(IVT_SYSTEM_NAMES[0]));

bool ivtIsSystemName(const QString &sName)
{
    for (qint32 i = 0; i < IVT_SYSTEM_NAME_COUNT; ++i) {
        if (sName == QLatin1String(IVT_SYSTEM_NAMES[i])) return true;
    }
    return false;
}

// Internal names routinely contain '>' (topic streams are ">00000001"), which
// no host file system accepts; the reference tool substitutes and so do we.
QString ivtSanitizeName(const QString &sName)
{
    QString sResult = sName;
    for (qint32 i = 0; i < sResult.size(); ++i) {
        const ushort nCharacter = sResult.at(i).unicode();
        if ((nCharacter < 0x20) || (nCharacter == '<') || (nCharacter == '>') || (nCharacter == ':') || (nCharacter == '"') ||
            (nCharacter == '/') || (nCharacter == '\\') || (nCharacter == '|') || (nCharacter == '?') || (nCharacter == '*')) {
            sResult[i] = QLatin1Char('_');
        }
    }
    return sResult;
}
}  // namespace

XIVT::XIVT(QIODevice *pDevice) : XArchive(pDevice)
{
}

XIVT::~XIVT()
{
}

bool XIVT::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XIVT> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < IVT_FILE_HEADER_SIZE + IVT_BTREE_HEADER_SIZE) return false;

    const QByteArray baFileHeader = read_array_process(0, IVT_FILE_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baFileHeader.size() != IVT_FILE_HEADER_SIZE)) return false;
    const uchar *pFileHeader = reinterpret_cast<const uchar *>(baFileHeader.constData());
    if (qFromLittleEndian<quint32>(pFileHeader) != IVT_MAGIC) return false;

    const qint64 nDirectoryOffset = qint64(static_cast<qint32>(qFromLittleEndian<quint32>(pFileHeader + 4)));
    if ((nDirectoryOffset < IVT_FILE_HEADER_SIZE) || (nDirectoryOffset > context.nInputSize - IVT_BTREE_HEADER_SIZE)) return false;
    context.nDirectoryOffset = nDirectoryOffset;

    const QByteArray baBtree = read_array_process(nDirectoryOffset, IVT_BTREE_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baBtree.size() != IVT_BTREE_HEADER_SIZE)) return false;
    const uchar *pBtree = reinterpret_cast<const uchar *>(baBtree.constData());
    if (qFromLittleEndian<quint16>(pBtree) != IVT_BTREE_MAGIC) return false;
    if (qFromLittleEndian<quint16>(pBtree + 4) != IVT_PAGE_SIZE) return false;

    const qint32 nTotalPages = static_cast<qint32>(qFromLittleEndian<quint32>(pBtree + 0x26));
    const quint16 nLevels = qFromLittleEndian<quint16>(pBtree + 0x2a);
    const qint32 nTotalEntries = static_cast<qint32>(qFromLittleEndian<quint32>(pBtree + 0x2c));
    if ((nTotalPages <= 0) || (nTotalPages > IVT_MAX_PAGES)) return false;
    if ((nTotalEntries <= 0) || (nTotalEntries > IVT_MAX_ENTRIES)) return false;
    context.nTotalEntries = nTotalEntries;

    const qint64 nPagesOffset = nDirectoryOffset + IVT_BTREE_HEADER_SIZE;
    if (qint64(nTotalPages) * IVT_PAGE_SIZE > context.nInputSize - nPagesOffset) return false;
    context.nDirectorySize = IVT_BTREE_HEADER_SIZE + qint64(nTotalPages) * IVT_PAGE_SIZE;

    qint32 nSeenEntries = 0;
    for (qint32 nPage = 0; nPage < nTotalPages; ++nPage) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        // Index pages carry no file entries; the reference tool steps over the
        // page whose ordinal equals the level count and reads the rest as
        // leaves.  Every title in the corpus is a single-level tree.
        if (nPage == qint32(nLevels)) continue;

        const qint64 nPageOffset = nPagesOffset + qint64(nPage) * IVT_PAGE_SIZE;
        const QByteArray baPage = read_array_process(nPageOffset, qint32(IVT_PAGE_SIZE), pPdStruct);
        if (!guardedThis || !guardedSource || (baPage.size() != qint32(IVT_PAGE_SIZE))) return false;
        const uchar *pPage = reinterpret_cast<const uchar *>(baPage.constData());
        const qint64 nUnused = qint64(qFromLittleEndian<quint16>(pPage));
        const qint32 nEntryCount = qint32(qFromLittleEndian<quint16>(pPage + 2));
        if (nUnused > IVT_PAGE_SIZE - IVT_NODE_HEADER_SIZE) return false;
        if (nSeenEntries + nEntryCount > nTotalEntries) return false;

        qint64 nPosition = IVT_NODE_HEADER_SIZE;
        for (qint32 i = 0; i < nEntryCount; ++i) {
            if (nPosition >= IVT_PAGE_SIZE) return false;
            const qint32 nNameLength = qint32(pPage[nPosition]);
            ++nPosition;
            if (nNameLength == 0) return false;
            if (nPosition + nNameLength > IVT_PAGE_SIZE) return false;
            const QString sInternalName = QString::fromLatin1(baPage.constData() + nPosition, nNameLength);
            nPosition += nNameLength;

            // Two LEB128 values: absolute file offset, then declared size.
            qint64 nValues[2] = {0, 0};
            for (qint32 v = 0; v < 2; ++v) {
                quint64 nValue = 0;
                qint32 nShift = 0;
                bool bDone = false;
                while (!bDone) {
                    if (nPosition >= IVT_PAGE_SIZE) return false;
                    const quint8 nByte = pPage[nPosition];
                    ++nPosition;
                    if (nShift > 28) return false;
                    nValue += quint64(nByte & 0x7fU) << nShift;
                    if ((nByte & 0x80U) == 0) bDone = true;
                    nShift += 7;
                }
                if (nValue > quint64(0x7fffffff)) return false;
                nValues[v] = qint64(nValue);
            }
            if (nPosition >= IVT_PAGE_SIZE) return false;
            if (pPage[nPosition] != 0) return false;
            ++nPosition;

            ++nSeenEntries;

            const qint64 nDataOffset = nValues[0];
            const qint64 nStreamSize = nValues[1];
            if (nStreamSize == 0) continue;
            if (ivtIsSystemName(sInternalName)) continue;
            if ((nDataOffset < 0) || (nStreamSize < 0) || (nStreamSize > IVT_MAX_MEMBER_SIZE)) return false;
            if (nDataOffset > context.nInputSize - nStreamSize) return false;

            MEMBER member = {};
            member.nDataOffset = nDataOffset;
            member.nStreamSize = nStreamSize;
            member.nUncompressedSize = nStreamSize;
            member.bCompressed = false;
            member.sInternalName = sInternalName;
            member.sFileName = ivtSanitizeName(sInternalName);
            if (member.sFileName.isEmpty()) return false;

            if (nStreamSize >= IVT_MSZIP_HEADER_SIZE) {
                const QByteArray baMemberHeader = read_array_process(nDataOffset, qint32(IVT_MSZIP_HEADER_SIZE), pPdStruct);
                if (!guardedThis || !guardedSource || (baMemberHeader.size() != IVT_MSZIP_HEADER_SIZE)) return false;
                if (baMemberHeader.startsWith("mszp") || baMemberHeader.startsWith("nszp")) {
                    const qint64 nDeclared =
                        qint64(qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baMemberHeader.constData()) + 4));
                    if (nDeclared > IVT_MAX_MEMBER_SIZE) return false;
                    member.bCompressed = true;
                    member.nUncompressedSize = nDeclared;
                }
            }

            context.listMembers.append(member);
        }

        // The entries plus the page's own free space must fill the page.
        if (nPosition + nUnused != IVT_PAGE_SIZE) return false;
    }

    if (nSeenEntries != nTotalEntries) return false;
    if (context.listMembers.isEmpty()) return false;
    if (!guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return true;
}

bool XIVT::isValid(PDSTRUCT *pPdStruct)
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

bool XIVT::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XIVT archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XIVT::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XIVT(pDevice);
}

XBinary::FT XIVT::getFileType()
{
    return FT_IVT;
}

XBinary::MODE XIVT::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XIVT::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XIVT::getArch()
{
    return QString();
}

QString XIVT::getFileFormatExt()
{
    return QStringLiteral("m20");
}

QString XIVT::getFileFormatExtsString()
{
    return QStringLiteral("Multimedia Viewer title (*.m20 *.mvb)");
}

QString XIVT::getMIMEString()
{
    return QStringLiteral("application/x-mediaview-title");
}

QString XIVT::getVersion()
{
    // 3F 5F 04 01: the WinHelp family's version/revision pair.
    return QStringLiteral("4.1");
}

qint64 XIVT::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XIVT::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XIVT::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XIVT::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XIVT::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = IVT_FILE_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }
    if ((nFileParts & FILEPART_TABLE) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_TABLE;
        part.nFileOffset = context.nDirectoryOffset;
        part.nFileSize = context.nDirectorySize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Directory");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, result.size())) break;
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nStreamSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, member.bCompressed ? HANDLE_METHOD_IVT : HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, member.bCompressed ? QStringLiteral("MSZIP") : QStringLiteral("Store"));
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
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XIVT::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XIVT::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XIVT> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Multimedia Viewer title; MSZIP and stored internal files"));
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

XBinary::ARCHIVERECORD XIVT::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, member.bCompressed ? HANDLE_METHOD_IVT : HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, member.bCompressed ? QStringLiteral("MSZIP") : QStringLiteral("Store"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (member.sInternalName != member.sFileName) {
        result.mapProperties.insert(FPART_PROP_FILECOMMENT, member.sInternalName);
    }
    return result;
}

bool XIVT::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XIVT::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
