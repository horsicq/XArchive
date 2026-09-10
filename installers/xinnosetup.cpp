/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xinnosetup.h"

#include "xpe.h"
#include "xbzip2decoder.h"
#include "xdeflatedecoder.h"
#include "xstoredecoder.h"
#include "xlzmadecoder.h"

#include <QBuffer>
#include <QtEndian>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTemporaryFile>
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
#include <QTextCodec>
#else
#include <QStringConverter>
#endif

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

#include <limits>
#include <new>
#include <algorithm>

#include "xarchive.h"

XBinary::XCONVERT _TABLE_XINNOSETUP_STRUCTID[] = {
    {XInnoSetup::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XInnoSetup::STRUCTID_HEADER, "HEADER", QString("Header")},
};

// rDlPtS magic bytes for InnoSetup offset table (version 5.1.5+)
static const quint8 g_aRDlPtSMagic[] = {0x72, 0x44, 0x6C, 0x50, 0x74, 0x53, 0xCD, 0xE6, 0xD7, 0x7B, 0x0B, 0x2A};
static const quint8 g_aRDlPtS02Magic[] = {0x72, 0x44, 0x6C, 0x50, 0x74, 0x53, 0x30, 0x32, 0x87, 0x65, 0x56, 0x78};
static const quint8 g_aRDlPtS04Magic[] = {0x72, 0x44, 0x6C, 0x50, 0x74, 0x53, 0x30, 0x34, 0x87, 0x65, 0x56, 0x78};
static const quint8 g_aRDlPtS05Magic[] = {0x72, 0x44, 0x6C, 0x50, 0x74, 0x53, 0x30, 0x35, 0x87, 0x65, 0x56, 0x78};
static const quint8 g_aRDlPtS06Magic[] = {0x72, 0x44, 0x6C, 0x50, 0x74, 0x53, 0x30, 0x36, 0x87, 0x65, 0x56, 0x78};
static const quint8 g_aRDlPtS07Magic[] = {0x72, 0x44, 0x6C, 0x50, 0x74, 0x53, 0x30, 0x37, 0x87, 0x65, 0x56, 0x78};
static const quint8 g_aRDlPtSVxMagic[] = {0x72, 0x44, 0x6C, 0x50, 0x74, 0x53, 0x56, 0x78, 0x87, 0x65, 0x56, 0x78};
static const qint32 g_nRDlPtSMagicSize = 12;

namespace {
const quint16 INNO_LOADER_LEGACY_VX = 0x100;
const quint16 INNO_LOADER_LEGACY_EOF40 = 0x101;
const qint64 INNO_MAX_HEADER_BLOCK_SIZE = 64LL * 1024 * 1024;
const qint64 INNO_MAX_HEADER_STORED_SIZE = INNO_MAX_HEADER_BLOCK_SIZE + ((INNO_MAX_HEADER_BLOCK_SIZE + 4095) / 4096) * 4;
const quint32 INNO_MAX_HEADER_LZMA_DICTIONARY_SIZE = 64U * 1024 * 1024;
const qint32 INNO_MAX_KDF_ITERATIONS = 10000000;
const qint32 INNO_MAX_EXTERNAL_SLICES = 4096;
const qint32 INNO_MAX_PASSWORD_BYTES = 1024 * 1024;
const qint64 INNO_MAX_LEGACY_BLOCK_SIZE = 16LL * 1024 * 1024;
const qint32 INNO_MAX_LEGACY_RECORDS = 100000;
const qint64 INNO_MAX_FALLBACK_DATA_SIZE = 1024LL * 1024 * 1024;
const qint64 INNO_MAX_FALLBACK_TOTAL_OUTPUT = 1024LL * 1024 * 1024;

bool innoCalculateAdler32(const QByteArray &baData, quint32 *pnResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (pnResult) *pnResult = 0;
    if (!pnResult || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    static const quint32 nModAdler = 65521U;
    const uchar *pData = reinterpret_cast<const uchar *>(baData.constData());
    qint32 nRemaining = baData.size();
    quint32 a = 1;
    quint32 b = 0;

    // 5552 bytes is the largest safe deferred-modulo block for Adler-32.
    while (nRemaining > 0) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint32 nBlockSize = qMin(nRemaining, 5552);
        nRemaining -= nBlockSize;
        for (qint32 i = 0; i < nBlockSize; i++) {
            a += *pData++;
            b += a;
        }
        a %= nModAdler;
        b %= nModAdler;
    }

    *pnResult = (b << 16) | a;
    return true;
}

void innoSecureClear(QByteArray *pData)
{
    if (!pData) return;
    volatile char *p = pData->isEmpty() ? nullptr : pData->data();
    for (qint32 i = 0; p && (i < pData->size()); i++) p[i] = 0;
    pData->clear();
}

quint32 innoRotateLeft32(quint32 nValue, quint32 nCount)
{
    return (nValue << nCount) | (nValue >> (32U - nCount));
}

void innoChaChaQuarterRound(quint32 &a, quint32 &b, quint32 &c, quint32 &d)
{
    a += b;
    d ^= a;
    d = innoRotateLeft32(d, 16);
    c += d;
    b ^= c;
    b = innoRotateLeft32(b, 12);
    a += b;
    d ^= a;
    d = innoRotateLeft32(d, 8);
    c += d;
    b ^= c;
    b = innoRotateLeft32(b, 7);
}

void innoChaChaRounds(quint32 *pState)
{
    for (qint32 i = 0; i < 10; i++) {
        innoChaChaQuarterRound(pState[0], pState[4], pState[8], pState[12]);
        innoChaChaQuarterRound(pState[1], pState[5], pState[9], pState[13]);
        innoChaChaQuarterRound(pState[2], pState[6], pState[10], pState[14]);
        innoChaChaQuarterRound(pState[3], pState[7], pState[11], pState[15]);
        innoChaChaQuarterRound(pState[0], pState[5], pState[10], pState[15]);
        innoChaChaQuarterRound(pState[1], pState[6], pState[11], pState[12]);
        innoChaChaQuarterRound(pState[2], pState[7], pState[8], pState[13]);
        innoChaChaQuarterRound(pState[3], pState[4], pState[9], pState[14]);
    }
}

QByteArray innoHmacSha256(const QByteArray &baKey, const QByteArray &baMessage)
{
    QByteArray baBlockKey = baKey;
    if (baBlockKey.size() > 64) baBlockKey = QCryptographicHash::hash(baBlockKey, QCryptographicHash::Sha256);
    if (baBlockKey.size() < 64) baBlockKey.append(QByteArray(64 - baBlockKey.size(), '\0'));

    QByteArray baInnerPad(64, char(0x36));
    QByteArray baOuterPad(64, char(0x5c));
    for (qint32 i = 0; i < 64; i++) {
        baInnerPad[i] = char((quint8)baInnerPad.at(i) ^ (quint8)baBlockKey.at(i));
        baOuterPad[i] = char((quint8)baOuterPad.at(i) ^ (quint8)baBlockKey.at(i));
    }

    QByteArray baInner = baInnerPad;
    baInner.append(baMessage);
    QByteArray baInnerHash = QCryptographicHash::hash(baInner, QCryptographicHash::Sha256);
    QByteArray baOuter = baOuterPad;
    baOuter.append(baInnerHash);
    QByteArray baResult = QCryptographicHash::hash(baOuter, QCryptographicHash::Sha256);

    innoSecureClear(&baBlockKey);
    innoSecureClear(&baInnerPad);
    innoSecureClear(&baOuterPad);
    innoSecureClear(&baInner);
    innoSecureClear(&baInnerHash);
    innoSecureClear(&baOuter);
    return baResult;
}

QByteArray innoCreateNonce(const QByteArray &baBaseNonce, quint64 nStartOffset, quint32 nFirstSlice)
{
    if (baBaseNonce.size() != 24) return QByteArray();
    QByteArray baNonce = baBaseNonce;
    uchar aStart[8];
    uchar aSlice[4];
    qToLittleEndian<quint64>(nStartOffset, aStart);
    qToLittleEndian<quint32>(nFirstSlice, aSlice);
    for (qint32 i = 0; i < 8; i++) baNonce[i] = char((quint8)baNonce.at(i) ^ aStart[i]);
    for (qint32 i = 0; i < 4; i++) baNonce[8 + i] = char((quint8)baNonce.at(8 + i) ^ aSlice[i]);
    return baNonce;
}

class InnoBoundedBuffer : public QBuffer {
public:
    explicit InnoBoundedBuffer(qint64 nLimit) : m_nLimit(nLimit)
    {
    }

protected:
    qint64 writeData(const char *pData, qint64 nSize) override
    {
        if ((nSize < 0) || (pos() < 0) || (pos() > m_nLimit) || (nSize > m_nLimit - pos())) return -1;
        return QBuffer::writeData(pData, nSize);
    }

private:
    qint64 m_nLimit;
};

struct InnoCRCProgressBridge {
    XBinary::PDSTRUCT *pOriginal;
    XBinary::PDSTRUCTLIFETIME originalLifetime;
};

void innoCRCProgressCallback(void *pUserData, XBinary::PDSTRUCT *pLocalProgress)
{
    InnoCRCProgressBridge *pBridge = static_cast<InnoCRCProgressBridge *>(pUserData);
    if (!pBridge || !pLocalProgress) return;

    if (!XBinary::isPdStructLifetimeAlive(pBridge->originalLifetime) || !XBinary::isPdStructNotCanceled(pBridge->pOriginal)) {
        XBinary::setPdStructStopped(pLocalProgress);
    }
}

class InnoOperationGuard {
public:
    explicit InnoOperationGuard(const QSharedPointer<XInnoSetup::UNPACK_LIFETIME_STATE> &pState) : m_pState(pState), m_bAcquired(false)
    {
        if (m_pState && m_pState->bOwnerAlive && !m_pState->bOperationInProgress) {
            m_pState->bOperationInProgress = true;
            m_bAcquired = true;
        }
    }
    ~InnoOperationGuard()
    {
        if (m_pState && m_bAcquired) m_pState->bOperationInProgress = false;
    }
    bool isAcquired() const
    {
        return m_bAcquired;
    }

private:
    QSharedPointer<XInnoSetup::UNPACK_LIFETIME_STATE> m_pState;
    bool m_bAcquired;
};

class InnoPublisher : public XArchive {
public:
    explicit InnoPublisher(QIODevice *pDevice) : XArchive(pDevice)
    {
    }
    using XArchive::publishUnpackOutput;
};

class InnoContextValidator {
public:
    InnoContextValidator(const QPointer<XInnoSetup> &pOwner, const QPointer<QIODevice> &pOutput,
                         const QSharedPointer<XInnoSetup::UNPACK_LIFETIME_STATE> &pLifetime, XInnoSetup::UNPACK_CONTEXT *pContext,
                         XBinary::UNPACK_STATE *pState)
        : m_pOwner(pOwner), m_pOutput(pOutput), m_pLifetime(pLifetime), m_pContext(pContext), m_pState(pState)
    {
    }

    bool isCurrent() const
    {
        return m_pOwner && m_pOutput && m_pLifetime && m_pLifetime->bOwnerAlive && m_pLifetime->setContexts.contains(m_pContext) &&
               (m_pContext->pOwnerState == m_pState);
    }

private:
    QPointer<XInnoSetup> m_pOwner;
    QPointer<QIODevice> m_pOutput;
    QSharedPointer<XInnoSetup::UNPACK_LIFETIME_STATE> m_pLifetime;
    XInnoSetup::UNPACK_CONTEXT *m_pContext;
    XBinary::UNPACK_STATE *m_pState;
};

bool innoIsOriginalProgressAlive(XBinary::PDSTRUCT *pPdStruct, const XBinary::PDSTRUCTLIFETIME &progressLifetime)
{
    return !pPdStruct || (XBinary::isPdStructLifetimeAlive(progressLifetime) && XBinary::isPdStructNotCanceled(pPdStruct));
}

bool innoVerifyCRC(const XBinary::ARCHIVERECORD &record, const XBinary::UNPACK_STATE &stageState, QTemporaryFile *pStage,
                   XBinary::PDSTRUCT *pPdStruct, const XBinary::PDSTRUCTLIFETIME &progressLifetime, XBinary::PDSTRUCT *pCRCProgress)
{
    if (!pStage || !pCRCProgress) return false;

    const XBinary::CRC_TYPE crcType =
        (XBinary::CRC_TYPE)record.mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();

    if (!XBinary::isUnpackCRCEnabled(stageState.mapUnpackProperties, crcType) || (crcType == XBinary::CRC_TYPE_UNKNOWN) ||
        !record.mapProperties.contains(XBinary::FPART_PROP_RESULTCRC)) {
        return true;
    }

    if (!innoIsOriginalProgressAlive(pPdStruct, progressLifetime)) return false;

    if (!pStage->seek(0)) {
        if (innoIsOriginalProgressAlive(pPdStruct, progressLifetime)) {
            XBinary::setPdStructErrorString(pPdStruct, XInnoSetup::tr("CRC check requires a readable output device"));
        }
        return false;
    }

    const bool bCRCOk = XBinary::checkCRC(pStage, crcType, record.mapProperties.value(XBinary::FPART_PROP_RESULTCRC), pCRCProgress);

    if (!innoIsOriginalProgressAlive(pPdStruct, progressLifetime)) return false;
    if (!bCRCOk) XBinary::setPdStructErrorString(pPdStruct, XInnoSetup::tr("Invalid CRC"));

    return bCRCOk;
}

bool innoVerifyChecksum(const XBinary::ARCHIVERECORD &record, QTemporaryFile *pStage, const InnoContextValidator &contextValidator,
                        XBinary::PDSTRUCT *pPdStruct)
{
    if (!pStage) return false;

    const bool bHasChecksum = record.mapProperties.contains(XBinary::FPART_PROP_CHECKSUM);
    const bool bHasType = record.mapProperties.contains(XBinary::FPART_PROP_CHECKSUMTYPE);
    if (!bHasChecksum && !bHasType) return true;
    if (!bHasChecksum || !bHasType || !pStage->seek(0)) return false;

    const QString sType = record.mapProperties.value(XBinary::FPART_PROP_CHECKSUMTYPE).toString();
    QCryptographicHash::Algorithm algorithm;
    if (sType == QLatin1String("MD5")) algorithm = QCryptographicHash::Md5;
    else if (sType == QLatin1String("SHA1")) algorithm = QCryptographicHash::Sha1;
    else if (sType == QLatin1String("SHA256")) algorithm = QCryptographicHash::Sha256;
    else return false;

    QCryptographicHash hash(algorithm);
    QByteArray baBuffer(65536, '\0');

    while (true) {
        if (!contextValidator.isCurrent() || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nRead = pStage->read(baBuffer.data(), baBuffer.size());
        if (nRead < 0) return false;
        if (nRead == 0) break;
        hash.addData(QByteArray::fromRawData(baBuffer.constData(), nRead));
    }

    const QByteArray baExpected = record.mapProperties.value(XBinary::FPART_PROP_CHECKSUM).toString().toLatin1().toLower();
    const bool bOk = (hash.result().toHex() == baExpected);
    if (!bOk) XBinary::setPdStructErrorString(pPdStruct, XInnoSetup::tr("Invalid checksum"));
    return bOk;
}

quint64 innoVersionValue(const XInnoSetup::INNO_VERSION &version)
{
    return (quint64(version.nMajor) << 48) | (quint64(version.nMinor) << 32) | (quint64(version.nPatch) << 16) | quint64(version.nRevision);
}

quint64 innoVersionValue(quint16 nMajor, quint16 nMinor, quint16 nPatch = 0, quint16 nRevision = 0)
{
    return (quint64(nMajor) << 48) | (quint64(nMinor) << 32) | (quint64(nPatch) << 16) | quint64(nRevision);
}

bool innoSkipBytes(const QByteArray &baData, qint32 nSize, qint32 *pnOffset)
{
    if (!pnOffset || (nSize < 0) || (*pnOffset < 0) || (*pnOffset > baData.size()) || (nSize > baData.size() - *pnOffset)) return false;
    *pnOffset += nSize;
    return true;
}

bool innoSkipSerializedString(const QByteArray &baData, qint32 *pnOffset)
{
    if (!pnOffset || (*pnOffset < 0) || (*pnOffset > baData.size() - 4)) return false;
    const quint32 nLength = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData() + *pnOffset));
    if ((nLength > 0x10000000U) || (nLength > quint32(baData.size() - *pnOffset - 4))) return false;
    *pnOffset += 4 + (qint32)nLength;
    return true;
}

bool innoSkipSerializedStrings(const QByteArray &baData, qint32 nCount, qint32 *pnOffset)
{
    if (nCount < 0) return false;
    for (qint32 i = 0; i < nCount; i++) {
        if (!innoSkipSerializedString(baData, pnOffset)) return false;
    }
    return true;
}

bool innoReadWideString(const QByteArray &baData, qint32 *pnOffset, QString *psValue)
{
    if (!pnOffset || (*pnOffset < 0) || (*pnOffset > baData.size() - 4)) return false;
    const quint32 nByteLength = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData() + *pnOffset));
    if (((nByteLength & 1U) != 0) || (nByteLength > 0x1000000U) || (nByteLength > quint32(baData.size() - *pnOffset - 4))) return false;

    const qint32 nStringOffset = *pnOffset + 4;
    const qint32 nCharacterCount = (qint32)(nByteLength / 2);
    if (psValue) {
        psValue->resize(nCharacterCount);
        const uchar *pStringData = reinterpret_cast<const uchar *>(baData.constData() + nStringOffset);
        for (qint32 i = 0; i < nCharacterCount; i++) {
            (*psValue)[i] = QChar(qFromLittleEndian<quint16>(pStringData + i * 2));
        }
    }
    *pnOffset = nStringOffset + (qint32)nByteLength;
    return true;
}

bool innoReadAnsiBytes(const QByteArray &baData, qint32 *pnOffset, QByteArray *pValue)
{
    if (!pnOffset || (*pnOffset < 0) || (*pnOffset > baData.size() - 4)) return false;
    const quint32 nByteLength = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData() + *pnOffset));
    if ((nByteLength > 0x1000000U) || (nByteLength > quint32(baData.size() - *pnOffset - 4))) return false;

    const qint32 nStringOffset = *pnOffset + 4;
    if (pValue) *pValue = baData.mid(nStringOffset, (qint32)nByteLength);
    *pnOffset = nStringOffset + (qint32)nByteLength;
    return true;
}

bool innoSkipUnicodeEntries(const QByteArray &baData, qint32 nCount, qint32 nWideStrings, qint32 nAnsiStrings, qint32 nFixedSize,
                            qint32 *pnOffset)
{
    if ((nCount < 0) || (nWideStrings < 0) || (nAnsiStrings < 0) || !pnOffset) return false;
    for (qint32 i = 0; i < nCount; i++) {
        for (qint32 j = 0; j < nWideStrings; j++) {
            if (!innoReadWideString(baData, pnOffset, nullptr)) return false;
        }
        for (qint32 j = 0; j < nAnsiStrings; j++) {
            if (!innoReadAnsiBytes(baData, pnOffset, nullptr)) return false;
        }
        if (!innoSkipBytes(baData, nFixedSize, pnOffset)) return false;
    }
    return true;
}

bool innoSkipAnsiEntries(const QByteArray &baData, qint32 nCount, qint32 nStringCount, qint32 nFixedSize, qint32 *pnOffset)
{
    if ((nCount < 0) || (nStringCount < 0) || !pnOffset) return false;
    for (qint32 i = 0; i < nCount; i++) {
        for (qint32 j = 0; j < nStringCount; j++) {
            if (!innoReadAnsiBytes(baData, pnOffset, nullptr)) return false;
        }
        if (!innoSkipBytes(baData, nFixedSize, pnOffset)) return false;
    }
    return true;
}

bool innoReadSizedAnsiRecord(const QByteArray &baData, qint32 nStringCount, qint32 nTailSize, qint32 *pnOffset, QList<QByteArray> *pStrings,
                             qint32 *pnTailOffset)
{
    if ((nStringCount < 0) || (nTailSize < 0) || !pnOffset || (*pnOffset < 0) || (*pnOffset > baData.size() - 4)) return false;
    const quint32 nRecordSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData() + *pnOffset));
    const quint32 nMinimumSize = quint32(nStringCount * 4 + nTailSize);
    if ((nRecordSize < nMinimumSize) || (nRecordSize > quint32(baData.size() - *pnOffset - 4))) return false;
    const qint32 nRecordEnd = *pnOffset + 4 + (qint32)nRecordSize;
    *pnOffset += 4;
    if (pStrings) {
        pStrings->clear();
        pStrings->reserve(nStringCount);
    }
    for (qint32 i = 0; i < nStringCount; i++) {
        QByteArray baValue;
        if (!innoReadAnsiBytes(baData, pnOffset, &baValue) || (*pnOffset > nRecordEnd)) return false;
        if (pStrings) pStrings->append(baValue);
    }
    if ((*pnOffset > nRecordEnd) || (nRecordEnd - *pnOffset != nTailSize)) return false;
    if (pnTailOffset) *pnTailOffset = *pnOffset;
    *pnOffset = nRecordEnd;
    return true;
}

void innoSelectLanguageCodePage(quint32 nLanguageId, quint32 nStoredCodePage, quint32 *pnCodePage)
{
    if (!pnCodePage) return;
    quint32 nCandidate = nStoredCodePage;
#ifdef Q_OS_WIN
    if ((nCandidate == 0) && (nLanguageId != 0)) {
        wchar_t aCodePage[16] = {};
        if (GetLocaleInfoW(MAKELCID((LANGID)nLanguageId, SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, aCodePage,
                           sizeof(aCodePage) / sizeof(aCodePage[0])) > 1) {
            bool bOk = false;
            const uint nValue = QString::fromWCharArray(aCodePage).toUInt(&bOk);
            if (bOk) nCandidate = nValue;
        }
    }
#else
    Q_UNUSED(nLanguageId)
#endif
    if (nCandidate != 0) *pnCodePage = nCandidate;
}

bool innoSkipAnsiLanguageEntries(const QByteArray &baData, qint32 nCount, qint32 nStringCount, qint32 nFixedSize, bool bHasStoredCodePage,
                                 qint32 *pnOffset, quint32 *pnCodePage)
{
    if ((nCount < 0) || (nStringCount < 0) || (nFixedSize < 0) || !pnOffset || !pnCodePage) return false;
    for (qint32 i = 0; i < nCount; i++) {
        for (qint32 j = 0; j < nStringCount; j++) {
            if (!innoReadAnsiBytes(baData, pnOffset, nullptr)) return false;
        }
        if ((*pnOffset < 0) || (*pnOffset > baData.size() - nFixedSize)) return false;
        const quint32 nLanguageId = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData() + *pnOffset));
        const quint32 nStoredCodePage =
            bHasStoredCodePage ? qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData() + *pnOffset + 4)) : 0;
        if ((i == 0) || (nStoredCodePage == 1252)) innoSelectLanguageCodePage(nLanguageId, nStoredCodePage, pnCodePage);
        if (!innoSkipBytes(baData, nFixedSize, pnOffset)) return false;
    }
    return true;
}

void innoAddHeaderFlag(bool bPresent, qint32 *pnFlagCount)
{
    if (bPresent && pnFlagCount) (*pnFlagCount)++;
}

class InnoByteSkipper {
public:
    explicit InnoByteSkipper(const QByteArray &baData) : m_baData(baData)
    {
    }
    bool operator()(qint32 nSize, qint32 *pnOffset) const
    {
        return innoSkipBytes(m_baData, nSize, pnOffset);
    }

private:
    const QByteArray &m_baData;
};

class InnoSerializedStringSkipper {
public:
    explicit InnoSerializedStringSkipper(const QByteArray &baData) : m_baData(baData)
    {
    }
    bool operator()(qint32 *pnOffset) const
    {
        return innoSkipSerializedString(m_baData, pnOffset);
    }

private:
    const QByteArray &m_baData;
};

class InnoSerializedStringsSkipper {
public:
    explicit InnoSerializedStringsSkipper(const QByteArray &baData) : m_baData(baData)
    {
    }
    bool operator()(qint32 nCount, qint32 *pnOffset) const
    {
        return innoSkipSerializedStrings(m_baData, nCount, pnOffset);
    }

private:
    const QByteArray &m_baData;
};

class InnoWideStringReader {
public:
    explicit InnoWideStringReader(const QByteArray &baData) : m_baData(baData)
    {
    }
    bool operator()(qint32 *pnOffset, QString *psValue) const
    {
        return innoReadWideString(m_baData, pnOffset, psValue);
    }

private:
    const QByteArray &m_baData;
};

class InnoAnsiStringReader {
public:
    explicit InnoAnsiStringReader(const QByteArray &baData) : m_baData(baData)
    {
    }
    bool operator()(qint32 *pnOffset, QByteArray *pValue) const
    {
        return innoReadAnsiBytes(m_baData, pnOffset, pValue);
    }

private:
    const QByteArray &m_baData;
};

class InnoAnsiStringSkipper {
public:
    explicit InnoAnsiStringSkipper(const QByteArray &baData) : m_baData(baData)
    {
    }
    bool operator()(qint32 *pnOffset) const
    {
        return innoReadAnsiBytes(m_baData, pnOffset, nullptr);
    }

private:
    const QByteArray &m_baData;
};

class InnoUnicodeEntriesSkipper {
public:
    explicit InnoUnicodeEntriesSkipper(const QByteArray &baData) : m_baData(baData)
    {
    }
    bool operator()(qint32 nCount, qint32 nWideStrings, qint32 nAnsiStrings, qint32 nFixedSize, qint32 *pnOffset) const
    {
        return innoSkipUnicodeEntries(m_baData, nCount, nWideStrings, nAnsiStrings, nFixedSize, pnOffset);
    }

private:
    const QByteArray &m_baData;
};

class InnoAnsiEntriesSkipper {
public:
    explicit InnoAnsiEntriesSkipper(const QByteArray &baData) : m_baData(baData)
    {
    }
    bool operator()(qint32 nCount, qint32 nStringCount, qint32 nFixedSize, qint32 *pnOffset) const
    {
        return innoSkipAnsiEntries(m_baData, nCount, nStringCount, nFixedSize, pnOffset);
    }

private:
    const QByteArray &m_baData;
};

class InnoSizedAnsiRecordReader {
public:
    InnoSizedAnsiRecordReader(const QByteArray &baData, qint32 *pnOffset) : m_baData(baData), m_pnOffset(pnOffset)
    {
    }
    bool operator()(qint32 nStringCount, qint32 nTailSize, QList<QByteArray> *pStrings, qint32 *pnTailOffset) const
    {
        return innoReadSizedAnsiRecord(m_baData, nStringCount, nTailSize, m_pnOffset, pStrings, pnTailOffset);
    }

private:
    const QByteArray &m_baData;
    qint32 *m_pnOffset;
};

class InnoAnsiBlobSkipper {
public:
    InnoAnsiBlobSkipper(const QByteArray &baData, qint32 *pnOffset) : m_baData(baData), m_pnOffset(pnOffset)
    {
    }
    bool operator()() const
    {
        return innoReadAnsiBytes(m_baData, m_pnOffset, nullptr);
    }

private:
    const QByteArray &m_baData;
    qint32 *m_pnOffset;
};

class InnoAnsiLanguageEntriesSkipper {
public:
    InnoAnsiLanguageEntriesSkipper(const QByteArray &baData, qint32 *pnOffset, quint32 *pnCodePage)
        : m_baData(baData), m_pnOffset(pnOffset), m_pnCodePage(pnCodePage)
    {
    }
    bool operator()(qint32 nCount, qint32 nStringCount, qint32 nFixedSize, bool bHasStoredCodePage) const
    {
        return innoSkipAnsiLanguageEntries(m_baData, nCount, nStringCount, nFixedSize, bHasStoredCodePage, m_pnOffset, m_pnCodePage);
    }

private:
    const QByteArray &m_baData;
    qint32 *m_pnOffset;
    quint32 *m_pnCodePage;
};

class InnoHeaderFlagCounter {
public:
    explicit InnoHeaderFlagCounter(qint32 *pnFlagCount) : m_pnFlagCount(pnFlagCount)
    {
    }
    void operator()(bool bPresent) const
    {
        innoAddHeaderFlag(bPresent, m_pnFlagCount);
    }

private:
    qint32 *m_pnFlagCount;
};

XBinary::HANDLE_METHOD innoCompressionToHandleMethod(XInnoSetup::INNO_COMPRESSION compression)
{
    switch (compression) {
        case XInnoSetup::INNO_COMPRESSION_STORE: return XBinary::HANDLE_METHOD_STORE;
        case XInnoSetup::INNO_COMPRESSION_ZLIB: return XBinary::HANDLE_METHOD_ZLIB;
        case XInnoSetup::INNO_COMPRESSION_BZIP2: return XBinary::HANDLE_METHOD_BZIP2;
        case XInnoSetup::INNO_COMPRESSION_LZMA1: return XBinary::HANDLE_METHOD_LZMA;
        case XInnoSetup::INNO_COMPRESSION_LZMA2: return XBinary::HANDLE_METHOD_LZMA2;
        default: return XBinary::HANDLE_METHOD_UNKNOWN;
    }
}

XInnoSetup::INNO_COMPRESSION innoHandleMethodToCompression(XBinary::HANDLE_METHOD handleMethod)
{
    switch (handleMethod) {
        case XBinary::HANDLE_METHOD_STORE: return XInnoSetup::INNO_COMPRESSION_STORE;
        case XBinary::HANDLE_METHOD_ZLIB: return XInnoSetup::INNO_COMPRESSION_ZLIB;
        case XBinary::HANDLE_METHOD_BZIP2: return XInnoSetup::INNO_COMPRESSION_BZIP2;
        case XBinary::HANDLE_METHOD_LZMA: return XInnoSetup::INNO_COMPRESSION_LZMA1;
        case XBinary::HANDLE_METHOD_LZMA2: return XInnoSetup::INNO_COMPRESSION_LZMA2;
        default: return XInnoSetup::INNO_COMPRESSION_UNKNOWN;
    }
}

void innoDecodeExeFilter4108(QByteArray *pData)
{
    if (!pData) return;

    quint32 nAddress = 0;
    qint32 nAddressBytesLeft = 0;

    for (qint32 i = 0; i < pData->size(); i++) {
        quint8 nByte = (quint8)pData->at(i);

        if (nAddressBytesLeft == 0) {
            if ((nByte == 0xe8) || (nByte == 0xe9)) {
                // Inno's pre-5.2 filter makes CALL/JMP operands relative to
                // the address immediately after the five-byte instruction.
                nAddress = ~quint32(i + 5) + 1;
                nAddressBytesLeft = 4;
            }
        } else {
            nAddress += nByte;
            (*pData)[i] = char(quint8(nAddress));
            nAddress >>= 8;
            nAddressBytesLeft--;
        }
    }
}

void innoDecodeExeFilter5200(QByteArray *pData, bool bFlipHighByte)
{
    if (!pData) return;

    qint32 nPos = 0;

    while (nPos < pData->size()) {
        const quint8 nOpcode = (quint8)pData->at(nPos);

        if (((nOpcode != 0xe8) && (nOpcode != 0xe9)) || ((0x10000 - (nPos % 0x10000)) < 5) || (nPos + 5 > pData->size())) {
            nPos++;
            continue;
        }

        quint8 nHigh = (quint8)pData->at(nPos + 4);

        if ((nHigh == 0x00) || (nHigh == 0xff)) {
            quint32 nRelative = (quint8)pData->at(nPos + 1) | (quint32((quint8)pData->at(nPos + 2)) << 8) | (quint32((quint8)pData->at(nPos + 3)) << 16);
            nRelative -= quint32(nPos + 5) & 0x00ffffff;
            (*pData)[nPos + 1] = char(quint8(nRelative));
            (*pData)[nPos + 2] = char(quint8(nRelative >> 8));
            (*pData)[nPos + 3] = char(quint8(nRelative >> 16));

            if (bFlipHighByte && (nRelative & 0x00800000)) {
                (*pData)[nPos + 4] = char(quint8(~nHigh));
            }
        }

        nPos += 5;
    }
}

void innoDecodeExeFilter(QByteArray *pData, const XInnoSetup::INNO_VERSION &version)
{
    const quint64 nVersion = innoVersionValue(version);

    if (nVersion < innoVersionValue(5, 2, 0)) {
        innoDecodeExeFilter4108(pData);
    } else {
        innoDecodeExeFilter5200(pData, nVersion >= innoVersionValue(5, 3, 9));
    }
}
}  // namespace

QByteArray XInnoSetup::_encodeLegacyPassword(const QString &sPassword, bool bUnicode, quint32 nAnsiCodePage, bool *pbOk)
{
    if (pbOk) *pbOk = false;
    if (bUnicode) {
        if (sPassword.size() > (std::numeric_limits<qint32>::max)() / 2) return QByteArray();
        QByteArray baResult(sPassword.size() * 2, '\0');
        for (qint32 i = 0; i < sPassword.size(); i++) {
            qToLittleEndian<quint16>(sPassword.at(i).unicode(), reinterpret_cast<uchar *>(baResult.data() + i * 2));
        }
        if (pbOk) *pbOk = true;
        return baResult;
    }

    if (nAnsiCodePage == 0) return QByteArray();

#ifdef Q_OS_WIN
    if (!IsValidCodePage((UINT)nAnsiCodePage)) return QByteArray();
    if (sPassword.isEmpty()) {
        if (pbOk) *pbOk = true;
        return QByteArray("");
    }

    DWORD nFlags = 0;
    BOOL bUsedDefault = FALSE;
    LPBOOL pUsedDefault = &bUsedDefault;
    if ((nAnsiCodePage == CP_UTF8) || (nAnsiCodePage == 54936U)) {
        nFlags = WC_ERR_INVALID_CHARS;
        pUsedDefault = nullptr;
    } else {
        nFlags = WC_NO_BEST_FIT_CHARS;
    }

    int nBytes = WideCharToMultiByte((UINT)nAnsiCodePage, nFlags, reinterpret_cast<LPCWCH>(sPassword.constData()), sPassword.size(), nullptr, 0, nullptr, pUsedDefault);
    if ((nBytes <= 0) && (GetLastError() == ERROR_INVALID_FLAGS)) {
        nFlags = 0;
        bUsedDefault = FALSE;
        pUsedDefault = ((nAnsiCodePage == CP_UTF8) || (nAnsiCodePage == 54936U)) ? nullptr : &bUsedDefault;
        nBytes = WideCharToMultiByte((UINT)nAnsiCodePage, nFlags, reinterpret_cast<LPCWCH>(sPassword.constData()), sPassword.size(), nullptr, 0, nullptr, pUsedDefault);
    }
    if ((nBytes <= 0) || bUsedDefault) return QByteArray();

    QByteArray baResult(nBytes, '\0');
    if ((WideCharToMultiByte((UINT)nAnsiCodePage, nFlags, reinterpret_cast<LPCWCH>(sPassword.constData()), sPassword.size(), baResult.data(), baResult.size(), nullptr,
                             pUsedDefault) != nBytes) ||
        bUsedDefault) {
        innoSecureClear(&baResult);
        return QByteArray();
    }
#elif (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
    QByteArray baCodePageName = QByteArrayLiteral("CP") + QByteArray::number(nAnsiCodePage);
    QTextCodec *pCodec = QTextCodec::codecForName(baCodePageName);
    if (!pCodec) {
        baCodePageName = QByteArrayLiteral("Windows-") + QByteArray::number(nAnsiCodePage);
        pCodec = QTextCodec::codecForName(baCodePageName);
    }
    if (!pCodec && (nAnsiCodePage == 65001U)) pCodec = QTextCodec::codecForName("UTF-8");
    if (!pCodec) return QByteArray();
    QTextCodec::ConverterState state(QTextCodec::ConvertInvalidToNull);
    QByteArray baResult = pCodec->fromUnicode(sPassword.constData(), sPassword.size(), &state);
    if (state.invalidChars != 0) {
        innoSecureClear(&baResult);
        return QByteArray();
    }
#else
    // Qt6 without Qt5Compat: QStringConverter only covers the Unicode and
    // Latin-1 encodings, so an arbitrary Windows code page cannot be produced.
    if (nAnsiCodePage != 65001U) return QByteArray();
    QStringEncoder encoder(QStringEncoder::Utf8);
    QByteArray baResult = encoder.encode(sPassword);
    if (encoder.hasError()) {
        innoSecureClear(&baResult);
        return QByteArray();
    }
#endif

    QString sRoundTrip;
    if (!_decodeAnsiCodePage(baResult, nAnsiCodePage, &sRoundTrip) || (sRoundTrip != sPassword)) {
        innoSecureClear(&baResult);
        return QByteArray();
    }
    if (pbOk) *pbOk = true;
    return baResult;
}

bool XInnoSetup::_arcFourCrypt(QByteArray *pData, const QByteArray &baPassword, const QByteArray &baSalt, INNO_ENCRYPTION encryption)
{
    if (!pData || (baSalt.size() != 8) || ((encryption != INNO_ENCRYPTION_ARC4_MD5) && (encryption != INNO_ENCRYPTION_ARC4_SHA1))) return false;

    QCryptographicHash hash(encryption == INNO_ENCRYPTION_ARC4_SHA1 ? QCryptographicHash::Sha1 : QCryptographicHash::Md5);
    hash.addData(baSalt);
    hash.addData(baPassword);
    QByteArray baKey = hash.result();
    if (baKey.isEmpty()) return false;

    quint8 anState[256];
    for (qint32 i = 0; i < 256; i++) anState[i] = quint8(i);
    quint32 nY = 0;
    for (qint32 i = 0; i < 256; i++) {
        nY = (nY + anState[i] + quint8(baKey.at(i % baKey.size()))) & 0xffU;
        std::swap(anState[i], anState[nY]);
    }

    quint32 nX = 0;
    nY = 0;
    const qint64 nTotal = qint64(1000) + pData->size();
    for (qint64 i = 0; i < nTotal; i++) {
        nX = (nX + 1) & 0xffU;
        nY = (nY + anState[nX]) & 0xffU;
        std::swap(anState[nX], anState[nY]);
        if (i >= 1000) {
            const quint8 nKeyByte = anState[(quint32(anState[nX]) + anState[nY]) & 0xffU];
            const qint32 nOffset = qint32(i - 1000);
            (*pData)[nOffset] = char(quint8(pData->at(nOffset)) ^ nKeyByte);
        }
    }

    innoSecureClear(&baKey);
    volatile quint8 *pState = anState;
    for (qint32 i = 0; i < 256; i++) pState[i] = 0;
    return true;
}

bool XInnoSetup::_xChaCha20Crypt(QByteArray *pData, const QByteArray &baKey, const QByteArray &baNonce)
{
    if (!pData || (baKey.size() != 32) || (baNonce.size() != 24)) return false;

    static const quint32 anConstants[4] = {0x61707865U, 0x3320646eU, 0x79622d32U, 0x6b206574U};
    quint32 anHState[16] = {};
    for (qint32 i = 0; i < 4; i++) anHState[i] = anConstants[i];
    for (qint32 i = 0; i < 8; i++) {
        anHState[4 + i] = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baKey.constData() + i * 4));
    }
    for (qint32 i = 0; i < 4; i++) {
        anHState[12 + i] = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baNonce.constData() + i * 4));
    }
    innoChaChaRounds(anHState);

    QByteArray baSubKey(32, '\0');
    const qint32 anSubKeyWords[8] = {0, 1, 2, 3, 12, 13, 14, 15};
    for (qint32 i = 0; i < 8; i++) {
        qToLittleEndian<quint32>(anHState[anSubKeyWords[i]], reinterpret_cast<uchar *>(baSubKey.data() + i * 4));
    }

    quint32 anState[16] = {};
    for (qint32 i = 0; i < 4; i++) anState[i] = anConstants[i];
    for (qint32 i = 0; i < 8; i++) {
        anState[4 + i] = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baSubKey.constData() + i * 4));
    }
    anState[12] = 0;
    anState[13] = 0;
    anState[14] = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baNonce.constData() + 16));
    anState[15] = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baNonce.constData() + 20));

    qint32 nOffset = 0;
    while (nOffset < pData->size()) {
        quint32 anBlockState[16];
        for (qint32 i = 0; i < 16; i++) anBlockState[i] = anState[i];
        innoChaChaRounds(anBlockState);

        uchar aKeyStream[64];
        for (qint32 i = 0; i < 16; i++) {
            qToLittleEndian<quint32>(anBlockState[i] + anState[i], aKeyStream + i * 4);
        }

        const qint32 nBytes = qMin(64, pData->size() - nOffset);
        for (qint32 i = 0; i < nBytes; i++) {
            (*pData)[nOffset + i] = char((quint8)pData->at(nOffset + i) ^ aKeyStream[i]);
        }
        nOffset += nBytes;

        const quint32 nOldLow = anState[12]++;
        if (nOldLow == 0xffffffffU) {
            const quint32 nOldHigh = anState[13]++;
            if ((nOldHigh == 0xffffffffU) && (nOffset < pData->size())) {
                innoSecureClear(&baSubKey);
                return false;
            }
        }

        volatile quint32 *pBlock = anBlockState;
        for (qint32 i = 0; i < 16; i++) pBlock[i] = 0;
        volatile uchar *pStream = aKeyStream;
        for (qint32 i = 0; i < 64; i++) pStream[i] = 0;
    }

    innoSecureClear(&baSubKey);
    volatile quint32 *pHState = anHState;
    volatile quint32 *pState = anState;
    for (qint32 i = 0; i < 16; i++) {
        pHState[i] = 0;
        pState[i] = 0;
    }
    return true;
}

bool XInnoSetup::_deriveEncryptionKey(const QString &sPassword, const QByteArray &baSalt, qint32 nIterations, QByteArray *pKey, PDSTRUCT *pPdStruct)
{
    if (pKey) innoSecureClear(pKey);
    if (!pKey || (baSalt.size() != 16) || (nIterations <= 0) || (nIterations > INNO_MAX_KDF_ITERATIONS) ||
        (sPassword.size() > (std::numeric_limits<qint32>::max)() / 2) || !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    QByteArray baPassword(sPassword.size() * 2, '\0');
    for (qint32 i = 0; i < sPassword.size(); i++) {
        qToLittleEndian<quint16>(sPassword.at(i).unicode(), reinterpret_cast<uchar *>(baPassword.data() + i * 2));
    }

    QByteArray baSaltBlock = baSalt;
    baSaltBlock.append(QByteArray::fromHex("00000001"));
    QByteArray baU = innoHmacSha256(baPassword, baSaltBlock);
    QByteArray baResult = baU;

    for (qint32 i = 1; i < nIterations; i++) {
        if (((i & 0x3ff) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) {
            innoSecureClear(&baPassword);
            innoSecureClear(&baSaltBlock);
            innoSecureClear(&baU);
            innoSecureClear(&baResult);
            return false;
        }
        QByteArray baNext = innoHmacSha256(baPassword, baU);
        if (baNext.size() != baResult.size()) {
            innoSecureClear(&baNext);
            innoSecureClear(&baPassword);
            innoSecureClear(&baSaltBlock);
            innoSecureClear(&baU);
            innoSecureClear(&baResult);
            return false;
        }
        for (qint32 j = 0; j < baResult.size(); j++) {
            baResult[j] = char((quint8)baResult.at(j) ^ (quint8)baNext.at(j));
        }
        innoSecureClear(&baU);
        baU = baNext;
    }

    const bool bOk = (baResult.size() == 32) && XBinary::isPdStructNotCanceled(pPdStruct);
    if (bOk) *pKey = baResult;
    innoSecureClear(&baPassword);
    innoSecureClear(&baSaltBlock);
    innoSecureClear(&baU);
    innoSecureClear(&baResult);
    return bOk;
}

XInnoSetup::UNPACK_LIFETIME_STATE::~UNPACK_LIFETIME_STATE()
{
    const QSet<UNPACK_CONTEXT *> contexts = setContexts;
    setContexts.clear();
    for (UNPACK_CONTEXT *pContext : contexts) XInnoSetup::deleteUnpackContext(pContext);
}

void XInnoSetup::deleteUnpackContext(UNPACK_CONTEXT *pContext)
{
    if (!pContext) return;
    for (SLICE_SOURCE *pSlice : pContext->mapSliceSources) {
        if (!pSlice) continue;
        if (pSlice->pValidator) {
            pSlice->pValidator->releaseUnpackSource(&pSlice->validationState);
            delete pSlice->pValidator;
        }
        if (pSlice->pFile) {
            pSlice->pFile->close();
            delete pSlice->pFile;
        }
        delete pSlice;
    }
    pContext->mapSliceSources.clear();
    if (pContext->pSourceValidator) {
        pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
        delete pContext->pSourceValidator;
    }
    innoSecureClear(&pContext->baEncryptionKey);
    innoSecureClear(&pContext->baEncryptionBaseNonce);
    innoSecureClear(&pContext->baPasswordBytes);
    pContext->sPassword.fill(QChar());
    pContext->sPassword.clear();
    delete pContext;
}

XInnoSetup::XInnoSetup(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    m_pUnpackLifetimeState = QSharedPointer<UNPACK_LIFETIME_STATE>::create();
}

XInnoSetup::~XInnoSetup()
{
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    if (pLifetime) pLifetime->bOwnerAlive = false;
    m_pUnpackLifetimeState.clear();
}

QString XInnoSetup::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XINNOSETUP_STRUCTID, sizeof(_TABLE_XINNOSETUP_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XInnoSetup::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XINNOSETUP_STRUCTID, sizeof(_TABLE_XINNOSETUP_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XInnoSetup::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XINNOSETUP_STRUCTID, sizeof(_TABLE_XINNOSETUP_STRUCTID) / sizeof(XBinary::XCONVERT));
}

XBinary::FT XInnoSetup::getFileType()
{
    XPE pe(getDevice());

    if (pe.isValid() && pe.is64()) {
        return FT_PE64_INNOSETUP;
    }

    return FT_PE32_INNOSETUP;
}

bool XInnoSetup::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XInnoSetup> guardedThis(this);
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(guardedThis->getInternalInfo(pPdStruct));
    return guardedThis && pInfo && pInfo->bIsValid;
}

XInnoSetup::INTERNAL_INFO XInnoSetup::_getInternalInfo(PDSTRUCT *pPdStruct)
{
    return _analyse(pPdStruct);
}

// Cache format-specific parsing together with the XBinary memory map.
bool XInnoSetup::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XInnoSetup> guardedThis(this);
    const bool bAlreadyHandled = guardedThis->isInternalInfoHandled();
    if (!guardedThis) return false;

    if (!bAlreadyHandled) {
        const quint64 nTransaction = guardedThis->beginInternalInfoTransaction();
        if (!nTransaction) return false;

        // The transaction supplies the recursion sentinel. Keep every
        // source-derived value local until the same binding is revalidated.
        guardedThis->m_internalInfo = INTERNAL_INFO();
        INTERNAL_INFO info = guardedThis->_getInternalInfo(pPdStruct);
        if (!guardedThis) return false;
        if (!guardedThis->isInternalInfoTransactionCurrent(nTransaction) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }

        const XBinary::_MEMORY_MAP memoryMap = guardedThis->getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (!guardedThis) return false;
        if (!guardedThis->isInternalInfoTransactionCurrent(nTransaction) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
        info.memoryMap = memoryMap;

        if (!guardedThis->isInternalInfoTransactionCurrent(nTransaction)) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
        guardedThis->m_internalInfo = info;
        if (!guardedThis->commitInternalInfoTransaction(nTransaction, static_cast<XBinary::INTERNAL_INFO *>(&guardedThis->m_internalInfo))) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
    }

    return true;
}

void *XInnoSetup::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XInnoSetup> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XInnoSetup::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        setIsInternalInfoHandled(true);
        XBinary::setInternalInfo(static_cast<XBinary::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        setIsInternalInfoHandled(false);
        XBinary::setInternalInfo(nullptr);
    }
}

XInnoSetup::INTERNAL_INFO XInnoSetup::_analyse(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO result = {};

    const char *apszSignatures[] = {"Inno Setup Setup Data", "Inno Setup: Setup Data"};
    const qint32 nSignatureCount = sizeof(apszSignatures) / sizeof(const char *);
    qint64 nFileSize = getSize();

    // Prefer a loader table anchored to the exact setup-data offset. Pointer-
    // era installers and both supported pre-pointer layouts can be recognized
    // without scanning multi-megabyte payloads for a textual banner.
    if ((nFileSize >= 8) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const OFFSET_TABLE table = _findOffsetTable(pPdStruct);
        if (table.bIsValid && (table.nHeaderOffset >= 0) && (table.nHeaderOffset <= nFileSize - 8)) {
            INNO_VERSION version = _parseVersionId(read_array(table.nHeaderOffset, 8));
            if (!version.bIsValid && (table.nHeaderOffset <= nFileSize - 12)) version = _parseVersionId(read_array(table.nHeaderOffset, 12));
            if (!version.bIsValid && (table.nHeaderOffset <= nFileSize - 64)) version = _parseVersionId(read_array(table.nHeaderOffset, 64));

            const bool bLegacyPair = version.bIsValid && version.bLegacyShortId && !version.bWin16 &&
                                     (((version.nMinor == 9) && (table.nLoaderVersion == INNO_LOADER_LEGACY_VX)) ||
                                      ((version.nMinor == 11) && (table.nLoaderVersion == INNO_LOADER_LEGACY_EOF40)));
            const bool bNative12 = version.bIsValid && !version.bLegacyShortId && (innoVersionValue(version) == innoVersionValue(1, 2, 10));
            bool bNativePair = false;
            if (bNative12 && (table.nRevision == 0) && (table.nLoaderVersion == 2) && (nFileSize >= 0x40) && (read_uint16(0, false) == 0x5a4dU)) {
                const quint32 nNewHeaderOffset = read_uint32(0x3c, false);
                if ((nNewHeaderOffset >= 0x40U) && (quint64(nNewHeaderOffset) <= quint64(nFileSize - (version.bWin16 ? 2 : 4)))) {
                    bNativePair = version.bWin16 ? (read_uint16(nNewHeaderOffset, false) == 0x454eU) : (read_uint32(nNewHeaderOffset, false) == 0x00004550U);
                }
            }
            const bool bBannerPair = version.bIsValid && !version.bLegacyShortId && !bNative12 &&
                                     (table.nLoaderVersion != INNO_LOADER_LEGACY_VX) && (table.nLoaderVersion != INNO_LOADER_LEGACY_EOF40);

            if (bLegacyPair || bNativePair || bBannerPair) {
                result.bIsValid = true;
                result.nSignatureOffset = table.nHeaderOffset;
                if (version.bLegacyShortId) {
                    result.sVersion = QStringLiteral("1.%1-32").arg(version.nMinor, 2, 10, QChar('0'));
                } else {
                    result.sVersion = QStringLiteral("%1.%2.%3").arg(version.nMajor).arg(version.nMinor).arg(version.nPatch);
                    if (version.nRevision != 0) result.sVersion += QStringLiteral(".%1").arg(version.nRevision);
                    if (version.bIsx) result.sVersion += QStringLiteral(" with ISX");
                }
            }
        }
    }

    for (qint32 i = 0; (i < nSignatureCount) && (!result.bIsValid); i++) {
        QString sSignature = QString::fromLatin1(apszSignatures[i]);
        qint64 nOffset = find_utf8String(0, nFileSize, sSignature, pPdStruct);

        if (nOffset != -1) {
            result.bIsValid = true;
            result.nSignatureOffset = nOffset;

            const qint32 nWindowSize = 0x80;
            qint64 nRemaining = nFileSize - nOffset;
            if (nRemaining < 0) {
                nRemaining = 0;
            }
            qint64 nBytesToRead = qMin((qint64)nWindowSize, nRemaining);

            if (nBytesToRead > 0) {
                QByteArray baData = read_array_process(nOffset, nBytesToRead, pPdStruct);
                QString sWindow = QString::fromLatin1(baData.constData(), baData.size());

                qint32 nLeftBracket = sWindow.indexOf('(');
                if (nLeftBracket != -1) {
                    qint32 nRightBracket = sWindow.indexOf(')', nLeftBracket + 1);
                    if ((nRightBracket != -1) && (nRightBracket > nLeftBracket)) {
                        QString sVersionCandidate = sWindow.mid(nLeftBracket + 1, nRightBracket - nLeftBracket - 1).trimmed();
                        result.sVersion = sVersionCandidate;
                    }
                }

                if (result.sVersion.isEmpty()) {
                    qint32 nSignaturePos = sWindow.indexOf(sSignature);
                    if (nSignaturePos != -1) {
                        qint32 nSearchPos = nSignaturePos + sSignature.size();
                        while ((nSearchPos < sWindow.size()) && sWindow.at(nSearchPos).isSpace()) {
                            nSearchPos++;
                        }
                        if ((nSearchPos < sWindow.size()) && (sWindow.at(nSearchPos) == QChar('v'))) {
                            nSearchPos++;
                        }
                        qint32 nVersionStart = nSearchPos;
                        while ((nSearchPos < sWindow.size()) &&
                               ((sWindow.at(nSearchPos).isDigit()) || (sWindow.at(nSearchPos) == QChar('.')) || (sWindow.at(nSearchPos) == QChar('_')))) {
                            nSearchPos++;
                        }
                        if (nSearchPos > nVersionStart) {
                            result.sVersion = sWindow.mid(nVersionStart, nSearchPos - nVersionStart).trimmed();
                        }
                    }
                }
            }
        }
    }

    // Inno 1.2.x has no descriptive "Inno Setup Setup Data" marker. Recognize
    // either published native-width ID only through the genuine executable
    // kind, its fixed S02 table, and the exact ID at Offset0.
    if (!result.bIsValid && (nFileSize >= 0x40)) {
        const OFFSET_TABLE table = _findOffsetTable(pPdStruct);
        const quint32 nNewHeaderOffset = read_uint32(0x3c, false);
        if (table.bIsValid && (table.nRevision == 0) && (table.nLoaderVersion == 2) && (table.nHeaderOffset >= 0) && (table.nHeaderOffset <= nFileSize - 12) &&
            (read_uint16(0, false) == 0x5a4dU) && (nNewHeaderOffset >= 0x40U) && (quint64(nNewHeaderOffset) <= quint64(nFileSize - 4))) {
            const QByteArray baLegacyId = read_array(table.nHeaderOffset, 12);
            const bool bWin16 = (baLegacyId == QByteArray("i1.2.10--16\x1a", 12)) && (read_uint16(nNewHeaderOffset, false) == 0x454eU);
            const bool bWin32 = (baLegacyId == QByteArray("i1.2.10--32\x1a", 12)) && (read_uint32(nNewHeaderOffset, false) == 0x00004550U);
            if (bWin16 || bWin32) {
                result.bIsValid = true;
                result.nSignatureOffset = table.nHeaderOffset;
                result.sVersion = bWin16 ? QStringLiteral("1.2.10--16") : QStringLiteral("1.2.10--32");
            }
        }
    }

    return result;
}

// Synthetic InnoSetup test format ("ISDF" marker):
// After the null-terminated InnoSetup signature:
//   "ISDF"     - 4-byte magic
//   uint32_le  - number of files
//   uint64_le  - data area offset (absolute)
//   Per file entry:
//     uint16_le  - filename length
//     char[N]    - filename (UTF-8)
//     uint64_le  - data offset (absolute)
//     uint64_le  - data size
//     uint32_le  - CRC32

QList<XBinary::ARCHIVERECORD> XInnoSetup::_parseSyntheticFileEntries(qint64 nSignatureOffset, PDSTRUCT *pPdStruct)
{
    QList<ARCHIVERECORD> listResult;

    qint64 nFileSize = getSize();

    // Skip past the null-terminated signature string
    qint64 nOffset = nSignatureOffset;
    qint64 nMaxScan = qMin(nSignatureOffset + 0x100, nFileSize);

    while (nOffset < nMaxScan) {
        quint8 nByte = read_uint8(nOffset);

        if (nByte == 0) {
            nOffset++;  // Skip the null terminator
            break;
        }

        nOffset++;
    }

    // Check for "ISDF" magic (4 bytes)
    if (nOffset + 4 > nFileSize) {
        return listResult;
    }

    QByteArray baMagic = read_array(nOffset, 4);

    if (baMagic != QByteArray("ISDF", 4)) {
        return listResult;  // Not a synthetic test file
    }

    nOffset += 4;

    // Read number of files (uint32_le)
    if (nOffset + 4 > nFileSize) {
        return listResult;
    }

    quint32 nNumberOfFiles = read_uint32(nOffset, false);
    nOffset += 4;

    // Read data area offset (uint64_le)
    if (nOffset + 8 > nFileSize) {
        return listResult;
    }

    quint64 nDataAreaOffset = read_uint64(nOffset, false);
    nOffset += 8;

    Q_UNUSED(nDataAreaOffset)

    // Read file entries
    for (quint32 i = 0; (i < nNumberOfFiles) && isPdStructNotCanceled(pPdStruct); i++) {
        // Read filename length (uint16_le)
        if (nOffset + 2 > nFileSize) {
            break;
        }

        quint16 nNameLen = read_uint16(nOffset, false);
        nOffset += 2;

        // Read filename
        if (nOffset + nNameLen > nFileSize) {
            break;
        }

        QByteArray baName = read_array(nOffset, nNameLen);
        QString sFileName = QString::fromUtf8(baName);
        nOffset += nNameLen;

        // Read data offset (uint64_le)
        if (nOffset + 8 > nFileSize) {
            break;
        }

        quint64 nDataOffset = read_uint64(nOffset, false);
        nOffset += 8;

        // Read data size (uint64_le)
        if (nOffset + 8 > nFileSize) {
            break;
        }

        quint64 nDataSize = read_uint64(nOffset, false);
        nOffset += 8;

        // Read CRC32 (uint32_le)
        if (nOffset + 4 > nFileSize) {
            break;
        }

        quint32 nCRC32 = read_uint32(nOffset, false);
        nOffset += 4;

        // Build ARCHIVERECORD
        ARCHIVERECORD record = {};
        record.mapProperties.insert(FPART_PROP_ORIGINALNAME, sFileName);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)nDataSize);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, (qint64)nDataSize);
        record.mapProperties.insert(FPART_PROP_RESULTCRC, nCRC32);
        record.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
        record.mapProperties.insert(FPART_PROP_ISFOLDER, false);
        record.mapProperties.insert(FPART_PROP_STREAMOFFSET, (qint64)nDataOffset);
        record.mapProperties.insert(FPART_PROP_STREAMSIZE, (qint64)nDataSize);

        listResult.append(record);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XInnoSetup::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XBinary::getDefaultUnpackProperties();
    result.insert(UNPACK_PROP_PASSWORD, QString());
    result.insert(UNPACK_PROP_CODEPAGE, 0U);

    return result;
}

bool XInnoSetup::initUnpack(XBinary::UNPACK_STATE *pState, const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->baUnpackSourceToken.isEmpty()) return false;
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    InnoOperationGuard operationGuard(pLifetime);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XInnoSetup> guardedThis(this);
    if (pState->pContext) {
        UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!pLifetime->setContexts.contains(pOldContext) || (pOldContext->pOwnerState != pState)) return false;
        pLifetime->setContexts.remove(pOldContext);
        pState->pContext = nullptr;
        deleteUnpackContext(pOldContext);
        *pState = UNPACK_STATE();
        if (!guardedThis || !pLifetime->bOwnerAlive) return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    *pState = UNPACK_STATE();
    pState->mapUnpackProperties = mapProperties;

    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    XArchive *pSourceValidator = new XArchive(guardedSource.data());
    UNPACK_STATE sourceValidationState = {};
    if (!pSourceValidator->bindUnpackSource(&sourceValidationState, pPdStruct) || !guardedThis || !guardedSource) {
        pSourceValidator->releaseUnpackSource(&sourceValidationState);
        delete pSourceValidator;
        return false;
    }
    XInnoSetup detector(guardedSource.data(), isImage(), getModuleAddress());
    const qint64 nTotalSize = guardedSource->size();
    INTERNAL_INFO info = detector._analyse(pPdStruct);
    if (!guardedThis || !guardedSource || (nTotalSize < 0) || !info.bIsValid || !pSourceValidator->isUnpackSourceCurrent(&sourceValidationState, pPdStruct) ||
        !guardedThis || !guardedSource) {
        pSourceValidator->releaseUnpackSource(&sourceValidationState);
        delete pSourceValidator;
        return false;
    }

    UNPACK_CONTEXT *pContext = new UNPACK_CONTEXT();
    pContext->pOuterSourceDevice = guardedSource;
    pContext->nOwnerDeviceGeneration = getDeviceGeneration();
    pContext->pSourceValidator = pSourceValidator;
    pContext->sourceValidationState = UNPACK_STATE();
    if (!pContext->pSourceValidator->transferUnpackSourceOwnership(&sourceValidationState, &pContext->sourceValidationState)) {
        pContext->pSourceValidator->releaseUnpackSource(&sourceValidationState);
        deleteUnpackContext(pContext);
        return false;
    }
    pContext->bIsRealFormat = false;
    pContext->version = {};
    pContext->nSignatureOffset = info.nSignatureOffset;
    pContext->nDataStreamOffset = 0;
    pContext->encryption = INNO_ENCRYPTION_NONE;
    pContext->nEncryptionUse = 0;
    pContext->sPassword = mapProperties.value(UNPACK_PROP_PASSWORD).toString();
    pContext->nAnsiCodePageOverride = 0;
    if (mapProperties.contains(UNPACK_PROP_CODEPAGE)) {
        bool bCodePageOk = false;
        const quint32 nCodePage = mapProperties.value(UNPACK_PROP_CODEPAGE).toUInt(&bCodePageOk);
        QString sCodePageProbe;
        if (!bCodePageOk || ((nCodePage != 0) && !_decodeAnsiCodePage(QByteArray(), nCodePage, &sCodePageProbe))) {
            setPdStructErrorString(pPdStruct, tr("Invalid legacy Inno Setup code page"));
            deleteUnpackContext(pContext);
            return false;
        }
        pContext->nAnsiCodePageOverride = nCodePage;
    }
    pContext->nAnsiCodePage = 0;
    pContext->bHasPasswordBytes = false;
    if (mapProperties.contains(UNPACK_PROP_PASSWORD_BYTES)) {
        pContext->baPasswordBytes = mapProperties.value(UNPACK_PROP_PASSWORD_BYTES).toByteArray();
        pContext->bHasPasswordBytes = !pContext->baPasswordBytes.isNull();
        if (pContext->baPasswordBytes.size() > INNO_MAX_PASSWORD_BYTES) {
            setPdStructErrorString(pPdStruct, tr("Legacy Inno Setup password bytes exceed the supported size limit"));
            deleteUnpackContext(pContext);
            return false;
        }
    }
    pContext->chunkCache.nFirstSlice = 0;
    pContext->chunkCache.nLastSlice = 0;
    pContext->chunkCache.nChunkOffset = -1;
    pContext->chunkCache.nChunkCompressedSize = -1;
    pContext->chunkCache.compression = INNO_COMPRESSION_UNKNOWN;
    pContext->chunkCache.bEncrypted = false;

    // Try real InnoSetup format first
    if (detector._parseRealInnoSetup(pContext, pPdStruct)) {
        pContext->bIsRealFormat = true;
    } else {
        // Fallback to synthetic ISDF format
        QList<ARCHIVERECORD> listRecords = detector._parseSyntheticFileEntries(info.nSignatureOffset, pPdStruct);
        pContext->listAllRecords = listRecords;
        pContext->bIsRealFormat = false;
    }

    if (!guardedThis || !guardedSource || pContext->listAllRecords.isEmpty() ||
        !pContext->pSourceValidator->validateAndFinalizeUnpackSource(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !guardedSource) {
        deleteUnpackContext(pContext);
        return false;
    }

    pState->nTotalSize = nTotalSize;
    pContext->pOwnerState = pState;
    pState->pContext = pContext;
    pState->nNumberOfRecords = pContext->listAllRecords.count();
    pLifetime->setContexts.insert(pContext);

    return true;
}

XBinary::ARCHIVERECORD XInnoSetup::infoCurrent(XBinary::UNPACK_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    InnoOperationGuard operationGuard(pLifetime);
    if (!operationGuard.isAcquired()) return result;
    QPointer<XInnoSetup> guardedThis(this);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return result;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!pLifetime->setContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pSourceValidator ||
        (pState->nNumberOfRecords != pContext->listAllRecords.size()) ||
        (pContext->bIsRealFormat && (pContext->listRecordDataEntryIndexes.size() != pContext->listAllRecords.size())) ||
        !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !pLifetime->bOwnerAlive ||
        !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext))
        return result;
    result = pContext->listAllRecords.at(pState->nCurrentIndex);
    return result;
}

static bool xinnoFailOutput(bool bSeekableOutput, QIODevice *pDevice, XBinary::UNPACK_STATE *pState)
{
    if (bSeekableOutput) {
        XBinary::resize(pDevice, 0);
        pDevice->seek(0);
    }
    pState->nCurrentOffset = 0;
    return false;
}

static bool xinnoVerifyCRC(const XBinary::ARCHIVERECORD &record, XBinary::UNPACK_STATE *pState, QIODevice *pDevice, XBinary::PDSTRUCT *pPdStruct)
{
    XBinary::CRC_TYPE crcType = (XBinary::CRC_TYPE)record.mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();

    if (!XBinary::isUnpackCRCEnabled(pState->mapUnpackProperties, crcType) || (crcType == XBinary::CRC_TYPE_UNKNOWN) ||
        !record.mapProperties.contains(XBinary::FPART_PROP_RESULTCRC)) {
        return true;
    }

    if (!pDevice || !pDevice->isReadable() || !pDevice->seek(0)) {
        XBinary::setPdStructErrorString(pPdStruct, XInnoSetup::tr("CRC check requires a readable output device"));
        return false;
    }

    bool bCRCOk = XBinary::checkCRC(pDevice, crcType, record.mapProperties.value(XBinary::FPART_PROP_RESULTCRC), pPdStruct);

    if (!bCRCOk) {
        XBinary::setPdStructErrorString(pPdStruct, XInnoSetup::tr("Invalid CRC"));
    }

    return bCRCOk;
}

bool XInnoSetup::unpackCurrent(XBinary::UNPACK_STATE *pState, QIODevice *pDevice, XBinary::PDSTRUCT *pPdStruct)
{
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    InnoOperationGuard operationGuard(pLifetime);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XInnoSetup> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !guardedOutput || !guardedOutput->isOpen() || !guardedOutput->isWritable() ||
        guardedOutput->isSequential() || !guardedThis || !guardedOutput || (guardedOutput->openMode() & (QIODevice::Append | QIODevice::Text)) ||
        !XBinary::isResizeEnable(guardedOutput.data()) || !guardedThis || !guardedOutput || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const XBinary::PDSTRUCTLIFETIME progressLifetime = XBinary::retainPdStructLifetime(pPdStruct);
    // CRC traversal updates progress on every chunk.  Keep those notifications
    // private so a caller callback cannot destroy the public state mid-check;
    // the bridge still observes cancellation without invoking user code.
    XBinary::PDSTRUCT crcProgress = XBinary::createPdStruct();
    InnoCRCProgressBridge crcProgressBridge = {pPdStruct, progressLifetime};
    if (pPdStruct) {
        const XBinary::PDSTRUCT progressSnapshot = XBinary::getPdStructSnapshot(pPdStruct);
        crcProgress.nBufferSize.storeRelease(progressSnapshot.nBufferSize.loadAcquire());
        crcProgress.nFileBufferSize.storeRelease(progressSnapshot.nFileBufferSize.loadAcquire());
        XBinary::setPdStructCallback(&crcProgress, innoCRCProgressCallback, &crcProgressBridge);
    }

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);

    if (!pLifetime->setContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pSourceValidator ||
        (pState->nNumberOfRecords != pContext->listAllRecords.size()) ||
        (pContext->bIsRealFormat && (pContext->listRecordDataEntryIndexes.size() != pContext->listAllRecords.size())) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pContext->listAllRecords.count()) || XBinary::devicesAlias(pContext->pOuterSourceDevice.data(), guardedOutput.data()) || !guardedThis ||
        !guardedOutput || !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !guardedOutput ||
        !pLifetime->bOwnerAlive || !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext)) {
        return false;
    }

    ARCHIVERECORD record = pContext->listAllRecords.at(pState->nCurrentIndex);
    QTemporaryFile stage;
    if (!stage.open()) return false;
    UNPACK_STATE stageState = *pState;
    stageState.baUnpackSourceToken.clear();
    stageState.pContext = nullptr;
    const bool bOuterIsImage = isImage();
    const XADDR nOuterModuleAddress = getModuleAddress();
    XInnoSetup decoder(pContext->pOuterSourceDevice.data(), bOuterIsImage, nOuterModuleAddress);

    const InnoContextValidator contextValidator(guardedThis, guardedOutput, pLifetime, pContext, pState);

    qint64 nUncompressedSize = record.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();

    // Empty file — nothing to write
    if (nUncompressedSize == 0) {
        if (!innoVerifyChecksum(record, &stage, contextValidator, pPdStruct) ||
            !innoVerifyCRC(record, stageState, &stage, pPdStruct, progressLifetime, &crcProgress) || !contextValidator.isCurrent())
            return false;
    }

    if (nUncompressedSize < 0) return false;

    // This override bypasses the base decode chain's per-entry gate; account
    // the member here. Produced bytes are charged once: by writeUnpackData
    // through stageState (real format) or by _writeDevice through
    // decompressState (synthetic store). The publish copy stays uncharged.
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, record.mapProperties.value(FPART_PROP_ORIGINALNAME).toString())) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    if (nUncompressedSize > 0 && pContext->bIsRealFormat) {
        qint64 nDecompressedOffset = record.mapProperties.value(FPART_PROP_STREAMOFFSET).toLongLong();  // Offset within decompressed chunk
        const qint32 nLocationIndex = pContext->listRecordDataEntryIndexes.at(pState->nCurrentIndex);
        if ((nLocationIndex < 0) || (nLocationIndex >= pContext->listDataEntries.size())) return false;
        const DATA_ENTRY &targetEntry = pContext->listDataEntries.at(nLocationIndex);
        const INNO_COMPRESSION compression = targetEntry.compression;
        if ((compression == INNO_COMPRESSION_UNKNOWN) ||
            (compression != innoHandleMethodToCompression((HANDLE_METHOD)record.mapProperties.value(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN).toUInt())))
            return false;

        // Solid members share the same first slice and start offset. Equal
        // compressed sizes alone do not identify a chunk.
        if ((pContext->chunkCache.nFirstSlice != targetEntry.nFirstSlice) || (pContext->chunkCache.nLastSlice != targetEntry.nLastSlice) ||
            (pContext->chunkCache.nChunkOffset != targetEntry.nChunkStartOffset) || (pContext->chunkCache.nChunkCompressedSize != targetEntry.nChunkCompressedSize) ||
            (pContext->chunkCache.compression != compression) || (pContext->chunkCache.bEncrypted != targetEntry.bChunkEncrypted) ||
            pContext->chunkCache.baDecompressedData.isEmpty()) {
            qint64 nChunkOutputLimit = 0;
            for (const DATA_ENTRY &entry : pContext->listDataEntries) {
                if ((entry.nFirstSlice != targetEntry.nFirstSlice) || (entry.nLastSlice != targetEntry.nLastSlice) ||
                    (entry.nChunkStartOffset != targetEntry.nChunkStartOffset))
                    continue;
                if ((entry.compression != compression) || (entry.nChunkCompressedSize != targetEntry.nChunkCompressedSize) ||
                    (entry.bChunkEncrypted != targetEntry.bChunkEncrypted) || (entry.nChunkSubOffset > (std::numeric_limits<qint64>::max)() - entry.nOriginalSize))
                    return false;
                nChunkOutputLimit = qMax(nChunkOutputLimit, entry.nChunkSubOffset + entry.nOriginalSize);
            }

            // QByteArray-backed caching cannot safely represent an unbounded or
            // multi-gigabyte solid chunk. The decoder's processed window also
            // prevents corrupt input from allocating past the metadata-derived end.
            if ((nChunkOutputLimit <= 0) || (nChunkOutputLimit > (std::numeric_limits<qint32>::max)())) return false;
            pContext->chunkCache.baDecompressedData.clear();
            pContext->chunkCache.memoryReservation.release();
            if (!pContext->chunkCache.memoryReservation.acquire(stageState.mapUnpackProperties, nChunkOutputLimit)) {
                setPdStructErrorString(pPdStruct, tr("Inno Setup solid chunk exceeds the shared memory limit"));
                return false;
            }
            QByteArray baDecompressed = decoder._decompressDataChunk(pContext, targetEntry, nChunkOutputLimit, stageState.mapUnpackProperties, pPdStruct);

            if (!contextValidator.isCurrent() || baDecompressed.isEmpty()) {
                pContext->chunkCache.memoryReservation.release();
                qWarning() << "[InnoSetup] Failed to decompress data chunk at offset" << targetEntry.nChunkStartOffset;
                return false;
            }

            pContext->chunkCache.nFirstSlice = targetEntry.nFirstSlice;
            pContext->chunkCache.nLastSlice = targetEntry.nLastSlice;
            pContext->chunkCache.nChunkOffset = targetEntry.nChunkStartOffset;
            pContext->chunkCache.nChunkCompressedSize = targetEntry.nChunkCompressedSize;
            pContext->chunkCache.compression = compression;
            pContext->chunkCache.bEncrypted = targetEntry.bChunkEncrypted;
            pContext->chunkCache.baDecompressedData = baDecompressed;
        }

        // Extract this file's data from the decompressed chunk
        const QByteArray &baChunk = pContext->chunkCache.baDecompressedData;

        if ((nDecompressedOffset < 0) || (nDecompressedOffset > baChunk.size()) || (nUncompressedSize > ((qint64)baChunk.size() - nDecompressedOffset))) {
            qWarning() << "[InnoSetup] File data exceeds chunk boundary: offset" << nDecompressedOffset << "size" << nUncompressedSize << "chunk size" << baChunk.size();
            return false;
        }

        if (record.mapProperties.value(FPART_PROP_HANDLEMETHOD2, HANDLE_METHOD_UNKNOWN).toUInt() == HANDLE_METHOD_BCJ) {
            UNPACK_MEMORY_RESERVATION fileReservation;
            if (!fileReservation.acquire(stageState.mapUnpackProperties, nUncompressedSize)) {
                setPdStructErrorString(pPdStruct, tr("Inno Setup filtered file exceeds the shared memory limit"));
                return false;
            }
            QByteArray baFileData = baChunk.mid((qint32)nDecompressedOffset, (qint32)nUncompressedSize);
            if (baFileData.size() != nUncompressedSize) return false;
            innoDecodeExeFilter(&baFileData, pContext->version);
            if (!XBinary::writeUnpackData(&stageState, &stage, baFileData.constData(), baFileData.size(), pPdStruct) || !contextValidator.isCurrent()) return false;
        } else if (!XBinary::writeUnpackData(&stageState, &stage, baChunk.constData() + (qint32)nDecompressedOffset, nUncompressedSize, pPdStruct) ||
                   !contextValidator.isCurrent()) {
            return false;
        }
        if (!innoVerifyChecksum(record, &stage, contextValidator, pPdStruct) ||
            !innoVerifyCRC(record, stageState, &stage, pPdStruct, progressLifetime, &crcProgress) || !contextValidator.isCurrent())
            return false;
    } else if (nUncompressedSize > 0) {
        // Synthetic ISDF: stored data — direct copy using XStoreDecoder
        qint64 nStreamOffset = record.mapProperties.value(FPART_PROP_STREAMOFFSET).toLongLong();
        qint64 nStreamSize = record.mapProperties.value(FPART_PROP_STREAMSIZE).toLongLong();

        if (nStreamSize == 0) {
            return false;
        }

        // Clamp to file size
        qint64 nFileSize = decoder.getSize();
        if (!contextValidator.isCurrent()) return false;

        if ((nStreamOffset < 0) || (nStreamSize < 0) || (nStreamOffset > nFileSize)) {
            return false;
        }

        if (nStreamSize > (nFileSize - nStreamOffset)) {
            nStreamSize = nFileSize - nStreamOffset;

            if (nStreamSize <= 0) {
                return false;
            }
        }

        XBinary::DATAPROCESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_STORE);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nStreamSize);
        decompressState.spOutputBudget = pState->spOutputBudget;
        decompressState.pDeviceInput = pContext->pOuterSourceDevice.data();
        decompressState.pDeviceOutput = &stage;
        decompressState.nInputOffset = nStreamOffset;
        decompressState.nInputLimit = nStreamSize;
        decompressState.nProcessedOffset = 0;
        decompressState.nProcessedLimit = -1;

        if (!XStoreDecoder::decompress(&decompressState, pPdStruct) || (decompressState.nCountInput != nStreamSize) || (decompressState.nCountOutput != nStreamSize) ||
            !contextValidator.isCurrent()) {
            return false;
        }

        stageState.nCurrentOffset = decompressState.nCountOutput;
        if (!innoVerifyChecksum(record, &stage, contextValidator, pPdStruct) ||
            !innoVerifyCRC(record, stageState, &stage, pPdStruct, progressLifetime, &crcProgress) || !contextValidator.isCurrent())
            return false;
    }

    if (!guardedThis || !guardedOutput || !pLifetime->bOwnerAlive || !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext) ||
        !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !guardedOutput || !pLifetime->bOwnerAlive ||
        !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext))
        return false;
    InnoPublisher publisher(pContext->pOuterSourceDevice.data());
    UNPACK_STATE publicationState = {};
    if (!publisher.bindUnpackSource(&publicationState, pPdStruct) || !publisher.validateAndFinalizeUnpackSource(&publicationState, pPdStruct) || !guardedThis ||
        !guardedOutput || !pLifetime->bOwnerAlive || !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext) ||
        !publisher.publishUnpackOutput(&stage, guardedOutput.data(), &publicationState, pPdStruct) || !contextValidator.isCurrent())
        return false;
    pState->nCurrentOffset = stage.size();
    return true;
}

bool XInnoSetup::moveToNext(XBinary::UNPACK_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    InnoOperationGuard operationGuard(pLifetime);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XInnoSetup> guardedThis(this);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!pLifetime->setContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pSourceValidator ||
        (pState->nNumberOfRecords != pContext->listAllRecords.size()) ||
        (pContext->bIsRealFormat && (pContext->listRecordDataEntryIndexes.size() != pContext->listAllRecords.size())) ||
        !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !pLifetime->bOwnerAlive ||
        !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext))
        return false;
    pState->nCurrentIndex++;
    pState->nCurrentOffset = 0;
    const bool bCurrent = pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct);
    return bCurrent && guardedThis && pLifetime->bOwnerAlive && pLifetime->setContexts.contains(pContext) && (pState->pContext == pContext) &&
           (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XInnoSetup::finishUnpack(XBinary::UNPACK_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) return false;
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    InnoOperationGuard operationGuard(pLifetime);
    if (!operationGuard.isAcquired()) return false;
    if (pState->pContext) {
        UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!pLifetime->setContexts.contains(pContext) || (pContext->pOwnerState != pState)) return false;
        pLifetime->setContexts.remove(pContext);
        pState->pContext = nullptr;
        deleteUnpackContext(pContext);
    }
    *pState = UNPACK_STATE();
    return true;
}

// ============================================================================
// Real InnoSetup format parsing methods
// ============================================================================

XInnoSetup::OFFSET_TABLE XInnoSetup::_decodeLegacyOffsetTable(
    qint64 nTableOffset, const QByteArray &baExpectedId,
    quint16 nLoaderVersion, qint64 nFileSize, PDSTRUCT *pPdStruct)
{
    OFFSET_TABLE result = {};
    if ((nTableOffset < 0) || (nTableOffset > nFileSize - 40) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return result;
    }

    const QByteArray baTable = read_array(nTableOffset, 40);
    if ((baTable.size() != 40) || (baTable.left(12) != baExpectedId)) {
        return result;
    }

    result.nTableOffset = nTableOffset;
    result.nRevision = 0;
    result.nLoaderVersion = nLoaderVersion;
    result.nTotalSize = qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(baTable.constData() + 12));
    result.nExeOffset = qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(baTable.constData() + 16));
    const quint32 nExeCompressedSize = qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(baTable.constData() + 20));
    result.nExeUncompressedSize = qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(baTable.constData() + 24));
    result.nExeChecksum = qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(baTable.constData() + 28));
    const quint64 nHeaderOffset = qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(baTable.constData() + 32));
    const quint64 nDataOffset = qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(baTable.constData() + 36));
    const quint64 nExeOffset = quint64(result.nExeOffset);
    const quint64 nExeEnd = nExeOffset + quint64(nExeCompressedSize);

    if ((result.nTotalSize != quint64(nFileSize)) ||
        (result.nExeOffset <= 0) || (nExeCompressedSize == 0) ||
        (result.nExeUncompressedSize == 0) || (nExeEnd < nExeOffset) ||
        (nExeEnd != nHeaderOffset) ||
        (nHeaderOffset > quint64(nFileSize - 8)) ||
        (nDataOffset > quint64(nFileSize - 12))) {
        return OFFSET_TABLE();
    }

    const bool bEof40 = nLoaderVersion == INNO_LOADER_LEGACY_EOF40;
    const bool bLayoutValid =
        bEof40 ? ((nTableOffset == nFileSize - 40) &&
                  (nDataOffset >= 12) && (nDataOffset < nExeOffset) &&
                  (nExeOffset < nHeaderOffset) &&
                  (nHeaderOffset <= quint64(nTableOffset - 8)))
               : ((nTableOffset <= result.nExeOffset - 40) &&
                  (nExeOffset < nHeaderOffset) &&
                  (nHeaderOffset + 8 <= nDataOffset) &&
                  (nDataOffset <= quint64(nFileSize - 12)));
    if (!bLayoutValid) return OFFSET_TABLE();

    result.nHeaderOffset = (qint64)nHeaderOffset;
    result.nDataOffset = (qint64)nDataOffset;
    result.bIsValid = true;
    return result;
}

XInnoSetup::OFFSET_TABLE XInnoSetup::_findOffsetTable(PDSTRUCT *pPdStruct)
{
    OFFSET_TABLE invalidResult = {};
    const qint64 nFileSize = getSize();

    // Inno Setup 1.x through 5.1.4 stored a pointer/complement pair at 0x30.
    // The exact table ID selects its packed layout. S02/S04/S05 predate table
    // CRCs; require the complemented pointer, exact ID, and bounded embedded
    // offsets before trusting them (plus S02's historical terminal extent).
    if ((nFileSize >= 0x3c) && XBinary::isPdStructNotCanceled(pPdStruct) && (read_uint32(0x30, false) == 0x6f6e6e49U)) {
        const quint32 nTablePointer = read_uint32(0x34, false);
        const quint32 nNotTablePointer = read_uint32(0x38, false);
        if ((nTablePointer == ~nNotTablePointer) && (quint64(nTablePointer) <= quint64(nFileSize - 12))) {
            const QByteArray baId = read_array(nTablePointer, 12);
            const QByteArray ba02(reinterpret_cast<const char *>(g_aRDlPtS02Magic), 12);
            const QByteArray ba04(reinterpret_cast<const char *>(g_aRDlPtS04Magic), 12);
            const QByteArray ba05(reinterpret_cast<const char *>(g_aRDlPtS05Magic), 12);
            const QByteArray ba06(reinterpret_cast<const char *>(g_aRDlPtS06Magic), 12);
            const QByteArray ba07(reinterpret_cast<const char *>(g_aRDlPtS07Magic), 12);
            const quint16 nLoaderVersion = (baId == ba02) ? 2 : ((baId == ba04) ? 4 : ((baId == ba05) ? 5 : ((baId == ba06) ? 6 : ((baId == ba07) ? 7 : 0))));
            const qint32 nTableSize =
                ((nLoaderVersion == 2) || (nLoaderVersion == 6)) ? 44 : (((nLoaderVersion == 4) || (nLoaderVersion == 5) || (nLoaderVersion == 7)) ? 40 : 0);

            if ((nTableSize != 0) && (quint64(nTablePointer) <= quint64(nFileSize - nTableSize))) {
                const QByteArray baTable = read_array(nTablePointer, nTableSize);
                if (baTable.size() != nTableSize) return invalidResult;
                bool bTableCRCValid = true;
                if (nLoaderVersion >= 6) {
                    const quint32 nExpectedCRC = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + nTableSize - 4));
                    const quint32 nActualCRC = _getCRC32(baTable.constData(), nTableSize - 4, 0xFFFFFFFF, _getCRC32Table_EDB88320()) ^ 0xFFFFFFFF;
                    bTableCRCValid = nExpectedCRC == nActualCRC;
                }

                if (bTableCRCValid) {
                    OFFSET_TABLE candidate = {};
                    candidate.nTableOffset = nTablePointer;
                    candidate.nRevision = 0;
                    candidate.nLoaderVersion = nLoaderVersion;
                    candidate.nTotalSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 12));
                    candidate.nExeOffset = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 16));

                    quint32 nExeCompressedSize = 0;
                    quint64 nHeaderOffset = 0;
                    quint64 nDataOffset = 0;
                    if (nLoaderVersion != 7) {
                        nExeCompressedSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 20));
                        candidate.nExeUncompressedSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 24));
                        candidate.nExeChecksum = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 28));
                        nHeaderOffset = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + (nLoaderVersion == 2 ? 36 : 32)));
                        nDataOffset = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + (nLoaderVersion == 2 ? 40 : 36)));
                    } else {
                        candidate.nExeUncompressedSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 20));
                        candidate.nExeChecksum = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 24));
                        nHeaderOffset = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 28));
                        nDataOffset = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 32));
                    }

                    const quint64 nTableEnd = quint64(nTablePointer) + quint64(nTableSize);
                    const quint64 nMinimumHeaderEnd = nHeaderOffset + 64;
                    const bool bExeExtentValid = (nLoaderVersion == 7) ? (quint64(candidate.nExeOffset) < candidate.nTotalSize)
                                                                       : ((nExeCompressedSize > 0) && (quint64(candidate.nExeOffset) <= candidate.nTotalSize) &&
                                                                          (quint64(nExeCompressedSize) <= candidate.nTotalSize - quint64(candidate.nExeOffset)));

                    const bool bTotalExtentValid = (nLoaderVersion == 2) ? (candidate.nTotalSize == nTableEnd) : (candidate.nTotalSize >= nTableEnd);
                    if (bTotalExtentValid && (candidate.nTotalSize <= quint64(nFileSize)) && (candidate.nExeUncompressedSize > 0) && (candidate.nExeOffset > 0) &&
                        bExeExtentValid && (nHeaderOffset > 0) && (nMinimumHeaderEnd >= nHeaderOffset) && (nMinimumHeaderEnd <= candidate.nTotalSize) &&
                        ((nDataOffset == 0) || (nDataOffset <= candidate.nTotalSize - 4))) {
                        candidate.nHeaderOffset = (qint64)nHeaderOffset;
                        candidate.nDataOffset = (qint64)nDataOffset;
                        candidate.bIsValid = true;
                        return candidate;
                    }
                }
            }
        }
    }

    // Inno 1.09 and 1.11 predate the loader pointer at 0x30. They use the
    // same seven-dword body, but always a 40-byte table with Offset0/Offset1
    // at +32/+36. Decode only the two layouts whose complete extent and
    // ordering can be anchored to this device.
    const QByteArray ba02(reinterpret_cast<const char *>(g_aRDlPtS02Magic), 12);
    if ((nFileSize >= 40) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const OFFSET_TABLE eofCandidate = _decodeLegacyOffsetTable(
            nFileSize - 40, ba02, INNO_LOADER_LEGACY_EOF40,
            nFileSize, pPdStruct);
        if (eofCandidate.bIsValid) return eofCandidate;
    }

    const QByteArray baVx(reinterpret_cast<const char *>(g_aRDlPtSVxMagic), 12);
    const qint64 nLoaderWindowSize = qMin<qint64>(nFileSize, 1024LL * 1024);
    if ((nLoaderWindowSize >= baVx.size()) && (nLoaderWindowSize <= (std::numeric_limits<qint32>::max)()) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const QByteArray baLoaderWindow = read_array(0, nLoaderWindowSize);
        qint32 nSearchOffset = 0;
        while ((baLoaderWindow.size() == nLoaderWindowSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            const qint32 nRelativeOffset = baLoaderWindow.indexOf(baVx, nSearchOffset);
            if (nRelativeOffset < 0) break;
            const OFFSET_TABLE vxCandidate = _decodeLegacyOffsetTable(
                nRelativeOffset, baVx, INNO_LOADER_LEGACY_VX,
                nFileSize, pPdStruct);
            if (vxCandidate.bIsValid) return vxCandidate;
            if (nRelativeOffset == (std::numeric_limits<qint32>::max)()) break;
            nSearchOffset = nRelativeOffset + 1;
        }
    }

    const QByteArray baMagic(reinterpret_cast<const char *>(g_aRDlPtSMagic), g_nRDlPtSMagicSize);
    qint64 nSearchOffset = 0;

    // The ID may occur in payload text. Keep scanning until a complete, CRC-valid
    // loader table is found instead of trusting the first byte match.
    while ((nSearchOffset >= 0) && (nSearchOffset <= nFileSize - g_nRDlPtSMagicSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nTableOffset = find_array(nSearchOffset, nFileSize - nSearchOffset, baMagic.constData(), baMagic.size(), pPdStruct);
        if (nTableOffset == -1) break;

        if (nTableOffset <= nFileSize - 16) {
            const quint32 nRevision = read_uint32(nTableOffset + 12, false);
            const qint32 nTableSize = (nRevision == 1) ? 44 : ((nRevision == 2) ? 64 : 0);

            if ((nTableSize != 0) && (nTableOffset <= nFileSize - nTableSize)) {
                const QByteArray baTable = read_array(nTableOffset, nTableSize);
                if (baTable.size() == nTableSize) {
                    const quint32 nExpectedCRC = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + nTableSize - 4));
                    const quint32 nActualCRC = _getCRC32(baTable.constData(), nTableSize - 4, 0xFFFFFFFF, _getCRC32Table_EDB88320()) ^ 0xFFFFFFFF;

                    if (nExpectedCRC == nActualCRC) {
                        OFFSET_TABLE candidate = {};
                        candidate.nTableOffset = nTableOffset;
                        candidate.nRevision = nRevision;

                        quint64 nHeaderOffset = 0;
                        quint64 nDataOffset = 0;

                        if (nRevision == 1) {
                            candidate.nTotalSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 16));
                            candidate.nExeOffset = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 20));
                            candidate.nExeUncompressedSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 24));
                            candidate.nExeChecksum = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 28));
                            nHeaderOffset = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 32));
                            nDataOffset = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 36));
                        } else {
                            candidate.nTotalSize = qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(baTable.constData() + 16));
                            const quint64 nExeOffset = qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(baTable.constData() + 24));
                            candidate.nExeUncompressedSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 32));
                            candidate.nExeChecksum = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 36));
                            nHeaderOffset = qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(baTable.constData() + 40));
                            nDataOffset = qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(baTable.constData() + 48));
                            const quint32 nReserved = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTable.constData() + 56));
                            if ((nExeOffset > quint64((std::numeric_limits<qint64>::max)())) || (nReserved != 0)) {
                                nHeaderOffset = 0;
                                nDataOffset = 0;
                            } else {
                                candidate.nExeOffset = (qint64)nExeOffset;
                            }
                        }

                        const quint64 nTableEnd = quint64(nTableOffset) + quint64(nTableSize);
                        const quint64 nMinimumHeaderEnd = nHeaderOffset + 64;
                        if ((candidate.nTotalSize >= nTableEnd) && (candidate.nTotalSize <= quint64(nFileSize)) && (candidate.nExeUncompressedSize > 0) &&
                            (candidate.nExeOffset > 0) && (quint64(candidate.nExeOffset) < candidate.nTotalSize) && (nHeaderOffset > 0) &&
                            (nHeaderOffset <= quint64((std::numeric_limits<qint64>::max)())) && (nMinimumHeaderEnd >= nHeaderOffset) &&
                            (nMinimumHeaderEnd <= quint64(candidate.nExeOffset)) && ((nDataOffset == 0) || (nDataOffset <= nHeaderOffset)) &&
                            (nDataOffset <= quint64((std::numeric_limits<qint64>::max)()))) {
                            candidate.nHeaderOffset = (qint64)nHeaderOffset;
                            candidate.nDataOffset = (qint64)nDataOffset;
                            candidate.bIsValid = true;
                            return candidate;
                        }
                    }
                }
            }
        }

        if (nTableOffset == (std::numeric_limits<qint64>::max)()) break;
        nSearchOffset = nTableOffset + 1;
    }

    return invalidResult;
}

QByteArray XInnoSetup::_readLegacyBlock(qint64 nOffset, qint64 nLimit, qint64 *pnConsumed, PDSTRUCT *pPdStruct)
{
    if (pnConsumed) *pnConsumed = 0;
    const qint64 nFileSize = getSize();
    if ((nOffset < 0) || (nLimit < 0) || (nLimit > nFileSize) || (nOffset > nLimit - 16) || !XBinary::isPdStructNotCanceled(pPdStruct)) return QByteArray();

    const quint32 nExpectedHeaderAdler = read_uint32(nOffset, false);
    const QByteArray baHeader = read_array(nOffset + 4, 12);
    quint32 nActualHeaderAdler = 0;
    if ((baHeader.size() != 12) || !innoCalculateAdler32(baHeader, &nActualHeaderAdler, pPdStruct) || (nActualHeaderAdler != nExpectedHeaderAdler)) {
        setPdStructErrorString(pPdStruct, tr("Invalid legacy Inno Setup block header Adler-32"));
        return QByteArray();
    }

    const quint32 nCompressedSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baHeader.constData()));
    const quint32 nUncompressedSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baHeader.constData() + 4));
    const quint32 nExpectedDataAdler = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baHeader.constData() + 8));
    const bool bStored = nCompressedSize == 0xffffffffU;
    const quint64 nPayloadSize = bStored ? quint64(nUncompressedSize) : quint64(nCompressedSize);

    if ((nUncompressedSize == 0) || (nUncompressedSize > quint64(INNO_MAX_LEGACY_BLOCK_SIZE)) || (nPayloadSize == 0) ||
        (nPayloadSize > quint64(INNO_MAX_LEGACY_BLOCK_SIZE)) || (nPayloadSize > quint64(nLimit - nOffset - 16))) {
        setPdStructErrorString(pPdStruct, tr("Invalid legacy Inno Setup block size"));
        return QByteArray();
    }

    const QByteArray baPayload = read_array(nOffset + 16, (qint64)nPayloadSize);
    if (quint64(baPayload.size()) != nPayloadSize) return QByteArray();
    const QByteArray baResult = bStored ? baPayload : _decompressZlib(baPayload, nUncompressedSize);
    if (baResult.size() != nUncompressedSize) {
        setPdStructErrorString(pPdStruct, tr("Could not decompress legacy Inno Setup block"));
        return QByteArray();
    }

    quint32 nActualDataAdler = 0;
    if (!innoCalculateAdler32(baResult, &nActualDataAdler, pPdStruct) || (nActualDataAdler != nExpectedDataAdler)) {
        setPdStructErrorString(pPdStruct, tr("Invalid legacy Inno Setup block data Adler-32"));
        return QByteArray();
    }

    if (pnConsumed) *pnConsumed = 16 + (qint64)nPayloadSize;
    return baResult;
}

QByteArray XInnoSetup::_stripCRCChunks(const QByteArray &baData, bool *pbValid)
{
    // Block stream data is split into chunks: [4-byte CRC32][up to 4096 bytes payload]
    // Validate the CRC32 prefixes before concatenating any payload.
    if (pbValid) *pbValid = false;
    QByteArray baResult;
    qint32 nPos = 0;
    qint32 nSize = baData.size();

    while (nPos < nSize) {
        // A checksum without at least one payload byte is malformed. This also
        // rejects the one-to-four trailing bytes that the old parser ignored.
        if (nSize - nPos <= 4) return QByteArray();

        const quint32 nExpectedCRC = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData() + nPos));
        nPos += 4;

        qint32 nChunkSize = qMin(4096, nSize - nPos);
        const quint32 nActualCRC = _getCRC32(baData.constData() + nPos, nChunkSize, 0xFFFFFFFF, _getCRC32Table_EDB88320()) ^ 0xFFFFFFFF;
        if (nActualCRC != nExpectedCRC) return QByteArray();

        baResult.append(baData.constData() + nPos, nChunkSize);
        nPos += nChunkSize;
    }

    if (pbValid) *pbValid = true;
    return baResult;
}

QByteArray XInnoSetup::_decompressLZMA1(const QByteArray &baData)
{
    // LZMA1 format: 1-byte props + 4-byte dict_size (LE) + raw stream
    if (baData.size() < 6) {
        return QByteArray();
    }

    const quint32 nDictionarySize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData() + 1));
    if (nDictionarySize > INNO_MAX_HEADER_LZMA_DICTIONARY_SIZE) return QByteArray();

    QByteArray baProperty = baData.left(5);  // props byte + 4-byte dict_size
    QByteArray baCompressed = baData.mid(5);

    QBuffer bufInput;
    bufInput.setData(baCompressed);

    InnoBoundedBuffer bufOutput(INNO_MAX_HEADER_BLOCK_SIZE);

    if (!bufInput.open(QIODevice::ReadOnly) || !bufOutput.open(QIODevice::WriteOnly)) {
        bufInput.close();
        bufOutput.close();
        return QByteArray();
    }

    XBinary::DATAPROCESS_STATE decompressState = {};
    decompressState.pDeviceInput = &bufInput;
    decompressState.pDeviceOutput = &bufOutput;
    decompressState.nInputOffset = 0;
    decompressState.nInputLimit = baCompressed.size();
    decompressState.nProcessedOffset = 0;
    // Permit the first byte beyond the cap to reach InnoBoundedBuffer. It then
    // fails the decoder immediately; bytes outside the requested window would
    // otherwise be silently counted and discarded by XBinary::_writeDevice.
    decompressState.nProcessedLimit = INNO_MAX_HEADER_BLOCK_SIZE + 1;

    bool bOk = XLZMADecoder::decompress(&decompressState, baProperty, nullptr);

    bufInput.close();
    bufOutput.close();

    if (!bOk || decompressState.bReadError || decompressState.bWriteError || (decompressState.nCountInput != baCompressed.size()) || (decompressState.nCountOutput < 0) ||
        (decompressState.nCountOutput > INNO_MAX_HEADER_BLOCK_SIZE) || (decompressState.nCountOutput != bufOutput.data().size())) {
        return QByteArray();
    }

    return bufOutput.data();
}

QByteArray XInnoSetup::_decompressZlib(const QByteArray &baData, qint64 nExpectedSize)
{
    if (baData.isEmpty() || (nExpectedSize < -1) || (nExpectedSize > INNO_MAX_HEADER_BLOCK_SIZE)) return QByteArray();

    QBuffer bufInput;
    bufInput.setData(baData);
    InnoBoundedBuffer bufOutput(INNO_MAX_HEADER_BLOCK_SIZE);
    if (!bufInput.open(QIODevice::ReadOnly) || !bufOutput.open(QIODevice::ReadWrite)) {
        return QByteArray();
    }

    XBinary::DATAPROCESS_STATE decompressState = {};
    decompressState.pDeviceInput = &bufInput;
    decompressState.pDeviceOutput = &bufOutput;
    decompressState.nInputOffset = 0;
    decompressState.nInputLimit = baData.size();
    decompressState.nProcessedOffset = 0;
    decompressState.nProcessedLimit = ((nExpectedSize >= 0) ? nExpectedSize : INNO_MAX_HEADER_BLOCK_SIZE) + 1;

    const bool bOk = XDeflateDecoder::decompress_zlib(&decompressState, nullptr);
    bufInput.close();
    bufOutput.close();

    if (!bOk || decompressState.bReadError || decompressState.bWriteError || (decompressState.nCountInput != baData.size()) || (decompressState.nCountOutput < 0) ||
        (decompressState.nCountOutput > INNO_MAX_HEADER_BLOCK_SIZE) || ((nExpectedSize >= 0) && (decompressState.nCountOutput != nExpectedSize)) ||
        (decompressState.nCountOutput != bufOutput.data().size()))
        return QByteArray();

    return bufOutput.data();
}

QByteArray XInnoSetup::_readBlockStream(qint64 nOffset, qint64 *pnConsumed, PDSTRUCT *pPdStruct, bool b64BitStoredSize, const QByteArray &baCryptKey,
                                        const QByteArray &baCryptNonce, const INNO_VERSION *pVersion)
{
    // Block stream format:
    // uint32 CRC32 of (stored_size + compressed_flag)
    // uint32/uint64 stored_size (widened by setup-data 6.7.0)
    // uint8  compressed_flag (1 = LZMA1 compressed)
    // <stored_size bytes of CRC-chunked data>

    if (pnConsumed) *pnConsumed = 0;

    const qint64 nFileSize = getSize();
    const bool bHistorical = pVersion && pVersion->bIsValid && (pVersion->nMajor < 5);
    const quint64 nVersion = pVersion ? innoVersionValue(*pVersion) : 0;
    const bool bOldDeflateHeader = bHistorical && (nVersion < innoVersionValue(4, 0, 9));

    // 6.5/6.6 revision-2 installers retain the packed uint32 + Boolean
    // record. Setup-data 6.7.0 widens StoredSize to UInt64.
    const qint64 nHeaderSize = bOldDeflateHeader ? 12 : (b64BitStoredSize ? 13 : 9);

    if ((nOffset < 0) || (nHeaderSize <= 0) || (nFileSize < nHeaderSize) || (nOffset > nFileSize - nHeaderSize)) {
        return QByteArray();
    }

    // The header checksum covers the encoded stored size and compressed flag.
    const quint32 nExpectedHeaderCRC = read_uint32(nOffset, false);
    const QByteArray baHeaderPayload = read_array(nOffset + 4, nHeaderSize - 4);
    if (baHeaderPayload.size() != nHeaderSize - 4) return QByteArray();
    const quint32 nActualHeaderCRC = _getCRC32(baHeaderPayload, 0xFFFFFFFF, _getCRC32Table_EDB88320()) ^ 0xFFFFFFFF;
    if (nActualHeaderCRC != nExpectedHeaderCRC) {
        setPdStructErrorString(pPdStruct, tr("Invalid Inno Setup block header CRC"));
        return QByteArray();
    }

    quint64 nStoredSize = 0;
    quint8 nCompressedFlag = 0;
    qint64 nExpectedSize = -1;
    if (bOldDeflateHeader) {
        const qint32 nCompressedSize = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(baHeaderPayload.constData()));
        const qint32 nUncompressedSize = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(baHeaderPayload.constData() + 4));
        if ((nUncompressedSize <= 0) || (nUncompressedSize > INNO_MAX_HEADER_BLOCK_SIZE) || ((nCompressedSize != -1) && (nCompressedSize <= 0))) {
            setPdStructErrorString(pPdStruct, tr("Invalid historical Inno Setup block size"));
            return QByteArray();
        }
        const quint64 nPayloadSize = (nCompressedSize == -1) ? quint64(nUncompressedSize) : quint64(nCompressedSize);
        const quint64 nChunkCount = (nPayloadSize + 4095) / 4096;
        nStoredSize = nPayloadSize + nChunkCount * 4;
        nCompressedFlag = (nCompressedSize == -1) ? 0 : 1;
        nExpectedSize = nUncompressedSize;
    } else {
        nStoredSize = b64BitStoredSize ? read_uint64(nOffset + 4, false) : read_uint32(nOffset + 4, false);
        nCompressedFlag = read_uint8(nOffset + nHeaderSize - 1);
        if (nCompressedFlag > 1) {
            setPdStructErrorString(pPdStruct, tr("Invalid Inno Setup block compression flag"));
            return QByteArray();
        }
    }

    if (nStoredSize > quint64(INNO_MAX_HEADER_STORED_SIZE)) {
        setPdStructErrorString(pPdStruct, tr("Inno Setup header block exceeds the supported size limit"));
        return QByteArray();
    }

    qint64 nConsumed = nHeaderSize + (qint64)nStoredSize;

    if ((nOffset < 0) || (nConsumed < nHeaderSize) || (nOffset > nFileSize - nConsumed)) {
        return QByteArray();
    }

    // Read the stored data
    QByteArray baStoredData = read_array(nOffset + nHeaderSize, (qint64)nStoredSize);
    if (quint64(baStoredData.size()) != nStoredSize) return QByteArray();

    // Strip and validate per-chunk CRC32 prefixes.
    bool bChunksValid = false;
    QByteArray baPayload = _stripCRCChunks(baStoredData, &bChunksValid);
    if (!bChunksValid) {
        setPdStructErrorString(pPdStruct, tr("Invalid Inno Setup block CRC"));
        return QByteArray();
    }
    if (baPayload.size() > INNO_MAX_HEADER_BLOCK_SIZE) {
        setPdStructErrorString(pPdStruct, tr("Inno Setup header block exceeds the supported size limit"));
        return QByteArray();
    }

    const bool bDecrypt = !baCryptKey.isEmpty() || !baCryptNonce.isEmpty();
    if (bDecrypt && !_xChaCha20Crypt(&baPayload, baCryptKey, baCryptNonce)) {
        setPdStructErrorString(pPdStruct, tr("Invalid Inno Setup header encryption parameters"));
        return QByteArray();
    }

    if (pnConsumed) {
        *pnConsumed = nConsumed;
    }

    if (nCompressedFlag == 1) {
        if (bHistorical && (nVersion < innoVersionValue(4, 1, 6))) {
            return _decompressZlib(baPayload, nExpectedSize);
        }
        return _decompressLZMA1(baPayload);
    }

    if ((nExpectedSize >= 0) && (baPayload.size() != nExpectedSize)) return QByteArray();

    return baPayload;
}

QString XInnoSetup::_readWideString(const QByteArray &baData, qint32 nOffset, qint32 *pnNewOffset)
{
    // Delphi WideString format: uint32 length_in_bytes, then UTF-16LE data
    if ((nOffset < 0) || (nOffset > baData.size() - 4)) {
        if (pnNewOffset) {
            *pnNewOffset = nOffset;
        }

        return QString();
    }

    quint32 nByteLen = qFromLittleEndian<quint32>((const uchar *)(baData.constData() + nOffset));

    if (nByteLen == 0) {
        if (pnNewOffset) {
            *pnNewOffset = nOffset + 4;
        }

        return QString();
    }

    if (((nByteLen & 1U) != 0) || (nByteLen > 0x1000000) || (nByteLen > quint32(baData.size() - nOffset - 4))) {
        if (pnNewOffset) {
            *pnNewOffset = nOffset;
        }

        return QString();
    }

    const qint32 nCharCount = (qint32)(nByteLen / 2);
    QString sResult(nCharCount, QChar());
    const uchar *pStringData = reinterpret_cast<const uchar *>(baData.constData() + nOffset + 4);
    for (qint32 i = 0; i < nCharCount; i++) {
        sResult[i] = QChar(qFromLittleEndian<quint16>(pStringData + i * 2));
    }

    if (pnNewOffset) {
        *pnNewOffset = nOffset + 4 + nByteLen;
    }

    return sResult;
}

QString XInnoSetup::_decodeWindows1252(const char *pData, qint32 nSize)
{
    static const ushort anC1Map[32] = {
        0x20ac, 0xfffd, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0xfffd, 0x017d, 0xfffd,
        0xfffd, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0xfffd, 0x017e, 0x0178,
    };

    QString result;

    if (!pData || (nSize <= 0)) return result;
    result.reserve(nSize);

    for (qint32 i = 0; i < nSize; i++) {
        const quint8 nByte = (quint8)pData[i];
        result.append(QChar((nByte >= 0x80) && (nByte <= 0x9f) ? anC1Map[nByte - 0x80] : nByte));
    }

    return result;
}

QString XInnoSetup::_readAnsiString(const QByteArray &baData, qint32 nOffset, qint32 *pnNewOffset)
{
    // Delphi AnsiString format (Inno < 5.3.0): uint32 length_in_bytes, then single-byte chars.
    if (nOffset + 4 > baData.size()) {
        if (pnNewOffset) {
            *pnNewOffset = nOffset;
        }

        return QString();
    }

    quint32 nByteLen = qFromLittleEndian<quint32>((const uchar *)(baData.constData() + nOffset));

    if (nByteLen == 0) {
        if (pnNewOffset) {
            *pnNewOffset = nOffset + 4;
        }

        return QString();
    }

    if ((nByteLen > 0x1000000) || (nOffset + 4 + (qint32)nByteLen > baData.size())) {
        if (pnNewOffset) {
            *pnNewOffset = nOffset;
        }

        return QString();
    }

    // The default Inno ANSI code page is Windows-1252. Language-specific code pages require
    // parsing the selected language entry and are deliberately not guessed here.
    QString sResult = _decodeWindows1252(baData.constData() + nOffset + 4, (qint32)nByteLen);

    if (pnNewOffset) {
        *pnNewOffset = nOffset + 4 + nByteLen;
    }

    return sResult;
}

QByteArray XInnoSetup::_readAnsiBytes(const QByteArray &baData, qint32 nOffset, qint32 *pnNewOffset)
{
    if ((nOffset < 0) || (nOffset > baData.size() - 4)) {
        if (pnNewOffset) *pnNewOffset = nOffset;
        return QByteArray();
    }

    const quint32 nByteLen = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData() + nOffset));
    if ((nByteLen > 0x1000000U) || (nByteLen > quint32(baData.size() - nOffset - 4))) {
        if (pnNewOffset) *pnNewOffset = nOffset;
        return QByteArray();
    }

    if (pnNewOffset) *pnNewOffset = nOffset + 4 + (qint32)nByteLen;
    return baData.mid(nOffset + 4, (qint32)nByteLen);
}

bool XInnoSetup::_decodeAnsiCodePage(const QByteArray &baData, quint32 nCodePage, QString *psResult)
{
    if (psResult) psResult->clear();
    if (!psResult || (nCodePage == 0)) return false;

#ifdef Q_OS_WIN
    if (!IsValidCodePage((UINT)nCodePage)) return false;
    if (baData.isEmpty()) return true;

    DWORD nFlags = MB_ERR_INVALID_CHARS;
    int nChars = MultiByteToWideChar((UINT)nCodePage, nFlags, baData.constData(), baData.size(), nullptr, 0);
    if ((nChars <= 0) && (GetLastError() == ERROR_INVALID_FLAGS)) {
        nFlags = 0;
        nChars = MultiByteToWideChar((UINT)nCodePage, nFlags, baData.constData(), baData.size(), nullptr, 0);
    }
    if (nChars <= 0) return false;

    QString sResult(nChars, QChar());
    if (MultiByteToWideChar((UINT)nCodePage, nFlags, baData.constData(), baData.size(), reinterpret_cast<LPWSTR>(sResult.data()), nChars) != nChars) return false;

    DWORD nOutFlags = 0;
    BOOL bUsedDefault = FALSE;
    LPBOOL pUsedDefault = &bUsedDefault;
    if ((nCodePage == CP_UTF8) || (nCodePage == 54936U)) {
        nOutFlags = WC_ERR_INVALID_CHARS;
        pUsedDefault = nullptr;
    } else {
        nOutFlags = WC_NO_BEST_FIT_CHARS;
    }

    int nBytes = WideCharToMultiByte((UINT)nCodePage, nOutFlags, reinterpret_cast<LPCWCH>(sResult.constData()), sResult.size(), nullptr, 0, nullptr, pUsedDefault);
    if ((nBytes <= 0) && (GetLastError() == ERROR_INVALID_FLAGS)) {
        nOutFlags = 0;
        bUsedDefault = FALSE;
        pUsedDefault = ((nCodePage == CP_UTF8) || (nCodePage == 54936U)) ? nullptr : &bUsedDefault;
        nBytes = WideCharToMultiByte((UINT)nCodePage, nOutFlags, reinterpret_cast<LPCWCH>(sResult.constData()), sResult.size(), nullptr, 0, nullptr, pUsedDefault);
    }
    if ((nBytes <= 0) || bUsedDefault) return false;
    QByteArray baRoundTrip(nBytes, '\0');
    if (WideCharToMultiByte((UINT)nCodePage, nOutFlags, reinterpret_cast<LPCWCH>(sResult.constData()), sResult.size(), baRoundTrip.data(), baRoundTrip.size(), nullptr,
                            pUsedDefault) != nBytes ||
        bUsedDefault || (baRoundTrip != baData))
        return false;
    *psResult = sResult;
    return true;
#elif (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
    QByteArray baCodePageName = QByteArrayLiteral("CP") + QByteArray::number(nCodePage);
    QTextCodec *pCodec = QTextCodec::codecForName(baCodePageName);
    if (!pCodec) {
        baCodePageName = QByteArrayLiteral("Windows-") + QByteArray::number(nCodePage);
        pCodec = QTextCodec::codecForName(baCodePageName);
    }
    if (!pCodec && (nCodePage == 65001U)) pCodec = QTextCodec::codecForName("UTF-8");
    if (!pCodec) return false;

    QTextCodec::ConverterState decodeState(QTextCodec::ConvertInvalidToNull);
    const QString sResult = pCodec->toUnicode(baData.constData(), baData.size(), &decodeState);
    if (decodeState.invalidChars != 0) return false;
    QTextCodec::ConverterState encodeState(QTextCodec::ConvertInvalidToNull);
    const QByteArray baRoundTrip = pCodec->fromUnicode(sResult.constData(), sResult.size(), &encodeState);
    if ((encodeState.invalidChars != 0) || (baRoundTrip != baData)) return false;
    *psResult = sResult;
    return true;
#else
    // Qt6 without Qt5Compat: see the note in _encodeLegacyPassword above.
    if (nCodePage != 65001U) return false;
    QStringDecoder decoder(QStringDecoder::Utf8);
    const QString sResult = decoder.decode(baData);
    if (decoder.hasError()) return false;
    QStringEncoder encoder(QStringEncoder::Utf8);
    const QByteArray baRoundTrip = encoder.encode(sResult);
    if (encoder.hasError() || (baRoundTrip != baData)) return false;
    *psResult = sResult;
    return true;
#endif
}

QString XInnoSetup::_getOptionalDestinationPath(const QString &sDestination)
{
    const QString sNormalized = QString(sDestination).replace(QLatin1Char('\\'), QLatin1Char('/'));
    const qint32 nLastSlash = sNormalized.lastIndexOf(QLatin1Char('/'));
    return (nLastSlash > 0) ? sNormalized.left(nLastSlash) : QString();
}

void XInnoSetup::_setDestinationProperties(ARCHIVERECORD *pRecord, const QString &sDestination)
{
    if (!pRecord) return;

    const QString sFileName = QString(sDestination).replace(QLatin1Char('\\'), QLatin1Char('/'));
    pRecord->mapProperties.insert(FPART_PROP_ORIGINALNAME, sFileName);

    const QString sOptionalPath = _getOptionalDestinationPath(sFileName);
    if (!sOptionalPath.isEmpty()) {
        pRecord->mapProperties.insert(FPART_PROP_OPTIONAL_PATH, sOptionalPath);
    }
}

QString XInnoSetup::_readSetupString(const QByteArray &baData, qint32 nOffset, qint32 *pnNewOffset, bool bUnicode)
{
    if (bUnicode) {
        return _readWideString(baData, nOffset, pnNewOffset);
    }

    return _readAnsiString(baData, nOffset, pnNewOffset);
}

XInnoSetup::INNO_VERSION XInnoSetup::_parseVersionId(const QByteArray &baVersionId)
{
    INNO_VERSION result = {};

    // Inno 1.09/1.11 used compact binary IDs. Keep this deliberately
    // generation-whitelisted: adjacent releases changed serialized records.
    if ((baVersionId.size() == 8) && (baVersionId.at(0) == 'i') && (baVersionId.at(4) == '-') && (baVersionId.at(7) == '\x1a') &&
        ((baVersionId.mid(5, 2) == "16") || (baVersionId.mid(5, 2) == "32"))) {
        bool bDigits = true;
        for (qint32 i = 1; i <= 3; i++) {
            if ((baVersionId.at(i) < '0') || (baVersionId.at(i) > '9')) bDigits = false;
        }
        const QByteArray baCompactVersion = baVersionId.mid(1, 3);
        if (bDigits && ((baCompactVersion == "109") || (baCompactVersion == "111"))) {
            result.bIsValid = true;
            result.bUnicode = false;
            result.bWin16 = baVersionId.mid(5, 2) == "16";
            result.bLegacyShortId = true;
            result.nMajor = 1;
            result.nMinor = (baCompactVersion == "109") ? 9 : 11;
            return result;
        }
    }

    // Inno 1.2.10-1.2.16 used native-width 12-byte IDs. The bitness is part of
    // the schema because Integer, Cardinal, pointer-bearing record tails, and
    // file times differ between the two compilers.
    if ((baVersionId == QByteArray("i1.2.10--16\x1a", 12)) || (baVersionId == QByteArray("i1.2.10--32\x1a", 12))) {
        result.bIsValid = true;
        result.bUnicode = false;
        result.bWin16 = baVersionId.at(9) == '1';
        result.nMajor = 1;
        result.nMinor = 2;
        result.nPatch = 10;
        result.nRevision = 0;
        return result;
    }

    if (baVersionId.size() != 64) return result;

    qint32 nLength = baVersionId.indexOf('\0');
    if (nLength < 0) nLength = baVersionId.size();

    for (qint32 i = nLength; i < baVersionId.size(); i++) {
        if (baVersionId.at(i) != '\0') return result;
    }

    const QByteArray baId = baVersionId.left(nLength);
    const QByteArray baPrefix("Inno Setup Setup Data (");

    if (!baId.startsWith(baPrefix)) return result;

    const qint32 nClose = baId.indexOf(')', baPrefix.size());
    if (nClose < 0) return result;

    QByteArray baVersion = baId.mid(baPrefix.size(), nClose - baPrefix.size());
    const QByteArray baSuffix = baId.mid(nClose + 1);

    bool bIsx = false;
    if (!baSuffix.isEmpty() && (baSuffix != " (u)") && (baSuffix != " (U)")) {
        static const QByteArray baIsxPrefix(" with ISX (");
        if (!baSuffix.startsWith(baIsxPrefix) || !baSuffix.endsWith(')')) return result;
        const QByteArray baIsxVersion = baSuffix.mid(baIsxPrefix.size(), baSuffix.size() - baIsxPrefix.size() - 1);
        const QList<QByteArray> listIsxParts = baIsxVersion.split('.');
        if ((listIsxParts.size() != 3) && (listIsxParts.size() != 4)) return result;
        for (const QByteArray &baPart : listIsxParts) {
            if (baPart.isEmpty()) return result;
            for (char ch : baPart) {
                if ((ch < '0') || (ch > '9')) return result;
            }
            bool bOk = false;
            const uint nValue = baPart.toUInt(&bOk);
            if (!bOk || (nValue > 0xffff)) return result;
        }
        bIsx = true;
    }

    bool bAlphaSchema = false;
    if (baVersion.endsWith('a')) {
        bAlphaSchema = true;
        baVersion.chop(1);
    }

    const QList<QByteArray> listParts = baVersion.split('.');
    if ((listParts.count() != 3) && (listParts.count() != 4)) return result;

    quint16 anParts[4] = {};

    for (qint32 i = 0; i < listParts.count(); i++) {
        if (listParts.at(i).isEmpty()) return result;
        for (char ch : listParts.at(i)) {
            if ((ch < '0') || (ch > '9')) return result;
        }
        bool bOk = false;
        const uint nValue = listParts.at(i).toUInt(&bOk);
        if (!bOk || (nValue > 0xffff)) return result;
        anParts[i] = (quint16)nValue;
    }

    result.nMajor = anParts[0];
    result.nMinor = anParts[1];
    result.nPatch = anParts[2];
    result.nRevision = anParts[3];
    result.bUnicode = !baSuffix.isEmpty() || (result.nMajor >= 6);
    result.bIsx = bIsx;
    if (bIsx) result.bUnicode = false;
    result.bWin16 = false;

    // These are the stock JR Software base setup-data IDs emitted before 5.0.
    // ISX record deltas use a separate, bounded data-chunk fallback.
    const bool bHistoricalSchema =
        (result.nRevision == 0) &&
        (((result.nMajor == 1) && (result.nMinor == 3) &&
          ((result.nPatch == 3) || (result.nPatch == 9) || (result.nPatch == 10) || (result.nPatch == 21) || (result.nPatch == 24) || (result.nPatch == 25))) ||
         ((result.nMajor == 2) && (result.nMinor == 0) &&
          ((result.nPatch == 0) || (result.nPatch == 1) || (result.nPatch == 2) || (result.nPatch == 5) || (result.nPatch == 6) || (result.nPatch == 7) ||
           (result.nPatch == 8) || (result.nPatch == 11) || (result.nPatch == 17) || (result.nPatch == 18))) ||
         ((result.nMajor == 3) && (result.nMinor == 0) &&
          ((result.nPatch == 0) || (result.nPatch == 1) || (result.nPatch == 3) || (result.nPatch == 4) || (result.nPatch == 5))) ||
         ((result.nMajor == 4) && (result.nMinor == 0) &&
          ((result.nPatch == 0) || (result.nPatch == 1) || (result.nPatch == 3) || (result.nPatch == 5) || (result.nPatch == 9) || (result.nPatch == 10) ||
           (result.nPatch == 11))) ||
         ((result.nMajor == 4) && (result.nMinor == 1) &&
          ((result.nPatch == 0) || (result.nPatch == 2) || (result.nPatch == 3) || (result.nPatch == 4) || (result.nPatch == 5) || (result.nPatch == 6) ||
           (result.nPatch == 8))) ||
         ((result.nMajor == 4) && (result.nMinor == 2) && (result.nPatch <= 6)));
    const bool bExpectedAlphaSchema = ((result.nMajor == 2) && (result.nMinor == 0) && (result.nPatch == 6)) ||
                                      ((result.nMajor == 3) && (result.nMinor == 0) && (result.nPatch == 0)) ||
                                      ((result.nMajor == 4) && (result.nMinor == 0) && (result.nPatch == 0));
    if ((bHistoricalSchema && (result.bUnicode || (bAlphaSchema != bExpectedAlphaSchema))) || (!bHistoricalSchema && bAlphaSchema) ||
        (!bHistoricalSchema && ((result.nMajor < 5) || (result.nMajor > 7))))
        return INNO_VERSION();
    if ((result.nMajor == 7) && ((result.nMinor != 0) || (result.nPatch != 0) || (result.nRevision != 3))) return INNO_VERSION();

    result.bIsValid = true;
    return result;
}

XInnoSetup::HEADER_INFO XInnoSetup::_parseHeaderInfo(const QByteArray &baBlock1, const INNO_VERSION &version)
{
    HEADER_INFO result = {};
    result.compression = INNO_COMPRESSION_UNKNOWN;
    result.encryption = INNO_ENCRYPTION_NONE;

    if (!version.bIsValid || baBlock1.isEmpty()) return result;

    const quint64 nVersion = innoVersionValue(version);

    qint32 nOffset = 0;
    const InnoByteSkipper skipBytes(baBlock1);
    const InnoSerializedStringSkipper skipString(baBlock1);
    const InnoSerializedStringsSkipper skipStrings(baBlock1);

    if (nVersion == innoVersionValue(1, 2, 10)) {
        // Both native-width compilers prefix pointer-bearing records with a
        // Longint TotalSize. Seven pointers become length-prefixed strings;
        // the remaining header is 53 bytes on Win16 and 80 bytes on Win32.
        const qint32 nTailSize = version.bWin16 ? 53 : 80;
        const qint32 nCountWidth = version.bWin16 ? 2 : 4;
        if (baBlock1.size() < 4) return result;
        const quint32 nRecordSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baBlock1.constData()));
        if ((nRecordSize < quint32(7 * 4 + nTailSize)) || (nRecordSize > quint32(baBlock1.size() - 4))) return result;
        const qint32 nRecordEnd = 4 + (qint32)nRecordSize;
        nOffset = 4;
        for (qint32 i = 0; i < 7; i++) {
            if (!skipString(&nOffset) || (nOffset > nRecordEnd)) return HEADER_INFO();
        }
        if ((nOffset > nRecordEnd) || (nRecordEnd - nOffset != nTailSize)) {
            return HEADER_INFO();
        }

        const qint32 nTailOffset = nOffset;
        result.nCountCount = 10;
        for (qint32 i = 0; i < result.nCountCount; i++) {
            quint32 nCount = 0;
            if (version.bWin16) {
                const qint16 nSignedCount = qFromLittleEndian<qint16>(reinterpret_cast<const uchar *>(baBlock1.constData() + nTailOffset + i * nCountWidth));
                if (nSignedCount < 0) return HEADER_INFO();
                nCount = (quint32)nSignedCount;
            } else {
                nCount = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baBlock1.constData() + nTailOffset + i * nCountWidth));
            }
            if (nCount > 1000000U) return HEADER_INFO();
            result.anCounts[i] = (qint32)nCount;
        }
        result.nFileCount = result.anCounts[1];
        result.nDataEntryCount = result.anCounts[2];
        const bool bOptionsValid = version.bWin16
                                       ? (((quint8)baBlock1.at(nTailOffset + 52) & 0xf8U) == 0)
                                       : ((qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baBlock1.constData() + nTailOffset + 76)) & 0xff800000U) == 0);
        if ((result.nFileCount <= 0) || (result.nDataEntryCount <= 0) || !bOptionsValid) {
            return HEADER_INFO();
        }

        const qint32 nSizeOffset = result.nCountCount * nCountWidth;
        quint32 nLicenseSize = 0;
        quint32 nInfoBeforeSize = 0;
        quint32 nInfoAfterSize = 0;
        if (version.bWin16) {
            nLicenseSize = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baBlock1.constData() + nTailOffset + nSizeOffset));
            nInfoBeforeSize = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baBlock1.constData() + nTailOffset + nSizeOffset + 2));
            nInfoAfterSize = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baBlock1.constData() + nTailOffset + nSizeOffset + 4));
        } else {
            nLicenseSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baBlock1.constData() + nTailOffset + nSizeOffset));
            nInfoBeforeSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baBlock1.constData() + nTailOffset + nSizeOffset + 4));
            nInfoAfterSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baBlock1.constData() + nTailOffset + nSizeOffset + 8));
        }
        if ((nLicenseSize > quint32((std::numeric_limits<qint32>::max)())) || (nInfoBeforeSize > quint32((std::numeric_limits<qint32>::max)())) ||
            (nInfoAfterSize > quint32((std::numeric_limits<qint32>::max)()))) {
            return HEADER_INFO();
        }
        nOffset = nRecordEnd;
        if (!skipBytes((qint32)nLicenseSize, &nOffset) || !skipBytes((qint32)nInfoBeforeSize, &nOffset) || !skipBytes((qint32)nInfoAfterSize, &nOffset) ||
            (nOffset > baBlock1.size() - 4))
            return HEADER_INFO();
        const qint32 nWizardImageSize = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(baBlock1.constData() + nOffset));
        if ((nWizardImageSize < 0) || !skipBytes(4, &nOffset) || !skipBytes(nWizardImageSize, &nOffset)) return HEADER_INFO();

        result.nHeaderEndOffset = nOffset;
        result.compression = INNO_COMPRESSION_ZLIB;
        result.bIsValid = true;
        return result;
    }

    if (version.nMajor < 5) {
        // Standard 1.3-4.2 headers are field-versioned. Walk the official
        // field order instead of accepting a handful of terminal snapshots.
        qint32 nHeaderStringCount = 12;  // through BaseFilename (sans old uninstall icon)
        if (nVersion < innoVersionValue(3, 0, 0)) nHeaderStringCount++;
        nHeaderStringCount += 3;  // LicenseText, InfoBefore, InfoAfter
        if (nVersion >= innoVersionValue(1, 3, 3)) nHeaderStringCount++;
        if (nVersion >= innoVersionValue(1, 3, 6)) nHeaderStringCount += 2;
        if (nVersion >= innoVersionValue(1, 3, 14)) nHeaderStringCount++;
        if (nVersion >= innoVersionValue(3, 0, 0)) nHeaderStringCount += 2;
        if (nVersion >= innoVersionValue(4, 0, 0)) nHeaderStringCount += 2;
        if (nVersion >= innoVersionValue(4, 2, 4)) nHeaderStringCount += 4;

        qint32 nCountCount = 10;
        qint32 nCountPrefix = 0;
        if (nVersion >= innoVersionValue(4, 0, 0)) {
            nCountCount++;
            nCountPrefix++;
        }
        if (nVersion >= innoVersionValue(4, 2, 1)) {
            nCountCount++;
            nCountPrefix++;
        }
        if (nVersion >= innoVersionValue(4, 1, 0)) {
            nCountCount++;
            nCountPrefix++;
        }
        if (nVersion >= innoVersionValue(2, 0, 0)) {
            nCountCount += 3;
            nCountPrefix += 3;
        }
        const qint32 nFileCountIndex = nCountPrefix + 1;
        const qint32 nDataCountIndex = nCountPrefix + 2;

        if (!skipStrings(nHeaderStringCount, &nOffset) || ((nVersion >= innoVersionValue(2, 0, 6)) && !skipBytes(32, &nOffset))) return result;

        result.nCountCount = nCountCount;
        if (!skipBytes(nCountCount * 4, &nOffset)) return result;
        const qint32 nCountsOffset = nOffset - nCountCount * 4;
        for (qint32 i = 0; i < nCountCount; i++) {
            const quint32 nCount = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baBlock1.constData() + nCountsOffset + i * 4));
            if (nCount > 1000000U) return HEADER_INFO();
            result.anCounts[i] = (qint32)nCount;
        }
        result.nFileCount = result.anCounts[nFileCountIndex];
        result.nDataEntryCount = result.anCounts[nDataCountIndex];
        if ((result.nFileCount <= 0) || (result.nDataEntryCount <= 0)) {
            return HEADER_INFO();
        }

        const qint32 nWindowsVersionRangeSize = (nVersion >= innoVersionValue(1, 3, 19)) ? 20 : 8;
        if (!skipBytes(nWindowsVersionRangeSize, &nOffset) || !skipBytes(4, &nOffset) ||                        // BackColor
            ((nVersion >= innoVersionValue(1, 3, 3)) && !skipBytes(4, &nOffset)) || !skipBytes(4, &nOffset) ||  // WizardImageBackColor
            ((nVersion >= innoVersionValue(2, 0, 0)) && !skipBytes(4, &nOffset)))
            return HEADER_INFO();

        const qint32 nPasswordOffset = nOffset;
        if (!skipBytes(nVersion >= innoVersionValue(4, 2, 0) ? 16 : 4, &nOffset)) return HEADER_INFO();
        qint32 nPasswordSaltOffset = -1;
        if (nVersion >= innoVersionValue(4, 2, 2)) {
            nPasswordSaltOffset = nOffset;
            if (!skipBytes(8, &nOffset)) return HEADER_INFO();
        }

        if (!skipBytes(nVersion >= innoVersionValue(4, 0, 0) ? 12 : 4, &nOffset) || ((nVersion >= innoVersionValue(2, 0, 0)) && !skipBytes(1, &nOffset)) ||
            !skipBytes(1, &nOffset) || ((nVersion >= innoVersionValue(2, 0, 0)) && !skipBytes(1, &nOffset)) ||
            ((nVersion >= innoVersionValue(1, 3, 6)) && !skipBytes(1, &nOffset)) ||
            ((nVersion >= innoVersionValue(3, 0, 0)) && (nVersion < innoVersionValue(3, 0, 3)) && !skipBytes(1, &nOffset)) ||
            ((nVersion >= innoVersionValue(3, 0, 4)) && !skipBytes(1, &nOffset)) || ((nVersion >= innoVersionValue(4, 0, 10)) && !skipBytes(2, &nOffset)))
            return HEADER_INFO();

        qint32 nStoredCompressionOffset = -1;
        if (nVersion >= innoVersionValue(4, 1, 5)) {
            nStoredCompressionOffset = nOffset;
            if (!skipBytes(1, &nOffset)) return HEADER_INFO();
        }

        qint32 nFlagCount = 0;
        qint32 nBzipFlag = -1;
        qint32 nEncryptionUsedFlag = -1;
        const InnoHeaderFlagCounter addHeaderFlag(&nFlagCount);
        addHeaderFlag(true);  // DisableStartupPrompt
        addHeaderFlag(true);  // Uninstallable
        addHeaderFlag(true);  // CreateAppDir
        addHeaderFlag(true);  // DisableDirPage
        addHeaderFlag(nVersion < innoVersionValue(1, 3, 6));
        addHeaderFlag(true);  // DisableProgramGroupPage
        addHeaderFlag(true);  // AllowNoIcons
        addHeaderFlag((nVersion < innoVersionValue(3, 0, 0)) || (nVersion >= innoVersionValue(3, 0, 3)));
        addHeaderFlag(nVersion < innoVersionValue(1, 3, 3));
        addHeaderFlag(true);  // AlwaysUsePersonalGroup
        addHeaderFlag(true);
        addHeaderFlag(true);
        addHeaderFlag(true);
        addHeaderFlag(true);
        addHeaderFlag(true);  // EnableDirDoesntExistWarning
        addHeaderFlag(nVersion < innoVersionValue(4, 1, 2));
        addHeaderFlag(true);  // Password
        addHeaderFlag(true);  // AllowRootDirectory
        addHeaderFlag(true);  // DisableFinishedPage
        addHeaderFlag(nVersion < innoVersionValue(3, 0, 4));
        addHeaderFlag(nVersion < innoVersionValue(3, 0, 0));
        addHeaderFlag(nVersion < innoVersionValue(1, 3, 6));
        addHeaderFlag(true);  // ChangesAssociations
        addHeaderFlag(true);  // CreateUninstallRegKey
        addHeaderFlag(true);  // UsePreviousAppDir
        addHeaderFlag(nVersion >= innoVersionValue(1, 3, 3));
        addHeaderFlag(nVersion >= innoVersionValue(1, 3, 10));
        addHeaderFlag(nVersion >= innoVersionValue(1, 3, 20));
        addHeaderFlag(nVersion >= innoVersionValue(2, 0, 0));
        for (qint32 i = 0; i < 6; i++) addHeaderFlag(nVersion >= innoVersionValue(2, 0, 0));
        addHeaderFlag(nVersion >= innoVersionValue(2, 0, 7));
        addHeaderFlag(nVersion >= innoVersionValue(2, 0, 7));
        if ((nVersion >= innoVersionValue(2, 0, 17)) && (nVersion < innoVersionValue(4, 1, 5))) {
            nBzipFlag = nFlagCount;
            addHeaderFlag(true);
        }
        addHeaderFlag(nVersion >= innoVersionValue(2, 0, 18));
        addHeaderFlag(nVersion >= innoVersionValue(3, 0, 0));
        addHeaderFlag(nVersion >= innoVersionValue(3, 0, 0));
        addHeaderFlag(nVersion >= innoVersionValue(3, 0, 1));
        addHeaderFlag(nVersion >= innoVersionValue(3, 0, 3));
        addHeaderFlag(nVersion >= innoVersionValue(4, 0, 0));
        addHeaderFlag((nVersion >= innoVersionValue(4, 0, 0)) && (nVersion < innoVersionValue(4, 0, 10)));
        addHeaderFlag((nVersion >= innoVersionValue(4, 0, 1)) && (nVersion < innoVersionValue(4, 0, 10)));
        addHeaderFlag(nVersion >= innoVersionValue(4, 0, 9));
        addHeaderFlag(nVersion >= innoVersionValue(4, 1, 3));
        addHeaderFlag(nVersion >= innoVersionValue(4, 1, 8));
        addHeaderFlag(nVersion >= innoVersionValue(4, 1, 8));
        if (nVersion >= innoVersionValue(4, 2, 2)) {
            nEncryptionUsedFlag = nFlagCount;
            addHeaderFlag(true);
        }

        qint32 nOptionBytes = (nFlagCount + 7) / 8;
        if (nOptionBytes == 3) nOptionBytes = 4;
        const qint32 nOptionsOffset = nOffset;
        if (!skipBytes(nOptionBytes, &nOffset)) return HEADER_INFO();

        if (nStoredCompressionOffset >= 0) {
            const quint8 nStoredCompression = (quint8)baBlock1.at(nStoredCompressionOffset);
            if (nVersion < innoVersionValue(4, 2, 5)) {
                static const INNO_COMPRESSION aCompression[] = {
                    INNO_COMPRESSION_ZLIB,
                    INNO_COMPRESSION_BZIP2,
                    INNO_COMPRESSION_LZMA1,
                };
                if (nStoredCompression >= sizeof(aCompression) / sizeof(aCompression[0])) return HEADER_INFO();
                result.compression = aCompression[nStoredCompression];
            } else if (nVersion < innoVersionValue(4, 2, 6)) {
                static const INNO_COMPRESSION aCompression[] = {
                    INNO_COMPRESSION_STORE,
                    INNO_COMPRESSION_BZIP2,
                    INNO_COMPRESSION_LZMA1,
                };
                if (nStoredCompression >= sizeof(aCompression) / sizeof(aCompression[0])) return HEADER_INFO();
                result.compression = aCompression[nStoredCompression];
            } else {
                if (nStoredCompression > (quint8)INNO_COMPRESSION_LZMA1) return HEADER_INFO();
                result.compression = (INNO_COMPRESSION)nStoredCompression;
            }
        } else {
            const bool bBzip = (nBzipFlag >= 0) && (((quint8)baBlock1.at(nOptionsOffset + nBzipFlag / 8) & (1U << (nBzipFlag & 7))) != 0);
            result.compression = bBzip ? INNO_COMPRESSION_BZIP2 : INNO_COMPRESSION_ZLIB;
        }

        result.bEncryptionUsed = (nEncryptionUsedFlag >= 0) && (((quint8)baBlock1.at(nOptionsOffset + nEncryptionUsedFlag / 8) & (1U << (nEncryptionUsedFlag & 7))) != 0);

        if (nVersion >= innoVersionValue(4, 2, 2)) {
            result.encryption = INNO_ENCRYPTION_ARC4_MD5;
            result.baPasswordTest = baBlock1.mid(nPasswordOffset, 16);
            result.baPasswordSalt = baBlock1.mid(nPasswordSaltOffset, 8);
            if ((result.baPasswordTest.size() != 16) || (result.baPasswordSalt.size() != 8)) return HEADER_INFO();
        }

        result.nHeaderEndOffset = nOffset;
        result.bIsValid = true;
        return result;
    }

    // TSetupHeader string fields, in their serialized order.
    if (!skipStrings(6, &nOffset)) return result;
    if ((nVersion >= innoVersionValue(5, 1, 13)) && !skipString(&nOffset)) return result;
    if (!skipStrings(6, &nOffset)) return result;  // support URL through base filename
    if ((nVersion < innoVersionValue(5, 2, 5)) && !skipStrings(3, &nOffset)) return result;
    if (!skipStrings(7, &nOffset)) return result;  // uninstall files through serial
    if ((nVersion < innoVersionValue(5, 2, 5)) && !skipString(&nOffset)) return result;
    if (!skipStrings(4, &nOffset)) return result;
    if ((nVersion >= innoVersionValue(5, 3, 8)) && !skipString(&nOffset)) return result;
    if ((nVersion >= innoVersionValue(5, 3, 10)) && !skipString(&nOffset)) return result;
    if ((nVersion >= innoVersionValue(5, 5, 0)) && !skipString(&nOffset)) return result;
    if ((nVersion >= innoVersionValue(5, 5, 6)) && !skipString(&nOffset)) return result;
    if ((nVersion >= innoVersionValue(5, 6, 1)) && !skipStrings(2, &nOffset)) return result;
    if ((nVersion >= innoVersionValue(6, 3, 0)) && !skipStrings(2, &nOffset)) return result;
    if ((nVersion >= innoVersionValue(6, 4, 0)) && !skipString(&nOffset)) return result;  // close-app excludes
    if ((nVersion >= innoVersionValue(6, 7, 0)) && !skipStrings(6, &nOffset)) return result;
    if ((nVersion >= innoVersionValue(6, 5, 0)) && (nVersion < innoVersionValue(6, 7, 0)) && !skipString(&nOffset)) return result;  // SevenZipLibraryName
    if ((nVersion >= innoVersionValue(5, 2, 5)) && !skipStrings(3, &nOffset)) return result;
    if ((nVersion >= innoVersionValue(5, 2, 1)) && (nVersion < innoVersionValue(5, 3, 10)) && !skipString(&nOffset)) return result;
    if ((nVersion >= innoVersionValue(5, 2, 5)) && !skipString(&nOffset)) return result;

    if (!version.bUnicode && !skipBytes(32, &nOffset)) return result;  // stored ANSI lead-byte set

    result.nCountCount = (nVersion >= innoVersionValue(6, 5, 0)) ? 17 : 16;
    if (!skipBytes(result.nCountCount * 4, &nOffset)) return result;

    const qint32 nCountsOffset = nOffset - result.nCountCount * 4;
    for (qint32 i = 0; i < result.nCountCount; i++) {
        const quint32 nCount = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baBlock1.constData() + nCountsOffset + i * 4));
        if (nCount > 1000000U) return HEADER_INFO();
        result.anCounts[i] = (qint32)nCount;
    }

    if (result.nCountCount == 17) {
        result.nFileCount = result.anCounts[8];
        result.nDataEntryCount = result.anCounts[9];
    } else {
        result.nFileCount = result.anCounts[7];
        result.nDataEntryCount = result.anCounts[8];
    }

    // FileCount and FileLocationCount are independent.  An all-external setup can
    // legally contain file records while carrying no embedded data locations.
    if (result.nFileCount <= 0) return HEADER_INFO();

    if (version.nMajor >= 7) {
        if (!skipBytes(4, &nOffset)) return HEADER_INFO();  // CompiledCodeVersion
    }

    if (nVersion >= innoVersionValue(6, 7, 0)) {
        // Current packed header: version range, wizard geometry/appearance, disk fields,
        // and six one-byte enums immediately precede CompressMethod.
        if (!skipBytes(75, &nOffset)) return HEADER_INFO();
    } else if (nVersion >= innoVersionValue(6, 6, 1)) {
        // 6.6.1 added WizardImageOpacity to the 6.6.0 header layout.
        if (!skipBytes(65, &nOffset)) return HEADER_INFO();
    } else if (nVersion >= innoVersionValue(6, 6, 0)) {
        // 6.6.0: version range, wizard dimensions/dark style/alpha mode, four
        // background colors, disk fields, and six one-byte enums.
        if (!skipBytes(64, &nOffset)) return HEADER_INFO();
    } else if (nVersion >= innoVersionValue(6, 5, 2)) {
        // 6.5.2 added wizard image background colors alongside the widened
        // file-location and disk-slice offsets.
        if (!skipBytes(56, &nOffset)) return HEADER_INFO();
    } else if (nVersion >= innoVersionValue(6, 5, 0)) {
        // 6.5: version range, classic/modern style, wizard dimensions/alpha
        // mode, disk fields, and six one-byte enums.
        if (!skipBytes(48, &nOffset)) return HEADER_INFO();
    } else {
        if (!skipBytes(20, &nOffset)) return HEADER_INFO();  // MinVersion + OnlyBelowVersion
        if ((nVersion < innoVersionValue(6, 4, 0, 1)) && !skipBytes(4, &nOffset)) return HEADER_INFO();
        if ((nVersion < innoVersionValue(6, 4, 0, 1)) && !skipBytes(4, &nOffset)) return HEADER_INFO();
        if ((nVersion < innoVersionValue(5, 5, 7)) && !skipBytes(4, &nOffset)) return HEADER_INFO();
        if ((nVersion < innoVersionValue(5, 0, 4)) && !skipBytes(4, &nOffset)) return HEADER_INFO();
        if ((nVersion >= innoVersionValue(6, 0, 0)) && !skipBytes(9, &nOffset)) return HEADER_INFO();
        if ((nVersion >= innoVersionValue(5, 5, 7)) && !skipBytes(1, &nOffset)) return HEADER_INFO();

        if (nVersion >= innoVersionValue(6, 4, 0)) {
            if ((nOffset < 0) || (nOffset > baBlock1.size() - 48)) return HEADER_INFO();
            result.encryption = INNO_ENCRYPTION_XCHACHA20;
            result.baPasswordTest = baBlock1.mid(nOffset, 4);
            result.baPasswordSalt = baBlock1.mid(nOffset + 4, 16);
            result.nKdfIterations = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(baBlock1.constData() + nOffset + 20));
            result.baEncryptionBaseNonce = baBlock1.mid(nOffset + 24, 24);
            if (!skipBytes(48, &nOffset)) return HEADER_INFO();
        } else if (nVersion >= innoVersionValue(5, 3, 9)) {
            if ((nOffset < 0) || (nOffset > baBlock1.size() - 28)) return HEADER_INFO();
            result.encryption = INNO_ENCRYPTION_ARC4_SHA1;
            result.baPasswordTest = baBlock1.mid(nOffset, 20);
            result.baPasswordSalt = baBlock1.mid(nOffset + 20, 8);
            if (!skipBytes(28, &nOffset)) return HEADER_INFO();
        } else {
            if ((nOffset < 0) || (nOffset > baBlock1.size() - 24)) return HEADER_INFO();
            result.encryption = INNO_ENCRYPTION_ARC4_MD5;
            result.baPasswordTest = baBlock1.mid(nOffset, 16);
            result.baPasswordSalt = baBlock1.mid(nOffset + 16, 8);
            if (!skipBytes(24, &nOffset)) return HEADER_INFO();
        }
        if (!skipBytes(12, &nOffset)) return HEADER_INFO();
        if (!skipBytes(3, &nOffset)) return HEADER_INFO();  // log, dir warning, privileges
        if ((nVersion >= innoVersionValue(5, 7, 0)) && !skipBytes(1, &nOffset)) return HEADER_INFO();
        if (!skipBytes(2, &nOffset)) return HEADER_INFO();  // language dialog/detection
    }

    if ((nOffset < 0) || (nOffset >= baBlock1.size())) return HEADER_INFO();
    const quint8 nCompression = (quint8)baBlock1.at(nOffset++);
    const quint8 nMaxCompression = (nVersion >= innoVersionValue(5, 3, 9)) ? 4 : 3;
    if (nCompression > nMaxCompression) return HEADER_INFO();
    result.compression = (INNO_COMPRESSION)nCompression;

    if (nVersion >= innoVersionValue(6, 7, 0)) {
        if (!skipBytes(18, &nOffset)) return HEADER_INFO();  // page enums, display size, option flags
    } else if (nVersion >= innoVersionValue(6, 5, 0)) {
        if (!skipBytes(16, &nOffset)) return HEADER_INFO();  // page enums, display size, option flags
    } else {
        if ((nVersion >= innoVersionValue(5, 1, 0)) && (nVersion < innoVersionValue(6, 3, 0)) && !skipBytes(2, &nOffset)) return HEADER_INFO();
        if ((nVersion >= innoVersionValue(5, 2, 1)) && (nVersion < innoVersionValue(5, 3, 10)) && !skipBytes(8, &nOffset)) return HEADER_INFO();
        if ((nVersion >= innoVersionValue(5, 3, 3)) && !skipBytes(2, &nOffset)) return HEADER_INFO();
        if (nVersion >= innoVersionValue(5, 5, 0)) {
            if (!skipBytes(8, &nOffset)) return HEADER_INFO();
        } else if ((nVersion >= innoVersionValue(5, 3, 6)) && !skipBytes(4, &nOffset)) {
            return HEADER_INFO();
        }
        // The 6.3.0 setup-data schema adds shUninstallLogging as the 49th
        // active Unicode option and therefore occupies seven packed bytes.
        // Earlier Inno 6 schemas have 48 active options (the apparent extra
        // symbol is IFNDEF UNICODE); 6.4 removes obsolete options again.
        const qint32 nOptionBytes = ((nVersion >= innoVersionValue(6, 3, 0)) && (nVersion < innoVersionValue(6, 4, 0))) ? 7 : 6;
        if (!skipBytes(nOptionBytes, &nOffset)) return HEADER_INFO();
    }

    result.nHeaderEndOffset = nOffset;
    result.bIsValid = true;
    return result;
}

QList<XInnoSetup::DATA_ENTRY> XInnoSetup::_parseDataEntries(const QByteArray &baBlock2, const INNO_VERSION &version, bool bRev2, qint32 nExpectedCount,
                                                            INNO_COMPRESSION headerCompression)
{
    QList<DATA_ENTRY> listResult;

    if (!version.bIsValid || (nExpectedCount <= 0) || (headerCompression == INNO_COMPRESSION_UNKNOWN)) return listResult;

    const quint64 nVersion = innoVersionValue(version);

    if (version.nMajor < 5) {
        if ((nVersion == innoVersionValue(1, 2, 10)) && version.bWin16) {
            // Win16 TSetupFileLocationEntry: two 16-bit disk indexes followed
            // by seven Longints and a one-byte, two-value flag set.
            static const qint32 nEntrySize = 33;
            if (bRev2 || (headerCompression != INNO_COMPRESSION_ZLIB) || (nExpectedCount > (std::numeric_limits<qint32>::max() / nEntrySize)) ||
                (baBlock2.size() != nExpectedCount * nEntrySize))
                return listResult;

            for (qint32 i = 0; i < nExpectedCount; i++) {
                const qint32 nBase = i * nEntrySize;
                const char *pEntry = baBlock2.constData() + nBase;
                const qint16 nFirstDisk = qFromLittleEndian<qint16>(reinterpret_cast<const uchar *>(pEntry));
                const qint16 nLastDisk = qFromLittleEndian<qint16>(reinterpret_cast<const uchar *>(pEntry + 2));
                DATA_ENTRY entry = {};
                entry.nChunkStartOffset = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pEntry + 4));
                entry.nOriginalSize = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pEntry + 8));
                entry.nChunkCompressedSize = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pEntry + 12));
                entry.baChecksum = baBlock2.mid(nBase + 16, 4);
                entry.checksumType = INNO_CHECKSUM_ADLER32;
                entry.nFileTime = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pEntry + 20));
                entry.nFileVersionMS = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pEntry + 24));
                entry.nFileVersionLS = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pEntry + 28));
                entry.nFlags = (quint8)pEntry[32];

                if ((nFirstDisk <= 0) || (nLastDisk < nFirstDisk) || (entry.nChunkStartOffset < 12) || (entry.nOriginalSize < 0) || (entry.nChunkCompressedSize < 0) ||
                    (entry.baChecksum.size() != 4) || ((entry.nFlags & ~quint16(0x03U)) != 0)) {
                    return QList<DATA_ENTRY>();
                }
                entry.nFirstSlice = (quint32)(nFirstDisk - 1);
                entry.nLastSlice = (quint32)(nLastDisk - 1);
                entry.nChunkSubOffset = 0;
                entry.bCallInstructionOptimized = false;
                entry.bChunkEncrypted = false;
                entry.bChunkCompressed = true;
                entry.compression = INNO_COMPRESSION_ZLIB;
                listResult.append(entry);
            }
            return listResult;
        }

        qint32 nEntrySize = 0;
        qint32 nDigestSize = 4;
        qint32 nTimeOffset = 0;
        qint32 nFlagsOffset = 0;
        INNO_CHECKSUM checksumType = INNO_CHECKSUM_UNKNOWN;
        const bool bPreSlices = nVersion < innoVersionValue(4, 0, 0);

        if (nVersion < innoVersionValue(4, 0, 0)) {
            nEntrySize = 41;
            nTimeOffset = 24;
            nFlagsOffset = 40;
            checksumType = INNO_CHECKSUM_ADLER32;
        } else if (nVersion == innoVersionValue(4, 0, 0)) {
            nEntrySize = 49;
            nTimeOffset = 32;
            nFlagsOffset = 48;
            checksumType = INNO_CHECKSUM_ADLER32;
        } else if (nVersion < innoVersionValue(4, 2, 0)) {
            nEntrySize = 57;
            nTimeOffset = 40;
            nFlagsOffset = 56;
            checksumType = INNO_CHECKSUM_CRC32;
        } else {
            nEntrySize = 69;
            nDigestSize = 16;
            nTimeOffset = 52;
            nFlagsOffset = 68;
            checksumType = INNO_CHECKSUM_MD5;
        }

        if ((nExpectedCount > (std::numeric_limits<qint32>::max() / nEntrySize)) || (baBlock2.size() != nExpectedCount * nEntrySize)) return listResult;

        for (qint32 i = 0; i < nExpectedCount; i++) {
            const qint32 nBase = i * nEntrySize;
            const char *pEntry = baBlock2.constData() + nBase;
            const qint32 nFirst = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pEntry));
            const qint32 nLast = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pEntry + 4));
            DATA_ENTRY entry = {};

            if (bPreSlices) {
                if ((nFirst <= 0) || (nLast < nFirst)) return QList<DATA_ENTRY>();
                entry.nFirstSlice = (quint32)(nFirst - 1);
                entry.nLastSlice = (quint32)(nLast - 1);
                entry.nChunkStartOffset = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pEntry + 8));
                entry.nChunkSubOffset = 0;
                entry.nOriginalSize = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pEntry + 12));
                entry.nChunkCompressedSize = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pEntry + 16));
            } else if (nVersion == innoVersionValue(4, 0, 0)) {
                if ((nFirst < 0) || (nLast < nFirst)) return QList<DATA_ENTRY>();
                entry.nFirstSlice = (quint32)nFirst;
                entry.nLastSlice = (quint32)nLast;
                entry.nChunkStartOffset = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pEntry + 8));
                entry.nChunkSubOffset = 0;
                entry.nOriginalSize = qFromLittleEndian<qint64>(reinterpret_cast<const uchar *>(pEntry + 12));
                entry.nChunkCompressedSize = qFromLittleEndian<qint64>(reinterpret_cast<const uchar *>(pEntry + 20));
            } else {
                if ((nFirst < 0) || (nLast < nFirst)) return QList<DATA_ENTRY>();
                entry.nFirstSlice = (quint32)nFirst;
                entry.nLastSlice = (quint32)nLast;
                entry.nChunkStartOffset = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pEntry + 8));
                entry.nChunkSubOffset = qFromLittleEndian<qint64>(reinterpret_cast<const uchar *>(pEntry + 12));
                entry.nOriginalSize = qFromLittleEndian<qint64>(reinterpret_cast<const uchar *>(pEntry + 20));
                entry.nChunkCompressedSize = qFromLittleEndian<qint64>(reinterpret_cast<const uchar *>(pEntry + 28));
            }

            const qint32 nDigestOffset = bPreSlices ? 20 : ((nVersion == innoVersionValue(4, 0, 0)) ? 28 : 36);
            entry.baChecksum = baBlock2.mid(nBase + nDigestOffset, nDigestSize);
            entry.checksumType = checksumType;
            entry.nFileTime = qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(pEntry + nTimeOffset));
            entry.nFileVersionMS = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pEntry + nTimeOffset + 8));
            entry.nFileVersionLS = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pEntry + nTimeOffset + 12));
            entry.nFlags = (quint8)pEntry[nFlagsOffset];

            if ((entry.nLastSlice > quint32((std::numeric_limits<qint32>::max)())) || (entry.nChunkStartOffset < 0) || (entry.nChunkSubOffset < 0) ||
                (entry.nOriginalSize < 0) || (entry.nChunkCompressedSize < 0) || (entry.nChunkSubOffset > (std::numeric_limits<qint64>::max)() - entry.nOriginalSize) ||
                (entry.baChecksum.size() != nDigestSize))
                return QList<DATA_ENTRY>();

            entry.bCallInstructionOptimized = (nVersion >= innoVersionValue(4, 1, 8)) && ((entry.nFlags & (1U << 4)) != 0);
            entry.bChunkEncrypted = (nVersion >= innoVersionValue(4, 2, 2)) && ((entry.nFlags & (1U << 6)) != 0);
            entry.bChunkCompressed = (nVersion < innoVersionValue(4, 2, 5)) || ((entry.nFlags & (1U << 7)) != 0);

            if ((nVersion >= innoVersionValue(2, 0, 17)) && (nVersion < innoVersionValue(4, 0, 1))) {
                entry.compression = (entry.nFlags & (1U << 2)) ? INNO_COMPRESSION_BZIP2 : INNO_COMPRESSION_ZLIB;
            } else {
                entry.compression = entry.bChunkCompressed ? headerCompression : INNO_COMPRESSION_STORE;
            }
            if (entry.compression == INNO_COMPRESSION_UNKNOWN) {
                return QList<DATA_ENTRY>();
            }
            listResult.append(entry);
        }
        return listResult;
    }

    qint32 nEntrySize = 0;
    qint32 nDigestOffset = 36;
    qint32 nDigestSize = 0;
    qint32 nTimeOffset = 0;
    qint32 nFlagsOffset = 0;
    qint32 nFlagsSize = 0;
    INNO_CHECKSUM checksumType = INNO_CHECKSUM_UNKNOWN;

    qint32 nShift = 0;
    bool b64BitStartOffset = false;

    if (bRev2) {
        if (nVersion < innoVersionValue(6, 5, 0)) return listResult;

        // The loader table and compressed-block sizes widened in 6.5.0, but
        // TSetupFileLocationEntry.StartOffset stayed 32-bit in the 6.5.0
        // setup-data schema. Inno 6.5.2 introduced a new setup-data ID and
        // widened that field to Int64 (the layout retained by 6.6+).
        b64BitStartOffset = nVersion >= innoVersionValue(6, 5, 2);
        nShift = b64BitStartOffset ? 4 : 0;
        nEntrySize = b64BitStartOffset ? 89 : 85;
        nDigestOffset = 36 + nShift;
        nDigestSize = 32;
        nTimeOffset = 68 + nShift;
        nFlagsOffset = 84 + nShift;
        nFlagsSize = 1;
        checksumType = INNO_CHECKSUM_SHA256;
    } else if (nVersion >= innoVersionValue(6, 4, 3)) {
        // Shared.Struct.pas 6.4.3: the SHA-256 entry ends with a five-value
        // packed flag set (one byte), for an exact record size of 85 bytes.
        nEntrySize = 85;
        nDigestSize = 32;
        nTimeOffset = 68;
        nFlagsOffset = 84;
        nFlagsSize = 1;
        checksumType = INNO_CHECKSUM_SHA256;
    } else if (nVersion >= innoVersionValue(6, 4, 0)) {
        // 6.4.0.1 and 6.4.2 retain the two-byte legacy flag set plus Sign.
        nEntrySize = 87;
        nDigestSize = 32;
        nTimeOffset = 68;
        nFlagsOffset = 84;
        nFlagsSize = 2;
        checksumType = INNO_CHECKSUM_SHA256;
    } else if (nVersion >= innoVersionValue(6, 3, 0)) {
        nEntrySize = 75;
        nDigestSize = 20;
        nTimeOffset = 56;
        nFlagsOffset = 72;
        nFlagsSize = 2;
        checksumType = INNO_CHECKSUM_SHA1;
    } else if (nVersion >= innoVersionValue(5, 3, 9)) {
        nEntrySize = 74;
        nDigestSize = 20;
        nTimeOffset = 56;
        nFlagsOffset = 72;
        nFlagsSize = 2;
        checksumType = INNO_CHECKSUM_SHA1;
    } else if (nVersion >= innoVersionValue(5, 1, 13)) {
        nEntrySize = 70;
        nDigestSize = 16;
        nTimeOffset = 52;
        nFlagsOffset = 68;
        nFlagsSize = 2;
        checksumType = INNO_CHECKSUM_MD5;
    } else if (nVersion >= innoVersionValue(5, 0, 0)) {
        nEntrySize = 69;
        nDigestSize = 16;
        nTimeOffset = 52;
        nFlagsOffset = 68;
        nFlagsSize = 1;
        checksumType = INNO_CHECKSUM_MD5;
    } else {
        return listResult;
    }

    if ((nExpectedCount > (std::numeric_limits<qint32>::max() / nEntrySize)) || (baBlock2.size() != nExpectedCount * nEntrySize)) return listResult;

    for (qint32 i = 0; i < nExpectedCount; i++) {
        const qint32 nBase = i * nEntrySize;
        const char *pEntry = baBlock2.constData() + nBase;
        DATA_ENTRY entry = {};

        entry.nFirstSlice = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pEntry));
        entry.nLastSlice = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pEntry + 4));
        entry.nChunkStartOffset = b64BitStartOffset ? qFromLittleEndian<qint64>(reinterpret_cast<const uchar *>(pEntry + 8))
                                                    : qint64(qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pEntry + 8)));
        entry.nChunkSubOffset = qFromLittleEndian<qint64>(reinterpret_cast<const uchar *>(pEntry + 12 + nShift));
        entry.nOriginalSize = qFromLittleEndian<qint64>(reinterpret_cast<const uchar *>(pEntry + 20 + nShift));
        entry.nChunkCompressedSize = qFromLittleEndian<qint64>(reinterpret_cast<const uchar *>(pEntry + 28 + nShift));
        entry.baChecksum = baBlock2.mid(nBase + nDigestOffset, nDigestSize);
        entry.checksumType = checksumType;
        entry.nFileTime = qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(pEntry + nTimeOffset));
        entry.nFileVersionMS = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pEntry + nTimeOffset + 8));
        entry.nFileVersionLS = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pEntry + nTimeOffset + 12));
        entry.nFlags = (quint8)pEntry[nFlagsOffset];
        if (nFlagsSize == 2) entry.nFlags |= quint16((quint8)pEntry[nFlagsOffset + 1]) << 8;

        if ((entry.nFirstSlice > entry.nLastSlice) || (entry.nLastSlice > quint32((std::numeric_limits<qint32>::max)())) || (entry.nChunkStartOffset < 0) ||
            (entry.nChunkSubOffset < 0) || (entry.nOriginalSize < 0) || (entry.nChunkCompressedSize < 0) ||
            (entry.nChunkSubOffset > (std::numeric_limits<qint64>::max)() - entry.nOriginalSize) || (entry.baChecksum.size() != nDigestSize))
            return QList<DATA_ENTRY>();

        if (bRev2 || (nVersion >= innoVersionValue(6, 4, 3))) {
            // 6.4.3 replaced the legacy nine-value location flag set with
            // floVersionInfoValid/TimeStampInUTC/CallOptimized/Encrypted/
            // Compressed. Revision 2 retains that five-bit ordering.
            entry.bCallInstructionOptimized = (entry.nFlags & (1U << 2)) != 0;
            entry.bChunkEncrypted = (entry.nFlags & (1U << 3)) != 0;
            entry.bChunkCompressed = (entry.nFlags & (1U << 4)) != 0;
        } else {
            entry.bCallInstructionOptimized = (entry.nFlags & (1U << 4)) != 0;
            entry.bChunkEncrypted = (entry.nFlags & (1U << 6)) != 0;
            entry.bChunkCompressed = (entry.nFlags & (1U << 7)) != 0;
        }

        entry.compression = entry.bChunkCompressed ? headerCompression : INNO_COMPRESSION_STORE;
        if (entry.compression == INNO_COMPRESSION_UNKNOWN) return QList<DATA_ENTRY>();

        listResult.append(entry);
    }

    return listResult;
}

QList<XInnoSetup::FILE_ENTRY> XInnoSetup::_parseFileEntries(const QByteArray &baBlock1, const HEADER_INFO &headerInfo, const INNO_VERSION &version, bool bRev2,
                                                            quint32 nAnsiCodePageOverride, quint32 *pnAnsiCodePage)
{
    QList<FILE_ENTRY> listResult;
    if (pnAnsiCodePage) *pnAnsiCodePage = 0;

    if (!headerInfo.bIsValid || !version.bIsValid || (headerInfo.nHeaderEndOffset < 0)) return listResult;
    if (!version.bUnicode) {
        return _parseFileEntriesAnsi(baBlock1, headerInfo, version, nAnsiCodePageOverride, pnAnsiCodePage);
    }

    const quint64 nVersion = innoVersionValue(version);
    const bool bLegacySchema = !bRev2 && ((version.nMajor == 5) || ((version.nMajor == 6) && (version.nMinor <= 4)));
    const bool bModernSchema = bRev2 && (((version.nMajor == 6) && (version.nMinor >= 5)) || (version.nMajor == 7));

    // Each accepted range below has one exact serialized layout. Any other
    // unmodelled loader/schema combination fails closed.
    if ((!bLegacySchema && !bModernSchema) || (bLegacySchema && (headerInfo.nCountCount != 16)) || (bModernSchema && (headerInfo.nCountCount != 17))) return listResult;

    qint32 nOffset = headerInfo.nHeaderEndOffset;
    const InnoByteSkipper skipBytes(baBlock1);
    const InnoWideStringReader readWide(baBlock1);
    const InnoAnsiStringSkipper readAnsi(baBlock1);
    const InnoUnicodeEntriesSkipper skipEntryArray(baBlock1);

    qint32 nFileWideStrings = 0;
    qint32 nFileAnsiStrings = 0;
    qint32 nFileFixedSize = 0;
    qint32 nLocationOffset = 0;
    qint32 nFileTypeOffset = 0;
    qint32 nVerificationTypeOffset = -1;
    qint32 nBitnessOffset = -1;

    if (bLegacySchema) {
        // Serialized 5.x-6.4 arrays from Shared.Struct/SECompressedBlockWrite.
        // Released Unicode setup-data uses 21 fixed bytes. The narrow 5.2.5
        // Unicode beta retained the legacy LanguageCodePage dword (25 bytes).
        if (nVersion < innoVersionValue(5, 2, 5)) return QList<FILE_ENTRY>();
        const qint32 nLanguageTail = (nVersion < innoVersionValue(5, 3, 0)) ? 25 : 21;
        if (!skipEntryArray(headerInfo.anCounts[0], 6, 4, nLanguageTail, &nOffset) || !skipEntryArray(headerInfo.anCounts[1], 2, 0, 4, &nOffset) ||
            !skipEntryArray(headerInfo.anCounts[2], 0, 1, 0, &nOffset) || !skipEntryArray(headerInfo.anCounts[3], 4, 0, 30, &nOffset) ||
            !skipEntryArray(headerInfo.anCounts[4], 5, 0, 42, &nOffset) || !skipEntryArray(headerInfo.anCounts[5], 6, 0, 26, &nOffset) ||
            !skipEntryArray(headerInfo.anCounts[6], 7, 0, 27, &nOffset))
            return QList<FILE_ENTRY>();

        nFileWideStrings = (nVersion >= innoVersionValue(5, 2, 5)) ? 10 : 9;
        nFileFixedSize = 43;
        nLocationOffset = 20;
        nFileTypeOffset = 42;
    } else {
        // 6.5+ adds the ISSig-key array and verification/download fields.
        // Language appearance and component/task level fields changed again
        // in 6.6 and 6.7 respectively.
        qint32 nLanguageWideStrings = 6;
        qint32 nLanguageFixedSize = 21;
        qint32 nComponentFixedSize = 42;
        qint32 nTaskFixedSize = 26;

        if (nVersion >= innoVersionValue(6, 6, 0)) {
            nLanguageWideStrings = 4;
            nLanguageFixedSize = 19;
        }
        if (nVersion >= innoVersionValue(6, 7, 0)) {
            nComponentFixedSize = 39;
            nTaskFixedSize = 23;
        }

        if (!skipEntryArray(headerInfo.anCounts[0], nLanguageWideStrings, 4, nLanguageFixedSize, &nOffset) ||
            !skipEntryArray(headerInfo.anCounts[1], 2, 0, 4, &nOffset) || !skipEntryArray(headerInfo.anCounts[2], 0, 1, 0, &nOffset) ||
            !skipEntryArray(headerInfo.anCounts[3], 4, 0, 30, &nOffset) || !skipEntryArray(headerInfo.anCounts[4], 5, 0, nComponentFixedSize, &nOffset) ||
            !skipEntryArray(headerInfo.anCounts[5], 6, 0, nTaskFixedSize, &nOffset) || !skipEntryArray(headerInfo.anCounts[6], 7, 0, 27, &nOffset) ||
            !skipEntryArray(headerInfo.anCounts[7], 3, 0, 0, &nOffset))
            return QList<FILE_ENTRY>();

        nFileWideStrings = 15;
        nFileAnsiStrings = 1;
        nFileFixedSize = (nVersion >= innoVersionValue(6, 7, 0)) ? ((version.nMajor >= 7) ? 81 : 80) : 77;
        nVerificationTypeOffset = 32;
        nLocationOffset = 53;
        nBitnessOffset = (version.nMajor >= 7) ? 71 : -1;
        nFileTypeOffset = nFileFixedSize - 1;
    }

    for (qint32 i = 0; i < headerInfo.nFileCount; i++) {
        QString sDestination;
        for (qint32 j = 0; j < nFileWideStrings; j++) {
            QString sValue;
            if (!readWide(&nOffset, &sValue)) return QList<FILE_ENTRY>();
            if (j == 1) sDestination = sValue;
        }
        for (qint32 j = 0; j < nFileAnsiStrings; j++) {
            if (!readAnsi(&nOffset)) return QList<FILE_ENTRY>();
        }

        const qint32 nFixedOffset = nOffset;
        if (!skipBytes(nFileFixedSize, &nOffset)) return QList<FILE_ENTRY>();

        const quint32 nLocation = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baBlock1.constData() + nFixedOffset + nLocationOffset));
        const quint8 nFileType = (quint8)baBlock1.at(nFixedOffset + nFileTypeOffset);
        if ((nFileType > 1) || ((nVerificationTypeOffset >= 0) && ((quint8)baBlock1.at(nFixedOffset + nVerificationTypeOffset) > 2)) ||
            ((nBitnessOffset >= 0) && ((quint8)baBlock1.at(nFixedOffset + nBitnessOffset) > 4)) ||
            ((nLocation != 0xffffffffU) && (nLocation >= (quint32)headerInfo.nDataEntryCount))) {
            return QList<FILE_ENTRY>();
        }

        for (QChar ch : sDestination) {
            if ((ch.unicode() == 0) || (ch.unicode() < 0x20)) return QList<FILE_ENTRY>();
        }

        // LocationEntry=-1 denotes no data. Shared/reordered locations are valid:
        // traverse nFileCount records and preserve each serialized index.
        if (!sDestination.isEmpty() && (nLocation != 0xffffffffU)) {
            FILE_ENTRY fileEntry = {};
            fileEntry.sDestName = sDestination;
            fileEntry.nLocationEntry = (qint32)nLocation;
            listResult.append(fileEntry);
        }
    }

    return listResult;
}

QList<XInnoSetup::FILE_ENTRY> XInnoSetup::_parseFileEntriesAnsi(const QByteArray &baBlock1, const HEADER_INFO &headerInfo, const INNO_VERSION &version,
                                                                quint32 nAnsiCodePageOverride, quint32 *pnAnsiCodePage)
{
    QList<FILE_ENTRY> listResult;
    if (pnAnsiCodePage) *pnAnsiCodePage = 0;

    if (!headerInfo.bIsValid || !version.bIsValid || version.bUnicode || (version.nMajor > 5) || (headerInfo.nHeaderEndOffset < 0)) return listResult;

    if (nAnsiCodePageOverride != 0) {
        QString sCodePageProbe;
        if (!_decodeAnsiCodePage(QByteArray(), nAnsiCodePageOverride, &sCodePageProbe)) return listResult;
    }

    qint32 nOffset = headerInfo.nHeaderEndOffset;
    const quint64 nVersion = innoVersionValue(version);

    const InnoByteSkipper skipBytes(baBlock1);
    const InnoAnsiStringReader readString(baBlock1);
    const InnoAnsiEntriesSkipper skipEntryArray(baBlock1);

    if (version.nMajor < 5) {
        if (nVersion == innoVersionValue(1, 2, 10)) {
            const quint32 nCodePage = nAnsiCodePageOverride != 0 ? nAnsiCodePageOverride : 1252U;
            if (pnAnsiCodePage) *pnAnsiCodePage = nCodePage;
            // SetupEnt serializes each native-width pointer record as Longint
            // TotalSize, length-prefixed strings, then its pointer-free tail.
            // Keep every string inside that boundary and consume all arrays.
            const InnoSizedAnsiRecordReader readSizedRecord(baBlock1, &nOffset);

            for (qint32 i = 0; i < headerInfo.anCounts[0]; i++) {
                qint32 nTailOffset = -1;
                if (!readSizedRecord(1, 9, nullptr, &nTailOffset) || (((quint8)baBlock1.at(nTailOffset + 8) & 0xf8U) != 0)) {
                    return QList<FILE_ENTRY>();
                }
            }

            for (qint32 i = 0; i < headerInfo.nFileCount; i++) {
                QList<QByteArray> listStrings;
                qint32 nTailOffset = -1;
                const qint32 nFileTailSize = version.bWin16 ? 20 : 24;
                if (!readSizedRecord(3, nFileTailSize, &listStrings, &nTailOffset) || (listStrings.size() != 3)) return QList<FILE_ENTRY>();

                const qint32 nLocation = version.bWin16 ? qint32(qFromLittleEndian<qint16>(reinterpret_cast<const uchar *>(baBlock1.constData() + nTailOffset + 8)))
                                                        : qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(baBlock1.constData() + nTailOffset + 8));
                const qint32 nCopyModeOffset = version.bWin16 ? 16 : 20;
                const qint32 nOptionHighOffset = version.bWin16 ? 18 : 22;
                const quint8 nCopyMode = (quint8)baBlock1.at(nTailOffset + nCopyModeOffset);
                const quint8 nFileOptionHigh = (quint8)baBlock1.at(nTailOffset + nOptionHighOffset);
                const quint8 nFileType = (quint8)baBlock1.at(nTailOffset + nFileTailSize - 1);
                const quint8 nInvalidOptionMask = version.bWin16 ? 0xfeU : 0xf0U;
                const quint8 nMaximumFileType = version.bWin16 ? 1U : 2U;
                if ((nCopyMode > 3) || ((nFileOptionHigh & nInvalidOptionMask) != 0) || (nFileType > nMaximumFileType) || (nLocation < -1) ||
                    ((nLocation >= 0) && (nLocation >= headerInfo.nDataEntryCount))) {
                    return QList<FILE_ENTRY>();
                }

                const QByteArray &baDestination = listStrings.at(1);
                QString sDestination;
                if (!baDestination.isEmpty() && !_decodeAnsiCodePage(baDestination, nCodePage, &sDestination)) {
                    return QList<FILE_ENTRY>();
                }
                for (QChar ch : sDestination) {
                    if ((ch.unicode() == 0) || (ch.unicode() < 0x20)) {
                        return QList<FILE_ENTRY>();
                    }
                }
                if (!sDestination.isEmpty() && (nLocation >= 0)) {
                    FILE_ENTRY fileEntry = {};
                    fileEntry.sDestName = sDestination;
                    fileEntry.nLocationEntry = nLocation;
                    listResult.append(fileEntry);
                }
            }

            for (qint32 i = 0; i < headerInfo.anCounts[3]; i++) {
                qint32 nTailOffset = -1;
                const qint32 nIconTailSize = version.bWin16 ? 11 : 13;
                if (!readSizedRecord(6, nIconTailSize, nullptr, &nTailOffset) ||
                    (((quint8)baBlock1.at(nTailOffset + nIconTailSize - 1) & (version.bWin16 ? 0xf8U : 0xf0U)) != 0)) {
                    return QList<FILE_ENTRY>();
                }
            }
            for (qint32 i = 0; i < headerInfo.anCounts[4]; i++) {
                qint32 nTailOffset = -1;
                if (!readSizedRecord(4, 9, nullptr, &nTailOffset) || (((quint8)baBlock1.at(nTailOffset + 8) & 0xe0U) != 0)) {
                    return QList<FILE_ENTRY>();
                }
            }
            for (qint32 i = 0; i < headerInfo.anCounts[5]; i++) {
                qint32 nTailOffset = -1;
                const qint32 nRegistryStringCount = version.bWin16 ? 2 : 3;
                const qint32 nRegistryTailSize = version.bWin16 ? 10 : 14;
                const qint32 nRegistryTypeOffset = version.bWin16 ? 8 : 12;
                const qint32 nRegistryOptionsOffset = version.bWin16 ? 9 : 13;
                if (!readSizedRecord(nRegistryStringCount, nRegistryTailSize, nullptr, &nTailOffset) ||
                    ((quint8)baBlock1.at(nTailOffset + nRegistryTypeOffset) > (version.bWin16 ? 1U : 5U)) ||
                    (((quint8)baBlock1.at(nTailOffset + nRegistryOptionsOffset) & (version.bWin16 ? 0xf0U : 0xc0U)) != 0)) {
                    return QList<FILE_ENTRY>();
                }
            }
            for (qint32 nArray = 6; nArray <= 7; nArray++) {
                for (qint32 i = 0; i < headerInfo.anCounts[nArray]; i++) {
                    qint32 nTailOffset = -1;
                    if (!readSizedRecord(1, 9, nullptr, &nTailOffset) || ((quint8)baBlock1.at(nTailOffset + 8) > 2)) {
                        return QList<FILE_ENTRY>();
                    }
                }
            }
            for (qint32 nArray = 8; nArray <= 9; nArray++) {
                for (qint32 i = 0; i < headerInfo.anCounts[nArray]; i++) {
                    qint32 nTailOffset = -1;
                    if (!readSizedRecord(3, 10, nullptr, &nTailOffset) || ((quint8)baBlock1.at(nTailOffset + 8) > 2) ||
                        (((quint8)baBlock1.at(nTailOffset + 9) & 0xfeU) != 0)) {
                        return QList<FILE_ENTRY>();
                    }
                }
            }
            if (nOffset != baBlock1.size()) return QList<FILE_ENTRY>();
            return listResult;
        }

        quint32 nCodePage = 1252;
        const InnoAnsiBlobSkipper skipBlob(baBlock1, &nOffset);
        const InnoAnsiLanguageEntriesSkipper skipLanguage(baBlock1, &nOffset, &nCodePage);

        // Header counts are serialized in field-introduction order. Derive
        // their indices so every standard 1.3-4.2 schema follows one bounded
        // walk instead of a list of terminal-version snapshots.
        qint32 nCountIndex = 0;
        qint32 nLanguageIndex = -1;
        qint32 nMessageIndex = -1;
        qint32 nPermissionIndex = -1;
        qint32 nTypeIndex = -1;
        qint32 nComponentIndex = -1;
        qint32 nTaskIndex = -1;
        if (nVersion >= innoVersionValue(4, 0, 0)) nLanguageIndex = nCountIndex++;
        if (nVersion >= innoVersionValue(4, 2, 1)) nMessageIndex = nCountIndex++;
        if (nVersion >= innoVersionValue(4, 1, 0)) nPermissionIndex = nCountIndex++;
        if (nVersion >= innoVersionValue(2, 0, 0)) {
            nTypeIndex = nCountIndex++;
            nComponentIndex = nCountIndex++;
            nTaskIndex = nCountIndex++;
        }
        const qint32 nDirectoryIndex = nCountIndex++;
        const qint32 nFileIndex = nCountIndex++;
        const qint32 nDataIndex = nCountIndex++;
        const qint32 nIconIndex = nCountIndex++;
        const qint32 nIniIndex = nCountIndex++;
        const qint32 nRegistryIndex = nCountIndex++;
        const qint32 nDeleteIndex = nCountIndex++;
        const qint32 nUninstallDeleteIndex = nCountIndex++;
        const qint32 nRunIndex = nCountIndex++;
        const qint32 nUninstallRunIndex = nCountIndex++;
        if ((nCountIndex != headerInfo.nCountCount) || (headerInfo.anCounts[nFileIndex] != headerInfo.nFileCount) ||
            (headerInfo.anCounts[nDataIndex] != headerInfo.nDataEntryCount)) {
            return QList<FILE_ENTRY>();
        }

        if (nLanguageIndex >= 0) {
            const qint32 nLanguageStrings = 7 + (nVersion >= innoVersionValue(4, 0, 1) ? 3 : 0);
            const bool bStoredCodePage = nVersion >= innoVersionValue(4, 2, 2);
            const qint32 nLanguageFixedSize = 4 + (bStoredCodePage ? 4 : 0) + (nVersion < innoVersionValue(4, 1, 0) ? 20 : 16);
            if (!skipLanguage(headerInfo.anCounts[nLanguageIndex], nLanguageStrings, nLanguageFixedSize, bStoredCodePage)) return QList<FILE_ENTRY>();
        } else if ((nVersion >= innoVersionValue(2, 0, 1)) && (nVersion < innoVersionValue(4, 0, 0))) {
            // Inno 2.0.1-3.x has one implicit language record; it predates the
            // language-count field introduced by 4.0.
            if (!skipLanguage(1, 5, 24, false)) return QList<FILE_ENTRY>();
        }

        if (nVersion < innoVersionValue(4, 0, 0)) {
            if (!skipBlob() || ((nVersion >= innoVersionValue(2, 0, 0)) && !skipBlob()) || ((headerInfo.compression == INNO_COMPRESSION_BZIP2) && !skipBlob()))
                return QList<FILE_ENTRY>();
        }

        const qint32 nWindowsVersionRangeSize = nVersion >= innoVersionValue(1, 3, 19) ? 20 : 8;
        if ((nMessageIndex >= 0) && !skipEntryArray(headerInfo.anCounts[nMessageIndex], 2, 4, &nOffset)) return QList<FILE_ENTRY>();
        if ((nPermissionIndex >= 0) && !skipEntryArray(headerInfo.anCounts[nPermissionIndex], 1, 0, &nOffset)) return QList<FILE_ENTRY>();
        if (nTypeIndex >= 0) {
            const qint32 nTypeStrings = 2 + (nVersion >= innoVersionValue(4, 0, 0) ? 1 : 0) + (nVersion >= innoVersionValue(4, 0, 1) ? 1 : 0);
            const qint32 nTypeFixedSize =
                nWindowsVersionRangeSize + 1 + (nVersion >= innoVersionValue(4, 0, 3) ? 1 : 0) + (nVersion >= innoVersionValue(4, 0, 0) ? 8 : 4);
            const qint32 nComponentStrings = 3 + (nVersion >= innoVersionValue(4, 0, 0) ? 1 : 0) + (nVersion >= innoVersionValue(4, 0, 1) ? 1 : 0);
            const qint32 nComponentFixedSize = (nVersion >= innoVersionValue(4, 0, 0) ? 8 : 4) + (nVersion >= innoVersionValue(4, 0, 0) ? 5 : 0) +
                                               nWindowsVersionRangeSize + 1 + (nVersion >= innoVersionValue(4, 0, 0) ? 8 : 4);
            const qint32 nTaskStrings = 4 + (nVersion >= innoVersionValue(4, 0, 0) ? 1 : 0) + (nVersion >= innoVersionValue(4, 0, 1) ? 1 : 0);
            const qint32 nTaskFixedSize = (nVersion >= innoVersionValue(4, 0, 0) ? 5 : 0) + nWindowsVersionRangeSize + 1;
            if (!skipEntryArray(headerInfo.anCounts[nTypeIndex], nTypeStrings, nTypeFixedSize, &nOffset) ||
                !skipEntryArray(headerInfo.anCounts[nComponentIndex], nComponentStrings, nComponentFixedSize, &nOffset) ||
                !skipEntryArray(headerInfo.anCounts[nTaskIndex], nTaskStrings, nTaskFixedSize, &nOffset)) {
                return QList<FILE_ENTRY>();
            }
        }

        qint32 nConditionStrings = 0;
        if (nVersion >= innoVersionValue(2, 0, 0)) nConditionStrings += 2;
        if (nVersion >= innoVersionValue(4, 0, 0)) nConditionStrings++;
        if (nVersion >= innoVersionValue(4, 0, 1)) nConditionStrings++;
        if (nVersion >= innoVersionValue(4, 1, 0)) nConditionStrings += 2;
        const qint32 nDirectoryStrings = 1 + nConditionStrings + (((nVersion >= innoVersionValue(4, 0, 11)) && (nVersion < innoVersionValue(4, 1, 0))) ? 1 : 0);
        const qint32 nDirectoryFixedSize =
            nWindowsVersionRangeSize + 1 + (nVersion >= innoVersionValue(2, 0, 11) ? 4 : 0) + (nVersion >= innoVersionValue(4, 1, 0) ? 2 : 0);
        if (!skipEntryArray(headerInfo.anCounts[nDirectoryIndex], nDirectoryStrings, nDirectoryFixedSize, &nOffset)) return QList<FILE_ENTRY>();

        const qint32 nFileStringCount = 3 + nConditionStrings;
        qint32 nFileFlagCount = 4 + 3 + 2 + 1 + 1;
        if (nVersion < innoVersionValue(2, 0, 0)) nFileFlagCount++;
        if (nVersion >= innoVersionValue(1, 3, 21)) nFileFlagCount += 2;
        if (nVersion >= innoVersionValue(1, 3, 25)) nFileFlagCount++;
        if (nVersion >= innoVersionValue(2, 0, 5)) nFileFlagCount++;
        if (nVersion >= innoVersionValue(3, 0, 1)) nFileFlagCount++;
        if (nVersion >= innoVersionValue(3, 0, 5)) nFileFlagCount += 3;
        if (nVersion >= innoVersionValue(4, 0, 0)) nFileFlagCount++;
        if (nVersion >= innoVersionValue(4, 0, 5)) nFileFlagCount++;
        if (nVersion >= innoVersionValue(4, 1, 8)) nFileFlagCount++;
        if (nVersion >= innoVersionValue(4, 2, 1)) nFileFlagCount++;
        if (nVersion >= innoVersionValue(4, 2, 5)) nFileFlagCount++;
        qint32 nFileFlagBytes = (nFileFlagCount + 7) / 8;
        if (nFileFlagBytes == 3) nFileFlagBytes = 4;
        const qint32 nFileFixedSize = nWindowsVersionRangeSize + 4 + 4 + (nVersion >= innoVersionValue(4, 0, 0) ? 8 : 4) +
                                      (nVersion < innoVersionValue(3, 0, 5) ? 1 : 0) + (nVersion >= innoVersionValue(4, 1, 0) ? 2 : 0) + nFileFlagBytes + 1;
        const qint32 nFileLocationOffset = nWindowsVersionRangeSize;

        if (nAnsiCodePageOverride != 0) nCodePage = nAnsiCodePageOverride;
        if (pnAnsiCodePage) *pnAnsiCodePage = nCodePage;

        for (qint32 i = 0; i < headerInfo.nFileCount; i++) {
            QByteArray baDestination;
            for (qint32 j = 0; j < nFileStringCount; j++) {
                QByteArray baValue;
                if (!readString(&nOffset, &baValue)) return QList<FILE_ENTRY>();
                if (j == 1) baDestination = baValue;
            }

            const qint32 nTailOffset = nOffset;
            if (!skipBytes(nFileFixedSize, &nOffset)) return QList<FILE_ENTRY>();
            const quint32 nLocation = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baBlock1.constData() + nTailOffset + nFileLocationOffset));
            const quint8 nFileType = (quint8)baBlock1.at(nTailOffset + nFileFixedSize - 1);
            if ((nFileType > 2) || ((nLocation != 0xffffffffU) && (nLocation >= (quint32)headerInfo.nDataEntryCount))) {
                return QList<FILE_ENTRY>();
            }

            QString sDestination;
            if (!_decodeAnsiCodePage(baDestination, nCodePage, &sDestination)) {
                return QList<FILE_ENTRY>();
            }
            for (QChar ch : sDestination) {
                if ((ch.unicode() == 0) || (ch.unicode() < 0x20)) {
                    return QList<FILE_ENTRY>();
                }
            }
            if (!sDestination.isEmpty() && (nLocation != 0xffffffffU)) {
                FILE_ENTRY fileEntry = {};
                fileEntry.sDestName = sDestination;
                fileEntry.nLocationEntry = (qint32)nLocation;
                listResult.append(fileEntry);
            }
        }

        const qint32 nIconStrings = 6 + nConditionStrings;
        const qint32 nIconFixedSize = nWindowsVersionRangeSize + 4 + (nVersion >= innoVersionValue(1, 3, 24) ? 4 : 0) + (nVersion >= innoVersionValue(1, 3, 15) ? 1 : 0) +
                                      (nVersion >= innoVersionValue(2, 0, 7) ? 2 : 0) + 1;
        const qint32 nIniStrings = 4 + nConditionStrings;
        const qint32 nIniFixedSize = nWindowsVersionRangeSize + 1;
        const qint32 nRegistryStrings = 3 + nConditionStrings + (((nVersion >= innoVersionValue(4, 0, 11)) && (nVersion < innoVersionValue(4, 1, 0))) ? 1 : 0);
        const qint32 nRegistryFlagBytes = nVersion >= innoVersionValue(1, 3, 12) ? 2 : 1;
        const qint32 nRegistryFixedSize = nWindowsVersionRangeSize + 4 + (nVersion >= innoVersionValue(4, 1, 0) ? 2 : 0) + 1 + nRegistryFlagBytes;
        const qint32 nDeleteStrings = 1 + nConditionStrings;
        const qint32 nDeleteFixedSize = nWindowsVersionRangeSize + 1;
        const qint32 nRunStrings = 3 + nConditionStrings + (nVersion >= innoVersionValue(1, 3, 9) ? 1 : 0) + (nVersion >= innoVersionValue(2, 0, 2) ? 1 : 0) +
                                   (nVersion >= innoVersionValue(2, 0, 0) ? 1 : 0);
        const qint32 nRunFixedSize = nWindowsVersionRangeSize + (nVersion >= innoVersionValue(1, 3, 24) ? 4 : 0) + 2;
        if (!skipEntryArray(headerInfo.anCounts[nIconIndex], nIconStrings, nIconFixedSize, &nOffset) ||
            !skipEntryArray(headerInfo.anCounts[nIniIndex], nIniStrings, nIniFixedSize, &nOffset) ||
            !skipEntryArray(headerInfo.anCounts[nRegistryIndex], nRegistryStrings, nRegistryFixedSize, &nOffset) ||
            !skipEntryArray(headerInfo.anCounts[nDeleteIndex], nDeleteStrings, nDeleteFixedSize, &nOffset) ||
            !skipEntryArray(headerInfo.anCounts[nUninstallDeleteIndex], nDeleteStrings, nDeleteFixedSize, &nOffset) ||
            !skipEntryArray(headerInfo.anCounts[nRunIndex], nRunStrings, nRunFixedSize, &nOffset) ||
            !skipEntryArray(headerInfo.anCounts[nUninstallRunIndex], nRunStrings, nRunFixedSize, &nOffset)) {
            return QList<FILE_ENTRY>();
        }

        if (nVersion >= innoVersionValue(4, 0, 0)) {
            if (!skipBlob() || !skipBlob()) return QList<FILE_ENTRY>();
            const bool bHasDecompressor = (headerInfo.compression == INNO_COMPRESSION_BZIP2) ||
                                          ((nVersion == innoVersionValue(4, 1, 5)) && (headerInfo.compression == INNO_COMPRESSION_LZMA1)) ||
                                          ((nVersion >= innoVersionValue(4, 2, 6)) && (headerInfo.compression == INNO_COMPRESSION_ZLIB));
            if ((bHasDecompressor && !skipBlob()) || (headerInfo.bEncryptionUsed && !skipBlob())) {
                return QList<FILE_ENTRY>();
            }
        }
        if (nOffset != baBlock1.size()) return QList<FILE_ENTRY>();
        return listResult;
    }

    if (version.nMajor != 5) return listResult;

    // Non-Unicode setup data does not record the build machine's ACP. Match
    // Setup's long-standing behaviour as closely as the format permits: use
    // the first language code page, preferring Windows-1252 when present.
    // This is the same deterministic heuristic used by innoextract.
    const qint32 nLanguageTail = 24 + ((nVersion >= innoVersionValue(5, 2, 3)) ? 1 : 0);
    quint32 nCodePage = 1252;
    bool bHaveLanguageCodePage = false;
    for (qint32 i = 0; i < headerInfo.anCounts[0]; i++) {
        for (qint32 j = 0; j < 10; j++) {
            if (!readString(&nOffset, nullptr)) return QList<FILE_ENTRY>();
        }
        if ((nOffset < 0) || (nOffset > baBlock1.size() - nLanguageTail)) return QList<FILE_ENTRY>();
        quint32 nLanguageCodePage = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baBlock1.constData() + nOffset + 4));
        if (nLanguageCodePage == 0) nLanguageCodePage = 1252;
        if (!bHaveLanguageCodePage || (nLanguageCodePage == 1252)) nCodePage = nLanguageCodePage;
        bHaveLanguageCodePage = true;
        if (!skipBytes(nLanguageTail, &nOffset)) return QList<FILE_ENTRY>();
    }
    if (nAnsiCodePageOverride != 0) nCodePage = nAnsiCodePageOverride;
    if (pnAnsiCodePage) *pnAnsiCodePage = nCodePage;

    if (!skipEntryArray(headerInfo.anCounts[1], 2, 4, &nOffset) || !skipEntryArray(headerInfo.anCounts[2], 1, 0, &nOffset) ||
        !skipEntryArray(headerInfo.anCounts[3], 4, 30, &nOffset) || !skipEntryArray(headerInfo.anCounts[4], 5, 42, &nOffset) ||
        !skipEntryArray(headerInfo.anCounts[5], 6, 26, &nOffset) || !skipEntryArray(headerInfo.anCounts[6], 7, 27, &nOffset))
        return QList<FILE_ENTRY>();

    const qint32 nStringCount = (nVersion >= innoVersionValue(5, 2, 5)) ? 10 : 9;

    for (qint32 i = 0; i < headerInfo.nFileCount; i++) {
        QByteArray baDestination;

        for (qint32 j = 0; j < nStringCount; j++) {
            QByteArray baValue;
            if (!readString(&nOffset, &baValue)) return QList<FILE_ENTRY>();
            if (j == 1) baDestination = baValue;
        }

        const qint32 nTailOffset = nOffset;
        if (!skipBytes(43, &nOffset)) return QList<FILE_ENTRY>();

        const quint32 nLocation = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baBlock1.constData() + nTailOffset + 20));
        const quint8 nFileType = (quint8)baBlock1.at(nTailOffset + 42);

        if ((nFileType > 1) || ((nLocation != 0xffffffffU) && (nLocation >= (quint32)headerInfo.nDataEntryCount))) {
            return QList<FILE_ENTRY>();
        }

        QString sDestination;
        if (!_decodeAnsiCodePage(baDestination, nCodePage, &sDestination)) return QList<FILE_ENTRY>();
        if (!sDestination.isEmpty()) {
            for (QChar ch : sDestination) {
                if ((ch.unicode() == 0) || (ch.unicode() < 0x20)) return QList<FILE_ENTRY>();
            }
        }

        if (!sDestination.isEmpty() && (nLocation != 0xffffffffU)) {
            FILE_ENTRY fileEntry = {};
            fileEntry.sDestName = sDestination;
            fileEntry.nLocationEntry = (qint32)nLocation;
            listResult.append(fileEntry);
        }
    }

    return listResult;
}

bool XInnoSetup::_readNextLegacySetupBlock(
    qint64 *pnCursor, qint64 nSetup0End, QByteArray *pBlock,
    PDSTRUCT *pPdStruct)
{
    if (pBlock) pBlock->clear();
    if (!pnCursor || !pBlock ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    qint64 nConsumed = 0;
    const QByteArray baBlock = _readLegacyBlock(
        *pnCursor, nSetup0End, &nConsumed, pPdStruct);
    if (baBlock.isEmpty() || (nConsumed <= 16) ||
        (*pnCursor > nSetup0End - nConsumed)) {
        return false;
    }
    *pnCursor += nConsumed;
    *pBlock = baBlock;
    return true;
}

bool XInnoSetup::_parseLegacySetup0(UNPACK_CONTEXT *pContext, const OFFSET_TABLE &offsetTable, const INNO_VERSION &version, qint32 nVersionIdSize,
                                     PDSTRUCT *pPdStruct)
{
    if (!pContext || !offsetTable.bIsValid || !version.bIsValid || !version.bLegacyShortId || version.bWin16 || (nVersionIdSize != 8)) return false;

    const bool b109 = (version.nMajor == 1) && (version.nMinor == 9) && (offsetTable.nLoaderVersion == INNO_LOADER_LEGACY_VX);
    const bool b111 = (version.nMajor == 1) && (version.nMinor == 11) && (offsetTable.nLoaderVersion == INNO_LOADER_LEGACY_EOF40);
    if (!b109 && !b111) return false;

    const qint64 nFileSize = getSize();
    const qint64 nSetup0End = b109 ? offsetTable.nDataOffset : offsetTable.nTableOffset;
    if ((offsetTable.nHeaderOffset < 0) || (offsetTable.nHeaderOffset > nSetup0End - nVersionIdSize - 16) || (nSetup0End > nFileSize)) return false;

    qint64 nCursor = offsetTable.nHeaderOffset + nVersionIdSize;

    QByteArray baHeader;
    if (!_readNextLegacySetupBlock(&nCursor, nSetup0End, &baHeader,
                                   pPdStruct)) {
        return false;
    }

    quint32 anCounts[9] = {};
    quint32 anInfoSizes[3] = {};
    qint32 nCountCount = 0;
    if (b109) {
        if (baHeader.size() != 434) return false;
        nCountCount = 8;
        for (qint32 i = 0; i < nCountCount; i++) {
            anCounts[i] = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baHeader.constData() + 0x180 + i * 4));
        }
    } else {
        qint32 nHeaderOffset = 0;
        for (qint32 i = 0; i < 6; i++) {
            if ((nHeaderOffset < 0) || (nHeaderOffset > baHeader.size() - 4)) return false;
            const quint32 nLength = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baHeader.constData() + nHeaderOffset));
            if ((nLength > 0x1000000U) || (nLength > quint32(baHeader.size() - nHeaderOffset - 4))) return false;
            nHeaderOffset += 4 + (qint32)nLength;
        }
        // Nine counts, three optional-text sizes, and the exact 14-byte 1.11
        // options tail form the generation anchor after the six strings.
        if ((nHeaderOffset < 0) || (baHeader.size() - nHeaderOffset != 62)) return false;
        nCountCount = 9;
        for (qint32 i = 0; i < nCountCount; i++) {
            anCounts[i] = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baHeader.constData() + nHeaderOffset + i * 4));
        }
        for (qint32 i = 0; i < 3; i++) {
            anInfoSizes[i] = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baHeader.constData() + nHeaderOffset + (9 + i) * 4));
        }
    }

    quint64 nBlockCount = 1;
    for (qint32 i = 0; i < nCountCount; i++) {
        if (anCounts[i] > quint32(INNO_MAX_LEGACY_RECORDS)) return false;
        nBlockCount += anCounts[i];
    }
    nBlockCount += anCounts[1];  // One location record per file record.
    for (quint32 nSize : anInfoSizes) {
        if (nSize > quint32(INNO_MAX_LEGACY_BLOCK_SIZE)) return false;
        if (nSize != 0) nBlockCount++;
    }
    if ((anCounts[1] == 0) || (nBlockCount > quint64(INNO_MAX_LEGACY_RECORDS))) return false;

    for (quint32 nExpectedSize : anInfoSizes) {
        if (nExpectedSize == 0) continue;
        QByteArray baInfo;
        if (!_readNextLegacySetupBlock(&nCursor, nSetup0End, &baInfo,
                                       pPdStruct) ||
            (baInfo.size() != nExpectedSize)) {
            return false;
        }
    }
    for (quint32 i = 0; i < anCounts[0]; i++) {
        QByteArray baDirectory;
        if (!_readNextLegacySetupBlock(&nCursor, nSetup0End, &baDirectory,
                                       pPdStruct)) {
            return false;
        }
    }

    struct LEGACY_FILE_INFO {
        QString sName;
        quint64 nFileTime;
        quint32 nOriginalSize;
        quint32 nCompressedSize;
        quint32 nAdler32;
    };
    QList<LEGACY_FILE_INFO> listLegacyFiles;
    listLegacyFiles.reserve((qint32)anCounts[1]);
    const quint32 nCodePage = pContext->nAnsiCodePageOverride ? pContext->nAnsiCodePageOverride : 1252U;

    for (quint32 i = 0; i < anCounts[1]; i++) {
        QByteArray baFile;
        if (!_readNextLegacySetupBlock(&nCursor, nSetup0End, &baFile,
                                       pPdStruct)) {
            return false;
        }

        QByteArray baName;
        LEGACY_FILE_INFO fileInfo = {};
        if (b109) {
            if (baFile.size() != 95) return false;
            const quint8 nNameLength = (quint8)baFile.at(0);
            if (nNameLength > 63) return false;
            baName = baFile.mid(1, nNameLength);
            fileInfo.nFileTime = qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(baFile.constData() + 0x40));
            fileInfo.nOriginalSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baFile.constData() + 0x50));
            fileInfo.nCompressedSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baFile.constData() + 0x54));
            fileInfo.nAdler32 = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baFile.constData() + 0x58));
        } else {
            if (baFile.size() < 51) return false;
            const quint32 nNameLength = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baFile.constData()));
            if ((nNameLength > 0x1000000U) || (nNameLength > quint32(baFile.size() - 4)) || (quint64(nNameLength) + 51 != quint64(baFile.size()))) return false;
            baName = baFile.mid(4, (qint32)nNameLength);
            const qint32 nNameEnd = 4 + (qint32)nNameLength;
            fileInfo.nFileTime = qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(baFile.constData() + nNameEnd + 0x0c));
            fileInfo.nOriginalSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baFile.constData() + baFile.size() - 19));
            fileInfo.nCompressedSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baFile.constData() + baFile.size() - 15));
            fileInfo.nAdler32 = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baFile.constData() + baFile.size() - 11));
        }

        if ((fileInfo.nOriginalSize > quint32((std::numeric_limits<qint32>::max)())) || (fileInfo.nCompressedSize == 0) ||
            (fileInfo.nCompressedSize > quint32((std::numeric_limits<qint32>::max)())))
            return false;
        if (pContext->nAnsiCodePageOverride != 0) {
            if (!_decodeAnsiCodePage(baName, nCodePage, &fileInfo.sName)) return false;
        } else {
            fileInfo.sName = _decodeWindows1252(baName.constData(), baName.size());
        }
        for (QChar ch : fileInfo.sName) {
            if ((ch.unicode() == 0) || (ch.unicode() < 0x20)) return false;
        }
        listLegacyFiles.append(fileInfo);
    }

    if ((offsetTable.nDataOffset < 0) || (offsetTable.nDataOffset > nFileSize - 12) ||
        (read_array(offsetTable.nDataOffset, 8) != QByteArray("idska32\x1a", 8)))
        return false;
    const quint32 nDeclaredDataSize = read_uint32(offsetTable.nDataOffset + 8, false);
    const qint64 nDataEnd = b109 ? nFileSize : offsetTable.nExeOffset;
    if ((nDataEnd <= offsetTable.nDataOffset + 12) || (nDataEnd > nFileSize) ||
        (b109 ? (quint64(nDeclaredDataSize) != quint64(nDataEnd - offsetTable.nDataOffset)) : (nDeclaredDataSize != 0)))
        return false;

    QList<DATA_ENTRY> listDataEntries;
    QList<FILE_ENTRY> listFileEntries;
    listDataEntries.reserve(listLegacyFiles.size());
    listFileEntries.reserve(listLegacyFiles.size());
    qint64 nExpectedStart = 12;
    const qint64 nDataExtent = nDataEnd - offsetTable.nDataOffset;
    for (qint32 i = 0; i < listLegacyFiles.size(); i++) {
        QByteArray baLocation;
        if (!_readNextLegacySetupBlock(&nCursor, nSetup0End, &baLocation,
                                       pPdStruct) ||
            (baLocation.size() != 12)) {
            return false;
        }
        const quint32 nFirstDisk = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baLocation.constData()));
        const quint32 nLastDisk = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baLocation.constData() + 4));
        const quint32 nStartOffset = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baLocation.constData() + 8));
        const LEGACY_FILE_INFO &fileInfo = listLegacyFiles.at(i);

        if ((nFirstDisk != 1) || (nLastDisk != 1) || (qint64(nStartOffset) != nExpectedStart) || (nExpectedStart > nDataExtent - 4) ||
            (qint64(fileInfo.nCompressedSize) > nDataExtent - nExpectedStart - 4) ||
            (read_array(offsetTable.nDataOffset + nExpectedStart, 4) != QByteArray("zlb\x1a", 4)))
            return false;

        DATA_ENTRY dataEntry = {};
        dataEntry.nFirstSlice = 0;
        dataEntry.nLastSlice = 0;
        dataEntry.nChunkStartOffset = nExpectedStart;
        dataEntry.nChunkSubOffset = 0;
        dataEntry.nOriginalSize = fileInfo.nOriginalSize;
        dataEntry.nChunkCompressedSize = fileInfo.nCompressedSize;
        dataEntry.baChecksum.resize(4);
        qToLittleEndian<quint32>(fileInfo.nAdler32, reinterpret_cast<uchar *>(dataEntry.baChecksum.data()));
        dataEntry.checksumType = INNO_CHECKSUM_ADLER32;
        dataEntry.compression = INNO_COMPRESSION_ZLIB;
        dataEntry.nFileTime = fileInfo.nFileTime;
        dataEntry.bChunkCompressed = true;
        listDataEntries.append(dataEntry);

        if (!fileInfo.sName.isEmpty()) {
            FILE_ENTRY fileEntry = {};
            fileEntry.sDestName = fileInfo.sName;
            fileEntry.nLocationEntry = i;
            listFileEntries.append(fileEntry);
        }
        nExpectedStart += 4 + qint64(fileInfo.nCompressedSize);
    }
    if (nExpectedStart != nDataExtent) return false;

    for (qint32 nCountIndex = 2; nCountIndex < nCountCount; nCountIndex++) {
        for (quint32 i = 0; i < anCounts[nCountIndex]; i++) {
            QByteArray baIgnored;
            if (!_readNextLegacySetupBlock(&nCursor, nSetup0End, &baIgnored,
                                           pPdStruct)) {
                return false;
            }
        }
    }
    if (nCursor != nSetup0End) return false;

    pContext->nAnsiCodePage = nCodePage;
    return _finalizeRealRecords(pContext, listDataEntries, listFileEntries, offsetTable.nDataOffset, version, pPdStruct);
}

bool XInnoSetup::_probeZlibMember(qint64 nOffset, qint64 nLimit, qint64 *pnCompressedSize, qint64 *pnOriginalSize, quint32 *pnAdler32, PDSTRUCT *pPdStruct)
{
    if (pnCompressedSize) *pnCompressedSize = 0;
    if (pnOriginalSize) *pnOriginalSize = 0;
    if (pnAdler32) *pnAdler32 = 0;
    if (!pnCompressedSize || !pnOriginalSize || !pnAdler32 || (nOffset < 0) || (nLimit > getSize()) || (nOffset > nLimit - 6) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    const QByteArray baHeader = read_array(nOffset, 2);
    if (baHeader.size() != 2) return false;
    const quint8 nCMF = (quint8)baHeader.at(0);
    const quint8 nFLG = (quint8)baHeader.at(1);
    const quint16 nHeader = (quint16(nCMF) << 8) | nFLG;
    if (((nCMF & 0x0fU) != 8U) || ((nCMF >> 4) > 7U) || ((nHeader % 31U) != 0U) || ((nFLG & 0x20U) != 0U)) return false;

    InnoBoundedBuffer output(INNO_MAX_HEADER_BLOCK_SIZE);
    if (!output.open(QIODevice::ReadWrite)) return false;
    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = getDevice();
    state.pDeviceOutput = &output;
    state.nInputOffset = nOffset + 2;
    state.nInputLimit = nLimit - state.nInputOffset;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = INNO_MAX_HEADER_BLOCK_SIZE + 1;
    const bool bDecoded = XDeflateDecoder::decompress(&state, pPdStruct);
    output.close();
    if (!bDecoded || state.bReadError || state.bWriteError || (state.nCountInput <= 0) || (state.nCountOutput < 0) ||
        (state.nCountOutput > INNO_MAX_HEADER_BLOCK_SIZE) || (state.nCountOutput != output.data().size()) ||
        (state.nCountInput > nLimit - nOffset - 6))
        return false;

    const qint64 nFooterOffset = nOffset + 2 + state.nCountInput;
    if ((nFooterOffset < nOffset) || (nFooterOffset > nLimit - 4)) return false;
    const QByteArray baFooter = read_array(nFooterOffset, 4);
    if (baFooter.size() != 4) return false;
    const quint32 nExpectedAdler = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(baFooter.constData()));
    quint32 nActualAdler = 0;
    if (!innoCalculateAdler32(output.data(), &nActualAdler, pPdStruct) || (nActualAdler != nExpectedAdler)) return false;

    *pnCompressedSize = 2 + state.nCountInput + 4;
    *pnOriginalSize = state.nCountOutput;
    *pnAdler32 = nExpectedAdler;
    return true;
}

bool XInnoSetup::_parseISXDataFallback(UNPACK_CONTEXT *pContext, const OFFSET_TABLE &offsetTable, const INNO_VERSION &version, PDSTRUCT *pPdStruct)
{
    if (!pContext || !offsetTable.bIsValid || !version.bIsValid || !version.bIsx || (offsetTable.nRevision != 0) || (offsetTable.nLoaderVersion != 2)) return false;
    const qint64 nDataOffset = offsetTable.nDataOffset;
    const qint64 nDataEnd = offsetTable.nExeOffset;
    if ((nDataOffset <= 0) || (nDataEnd <= nDataOffset) || (nDataEnd > getSize()) || (nDataEnd - nDataOffset > INNO_MAX_FALLBACK_DATA_SIZE)) return false;

    QList<DATA_ENTRY> listDataEntries;
    QList<FILE_ENTRY> listFileEntries;
    qint64 nPosition = nDataOffset;
    qint64 nTotalOutput = 0;
    while ((nPosition < nDataEnd) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if ((listDataEntries.size() >= INNO_MAX_LEGACY_RECORDS) || (nPosition > nDataEnd - 4) ||
            (read_array(nPosition, 4) != QByteArray("zlb\x1a", 4)))
            return false;

        qint64 nCompressedSize = 0;
        qint64 nOriginalSize = 0;
        quint32 nAdler32 = 0;
        if (!_probeZlibMember(nPosition + 4, nDataEnd, &nCompressedSize, &nOriginalSize, &nAdler32, pPdStruct) ||
            (nCompressedSize <= 0) || (nCompressedSize > (std::numeric_limits<qint32>::max)()) ||
            (nOriginalSize > (std::numeric_limits<qint32>::max)()) || (nTotalOutput > INNO_MAX_FALLBACK_TOTAL_OUTPUT - nOriginalSize))
            return false;

        DATA_ENTRY dataEntry = {};
        dataEntry.nFirstSlice = 0;
        dataEntry.nLastSlice = 0;
        dataEntry.nChunkStartOffset = nPosition - nDataOffset;
        dataEntry.nChunkSubOffset = 0;
        dataEntry.nOriginalSize = nOriginalSize;
        dataEntry.nChunkCompressedSize = nCompressedSize;
        dataEntry.baChecksum.resize(4);
        qToLittleEndian<quint32>(nAdler32, reinterpret_cast<uchar *>(dataEntry.baChecksum.data()));
        dataEntry.checksumType = INNO_CHECKSUM_ADLER32;
        dataEntry.compression = INNO_COMPRESSION_ZLIB;
        dataEntry.bChunkCompressed = true;
        listDataEntries.append(dataEntry);

        FILE_ENTRY fileEntry = {};
        fileEntry.sDestName = QStringLiteral("data/%1.bin").arg(listDataEntries.size(), 4, 10, QChar('0'));
        fileEntry.nLocationEntry = listDataEntries.size() - 1;
        listFileEntries.append(fileEntry);

        nTotalOutput += nOriginalSize;
        if (nPosition > nDataEnd - 4 - nCompressedSize) return false;
        nPosition += 4 + nCompressedSize;
    }

    if ((nPosition != nDataEnd) || listDataEntries.isEmpty()) return false;
    return _finalizeRealRecords(pContext, listDataEntries, listFileEntries, nDataOffset, version, pPdStruct);
}

bool XInnoSetup::_finalizeRealRecords(UNPACK_CONTEXT *pContext, const QList<DATA_ENTRY> &listDataEntries, const QList<FILE_ENTRY> &listFileEntries,
                                      qint64 nDataStreamOffset, const INNO_VERSION &version, PDSTRUCT *pPdStruct)
{
    if (!pContext || !version.bIsValid || listDataEntries.isEmpty() || listFileEntries.isEmpty() || (nDataStreamOffset < 0)) return false;
    const qint64 nFileSize = getSize();
    const qint32 nNumDataEntries = listDataEntries.count();

    pContext->listDataEntries = listDataEntries;
    pContext->listFileEntries = listFileEntries;
    pContext->nDataStreamOffset = nDataStreamOffset;
    pContext->version = version;
    pContext->sPassword.fill(QChar());
    pContext->sPassword.clear();
    innoSecureClear(&pContext->baPasswordBytes);
    pContext->bHasPasswordBytes = false;

    for (const DATA_ENTRY &entry : listDataEntries) {
        if (entry.bChunkEncrypted && ((pContext->nEncryptionUse == 0) || (pContext->encryption == INNO_ENCRYPTION_NONE))) {
            setPdStructErrorString(pPdStruct, tr("Invalid encrypted Inno Setup data entry"));
            return false;
        }
    }

    const bool bExternalSlices = (nDataStreamOffset == 0);
    if (bExternalSlices && !_prepareSliceSources(pContext, listDataEntries, pPdStruct)) return false;

    // Validate every exact StartOffset. Solid members share a start; unrelated chunks may
    // legitimately have equal compressed sizes and must remain distinct.
    QMap<QString, QString> mapChunks;
    for (const DATA_ENTRY &entry : listDataEntries) {
        if ((entry.nChunkCompressedSize < 0) || (entry.nChunkCompressedSize > (std::numeric_limits<qint32>::max)())) return false;
        const qint64 nEncryptionOverhead =
            entry.bChunkEncrypted && ((pContext->encryption == INNO_ENCRYPTION_ARC4_MD5) || (pContext->encryption == INNO_ENCRYPTION_ARC4_SHA1)) ? 8 : 0;

        if (!bExternalSlices) {
            if ((entry.nFirstSlice != 0) || (entry.nLastSlice != 0) || (nDataStreamOffset > nFileSize - 4) ||
                (entry.nChunkStartOffset > nFileSize - nDataStreamOffset - 4) || (nEncryptionOverhead > nFileSize - nDataStreamOffset - entry.nChunkStartOffset - 4) ||
                (entry.nChunkCompressedSize > nFileSize - nDataStreamOffset - entry.nChunkStartOffset - 4 - nEncryptionOverhead))
                return false;
            const qint64 nChunkOffset = nDataStreamOffset + entry.nChunkStartOffset;
            if (read_array(nChunkOffset, 4) != QByteArray("zlb\x1a", 4)) return false;
        } else {
            SLICE_SOURCE *pFirst = pContext->mapSliceSources.value(entry.nFirstSlice, nullptr);
            if (!pFirst || !pFirst->pFile || !pFirst->pValidator || (entry.nChunkStartOffset < pFirst->nDataOffset) ||
                (entry.nChunkStartOffset > pFirst->pFile->size() - 4) || !pFirst->pValidator->isUnpackSourceCurrent(&pFirst->validationState, pPdStruct) ||
                !pFirst->pFile->seek(entry.nChunkStartOffset) || (pFirst->pFile->read(4) != QByteArray("zlb\x1a", 4)) ||
                !pFirst->pValidator->isUnpackSourceCurrent(&pFirst->validationState, pPdStruct))
                return false;

            qint64 nRequired = entry.nChunkCompressedSize + 4 + nEncryptionOverhead;
            for (quint64 nSlice64 = entry.nFirstSlice; nSlice64 <= quint64(entry.nLastSlice); nSlice64++) {
                SLICE_SOURCE *pSlice = pContext->mapSliceSources.value((quint32)nSlice64, nullptr);
                if (!pSlice || !pSlice->pFile) return false;
                const qint64 nStart = (nSlice64 == entry.nFirstSlice) ? entry.nChunkStartOffset : pSlice->nDataOffset;
                if ((nStart < pSlice->nDataOffset) || (nStart > pSlice->pFile->size())) return false;
                const qint64 nAvailable = pSlice->pFile->size() - nStart;
                if (nSlice64 < entry.nLastSlice) {
                    if (nRequired <= nAvailable) return false;
                    nRequired -= nAvailable;
                } else if (nRequired > nAvailable) {
                    return false;
                }
            }
        }

        const QString sChunkKey = QString::number(entry.nFirstSlice) + QLatin1Char(':') + QString::number(entry.nChunkStartOffset);
        const QString sChunkSignature = QString::number(entry.nLastSlice) + QLatin1Char(':') + QString::number(entry.nChunkCompressedSize) + QLatin1Char(':') +
                                        QString::number((qint32)entry.compression) + QLatin1Char(':') + QString::number(entry.bChunkEncrypted ? 1 : 0);
        if (mapChunks.contains(sChunkKey)) {
            if (mapChunks.value(sChunkKey) != sChunkSignature) return false;
        } else {
            mapChunks.insert(sChunkKey, sChunkSignature);
        }
    }

    QList<ARCHIVERECORD> listRecords;
    QList<qint32> listRecordDataEntryIndexes;
    for (const FILE_ENTRY &fileEntry : listFileEntries) {
        const qint32 nLocIdx = fileEntry.nLocationEntry;
        if ((nLocIdx < 0) || (nLocIdx >= nNumDataEntries)) continue;
        const DATA_ENTRY &dataEntry = listDataEntries.at(nLocIdx);

        ARCHIVERECORD record = {};
        if (!bExternalSlices && (dataEntry.nChunkStartOffset > (std::numeric_limits<qint64>::max)() - nDataStreamOffset)) return false;
        record.nStreamOffset = bExternalSlices ? dataEntry.nChunkStartOffset : nDataStreamOffset + dataEntry.nChunkStartOffset;
        record.nStreamSize = dataEntry.nChunkCompressedSize;
        _setDestinationProperties(&record, fileEntry.sDestName);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, dataEntry.nOriginalSize);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, dataEntry.nChunkCompressedSize);
        record.mapProperties.insert(FPART_PROP_STREAMOFFSET, dataEntry.nChunkSubOffset);
        record.mapProperties.insert(FPART_PROP_STREAMSIZE, dataEntry.nOriginalSize);
        record.mapProperties.insert(FPART_PROP_ISFOLDER, false);
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (quint32)innoCompressionToHandleMethod(dataEntry.compression));
        if (dataEntry.bCallInstructionOptimized) record.mapProperties.insert(FPART_PROP_HANDLEMETHOD2, (quint32)HANDLE_METHOD_BCJ);

        if (!dataEntry.baChecksum.isEmpty()) {
            if ((dataEntry.checksumType == INNO_CHECKSUM_ADLER32) || (dataEntry.checksumType == INNO_CHECKSUM_CRC32)) {
                if (dataEntry.baChecksum.size() != 4) return false;
                const quint32 nChecksum = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(dataEntry.baChecksum.constData()));
                record.mapProperties.insert(FPART_PROP_RESULTCRC, nChecksum);
                record.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                            dataEntry.checksumType == INNO_CHECKSUM_ADLER32 ? CRC_TYPE_ADLER32 : CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
            } else {
                QString sChecksumType;
                if (dataEntry.checksumType == INNO_CHECKSUM_MD5) sChecksumType = QStringLiteral("MD5");
                else if (dataEntry.checksumType == INNO_CHECKSUM_SHA1) sChecksumType = QStringLiteral("SHA1");
                else if (dataEntry.checksumType == INNO_CHECKSUM_SHA256) sChecksumType = QStringLiteral("SHA256");
                else return false;
                record.mapProperties.insert(FPART_PROP_CHECKSUM, QString::fromLatin1(dataEntry.baChecksum.toHex()));
                record.mapProperties.insert(FPART_PROP_CHECKSUMTYPE, sChecksumType);
            }
        }

        listRecords.append(record);
        listRecordDataEntryIndexes.append(nLocIdx);
    }

    if (listRecords.isEmpty() || (listRecords.size() != listRecordDataEntryIndexes.size())) return false;
    pContext->listAllRecords = listRecords;
    pContext->listRecordDataEntryIndexes = listRecordDataEntryIndexes;
    return true;
}

bool XInnoSetup::_parseRealInnoSetup(UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext) return false;
    // Find the offset table (rDlPtS magic)
    OFFSET_TABLE offsetTable = _findOffsetTable(pPdStruct);

    if (!offsetTable.bIsValid) {
        return false;
    }

    // header_offset and data_offset are absolute file offsets
    qint64 nSetup0Offset = offsetTable.nHeaderOffset;
    qint64 nDataStreamOffset = offsetTable.nDataOffset;
    qint64 nFileSize = getSize();

    if ((nSetup0Offset < 0) || (nSetup0Offset >= nFileSize) || (nDataStreamOffset < 0) || ((nDataStreamOffset > 0) && (nDataStreamOffset >= nFileSize))) {
        return false;
    }

    // 1.09/1.11 use an 8-byte ID, 1.2.x uses an exact 12-byte binary ID,
    // and later releases use the descriptive zero-padded 64-byte string.
    if (nSetup0Offset > nFileSize - 8) return false;
    qint32 nVersionIdSize = 8;
    QByteArray baVersionId = read_array(nSetup0Offset, nVersionIdSize);
    INNO_VERSION version = _parseVersionId(baVersionId);
    if (!version.bIsValid) {
        nVersionIdSize = 12;
        if (nSetup0Offset > nFileSize - nVersionIdSize) return false;
        baVersionId = read_array(nSetup0Offset, nVersionIdSize);
        version = _parseVersionId(baVersionId);
    }
    if (!version.bIsValid) {
        nVersionIdSize = 64;
        if (nSetup0Offset > nFileSize - nVersionIdSize) return false;
        baVersionId = read_array(nSetup0Offset, nVersionIdSize);
        version = _parseVersionId(baVersionId);
    }
    if (!version.bIsValid) return false;

    const bool bNative12SetupData = innoVersionValue(version) == innoVersionValue(1, 2, 10);
    const bool bWin16SetupData = version.bWin16;
    if (bNative12SetupData) {
        const quint32 nNewHeaderOffset = read_uint32(0x3c, false);
        if ((nFileSize < 0x40) || (read_uint16(0, false) != 0x5a4dU) || (nNewHeaderOffset < 0x40U) ||
            (quint64(nNewHeaderOffset) > quint64(nFileSize - (bWin16SetupData ? 2 : 4))))
            return false;
        if (bWin16SetupData) {
            if (read_uint16(nNewHeaderOffset, false) != 0x454eU) return false;
        } else if (read_uint32(nNewHeaderOffset, false) != 0x00004550U) {
            return false;
        }
    }

    // Read Block Stream 1 (file entries + setup header)
    qint64 nBlock1Offset = nSetup0Offset + nVersionIdSize;
    const bool bRev2 = (offsetTable.nRevision == 2);
    const quint64 nVersion = innoVersionValue(version);

    // Couple each fixed-pointer loader generation to the historical interval
    // in which the official loader emitted it. The setup-data whitelist still
    // rejects unknown/custom schemas inside these broad loader intervals.
    const bool bLegacyLoaderSchema = version.bLegacyShortId && !version.bWin16 &&
                                     (((version.nMajor == 1) && (version.nMinor == 9) && (offsetTable.nLoaderVersion == INNO_LOADER_LEGACY_VX)) ||
                                      ((version.nMajor == 1) && (version.nMinor == 11) && (offsetTable.nLoaderVersion == INNO_LOADER_LEGACY_EOF40)));
    const bool bHistoricalLoaderSchema =
        (offsetTable.nRevision == 0) &&
        (bLegacyLoaderSchema ||
         (!version.bLegacyShortId && (((offsetTable.nLoaderVersion == 2) && (nVersion >= innoVersionValue(1, 2, 10)) && (nVersion < innoVersionValue(4, 0, 0))) ||
                                      ((offsetTable.nLoaderVersion == 4) && (nVersion >= innoVersionValue(4, 0, 0)) && (nVersion < innoVersionValue(4, 0, 3))) ||
                                      ((offsetTable.nLoaderVersion == 5) && (nVersion >= innoVersionValue(4, 0, 3)) && (nVersion < innoVersionValue(4, 0, 10))) ||
                                      ((offsetTable.nLoaderVersion == 6) && (nVersion >= innoVersionValue(4, 0, 10)) && (nVersion < innoVersionValue(4, 1, 6))) ||
                                      ((offsetTable.nLoaderVersion == 7) && (nVersion >= innoVersionValue(4, 1, 6)) && (nVersion < innoVersionValue(5, 1, 5))))));
    if (((offsetTable.nRevision == 0) && !bHistoricalLoaderSchema) ||
        ((offsetTable.nRevision == 1) && ((nVersion < innoVersionValue(5, 1, 5)) || (nVersion >= innoVersionValue(6, 5, 0)))) ||
        ((offsetTable.nRevision == 2) && (nVersion < innoVersionValue(6, 5, 0))) || (offsetTable.nRevision > 2))
        return false;

    if (version.bLegacyShortId) return _parseLegacySetup0(pContext, offsetTable, version, nVersionIdSize, pPdStruct);
    if (version.bIsx) return _parseISXDataFallback(pContext, offsetTable, version, pPdStruct);

    QByteArray baBlock1Key;
    QByteArray baBlock1Nonce;
    QByteArray baBlock2Key;
    QByteArray baBlock2Nonce;

    if (bRev2) {
        // Inno Setup >= 6.5.0 (revision 2): the version string is followed by
        // CRC32 + TSetupEncryptionHeader (49 bytes). Only the first compressed
        // block carries this prefix; block stored-size fields are 64-bit.
        static const qint64 nRev2Prefix = 53;

        if (nBlock1Offset > nFileSize - nRev2Prefix) {
            return false;
        }

        const QByteArray baEncryptionPrefix = read_array(nBlock1Offset, nRev2Prefix);
        if (baEncryptionPrefix.size() != nRev2Prefix) return false;
        const quint32 nExpectedEncryptionCRC = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baEncryptionPrefix.constData()));
        const quint32 nActualEncryptionCRC = _getCRC32(baEncryptionPrefix.constData() + 4, nRev2Prefix - 4, 0xFFFFFFFF, _getCRC32Table_EDB88320()) ^ 0xFFFFFFFF;
        if (nExpectedEncryptionCRC != nActualEncryptionCRC) {
            setPdStructErrorString(pPdStruct, tr("Invalid Inno Setup encryption-header CRC"));
            return false;
        }

        pContext->nEncryptionUse = (quint8)baEncryptionPrefix.at(4);
        if (pContext->nEncryptionUse > 2) {
            setPdStructErrorString(pPdStruct, tr("Invalid Inno Setup encryption mode"));
            return false;
        }

        if (pContext->nEncryptionUse != 0) {
            if (pContext->bHasPasswordBytes) {
                setPdStructErrorString(pPdStruct, tr("Raw password bytes are supported only by legacy Inno Setup ARC4 encryption"));
                return false;
            }
            pContext->encryption = INNO_ENCRYPTION_XCHACHA20;
            const QByteArray baSalt = baEncryptionPrefix.mid(5, 16);
            const qint32 nIterations = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(baEncryptionPrefix.constData() + 21));
            pContext->baEncryptionBaseNonce = baEncryptionPrefix.mid(25, 24);
            if ((nIterations <= 0) || (nIterations > INNO_MAX_KDF_ITERATIONS)) {
                setPdStructErrorString(pPdStruct, tr("Unsupported Inno Setup key-derivation iteration count"));
                return false;
            }
            if (!_deriveEncryptionKey(pContext->sPassword, baSalt, nIterations, &pContext->baEncryptionKey, pPdStruct)) {
                setPdStructErrorString(pPdStruct, tr("Could not derive the Inno Setup encryption key"));
                return false;
            }

            QByteArray baPasswordTest(4, '\0');
            const QByteArray baTestNonce = innoCreateNonce(pContext->baEncryptionBaseNonce, 0, quint32(qint32(-1)));
            if (!_xChaCha20Crypt(&baPasswordTest, pContext->baEncryptionKey, baTestNonce)) return false;
            quint8 nDifference = 0;
            for (qint32 i = 0; i < 4; i++) {
                nDifference |= (quint8)baPasswordTest.at(i) ^ (quint8)baEncryptionPrefix.at(49 + i);
            }
            innoSecureClear(&baPasswordTest);
            if (nDifference != 0) {
                setPdStructErrorString(pPdStruct, tr("Wrong Inno Setup password"));
                return false;
            }

            if (pContext->nEncryptionUse == 2) {
                baBlock1Key = pContext->baEncryptionKey;
                baBlock2Key = pContext->baEncryptionKey;
                baBlock1Nonce = innoCreateNonce(pContext->baEncryptionBaseNonce, 0, quint32(qint32(-2)));
                baBlock2Nonce = innoCreateNonce(pContext->baEncryptionBaseNonce, 0, quint32(qint32(-3)));
            }
        }

        nBlock1Offset += nRev2Prefix;
    }

    const bool b64BitBlockSize = nVersion >= innoVersionValue(6, 7, 0);

    qint64 nBlock1Consumed = 0;
    QByteArray baBlock1 = _readBlockStream(nBlock1Offset, &nBlock1Consumed, pPdStruct, b64BitBlockSize, baBlock1Key, baBlock1Nonce, &version);
    innoSecureClear(&baBlock1Key);
    innoSecureClear(&baBlock1Nonce);

    if (baBlock1.isEmpty()) {
        innoSecureClear(&baBlock2Key);
        innoSecureClear(&baBlock2Nonce);
        return false;
    }

    // Read Block Stream 2 (data entries)
    qint64 nBlock2Offset = nBlock1Offset + nBlock1Consumed;
    qint64 nBlock2Consumed = 0;
    QByteArray baBlock2 = _readBlockStream(nBlock2Offset, &nBlock2Consumed, pPdStruct, b64BitBlockSize, baBlock2Key, baBlock2Nonce, &version);
    innoSecureClear(&baBlock2Key);
    innoSecureClear(&baBlock2Nonce);

    if (baBlock2.isEmpty()) {
        return false;
    }

    if (bNative12SetupData) {
        if ((nBlock2Consumed <= 0) || (nBlock2Offset != offsetTable.nTableOffset - nBlock2Consumed)) {
            return false;
        }
        if (nDataStreamOffset > 0) {
            const QByteArray baDiskId = bWin16SetupData ? QByteArray("idska16\x1a", 8) : QByteArray("idska32\x1a", 8);
            if ((nDataStreamOffset > nFileSize - 12) || (read_array(nDataStreamOffset, 8) != baDiskId)) {
                return false;
            }
            const quint32 nDeclaredDiskSize = read_uint32(nDataStreamOffset + 8, false);
            if ((nDeclaredDiskSize < 12U) || (offsetTable.nExeOffset <= nDataStreamOffset) ||
                (quint64(nDeclaredDiskSize) != quint64(offsetTable.nExeOffset - nDataStreamOffset)))
                return false;
        }
    }

    QList<INNO_VERSION> listVersionCandidates;
    listVersionCandidates.append(version);
    if (nVersion == innoVersionValue(1, 3, 21)) {
        INNO_VERSION alternate = version;
        alternate.nPatch = 24;
        listVersionCandidates.append(alternate);
    } else if (nVersion == innoVersionValue(2, 0, 1)) {
        INNO_VERSION alternate = version;
        alternate.nPatch = 2;
        listVersionCandidates.append(alternate);
    } else if (nVersion == innoVersionValue(3, 0, 3)) {
        INNO_VERSION alternate = version;
        alternate.nPatch = 4;
        listVersionCandidates.append(alternate);
    } else if (nVersion == innoVersionValue(4, 2, 3)) {
        INNO_VERSION alternate = version;
        alternate.nPatch = 4;
        listVersionCandidates.append(alternate);
    }

    HEADER_INFO headerInfo = {};
    QList<DATA_ENTRY> listDataEntries;
    QList<FILE_ENTRY> listFileEntries;
    quint32 nAnsiCodePage = 0;
    bool bParsedSchema = false;
    for (const INNO_VERSION &candidate : listVersionCandidates) {
        const HEADER_INFO candidateHeader = _parseHeaderInfo(baBlock1, candidate);
        if (!candidateHeader.bIsValid) continue;

        QList<DATA_ENTRY> candidateData = _parseDataEntries(baBlock2, candidate, bRev2, candidateHeader.nDataEntryCount, candidateHeader.compression);
        if (candidateData.isEmpty()) continue;

        quint32 nCandidateCodePage = 0;
        QList<FILE_ENTRY> candidateFiles = _parseFileEntries(baBlock1, candidateHeader, candidate, bRev2, pContext->nAnsiCodePageOverride, &nCandidateCodePage);
        if (candidateFiles.isEmpty()) continue;

        // Lagged setup IDs can name two different serialized layouts. A full
        // block walk must select exactly one; accepting the first would make
        // malformed overlap dependent on candidate order.
        if (bParsedSchema) return false;
        version = candidate;
        headerInfo = candidateHeader;
        listDataEntries = candidateData;
        listFileEntries = candidateFiles;
        nAnsiCodePage = nCandidateCodePage;
        bParsedSchema = true;
    }
    if (!bParsedSchema) return false;
    pContext->nAnsiCodePage = nAnsiCodePage;

    qint32 nNumDataEntries = listDataEntries.count();

    bool bHasEncryptedChunk = false;
    for (const DATA_ENTRY &entry : listDataEntries) {
        if (entry.bChunkEncrypted) bHasEncryptedChunk = true;
    }

    if (bHasEncryptedChunk && !bRev2) {
        pContext->encryption = headerInfo.encryption;
        if (headerInfo.encryption == INNO_ENCRYPTION_XCHACHA20) {
            if (pContext->bHasPasswordBytes) {
                setPdStructErrorString(pPdStruct, tr("Raw password bytes are supported only by legacy Inno Setup ARC4 encryption"));
                return false;
            }
            if ((headerInfo.baPasswordTest.size() != 4) || (headerInfo.baPasswordSalt.size() != 16) || (headerInfo.baEncryptionBaseNonce.size() != 24) ||
                (headerInfo.nKdfIterations <= 0) || (headerInfo.nKdfIterations > INNO_MAX_KDF_ITERATIONS) ||
                !_deriveEncryptionKey(pContext->sPassword, headerInfo.baPasswordSalt, headerInfo.nKdfIterations, &pContext->baEncryptionKey, pPdStruct)) {
                setPdStructErrorString(pPdStruct, tr("Invalid legacy Inno Setup encryption parameters"));
                return false;
            }
            pContext->baEncryptionBaseNonce = headerInfo.baEncryptionBaseNonce;
            QByteArray baPasswordTest(4, '\0');
            const QByteArray baTestNonce = innoCreateNonce(pContext->baEncryptionBaseNonce, 0, quint32(qint32(-1)));
            if (!_xChaCha20Crypt(&baPasswordTest, pContext->baEncryptionKey, baTestNonce)) return false;
            quint8 nDifference = 0;
            for (qint32 i = 0; i < 4; i++) {
                nDifference |= quint8(baPasswordTest.at(i)) ^ quint8(headerInfo.baPasswordTest.at(i));
            }
            innoSecureClear(&baPasswordTest);
            if (nDifference != 0) {
                setPdStructErrorString(pPdStruct, tr("Wrong Inno Setup password"));
                return false;
            }
            pContext->nEncryptionUse = 1;
        } else if ((headerInfo.encryption == INNO_ENCRYPTION_ARC4_MD5) || (headerInfo.encryption == INNO_ENCRYPTION_ARC4_SHA1)) {
            if ((headerInfo.baPasswordSalt.size() != 8) || (headerInfo.baPasswordTest.size() != (headerInfo.encryption == INNO_ENCRYPTION_ARC4_SHA1 ? 20 : 16))) {
                setPdStructErrorString(pPdStruct, tr("Invalid legacy Inno Setup password metadata"));
                return false;
            }
            QList<QByteArray> listPasswordCandidates;

            if (pContext->bHasPasswordBytes) {
                listPasswordCandidates.append(pContext->baPasswordBytes);
            } else if (version.bUnicode) {
                bool bEncoded = false;
                QByteArray baCandidate = _encodeLegacyPassword(pContext->sPassword, true, 0, &bEncoded);
                if (bEncoded) listPasswordCandidates.append(baCandidate);
                innoSecureClear(&baCandidate);
            } else {
                QList<quint32> listCandidateCodePages;
                if (pContext->nAnsiCodePageOverride != 0) {
                    listCandidateCodePages.append(pContext->nAnsiCodePageOverride);
                } else {
                    listCandidateCodePages.append(pContext->nAnsiCodePage);
                }
                static const quint32 anCandidateCodePages[] = {
                    1252U, 874U, 1250U, 1251U, 1253U, 1254U, 1255U, 1256U, 1257U, 1258U, 932U, 936U, 949U, 950U, 1361U, 65001U,
                };
                if (pContext->nAnsiCodePageOverride == 0) {
                    for (quint32 nCodePage : anCandidateCodePages) listCandidateCodePages.append(nCodePage);
                }
                for (quint32 nCodePage : listCandidateCodePages) {
                    if (nCodePage == 0) continue;
                    bool bEncoded = false;
                    QByteArray baCandidate = _encodeLegacyPassword(pContext->sPassword, false, nCodePage, &bEncoded);
                    if (bEncoded && !listPasswordCandidates.contains(baCandidate)) listPasswordCandidates.append(baCandidate);
                    innoSecureClear(&baCandidate);
                }
            }

            bool bHaveMatch = false;
            bool bAmbiguousMatch = false;
            QByteArray baMatchedPassword;
            for (const QByteArray &baCandidate : listPasswordCandidates) {
                QCryptographicHash passwordHash(headerInfo.encryption == INNO_ENCRYPTION_ARC4_SHA1 ? QCryptographicHash::Sha1 : QCryptographicHash::Md5);
                passwordHash.addData(QByteArray::fromRawData("PasswordCheckHash", 17));
                passwordHash.addData(headerInfo.baPasswordSalt);
                passwordHash.addData(baCandidate);
                QByteArray baActualTest = passwordHash.result();
                quint8 nDifference = quint8(baActualTest.size() != headerInfo.baPasswordTest.size());
                if (nDifference == 0) {
                    for (qint32 i = 0; i < baActualTest.size(); i++) {
                        nDifference |= quint8(baActualTest.at(i)) ^ quint8(headerInfo.baPasswordTest.at(i));
                    }
                }
                innoSecureClear(&baActualTest);
                if (nDifference == 0) {
                    if (bHaveMatch) {
                        bAmbiguousMatch = true;
                    } else {
                        baMatchedPassword = baCandidate;
                        bHaveMatch = true;
                    }
                }
            }
            for (QByteArray &baCandidate : listPasswordCandidates) {
                innoSecureClear(&baCandidate);
            }
            listPasswordCandidates.clear();

            if (!bHaveMatch || bAmbiguousMatch) {
                innoSecureClear(&baMatchedPassword);
                setPdStructErrorString(pPdStruct, bAmbiguousMatch ? tr("Ambiguous legacy Inno Setup password encoding") : tr("Wrong Inno Setup password"));
                return false;
            }
            pContext->baEncryptionKey = baMatchedPassword;
            innoSecureClear(&baMatchedPassword);
            pContext->nEncryptionUse = 1;
        } else {
            setPdStructErrorString(pPdStruct, tr("Unsupported Inno Setup encryption mode"));
            return false;
        }
    }

    pContext->listDataEntries = listDataEntries;
    pContext->listFileEntries = listFileEntries;
    pContext->nDataStreamOffset = nDataStreamOffset;
    pContext->version = version;
    pContext->sPassword.fill(QChar());
    pContext->sPassword.clear();
    innoSecureClear(&pContext->baPasswordBytes);
    pContext->bHasPasswordBytes = false;

    for (const DATA_ENTRY &entry : listDataEntries) {
        if (entry.bChunkEncrypted && ((pContext->nEncryptionUse == 0) || (pContext->encryption == INNO_ENCRYPTION_NONE))) {
            setPdStructErrorString(pPdStruct, tr("Invalid encrypted Inno Setup data entry"));
            return false;
        }
    }

    const bool bExternalSlices = (nDataStreamOffset == 0);
    if (bExternalSlices && !_prepareSliceSources(pContext, listDataEntries, pPdStruct)) return false;

    // Validate every exact StartOffset. Solid members share a start; unrelated chunks may
    // legitimately have equal compressed sizes and must remain distinct.
    QMap<QString, QString> mapChunks;

    for (const DATA_ENTRY &entry : listDataEntries) {
        if ((entry.nChunkCompressedSize < 0) || (entry.nChunkCompressedSize > (std::numeric_limits<qint32>::max)())) return false;
        const qint64 nEncryptionOverhead =
            entry.bChunkEncrypted && ((pContext->encryption == INNO_ENCRYPTION_ARC4_MD5) || (pContext->encryption == INNO_ENCRYPTION_ARC4_SHA1)) ? 8 : 0;

        if (!bExternalSlices) {
            if ((entry.nFirstSlice != 0) || (entry.nLastSlice != 0) || (nDataStreamOffset > nFileSize - 4) ||
                (entry.nChunkStartOffset > nFileSize - nDataStreamOffset - 4) || (nEncryptionOverhead > nFileSize - nDataStreamOffset - entry.nChunkStartOffset - 4) ||
                (entry.nChunkCompressedSize > nFileSize - nDataStreamOffset - entry.nChunkStartOffset - 4 - nEncryptionOverhead))
                return false;
            const qint64 nChunkOffset = nDataStreamOffset + entry.nChunkStartOffset;
            if (read_array(nChunkOffset, 4) != QByteArray("zlb\x1a", 4)) return false;
        } else {
            SLICE_SOURCE *pFirst = pContext->mapSliceSources.value(entry.nFirstSlice, nullptr);
            if (!pFirst || !pFirst->pFile || !pFirst->pValidator || (entry.nChunkStartOffset < pFirst->nDataOffset) ||
                (entry.nChunkStartOffset > pFirst->pFile->size() - 4) || !pFirst->pValidator->isUnpackSourceCurrent(&pFirst->validationState, pPdStruct) ||
                !pFirst->pFile->seek(entry.nChunkStartOffset) || (pFirst->pFile->read(4) != QByteArray("zlb\x1a", 4)) ||
                !pFirst->pValidator->isUnpackSourceCurrent(&pFirst->validationState, pPdStruct))
                return false;

            qint64 nRequired = entry.nChunkCompressedSize + 4 + nEncryptionOverhead;
            for (quint64 nSlice64 = entry.nFirstSlice; nSlice64 <= quint64(entry.nLastSlice); nSlice64++) {
                SLICE_SOURCE *pSlice = pContext->mapSliceSources.value((quint32)nSlice64, nullptr);
                if (!pSlice || !pSlice->pFile) return false;
                const qint64 nStart = (nSlice64 == entry.nFirstSlice) ? entry.nChunkStartOffset : pSlice->nDataOffset;
                if ((nStart < pSlice->nDataOffset) || (nStart > pSlice->pFile->size())) return false;
                const qint64 nAvailable = pSlice->pFile->size() - nStart;
                if (nSlice64 < entry.nLastSlice) {
                    // Intermediate slices must be consumed to EOF; otherwise
                    // LastSlice is not the actual last slice of this chunk.
                    if (nRequired <= nAvailable) return false;
                    nRequired -= nAvailable;
                } else if (nRequired > nAvailable) {
                    return false;
                }
            }
        }

        const QString sChunkKey = QString::number(entry.nFirstSlice) + QLatin1Char(':') + QString::number(entry.nChunkStartOffset);
        const QString sChunkSignature = QString::number(entry.nLastSlice) + QLatin1Char(':') + QString::number(entry.nChunkCompressedSize) + QLatin1Char(':') +
                                        QString::number((qint32)entry.compression) + QLatin1Char(':') + QString::number(entry.bChunkEncrypted ? 1 : 0);
        if (mapChunks.contains(sChunkKey)) {
            if (mapChunks.value(sChunkKey) != sChunkSignature) return false;
        } else {
            mapChunks.insert(sChunkKey, sChunkSignature);
        }
    }

    // Build ARCHIVERECORDs matching file entries to file-location entries.
    QList<ARCHIVERECORD> listRecords;
    QList<qint32> listRecordDataEntryIndexes;
    for (qint32 i = 0; i < listFileEntries.count(); i++) {
        const FILE_ENTRY &fileEntry = listFileEntries.at(i);
        qint32 nLocIdx = fileEntry.nLocationEntry;

        if ((nLocIdx < 0) || (nLocIdx >= nNumDataEntries)) {
            continue;
        }

        const DATA_ENTRY &dataEntry = listDataEntries.at(nLocIdx);

        ARCHIVERECORD record = {};
        if (!bExternalSlices && (dataEntry.nChunkStartOffset > (std::numeric_limits<qint64>::max)() - nDataStreamOffset)) return false;
        record.nStreamOffset = bExternalSlices ? dataEntry.nChunkStartOffset : nDataStreamOffset + dataEntry.nChunkStartOffset;
        record.nStreamSize = dataEntry.nChunkCompressedSize;
        // Preserve the native destination directory separately for the
        // advanced Path column while ORIGINALNAME keeps the complete path.
        _setDestinationProperties(&record, fileEntry.sDestName);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, dataEntry.nOriginalSize);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, dataEntry.nChunkCompressedSize);
        record.mapProperties.insert(FPART_PROP_STREAMOFFSET, dataEntry.nChunkSubOffset);
        record.mapProperties.insert(FPART_PROP_STREAMSIZE, dataEntry.nOriginalSize);
        record.mapProperties.insert(FPART_PROP_ISFOLDER, false);
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (quint32)innoCompressionToHandleMethod(dataEntry.compression));
        if (dataEntry.bCallInstructionOptimized) {
            record.mapProperties.insert(FPART_PROP_HANDLEMETHOD2, (quint32)HANDLE_METHOD_BCJ);
        }

        if (!dataEntry.baChecksum.isEmpty()) {
            if ((dataEntry.checksumType == INNO_CHECKSUM_ADLER32) || (dataEntry.checksumType == INNO_CHECKSUM_CRC32)) {
                if (dataEntry.baChecksum.size() != 4) return false;
                const quint32 nChecksum = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(dataEntry.baChecksum.constData()));
                record.mapProperties.insert(FPART_PROP_RESULTCRC, nChecksum);
                record.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                            dataEntry.checksumType == INNO_CHECKSUM_ADLER32 ? CRC_TYPE_ADLER32 : CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
            } else {
                QString sChecksumType;
                if (dataEntry.checksumType == INNO_CHECKSUM_MD5) sChecksumType = QStringLiteral("MD5");
                else if (dataEntry.checksumType == INNO_CHECKSUM_SHA1) sChecksumType = QStringLiteral("SHA1");
                else if (dataEntry.checksumType == INNO_CHECKSUM_SHA256) sChecksumType = QStringLiteral("SHA256");
                else return false;

                record.mapProperties.insert(FPART_PROP_CHECKSUM, QString::fromLatin1(dataEntry.baChecksum.toHex()));
                record.mapProperties.insert(FPART_PROP_CHECKSUMTYPE, sChecksumType);
            }
        }

        listRecords.append(record);
        listRecordDataEntryIndexes.append(nLocIdx);
    }

    if (listRecords.isEmpty() || (listRecords.size() != listRecordDataEntryIndexes.size())) return false;
    pContext->listAllRecords = listRecords;
    pContext->listRecordDataEntryIndexes = listRecordDataEntryIndexes;
    return true;
}

bool XInnoSetup::_prepareSliceSources(UNPACK_CONTEXT *pContext, const QList<DATA_ENTRY> &listDataEntries, PDSTRUCT *pPdStruct)
{
    if (!pContext || !pContext->mapSliceSources.isEmpty() || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    QFile *pOuterFile = dynamic_cast<QFile *>(getDevice());
    if (!pOuterFile || pOuterFile->fileName().isEmpty()) {
        setPdStructErrorString(pPdStruct, tr("External Inno Setup slices require a file-backed installer"));
        return false;
    }

    QSet<quint32> setRequiredSlices;
    for (const DATA_ENTRY &entry : listDataEntries) {
        const quint64 nCount = quint64(entry.nLastSlice) - entry.nFirstSlice + 1;
        if ((nCount == 0) || (nCount > INNO_MAX_EXTERNAL_SLICES) || (quint64(setRequiredSlices.size()) + nCount > INNO_MAX_EXTERNAL_SLICES * 2ULL)) {
            setPdStructErrorString(pPdStruct, tr("Inno Setup uses too many external slices"));
            return false;
        }
        for (quint64 nSlice = entry.nFirstSlice; nSlice <= entry.nLastSlice; nSlice++) {
            setRequiredSlices.insert((quint32)nSlice);
            if (setRequiredSlices.size() > INNO_MAX_EXTERNAL_SLICES) {
                setPdStructErrorString(pPdStruct, tr("Inno Setup uses too many external slices"));
                return false;
            }
        }
    }
    if (setRequiredSlices.isEmpty()) return false;

    QList<quint32> listRequiredSlices = setRequiredSlices.values();
    std::sort(listRequiredSlices.begin(), listRequiredSlices.end());
    const QFileInfo outerInfo(pOuterFile->fileName());
    const QDir sourceDirectory = outerInfo.absoluteDir();
    const QString sPrefix = outerInfo.completeBaseName();
    if (sPrefix.isEmpty()) return false;

    QMap<quint32, QString> mapSelectedPaths;
    const quint64 nVersion = innoVersionValue(pContext->version);
    if (nVersion < innoVersionValue(2, 0, 0)) {
        for (quint32 nSlice : listRequiredSlices) {
            const QString sName = QStringLiteral("%1.%2").arg(sPrefix).arg(nSlice + 1);
            const QFileInfo sliceInfo(sourceDirectory.filePath(sName));
            if (!sliceInfo.exists() || !sliceInfo.isFile()) {
                setPdStructErrorString(pPdStruct, tr("Missing Inno Setup external slice"));
                return false;
            }
            QString sPath = sliceInfo.canonicalFilePath();
            if (sPath.isEmpty()) sPath = sliceInfo.absoluteFilePath();
            mapSelectedPaths.insert(nSlice, QDir::cleanPath(sPath));
        }
    } else {
        for (qint32 nSlicesPerDisk = 1; nSlicesPerDisk <= 26; nSlicesPerDisk++) {
            QMap<quint32, QString> mapCandidatePaths;
            bool bCandidate = true;
            for (quint32 nSlice : listRequiredSlices) {
                const quint32 nMajor = nSlice / quint32(nSlicesPerDisk) + 1;
                const quint32 nMinor = nSlice % quint32(nSlicesPerDisk);
                const QString sName = (nSlicesPerDisk == 1) ? QStringLiteral("%1-%2.bin").arg(sPrefix).arg(nMajor)
                                                            : QStringLiteral("%1-%2%3.bin").arg(sPrefix).arg(nMajor).arg(QChar('a' + nMinor));
                const QFileInfo sliceInfo(sourceDirectory.filePath(sName));
                if (!sliceInfo.exists() || !sliceInfo.isFile()) {
                    bCandidate = false;
                    break;
                }
                QString sPath = sliceInfo.canonicalFilePath();
                if (sPath.isEmpty()) sPath = sliceInfo.absoluteFilePath();
                mapCandidatePaths.insert(nSlice, QDir::cleanPath(sPath));
            }
            if (!bCandidate) continue;
            if (mapSelectedPaths.isEmpty()) {
                mapSelectedPaths = mapCandidatePaths;
            } else if (mapSelectedPaths != mapCandidatePaths) {
                setPdStructErrorString(pPdStruct, tr("Ambiguous Inno Setup external-slice naming"));
                return false;
            }
        }
    }

    if (mapSelectedPaths.size() != listRequiredSlices.size()) {
        setPdStructErrorString(pPdStruct, tr("Missing Inno Setup external slice"));
        return false;
    }

    const bool bWideSliceHeader = nVersion >= innoVersionValue(6, 5, 2);
    const qint32 nSliceHeaderSize = bWideSliceHeader ? 16 : 12;
    const QByteArray baSliceId =
        bWideSliceHeader ? QByteArray("idskb32\x1a", 8) : (pContext->version.bWin16 ? QByteArray("idska16\x1a", 8) : QByteArray("idska32\x1a", 8));
    for (quint32 nSlice : listRequiredSlices) {
        SLICE_SOURCE *pSlice = new SLICE_SOURCE();
        pSlice->nSlice = nSlice;
        pSlice->nDataOffset = nSliceHeaderSize;
        pSlice->pFile = new QFile(mapSelectedPaths.value(nSlice));
        pSlice->pValidator = nullptr;
        pSlice->validationState = UNPACK_STATE();

        bool bOk = pSlice->pFile->open(QIODevice::ReadOnly);
        QByteArray baHeader;
        if (bOk) baHeader = pSlice->pFile->read(nSliceHeaderSize);
        if (bOk) {
            bOk = (baHeader.size() == nSliceHeaderSize) && baHeader.startsWith(baSliceId);
        }
        if (bOk) {
            const qint64 nDeclaredSize = bWideSliceHeader ? qFromLittleEndian<qint64>(reinterpret_cast<const uchar *>(baHeader.constData() + 8))
                                                          : qint64(qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baHeader.constData() + 8)));
            bOk = (nDeclaredSize >= nSliceHeaderSize) && (nDeclaredSize == pSlice->pFile->size()) && pSlice->pFile->seek(nSliceHeaderSize);
        }
        if (bOk) {
            pSlice->pValidator = new XArchive(pSlice->pFile);
            bOk = pSlice->pValidator->bindUnpackSource(&pSlice->validationState, pPdStruct) &&
                  pSlice->pValidator->validateAndFinalizeUnpackSource(&pSlice->validationState, pPdStruct);
        }

        if (!bOk) {
            if (pSlice->pValidator) {
                pSlice->pValidator->releaseUnpackSource(&pSlice->validationState);
                delete pSlice->pValidator;
            }
            pSlice->pFile->close();
            delete pSlice->pFile;
            delete pSlice;
            setPdStructErrorString(pPdStruct, tr("Invalid Inno Setup external-slice header"));
            return false;
        }
        pContext->mapSliceSources.insert(nSlice, pSlice);
    }

    return true;
}

bool XInnoSetup::_areSliceSourcesCurrent(const UNPACK_CONTEXT *pContext, quint32 nFirstSlice, quint32 nLastSlice, PDSTRUCT *pPdStruct) const
{
    if (!pContext || (nFirstSlice > nLastSlice) || (quint64(nLastSlice) - nFirstSlice + 1 > INNO_MAX_EXTERNAL_SLICES)) return false;
    for (quint64 nSlice = nFirstSlice; nSlice <= nLastSlice; nSlice++) {
        SLICE_SOURCE *pSource = pContext->mapSliceSources.value((quint32)nSlice, nullptr);
        if (!pSource || !pSource->pFile || !pSource->pValidator || !pSource->pValidator->isUnpackSourceCurrent(&pSource->validationState, pPdStruct)) return false;
    }
    return true;
}

bool XInnoSetup::_readDataChunk(UNPACK_CONTEXT *pContext, const DATA_ENTRY &entry, QByteArray *pCompressedData, PDSTRUCT *pPdStruct)
{
    if (pCompressedData) pCompressedData->clear();
    if (!pContext || !pCompressedData || (entry.nChunkStartOffset < 0) || (entry.nChunkCompressedSize < 0) ||
        (entry.nChunkCompressedSize > (std::numeric_limits<qint32>::max)()) || !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    const bool bLegacyArcFour = entry.bChunkEncrypted && ((pContext->encryption == INNO_ENCRYPTION_ARC4_MD5) || (pContext->encryption == INNO_ENCRYPTION_ARC4_SHA1));
    const qint64 nEncryptionOverhead = bLegacyArcFour ? 8 : 0;
    QByteArray baLegacySalt;

    if (pContext->nDataStreamOffset > 0) {
        if ((entry.nFirstSlice != 0) || (entry.nLastSlice != 0) || (entry.nChunkStartOffset > (std::numeric_limits<qint64>::max)() - pContext->nDataStreamOffset))
            return false;
        const qint64 nChunkOffset = pContext->nDataStreamOffset + entry.nChunkStartOffset;
        if ((nChunkOffset > getSize() - 4) || (nEncryptionOverhead > getSize() - nChunkOffset - 4) ||
            (entry.nChunkCompressedSize > getSize() - nChunkOffset - 4 - nEncryptionOverhead) || (read_array(nChunkOffset, 4) != QByteArray("zlb\x1a", 4)))
            return false;
        if (bLegacyArcFour) {
            baLegacySalt = read_array(nChunkOffset + 4, 8);
            if (baLegacySalt.size() != 8) return false;
        }
        *pCompressedData = read_array(nChunkOffset + 4 + nEncryptionOverhead, entry.nChunkCompressedSize);
        if (pCompressedData->size() != entry.nChunkCompressedSize) return false;
    } else {
        if (!_areSliceSourcesCurrent(pContext, entry.nFirstSlice, entry.nLastSlice, pPdStruct)) return false;
        pCompressedData->resize((qint32)entry.nChunkCompressedSize);
        qint64 nWritten = 0;
        quint32 nCurrentSlice = entry.nFirstSlice;
        SLICE_SOURCE *pSlice = pContext->mapSliceSources.value(nCurrentSlice, nullptr);
        if (!pSlice || !pSlice->pFile || !pSlice->pFile->seek(entry.nChunkStartOffset) || (pSlice->pFile->read(4) != QByteArray("zlb\x1a", 4))) {
            pCompressedData->clear();
            return false;
        }
        if (bLegacyArcFour) {
            baLegacySalt = pSlice->pFile->read(8);
            if (baLegacySalt.size() != 8) {
                pCompressedData->clear();
                return false;
            }
        }

        while (nWritten < entry.nChunkCompressedSize) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                pCompressedData->clear();
                return false;
            }
            const qint64 nWanted = entry.nChunkCompressedSize - nWritten;
            const qint64 nRead = pSlice->pFile->read(pCompressedData->data() + nWritten, nWanted);
            if (nRead < 0) {
                pCompressedData->clear();
                return false;
            }
            if (nRead > 0) {
                nWritten += nRead;
                continue;
            }
            if (nCurrentSlice >= entry.nLastSlice) {
                pCompressedData->clear();
                return false;
            }
            nCurrentSlice++;
            pSlice = pContext->mapSliceSources.value(nCurrentSlice, nullptr);
            if (!pSlice || !pSlice->pFile || !pSlice->pFile->seek(pSlice->nDataOffset)) {
                pCompressedData->clear();
                return false;
            }
        }
        if ((nCurrentSlice != entry.nLastSlice) || !_areSliceSourcesCurrent(pContext, entry.nFirstSlice, entry.nLastSlice, pPdStruct)) {
            pCompressedData->clear();
            return false;
        }
    }

    if (entry.bChunkEncrypted) {
        if (pContext->nEncryptionUse == 0) {
            pCompressedData->clear();
            return false;
        }
        if (bLegacyArcFour) {
            if (!_arcFourCrypt(pCompressedData, pContext->baEncryptionKey, baLegacySalt, pContext->encryption)) {
                pCompressedData->clear();
                return false;
            }
        } else if (pContext->encryption == INNO_ENCRYPTION_XCHACHA20) {
            if ((pContext->baEncryptionKey.size() != 32) || (pContext->baEncryptionBaseNonce.size() != 24)) {
                pCompressedData->clear();
                return false;
            }
            const QByteArray baNonce = innoCreateNonce(pContext->baEncryptionBaseNonce, quint64(entry.nChunkStartOffset), entry.nFirstSlice);
            if (!_xChaCha20Crypt(pCompressedData, pContext->baEncryptionKey, baNonce)) {
                pCompressedData->clear();
                return false;
            }
        } else {
            pCompressedData->clear();
            return false;
        }
    }
    return true;
}

QByteArray XInnoSetup::_decompressDataChunk(UNPACK_CONTEXT *pContext, const DATA_ENTRY &entry, qint64 nOutputLimit, const QMap<UNPACK_PROP, QVariant> &mapProperties,
                                            PDSTRUCT *pPdStruct)
{
    if (!pContext || (entry.nChunkCompressedSize < 0) || (entry.nChunkCompressedSize > (std::numeric_limits<qint32>::max)()) || (nOutputLimit <= 0) ||
        (nOutputLimit > (std::numeric_limits<qint32>::max)()))
        return QByteArray();

    UNPACK_MEMORY_RESERVATION inputReservation;
    if (!inputReservation.acquire(mapProperties, entry.nChunkCompressedSize)) {
        setPdStructErrorString(pPdStruct, tr("Inno Setup compressed chunk exceeds the shared memory limit"));
        return QByteArray();
    }
    QByteArray baCompressed;
    if (!_readDataChunk(pContext, entry, &baCompressed, pPdStruct) || (baCompressed.size() != entry.nChunkCompressedSize)) return QByteArray();

    qint64 nInputOffset = 0;
    qint64 nInputSize = entry.nChunkCompressedSize;
    QByteArray baProperty;

    if (entry.compression == INNO_COMPRESSION_LZMA1) {
        if (nInputSize < 5) return QByteArray();
        baProperty = baCompressed.left(5);
        if (baProperty.size() != 5) return QByteArray();
        nInputOffset += 5;
        nInputSize -= 5;
    } else if (entry.compression == INNO_COMPRESSION_LZMA2) {
        if (nInputSize < 1) return QByteArray();
        baProperty = baCompressed.left(1);
        if (baProperty.size() != 1) return QByteArray();
        nInputOffset++;
        nInputSize--;
    } else if ((entry.compression != INNO_COMPRESSION_STORE) && (entry.compression != INNO_COMPRESSION_ZLIB) && (entry.compression != INNO_COMPRESSION_BZIP2)) {
        return QByteArray();
    }

    QBuffer bufInput(&baCompressed);
    QBuffer bufOutput;
    // Zlib validation rereads the produced bytes to verify its Adler-32 trailer.
    if (!bufInput.open(QIODevice::ReadOnly) || !bufOutput.open(QIODevice::ReadWrite)) return QByteArray();

    XBinary::DATAPROCESS_STATE decompressState = {};
    decompressState.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (quint32)innoCompressionToHandleMethod(entry.compression));
    decompressState.mapUnpackProperties = mapProperties;
    decompressState.pDeviceInput = &bufInput;
    decompressState.pDeviceOutput = &bufOutput;
    decompressState.nInputOffset = nInputOffset;
    decompressState.nInputLimit = nInputSize;
    decompressState.nProcessedOffset = 0;
    decompressState.nProcessedLimit = nOutputLimit;

    bool bOk = false;

    switch (entry.compression) {
        case INNO_COMPRESSION_STORE:
            bOk = XStoreDecoder::decompress(&decompressState, pPdStruct) && (decompressState.nCountInput == nInputSize) && (decompressState.nCountOutput == nInputSize);
            break;
        case INNO_COMPRESSION_ZLIB: bOk = XDeflateDecoder::decompress_zlib(&decompressState, pPdStruct); break;
        case INNO_COMPRESSION_BZIP2: bOk = XBZIP2Decoder::decompress(&decompressState, pPdStruct); break;
        case INNO_COMPRESSION_LZMA1: bOk = XLZMADecoder::decompress(&decompressState, baProperty, pPdStruct); break;
        case INNO_COMPRESSION_LZMA2: bOk = XLZMADecoder::decompressLZMA2(&decompressState, baProperty, pPdStruct); break;
        default: break;
    }

    bufInput.close();
    bufOutput.close();
    if (!bOk || (decompressState.nCountOutput != nOutputLimit) || (bufOutput.data().size() != nOutputLimit)) return QByteArray();
    return bufOutput.data();
}
