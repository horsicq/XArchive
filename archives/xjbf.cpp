/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xjbf.h"

#include <QtEndian>

#include <algorithm>
#include <new>

namespace {
const qint64 JBF_DIR_ENTRY_SIZE = 31;
const qint32 JBF_NAME_SIZE = 13;
const qint64 JBF_MAX_UNCOMPRESSED_SIZE = 0x40000000;  // 1 GB sanity cap
// The eight bytes at +0 are the opening bytes of the first member's LZHUF
// stream. They are identical in every known archive and are what the reference implementation's
// detector keys on, so they are kept as a cheap first filter in
// front of the trailer arithmetic that actually proves the format.
const quint64 JBF_LEAD_SIGNATURE = Q_UINT64_C(0x5cb470b3303163e4);

bool jbfIsValidName(const QByteArray &baField)
{
    // Name occupies the full 13-byte field; it is NUL padded and must not be
    // empty.
    qint32 nLength = 0;
    while ((nLength < baField.size()) && (baField.at(nLength) != '\0')) {
        ++nLength;
    }
    if ((nLength == 0) || (nLength >= baField.size())) return false;
    for (qint32 i = nLength; i < baField.size(); ++i) {
        if (baField.at(i) != '\0') return false;
    }
    for (qint32 i = 0; i < nLength; ++i) {
        const char cCharacter = baField.at(i);
        const quint8 nCharacter = static_cast<quint8>(cCharacter);
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
        if ((cCharacter == '"') || (cCharacter == '*') || (cCharacter == '<') ||
            (cCharacter == '>') || (cCharacter == '?') ||
            (cCharacter == '|') || (cCharacter == ':') ||
            (cCharacter == '/') || (cCharacter == '\\')) {
            return false;
        }
    }
    return true;
}
}  // namespace

XJBF::XJBF(QIODevice *pDevice) : XArchive(pDevice)
{
}

XJBF::~XJBF()
{
}

bool XJBF::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (8 + JBF_DIR_ENTRY_SIZE + 1)) return false;

    const QByteArray baLead = read_array_process(0, 8, pPdStruct);
    if ((baLead.size() != 8)) return false;
    if (qFromLittleEndian<quint64>(
            reinterpret_cast<const uchar *>(baLead.constData())) !=
        JBF_LEAD_SIGNATURE) {
        return false;
    }

    const QByteArray baCount =
        read_array_process(context.nInputSize - 1, 1, pPdStruct);
    if ((baCount.size() != 1)) return false;
    const qint32 nCount = static_cast<quint8>(baCount.at(0));
    if (nCount == 0) return false;

    context.nDirSize = qint64(nCount) * JBF_DIR_ENTRY_SIZE;
    context.nDirOffset = context.nInputSize - (context.nDirSize + 1);
    if (context.nDirOffset <= 0) return false;

    const QByteArray baDir = read_array_process(
        context.nDirOffset, qint32(context.nDirSize), pPdStruct);
    if ((baDir.size() != qint32(context.nDirSize))) {
        return false;
    }

    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nEntryOffset = qint64(i) * JBF_DIR_ENTRY_SIZE;
        const uchar *pEntry =
            reinterpret_cast<const uchar *>(baDir.constData()) + nEntryOffset;

        const QByteArray baName =
            baDir.mid(qint32(nEntryOffset), JBF_NAME_SIZE);
        if (!jbfIsValidName(baName)) return false;

        MEMBER member = {};
        member.nDirOffset = context.nDirOffset + nEntryOffset;
        member.nCompressedSize = static_cast<qint64>(
            static_cast<qint32>(qFromLittleEndian<quint32>(pEntry + 0x0d)));
        member.nUncompressedSize = static_cast<qint64>(
            static_cast<qint32>(qFromLittleEndian<quint32>(pEntry + 0x11)));
        member.nDataOffset = static_cast<qint64>(
            static_cast<qint32>(qFromLittleEndian<quint32>(pEntry + 0x15)));
        member.nChecksum = qFromLittleEndian<quint16>(pEntry + 0x19);
        member.nDosTime = qFromLittleEndian<quint16>(pEntry + 0x1b);
        member.nDosDate = qFromLittleEndian<quint16>(pEntry + 0x1d);
        // The reference implementation rejects the archive when any of the three 32-bit fields is
        // negative; the same three are the ones that have to stay inside the
        // data area here.
        if ((member.nCompressedSize < 0) || (member.nUncompressedSize < 0) ||
            (member.nDataOffset < 0)) {
            return false;
        }
        if ((member.nUncompressedSize > JBF_MAX_UNCOMPRESSED_SIZE) ||
            (member.nCompressedSize > context.nDirOffset) ||
            (member.nDataOffset > context.nDirOffset) ||
            (member.nCompressedSize >
             (context.nDirOffset - member.nDataOffset))) {
            return false;
        }
        if ((member.nUncompressedSize > 0) && (member.nCompressedSize == 0)) {
            return false;
        }
        member.sFileName =
            QString::fromLatin1(baName.left(qstrnlen(baName.constData(),
                                                     JBF_NAME_SIZE)));
        context.listEntries.append(member);
    }

    // A headerless container needs the trailer arithmetic to close exactly:
    // taken in offset order the members must tile [0, dirOffset) with no gap
    // and no overlap.  A random file whose last byte happens to be a small
    // count does not survive this.
    QList<qint64> listRanges;
    for (const MEMBER &member : context.listEntries) {
        listRanges.append(member.nDataOffset);
    }
    std::sort(listRanges.begin(), listRanges.end());
    qint64 nExpected = 0;
    for (qint64 nOffset : listRanges) {
        if (nOffset != nExpected) return false;
        qint64 nSize = -1;
        for (const MEMBER &member : context.listEntries) {
            if (member.nDataOffset == nOffset) {
                nSize = member.nCompressedSize;
                break;
            }
        }
        if (nSize < 0) return false;
        nExpected = nOffset + nSize;
    }
    if (nExpected != context.nDirOffset) return false;

    *pContext = context;
    return true;
}

bool XJBF::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if ((nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XJBF::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XJBF archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XJBF::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XJBF(pDevice);
}

QList<QString> XJBF::getSearchSignatures()
{
    return {QStringLiteral("E4633130B370B45C")};
}

XBinary::FT XJBF::getFileType()
{
    return FT_JBF;
}

XBinary::MODE XJBF::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XJBF::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XJBF::getArch()
{
    return QString();
}

QString XJBF::getFileFormatExt()
{
    return QStringLiteral("jbf");
}

QString XJBF::getFileFormatExtsString()
{
    return QStringLiteral("JBF archive (*.1 *.jbf)");
}

QString XJBF::getMIMEString()
{
    return QStringLiteral("application/x-jbf");
}

QString XJBF::getVersion()
{
    // No version field exists anywhere in the container.
    return QStringLiteral("1");
}

qint64 XJBF::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XJBF::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XJBF::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_STREAM | FILEPART_TABLE, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

QList<XBinary::FPART> XJBF::getFileParts(quint32 nFileParts, qint32 nLimit,
                                         PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    for (const MEMBER &member : context.listEntries) {
        if (!isPdStructNotCanceled(pPdStruct)) break;
        if ((nLimit > 0) && (listResult.size() >= nLimit)) break;
        if (!(nFileParts & FILEPART_STREAM)) break;
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
                                  (member.nUncompressedSize == 0)
                                      ? HANDLE_METHOD_STORE
                                      : HANDLE_METHOD_HZL);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                  QStringLiteral("LZHUF"));
        const QDateTime dtMTime =
            dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
        if (dtMTime.isValid()) {
            part.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
        }
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_TABLE) &&
        ((nLimit <= 0) || (listResult.size() < nLimit))) {
        FPART part = {};
        part.filePart = FILEPART_TABLE;
        part.nFileOffset = context.nDirOffset;
        part.nFileSize = context.nDirSize + 1;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Directory");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) &&
        ((nLimit <= 0) || (listResult.size() < nLimit))) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XJBF::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XJBF::initUnpack(UNPACK_STATE *pState,
                      const QMap<UNPACK_PROP, QVariant> &mapProperties,
                      PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) {
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
    if (!parseContext(pContext, pPdStruct) || pContext->listEntries.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("JBF archive; trailing directory, LZHUF members"));
    pState->nCurrentOffset = pContext->listEntries.first().nDirOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listEntries.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XJBF::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext || (pState->nCurrentIndex >= pContext->listEntries.size())) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listEntries.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nDirOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                (member.nUncompressedSize == 0)
                                    ? HANDLE_METHOD_STORE
                                    : HANDLE_METHOD_HZL);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("LZHUF"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    const QDateTime dtMTime =
        dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
    if (dtMTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
    }
    return result;
}

bool XJBF::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listEntries.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listEntries.size()) {
        pState->nCurrentOffset =
            pContext->listEntries.at(pState->nCurrentIndex).nDirOffset;
    } else {
        pState->nCurrentOffset = pContext->nInputSize;
    }
    return (pState->nCurrentIndex < pContext->listEntries.size());
}

bool XJBF::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
