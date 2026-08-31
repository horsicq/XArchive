/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xsqz.h"

#include <QDir>
#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const qint64 SQZ_ARCHIVE_HEADER_SIZE = 8;
const quint8 SQZ_MIN_HEADER_LENGTH = 19;
const qint32 SQZ_MAX_MEMBERS = 100000;

quint32 sqzRead32(const char *pData)
{
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pData));
}

quint16 sqzRead16(const char *pData)
{
    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(pData));
}
}  // namespace

XSQZ::XSQZ(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBinary::HANDLE_METHOD XSQZ::methodToHandle(quint8 nMethod)
{
    switch (nMethod) {
        case 0: return HANDLE_METHOD_STORE;
        case 1: return HANDLE_METHOD_SQZ1;
        case 2: return HANDLE_METHOD_SQZ2;
        case 3: return HANDLE_METHOD_SQZ3;
        case 4: return HANDLE_METHOD_SQZ4;
        default: return HANDLE_METHOD_UNKNOWN;
    }
}

QString XSQZ::reportedMethod(quint8 nMethod)
{
    return (nMethod == 0) ? QStringLiteral("Store") :
                            QStringLiteral("SQZ method %1").arg(nMethod);
}

QString XSQZ::safeMemberName(const QByteArray &baName)
{
    if (baName.isEmpty() || baName.contains('\0')) return QString();

    QString sName = QString::fromLatin1(baName).normalized(QString::NormalizationForm_C);
    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (sName.isEmpty() || QDir::isAbsolutePath(sName) || sName.startsWith(QLatin1Char('/')) || sName.contains(QLatin1Char(':'))) return QString();

    const QStringList listParts = sName.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) || (sPart == QLatin1String(".."))) return QString();
    }

    return (XBinary::fixFileName(sName) == sName) ? sName : QString();
}

QDateTime XSQZ::dosDateTime(quint32 nValue)
{
    return XBinary::dosDateTimeToQDateTime(static_cast<quint16>(nValue >> 16), static_cast<quint16>(nValue));
}

bool XSQZ::parseInternalInfo(INTERNAL_INFO *pInfo, PDSTRUCT *pPdStruct)
{
    if (pInfo) *pInfo = INTERNAL_INFO();
    if (!pInfo || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSQZ> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedThis || !guardedSource || guardedSource->isSequential()) return false;

    const qint64 nFileSize = getSize();
    if (!guardedThis || !guardedSource ||
        (nFileSize < (SQZ_ARCHIVE_HEADER_SIZE + 1))) return false;

    const QByteArray baArchiveHeader = read_array_process(0, SQZ_ARCHIVE_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baArchiveHeader.size() != SQZ_ARCHIVE_HEADER_SIZE) ||
        (memcmp(baArchiveHeader.constData(), "HLSQZ", 5) != 0)) {
        return false;
    }

    const quint8 nVersion = static_cast<quint8>(baArchiveHeader.at(5));
    if ((nVersion < 0x20) || (nVersion > 0x7e)) return false;

    QList<MEMBER> listMembers;
    qint64 nOffset = SQZ_ARCHIVE_HEADER_SIZE;
    qint64 nArchiveSize = 0;
    bool bSawPostfix = false;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (!guardedThis || !guardedSource || (nOffset < SQZ_ARCHIVE_HEADER_SIZE) || (nOffset >= nFileSize)) return false;

        const quint8 nHeaderLength = read_uint8(nOffset);
        if (!guardedThis || !guardedSource) return false;

        if (nHeaderLength == 0) {
            if (!bSawPostfix) return false;
            nArchiveSize = nOffset + 1;
            break;
        }
        if (nHeaderLength < SQZ_MIN_HEADER_LENGTH) {
            // Types 1..18 are length-prefixed opaque archive blocks.  Type 3
            // is the documented HLSQZ postfix; unlike member records these
            // blocks carry no additive header checksum.
            if ((nFileSize - nOffset) < 3) return false;
            const quint16 nExtraSize = read_uint16(nOffset + 1);
            if (!guardedThis || !guardedSource ||
                (static_cast<qint64>(nExtraSize) >
                 (nFileSize - nOffset - 3))) return false;
            if (nHeaderLength == 3) {
                if (bSawPostfix || (nExtraSize != 5) ||
                    !compareSignature("'HLSQZ'", nOffset + 3))
                    return false;
                bSawPostfix = true;
            }
            nOffset += 3 + static_cast<qint64>(nExtraSize);
            continue;
        }

        if (listMembers.count() >= SQZ_MAX_MEMBERS) return false;

        const qint64 nHeaderSize = static_cast<qint64>(nHeaderLength) + 2;
        if (nHeaderSize > (nFileSize - nOffset)) return false;
        const QByteArray baHeader = read_array_process(nOffset, nHeaderSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baHeader.size() != nHeaderSize)) return false;

        quint8 nCalculatedCheck = 0;
        for (qint32 i = 2; i < baHeader.size(); ++i) nCalculatedCheck = static_cast<quint8>(nCalculatedCheck + static_cast<quint8>(baHeader.at(i)));
        if (nCalculatedCheck != static_cast<quint8>(baHeader.at(1))) return false;

        const qint32 nNameSize = static_cast<qint32>(nHeaderLength) - 18;
        if ((nNameSize <= 0) || ((20 + nNameSize) != nHeaderSize)) return false;
        const QString sName = safeMemberName(baHeader.mid(20, nNameSize));
        if (sName.isEmpty()) return false;

        const quint8 nFlags = static_cast<quint8>(baHeader.at(2));
        const quint8 nMethod = nFlags & 0x0fU;
        if (nMethod > 4) return false;
        const quint32 nCompressedSize = sqzRead32(baHeader.constData() + 3);
        const quint32 nUncompressedSize = sqzRead32(baHeader.constData() + 7);
        if ((nMethod == 0) && (nCompressedSize != nUncompressedSize)) return false;

        const qint64 nDataOffset = nOffset + nHeaderSize;
        if (static_cast<qint64>(nCompressedSize) > (nFileSize - nDataOffset)) return false;

        MEMBER member;
        member.nHeaderOffset = nOffset;
        member.nHeaderSize = nHeaderSize;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nCompressedSize;
        member.nUncompressedSize = nUncompressedSize;
        member.nMethod = nMethod;
        member.nAttributes = static_cast<quint8>(baHeader.at(15));
        member.nCRC32 = sqzRead32(baHeader.constData() + 16);
        member.nDosDateTime = (static_cast<quint32>(sqzRead16(baHeader.constData() + 13)) << 16) | sqzRead16(baHeader.constData() + 11);
        member.sFileName = sName;
        listMembers.append(member);

        nOffset = nDataOffset + static_cast<qint64>(nCompressedSize);
    }

    if (!guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct) || listMembers.isEmpty() || (nArchiveSize <= 0)) return false;

    pInfo->bIsValid = true;
    pInfo->nFileSize = nFileSize;
    pInfo->nArchiveSize = nArchiveSize;
    pInfo->nVersion = nVersion;
    pInfo->listMembers = listMembers;
    return true;
}

bool XSQZ::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *pSource = getDevice();
    const qint64 nSavedPosition = pSource ? pSource->pos() : -1;

    INTERNAL_INFO info;
    const bool bResult = parseInternalInfo(&info, pPdStruct);

    if (pSource && (nSavedPosition >= 0)) pSource->seek(nSavedPosition);
    return bResult;
}

bool XSQZ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSQZ archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSQZ::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSQZ(pDevice);
}

QList<QString> XSQZ::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'HLSQZ'");
}

XBinary::FT XSQZ::getFileType()
{
    return FT_SQZ;
}

XBinary::MODE XSQZ::getMode()
{
    return MODE_DATA;
}

qint32 XSQZ::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XSQZ::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSQZ::getArch()
{
    return QString();
}

QString XSQZ::getFileFormatExt()
{
    return QStringLiteral("sqz");
}

QString XSQZ::getFileFormatExtsString()
{
    return QStringLiteral("SQZ(sqz)");
}

QString XSQZ::getMIMEString()
{
    return QStringLiteral("application/x-sqz");
}

QString XSQZ::getVersion()
{
    return read_ansiString(5, 1);
}

qint64 XSQZ::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO info;
    return parseInternalInfo(&info, pPdStruct) ? info.nArchiveSize : 0;
}

XBinary::OSNAME XSQZ::getOsName()
{
    return OSNAME_MSDOS;
}

QMap<XBinary::UNPACK_PROP, QVariant> XSQZ::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSQZ::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSQZ> guardedThis(this);
    if (m_bUnpackOperationInProgress) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    SQZ_UNPACK_CONTEXT *pOldContext = static_cast<SQZ_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (!bindUnpackSource(pState, pPdStruct) || !guardedThis) return false;

    INTERNAL_INFO info;
    if (!parseInternalInfo(&info, pPdStruct) || !guardedThis) {
        if (guardedThis) releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    SQZ_UNPACK_CONTEXT *pContext = new (std::nothrow) SQZ_UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pContext->info = info;
    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = info.listMembers.count();
    pState->nCurrentOffset = info.listMembers.constFirst().nHeaderOffset;
    pState->nTotalSize = info.nFileSize;
    pState->mapUnpackProperties = mapProperties;

    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XSQZ::rawRecord(const MEMBER &member) const
{
    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandle(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, reportedMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    result.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
    result.mapProperties.insert(FPART_PROP_HEADER_OFFSET, member.nHeaderOffset);
    result.mapProperties.insert(FPART_PROP_HEADER_SIZE, member.nHeaderSize);
    result.mapProperties.insert(FPART_PROP_ENCRYPTED, false);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_FILEMODE, (member.nAttributes & 0x01) ? static_cast<quint32>(0444) : static_cast<quint32>(0644));
    result.mapProperties.insert(FPART_PROP_ISREADONLY, (member.nAttributes & 0x01) != 0);
    result.mapProperties.insert(FPART_PROP_ISHIDDEN, (member.nAttributes & 0x02) != 0);
    result.mapProperties.insert(FPART_PROP_ISSYSTEM, (member.nAttributes & 0x04) != 0);
    result.mapProperties.insert(FPART_PROP_ISARCHIVE, (member.nAttributes & 0x20) != 0);

    const QDateTime dateTime = dosDateTime(member.nDosDateTime);
    if (dateTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dateTime);
        result.mapProperties.insert(FPART_PROP_MTIME, dateTime);
    }

    return result;
}

XBinary::ARCHIVERECORD XSQZ::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XSQZ> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext) return ARCHIVERECORD();
    if (!isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) return ARCHIVERECORD();

    SQZ_UNPACK_CONTEXT *pContext = static_cast<SQZ_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->info.listMembers.count()) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) || (pState->nTotalSize != pContext->info.nFileSize)) {
        return ARCHIVERECORD();
    }

    return rawRecord(pContext->info.listMembers.at(pState->nCurrentIndex));
}

bool XSQZ::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XSQZ> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    SQZ_UNPACK_CONTEXT *pContext = static_cast<SQZ_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->info.listMembers.count()) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->info.nArchiveSize;
        return false;
    }

    pState->nCurrentOffset = pContext->info.listMembers.at(pState->nCurrentIndex).nHeaderOffset;
    return true;
}

bool XSQZ::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    SQZ_UNPACK_CONTEXT *pContext = static_cast<SQZ_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    delete pContext;
    return true;
}

QList<XBinary::FPART> XSQZ::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    INTERNAL_INFO info;
    if (!parseInternalInfo(&info, pPdStruct)) return listResult;

    for (const MEMBER &member : info.listMembers) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) || ((nLimit > 0) && (listResult.count() >= nLimit))) break;

        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nHeaderSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Header");
            listResult.append(part);
        }

        if ((nFileParts & FILEPART_STREAM) && ((nLimit <= 0) || (listResult.count() < nLimit))) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties = rawRecord(member).mapProperties;
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_OVERLAY) && (info.nArchiveSize < info.nFileSize) && ((nLimit <= 0) || (listResult.count() < nLimit))) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = info.nArchiveSize;
        part.nFileSize = info.nFileSize - info.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    return listResult;
}

QList<XBinary::FPART_PROP> XSQZ::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_STREAMOFFSET
                               << FPART_PROP_STREAMSIZE << FPART_PROP_HANDLEMETHOD << FPART_PROP_REPORTEDMETHOD << FPART_PROP_RESULTCRC << FPART_PROP_CRC_TYPE
                               << FPART_PROP_HEADER_OFFSET << FPART_PROP_HEADER_SIZE << FPART_PROP_ENCRYPTED << FPART_PROP_FILEMODE << FPART_PROP_ISFOLDER
                               << FPART_PROP_DATETIME << FPART_PROP_MTIME << FPART_PROP_ISREADONLY << FPART_PROP_ISHIDDEN << FPART_PROP_ISSYSTEM
                               << FPART_PROP_ISARCHIVE;
}
