/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xwiiwad.h"

#include "Algos/xaesdecoder.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QSet>
#include <QtEndian>

#include <cstring>
#include <limits>
#include <new>

namespace {
const qint64 WIIWAD_HEADER_SIZE = 0x20;
const qint64 WIIWAD_HEADER_PADDED_SIZE = 0x40;
const qint64 WIIWAD_SECTION_ALIGN = 0x40;
const qint64 WIIWAD_AES_BLOCK = 0x10;
// Header 0x40 + ticket 0x2A4 at 0x40 + TMD 0x1E4 at align64(0x2E4) = 0x300:
// the smallest WAD with no certificates and no contents.
const qint64 WIIWAD_MIN_FILE_SIZE = 0x4E4;
const qint64 WIIWAD_TICKET_MIN_SIZE = 0x2A4;
const qint64 WIIWAD_TMD_HEADER_SIZE = 0x1E4;
const qint64 WIIWAD_TMD_RECORD_SIZE = 0x24;
const quint32 WIIWAD_SIG_RSA4096 = 0x00010000U;
const quint32 WIIWAD_SIG_RSA2048 = 0x00010001U;
const quint32 WIIWAD_SIG_ECC = 0x00010002U;
// The content index is a u16 IV seed and the largest retail titles have well
// under 100 contents; 1024 keeps the TMD read inside the section cap.
const qint64 WIIWAD_MAX_CONTENTS = 1024;
// Whole-buffer caps for the metadata sections read into memory.  Real
// values: 0xA00 certs, 0 CRL, 0x2A4 ticket, 0x1E4 + 0x24n (+0xA00) TMD,
// 0x40 footer.
const qint64 WIIWAD_MAX_SECTION_SIZE = Q_INT64_C(1) * 1024 * 1024;
// Wii NAND capacity: no installable content can exceed it.  Ciphertext and
// plaintext have the same length, so there is no expansion to cap; this only
// bounds the staging buffer.
const qint64 WIIWAD_MAX_CONTENT_SIZE = Q_INT64_C(512) * 1024 * 1024;
const qint64 WIIWAD_MAX_CERTS_WALKED = 8;
const qint64 WIIWAD_DECRYPT_CHUNK = 0x10000;
const qint64 WIIWAD_COMMON_KEY_SIZE = 16;
const qint64 WIIWAD_TITLE_ID_SIZE = 8;
const qint64 WIIWAD_SHA1_SIZE = 20;
const qint64 WIIWAD_NAME_FIELD_SIZE = 0x40;
const qint64 WIIWAD_FOOTER_STAMP_SIZE = 16;  // 6-byte tag + 10 decimal digits

// Ticket (v0) field offsets.
const qint64 WIIWAD_TIK_ISSUER = 0x140;
const qint64 WIIWAD_TIK_VERSION = 0x1BC;
const qint64 WIIWAD_TIK_TITLEKEY = 0x1BF;
const qint64 WIIWAD_TIK_TICKETID = 0x1D0;
const qint64 WIIWAD_TIK_CONSOLEID = 0x1D8;
const qint64 WIIWAD_TIK_TITLEID = 0x1DC;
const qint64 WIIWAD_TIK_TITLEVERSION = 0x1E6;
const qint64 WIIWAD_TIK_KEYINDEX = 0x1F1;

// TMD field offsets.
const qint64 WIIWAD_TMD_ISSUER = 0x140;
const qint64 WIIWAD_TMD_VERSION = 0x180;
const qint64 WIIWAD_TMD_ISVWII = 0x183;
const qint64 WIIWAD_TMD_SYSVERSION = 0x184;
const qint64 WIIWAD_TMD_TITLEID = 0x18C;
const qint64 WIIWAD_TMD_GROUPID = 0x198;
const qint64 WIIWAD_TMD_REGION = 0x19C;
const qint64 WIIWAD_TMD_ACCESSRIGHTS = 0x1D8;
const qint64 WIIWAD_TMD_TITLEVERSION = 0x1DC;
const qint64 WIIWAD_TMD_NUMCONTENTS = 0x1DE;
const qint64 WIIWAD_TMD_BOOTINDEX = 0x1E0;

// Certificate layout relative to the signature length x.
const qint64 WIIWAD_CERT_ISSUER = 0x40;
const qint64 WIIWAD_CERT_KEYTYPE = 0x80;
const qint64 WIIWAD_CERT_SUBJECT = 0x84;
const qint64 WIIWAD_CERT_KEY = 0xC8;

bool wiiWadRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

bool wiiWadCheckedAdd(qint64 nLeft, qint64 nRight, qint64 *pnResult)
{
    if (!pnResult || (nLeft < 0) || (nRight < 0) ||
        (nRight > ((std::numeric_limits<qint64>::max)() - nLeft))) {
        return false;
    }
    *pnResult = nLeft + nRight;
    return true;
}

bool wiiWadCheckedMultiply(qint64 nLeft, qint64 nRight, qint64 *pnResult)
{
    if (!pnResult || (nLeft < 0) || (nRight < 0)) return false;
    if ((nLeft != 0) &&
        (nRight > ((std::numeric_limits<qint64>::max)() / nLeft))) {
        return false;
    }
    *pnResult = nLeft * nRight;
    return true;
}

bool wiiWadAlignUp(qint64 nValue, qint64 nAlignment, qint64 *pnResult)
{
    if (!pnResult || (nValue < 0) || (nAlignment <= 0)) return false;
    const qint64 nRemainder = nValue % nAlignment;
    if (nRemainder == 0) {
        *pnResult = nValue;
        return true;
    }
    return wiiWadCheckedAdd(nValue, nAlignment - nRemainder, pnResult);
}

// Places one section at the running cursor: the section must lie entirely
// inside the device, and the cursor moves to the next 0x40 boundary behind it.
// A zero-length section (no certs, no CRL, no content data) never reads a
// byte, so it may sit at a cursor at or beyond the device end: the minimal
// WAD (header + ticket + TMD, no contents) ends at the unaligned TMD end and
// has its empty data section "placed" behind it, exactly like the empty
// footer.  Every non-empty section that follows still goes through
// wiiWadRangeWithin, so nothing past the device end can be read.
bool wiiWadPlaceSection(qint64 nSourceSize, qint64 nSize, qint64 *pnCursor,
                        qint64 *pnOffset)
{
    if (!pnCursor || !pnOffset) return false;
    if (nSize == 0) {
        if (*pnCursor < 0) return false;
        *pnOffset = *pnCursor;  // the cursor is already 0x40-aligned
        return true;
    }
    if (!wiiWadRangeWithin(nSourceSize, *pnCursor, nSize)) return false;
    *pnOffset = *pnCursor;
    qint64 nEnd = 0;
    if (!wiiWadCheckedAdd(*pnCursor, nSize, &nEnd)) return false;
    return wiiWadAlignUp(nEnd, WIIWAD_SECTION_ALIGN, pnCursor);
}

quint16 wiiWadReadBE16(const uchar *pData)
{
    return qFromBigEndian<quint16>(pData);
}

quint32 wiiWadReadBE32(const uchar *pData)
{
    return qFromBigEndian<quint32>(pData);
}

quint64 wiiWadReadBE64(const uchar *pData)
{
    return qFromBigEndian<quint64>(pData);
}

// NUL-padded printable ASCII field; reads the FULL field size and stops at
// the first NUL or non-printable byte.
QString wiiWadFixedString(const uchar *pData, qint64 nFieldSize)
{
    QByteArray baResult;
    for (qint64 i = 0; i < nFieldSize; ++i) {
        const uchar nValue = pData[i];
        if ((nValue < 0x20U) || (nValue > 0x7EU)) break;
        baResult.append(static_cast<char>(nValue));
    }
    return QString::fromLatin1(baResult);
}

bool wiiWadIsHexString(const QString &sValue)
{
    const qint32 nSize = sValue.size();
    for (qint32 i = 0; i < nSize; ++i) {
        const QChar cChar = sValue.at(i);
        const bool bHex = ((cChar >= QLatin1Char('0')) && (cChar <= QLatin1Char('9'))) ||
                          ((cChar >= QLatin1Char('a')) && (cChar <= QLatin1Char('f'))) ||
                          ((cChar >= QLatin1Char('A')) && (cChar <= QLatin1Char('F')));
        if (!bHex) return false;
    }
    return true;
}

QString wiiWadKeyIndexName(quint8 nIndex)
{
    if (nIndex == 0) return QStringLiteral("Common");
    if (nIndex == 1) return QStringLiteral("Korean");
    if (nIndex == 2) return QStringLiteral("vWii");
    // Fakesigned homebrew WADs leave garbage here; every reference reader
    // falls back to the standard key, so this is only reported.
    return QStringLiteral("unknown, standard key assumed");
}

QString wiiWadRegionName(quint16 nRegion)
{
    if (nRegion == 0) return QStringLiteral("JPN");
    if (nRegion == 1) return QStringLiteral("USA");
    if (nRegion == 2) return QStringLiteral("EUR");
    if (nRegion == 3) return QStringLiteral("region-free");
    if (nRegion == 4) return QStringLiteral("KOR");
    return QStringLiteral("region %1").arg(nRegion);
}

QString wiiWadTitleClassName(quint32 nHigh)
{
    if (nHigh == 0x00000001U) return QStringLiteral("System");
    if (nHigh == 0x00010000U) return QStringLiteral("Disc game");
    if (nHigh == 0x00010001U) return QStringLiteral("Channel");
    if (nHigh == 0x00010002U) return QStringLiteral("System channel");
    if (nHigh == 0x00010004U) return QStringLiteral("Game channel");
    if (nHigh == 0x00010005U) return QStringLiteral("DLC");
    if (nHigh == 0x00010008U) return QStringLiteral("Hidden channel");
    return QStringLiteral("type 0x%1").arg(nHigh, 8, 16, QLatin1Char('0'));
}

QString wiiWadContentTypeName(quint16 nType)
{
    if (nType == 0x0001U) return QStringLiteral("Normal");
    if (nType == 0x4001U) return QStringLiteral("DLC");
    if (nType == 0x8001U) return QStringLiteral("Shared");
    return QStringLiteral("0x%1").arg(nType, 4, 16, QLatin1Char('0'));
}

// "TmStmp" (libWiiSharp) or "CMiiUT" (Nintendo SDK) followed by ten ASCII
// decimal digits of seconds since 1970.
bool wiiWadParseFooterStamp(const QByteArray &baFooter, QDateTime *pdtResult)
{
    if (!pdtResult || (baFooter.size() < WIIWAD_FOOTER_STAMP_SIZE)) return false;
    const QByteArray baTag = baFooter.left(6);
    if ((baTag != QByteArray("TmStmp")) && (baTag != QByteArray("CMiiUT"))) {
        return false;
    }
    qint64 nSeconds = 0;
    for (qint32 i = 6; i < WIIWAD_FOOTER_STAMP_SIZE; ++i) {
        const char cChar = baFooter.at(i);
        if ((cChar < '0') || (cChar > '9')) return false;
        nSeconds = (nSeconds * 10) + (cChar - '0');
    }
    *pdtResult = QDateTime::fromSecsSinceEpoch(nSeconds, Qt::UTC);
    return pdtResult->isValid();
}

// Informational certificate walk (never part of the gate): collects
// "<issuer>-<subject>" of the recognised certificates in file order (spec
// 1.4), e.g. "Root-CA00000001", "Root-CA00000001-CP00000004".
QStringList wiiWadWalkCertificates(const QByteArray &baCerts)
{
    QStringList listResult;
    const qint64 nEnd = baCerts.size();
    const uchar *pData = reinterpret_cast<const uchar *>(baCerts.constData());
    qint64 nPos = 0;
    while ((listResult.size() < WIIWAD_MAX_CERTS_WALKED) &&
           ((nPos + WIIWAD_CERT_KEY) <= nEnd)) {
        const quint32 nSigType = wiiWadReadBE32(pData + nPos);
        qint64 nSigLength = 0;
        if (nSigType == WIIWAD_SIG_RSA4096) nSigLength = 0x200;
        else if (nSigType == WIIWAD_SIG_RSA2048) nSigLength = 0x100;
        else if (nSigType == WIIWAD_SIG_ECC) nSigLength = 0x3C;
        else break;
        if ((nPos + WIIWAD_CERT_KEY + nSigLength) > nEnd) break;

        const quint32 nKeyType = wiiWadReadBE32(pData + nPos + WIIWAD_CERT_KEYTYPE + nSigLength);
        qint64 nKeyLength = 0;
        if (nKeyType == 0) nKeyLength = 0x200 + 4 + 0x34;
        else if (nKeyType == 1) nKeyLength = 0x100 + 4 + 0x34;
        else if (nKeyType == 2) nKeyLength = 0x3C + 0x3C;
        else break;

        qint64 nCertSize = 0;
        if (!wiiWadAlignUp(WIIWAD_CERT_KEY + nSigLength + nKeyLength, WIIWAD_SECTION_ALIGN, &nCertSize)) break;
        if ((nPos + nCertSize) > nEnd) break;

        const QString sIssuer = wiiWadFixedString(pData + nPos + WIIWAD_CERT_ISSUER + nSigLength, WIIWAD_NAME_FIELD_SIZE);
        const QString sSubject = wiiWadFixedString(pData + nPos + WIIWAD_CERT_SUBJECT + nSigLength, WIIWAD_NAME_FIELD_SIZE);
        QString sName;
        if (sIssuer.isEmpty() && sSubject.isEmpty()) {
            sName = QStringLiteral("?");
        } else if (sIssuer.isEmpty()) {
            sName = sSubject;
        } else if (sSubject.isEmpty()) {
            sName = sIssuer;
        } else {
            sName = sIssuer + QLatin1Char('-') + sSubject;
        }
        listResult.append(sName);
        nPos += nCertSize;
    }
    return listResult;
}

void wiiWadWipe(QByteArray *pbaValue)
{
    if (!pbaValue) return;
    if (!pbaValue->isEmpty()) {
        memset(pbaValue->data(), 0, static_cast<size_t>(pbaValue->size()));
    }
    pbaValue->clear();
}

// Owns the private staging device for the lifetime of one unpackCurrent()
// call so every early return frees it.
struct WiiWadStageHolder {
    QIODevice *pDevice;

    explicit WiiWadStageHolder(QIODevice *pStage) : pDevice(pStage)
    {
    }
    ~WiiWadStageHolder()
    {
        XBinary::freeFileBuffer(&pDevice);
    }

private:
    WiiWadStageHolder(const WiiWadStageHolder &);
    WiiWadStageHolder &operator=(const WiiWadStageHolder &);
};
}  // namespace

XWiiWAD::XWiiWAD(QIODevice *pDevice) : XArchive(pDevice)
{
}

XWiiWAD::~XWiiWAD()
{
}

bool XWiiWAD::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XWiiWAD> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nSourceSize = guardedSource->size();
    if (context.nSourceSize < WIIWAD_MIN_FILE_SIZE) return false;

    // ---- header -------------------------------------------------------
    const QByteArray baHeader = read_array_process(0, WIIWAD_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != WIIWAD_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    if (wiiWadReadBE32(pHeader) != static_cast<quint32>(WIIWAD_HEADER_SIZE)) return false;
    if ((pHeader[4] == 'I') && (pHeader[5] == 's')) {
        context.bIsBoot2 = false;
    } else if ((pHeader[4] == 'i') && (pHeader[5] == 'b')) {
        context.bIsBoot2 = true;
    } else {
        return false;
    }
    context.nVersion = wiiWadReadBE16(pHeader + 6);
    // Version pinned to 0: the 8-byte magic libWiiPy compares; see the header.
    if (context.nVersion != 0) return false;

    context.nCertSize = static_cast<qint64>(wiiWadReadBE32(pHeader + 0x08));
    context.nCrlSize = static_cast<qint64>(wiiWadReadBE32(pHeader + 0x0C));
    context.nTicketSize = static_cast<qint64>(wiiWadReadBE32(pHeader + 0x10));
    context.nTmdSize = static_cast<qint64>(wiiWadReadBE32(pHeader + 0x14));
    context.nDataSize = static_cast<qint64>(wiiWadReadBE32(pHeader + 0x18));
    context.nFooterSize = static_cast<qint64>(wiiWadReadBE32(pHeader + 0x1C));

    if ((context.nCertSize > WIIWAD_MAX_SECTION_SIZE) ||
        (context.nCrlSize > WIIWAD_MAX_SECTION_SIZE) ||
        (context.nTicketSize > WIIWAD_MAX_SECTION_SIZE) ||
        (context.nTmdSize > WIIWAD_MAX_SECTION_SIZE) ||
        (context.nFooterSize > WIIWAD_MAX_SECTION_SIZE)) {
        return false;
    }
    if (context.nTicketSize < WIIWAD_TICKET_MIN_SIZE) return false;
    if (context.nTmdSize < WIIWAD_TMD_HEADER_SIZE) return false;

    // ---- section chain, every section 0x40-aligned and inside the device --
    qint64 nCursor = WIIWAD_HEADER_PADDED_SIZE;
    if (!wiiWadPlaceSection(context.nSourceSize, context.nCertSize, &nCursor, &context.nCertOffset) ||
        !wiiWadPlaceSection(context.nSourceSize, context.nCrlSize, &nCursor, &context.nCrlOffset) ||
        !wiiWadPlaceSection(context.nSourceSize, context.nTicketSize, &nCursor, &context.nTicketOffset) ||
        !wiiWadPlaceSection(context.nSourceSize, context.nTmdSize, &nCursor, &context.nTmdOffset) ||
        !wiiWadPlaceSection(context.nSourceSize, context.nDataSize, &nCursor, &context.nDataOffset)) {
        return false;
    }
    context.nFooterOffset = nCursor;
    qint64 nArchiveEnd = context.nFooterOffset;
    if (context.nFooterSize > 0) {
        qint64 nFooterEnd = 0;
        if (!wiiWadRangeWithin(context.nSourceSize, context.nFooterOffset, context.nFooterSize) ||
            !wiiWadCheckedAdd(context.nFooterOffset, context.nFooterSize, &nFooterEnd) ||
            !wiiWadAlignUp(nFooterEnd, WIIWAD_SECTION_ALIGN, &nArchiveEnd)) {
            return false;
        }
    }
    // The final padding may be missing; trailing data behind it is foreign.
    context.nArchiveEnd = qMin(nArchiveEnd, context.nSourceSize);

    // ---- ticket -------------------------------------------------------
    const QByteArray baTicket = read_array_process(context.nTicketOffset, WIIWAD_TICKET_MIN_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baTicket.size() != WIIWAD_TICKET_MIN_SIZE)) return false;
    const uchar *pTicket = reinterpret_cast<const uchar *>(baTicket.constData());
    if (wiiWadReadBE32(pTicket) != WIIWAD_SIG_RSA2048) return false;

    const QString sTicketIssuer = wiiWadFixedString(pTicket + WIIWAD_TIK_ISSUER, WIIWAD_NAME_FIELD_SIZE);
    const quint8 nTicketVersion = pTicket[WIIWAD_TIK_VERSION];
    context.baEncryptedTitleKey = baTicket.mid(static_cast<qint32>(WIIWAD_TIK_TITLEKEY), static_cast<qint32>(WIIWAD_COMMON_KEY_SIZE));
    const quint64 nTicketId = wiiWadReadBE64(pTicket + WIIWAD_TIK_TICKETID);
    const quint32 nConsoleId = wiiWadReadBE32(pTicket + WIIWAD_TIK_CONSOLEID);
    context.baTitleId = baTicket.mid(static_cast<qint32>(WIIWAD_TIK_TITLEID), static_cast<qint32>(WIIWAD_TITLE_ID_SIZE));
    const quint64 nTicketTitleId = wiiWadReadBE64(pTicket + WIIWAD_TIK_TITLEID);
    const quint16 nTicketTitleVersion = wiiWadReadBE16(pTicket + WIIWAD_TIK_TITLEVERSION);
    context.nCommonKeyIndex = pTicket[WIIWAD_TIK_KEYINDEX];
    context.bDevTicket = sTicketIssuer.contains(QStringLiteral("XS00000004")) ||
                         sTicketIssuer.contains(QStringLiteral("XS00000006"));
    if ((context.baEncryptedTitleKey.size() != WIIWAD_COMMON_KEY_SIZE) ||
        (context.baTitleId.size() != WIIWAD_TITLE_ID_SIZE)) {
        return false;
    }

    // ---- TMD ----------------------------------------------------------
    const QByteArray baTmdHeader = read_array_process(context.nTmdOffset, WIIWAD_TMD_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baTmdHeader.size() != WIIWAD_TMD_HEADER_SIZE)) return false;
    const uchar *pTmd = reinterpret_cast<const uchar *>(baTmdHeader.constData());
    if (wiiWadReadBE32(pTmd) != WIIWAD_SIG_RSA2048) return false;

    const qint64 nNumberOfContents = wiiWadReadBE16(pTmd + WIIWAD_TMD_NUMCONTENTS);
    if (nNumberOfContents > WIIWAD_MAX_CONTENTS) return false;
    qint64 nRecordsSize = 0;
    qint64 nTmdRequired = 0;
    if (!wiiWadCheckedMultiply(nNumberOfContents, WIIWAD_TMD_RECORD_SIZE, &nRecordsSize) ||
        !wiiWadCheckedAdd(WIIWAD_TMD_HEADER_SIZE, nRecordsSize, &nTmdRequired) ||
        (nTmdRequired > context.nTmdSize)) {
        return false;
    }

    const QString sTmdIssuer = wiiWadFixedString(pTmd + WIIWAD_TMD_ISSUER, WIIWAD_NAME_FIELD_SIZE);
    const quint8 nTmdVersion = pTmd[WIIWAD_TMD_VERSION];
    const bool bIsVWii = (pTmd[WIIWAD_TMD_ISVWII] != 0);
    const quint64 nSystemVersion = wiiWadReadBE64(pTmd + WIIWAD_TMD_SYSVERSION);
    const quint64 nTmdTitleId = wiiWadReadBE64(pTmd + WIIWAD_TMD_TITLEID);
    const quint16 nGroupId = wiiWadReadBE16(pTmd + WIIWAD_TMD_GROUPID);
    const quint16 nRegion = wiiWadReadBE16(pTmd + WIIWAD_TMD_REGION);
    const quint32 nAccessRights = wiiWadReadBE32(pTmd + WIIWAD_TMD_ACCESSRIGHTS);
    const quint16 nTitleVersion = wiiWadReadBE16(pTmd + WIIWAD_TMD_TITLEVERSION);
    const quint16 nBootIndex = wiiWadReadBE16(pTmd + WIIWAD_TMD_BOOTINDEX);

    QByteArray baRecords;
    if (nRecordsSize > 0) {
        baRecords = read_array_process(context.nTmdOffset + WIIWAD_TMD_HEADER_SIZE, nRecordsSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baRecords.size() != nRecordsSize)) return false;
    }

    // ---- members: metadata sections ----------------------------------
    const QString sTitleIdHex = QString::fromLatin1(context.baTitleId.toHex());
    QList<ENTRY> listEntries;
    if (context.nCertSize > 0) {
        ENTRY entry = {};
        entry.bIsContent = false;
        entry.nOffset = context.nCertOffset;
        entry.nSize = context.nCertSize;
        entry.nUncompressedSize = context.nCertSize;
        entry.sFileName = sTitleIdHex + QStringLiteral(".cert");
        listEntries.append(entry);
    }
    if (context.nCrlSize > 0) {
        ENTRY entry = {};
        entry.bIsContent = false;
        entry.nOffset = context.nCrlOffset;
        entry.nSize = context.nCrlSize;
        entry.nUncompressedSize = context.nCrlSize;
        entry.sFileName = sTitleIdHex + QStringLiteral(".crl");
        listEntries.append(entry);
    }
    {
        ENTRY entry = {};
        entry.bIsContent = false;
        entry.nOffset = context.nTicketOffset;
        entry.nSize = context.nTicketSize;
        entry.nUncompressedSize = context.nTicketSize;
        entry.sFileName = sTitleIdHex + QStringLiteral(".tik");
        listEntries.append(entry);
    }
    {
        ENTRY entry = {};
        entry.bIsContent = false;
        entry.nOffset = context.nTmdOffset;
        entry.nSize = context.nTmdSize;
        entry.nUncompressedSize = context.nTmdSize;
        entry.sFileName = sTitleIdHex + QStringLiteral(".tmd");
        listEntries.append(entry);
    }

    // ---- members: contents, TMD record order, 0x40 slots ---------------
    const uchar *pRecords = reinterpret_cast<const uchar *>(baRecords.constData());
    QSet<QString> setContentNames;
    qint64 nSlotOffset = context.nDataOffset;
    qint64 nRequiredDataSize = 0;
    for (qint64 i = 0; i < nNumberOfContents; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const uchar *pRecord = pRecords + (i * WIIWAD_TMD_RECORD_SIZE);
        const quint32 nContentId = wiiWadReadBE32(pRecord);
        const quint16 nContentIndex = wiiWadReadBE16(pRecord + 4);
        const quint16 nContentType = wiiWadReadBE16(pRecord + 6);
        const quint64 nDeclaredSize = wiiWadReadBE64(pRecord + 8);
        // No installable title carries an empty content (the banner and the
        // boot binary are never 0 bytes); an empty record would otherwise be
        // published as a zero-byte .app at exit 0.
        if ((nDeclaredSize == 0) || (nDeclaredSize > static_cast<quint64>(WIIWAD_MAX_CONTENT_SIZE))) return false;
        const qint64 nPlainSize = static_cast<qint64>(nDeclaredSize);

        qint64 nCipherSize = 0;
        qint64 nSlotSize = 0;
        if (!wiiWadAlignUp(nPlainSize, WIIWAD_AES_BLOCK, &nCipherSize) ||
            !wiiWadAlignUp(nPlainSize, WIIWAD_SECTION_ALIGN, &nSlotSize) ||
            !wiiWadRangeWithin(context.nSourceSize, nSlotOffset, nCipherSize)) {
            return false;
        }

        ENTRY entry = {};
        entry.bIsContent = true;
        entry.nOffset = nSlotOffset;
        entry.nSize = nCipherSize;
        entry.nUncompressedSize = nPlainSize;
        entry.nContentId = nContentId;
        entry.nContentIndex = nContentIndex;
        entry.nContentType = nContentType;
        entry.baSha1 = baRecords.mid(static_cast<qint32>((i * WIIWAD_TMD_RECORD_SIZE) + 16), static_cast<qint32>(WIIWAD_SHA1_SIZE));
        if (entry.baSha1.size() != WIIWAD_SHA1_SIZE) return false;

        // Sharpii/libWiiSharp default: the content INDEX names the file
        // (00000000.app = banner, 00000001.app = main DOL).  The content id
        // (the NAND file name) goes into the per-member INFO.
        const QString sBaseName = QStringLiteral("%1").arg(nContentIndex, 8, 16, QLatin1Char('0'));
        QString sFileName = sBaseName + QStringLiteral(".app");
        qint64 nSuffix = i + 1;
        while (setContentNames.contains(sFileName)) {
            sFileName = sBaseName + QStringLiteral("-%1.app").arg(nSuffix++);
        }
        setContentNames.insert(sFileName);
        entry.sFileName = sFileName;
        entry.sInfo = QStringLiteral("content id 0x%1, index %2, type %3")
                          .arg(nContentId, 8, 16, QLatin1Char('0'))
                          .arg(nContentIndex)
                          .arg(wiiWadContentTypeName(nContentType));
        listEntries.append(entry);

        if (i < (nNumberOfContents - 1)) {
            if (!wiiWadCheckedAdd(nRequiredDataSize, nSlotSize, &nRequiredDataSize) ||
                !wiiWadCheckedAdd(nSlotOffset, nSlotSize, &nSlotOffset)) {
                return false;
            }
        } else {
            if (!wiiWadCheckedAdd(nRequiredDataSize, nCipherSize, &nRequiredDataSize)) return false;
        }
    }
    // Some writers declare the last content unpadded and some IOS WADs a
    // data size that is not a multiple of 16; align16 covers both.
    qint64 nDeclaredDataSize16 = 0;
    if (!wiiWadAlignUp(context.nDataSize, WIIWAD_AES_BLOCK, &nDeclaredDataSize16) ||
        (nRequiredDataSize > nDeclaredDataSize16)) {
        return false;
    }

    // ---- footer -------------------------------------------------------
    if (context.nFooterSize > 0) {
        ENTRY entry = {};
        entry.bIsContent = false;
        entry.nOffset = context.nFooterOffset;
        entry.nSize = context.nFooterSize;
        entry.nUncompressedSize = context.nFooterSize;
        entry.sFileName = sTitleIdHex + QStringLiteral(".footer");
        if (context.nFooterSize >= WIIWAD_FOOTER_STAMP_SIZE) {
            const QByteArray baStamp = read_array_process(context.nFooterOffset, WIIWAD_FOOTER_STAMP_SIZE, pPdStruct);
            if (!guardedThis || !guardedSource) return false;
            QDateTime dtStamp;
            if (wiiWadParseFooterStamp(baStamp, &dtStamp)) entry.dtFooter = dtStamp;
        }
        listEntries.append(entry);
    }

    // ---- names --------------------------------------------------------
    for (qint32 i = 0; i < listEntries.size(); ++i) {
        const QString sFixed = fixFileName(listEntries.at(i).sFileName);
        if (!guardedThis || !guardedSource) return false;
        if (sFixed.isEmpty()) return false;
        listEntries[i].sFileName = sFixed;
    }

    // ---- informational certificate walk --------------------------------
    QStringList listCertNames;
    if (context.nCertSize > 0) {
        const QByteArray baCerts = read_array_process(context.nCertOffset, context.nCertSize, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        if (baCerts.size() == context.nCertSize) listCertNames = wiiWadWalkCertificates(baCerts);
    }

    // ---- archive-level INFO --------------------------------------------
    QString sInfo = QStringLiteral("Wii WAD '%1' v%2")
                        .arg(context.bIsBoot2 ? QStringLiteral("ib") : QStringLiteral("Is"))
                        .arg(context.nVersion);
    {
        const quint32 nTitleHigh = static_cast<quint32>(nTmdTitleId >> 32);
        const quint32 nTitleLow = static_cast<quint32>(nTmdTitleId & 0xFFFFFFFFULL);
        QString sTitleTag;
        const uchar arrLow[4] = {static_cast<uchar>(nTitleLow >> 24), static_cast<uchar>(nTitleLow >> 16),
                                 static_cast<uchar>(nTitleLow >> 8), static_cast<uchar>(nTitleLow)};
        const QString sLowText = wiiWadFixedString(arrLow, 4);
        if (sLowText.size() == 4) sTitleTag = sLowText + QStringLiteral(", ");
        sInfo += QStringLiteral("; title %1 (%2%3)")
                     .arg(nTmdTitleId, 16, 16, QLatin1Char('0'))
                     .arg(sTitleTag)
                     .arg(wiiWadTitleClassName(nTitleHigh));
    }
    if (nTicketTitleId != nTmdTitleId) {
        sInfo += QStringLiteral("; ticket title id %1 differs from the TMD").arg(nTicketTitleId, 16, 16, QLatin1Char('0'));
    }
    if ((nSystemVersion >> 32) == 1ULL) {
        sInfo += QStringLiteral("; IOS%1").arg(static_cast<quint32>(nSystemVersion & 0xFFFFFFFFULL));
    } else if (nSystemVersion != 0) {
        sInfo += QStringLiteral("; system version %1").arg(nSystemVersion, 16, 16, QLatin1Char('0'));
    }
    sInfo += QStringLiteral("; title version %1").arg(nTitleVersion);
    if (nTicketTitleVersion != nTitleVersion) {
        sInfo += QStringLiteral(" (ticket %1)").arg(nTicketTitleVersion);
    }
    sInfo += QStringLiteral("; region %1").arg(wiiWadRegionName(nRegion));
    if (nGroupId != 0) sInfo += QStringLiteral("; group 0x%1").arg(nGroupId, 4, 16, QLatin1Char('0'));
    sInfo += QStringLiteral("; %1 content%2, boot index %3")
                 .arg(nNumberOfContents)
                 .arg((nNumberOfContents == 1) ? QString() : QStringLiteral("s"))
                 .arg(nBootIndex);
    sInfo += QStringLiteral("; TMD v%1 by %2").arg(nTmdVersion).arg(sTmdIssuer.isEmpty() ? QStringLiteral("?") : sTmdIssuer);
    if (bIsVWii) sInfo += QStringLiteral("; vWii");
    if (nAccessRights != 0) sInfo += QStringLiteral("; access rights 0x%1").arg(nAccessRights, 8, 16, QLatin1Char('0'));
    sInfo += QStringLiteral("; ticket v%1 by %2, key index %3 (%4)")
                 .arg(nTicketVersion)
                 .arg(sTicketIssuer.isEmpty() ? QStringLiteral("?") : sTicketIssuer)
                 .arg(context.nCommonKeyIndex)
                 .arg(wiiWadKeyIndexName(context.nCommonKeyIndex));
    if (context.bDevTicket) sInfo += QStringLiteral("; DEV ticket (development common key needed)");
    if (nTicketId != 0) sInfo += QStringLiteral("; ticket id %1").arg(nTicketId, 16, 16, QLatin1Char('0'));
    if (nConsoleId != 0) sInfo += QStringLiteral("; console id 0x%1").arg(nConsoleId, 8, 16, QLatin1Char('0'));
    if (context.nCertSize > 0) {
        sInfo += QStringLiteral("; certs %1").arg(listCertNames.isEmpty() ? QStringLiteral("none recognised") : listCertNames.join(QStringLiteral(", ")));
    } else {
        sInfo += QStringLiteral("; no certificate chain");
    }
    if (context.nCrlSize > 0) sInfo += QStringLiteral("; CRL %1 bytes").arg(context.nCrlSize);
    context.sInfo = sInfo;

    context.listEntries = listEntries;
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XWiiWAD::isValid(PDSTRUCT *pPdStruct)
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

bool XWiiWAD::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XWiiWAD archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XWiiWAD::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XWiiWAD(pDevice);
}

QList<QString> XWiiWAD::getSearchSignatures()
{
    return {QStringLiteral("00000020'Is'0000"), QStringLiteral("00000020'ib'0000")};
}

XBinary::FT XWiiWAD::getFileType()
{
    return FT_WII_WAD;
}

XBinary::MODE XWiiWAD::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XWiiWAD::getEndian()
{
    return ENDIAN_BIG;
}

QString XWiiWAD::getArch()
{
    return QString();
}

qint32 XWiiWAD::getType()
{
    return TYPE_ARCHIVE;
}

QString XWiiWAD::getFileFormatExt()
{
    return QStringLiteral("wad");
}

QString XWiiWAD::getFileFormatExtsString()
{
    return QStringLiteral("Nintendo Wii WAD package (*.wad)");
}

QString XWiiWAD::getMIMEString()
{
    return QStringLiteral("application/x-wii-wad");
}

QString XWiiWAD::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return QString::number(context.nVersion);
}

qint64 XWiiWAD::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveEnd : 0;
}

QList<XBinary::MAPMODE> XWiiWAD::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XWiiWAD::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XWiiWAD::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XWiiWAD::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = WIIWAD_HEADER_PADDED_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if (nFileParts & FILEPART_STREAM) {
        const qint32 nCount = context.listEntries.size();
        for (qint32 i = 0; i < nCount; ++i) {
            const ENTRY &entry = context.listEntries.at(i);
            if (!entry.bIsContent) continue;
            if (!canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) break;
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = entry.nOffset;
            part.nFileSize = entry.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = entry.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, entry.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, entry.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ARCHIVE_STREAM);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("AES-128-CBC (Wii title key)"));
            part.mapProperties.insert(FPART_PROP_ENCRYPTED, true);
            listResult.append(part);
        }
    }

    if (nFileParts & FILEPART_REGION) {
        const qint32 nCount = context.listEntries.size();
        for (qint32 i = 0; i < nCount; ++i) {
            const ENTRY &entry = context.listEntries.at(i);
            if (!canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) break;
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = entry.nOffset;
            part.nFileSize = entry.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = entry.sFileName;
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveEnd;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XWiiWAD::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XWiiWAD::resolveCommonKey(const QMap<UNPACK_PROP, QVariant> &mapProperties, const QString &sDeviceFileName, QByteArray *pbaKey,
                               QString *psError)
{
    if (!pbaKey || !psError) return false;
    pbaKey->clear();
    psError->clear();

    // 1. Exact bytes from the console's -H/--password-hex.  A wrong length
    //    (an empty value included) is an error, never a fallthrough: the
    //    user meant this to be the key.  Presence, not emptiness, is tested
    //    so that this site agrees with the "invalid length" status in
    //    initUnpack().
    if (mapProperties.contains(UNPACK_PROP_PASSWORD_BYTES)) {
        const QByteArray baPasswordBytes = mapProperties.value(UNPACK_PROP_PASSWORD_BYTES).toByteArray();
        if (baPasswordBytes.size() != WIIWAD_COMMON_KEY_SIZE) {
            *psError = tr("Wii common key must be exactly 16 bytes (32 hex digits)");
            return false;
        }
        *pbaKey = baPasswordBytes;
        return true;
    }

    // 2. -P with exactly 32 hex digits; anything else is a ZIP-style password
    //    that cannot be a key and is ignored.
    const QString sPassword = mapProperties.value(UNPACK_PROP_PASSWORD).toString();
    if ((sPassword.size() == (WIIWAD_COMMON_KEY_SIZE * 2)) && wiiWadIsHexString(sPassword)) {
        const QByteArray baKey = QByteArray::fromHex(sPassword.toLatin1());
        if (baKey.size() == WIIWAD_COMMON_KEY_SIZE) {
            *pbaKey = baKey;
            return true;
        }
    }

    // 3. Companion common-key.bin next to the WAD (WWPacker / wadder
    //    convention).  Memory devices have no file name and skip this.
    if (!sDeviceFileName.isEmpty()) {
        const QFileInfo fileInfo(sDeviceFileName);
        QFile fileKey(fileInfo.absolutePath() + QStringLiteral("/common-key.bin"));
        if (fileKey.exists() && (fileKey.size() == WIIWAD_COMMON_KEY_SIZE) && fileKey.open(QIODevice::ReadOnly)) {
            const QByteArray baKey = fileKey.read(WIIWAD_COMMON_KEY_SIZE);
            fileKey.close();
            if (baKey.size() == WIIWAD_COMMON_KEY_SIZE) {
                *pbaKey = baKey;
                return true;
            }
        }
    }

    *psError = tr("Wii common key required (use -H <32 hex digits> or put common-key.bin next to the WAD)");
    return false;
}

bool XWiiWAD::resolveTitleKey(UNPACK_STATE *pState, CONTEXT *pContext, QString *psError)
{
    if (!pState || !pContext || !psError) return false;
    QPointer<XWiiWAD> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    // Read the properties on every call: unpackToFolder() replaces the state's
    // property map after initUnpack(), so a key captured at init could be stale.
    const QString sDeviceFileName = XBinary::getDeviceFileName(guardedSource.data());
    if (!guardedThis || !guardedSource) return false;

    QByteArray baCommonKey;
    if (!resolveCommonKey(pState->mapUnpackProperties, sDeviceFileName, &baCommonKey, psError)) {
        wiiWadWipe(&baCommonKey);
        return false;
    }
    if ((pContext->baTitleKey.size() == WIIWAD_COMMON_KEY_SIZE) && (pContext->baCommonKeyUsed == baCommonKey)) {
        wiiWadWipe(&baCommonKey);
        return true;
    }

    // titleKey = AES-128-CBC-decrypt(commonKey, IV = titleId || 8 zero bytes,
    // ticket[0x1BF..0x1CE]) - one block.
    QByteArray baIV(static_cast<qint32>(WIIWAD_AES_BLOCK), '\0');
    memcpy(baIV.data(), pContext->baTitleId.constData(), static_cast<size_t>(WIIWAD_TITLE_ID_SIZE));
    QByteArray baTitleKey(static_cast<qint32>(WIIWAD_COMMON_KEY_SIZE), '\0');
    const bool bDecrypted = (pContext->baEncryptedTitleKey.size() == WIIWAD_COMMON_KEY_SIZE) &&
                            XAESDecoder::decryptAESCBC(baCommonKey, baIV, reinterpret_cast<const quint8 *>(pContext->baEncryptedTitleKey.constData()),
                                                       reinterpret_cast<quint8 *>(baTitleKey.data()), WIIWAD_COMMON_KEY_SIZE);
    if (!bDecrypted) {
        wiiWadWipe(&baCommonKey);
        wiiWadWipe(&baTitleKey);
        *psError = tr("Cannot derive the Wii title key");
        return false;
    }
    wiiWadWipe(&pContext->baTitleKey);
    wiiWadWipe(&pContext->baCommonKeyUsed);
    pContext->baTitleKey = baTitleKey;
    pContext->baCommonKeyUsed = baCommonKey;
    wiiWadWipe(&baCommonKey);
    wiiWadWipe(&baTitleKey);
    return true;
}

bool XWiiWAD::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XWiiWAD> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    // Malformed output-limit properties fail here rather than at extraction.
    OUTPUT_POLICY policy = {};
    if (!resolveUnpackOutputPolicy(mapProperties, &policy) || !guardedThis) {
        if (guardedThis) releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    // A missing key is NOT an init failure: the listing is complete from the
    // TMD and the metadata sections extract without one.
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || pContext->listEntries.isEmpty() ||
        (pContext->nSourceSize != guardedSource->size())) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    QString sKeyStatus = QStringLiteral("not supplied");
    {
        const QString sDeviceFileName = XBinary::getDeviceFileName(guardedSource.data());
        if (!guardedThis || !guardedSource) {
            if (guardedThis) releaseUnpackSource(pState);
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        QByteArray baKey;
        QString sKeyError;
        if (resolveCommonKey(mapProperties, sDeviceFileName, &baKey, &sKeyError)) {
            sKeyStatus = QStringLiteral("supplied");
        } else if (!sKeyError.isEmpty() && mapProperties.contains(UNPACK_PROP_PASSWORD_BYTES)) {
            sKeyStatus = QStringLiteral("invalid length");
        }
        wiiWadWipe(&baKey);
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, pContext->sInfo + QStringLiteral("; key: ") + sKeyStatus);
    {
        const qint32 nCount = pContext->listEntries.size();
        for (qint32 i = 0; i < nCount; ++i) {
            const ENTRY &entry = pContext->listEntries.at(i);
            if (!entry.bIsContent && entry.dtFooter.isValid()) {
                pState->mapArchiveProperties.insert(FPART_PROP_DATETIME, entry.dtFooter);
                break;
            }
        }
    }
    pState->nCurrentOffset = pContext->listEntries.first().nOffset;
    pState->nTotalSize = pContext->nSourceSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listEntries.size();
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

XBinary::ARCHIVERECORD XWiiWAD::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XWiiWAD> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        !isPdStructNotCanceled(pPdStruct)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (nCurrentSize != pContext->nSourceSize) || (pState->nTotalSize != nCurrentSize) ||
        (pState->nNumberOfRecords != pContext->listEntries.size()) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pContext->listEntries.size())) {
        return ARCHIVERECORD();
    }
    const ENTRY entry = pContext->listEntries.at(pState->nCurrentIndex);
    if (!wiiWadRangeWithin(nCurrentSize, entry.nOffset, entry.nSize) || (pState->nCurrentOffset != entry.nOffset) ||
        entry.sFileName.isEmpty()) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = entry.nOffset;
    result.nStreamSize = entry.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, entry.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, entry.nSize);
    result.mapProperties.insert(FPART_PROP_FILEMODE, static_cast<quint32>(0644));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    if (!entry.bIsContent) {
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Store"));
        if (entry.dtFooter.isValid()) {
            result.mapProperties.insert(FPART_PROP_DATETIME, entry.dtFooter);
            result.mapProperties.insert(FPART_PROP_MTIME, entry.dtFooter);
        }
        return result;
    }

    // ENCRYPTED describes the archive, not whether a key is present (XZip
    // does the same); the SHA-1 is the TMD's plaintext digest.
    if (entry.baSha1.size() != WIIWAD_SHA1_SIZE) return ARCHIVERECORD();
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("AES-128-CBC (Wii title key)"));
    result.mapProperties.insert(FPART_PROP_ENCRYPTED, true);
    result.mapProperties.insert(FPART_PROP_CHECKSUM, QString::fromLatin1(entry.baSha1.toHex()));
    result.mapProperties.insert(FPART_PROP_CHECKSUMTYPE, QStringLiteral("SHA1"));
    result.mapProperties.insert(FPART_PROP_INFO, entry.sInfo);
    return markArchiveStreamRecord(&result, pState->nCurrentIndex) ? result : ARCHIVERECORD();
}

bool XWiiWAD::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XWiiWAD> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    QPointer<QIODevice> guardedOutput(pDevice);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !guardedSource || !guardedOutput || !pState->pContext ||
        devicesAlias(guardedSource.data(), guardedOutput.data()) || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (nCurrentSize != pContext->nSourceSize) || (pState->nTotalSize != nCurrentSize) ||
        (pState->nNumberOfRecords != pContext->listEntries.size()) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pContext->listEntries.size())) {
        return false;
    }
    const ENTRY entry = pContext->listEntries.at(pState->nCurrentIndex);
    if (!wiiWadRangeWithin(nCurrentSize, entry.nOffset, entry.nSize) || (pState->nCurrentOffset != entry.nOffset)) {
        return false;
    }

    if (!entry.bIsContent) {
        // Raw STORE section: the base implementation runs the store codec
        // through XDecompress and acquires the operation guard itself.
        operationGuard.release();
        return guardedThis && guardedThis->XArchive::unpackCurrent(pState, pDevice, pPdStruct);
    }

    const qint64 nPlainSize = entry.nUncompressedSize;
    const qint64 nCipherSize = entry.nSize;
    if ((nPlainSize < 0) || (nCipherSize < nPlainSize) || ((nCipherSize % WIIWAD_AES_BLOCK) != 0) ||
        (entry.baSha1.size() != WIIWAD_SHA1_SIZE)) {
        return false;
    }

    QString sKeyError;
    if (!resolveTitleKey(pState, pContext, &sKeyError) || !guardedThis || !guardedSource || !guardedOutput) {
        if (!sKeyError.isEmpty()) setPdStructErrorString(pPdStruct, sKeyError);
        return false;
    }
    if (pContext->baTitleKey.size() != WIIWAD_COMMON_KEY_SIZE) return false;

    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nPlainSize)) {
        setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
        return false;
    }
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, entry.sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(nPlainSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    WiiWadStageHolder stage(createFileBuffer(nPlainSize, pPdStruct));
    if (!stage.pDevice || (stage.pDevice->size() != nPlainSize) || !stage.pDevice->seek(0) || !guardedThis || !guardedSource ||
        !guardedOutput || !isUnpackSourceCurrent(pState, pPdStruct)) {
        return false;
    }

    QByteArray baCipher(static_cast<qint32>(WIIWAD_DECRYPT_CHUNK), '\0');
    QByteArray baPlain(static_cast<qint32>(WIIWAD_DECRYPT_CHUNK), '\0');
    if ((baCipher.size() != WIIWAD_DECRYPT_CHUNK) || (baPlain.size() != WIIWAD_DECRYPT_CHUNK)) return false;

    // IV = content index as u16 big endian, then 14 zero bytes.  CBC is
    // chained by hand across chunks: the next IV is the last ciphertext block.
    QByteArray baIV(static_cast<qint32>(WIIWAD_AES_BLOCK), '\0');
    baIV[0] = static_cast<char>(entry.nContentIndex >> 8);
    baIV[1] = static_cast<char>(entry.nContentIndex & 0xFFU);

    // The SHA-1 is ALWAYS computed: it is not a CRC-style checksum but the
    // only detector of a wrong common key (spec 3.3), so it is deliberately
    // not gated on XBinary::isUnpackCRCEnabled - with the gate, a wrong key
    // would publish AES-CBC noise as a successful member.
    QCryptographicHash hash(QCryptographicHash::Sha1);
    qint64 nCipherDone = 0;
    qint64 nPlainWritten = 0;
    while (nCipherDone < nCipherSize) {
        if (!guardedThis || !guardedSource || !guardedOutput || !isUnpackSourceCurrent(pState, pPdStruct) ||
            !isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        const qint64 nChunk = qMin<qint64>(WIIWAD_DECRYPT_CHUNK, nCipherSize - nCipherDone);
        if ((nChunk <= 0) || ((nChunk % WIIWAD_AES_BLOCK) != 0)) return false;
        if ((read_array_process(entry.nOffset + nCipherDone, baCipher.data(), nChunk, pPdStruct) != nChunk) || !guardedThis ||
            !guardedSource || !guardedOutput) {
            return false;
        }
        if (!XAESDecoder::decryptAESCBC(pContext->baTitleKey, baIV, reinterpret_cast<const quint8 *>(baCipher.constData()),
                                        reinterpret_cast<quint8 *>(baPlain.data()), nChunk)) {
            return false;
        }
        const qint64 nToWrite = qMin<qint64>(nChunk, nPlainSize - nPlainWritten);
        if (nToWrite > 0) {
            if (safeWriteData(stage.pDevice, nPlainWritten, baPlain.constData(), nToWrite, pPdStruct) != nToWrite) {
                return false;
            }
            if (!guardedThis || !guardedSource || !guardedOutput) return false;
            hash.addData(baPlain.constData(), static_cast<int>(nToWrite));
            nPlainWritten += nToWrite;
        }
        memcpy(baIV.data(), baCipher.constData() + (nChunk - WIIWAD_AES_BLOCK), static_cast<size_t>(WIIWAD_AES_BLOCK));
        nCipherDone += nChunk;
    }
    if (nPlainWritten != nPlainSize) return false;

    // The TMD SHA-1 is the format's only integrity check and the only way to
    // notice a wrong common key; nothing is published on a mismatch.
    if (hash.result() != entry.baSha1) {
        setPdStructErrorString(pPdStruct, tr("Content SHA-1 mismatch: wrong Wii common key or corrupted content"));
        return false;
    }

    if (!guardedThis || !guardedSource || !guardedOutput || (stage.pDevice->size() != nPlainSize) || !stage.pDevice->seek(0) ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const bool bPublished = publishUnpackOutput(stage.pDevice, guardedOutput.data(), pState, pPdStruct);
    if (bPublished && guardedThis) {
        pState->nCurrentOffset = entry.nOffset + entry.nSize;
    }
    return bPublished && guardedThis;
}

bool XWiiWAD::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XWiiWAD> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if ((pState->nTotalSize != pContext->nSourceSize) || (pState->nNumberOfRecords != pContext->listEntries.size()) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listEntries.at(pState->nCurrentIndex).nOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveEnd;
    return false;
}

bool XWiiWAD::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    if (pContext) {
        wiiWadWipe(&pContext->baTitleKey);
        wiiWadWipe(&pContext->baCommonKeyUsed);
    }
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}

QList<XBinary::FPART_PROP> XWiiWAD::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD,
            FPART_PROP_REPORTEDMETHOD, FPART_PROP_ENCRYPTED,      FPART_PROP_CHECKSUM,       FPART_PROP_CHECKSUMTYPE,
            FPART_PROP_INFO,           FPART_PROP_FILEMODE,       FPART_PROP_ISFOLDER,       FPART_PROP_DATETIME,
            FPART_PROP_MTIME};
}
