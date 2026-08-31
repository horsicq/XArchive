/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xlegacyencoded.h"

#include <QPointer>
#include <QRegularExpression>
#include <QtEndian>

#include <limits>
#include <memory>
#include <new>

namespace
{
bool checkedAppend(QByteArray *pData, char value, qint64 nCount,
                   qint64 nLimit)
{
    if (!pData || (nCount < 0) || (pData->size() > nLimit) ||
        (nCount > (nLimit - pData->size())) ||
        (nCount > ((std::numeric_limits<int>::max)() - pData->size())))
        return false;
    pData->append(QByteArray((int)nCount, value));
    return true;
}

QString safeBaseName(const QString &sName)
{
    QString sResult = XBinary::fixFileName(sName);
    if (sResult.isEmpty() || sResult.contains(QLatin1Char('/')))
        sResult = QStringLiteral("decoded");
    return sResult;
}
} // namespace

XLegacyEncoded::XLegacyEncoded(QIODevice *pDevice, FT fileTypeHint)
    : XArchive(pDevice), m_fileTypeHint(fileTypeHint)
{
}

quint16 XLegacyEncoded::crc16CCITT(const char *pData, qint64 nSize)
{
    quint16 nCRC = 0;
    for (qint64 i = 0; i < nSize; ++i)
    {
        nCRC ^= static_cast<quint16>(static_cast<quint8>(pData[i])) << 8;
        for (qint32 j = 0; j < 8; ++j)
            nCRC = (nCRC & 0x8000U)
                       ? static_cast<quint16>((nCRC << 1) ^ 0x1021U)
                       : static_cast<quint16>(nCRC << 1);
    }
    return nCRC;
}

bool XLegacyEncoded::decodeBinHex(const QByteArray &baSource,
                                  QList<ITEM> *pItems)
{
    if (!pItems || (baSource.size() < 48) ||
        !baSource.startsWith("(This file must be converted with BinHex"))
        return false;

    const qint32 nLineEnd = baSource.indexOf('\n');
    const qint32 nCR = baSource.indexOf('\r');
    qint32 nHeaderEnd = nLineEnd;
    if ((nCR >= 0) && ((nHeaderEnd < 0) || (nCR < nHeaderEnd))) nHeaderEnd = nCR;
    if (nHeaderEnd < 0) return false;

    qint32 nColon = nHeaderEnd;
    while ((nColon < baSource.size()) &&
           ((baSource.at(nColon) == '\r') || (baSource.at(nColon) == '\n') ||
            (baSource.at(nColon) == '\t') || (baSource.at(nColon) == ' ')))
        ++nColon;
    if ((nColon >= baSource.size()) || (baSource.at(nColon) != ':')) return false;

    static const QByteArray baAlphabet(
        "!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr");
    QByteArray baSixBit;
    baSixBit.reserve(baSource.size() - nColon);
    bool bSawEnd = false;
    for (qint32 i = nColon + 1; i < baSource.size(); ++i)
    {
        const char c = baSource.at(i);
        if (c == ':')
        {
            bSawEnd = true;
            break;
        }
        const qint32 nValue = baAlphabet.indexOf(c);
        if (nValue >= 0)
            baSixBit.append(static_cast<char>(nValue));
        else if ((c != '\r') && (c != '\n') && (c != '\t') && (c != ' '))
            return false;
    }
    const bool bTruncated = !bSawEnd;  // no terminating ':' — a truncated transfer
    if (baSixBit.size() < 4) return false;

    QByteArray baRLE;
    baRLE.reserve((baSixBit.size() * 3) / 4);
    quint32 nBits = 0;
    qint32 nBitCount = 0;
    for (char c : baSixBit)
    {
        nBits = (nBits << 6) | static_cast<quint8>(c);
        nBitCount += 6;
        if (nBitCount >= 8)
        {
            nBitCount -= 8;
            if (baRLE.size() >= MAX_DECODED_SIZE) return false;
            baRLE.append(static_cast<char>((nBits >> nBitCount) & 0xffU));
            nBits &= nBitCount ? ((1U << nBitCount) - 1U) : 0U;
        }
    }

    QByteArray baDecoded;
    baDecoded.reserve(baRLE.size());
    qint32 nPosition = 0;
    qint32 nPrevious = -1;
    while (nPosition < baRLE.size())
    {
        const quint8 nByte = static_cast<quint8>(baRLE.at(nPosition++));
        if (nByte != 0x90U)
        {
            if (!checkedAppend(&baDecoded, static_cast<char>(nByte), 1,
                               MAX_DECODED_SIZE))
                return false;
            nPrevious = nByte;
            continue;
        }
        // A dangling 0x90 marker at the very end of a truncated stream has no
        // count byte; the reference drops it. A complete stream keeps failing.
        if (nPosition >= baRLE.size()) {
            if (bTruncated) break;
            return false;
        }
        const quint8 nCount = static_cast<quint8>(baRLE.at(nPosition++));
        if (nCount == 0)
        {
            if (!checkedAppend(&baDecoded, static_cast<char>(0x90), 1,
                               MAX_DECODED_SIZE))
                return false;
            nPrevious = 0x90;
        }
        else
        {
            if ((nCount == 1) || (nPrevious < 0) ||
                !checkedAppend(&baDecoded, static_cast<char>(nPrevious),
                               nCount - 1, MAX_DECODED_SIZE))
                return false;
        }
    }

    if (baDecoded.size() < 23) return false;
    const uchar *pData = reinterpret_cast<const uchar *>(baDecoded.constData());
    const quint8 nNameLength = pData[0];
    if ((nNameLength < 1) || (nNameLength > 63) ||
        (baDecoded.size() < 22 + nNameLength))
        return false;
    const qint64 nHeaderSize = 20 + nNameLength;
    const quint16 nHeaderCRC = qFromBigEndian<quint16>(pData + nHeaderSize);
    if (crc16CCITT(baDecoded.constData(), nHeaderSize) != nHeaderCRC)
        return false;

    const quint32 nDataSize = qFromBigEndian<quint32>(pData + 12 + nNameLength);
    const quint32 nResourceSize = qFromBigEndian<quint32>(pData + 16 + nNameLength);
    const quint64 nPayloadOffset = 22U + nNameLength;
    const quint64 nRequired = nPayloadOffset + nDataSize + 2U + nResourceSize + 2U;
    // The declared fork sizes are always bounded; the on-disk size only has to
    // match for a complete, undamaged stream.
    if ((static_cast<quint64>(nDataSize) + nResourceSize) > static_cast<quint64>(MAX_DECODED_SIZE))
        return false;
    const bool bComplete = (!bTruncated) && (nRequired == static_cast<quint64>(baDecoded.size()));

    const QString sName = safeBaseName(QString::fromLatin1(
        reinterpret_cast<const char *>(pData + 1), nNameLength));

    if (bComplete) {
        qint64 nOffset = nPayloadOffset;
        const quint16 nDataCRC = qFromBigEndian<quint16>(pData + nOffset + nDataSize);
        if (crc16CCITT(baDecoded.constData() + nOffset, nDataSize) != nDataCRC) {
            // A complete stream whose data fork fails its CRC is damaged;
            // fall through to the tolerant, extraction-gated path below.
        } else {
            ITEM dataItem;
            dataItem.sName = sName;
            dataItem.baData = baDecoded.mid(nOffset, nDataSize);
            dataItem.sMethod = QStringLiteral("BinHex 4.0 data fork");
            pItems->append(dataItem);

            nOffset += static_cast<qint64>(nDataSize) + 2;
            const quint16 nResourceCRC = qFromBigEndian<quint16>(pData + nOffset + nResourceSize);
            if (crc16CCITT(baDecoded.constData() + nOffset, nResourceSize) != nResourceCRC) return false;
            if (nResourceSize > 0) {
                ITEM resourceItem;
                resourceItem.sName = safeBaseName(sName + QStringLiteral(".rsrc"));
                resourceItem.baData = baDecoded.mid(nOffset, nResourceSize);
                resourceItem.sMethod = QStringLiteral("BinHex 4.0 resource fork");
                pItems->append(resourceItem);
            }
            return !pItems->isEmpty();
        }
    }

    // Tolerant path: the header CRC verified (this really is BinHex), but the
    // stream is truncated or the data fork is damaged. List one data-fork
    // member carrying the recovered prefix; mark it unverified so extraction
    // is refused by default (unpackCurrent honors UNPACK_PROP_CHECKCRC16). The
    // resource fork is never emitted on this path.
    const qint64 nAvailable = qMin<qint64>(static_cast<qint64>(nDataSize), qMax<qint64>(0, static_cast<qint64>(baDecoded.size()) - static_cast<qint64>(nPayloadOffset)));
    if (nAvailable <= 0) return false;
    ITEM dataItem;
    dataItem.sName = sName;
    dataItem.baData = baDecoded.mid(static_cast<qint32>(nPayloadOffset), static_cast<qint32>(nAvailable));
    dataItem.sMethod = bTruncated ? QStringLiteral("BinHex 4.0 data fork (truncated)") : QStringLiteral("BinHex 4.0 data fork (damaged)");
    dataItem.bVerified = false;
    dataItem.nDeclaredSize = nDataSize;
    pItems->append(dataItem);
    return !pItems->isEmpty();
}

bool XLegacyEncoded::decodeBtoa(const QByteArray &baSource,
                                const QString &sName, QList<ITEM> *pItems)
{
    if (!pItems || !baSource.startsWith("xbtoa Begin")) return false;
    qint32 nPayload = baSource.indexOf('\n');
    if (nPayload < 0) return false;
    ++nPayload;
    const qint32 nFooter = baSource.indexOf("xbtoa End N ", nPayload);
    if (nFooter < 0) return false;

    qint32 nFooterEnd = baSource.indexOf('\n', nFooter);
    if (nFooterEnd < 0) nFooterEnd = baSource.size();
    QString sFooter = QString::fromLatin1(
        baSource.mid(nFooter, nFooterEnd - nFooter)).trimmed();
    static const QRegularExpression reFooter(QStringLiteral(
        "^xbtoa End N\\s+([0-9]+)\\s+([0-9A-Fa-f]+)\\s+E\\s+([0-9A-Fa-f]+)\\s+S\\s+([0-9A-Fa-f]+)\\s+R\\s+([0-9A-Fa-f]+)$"));
    const QRegularExpressionMatch match = reFooter.match(sFooter);
    if (!match.hasMatch()) return false;

    bool bOkDecimal = false;
    bool bOkHex = false;
    bool bOkEor = false;
    bool bOkSum = false;
    bool bOkRot = false;
    const quint64 nDeclaredSize = match.captured(1).toULongLong(&bOkDecimal, 10);
    const quint64 nHexSize = match.captured(2).toULongLong(&bOkHex, 16);
    const quint32 nExpectedEor = match.captured(3).toUInt(&bOkEor, 16);
    const quint32 nExpectedSum = match.captured(4).toUInt(&bOkSum, 16);
    const quint32 nExpectedRot = match.captured(5).toUInt(&bOkRot, 16);
    if (!bOkDecimal || !bOkHex || !bOkEor || !bOkSum || !bOkRot ||
        (nDeclaredSize != nHexSize) || (nDeclaredSize > MAX_DECODED_SIZE))
        return false;

    QByteArray baDecoded;
    baDecoded.reserve(static_cast<int>(nDeclaredSize + 3));
    quint32 nWord = 0;
    qint32 nDigits = 0;
    for (qint32 i = nPayload; i < nFooter; ++i)
    {
        const quint8 c = static_cast<quint8>(baSource.at(i));
        if ((c == '\r') || (c == '\n') || (c == '\t') || (c == ' '))
            continue;
        if (c == 'z')
        {
            if ((nDigits != 0) ||
                !checkedAppend(&baDecoded, 0, 4, MAX_DECODED_SIZE + 3))
                return false;
            continue;
        }
        if ((c < '!') || (c > 'u')) return false;
        nWord = nWord * 85U + (c - '!');
        if (++nDigits == 5)
        {
            if ((baDecoded.size() > MAX_DECODED_SIZE - 1) ||
                !checkedAppend(&baDecoded, static_cast<char>(nWord >> 24), 1,
                               MAX_DECODED_SIZE + 3) ||
                !checkedAppend(&baDecoded, static_cast<char>(nWord >> 16), 1,
                               MAX_DECODED_SIZE + 3) ||
                !checkedAppend(&baDecoded, static_cast<char>(nWord >> 8), 1,
                               MAX_DECODED_SIZE + 3) ||
                !checkedAppend(&baDecoded, static_cast<char>(nWord), 1,
                               MAX_DECODED_SIZE + 3))
                return false;
            nWord = 0;
            nDigits = 0;
        }
    }
    if ((nDigits != 0) || (nDeclaredSize > static_cast<quint64>(baDecoded.size())) ||
        (baDecoded.size() - static_cast<qint64>(nDeclaredSize) > 3))
        return false;
    for (qint64 i = static_cast<qint64>(nDeclaredSize); i < baDecoded.size(); ++i)
        if (baDecoded.at(i) != 0) return false;

    quint32 nEor = 0;
    quint32 nSum = 0;
    quint32 nRot = 0;
    for (char value : baDecoded)
    {
        const quint8 c = static_cast<quint8>(value);
        nEor ^= c;
        nSum += c;
        nSum += 1;
        nRot = (nRot << 1) | (nRot >> 31);
        nRot += c;
    }
    if ((nEor != nExpectedEor) || (nSum != nExpectedSum) ||
        (nRot != nExpectedRot))
        return false;

    ITEM item;
    item.sName = safeBaseName(sName);
    item.baData = baDecoded.left(static_cast<int>(nDeclaredSize));
    item.sMethod = QStringLiteral("xbtoa Base85");
    pItems->append(item);
    return true;
}

bool XLegacyEncoded::decode(const QByteArray &baSource, FT fileTypeHint,
                            const QString &sName, FT *pFileType,
                            QList<ITEM> *pItems)
{
    if (!pFileType || !pItems) return false;
    pItems->clear();
    *pFileType = FT_UNKNOWN;
    if ((fileTypeHint == FT_UNKNOWN) || (fileTypeHint == FT_BINHEX))
    {
        if (decodeBinHex(baSource, pItems))
        {
            *pFileType = FT_BINHEX;
            return true;
        }
        pItems->clear();
    }
    if ((fileTypeHint == FT_UNKNOWN) || (fileTypeHint == FT_BTOA))
    {
        if (decodeBtoa(baSource, sName, pItems))
        {
            *pFileType = FT_BTOA;
            return true;
        }
        pItems->clear();
    }
    return false;
}

bool XLegacyEncoded::readSource(QByteArray *pData, PDSTRUCT *pPdStruct)
{
    if (!pData || !isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<XLegacyEncoded> guardedThis(this);
    const qint64 nSize = getSize();
    if (!guardedThis || (nSize <= 0) || (nSize > MAX_ENCODED_SIZE) ||
        (nSize > (std::numeric_limits<int>::max)()))
        return false;
    *pData = read_array_process(0, nSize, pPdStruct);
    return guardedThis && (pData->size() == nSize) &&
           isPdStructNotCanceled(pPdStruct);
}

bool XLegacyEncoded::isValid(QIODevice *pDevice, FT fileTypeHint,
                             PDSTRUCT *pPdStruct)
{
    XLegacyEncoded archive(pDevice, fileTypeHint);
    return archive.isValid(pPdStruct);
}

bool XLegacyEncoded::isValid(PDSTRUCT *pPdStruct)
{
    QByteArray baSource;
    FT fileType = FT_UNKNOWN;
    QList<ITEM> listItems;
    return readSource(&baSource, pPdStruct) &&
           decode(baSource, m_fileTypeHint,
                  XBinary::getDeviceFileBaseName(getDevice()), &fileType,
                  &listItems) &&
           isPdStructNotCanceled(pPdStruct);
}

XBinary::FT XLegacyEncoded::detectFileType(QIODevice *pDevice,
                                           PDSTRUCT *pPdStruct)
{
    XLegacyEncoded archive(pDevice);
    QByteArray baSource;
    FT fileType = FT_UNKNOWN;
    QList<ITEM> listItems;
    if (!archive.readSource(&baSource, pPdStruct) ||
        !decode(baSource, FT_UNKNOWN, XBinary::getDeviceFileBaseName(pDevice),
                &fileType, &listItems))
        return FT_UNKNOWN;
    return fileType;
}

XBinary::FT XLegacyEncoded::getFileType()
{
    return (m_fileTypeHint != FT_UNKNOWN)
               ? m_fileTypeHint
               : detectFileType(getDevice(), nullptr);
}

XBinary::MODE XLegacyEncoded::getMode() { return MODE_DATA; }
qint32 XLegacyEncoded::getType() { return TYPE_ARCHIVE; }
XBinary::ENDIAN XLegacyEncoded::getEndian() { return ENDIAN_BIG; }

QString XLegacyEncoded::getFileFormatExt()
{
    return getFileType() == FT_BINHEX ? QStringLiteral("hqx")
                                      : QStringLiteral("btoa");
}

QString XLegacyEncoded::getFileFormatExtsString()
{
    return getFileType() == FT_BINHEX
               ? QStringLiteral("BinHex 4.0 (*.hqx)")
               : QStringLiteral("xbtoa/Base85 (*.btoa)");
}

QString XLegacyEncoded::getMIMEString() { return QStringLiteral("text/plain"); }
qint64 XLegacyEncoded::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return isValid(pPdStruct) ? getSize() : 0;
}
XBinary::OSNAME XLegacyEncoded::getOsName()
{
    return getFileType() == FT_BINHEX ? OSNAME_MACOS : OSNAME_MULTIPLATFORM;
}
QString XLegacyEncoded::getVersion() { return QString(); }

QList<QString> XLegacyEncoded::getSearchSignatures()
{
    return getFileType() == FT_BINHEX
               ? QList<QString>{QStringLiteral("'(This file must be converted with BinHex'")}
               : QList<QString>{QStringLiteral("'xbtoa Begin'")};
}

XBinary *XLegacyEncoded::createInstance(QIODevice *pDevice, bool bIsImage,
                                        XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XLegacyEncoded(pDevice, m_fileTypeHint);
}

QMap<XBinary::UNPACK_PROP, QVariant>
XLegacyEncoded::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XLegacyEncoded::initUnpack(
    UNPACK_STATE *pState,
    const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    QPointer<XLegacyEncoded> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState))
        return false;

    UNPACK_CONTEXT *pOldContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !bindUnpackSource(pState, pPdStruct)) return false;

    QByteArray baSource;
    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext || !readSource(&baSource, pPdStruct) || !guardedThis ||
        !decode(baSource, m_fileTypeHint,
                XBinary::getDeviceFileBaseName(getDevice()),
                &pContext->fileType, &pContext->listItems) ||
        pContext->listItems.isEmpty())
    {
        delete pContext;
        if (guardedThis) releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listItems.size();
    pState->nCurrentOffset = 0;
    pState->nTotalSize = baSource.size();
    pState->mapUnpackProperties = mapProperties;
    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct))
    {
        if (!guardedThis) return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XLegacyEncoded::infoCurrent(
    UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XLegacyEncoded> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return ARCHIVERECORD();

    const UNPACK_CONTEXT *pContext =
        static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    if (pContext->listItems.size() != pState->nNumberOfRecords)
        return ARCHIVERECORD();
    const ITEM &item = pContext->listItems.at(pState->nCurrentIndex);
    ARCHIVERECORD result = {};
    result.nStreamOffset = 0;
    result.nStreamSize = pState->nTotalSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, item.sName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pState->nTotalSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, item.nDeclaredSize ? item.nDeclaredSize : static_cast<qint64>(item.baData.size()));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, item.sMethod);
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex))
        return ARCHIVERECORD();
    return result;
}

bool XLegacyEncoded::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                                   PDSTRUCT *pPdStruct)
{
    QPointer<XLegacyEncoded> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext ||
        !pDevice || !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) ||
        devicesAlias(getDevice(), pDevice))
        return false;

    QPointer<QIODevice> guardedOutput(pDevice);
    const UNPACK_CONTEXT *pContext =
        static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    if (pContext->listItems.size() != pState->nNumberOfRecords)
        return false;
    const ITEM &item = pContext->listItems.at(pState->nCurrentIndex);
    // A CRC-unverified member (truncated/damaged fork) is refused unless the
    // caller explicitly disabled CRC-16 enforcement.
    if (!item.bVerified) {
        const bool bCrcEnforced =
            pState->mapUnpackProperties.value(UNPACK_PROP_CHECKCRC16, pState->mapUnpackProperties.value(UNPACK_PROP_CHECKCRC, true)).toBool();
        if (bCrcEnforced) {
            setPdStructErrorString(pPdStruct, tr("BinHex member failed CRC verification (truncated or damaged)"));
            return false;
        }
    }
    const qint64 nSize = item.baData.size();
    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nSize))
        return false;

    if (pState->spOutputBudget)
    {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex,
                                                item.sName) &&
            pState->spOutputBudget->isEnforcing())
            return false;
        if (!pState->spOutputBudget->debit(nSize) &&
            pState->spOutputBudget->isEnforcing())
            return false;
    }

    std::unique_ptr<QIODevice> pStage(createFileBuffer(nSize, pPdStruct));
    if (!pStage || !guardedThis || !guardedOutput ||
        ((nSize > 0) && (pStage->write(item.baData) != nSize)) ||
        !pStage->seek(0) || !isUnpackSourceCurrent(pState, pPdStruct))
        return false;
    const bool bResult = publishUnpackOutput(pStage.get(), guardedOutput.data(),
                                             pState, pPdStruct);
    if (bResult && guardedThis) pState->nCurrentOffset = nSize;
    return bResult && guardedThis;
}

bool XLegacyEncoded::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XLegacyEncoded> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    ++pState->nCurrentIndex;
    pState->nCurrentOffset = (pState->nCurrentIndex == pState->nNumberOfRecords)
                                 ? pState->nTotalSize
                                 : 0;
    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool XLegacyEncoded::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState))
        return false;
    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}

QList<XBinary::FPART_PROP> XLegacyEncoded::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE,
            FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD,
            FPART_PROP_REPORTEDMETHOD};
}
