/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xwarpin.h"

#include <QDateTime>
#include <QtEndian>

#include <new>

namespace {
const quint32 WARPIN_MAGIC = 0xBE020477;
const quint16 WARPIN_MAX_VERSION = 5;  // the tool refuses anything from here up
const qint64 WARPIN_HEADER_SIZE = 0x214;
const qint64 WARPIN_VERSION_OFFSET = 0x04;
const qint64 WARPIN_PACKAGES_OFFSET = 0x20a;
const qint64 WARPIN_SCRIPTUNPACKED_OFFSET = 0x20c;
const qint64 WARPIN_SCRIPTPACKED_OFFSET = 0x20e;
const qint64 WARPIN_OPTIONAL_OFFSET = 0x210;
const qint64 WARPIN_PACKAGE_ENTRY_SIZE = 0x30;
const qint64 WARPIN_PACKAGE_LABEL_OFFSET = 0x10;
const qint64 WARPIN_PACKAGE_LABEL_SIZE = 0x20;
const qint64 WARPIN_MEMBER_HEADER_SIZE = 0x11d;
const quint16 WARPIN_MEMBER_MAGIC = 0xF012;
const qint64 WARPIN_MEMBER_METHOD_OFFSET = 0x04;
const qint64 WARPIN_MEMBER_PACKAGE_OFFSET = 0x06;
const qint64 WARPIN_MEMBER_UNPACKED_OFFSET = 0x08;
const qint64 WARPIN_MEMBER_PACKED_OFFSET = 0x0c;
const qint64 WARPIN_MEMBER_NAME_OFFSET = 0x14;
const qint64 WARPIN_MEMBER_NAME_SIZE = 0x100;
const qint64 WARPIN_MEMBER_MTIME_OFFSET = 0x114;
const qint64 WARPIN_MEMBER_TERMINATOR_OFFSET = 0x11c;
const quint16 WARPIN_METHOD_STORE = 0;
const quint16 WARPIN_METHOD_BZIP2 = 1;
// A package count is quint16, so the declared member total cannot exceed
// 65535 * 65535; this ceiling keeps a corrupt table from driving the walk.
const qint64 WARPIN_MAX_MEMBERS = 1000000;

bool warpinRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XWarpIn::XWarpIn(QIODevice *pDevice) : XArchive(pDevice)
{
}

XWarpIn::~XWarpIn()
{
}

// Member names are OS/2 paths with '\\' separators.  The separator is normalized
// to '/', everything the host filesystem cannot represent is escaped as %XX:
// escaping is reversible and, unlike folding to '_', cannot collapse two
// distinct members onto one output file.
QString XWarpIn::sanitizeName(const QByteArray &baRaw)
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

// The package label is display text only; it never reaches the filesystem, so
// unprintable bytes are dropped rather than escaped.
QString XWarpIn::labelString(const QByteArray &baRaw)
{
    QString sResult;
    for (qint32 i = 0; i < baRaw.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baRaw.at(i));
        if (nCharacter == 0) break;
        if ((nCharacter >= 0x20) && (nCharacter < 0x7f)) sResult.append(QChar(static_cast<ushort>(nCharacter)));
    }
    return sResult;
}

// A zero-length member carries no stream of any kind, whatever nMethod says.
// See the EMPTY MEMBERS note in xwarpin.h.
XBinary::HANDLE_METHOD XWarpIn::memberHandleMethod(const MEMBER &member)
{
    if ((member.nPackedSize == 0) && (member.nUnpackedSize == 0)) return HANDLE_METHOD_STORE;
    if (member.nMethod == WARPIN_METHOD_STORE) return HANDLE_METHOD_STORE;
    if (member.nMethod == WARPIN_METHOD_BZIP2) return HANDLE_METHOD_BZIP2;
    return HANDLE_METHOD_UNKNOWN;
}

QString XWarpIn::memberMethodName(const MEMBER &member)
{
    if (member.nMethod == WARPIN_METHOD_STORE) return QStringLiteral("Stored");
    if (member.nMethod == WARPIN_METHOD_BZIP2) return QStringLiteral("BZip2");
    return QString("Method %1").arg(member.nMethod);
}

void XWarpIn::fillRecordProperties(const MEMBER &member, QMap<FPART_PROP, QVariant> *pMapProperties)
{
    if (!pMapProperties) return;

    pMapProperties->insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    pMapProperties->insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
    pMapProperties->insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUnpackedSize);
    pMapProperties->insert(FPART_PROP_HANDLEMETHOD, memberHandleMethod(member));
    pMapProperties->insert(FPART_PROP_REPORTEDMETHOD, memberMethodName(member));
    pMapProperties->insert(FPART_PROP_ISFOLDER, false);
    if (!member.sPackage.isEmpty()) {
        pMapProperties->insert(FPART_PROP_INFO, member.sPackage);
    }
    if (member.nMTime) {
        const QDateTime dtModified = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(member.nMTime) * 1000, Qt::UTC);
        pMapProperties->insert(FPART_PROP_DATETIME, dtModified);
        pMapProperties->insert(FPART_PROP_MTIME, dtModified);
    }
}

bool XWarpIn::readMember(qint64 nOffset, const CONTEXT *pContext, MEMBER *pMember, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pContext || !pMember) return false;
    if (!warpinRangeWithin(pContext->nInputSize, nOffset, WARPIN_MEMBER_HEADER_SIZE)) return false;

    const QByteArray baRecord = read_array_process(nOffset, WARPIN_MEMBER_HEADER_SIZE, pPdStruct);
    if ((baRecord.size() != WARPIN_MEMBER_HEADER_SIZE)) return false;
    const uchar *pRecord = reinterpret_cast<const uchar *>(baRecord.constData());

    if (qFromLittleEndian<quint16>(pRecord) != WARPIN_MEMBER_MAGIC) return false;
    if (pRecord[WARPIN_MEMBER_TERMINATOR_OFFSET] != 0) return false;

    MEMBER member = {};
    member.nMethod = qFromLittleEndian<quint16>(pRecord + WARPIN_MEMBER_METHOD_OFFSET);
    member.nPackageId = qFromLittleEndian<quint16>(pRecord + WARPIN_MEMBER_PACKAGE_OFFSET);
    member.nUnpackedSize = static_cast<qint64>(qFromLittleEndian<qint32>(pRecord + WARPIN_MEMBER_UNPACKED_OFFSET));
    member.nPackedSize = static_cast<qint64>(qFromLittleEndian<qint32>(pRecord + WARPIN_MEMBER_PACKED_OFFSET));
    member.nMTime = qFromLittleEndian<quint32>(pRecord + WARPIN_MEMBER_MTIME_OFFSET);
    member.nDataOffset = nOffset + WARPIN_MEMBER_HEADER_SIZE;

    if ((member.nUnpackedSize < 0) || (member.nPackedSize < 0)) return false;
    // The only record with no packed bytes is the empty file; a non-empty
    // member whose stream is missing is a corrupt record, not an empty one.
    if ((member.nPackedSize == 0) && (member.nUnpackedSize != 0)) return false;
    // A stored member is a verbatim copy, so the two sizes must agree.
    if ((member.nMethod == WARPIN_METHOD_STORE) && (member.nPackedSize != member.nUnpackedSize)) return false;
    if (!warpinRangeWithin(pContext->nInputSize, member.nDataOffset, member.nPackedSize)) return false;

    qint64 nNameLength = 0;
    while ((nNameLength < WARPIN_MEMBER_NAME_SIZE) && (pRecord[WARPIN_MEMBER_NAME_OFFSET + nNameLength] != 0)) nNameLength++;
    // The writer NUL-terminates inside the field; a name that fills all 0x100
    // bytes has no terminator and is not a record.
    if ((nNameLength == 0) || (nNameLength >= WARPIN_MEMBER_NAME_SIZE)) return false;

    member.sFileName = sanitizeName(baRecord.mid(static_cast<qint32>(WARPIN_MEMBER_NAME_OFFSET), static_cast<qint32>(nNameLength)));
    if (member.sFileName.isEmpty()) return false;

    for (qint32 i = 0; i < pContext->listPackages.size(); i++) {
        if (pContext->listPackages.at(i).nId == member.nPackageId) {
            member.sPackage = pContext->listPackages.at(i).sLabel;
            break;
        }
    }

    *pMember = member;
    return true;
}

bool XWarpIn::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < WARPIN_HEADER_SIZE + WARPIN_MEMBER_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, WARPIN_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != WARPIN_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    if (qFromLittleEndian<quint32>(pHeader) != WARPIN_MAGIC) return false;

    context.nVersion = qFromLittleEndian<quint16>(pHeader + WARPIN_VERSION_OFFSET);
    if (context.nVersion >= WARPIN_MAX_VERSION) return false;

    const qint64 nPackages = static_cast<qint64>(qFromLittleEndian<quint16>(pHeader + WARPIN_PACKAGES_OFFSET));
    context.nScriptUnpacked = static_cast<qint64>(qFromLittleEndian<quint16>(pHeader + WARPIN_SCRIPTUNPACKED_OFFSET));
    context.nScriptPacked = static_cast<qint64>(qFromLittleEndian<quint16>(pHeader + WARPIN_SCRIPTPACKED_OFFSET));
    const qint64 nOptionalSize = static_cast<qint64>(qFromLittleEndian<qint32>(pHeader + WARPIN_OPTIONAL_OFFSET));

    if ((nPackages < 1) || (nOptionalSize < 0)) return false;
    if (context.nScriptPacked < 4) return false;

    context.nScriptOffset = WARPIN_HEADER_SIZE;
    if (!warpinRangeWithin(context.nInputSize, context.nScriptOffset, context.nScriptPacked)) return false;

    context.nPackageTableOffset = context.nScriptOffset + context.nScriptPacked + nOptionalSize;
    if (!warpinRangeWithin(context.nInputSize, context.nPackageTableOffset, nPackages * WARPIN_PACKAGE_ENTRY_SIZE)) return false;

    context.nMemberOffset = context.nPackageTableOffset + nPackages * WARPIN_PACKAGE_ENTRY_SIZE;
    if (!warpinRangeWithin(context.nInputSize, context.nMemberOffset, WARPIN_MEMBER_HEADER_SIZE)) return false;

    // The script is a bzip2 stream and its header is the cheapest structural
    // rule that separates a real .wpi from a file that merely opens with the
    // magic dword.
    const QByteArray baScriptMagic = read_array_process(context.nScriptOffset, 3, pPdStruct);
    if ((baScriptMagic.size() != 3)) return false;
    if (memcmp(baScriptMagic.constData(), "BZh", 3) != 0) return false;

    const QByteArray baTable = read_array_process(context.nPackageTableOffset, nPackages * WARPIN_PACKAGE_ENTRY_SIZE, pPdStruct);
    if ((baTable.size() != nPackages * WARPIN_PACKAGE_ENTRY_SIZE)) return false;

    qint64 nTotalMembers = 0;
    for (qint64 i = 0; i < nPackages; i++) {
        const uchar *pEntry = reinterpret_cast<const uchar *>(baTable.constData()) + i * WARPIN_PACKAGE_ENTRY_SIZE;
        PACKAGE package = {};
        package.nId = qFromLittleEndian<quint16>(pEntry);
        package.nFiles = qFromLittleEndian<quint16>(pEntry + 0x02);
        package.nDataOffset = static_cast<qint64>(qFromLittleEndian<qint32>(pEntry + 0x04));
        package.nTotalUnpacked = static_cast<qint64>(qFromLittleEndian<qint32>(pEntry + 0x08));
        package.nTotalPacked = static_cast<qint64>(qFromLittleEndian<qint32>(pEntry + 0x0c));
        package.sLabel = labelString(baTable.mid(static_cast<qint32>(i * WARPIN_PACKAGE_ENTRY_SIZE + WARPIN_PACKAGE_LABEL_OFFSET),
                                                static_cast<qint32>(WARPIN_PACKAGE_LABEL_SIZE)));
        nTotalMembers += package.nFiles;
        context.listPackages.append(package);
    }

    if ((nTotalMembers < 1) || (nTotalMembers > WARPIN_MAX_MEMBERS)) return false;
    // The first package always points at the head of the member chain; this is
    // the field that cross-checks the whole prefix arithmetic above.
    if (context.listPackages.first().nDataOffset != context.nMemberOffset) return false;

    qint64 nCurrent = context.nMemberOffset;
    for (qint64 i = 0; i < nTotalMembers; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        MEMBER member = {};
        if (!readMember(nCurrent, &context, &member, pPdStruct)) return false;
        context.listMembers.append(member);
        nCurrent = member.nDataOffset + member.nPackedSize;
    }

    context.nArchiveSize = nCurrent;
    if (context.nArchiveSize > context.nInputSize) return false;

    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XWarpIn::isValid(PDSTRUCT *pPdStruct)
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

bool XWarpIn::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XWarpIn archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XWarpIn::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XWarpIn(pDevice);
}

QList<QString> XWarpIn::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("770402BE"));
    return listResult;
}

XBinary::FT XWarpIn::getFileType()
{
    return FT_WARPIN;
}

XBinary::MODE XWarpIn::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XWarpIn::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XWarpIn::getArch()
{
    return QString();
}

QString XWarpIn::getFileFormatExt()
{
    return QStringLiteral("wpi");
}

QString XWarpIn::getFileFormatExtsString()
{
    return QStringLiteral("WarpIN package (*.wpi)");
}

QString XWarpIn::getMIMEString()
{
    return QStringLiteral("application/x-warpin");
}

QString XWarpIn::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return QString::number(context.nVersion);
}

qint64 XWarpIn::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XWarpIn::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XWarpIn::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XWarpIn::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XWarpIn::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nMemberOffset;
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
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nPackedSize;
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

QList<XBinary::FPART_PROP> XWarpIn::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD,
            FPART_PROP_REPORTEDMETHOD, FPART_PROP_ISFOLDER,     FPART_PROP_DATETIME,         FPART_PROP_INFO};
}

QMap<XBinary::UNPACK_PROP, QVariant> XWarpIn::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XWarpIn::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("WarpIN package"));
    pState->nCurrentOffset = pContext->listMembers.first().nDataOffset;
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

XBinary::ARCHIVERECORD XWarpIn::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XWarpIn::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XWarpIn::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
