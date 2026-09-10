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
#include "xvmssavesetdecoder.h"

#include <QtEndian>

namespace {
const qint64 VMS_BLOCK_HEADER_SIZE = 0x100;
const qint64 VMS_RECORD_HEADER_SIZE = 0x10;
const qint64 VMS_MAX_MEMBER_SIZE = Q_INT64_C(512) * 1024 * 1024;
const qint32 VMS_MAX_MEMBERS = 100000;
const qint32 VMS_MAX_BODY_RECORDS = 40000000;

bool vmsOpsysOk(quint16 nOpsys)
{
    return (nOpsys == 0x400) || (nOpsys == 0x800) || (nOpsys == 0x1000);
}

// A sequential cursor over the whole buffer; read() hands back a pointer into
// it rather than a copy, so assembling a member costs one append per record.
struct VMSCursor {
    const quint8 *pData;
    qint64 nSize;
    qint64 nPosition;

    bool read(qint64 nCount, const quint8 **ppResult)
    {
        if (!ppResult || (nCount < 0) || (nPosition < 0)) return false;
        if (nCount > (nSize - nPosition)) return false;
        *ppResult = pData + nPosition;
        nPosition += nCount;
        return true;
    }

    bool skip(qint64 nCount)
    {
        if ((nCount < 0) || (nPosition < 0) || (nCount > (nSize - nPosition))) return false;
        nPosition += nCount;
        return true;
    }

    bool seek(qint64 nTarget)
    {
        if ((nTarget < 0) || (nTarget > nSize)) return false;
        nPosition = nTarget;
        return true;
    }
};

// The block-and-record iterator.  nRemaining is allowed to go negative exactly
// as the reference lets it: the next block header is only fetched when it is
// EXACTLY zero.
struct VMSWalker {
    VMSCursor *pCursor;
    qint64 nRemaining;
    qint32 nBlockSize;

    bool nextRecord(qint32 *pnRecordSize, qint32 *pnRecordType)
    {
        if (!pCursor || !pnRecordSize || !pnRecordType) return false;

        if (nRemaining == 0) {
            while (true) {
                const quint8 *pHeader = nullptr;
                if (!pCursor->read(VMS_BLOCK_HEADER_SIZE, &pHeader)) return false;
                const quint16 nHeaderSize = qFromLittleEndian<quint16>(pHeader);
                const quint16 nOpsys = qFromLittleEndian<quint16>(pHeader + 2);
                const quint16 nSubsys = qFromLittleEndian<quint16>(pHeader + 4);
                const quint16 nApplic = qFromLittleEndian<quint16>(pHeader + 6);
                qint32 nBlock = (qint32)qFromLittleEndian<quint32>(pHeader + 0x28);
                if (nBlockSize == 0) nBlockSize = nBlock;
                if ((nHeaderSize != 0x100) || !vmsOpsysOk(nOpsys) || (nSubsys != 1)) return false;
                if ((nApplic != 1) && (nApplic != 2)) return false;
                if (qFromLittleEndian<quint64>(pHeader + 0x10) != 0) return false;
                if (qFromLittleEndian<quint64>(pHeader + 0x18) != 0) return false;
                if (qFromLittleEndian<quint64>(pHeader + 0xEC) != 0) return false;
                if (qFromLittleEndian<quint64>(pHeader + 0xF4) != 0) return false;
                if (qFromLittleEndian<quint16>(pHeader + 0xFC) != 0) return false;
                if ((nApplic == 2) && (nBlock == 0)) nBlock = nBlockSize;
                if (nBlock < 0x101) return false;
                nRemaining = (qint64)nBlock - VMS_BLOCK_HEADER_SIZE;
                if (nApplic != 2) break;
                if (!pCursor->skip(nRemaining)) return false;
            }
        }

        const quint8 *pRecord = nullptr;
        if (!pCursor->read(VMS_RECORD_HEADER_SIZE, &pRecord)) return false;
        nRemaining -= VMS_RECORD_HEADER_SIZE;
        const qint32 nRecordSize = (qint32)qFromLittleEndian<quint16>(pRecord);
        const qint32 nRecordType = (qint32)qFromLittleEndian<quint16>(pRecord + 2);
        if (qFromLittleEndian<quint32>(pRecord + 0x0C) != 0) return false;
        nRemaining -= nRecordSize;
        *pnRecordSize = nRecordSize;
        *pnRecordType = nRecordType;

        return true;
    }
};

struct VMSAttributes {
    QString sName;
    qint64 nSize;
    bool bIsDirectory;
    bool bVarRec;
};

// The rtype-3 file-attributes record.  The reference bounds this loop with the
// FULL rsize without subtracting the two magic bytes and then seeks to the
// record end, so the over-read is harmless; it is reproduced, not corrected.
bool vmsParseAttributes(VMSCursor *pCursor, qint32 nRecordSize, VMSAttributes *pAttributes)
{
    if (!pCursor || !pAttributes) return false;
    const quint8 *p = nullptr;
    if (!pCursor->read(2, &p)) return false;
    if ((p[0] != 1) || (p[1] != 1)) return false;

    VMSAttributes attributes;
    attributes.nSize = 0;
    attributes.bIsDirectory = false;
    attributes.bVarRec = false;

    qint64 nLeft = nRecordSize;
    while (nLeft > 3) {
        if (!pCursor->read(4, &p)) return false;
        const qint32 nLength = (qint32)qFromLittleEndian<quint16>(p);
        const qint32 nTag = (qint32)qFromLittleEndian<quint16>(p + 2);
        if ((nLeft - 4) < nLength) return false;
        nLeft = nLeft - 4 - nLength;

        if (nTag == 0x2A) {
            if (nLength == 0) return false;
            if (!pCursor->read(nLength, &p)) return false;
            attributes.sName = QString::fromLatin1((const char *)p, nLength);
        } else if (nTag == 0x33) {
            if (nLength != 4) return false;
            if (!pCursor->read(4, &p)) return false;
            attributes.bIsDirectory = ((qFromLittleEndian<quint32>(p) & 0x2000) != 0);
        } else if (nTag == 0x34) {
            if (nLength != 0x20) return false;
            if (!pCursor->read(0x20, &p)) return false;
            const quint64 nHigh = (quint64)qFromLittleEndian<quint16>(p + 8);
            const quint64 nLow = (quint64)qFromLittleEndian<quint16>(p + 10);
            const quint64 nFirstFreeByte = (quint64)qFromLittleEndian<quint16>(p + 12);
            // efblk is a VAX word-swapped longword.
            const quint64 nEndOfFileBlock = nHigh * 0x10000 + nLow;
            if (nEndOfFileBlock == 0) return false;
            attributes.nSize = (qint64)((nEndOfFileBlock - 1) * 512 + nFirstFreeByte);
            attributes.bVarRec = ((p[0] == 2) && (p[1] == 2));
        } else if ((nTag == 0x36) || (nTag == 0x37)) {
            if (nLength != 8) return false;
            if (!pCursor->read(8, &p)) return false;
        } else {
            if (!pCursor->skip(nLength)) return false;
        }
    }

    *pAttributes = attributes;

    return true;
}

// Gather the rtype-4 payloads that follow until nSize bytes exist; interleaved
// rtype-0 records are skipped and the last record is truncated to fit.  A null
// sink consumes the body without materialising it.
bool vmsCollectBody(VMSWalker *pWalker, VMSCursor *pCursor, qint64 nSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pWalker || !pCursor) return false;
    if (pbaResult) pbaResult->clear();
    if ((nSize < 0) || (nSize > VMS_MAX_MEMBER_SIZE)) return false;

    qint64 nLeft = nSize;
    qint32 nRecords = 0;
    while (nLeft >= 1) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (nRecords >= VMS_MAX_BODY_RECORDS) return false;
        nRecords++;

        qint32 nRecordSize = 0;
        qint32 nRecordType = 0;
        if (!pWalker->nextRecord(&nRecordSize, &nRecordType)) return false;
        if (nRecordType == 0) {
            if (!pCursor->skip(nRecordSize)) return false;
            continue;
        }
        if (nRecordType != 4) return false;
        const quint8 *p = nullptr;
        if (!pCursor->read(nRecordSize, &p)) return false;
        if (pbaResult) {
            const qint64 nTake = (nLeft < (qint64)nRecordSize) ? nLeft : (qint64)nRecordSize;
            if ((qint64)pbaResult->size() > (VMS_MAX_MEMBER_SIZE - nTake)) return false;
            pbaResult->append((const char *)p, (qint32)nTake);
        }
        nLeft -= nRecordSize;
    }

    return true;
}

// VMS variable-length records to CRLF text.
bool vmsConvertVarRec(const QByteArray &baBody, QByteArray *pbaResult)
{
    if (!pbaResult) return false;
    QByteArray baResult;
    const quint8 *pData = (const quint8 *)baBody.constData();
    qint64 nPosition = 0;
    qint64 nLeft = (qint64)baBody.size();

    while (nLeft > 0) {
        if (nLeft < 2) return false;
        const qint32 nLength = (qint32)qFromLittleEndian<quint16>(pData + nPosition);
        if ((nLeft - 2) < nLength) return false;
        if ((qint64)baResult.size() > (VMS_MAX_MEMBER_SIZE - nLength - 2)) return false;
        baResult.append((const char *)(pData + nPosition + 2), nLength);
        baResult.append("\r\n", 2);
        nPosition += 2 + nLength;
        nLeft -= 2 + nLength;
        if (nLength & 1) {
            if (nLeft < 1) return false;
            nPosition += 1;
            nLeft -= 1;
        }
    }

    *pbaResult = baResult;

    return true;
}

}  // namespace

bool XVMSSaveSetDecoder::isBlockHeaderValid(const QByteArray &baHeader)
{
    if ((qint64)baHeader.size() < VMS_BLOCK_HEADER_SIZE) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();
    if (qFromLittleEndian<quint16>(pHeader) != 0x100) return false;
    if (!vmsOpsysOk(qFromLittleEndian<quint16>(pHeader + 2))) return false;
    if (qFromLittleEndian<quint16>(pHeader + 4) != 1) return false;
    if (qFromLittleEndian<quint16>(pHeader + 6) != 1) return false;
    if ((qint32)qFromLittleEndian<quint32>(pHeader + 0x28) <= 0x100) return false;
    if (qFromLittleEndian<quint64>(pHeader + 0x10) != 0) return false;
    if (qFromLittleEndian<quint64>(pHeader + 0x18) != 0) return false;
    if (qFromLittleEndian<quint32>(pHeader + 0x20) != 0x10101) return false;
    if (qFromLittleEndian<quint64>(pHeader + 0xEC) != 0) return false;
    if (qFromLittleEndian<quint64>(pHeader + 0xF4) != 0) return false;
    if (qFromLittleEndian<quint16>(pHeader + 0xFC) != 0) return false;

    return true;
}

bool XVMSSaveSetDecoder::walk(const QByteArray &baData, QList<MEMBER> *pListMembers, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pListMembers) return false;
    pListMembers->clear();

    VMSCursor cursor;
    cursor.pData = (const quint8 *)baData.constData();
    cursor.nSize = (qint64)baData.size();
    cursor.nPosition = 0;

    VMSWalker walker;
    walker.pCursor = &cursor;
    walker.nRemaining = 0;
    walker.nBlockSize = 0;

    while (true) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) break;
        if (pListMembers->size() >= VMS_MAX_MEMBERS) break;

        qint32 nRecordSize = 0;
        qint32 nRecordType = 0;
        if (!walker.nextRecord(&nRecordSize, &nRecordType)) break;
        const qint64 nHeaderOffset = cursor.nPosition - VMS_RECORD_HEADER_SIZE;
        const qint64 nRecordEnd = cursor.nPosition + nRecordSize;

        if (nRecordType != 3) {
            if ((nRecordType != 0) && (nRecordType != 1) && (nRecordType != 0x0B) && (nRecordType != 4)) break;
            if (!cursor.seek(nRecordEnd)) break;
            continue;
        }

        VMSAttributes attributes;
        if (!vmsParseAttributes(&cursor, nRecordSize, &attributes)) break;
        if (!cursor.seek(nRecordEnd)) break;

        MEMBER member;
        member.nHeaderOffset = nHeaderOffset;
        member.nBodyOffset = cursor.nPosition;
        member.nBodySpanSize = 0;
        member.nRawSize = attributes.nSize;
        member.nReportedSize = attributes.nSize;
        member.nRemaining = walker.nRemaining;
        member.nBlockSize = walker.nBlockSize;
        member.bVarRec = attributes.bVarRec;
        member.sFileName = attributes.sName;

        if (attributes.bIsDirectory) {
            // A directory's body still has to be consumed or the walk
            // desynchronises, but the reference emits no member for it.
            if (!vmsCollectBody(&walker, &cursor, attributes.nSize, nullptr, pPdStruct)) break;
            continue;
        }

        QByteArray baBody;
        if (!vmsCollectBody(&walker, &cursor, attributes.nSize, attributes.bVarRec ? &baBody : nullptr, pPdStruct)) break;
        member.nBodySpanSize = cursor.nPosition - member.nBodyOffset;

        if (attributes.bVarRec) {
            QByteArray baConverted;
            if (!vmsConvertVarRec(baBody, &baConverted)) break;
            member.nReportedSize = (qint64)baConverted.size();
        }

        if (member.sFileName.isEmpty()) member.sFileName = QString("member%1").arg(pListMembers->size());
        pListMembers->append(member);
    }

    return true;
}

QByteArray XVMSSaveSetDecoder::memberProperties(const MEMBER &member)
{
    QByteArray baResult(PROPERTY_SIZE, (char)0);
    uchar *pData = (uchar *)baResult.data();
    qToLittleEndian<quint64>((quint64)member.nRemaining, pData);
    qToLittleEndian<quint64>((quint64)member.nRawSize, pData + 8);
    qToLittleEndian<quint32>((quint32)member.nBlockSize, pData + 16);
    pData[20] = member.bVarRec ? 1 : 0;

    return baResult;
}

bool XVMSSaveSetDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, const QByteArray &baProperty, QByteArray *pbaResult,
                                XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult || (nUncompressedSize < 0)) return false;
    pbaResult->clear();
    if (baProperty.size() != PROPERTY_SIZE) return false;

    const uchar *pProperty = (const uchar *)baProperty.constData();
    const qint64 nRemaining = (qint64)qFromLittleEndian<quint64>(pProperty);
    const qint64 nRawSize = (qint64)qFromLittleEndian<quint64>(pProperty + 8);
    const qint32 nBlockSize = (qint32)qFromLittleEndian<quint32>(pProperty + 16);
    const bool bVarRec = (pProperty[20] != 0);
    if ((nRawSize < 0) || (nRawSize > VMS_MAX_MEMBER_SIZE)) return false;

    VMSCursor cursor;
    cursor.pData = (const quint8 *)baPacked.constData();
    cursor.nSize = (qint64)baPacked.size();
    cursor.nPosition = 0;

    VMSWalker walker;
    walker.pCursor = &cursor;
    walker.nRemaining = nRemaining;
    walker.nBlockSize = nBlockSize;

    QByteArray baBody;
    if (!vmsCollectBody(&walker, &cursor, nRawSize, &baBody, pPdStruct)) return false;

    if (bVarRec) {
        QByteArray baConverted;
        if (!vmsConvertVarRec(baBody, &baConverted)) return false;
        baBody = baConverted;
    }

    if ((qint64)baBody.size() != nUncompressedSize) return false;
    *pbaResult = baBody;

    return true;
}
