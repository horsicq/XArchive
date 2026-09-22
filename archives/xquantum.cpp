/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xquantum.h"

#include <QtEndian>

#include <new>

#include "Algos/xdsquantumdecoder.h"

namespace {
const qint64 QUANTUM_HEADER_SIZE = 8;
const qint32 QUANTUM_MIN_WINDOW_BITS = 10;
const qint32 QUANTUM_MAX_WINDOW_BITS = 21;
const qint32 QUANTUM_OLD_VERSION_LIMIT = 0x17;
// The count field is 16 bit; nothing larger can be a real archive.
const qint32 QUANTUM_MAX_ENTRIES = 65535;
// The name and the packer tag are both length prefixed with the same 15 bit
// encoding, so 0x7fff is the producer ceiling for either.
const qint32 QUANTUM_MAX_STRING = 0x7fff;

bool quantumReadVarLength(const QByteArray &baData, qint64 *pnPosition, qint32 *pnValue)
{
    if (!pnPosition || !pnValue) return false;
    if ((*pnPosition < 0) || (*pnPosition >= baData.size())) return false;
    const quint8 nFirst = quint8(baData.at(qint32(*pnPosition)));
    (*pnPosition)++;
    if (nFirst & 0x80U) {
        if (*pnPosition >= baData.size()) return false;
        const quint8 nSecond = quint8(baData.at(qint32(*pnPosition)));
        (*pnPosition)++;
        *pnValue = qint32(((nFirst & 0x7fU) << 8) | nSecond);
    } else {
        *pnValue = qint32(nFirst);
    }
    return (*pnValue >= 0) && (*pnValue <= QUANTUM_MAX_STRING);
}
}  // namespace

XQuantum::XQuantum(QIODevice *pDevice) : XArchive(pDevice)
{
}

XQuantum::~XQuantum()
{
}

// The names are DOS paths (8.3, occasionally with a directory part).  Path
// separators and the Windows reserved punctuation are escaped as %XX rather
// than folded to '_': escaping is reversible and cannot collapse two distinct
// members onto one output file.
QString XQuantum::rawNameToString(const QByteArray &baName, qint32 nIndex)
{
    QString sResult;
    for (qint32 i = 0; i < baName.size(); i++) {
        const quint8 nCharacter = quint8(baName.at(i));
        const bool bSafe = (nCharacter > 0x20) && (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != '/') && (nCharacter != '\\') &&
                           (nCharacter != ':') && (nCharacter != '*') && (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') &&
                           (nCharacter != '>') && (nCharacter != '|');
        if (bSafe) {
            sResult.append(QLatin1Char(char(nCharacter)));
        } else {
            QString sHex = QString::number(nCharacter, 16).toUpper();
            if (sHex.size() < 2) sHex.prepend(QLatin1Char('0'));
            sResult.append(QLatin1Char('%'));
            sResult.append(sHex);
        }
    }
    if (sResult.isEmpty()) sResult = QStringLiteral("record%1").arg(nIndex);
    return sResult;
}

bool XQuantum::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // Header plus the smallest possible record plus at least one coded byte.
    if (context.nInputSize < QUANTUM_HEADER_SIZE + 12) return false;

    const QByteArray baHeader = read_array_process(0, QUANTUM_HEADER_SIZE, pPdStruct);
    if (baHeader.size() != QUANTUM_HEADER_SIZE) return false;
    const quint8 *pHeader = reinterpret_cast<const quint8 *>(baHeader.constData());

    if ((pHeader[0] != 'D') || (pHeader[1] != 'S') || (pHeader[2] != 0)) return false;
    context.nVersion = qint32(pHeader[3]);
    if (context.nVersion == 0) return false;
    const qint32 nNumberOfEntries = qint32(qFromLittleEndian<quint16>(pHeader + 4));
    if ((nNumberOfEntries < 1) || (nNumberOfEntries > QUANTUM_MAX_ENTRIES)) return false;
    context.nWindowBits = qint32(pHeader[6]);
    if ((context.nWindowBits < QUANTUM_MIN_WINDOW_BITS) || (context.nWindowBits > QUANTUM_MAX_WINDOW_BITS)) return false;
    context.nLevel = qint32(pHeader[7]);
    context.bOldVariant = (context.nVersion < QUANTUM_OLD_VERSION_LIMIT);

    // The directory is variable length, so it has to be walked rather than
    // indexed.  Read a bounded window of the body instead of the whole file:
    // 64 MiB is far past the worst case a 16 bit entry count can produce, and
    // it keeps isValid() from pulling a multi-gigabyte candidate into memory.
    const qint64 nRemaining = context.nInputSize - QUANTUM_HEADER_SIZE;
    const qint64 nBodyReadSize = qMin<qint64>(nRemaining, qint64(64) * 1024 * 1024);
    const QByteArray baBody = read_array_process(QUANTUM_HEADER_SIZE, nBodyReadSize, pPdStruct);
    if (baBody.size() != nBodyReadSize) return false;

    const qint64 nFixedSize = context.bOldVariant ? 10 : 8;
    qint64 nPosition = 0;
    for (qint32 i = 0; i < nNumberOfEntries; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;

        qint32 nNameSize = 0;
        if (!quantumReadVarLength(baBody, &nPosition, &nNameSize)) return false;
        if (nNameSize <= 0) return false;
        if (nNameSize > (baBody.size() - nPosition)) return false;
        const QByteArray baName = baBody.mid(qint32(nPosition), nNameSize);
        nPosition += nNameSize;
        for (qint32 j = 0; j < baName.size(); j++) {
            const quint8 nCharacter = quint8(baName.at(j));
            // Real names are printable DOS text; refusing control bytes is what
            // keeps a stray "DS\0" from parsing as a directory.
            if ((nCharacter < 0x20) || (nCharacter == 0x7f)) return false;
        }

        qint32 nExtraSize = 0;
        if (!quantumReadVarLength(baBody, &nPosition, &nExtraSize)) return false;
        if (nExtraSize > (baBody.size() - nPosition)) return false;
        nPosition += nExtraSize;

        if (nFixedSize > (baBody.size() - nPosition)) return false;
        const quint8 *pRecord = reinterpret_cast<const quint8 *>(baBody.constData()) + nPosition;

        MEMBER member = {};
        member.sFileName = rawNameToString(baName, i);
        member.nUncompressedSize = qint64(qFromLittleEndian<quint32>(pRecord));
        member.nDosTime = qFromLittleEndian<quint16>(pRecord + 4);
        member.nDosDate = qFromLittleEndian<quint16>(pRecord + 6);
        member.bHasCRC = context.bOldVariant;
        member.nCRC = context.bOldVariant ? qFromLittleEndian<quint16>(pRecord + 8) : 0;
        nPosition += nFixedSize;

        context.listMembers.append(member);
        context.listSizes.append(member.nUncompressedSize);
    }

    context.nStreamOffset = QUANTUM_HEADER_SIZE + nPosition;
    context.nStreamSize = context.nInputSize - context.nStreamOffset;
    // A solid stream always carries at least the 16 priming bits.
    if (context.nStreamSize < 2) return false;
    context.nArchiveSize = context.nInputSize;

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

QByteArray XQuantum::propertiesForMember(const CONTEXT &context, qint32 nIndex)
{
    return XDSQuantumDecoder::createProperties(context.nWindowBits, context.bOldVariant, nIndex, context.listSizes);
}

bool XQuantum::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XQuantum::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XQuantum archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XQuantum::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XQuantum(pDevice);
}

QList<QString> XQuantum::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'DS'00"));
    return listResult;
}

XBinary::FT XQuantum::getFileType()
{
    return FT_QUANTUM;
}

XBinary::MODE XQuantum::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XQuantum::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XQuantum::getArch()
{
    return QString();
}

QString XQuantum::getFileFormatExt()
{
    return QStringLiteral("pak");
}

QString XQuantum::getFileFormatExtsString()
{
    return QStringLiteral("Quantum archive (*.pak *.001)");
}

QString XQuantum::getMIMEString()
{
    return QStringLiteral("application/x-quantum-archive");
}

QString XQuantum::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return QString::number(context.nVersion);
}

qint64 XQuantum::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XQuantum::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XQuantum::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XQuantum::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XQuantum::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nStreamOffset;
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
            // Solid: every member's coded bytes are the whole body.
            part.nFileOffset = context.nStreamOffset;
            part.nFileSize = context.nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nStreamSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_QUANTUM);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Quantum"));
            // deliberately NOT FPART_PROP_ISSOLID - that property means "this
            // record is a substream of a 7z-style solid folder" and pulls in
            // a decode path needing STREAMUNPACKEDSIZE/SUBSTREAMOFFSET, which
            // Quantum does not have. Its solid stream is handled by the
            // decoder via FPART_PROP_COMPRESSPROPERTIES.
            part.mapProperties.insert(FPART_PROP_ISFOLDER, false);
            part.mapProperties.insert(FPART_PROP_WINDOWSIZE, qint64(1) << context.nWindowBits);
            const QByteArray baProperties = propertiesForMember(context, i);
            if (!baProperties.isEmpty()) {
                part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, baProperties);
            }
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = context.nStreamOffset;
        part.nFileSize = context.nStreamSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Solid stream");
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

QMap<XBinary::UNPACK_PROP, QVariant> XQuantum::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XQuantum::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Quantum archive"));
    pState->nCurrentOffset = pContext->nStreamOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
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

XBinary::ARCHIVERECORD XQuantum::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();
    if (pState->nCurrentOffset != pContext->nStreamOffset) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nStreamOffset;
    result.nStreamSize = pContext->nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_QUANTUM);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Quantum"));
    // NOT on the record: see the note in getFileParts. The solid layout is
    // the decoder's business, and claiming it here routes the member into
    // the 7z folder-slicing path, which needs properties this format has no
    // equivalent for and silently fails for every member.
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_WINDOWSIZE, qint64(1) << pContext->nWindowBits);
    const QDateTime dtModified = dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
    if (dtModified.isValid()) {
        result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
    }
    // The size table plus the window order is everything the decoder needs to
    // replay the members ahead of this one; without it the arm cannot know
    // where in the solid stream this member starts.
    const QByteArray baProperties = propertiesForMember(*pContext, pState->nCurrentIndex);
    if (!baProperties.isEmpty()) {
        result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, baProperties);
    }
    return result;
}

bool XQuantum::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->nStreamOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XQuantum::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
