/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xbwcfarchive.h"

#include <QDir>
#include <QtEndian>

#include <new>

namespace {
const qint64 BWCF_FILE_HEADER_SIZE = 0x56;
const qint64 BWCF_DESCRIPTION_OFFSET = 0x05;
const qint64 BWCF_V1_NAME_FIELD_SIZE = 0x10c;
const qint64 BWCF_V2_TAG_SIZE = 5;
const qint64 BWCF_DESCRIPTOR_SIZE = 0x11;
const qint64 BWCF_BLOCK_PREFIX_SIZE = 4;
const quint8 BWCF_VERSION_MIN = 1;
const quint8 BWCF_VERSION_MAX = 2;
const quint8 BWCF_METHOD_LZHUF = 1;
const quint8 BWCF_METHOD_STORE = 3;
const qint32 BWCF_MAX_MEMBERS = 200000;
const qint32 BWCF_MAX_NAME_SIZE = 255;
const qint64 BWCF_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(512) * 1024 * 1024;

bool bwcfRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

QString bwcfCString(const QByteArray &baField)
{
    qint32 nLength = 0;
    const qint32 nSize = static_cast<qint32>(baField.size());
    while ((nLength < nSize) && (baField.at(nLength) != '\0')) nLength++;

    return QString::fromLatin1(baField.constData(), nLength);
}

// Both string halves come straight out of the container, so the only thing done
// to them is the separator conversion every path-carrying reader here performs,
// plus a traversal guard.  No component is dropped and nothing is renamed: the
// directory is load bearing because the container states it as the member's
// destination and dropping it would publish a name the archive does not carry.
// (Bare names happen to be unique within each of the 19 corpus archives, so this
// is a fidelity requirement, not a collision workaround; Clean.dat, Names.dat and
// Data.z occur in several directories ACROSS the corpus.)
QString bwcfCombinedName(const QString &sDirectory, const QString &sFileName)
{
    QString sCombined = sDirectory + sFileName;
    sCombined.replace(QLatin1Char('\\'), QLatin1Char('/'));
    sCombined = QDir::cleanPath(sCombined);

    if (sCombined.isEmpty()) return QString();
    if (sCombined.startsWith(QLatin1Char('/'))) return QString();
    if (sCombined.contains(QLatin1Char(':'))) return QString();
    if ((sCombined == QLatin1String("..")) || sCombined.startsWith(QLatin1String("../")) || sCombined.contains(QLatin1String("/../"))) return QString();

    return sCombined;
}
}  // namespace

XBWCFArchive::XBWCFArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBWCFArchive::~XBWCFArchive()
{
}

bool XBWCFArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (BWCF_FILE_HEADER_SIZE + BWCF_DESCRIPTOR_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, BWCF_FILE_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != BWCF_FILE_HEADER_SIZE)) return false;
    if (baHeader.left(4) != QByteArray("BWCF", 4)) return false;

    context.nVersion = static_cast<quint8>(baHeader.at(4));
    if ((context.nVersion < BWCF_VERSION_MIN) || (context.nVersion > BWCF_VERSION_MAX)) return false;

    // The reference gate also demands that the first description byte be a
    // printable character; an empty or binary description is not this format.
    const quint8 nFirstDescriptionByte = static_cast<quint8>(baHeader.at(static_cast<int>(BWCF_DESCRIPTION_OFFSET)));
    if ((nFirstDescriptionByte < 0x20) || (nFirstDescriptionByte >= 0x7f)) return false;

    context.sDescription = bwcfCString(baHeader.mid(static_cast<int>(BWCF_DESCRIPTION_OFFSET)));

    qint64 nOffset = BWCF_FILE_HEADER_SIZE;

    while ((nOffset < context.nInputSize) && (context.listMembers.size() < BWCF_MAX_MEMBERS) && isPdStructNotCanceled(pPdStruct)) {
        const qint64 nHeaderOffset = nOffset;
        QString sDirectory;
        QString sFileName;

        if (context.nVersion == 1) {
            if (!bwcfRangeWithin(context.nInputSize, nOffset, BWCF_V1_NAME_FIELD_SIZE)) return false;
            const QByteArray baName = read_array_process(nOffset, BWCF_V1_NAME_FIELD_SIZE, pPdStruct);
            if ((baName.size() != BWCF_V1_NAME_FIELD_SIZE)) return false;
            sFileName = bwcfCString(baName);
            nOffset += BWCF_V1_NAME_FIELD_SIZE;
        } else {
            if (!bwcfRangeWithin(context.nInputSize, nOffset, BWCF_V2_TAG_SIZE)) return false;
            const QByteArray baTag = read_array_process(nOffset, BWCF_V2_TAG_SIZE, pPdStruct);
            if ((baTag.size() != BWCF_V2_TAG_SIZE)) return false;
            if (baTag.left(4) != QByteArray("MFTS", 4)) return false;
            if (static_cast<quint8>(baTag.at(4)) != 0x02) return false;
            nOffset += BWCF_V2_TAG_SIZE;

            // The name comes first and the destination directory second; both
            // are u8-length prefixed and either may be empty.
            for (qint32 i = 0; i < 2; i++) {
                if (!bwcfRangeWithin(context.nInputSize, nOffset, 1)) return false;
                const QByteArray baLength = read_array_process(nOffset, 1, pPdStruct);
                if ((baLength.size() != 1)) return false;
                const qint64 nLength = static_cast<qint64>(static_cast<quint8>(baLength.at(0)));
                nOffset += 1;
                if (nLength > BWCF_MAX_NAME_SIZE) return false;
                QString sValue;
                if (nLength > 0) {
                    if (!bwcfRangeWithin(context.nInputSize, nOffset, nLength)) return false;
                    const QByteArray baValue = read_array_process(nOffset, nLength, pPdStruct);
                    if ((baValue.size() != nLength)) return false;
                    sValue = QString::fromLatin1(baValue);
                    nOffset += nLength;
                }
                if (i == 0) {
                    sFileName = sValue;
                } else {
                    sDirectory = sValue;
                }
            }
        }

        if (!bwcfRangeWithin(context.nInputSize, nOffset, BWCF_DESCRIPTOR_SIZE)) return false;
        const QByteArray baDescriptor = read_array_process(nOffset, BWCF_DESCRIPTOR_SIZE, pPdStruct);
        if ((baDescriptor.size() != BWCF_DESCRIPTOR_SIZE)) return false;
        const uchar *pDescriptor = reinterpret_cast<const uchar *>(baDescriptor.constData());
        nOffset += BWCF_DESCRIPTOR_SIZE;

        const qint64 nUncompressedSize = static_cast<qint64>(static_cast<qint32>(qFromLittleEndian<quint32>(pDescriptor + 0x04)));
        const qint64 nBlockSize = static_cast<qint64>(static_cast<qint32>(qFromLittleEndian<quint32>(pDescriptor + 0x08)));
        const quint32 nReserved = qFromLittleEndian<quint32>(pDescriptor + 0x0c);
        const quint8 nMethod = static_cast<quint8>(baDescriptor.at(0x10));

        if ((nUncompressedSize < 0) || (nBlockSize < 0) || (nReserved != 0)) return false;
        if (nUncompressedSize > BWCF_MAX_UNCOMPRESSED_SIZE) return false;
        if (nBlockSize < BWCF_BLOCK_PREFIX_SIZE) return false;
        if (!bwcfRangeWithin(context.nInputSize, nOffset, nBlockSize)) return false;
        if ((nMethod != BWCF_METHOD_LZHUF) && (nMethod != BWCF_METHOD_STORE)) return false;

        // The block opens with a repeat of the uncompressed size; the reference
        // rejects a record whose two copies disagree, and so does this.
        const QByteArray baBlockPrefix = read_array_process(nOffset, BWCF_BLOCK_PREFIX_SIZE, pPdStruct);
        if ((baBlockPrefix.size() != BWCF_BLOCK_PREFIX_SIZE)) return false;
        const qint64 nRepeatedSize = static_cast<qint64>(static_cast<qint32>(qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baBlockPrefix.constData()))));
        if (nRepeatedSize != nUncompressedSize) return false;
        if ((nMethod == BWCF_METHOD_STORE) && (nBlockSize != (nUncompressedSize + BWCF_BLOCK_PREFIX_SIZE))) return false;

        MEMBER member = {};
        member.nHeaderOffset = nHeaderOffset;
        member.nHeaderSize = nOffset - nHeaderOffset;
        member.nDataOffset = nOffset + BWCF_BLOCK_PREFIX_SIZE;
        member.nCompressedSize = nBlockSize - BWCF_BLOCK_PREFIX_SIZE;
        member.nUncompressedSize = nUncompressedSize;
        member.nMethod = nMethod;
        member.nDosTime = qFromLittleEndian<quint16>(pDescriptor + 0x00);
        member.nDosDate = qFromLittleEndian<quint16>(pDescriptor + 0x02);
        member.sFileName = bwcfCombinedName(sDirectory, sFileName);
        if (member.sFileName.isEmpty()) return false;

        context.listMembers.append(member);
        nOffset += nBlockSize;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = nOffset;
    if (context.nArchiveSize > context.nInputSize) context.nArchiveSize = context.nInputSize;
    *pContext = context;

    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XBWCFArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XBWCFArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBWCFArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XBWCFArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XBWCFArchive(pDevice);
}

QList<QString> XBWCFArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'BWCF'");
}

XBinary::FT XBWCFArchive::getFileType()
{
    return FT_BWCF;
}

XBinary::MODE XBWCFArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XBWCFArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XBWCFArchive::getArch()
{
    return QString();
}

qint32 XBWCFArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XBWCFArchive::getFileFormatExt()
{
    return QStringLiteral("set");
}

QString XBWCFArchive::getFileFormatExtsString()
{
    return QStringLiteral("BWCF distribution set (*.set)");
}

QString XBWCFArchive::getMIMEString()
{
    return QStringLiteral("application/x-bwcf");
}

QString XBWCFArchive::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();

    return QString::number(context.nVersion);
}

qint64 XBWCFArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XBWCFArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XBWCFArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);

    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XBWCFArchive::methodToString(quint8 nMethod)
{
    if (nMethod == BWCF_METHOD_STORE) return QStringLiteral("Stored");
    if (nMethod == BWCF_METHOD_LZHUF) return QStringLiteral("LZHUF");

    return QStringLiteral("Unknown (%1)").arg(nMethod);
}

XBinary::HANDLE_METHOD XBWCFArchive::methodToHandleMethod(quint8 nMethod)
{
    if (nMethod == BWCF_METHOD_STORE) return HANDLE_METHOD_STORE;
    if (nMethod == BWCF_METHOD_LZHUF) return HANDLE_METHOD_BWCF_LZHUF;

    return HANDLE_METHOD_UNKNOWN;
}

bool XBWCFArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XBWCFArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = BWCF_FILE_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    const qint32 nCount = static_cast<qint32>(context.listMembers.size());
    for (qint32 i = 0; i < nCount; i++) {
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
    if ((nFileParts & FILEPART_OVERLAY) && (context.nArchiveSize < context.nInputSize) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XBWCFArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XBWCFArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    if (!pContext->sDescription.isEmpty()) pState->mapArchiveProperties.insert(FPART_PROP_INFO, pContext->sDescription);
    pState->nCurrentOffset = pContext->listMembers.at(0).nHeaderOffset;
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

XBinary::ARCHIVERECORD XBWCFArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (isValidDosDateTime(member.nDosDate, member.nDosTime)) {
        result.mapProperties.insert(FPART_PROP_MTIME, dosDateTimeToQDateTime(member.nDosDate, member.nDosTime));
    }

    return result;
}

bool XBWCFArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;

    return false;
}

bool XBWCFArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XBWCFArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER << FPART_PROP_MTIME;
}
