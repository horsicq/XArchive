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
#include "xvmarcdecoder.h"

#include <QtEndian>

#include <cstring>

namespace {
const qint32 VMARC_ENTRIES = 4096;        // dictionary entries
const qint32 VMARC_TOP = 0x101;           // first recyclable entry
const qint32 VMARC_LIMIT = 4095;          // last recyclable entry
const qint32 VMARC_SCRATCH = 4096;        // used when nothing can be recycled
const qint32 VMARC_SLOTS = 4097;          // entries plus the scratch slot
const qint32 VMARC_PARENT_ROOT = -1;      // the 257 seeded entries
const qint32 VMARC_PARENT_UNSET = -2;     // never assigned
const qint64 VMARC_CANCEL_STEP = 0x10000;

// The CMS record sink (translate flag OFF).  put() returns 1 to continue and
// 0 when the member is finished.
class RecordSink {
public:
    RecordSink(QByteArray *pbaOut, quint16 nLRECL, bool bFixed, qint64 nLimit);

    qint32 put(qint32 nSymbol);
    qint64 getCount() const;
    bool isOverflow() const;

private:
    QByteArray *m_pbaOut;
    qint32 m_nLRECL;
    bool m_bFixed;
    qint32 m_nNewLines;
    qint32 m_nColumn;
    qint64 m_nCount;
    qint64 m_nLimit;
    bool m_bOverflow;
};

RecordSink::RecordSink(QByteArray *pbaOut, quint16 nLRECL, bool bFixed, qint64 nLimit)
{
    m_pbaOut = pbaOut;
    m_nLRECL = nLRECL;
    m_bFixed = bFixed;
    m_nNewLines = 0;
    m_nColumn = 0;
    m_nCount = 0;
    m_nLimit = nLimit;
    m_bOverflow = false;
}

qint64 RecordSink::getCount() const
{
    return m_nCount;
}

bool RecordSink::isOverflow() const
{
    return m_bOverflow;
}

qint32 RecordSink::put(qint32 nSymbol)
{
    const qint32 nValue = nSymbol - 1;
    if (nValue < 0) {
        m_nNewLines++;
        // 'F' members end on the first end-of-record, 'V' members need two in
        // a row - any data byte resets the counter below.
        if ((m_nNewLines < 2) && (!m_bFixed)) return 1;
        return 0;
    }
    m_nNewLines = 0;
    if (m_bFixed) {
        // Column tracking exists in the reference sink but produces no output
        // of its own; it is kept so the shape of the port matches.
        if (m_nColumn == m_nLRECL) m_nColumn = 0;
        m_nColumn++;
    }
    if (m_nCount >= m_nLimit) {
        m_bOverflow = true;
        return -1;
    }
    if (m_pbaOut) m_pbaOut->append((char)(quint8)(nValue & 0xff));
    m_nCount++;

    return 1;
}

// 12-bit codes, MSB first.
class BitReader {
public:
    BitReader(const quint8 *pData, qint64 nOffset, qint64 nEnd);

    bool get(qint32 *pnCode);
    qint64 getPosition() const;

private:
    const quint8 *m_pData;
    qint64 m_nPosition;
    qint64 m_nEnd;
    quint32 m_nBits;
    qint32 m_nBitCount;
};

BitReader::BitReader(const quint8 *pData, qint64 nOffset, qint64 nEnd)
{
    m_pData = pData;
    m_nPosition = nOffset;
    m_nEnd = nEnd;
    m_nBits = 0;
    m_nBitCount = 0;
}

qint64 BitReader::getPosition() const
{
    return m_nPosition;
}

bool BitReader::get(qint32 *pnCode)
{
    while (m_nBitCount <= 11) {
        if (m_nPosition >= m_nEnd) return false;
        const quint32 nByte = m_pData[m_nPosition];
        m_nPosition++;
        m_nBits = (quint32)(m_nBits + (nByte << (24 - m_nBitCount)));
        m_nBitCount += 8;
    }
    *pnCode = (qint32)(m_nBits >> 20);
    m_nBits = (quint32)(m_nBits << 12);
    m_nBitCount -= 12;

    return true;
}

bool vmarcLzw(BitReader *pBits, RecordSink *pSink, XBinary::PDSTRUCT *pPdStruct)
{
    qint32 nParent[VMARC_SLOTS];
    qint32 nChar[VMARC_SLOTS];
    qint32 nRefs[VMARC_SLOTS];
    qint32 nChain[VMARC_SLOTS];

    for (qint32 i = 0; i < VMARC_SLOTS; i++) {
        nParent[i] = VMARC_PARENT_UNSET;
        nChar[i] = 0;
        nRefs[i] = 0;
    }
    for (qint32 i = 0; i < VMARC_TOP; i++) {
        nParent[i] = VMARC_PARENT_ROOT;
        nChar[i] = i;
        nRefs[i] = 1;
    }
    qint32 nRover = VMARC_TOP - 1;
    qint32 nPending = VMARC_TOP;
    nParent[nPending] = VMARC_PARENT_ROOT;

    qint64 nIterations = 0;

    for (;;) {
        nIterations++;
        if ((nIterations % VMARC_CANCEL_STEP) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }

        qint32 nCode = 0;
        if (!pBits->get(&nCode)) return false;
        if ((nCode < 0) || (nCode >= VMARC_ENTRIES)) return false;
        if (nParent[nCode] == VMARC_PARENT_UNSET) return false;

        qint32 nLength = 0;
        qint32 nNode = nCode;
        while (nNode != VMARC_PARENT_ROOT) {
            if ((nNode < 0) || (nNode >= VMARC_SLOTS) || (nLength >= VMARC_SLOTS)) return false;
            nChain[nLength] = nNode;
            nLength++;
            nNode = nParent[nNode];
            if (nNode == VMARC_PARENT_UNSET) return false;
        }
        if (nLength == 0) return false;

        // Lazy fill: the pending entry finally learns its character, which is
        // the first character of the string that has just been decoded.
        nChar[nPending] = nChar[nChain[nLength - 1]];

        for (qint32 i = nLength - 1; i >= 0; i--) {
            const qint32 nResult = pSink->put(nChar[nChain[i]]);
            if (nResult < 0) return false;
            if (nResult == 0) return true;
        }

        nRefs[nCode]++;

        // Round-robin search for a childless entry to recycle.
        qint32 nSlot = nRover;
        bool bWrapped = false;
        for (;;) {
            nSlot++;
            if (nSlot > VMARC_LIMIT) {
                if (bWrapped) break;
                bWrapped = true;
                nSlot = VMARC_TOP;
            }
            if (nRefs[nSlot] == 0) break;
        }

        if (nSlot > VMARC_LIMIT) {
            nRefs[nCode]--;
            nPending = VMARC_SCRATCH;
        } else {
            const qint32 nOldParent = nParent[nSlot];
            if ((nOldParent != VMARC_PARENT_UNSET) && (nOldParent != VMARC_PARENT_ROOT)) nRefs[nOldParent]--;
            nParent[nSlot] = nCode;
            nRover = nSlot;
            nPending = nSlot;
        }
    }
}

bool vmarcStored(const quint8 *pData, qint64 nOffset, qint64 nEnd, QByteArray *pbaOut, qint64 *pnEndOffset, qint64 nLimit)
{
    qint64 nPosition = nOffset;
    qint64 nCount = 0;

    while ((nPosition + 2) <= nEnd) {
        const qint64 nLength = (qint64)qFromBigEndian<quint16>(pData + nPosition);
        nPosition += 2;
        if (nLength == 0) {
            *pnEndOffset = nPosition;
            return true;
        }
        if ((nPosition + nLength) > nEnd) {
            *pnEndOffset = nPosition;
            return false;
        }
        if ((nCount + nLength) > nLimit) {
            *pnEndOffset = nPosition;
            return false;
        }
        if (pbaOut) pbaOut->append((const char *)(pData + nPosition), (qint32)nLength);
        nCount += nLength;
        nPosition += nLength;
    }
    *pnEndOffset = nPosition;

    return false;
}
}  // namespace

const qint64 XVMARCDecoder::MAX_UNCOMPRESSED_SIZE;

QByteArray XVMARCDecoder::packProperties(quint16 nLRECL, bool bFixed, quint8 nMode)
{
    QByteArray baResult(4, (char)0);
    baResult[0] = (char)(quint8)(nLRECL & 0xff);
    baResult[1] = (char)(quint8)((nLRECL >> 8) & 0xff);
    baResult[2] = (char)(quint8)(bFixed ? 1 : 0);
    baResult[3] = (char)nMode;

    return baResult;
}

bool XVMARCDecoder::unpackProperties(const QByteArray &baProperties, quint16 *pnLRECL, bool *pbFixed, quint8 *pnMode)
{
    if (!pnLRECL || !pbFixed || !pnMode) return false;
    if (baProperties.size() != 4) return false;
    const quint8 *pData = (const quint8 *)baProperties.constData();
    *pnLRECL = (quint16)(pData[0] | ((quint16)pData[1] << 8));
    *pbFixed = (pData[2] != 0);
    *pnMode = pData[3];

    return true;
}

bool XVMARCDecoder::run(const QByteArray &baData, qint64 nOffset, quint16 nLRECL, bool bFixed, quint8 nMode, QByteArray *pbaOut, qint64 *pnEndOffset,
                        XBinary::PDSTRUCT *pPdStruct)
{
    if (!pnEndOffset) return false;
    const qint64 nSize = baData.size();
    if ((nOffset < 0) || (nOffset > nSize)) return false;
    const quint8 *pData = (const quint8 *)baData.constData();
    *pnEndOffset = nOffset;

    if (nMode == MODE_STORED) {
        return vmarcStored(pData, nOffset, nSize, pbaOut, pnEndOffset, MAX_UNCOMPRESSED_SIZE);
    }

    BitReader bits(pData, nOffset, nSize);
    RecordSink sink(pbaOut, nLRECL, bFixed, MAX_UNCOMPRESSED_SIZE);
    const bool bResult = vmarcLzw(&bits, &sink, pPdStruct);
    *pnEndOffset = bits.getPosition();

    return bResult;
}

bool XVMARCDecoder::decode(const QByteArray &baPacked, const QByteArray &baProperties, qint64 nUncompressedSize, QByteArray *pbaResult,
                           XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    if ((nUncompressedSize < 0) || (nUncompressedSize > MAX_UNCOMPRESSED_SIZE)) return false;

    quint16 nLRECL = 0;
    bool bFixed = false;
    quint8 nMode = MODE_LZW;
    if (!unpackProperties(baProperties, &nLRECL, &bFixed, &nMode)) return false;

    QByteArray baOut;
    baOut.reserve((qint32)qMin<qint64>(nUncompressedSize + 1, 0x400000));
    qint64 nEndOffset = 0;
    // A member that runs off the end of the container still keeps everything
    // that was produced, which is what the reference does.
    run(baPacked, 0, nLRECL, bFixed, nMode, &baOut, &nEndOffset, pPdStruct);

    if ((qint64)baOut.size() != nUncompressedSize) return false;
    *pbaResult = baOut;

    return true;
}
