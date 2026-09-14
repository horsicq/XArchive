/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsbx.h"

#include <QDateTime>
#include <QPointer>
#include <QtEndian>

#include <new>

#include "xne.h"
#include "xpe.h"

namespace {
// "SB1\0" read as a little-endian dword.  The trailing NUL is part of the tag,
// not a terminator, so the four-byte form is the only correct comparison.
const quint32 SBX_SIGNATURE = 0x00314253;
const qint64 SBX_HEADER_SIZE = 0x0e;
// A record cannot be shorter than its own header plus the decoded-size dword
// plus one name byte.
const qint64 SBX_FIXED_OVERHEAD = 0x12;
const qint64 SBX_RECORDSIZE_OFFSET = 0x04;
const qint64 SBX_DOSDATE_OFFSET = 0x08;
const qint64 SBX_DOSTIME_OFFSET = 0x0a;
const qint64 SBX_ATTRIBUTES_OFFSET = 0x0c;
const qint64 SBX_NAMELENGTH_OFFSET = 0x0d;
// The DOS attribute bits the record byte can carry.
const quint8 SBX_ATTRIBUTE_READONLY = 0x01;
const quint8 SBX_ATTRIBUTE_HIDDEN = 0x02;
const quint8 SBX_ATTRIBUTE_SYSTEM = 0x04;
const quint8 SBX_ATTRIBUTE_ARCHIVE = 0x20;
// The authoring tool is a 32-bit desktop installer builder; these ceilings only
// stop a corrupt chain from asking for an unbounded read or an unbounded
// decode, they are not format limits.
const qint64 SBX_MAX_MEMBERS = 65536;
const qint64 SBX_MAX_MEMBER_SIZE = Q_INT64_C(0x20000000);
// A stray tag inside the stub's own data is common (four of the fifteen
// samples have one), so the fallback scan has to survive several misses; it
// does not have to survive a file engineered to hold thousands of them.
const qint32 SBX_MAX_CANDIDATES = 256;

bool sbxRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XSBX::XSBX(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSBX::~XSBX()
{
}

// Member names in the reach set are bare 8.3-style base names in plain ASCII,
// so this returns them byte for byte.  The escaping only matters for the
// directory-carrying archives the authoring tool documents but the reach set
// does not contain: a separator is normalized to '/', and anything the host
// filesystem cannot represent becomes %XX, which is reversible and - unlike
// folding to '_' - cannot collapse two distinct members onto one output file.
QString XSBX::sanitizeName(const QByteArray &baRaw)
{
    QString sResult;

    for (qint32 i = 0; i < baRaw.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baRaw.at(i));
        if ((nCharacter == '\\') || (nCharacter == '/')) {
            sResult.append(QLatin1Char('/'));
            continue;
        }
        const bool bSafe = (nCharacter >= 0x20) && (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != ':') && (nCharacter != '*') &&
                           (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') && (nCharacter != '>') && (nCharacter != '|');
        if (bSafe) {
            sResult.append(QChar(static_cast<ushort>(nCharacter)));
        } else {
            QString sHex = QString::number(nCharacter, 16).toUpper();
            while (sHex.size() < 2) sHex.prepend(QLatin1Char('0'));
            sResult.append(QLatin1Char('%'));
            sResult.append(sHex);
        }
    }

    return sResult;
}

// Every member of the reach set is LZHUF coded; there is no method field and no
// stored form.  A record with no bytes on either side is the one shape the
// codec cannot describe, so it takes the stored path - the same rule
// xdecompress.cpp already applies to ARJ's zero-length records.  No such record
// exists in the reach set, so that branch is inferred, not measured.
XBinary::HANDLE_METHOD XSBX::memberHandleMethod(const MEMBER &member)
{
    if ((member.nPackedSize == 0) && (member.nUncompressedSize == 0)) return HANDLE_METHOD_STORE;

    return HANDLE_METHOD_SBX_LZHUF;
}

void XSBX::fillRecordProperties(const MEMBER &member, QMap<FPART_PROP, QVariant> *pMapProperties)
{
    if (!pMapProperties) return;

    pMapProperties->insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    pMapProperties->insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
    pMapProperties->insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    pMapProperties->insert(FPART_PROP_HANDLEMETHOD, memberHandleMethod(member));
    pMapProperties->insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZHUF"));
    pMapProperties->insert(FPART_PROP_ISFOLDER, false);
    pMapProperties->insert(FPART_PROP_ISREADONLY, (member.nAttributes & SBX_ATTRIBUTE_READONLY) != 0);
    pMapProperties->insert(FPART_PROP_ISHIDDEN, (member.nAttributes & SBX_ATTRIBUTE_HIDDEN) != 0);
    pMapProperties->insert(FPART_PROP_ISSYSTEM, (member.nAttributes & SBX_ATTRIBUTE_SYSTEM) != 0);
    pMapProperties->insert(FPART_PROP_ISARCHIVE, (member.nAttributes & SBX_ATTRIBUTE_ARCHIVE) != 0);

    if (member.nDosDate != 0) {
        const QDateTime dtModified = dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
        if (dtModified.isValid()) {
            pMapProperties->insert(FPART_PROP_DATETIME, dtModified);
            pMapProperties->insert(FPART_PROP_MTIME, dtModified);
        }
    }
}

bool XSBX::readMember(qint64 nOffset, qint64 nInputSize, MEMBER *pMember, PDSTRUCT *pPdStruct)
{
    QPointer<XSBX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pMember || !guardedSource) return false;
    if (!sbxRangeWithin(nInputSize, nOffset, SBX_HEADER_SIZE)) return false;

    const QByteArray baHeader = read_array_process(nOffset, SBX_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != SBX_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    if (qFromLittleEndian<quint32>(pHeader) != SBX_SIGNATURE) return false;

    const qint64 nRecordSize = static_cast<qint64>(qFromLittleEndian<qint32>(pHeader + SBX_RECORDSIZE_OFFSET));
    const qint64 nNameLength = static_cast<qint64>(pHeader[SBX_NAMELENGTH_OFFSET]);
    // The reference implementation's own two structural rules: a name is never
    // empty, and the record has to be at least as long as its own overhead.
    if (nNameLength == 0) return false;
    if (nRecordSize < nNameLength + SBX_FIXED_OVERHEAD) return false;

    const qint64 nPackedSize = nRecordSize - nNameLength - SBX_FIXED_OVERHEAD;
    if (!sbxRangeWithin(nInputSize, nOffset + SBX_HEADER_SIZE, nNameLength + 4)) return false;

    const QByteArray baTail = read_array_process(nOffset + SBX_HEADER_SIZE, nNameLength + 4, pPdStruct);
    if (!guardedThis || !guardedSource || (baTail.size() != nNameLength + 4)) return false;

    const qint64 nUncompressedSize =
        static_cast<qint64>(qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(baTail.constData()) + nNameLength));
    if ((nUncompressedSize < 0) || (nUncompressedSize > SBX_MAX_MEMBER_SIZE)) return false;
    if (nPackedSize > SBX_MAX_MEMBER_SIZE) return false;

    MEMBER member = {};
    member.nHeaderOffset = nOffset;
    member.nDataOffset = nOffset + SBX_HEADER_SIZE + nNameLength + 4;
    member.nPackedSize = nPackedSize;
    member.nUncompressedSize = nUncompressedSize;
    member.nDosDate = qFromLittleEndian<quint16>(pHeader + SBX_DOSDATE_OFFSET);
    member.nDosTime = qFromLittleEndian<quint16>(pHeader + SBX_DOSTIME_OFFSET);
    member.nAttributes = pHeader[SBX_ATTRIBUTES_OFFSET];
    member.sFileName = sanitizeName(baTail.left(static_cast<qint32>(nNameLength)));
    if (member.sFileName.isEmpty()) return false;
    if (!sbxRangeWithin(nInputSize, member.nDataOffset, member.nPackedSize)) return false;
    // A member with bytes to produce and no bytes to produce them from is a
    // corrupt record, not an empty file.
    if ((member.nPackedSize == 0) && (member.nUncompressedSize != 0)) return false;

    *pMember = member;

    return true;
}

// The chain has no count and no terminator: landing on the last byte of the
// file IS the terminator.  That is also what makes the fallback scan safe, so
// this must never accept a short walk.
bool XSBX::walkChain(qint64 nStart, qint64 nInputSize, QList<MEMBER> *pListMembers, PDSTRUCT *pPdStruct)
{
    QPointer<XSBX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pListMembers || !guardedSource) return false;
    pListMembers->clear();
    if ((nStart < 0) || (nStart >= nInputSize)) return false;

    qint64 nOffset = nStart;

    while (nOffset < nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (pListMembers->size() >= SBX_MAX_MEMBERS) return false;

        MEMBER member = {};
        if (!readMember(nOffset, nInputSize, &member, pPdStruct) || !guardedThis || !guardedSource) return false;

        pListMembers->append(member);
        nOffset = member.nDataOffset + member.nPackedSize;
    }

    if ((nOffset != nInputSize) || pListMembers->isEmpty()) {
        pListMembers->clear();
        return false;
    }

    return true;
}

bool XSBX::locateContainer(qint64 *pnContainerOffset, PDSTRUCT *pPdStruct)
{
    QPointer<XSBX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pnContainerOffset || !guardedSource || guardedSource->isSequential()) return false;

    const qint64 nInputSize = guardedSource->size();
    if (nInputSize < SBX_HEADER_SIZE + SBX_FIXED_OVERHEAD) return false;

    // The carrier is an executable.  Refusing everything else keeps the scan
    // below from being reachable for data files at all.
    const QByteArray baMZ = read_array_process(0, 2, pPdStruct);
    if (!guardedThis || !guardedSource || (baMZ.size() != 2)) return false;
    if ((static_cast<quint8>(baMZ.at(0)) != 'M') || (static_cast<quint8>(baMZ.at(1)) != 'Z')) return false;

    QList<MEMBER> listMembers;

    // A PE carrier puts the chain exactly at the overlay, so try that first: it
    // costs one memory-map walk instead of a scan over the whole image.
    XPE pe(getDevice());
    if (pe.isValid(pPdStruct) && guardedThis && guardedSource) {
        const qint64 nOverlayOffset = pe.getOverlayOffset(pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        if ((nOverlayOffset > 0) && (nOverlayOffset < nInputSize)) {
            if (walkChain(nOverlayOffset, nInputSize, &listMembers, pPdStruct)) {
                *pnContainerOffset = nOverlayOffset;
                return true;
            }
            if (!guardedThis || !guardedSource) return false;
        }
    }
    if (!guardedThis || !guardedSource) return false;

    // NE carriers have no dependable overlay calculation, and a PE whose
    // sections do not account for the whole image would miss the offset above.
    char szTag[4] = {};
    szTag[0] = 'S';
    szTag[1] = 'B';
    szTag[2] = '1';
    szTag[3] = 0;

    qint64 nSearchOffset = 1;
    for (qint32 i = 0; i < SBX_MAX_CANDIDATES; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nSearchOffset >= nInputSize) break;

        const qint64 nCandidate = find_array(nSearchOffset, nInputSize - nSearchOffset, szTag, 4, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        if (nCandidate <= 0) break;

        if (walkChain(nCandidate, nInputSize, &listMembers, pPdStruct)) {
            *pnContainerOffset = nCandidate;
            return true;
        }
        if (!guardedThis || !guardedSource) return false;

        nSearchOffset = nCandidate + 1;
    }

    return false;
}

bool XSBX::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSBX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();

    if (!locateContainer(&context.nContainerOffset, pPdStruct) || !guardedThis || !guardedSource) return false;
    if (!walkChain(context.nContainerOffset, context.nInputSize, &context.listMembers, pPdStruct) || !guardedThis || !guardedSource) return false;

    // The chain closes on the last byte of the file by construction, so the
    // container plus its carrier is the whole file.
    context.nArchiveSize = context.nInputSize;

    *pContext = context;

    return isPdStructNotCanceled(pPdStruct);
}

bool XSBX::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    qint64 nContainerOffset = 0;
    const bool bResult = locateContainer(&nContainerOffset, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    return bResult;
}

bool XSBX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSBX archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XSBX::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XSBX(pDevice);
}

XBinary::FT XSBX::getFileType()
{
    return FT_SBX_SFX;
}

XBinary::MODE XSBX::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSBX::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSBX::getArch()
{
    return QString();
}

QString XSBX::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XSBX::getFileFormatExtsString()
{
    return QStringLiteral("SBX self-extracting archive (*.exe)");
}

QString XSBX::getMIMEString()
{
    return QStringLiteral("application/x-sbx-sfx");
}

// The record tag carries the container version and nothing else does.
QString XSBX::getVersion()
{
    return QStringLiteral("1");
}

qint64 XSBX::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSBX::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSBX::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XSBX::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSBX::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nContainerOffset;
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
            part.nFileSize = member.nPackedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            fillRecordProperties(member, &part.mapProperties);
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

QList<XBinary::FPART_PROP> XSBX::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD, FPART_PROP_REPORTEDMETHOD,
            FPART_PROP_ISFOLDER,     FPART_PROP_ISREADONLY,     FPART_PROP_ISHIDDEN,         FPART_PROP_ISSYSTEM,     FPART_PROP_ISARCHIVE,
            FPART_PROP_DATETIME,     FPART_PROP_MTIME};
}

QMap<XBinary::UNPACK_PROP, QVariant> XSBX::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSBX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSBX> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("SBX self-extracting archive"));
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

XBinary::ARCHIVERECORD XSBX::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    fillRecordProperties(member, &result.mapProperties);

    return result;
}

bool XSBX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XSBX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
