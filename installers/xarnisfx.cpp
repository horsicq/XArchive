/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xarnisfx.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
// "ARNI" read as a little-endian dword.
const quint32 ARNI_SIGNATURE = 0x494E5241;
// The word that follows the tag in "ARNING", the tail of the English word
// WARNING.  The reference implementation refuses it by name and so does this.
const quint16 ARNI_ARNING_WORD = 0x474E;
// Tag plus the decoded-size dword.
const qint64 ARNI_HEADER_SIZE = 8;
// The end record is two tags and a CRLF; every header is classified from that
// many bytes, which a record header always has because a member is never empty
// and the end record always follows it.
const qint64 ARNI_PROBE_SIZE = 10;
// The reference implementation's own bound on the decoded size field.
const qint64 ARNI_MAX_MEMBER_SIZE = Q_INT64_C(0x1000000);
// A carrier smaller than an MZ header plus one complete record cannot hold a
// chain.
const qint64 ARNI_MIN_CARRIER = 0x40 + ARNI_HEADER_SIZE + ARNI_PROBE_SIZE;
// Ceilings that only stop a corrupt carrier from asking for an unbounded walk.
const qint32 ARNI_MAX_MEMBERS = 65536;
const qint32 ARNI_MAX_CANDIDATES = 64;
const qint32 ARNI_MAX_PROBES = 65536;
// The stub of every reference carrier is under 100 KiB; the ceiling only keeps
// the name-table scan off the size of an unrelated executable.
const qint64 ARNI_MAX_NAMETABLE_SCAN = Q_INT64_C(0x400000);
// A run of fewer than this many names is too easy to hit by accident in an
// executable's own data to be trusted as the stub's file-name pool.
const qint32 ARNI_MIN_NAMETABLE = 3;

bool arniRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XArniSFX::XArniSFX(QIODevice *pDevice) : XArchive(pDevice)
{
}

XArniSFX::~XArniSFX()
{
}

// Returns true for both shapes the chain can present at an offset: a member
// record, and the end record that closes the chain.
bool XArniSFX::classifyHeader(const QByteArray &baHeader, bool *pbTerminator, qint64 *pnUncompressedSize)
{
    if (!pbTerminator || !pnUncompressedSize) return false;
    if (baHeader.size() < ARNI_PROBE_SIZE) return false;

    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    if (qFromLittleEndian<quint32>(pHeader) != ARNI_SIGNATURE) return false;

    if (qFromLittleEndian<quint32>(pHeader + 4) == ARNI_SIGNATURE) {
        if ((pHeader[8] != 0x0D) || (pHeader[9] != 0x0A)) return false;
        *pbTerminator = true;
        *pnUncompressedSize = 0;

        return true;
    }

    if (qFromLittleEndian<quint16>(pHeader + 4) == ARNI_ARNING_WORD) return false;

    const qint64 nUncompressedSize = static_cast<qint64>(qFromLittleEndian<qint32>(pHeader + 4));
    if ((nUncompressedSize <= 0) || (nUncompressedSize >= ARNI_MAX_MEMBER_SIZE)) return false;

    *pbTerminator = false;
    *pnUncompressedSize = nUncompressedSize;

    return true;
}

// The next offset at or after nFrom that holds something the chain could use.
// A four-byte tag is short, so most hits in an executable are rejected here.
qint64 XArniSFX::findHeader(qint64 nFrom, qint64 nInputSize, PDSTRUCT *pPdStruct)
{
    QPointer<XArniSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return -1;

    char szTag[4];
    szTag[0] = 'A';
    szTag[1] = 'R';
    szTag[2] = 'N';
    szTag[3] = 'I';

    qint64 nOffset = nFrom;
    if (nOffset < 0) nOffset = 0;

    for (qint32 i = 0; i < ARNI_MAX_PROBES; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return -1;
        if (!arniRangeWithin(nInputSize, nOffset, ARNI_PROBE_SIZE)) return -1;

        const qint64 nCandidate = find_array(nOffset, nInputSize - nOffset, szTag, 4, pPdStruct);
        if (!guardedThis || !guardedSource) return -1;
        if (nCandidate < 0) return -1;
        if (!arniRangeWithin(nInputSize, nCandidate, ARNI_PROBE_SIZE)) return -1;

        const QByteArray baHeader = read_array_process(nCandidate, ARNI_PROBE_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baHeader.size() != ARNI_PROBE_SIZE)) return -1;

        bool bTerminator = false;
        qint64 nUncompressedSize = 0;
        if (classifyHeader(baHeader, &bTerminator, &nUncompressedSize)) return nCandidate;

        nOffset = nCandidate + 1;
    }

    return -1;
}

// A record does not carry its packed length, so the member's extent is the
// distance to the next header.  Landing on the end record IS the terminator:
// this must never accept a walk that stops anywhere else.
bool XArniSFX::walkChain(qint64 nStart, qint64 nInputSize, QList<MEMBER> *pListMembers, qint64 *pnChainEnd, PDSTRUCT *pPdStruct)
{
    QPointer<XArniSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pListMembers || !pnChainEnd || !guardedSource) return false;

    pListMembers->clear();
    *pnChainEnd = 0;
    if ((nStart < 0) || (nStart >= nInputSize)) return false;

    qint64 nOffset = nStart;

    while (true) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (pListMembers->size() >= ARNI_MAX_MEMBERS) return false;
        if (!arniRangeWithin(nInputSize, nOffset, ARNI_PROBE_SIZE)) return false;

        const QByteArray baHeader = read_array_process(nOffset, ARNI_PROBE_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baHeader.size() != ARNI_PROBE_SIZE)) return false;

        bool bTerminator = false;
        qint64 nUncompressedSize = 0;
        if (!classifyHeader(baHeader, &bTerminator, &nUncompressedSize)) return false;

        if (bTerminator) {
            if (pListMembers->isEmpty()) return false;
            *pnChainEnd = nOffset + ARNI_PROBE_SIZE;

            return true;
        }

        const qint64 nDataOffset = nOffset + ARNI_HEADER_SIZE;
        // A member always holds at least one packed byte, so the next header
        // cannot start before the byte after the data begins.
        const qint64 nNext = findHeader(nDataOffset + 1, nInputSize, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        if (nNext <= nDataOffset) return false;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nDataOffset;
        member.nPackedSize = nNext - nDataOffset;
        member.nUncompressedSize = nUncompressedSize;
        if (!arniRangeWithin(nInputSize, member.nDataOffset, member.nPackedSize)) return false;

        pListMembers->append(member);
        nOffset = nNext;
    }
}

bool XArniSFX::locateContainer(qint64 *pnContainerOffset, PDSTRUCT *pPdStruct)
{
    QPointer<XArniSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pnContainerOffset || !guardedSource || guardedSource->isSequential()) return false;

    const qint64 nInputSize = guardedSource->size();
    if (nInputSize < ARNI_MIN_CARRIER) return false;

    // The carrier is an executable.  Refusing everything else keeps the scan
    // below from being reachable for data files at all.
    const QByteArray baMZ = read_array_process(0, 2, pPdStruct);
    if (!guardedThis || !guardedSource || (baMZ.size() != 2)) return false;
    if ((static_cast<quint8>(baMZ.at(0)) != 'M') || (static_cast<quint8>(baMZ.at(1)) != 'Z')) return false;

    // The ten-byte end record is the cheapest thing that separates an ARNI
    // carrier from every other executable: over the 73,823-file reference sweep
    // it appeared in the ten ARNI carriers and nowhere else, so the candidate
    // walk below is only ever reached for a file that really has a chain.
    char szTerminator[10];
    szTerminator[0] = 'A';
    szTerminator[1] = 'R';
    szTerminator[2] = 'N';
    szTerminator[3] = 'I';
    szTerminator[4] = 'A';
    szTerminator[5] = 'R';
    szTerminator[6] = 'N';
    szTerminator[7] = 'I';
    szTerminator[8] = 0x0D;
    szTerminator[9] = 0x0A;

    const qint64 nTerminator = find_array(0, nInputSize, szTerminator, 10, pPdStruct);
    if (!guardedThis || !guardedSource) return false;
    if (nTerminator < 0) return false;

    QList<MEMBER> listMembers;
    qint64 nChainEnd = 0;
    qint64 nSearchOffset = 2;

    for (qint32 i = 0; i < ARNI_MAX_CANDIDATES; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;

        const qint64 nCandidate = findHeader(nSearchOffset, nInputSize, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        if (nCandidate < 0) break;

        if (walkChain(nCandidate, nInputSize, &listMembers, &nChainEnd, pPdStruct)) {
            *pnContainerOffset = nCandidate;

            return true;
        }
        if (!guardedThis || !guardedSource) return false;

        nSearchOffset = nCandidate + 1;
    }

    return false;
}

// A bare file name the stub could have passed to sprintf("%s\\%s", ...): plain
// ASCII, no space, no path separator, no wildcard, and an extension of one to
// four alphanumerics.  Anything looser starts matching the stub's format
// strings and its import table.
bool XArniSFX::isPlainFileName(const QByteArray &baToken)
{
    const qint32 nSize = baToken.size();
    if ((nSize < 3) || (nSize > 64)) return false;
    if (baToken.at(0) == '.') return false;

    qint32 nLastDot = -1;

    for (qint32 i = 0; i < nSize; i++) {
        const quint8 nCharacter = static_cast<quint8>(baToken.at(i));
        const bool bAlnum = ((nCharacter >= 'A') && (nCharacter <= 'Z')) || ((nCharacter >= 'a') && (nCharacter <= 'z')) ||
                            ((nCharacter >= '0') && (nCharacter <= '9'));
        bool bSafe = bAlnum;
        if (!bSafe) {
            bSafe = (nCharacter == '_') || (nCharacter == '.') || (nCharacter == '~') || (nCharacter == '!') || (nCharacter == '@') ||
                    (nCharacter == '#') || (nCharacter == '$') || (nCharacter == '&') || (nCharacter == '(') || (nCharacter == ')') ||
                    (nCharacter == '-') || (nCharacter == '{') || (nCharacter == '}') || (nCharacter == '\'') || (nCharacter == '+') ||
                    (nCharacter == ',') || (nCharacter == ';') || (nCharacter == '=');
        }
        if (!bSafe) return false;
        if (nCharacter == '.') nLastDot = i;
    }

    if ((nLastDot <= 0) || (nLastDot >= nSize - 1)) return false;
    if ((nSize - nLastDot - 1) > 4) return false;

    for (qint32 i = nLastDot + 1; i < nSize; i++) {
        const quint8 nCharacter = static_cast<quint8>(baToken.at(i));
        const bool bAlnum = ((nCharacter >= 'A') && (nCharacter <= 'Z')) || ((nCharacter >= 'a') && (nCharacter <= 'z')) ||
                            ((nCharacter >= '0') && (nCharacter <= '9'));
        if (!bAlnum) return false;
    }

    return true;
}

// The stub's file-name pool is a sequence of NUL terminated strings padded to
// an even length, so the run breaks on anything that is not a file name and on
// more than two NUL bytes in a row.  Exactly one run of the right length is
// accepted; two candidate runs mean the pool cannot be told from the noise and
// nothing is published.
QList<QString> XArniSFX::collectNameTable(const QByteArray &baStub, qint32 nCount)
{
    QList<QString> listResult;
    if ((nCount < ARNI_MIN_NAMETABLE) || baStub.isEmpty()) return listResult;

    QList<QByteArray> listCurrent;
    QList<QList<QByteArray> > listRuns;
    const qint32 nSize = baStub.size();
    qint32 i = 0;

    while (i < nSize) {
        qint32 nPadding = 0;
        while ((i < nSize) && (baStub.at(i) == '\0')) {
            i++;
            nPadding++;
        }
        if (i >= nSize) break;
        if ((nPadding > 2) && !listCurrent.isEmpty()) {
            listRuns.append(listCurrent);
            listCurrent.clear();
        }

        const qint32 nEnd = baStub.indexOf('\0', i);
        if (nEnd < 0) break;

        const QByteArray baToken = baStub.mid(i, nEnd - i);
        if (isPlainFileName(baToken)) {
            listCurrent.append(baToken);
        } else if (!listCurrent.isEmpty()) {
            listRuns.append(listCurrent);
            listCurrent.clear();
        }

        i = nEnd + 1;
    }

    if (!listCurrent.isEmpty()) listRuns.append(listCurrent);

    qint32 nMatchIndex = -1;
    qint32 nMatchCount = 0;

    for (qint32 nRun = 0; nRun < listRuns.size(); nRun++) {
        if (listRuns.at(nRun).size() == nCount) {
            nMatchIndex = nRun;
            nMatchCount++;
        }
    }

    if (nMatchCount != 1) return listResult;

    const QList<QByteArray> &listNames = listRuns.at(nMatchIndex);
    QList<QString> listLower;

    for (qint32 nName = 0; nName < listNames.size(); nName++) {
        const QString sName = QString::fromLatin1(listNames.at(nName));
        const QString sLower = sName.toLower();
        // The destination is a Windows directory, so two names that differ only
        // in case would still collide there.
        if (listLower.contains(sLower)) return QList<QString>();
        listLower.append(sLower);
        listResult.append(sName);
    }

    return listResult;
}

void XArniSFX::applyNames(qint64 nContainerOffset, QList<MEMBER> *pListMembers, PDSTRUCT *pPdStruct)
{
    if (!pListMembers) return;

    QPointer<XArniSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    QList<QString> listNames;

    if (guardedSource && (nContainerOffset > 0) && (nContainerOffset <= ARNI_MAX_NAMETABLE_SCAN)) {
        const QByteArray baStub = read_array_process(0, nContainerOffset, pPdStruct);
        if (guardedThis && guardedSource && (baStub.size() == nContainerOffset)) {
            listNames = collectNameTable(baStub, pListMembers->size());
        }
    }

    for (qint32 i = 0; i < pListMembers->size(); i++) {
        if (i < listNames.size()) {
            (*pListMembers)[i].sFileName = listNames.at(i);
        } else {
            // What the reference implementation publishes when it has no name,
            // which is always.
            (*pListMembers)[i].sFileName = QString("File_%1.bin").arg(i);
        }
    }
}

bool XArniSFX::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XArniSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();

    if (!locateContainer(&context.nContainerOffset, pPdStruct) || !guardedThis || !guardedSource) return false;

    qint64 nChainEnd = 0;
    if (!walkChain(context.nContainerOffset, context.nInputSize, &context.listMembers, &nChainEnd, pPdStruct) || !guardedThis || !guardedSource) {
        return false;
    }
    if (context.listMembers.isEmpty()) return false;

    applyNames(context.nContainerOffset, &context.listMembers, pPdStruct);
    if (!guardedThis || !guardedSource) return false;

    // The container ends on its own end record; the resource directory and the
    // rest of the carrier's data follow it.
    context.nArchiveSize = nChainEnd;

    *pContext = context;

    return isPdStructNotCanceled(pPdStruct);
}

void XArniSFX::fillRecordProperties(const MEMBER &member, QMap<FPART_PROP, QVariant> *pMapProperties)
{
    if (!pMapProperties) return;

    pMapProperties->insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    pMapProperties->insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
    pMapProperties->insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    pMapProperties->insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ARNI_LZHUF);
    pMapProperties->insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZHUF"));
    // The container has no directory record and no attribute or time field.
    pMapProperties->insert(FPART_PROP_ISFOLDER, false);
}

bool XArniSFX::isValid(PDSTRUCT *pPdStruct)
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

bool XArniSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XArniSFX archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XArniSFX::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XArniSFX(pDevice);
}

XBinary::FT XArniSFX::getFileType()
{
    return FT_ARNI_SFX;
}

XBinary::MODE XArniSFX::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XArniSFX::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XArniSFX::getArch()
{
    return QString();
}

QString XArniSFX::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XArniSFX::getFileFormatExtsString()
{
    return QStringLiteral("ARNI self-extracting installer (*.exe)");
}

QString XArniSFX::getMIMEString()
{
    return QStringLiteral("application/x-arni-sfx");
}

// Nothing in the container carries a version.
QString XArniSFX::getVersion()
{
    return QString();
}

qint64 XArniSFX::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XArniSFX::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XArniSFX::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XArniSFX::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XArniSFX::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XArniSFX::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD, FPART_PROP_REPORTEDMETHOD,
            FPART_PROP_ISFOLDER};
}

QMap<XBinary::UNPACK_PROP, QVariant> XArniSFX::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XArniSFX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XArniSFX> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("ARNI self-extracting installer"));
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

XBinary::ARCHIVERECORD XArniSFX::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XArniSFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XArniSFX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
