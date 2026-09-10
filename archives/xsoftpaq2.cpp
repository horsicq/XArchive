/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsoftpaq2.h"

#include <QDateTime>
#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const qint64 SOFTPAQ2_MIN_SIZE = 0x40;
const qint64 SOFTPAQ2_PKLITE_OFFSET = 0x1e;
const qint64 SOFTPAQ2_LOCATOR_SIZE = 0x25;
const qint64 SOFTPAQ2_STORED_ENTRY_SIZE = 0x17;
const qint64 SOFTPAQ2_PACKED_ENTRY_SIZE = 0x26;
const qint64 SOFTPAQ2_NAME_SIZE = 8;
const qint64 SOFTPAQ2_EXT_SIZE = 4;
// 4 GiB of directory would be ~190 million members; anything close to that is
// not a SoftPaq.  The cap only guards the allocation, it is never reached.
const qint32 SOFTPAQ2_MAX_ENTRIES = 1000000;

const char SOFTPAQ2_PKLITE_BANNER[] = "PKLITE Copr. 199";
const qint64 SOFTPAQ2_PKLITE_BANNER_SIZE = 16;
const char SOFTPAQ2_LOCATOR_TAG[] = {'[', 'F', 'I', 'T', ']', 0, 1, 0};
const qint64 SOFTPAQ2_LOCATOR_TAG_SIZE = 8;

const quint16 SOFTPAQ2_METHOD_STORED = 0;
const quint16 SOFTPAQ2_METHOD_IMPLODE = 6;

bool softpaq2RangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XSoftPaq2::XSoftPaq2(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSoftPaq2::~XSoftPaq2()
{
}

// The 8-byte name and the 4-byte extension are space padded and the extension
// carries its own leading '.'.  Both are trimmed the way the reference
// extractor trims them (NUL terminates, surrounding spaces are dropped) and the
// two halves are then concatenated verbatim.  Anything that is not filesystem
// safe is escaped as %XX rather than folded to '_', so two distinct members can
// never collapse onto one output file.
QString XSoftPaq2::rawNameToString(const char *pName, const char *pExt, qint32 nIndex)
{
    QString sResult;

    for (qint32 nPart = 0; nPart < 2; nPart++) {
        const char *pField = (nPart == 0) ? pName : pExt;
        const qint32 nFieldSize = static_cast<qint32>((nPart == 0) ? SOFTPAQ2_NAME_SIZE : SOFTPAQ2_EXT_SIZE);

        qint32 nLength = 0;
        while ((nLength < nFieldSize) && (pField[nLength] != '\0')) nLength++;
        qint32 nStart = 0;
        while ((nStart < nLength) && (pField[nStart] == ' ')) nStart++;
        while ((nLength > nStart) && (pField[nLength - 1] == ' ')) nLength--;

        for (qint32 i = nStart; i < nLength; i++) {
            const quint8 nCharacter = static_cast<quint8>(pField[i]);
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
    }

    if (sResult.isEmpty()) {
        sResult = QStringLiteral("record%1").arg(nIndex);
    }

    return sResult;
}

XBinary::HANDLE_METHOD XSoftPaq2::methodToHandleMethod(quint16 nMethod)
{
    if (nMethod == SOFTPAQ2_METHOD_STORED) return HANDLE_METHOD_STORE;
    if (nMethod == SOFTPAQ2_METHOD_IMPLODE) {
        // The payload is handed over with its own two-byte prelude still
        // attached: the shared decoder reads the literal-mode and the
        // dictionary-bits byte off the front of the packed buffer itself.
        return HANDLE_METHOD_PKWARE_DCL_IMPLODE;
    }
    return HANDLE_METHOD_UNKNOWN;
}

QString XSoftPaq2::methodToString(quint16 nMethod)
{
    if (nMethod == SOFTPAQ2_METHOD_STORED) return QStringLiteral("Stored");
    if (nMethod == SOFTPAQ2_METHOD_IMPLODE) return QStringLiteral("PKWARE DCL Implode");
    return QStringLiteral("Unknown 0x%1").arg(nMethod, 4, 16, QLatin1Char('0'));
}

bool XSoftPaq2::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSoftPaq2> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < SOFTPAQ2_MIN_SIZE) return false;

    // Cheap gate first: the extractor stub is always a PKLITE compressed DOS
    // executable, so the banner sits at a fixed offset.  Only then is it worth
    // hunting for the directory locator.
    const QByteArray baStub = read_array_process(0, SOFTPAQ2_PKLITE_OFFSET + SOFTPAQ2_PKLITE_BANNER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baStub.size() != SOFTPAQ2_PKLITE_OFFSET + SOFTPAQ2_PKLITE_BANNER_SIZE)) return false;
    if ((baStub.at(0) != 'M') || (baStub.at(1) != 'Z')) return false;
    if (memcmp(baStub.constData() + SOFTPAQ2_PKLITE_OFFSET, SOFTPAQ2_PKLITE_BANNER, SOFTPAQ2_PKLITE_BANNER_SIZE) != 0) return false;

    bool bLocatorFound = false;
    qint64 nSearchOffset = 0;
    while (!bLocatorFound && (nSearchOffset + SOFTPAQ2_LOCATOR_SIZE <= context.nInputSize)) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nCandidate =
            find_array(nSearchOffset, context.nInputSize - nSearchOffset, SOFTPAQ2_LOCATOR_TAG, SOFTPAQ2_LOCATOR_TAG_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (nCandidate < 0)) break;
        nSearchOffset = nCandidate + 1;
        if (nCandidate + SOFTPAQ2_LOCATOR_SIZE > context.nInputSize) break;

        const QByteArray baLocator = read_array_process(nCandidate, SOFTPAQ2_LOCATOR_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baLocator.size() != SOFTPAQ2_LOCATOR_SIZE)) return false;
        const uchar *pLocator = reinterpret_cast<const uchar *>(baLocator.constData());

        // The locator repeats its own file offset; that is what makes a
        // free-floating anchor safe to search for.
        if (static_cast<qint64>(qFromLittleEndian<qint32>(pLocator + 0x10)) != nCandidate) continue;

        const qint64 nDirectoryOffset = static_cast<qint64>(qFromLittleEndian<qint32>(pLocator + 0x14));
        const qint64 nSplitOffset = static_cast<qint64>(qFromLittleEndian<qint32>(pLocator + 0x18));
        if ((nDirectoryOffset <= 0) || (nSplitOffset <= nDirectoryOffset) || (nSplitOffset > context.nInputSize)) continue;
        // Both tables tile their range exactly.
        if (((nSplitOffset - nDirectoryOffset) % SOFTPAQ2_STORED_ENTRY_SIZE) != 0) continue;
        if (((context.nInputSize - nSplitOffset) % SOFTPAQ2_PACKED_ENTRY_SIZE) != 0) continue;

        context.nLocatorOffset = nCandidate;
        context.nDirectoryOffset = nDirectoryOffset;
        context.nSplitOffset = nSplitOffset;
        bLocatorFound = true;
    }
    if (!bLocatorFound) return false;

    const qint64 nStoredCount = (context.nSplitOffset - context.nDirectoryOffset) / SOFTPAQ2_STORED_ENTRY_SIZE;
    const qint64 nPackedCount = (context.nInputSize - context.nSplitOffset) / SOFTPAQ2_PACKED_ENTRY_SIZE;
    if ((nStoredCount + nPackedCount) < 1) return false;
    if ((nStoredCount > SOFTPAQ2_MAX_ENTRIES) || (nPackedCount > SOFTPAQ2_MAX_ENTRIES)) return false;

    const qint64 nTableSize = context.nInputSize - context.nDirectoryOffset;
    const QByteArray baTable = read_array_process(context.nDirectoryOffset, nTableSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baTable.size() != nTableSize)) return false;
    const char *pTable = baTable.constData();

    qint32 nIndex = 0;
    for (qint64 i = 0; i < nStoredCount; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const char *pEntry = pTable + (i * SOFTPAQ2_STORED_ENTRY_SIZE);
        const uchar *pRaw = reinterpret_cast<const uchar *>(pEntry);
        if ((pRaw[0x08] != 0) || (pRaw[0x0d] != 0)) return false;
        const qint64 nSize = static_cast<qint64>(qFromLittleEndian<qint32>(pRaw + 0x0e));
        const qint64 nOffset = static_cast<qint64>(qFromLittleEndian<qint32>(pRaw + 0x12));
        if ((nSize < 0) || (nOffset < 0)) return false;
        if (!softpaq2RangeWithin(context.nInputSize, nOffset, nSize)) return false;
        // Entry 0 is the extractor stub itself; the publish flag is what keeps
        // it (and any other internal blob) out of the member list.
        if (pRaw[0x16] == 0) continue;

        MEMBER member = {};
        member.nDataOffset = nOffset;
        member.nCompressedSize = nSize;
        member.nUncompressedSize = nSize;
        member.nMethod = SOFTPAQ2_METHOD_STORED;
        member.bHasCRC = false;
        member.sFileName = rawNameToString(pEntry, pEntry + 0x09, nIndex);
        context.listMembers.append(member);
        nIndex++;
    }

    for (qint64 i = 0; i < nPackedCount; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const char *pEntry = pTable + (nStoredCount * SOFTPAQ2_STORED_ENTRY_SIZE) + (i * SOFTPAQ2_PACKED_ENTRY_SIZE);
        const uchar *pRaw = reinterpret_cast<const uchar *>(pEntry);
        if ((pRaw[0x08] != 0) || (pRaw[0x0d] != 0)) return false;
        const quint16 nMethod = qFromLittleEndian<quint16>(pRaw + 0x0e);
        if ((nMethod != SOFTPAQ2_METHOD_STORED) && (nMethod != SOFTPAQ2_METHOD_IMPLODE)) return false;
        const qint64 nUncompressedSize = static_cast<qint64>(qFromLittleEndian<qint32>(pRaw + 0x18));
        const qint64 nCompressedSize = static_cast<qint64>(qFromLittleEndian<qint32>(pRaw + 0x1c));
        const qint64 nOffset = static_cast<qint64>(qFromLittleEndian<qint32>(pRaw + 0x22));
        if ((nUncompressedSize < 0) || (nCompressedSize < 0) || (nOffset < 0)) return false;
        const qint64 nStreamSize = (nMethod == SOFTPAQ2_METHOD_STORED) ? nUncompressedSize : nCompressedSize;
        if (!softpaq2RangeWithin(context.nInputSize, nOffset, nStreamSize)) return false;

        MEMBER member = {};
        member.nDataOffset = nOffset;
        member.nCompressedSize = nStreamSize;
        member.nUncompressedSize = nUncompressedSize;
        member.nMethod = nMethod;
        // NOTE: this CRC-32 covers the STORED (still compressed) stream, not
        // the decompressed member - verified against the reference extractor on
        // the whole corpus.  It is therefore deliberately NOT published as
        // FPART_PROP_RESULTCRC, which the generic chain checks against the
        // UNPACKED bytes and would report as corruption for every member.
        member.nCRC32 = qFromLittleEndian<quint32>(pRaw + 0x14);
        member.bHasCRC = true;
        member.nDosTime = qFromLittleEndian<quint16>(pRaw + 0x10);
        member.nDosDate = qFromLittleEndian<quint16>(pRaw + 0x12);
        member.nAttributes = qFromLittleEndian<quint16>(pRaw + 0x20);
        member.sFileName = rawNameToString(pEntry, pEntry + 0x09, nIndex);
        context.listMembers.append(member);
        nIndex++;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = context.nInputSize;
    *pContext = context;

    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XSoftPaq2::isValid(PDSTRUCT *pPdStruct)
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

bool XSoftPaq2::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSoftPaq2 archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSoftPaq2::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSoftPaq2(pDevice);
}

QList<QString> XSoftPaq2::getSearchSignatures()
{
    QList<QString> listResult;
    // "MZ" plus the PKLITE banner at +0x1e; isValid then confirms the [FIT]
    // directory, which is what separates a SoftPaq from any other PKLITE EXE.
    listResult.append(QStringLiteral("'MZ'........................................................'PKLITE Copr. 199'"));
    return listResult;
}

XBinary::FT XSoftPaq2::getFileType()
{
    return FT_SOFTPAQ_2;
}

XBinary::MODE XSoftPaq2::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSoftPaq2::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSoftPaq2::getArch()
{
    return QString();
}

QString XSoftPaq2::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XSoftPaq2::getFileFormatExtsString()
{
    return QStringLiteral("SoftPaq distribution (*.exe)");
}

QString XSoftPaq2::getMIMEString()
{
    return QStringLiteral("application/x-softpaq");
}

QString XSoftPaq2::getVersion()
{
    return QString();
}

qint64 XSoftPaq2::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSoftPaq2::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSoftPaq2::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XSoftPaq2::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSoftPaq2::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = context.nLocatorOffset;
        part.nFileSize = SOFTPAQ2_LOCATOR_SIZE;
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
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XSoftPaq2::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSoftPaq2::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSoftPaq2> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("SoftPaq distribution"));
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

XBinary::ARCHIVERECORD XSoftPaq2::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (member.bHasCRC) {
        // member.nCRC32 is the CRC of the packed stream, see parseContext; only
        // the timestamp is publishable here.
        const QDateTime dtModified = dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
        if (dtModified.isValid()) result.mapProperties.insert(FPART_PROP_DATETIME, dtModified);
    }
    return result;
}

bool XSoftPaq2::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XSoftPaq2::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
