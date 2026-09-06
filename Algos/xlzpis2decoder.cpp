/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xlzpis2decoder.h"

#include "algo_utils.h"

#include <string.h>

namespace {

const qint32 LZ_N = 8192;            // ring buffer
const qint32 LZ_F = 66;              // longest ordinary match
const qint32 LZ_THRESHOLD = 2;       // shortest coded match is THRESHOLD + 1
const qint32 LZ_N_CHAR = 320;        // 256 literals + 64 length symbols
const qint32 LZ_T = LZ_N_CHAR * 2 - 1;
const qint32 LZ_R = LZ_T - 1;
const quint16 LZ_MAX_FREQ = 0x8000;
const qint32 LZ_MAX_LENGTH = LZ_F + 255;  // symbol 319 plus its 8-bit escape

// MSB-first bit reader.  Reads past the end return zero bits so that the very
// last token of a chunk may legally peek into the padding; the caller checks
// bytesUsed() against the chunk's declared compressed size, which for every
// chunk of the reference corpus is an exact fit or better.
class LzBitReader {
public:
    LzBitReader(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPos(0), m_nAcc(0), m_nBits(0), m_nUsed(0)
    {
    }

    quint32 peek(qint32 nCount)
    {
        fill(nCount);
        return (quint32)((m_nAcc >> (m_nBits - nCount)) & (((quint32)1 << nCount) - 1));
    }

    quint32 get(qint32 nCount)
    {
        if (nCount <= 0) return 0;
        fill(nCount);
        m_nBits -= nCount;
        m_nUsed += nCount;
        return (quint32)((m_nAcc >> m_nBits) & (((quint32)1 << nCount) - 1));
    }

    quint32 getBit()
    {
        return get(1);
    }

    qint64 bytesUsed() const
    {
        return (m_nUsed + 7) / 8;
    }

private:
    void fill(qint32 nCount)
    {
        while (m_nBits < nCount) {
            const quint32 nByte = (m_nPos < m_nSize) ? (quint32)m_pData[m_nPos] : (quint32)0;
            m_nPos++;
            m_nAcc = (m_nAcc << 8) | nByte;
            m_nBits += 8;
        }
    }

    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPos;
    quint32 m_nAcc;
    qint32 m_nBits;
    qint64 m_nUsed;
};

// Position high part: canonical prefix code, complete over 9 bits.
// len 3 x 2 | len 4 x 2 | len 5 x 4 | len 6 x 6 | len 7 x 20 | len 8 x 34 | len 9 x 60
static void posLookup(quint32 nPeek9, qint32 *pnValue, qint32 *pnLen)
{
    if (nPeek9 < 128) {
        *pnLen = 3;
        *pnValue = (qint32)(nPeek9 >> 6);
    } else if (nPeek9 < 192) {
        *pnLen = 4;
        *pnValue = 2 + (qint32)((nPeek9 - 128) >> 5);
    } else if (nPeek9 < 256) {
        *pnLen = 5;
        *pnValue = 4 + (qint32)((nPeek9 - 192) >> 4);
    } else if (nPeek9 < 304) {
        *pnLen = 6;
        *pnValue = 8 + (qint32)((nPeek9 - 256) >> 3);
    } else if (nPeek9 < 384) {
        *pnLen = 7;
        *pnValue = 14 + (qint32)((nPeek9 - 304) >> 2);
    } else if (nPeek9 < 452) {
        *pnLen = 8;
        *pnValue = 34 + (qint32)((nPeek9 - 384) >> 1);
    } else {
        *pnLen = 9;
        *pnValue = 68 + (qint32)(nPeek9 - 452);
    }
}

// Okumura's adaptive Huffman tree, verbatim in structure.
class LzHuffTree {
public:
    LzHuffTree()
    {
        start();
    }

    qint32 decodeChar(LzBitReader *pReader)
    {
        qint32 nNode = (qint32)m_son[LZ_R];

        qint32 nGuard = 0;

        while (nNode < LZ_T) {
            if ((nNode < 0) || (++nGuard > LZ_T)) return -1;
            nNode += (qint32)pReader->getBit();
            if ((nNode < 0) || (nNode >= LZ_T)) return -1;
            nNode = (qint32)m_son[nNode];
        }

        nNode -= LZ_T;

        if ((nNode < 0) || (nNode >= LZ_N_CHAR)) return -1;

        update(nNode);

        return nNode;
    }

private:
    void start()
    {
        memset(m_freq, 0, sizeof(m_freq));
        memset(m_son, 0, sizeof(m_son));
        memset(m_prnt, 0, sizeof(m_prnt));

        for (qint32 i = 0; i < LZ_N_CHAR; i++) {
            m_freq[i] = 1;
            m_son[i] = (quint16)(i + LZ_T);
            m_prnt[i + LZ_T] = (quint16)i;
        }

        qint32 i = 0;
        qint32 j = LZ_N_CHAR;

        while (j <= LZ_R) {
            m_freq[j] = (quint16)(m_freq[i] + m_freq[i + 1]);
            m_son[j] = (quint16)i;
            m_prnt[i] = (quint16)j;
            m_prnt[i + 1] = (quint16)j;
            i += 2;
            j++;
        }

        m_freq[LZ_T] = 0xFFFF;
        m_prnt[LZ_R] = 0;
    }

    void reconst()
    {
        qint32 j = 0;

        for (qint32 i = 0; i < LZ_T; i++) {
            if (m_son[i] >= LZ_T) {
                m_freq[j] = (quint16)((m_freq[i] + 1) / 2);
                m_son[j] = m_son[i];
                j++;
            }
        }

        qint32 i = 0;
        j = LZ_N_CHAR;

        while (j < LZ_T) {
            const quint16 nFreq = (quint16)(m_freq[i] + m_freq[i + 1]);
            m_freq[j] = nFreq;

            qint32 k = j - 1;
            while ((k >= 0) && (nFreq < m_freq[k])) k--;
            k++;

            for (qint32 m = j; m > k; m--) {
                m_freq[m] = m_freq[m - 1];
                m_son[m] = m_son[m - 1];
            }

            m_freq[k] = nFreq;
            m_son[k] = (quint16)i;

            i += 2;
            j++;
        }

        for (qint32 n = 0; n < LZ_T; n++) {
            const qint32 k = (qint32)m_son[n];
            if (k >= LZ_T) {
                m_prnt[k] = (quint16)n;
            } else {
                m_prnt[k] = (quint16)n;
                m_prnt[k + 1] = (quint16)n;
            }
        }
    }

    void update(qint32 nChar)
    {
        if (m_freq[LZ_R] == LZ_MAX_FREQ) {
            reconst();
        }

        qint32 c = (qint32)m_prnt[nChar + LZ_T];

        qint32 nGuard = 0;

        while (true) {
            if ((c < 0) || (c >= LZ_T) || (++nGuard > (LZ_T * 2))) return;

            m_freq[c]++;
            const quint16 k = m_freq[c];
            qint32 l = c + 1;

            if ((l <= LZ_T) && (k > m_freq[l])) {
                while ((l < LZ_T) && (k > m_freq[l])) l++;
                l--;

                if ((l < 0) || (l >= LZ_T)) return;

                m_freq[c] = m_freq[l];
                m_freq[l] = k;

                const qint32 i = (qint32)m_son[c];
                if ((i < 0) || (i >= (LZ_T + LZ_N_CHAR))) return;
                m_prnt[i] = (quint16)l;
                if (i < LZ_T) m_prnt[i + 1] = (quint16)l;

                const qint32 j = (qint32)m_son[l];
                if ((j < 0) || (j >= (LZ_T + LZ_N_CHAR))) return;
                m_son[l] = (quint16)i;
                m_prnt[j] = (quint16)c;
                if (j < LZ_T) m_prnt[j + 1] = (quint16)c;

                m_son[c] = (quint16)j;
                c = l;
            }

            c = (qint32)m_prnt[c];

            if (c == 0) break;
        }
    }

    quint16 m_freq[LZ_T + 1];
    quint16 m_son[LZ_T];
    quint16 m_prnt[LZ_T + LZ_N_CHAR];
};

}  // namespace

XLzpis2Decoder::XLzpis2Decoder(QObject *parent) : QObject(parent)
{
}

bool XLzpis2Decoder::decodeChunk(const quint8 *pIn, qint64 nInSize, quint8 *pOut, qint64 nOutSize)
{
    if ((!pIn) || (!pOut) || (nInSize <= 0) || (nOutSize <= 0) || (nOutSize > LZPIS2_MAX_CHUNK_SIZE)) {
        return false;
    }

    LzBitReader reader(pIn, nInSize);
    LzHuffTree tree;

    quint8 window[LZ_N];
    memset(window, 0, sizeof(window));

    qint32 r = LZ_N - LZ_F;
    qint64 nCount = 0;

    while (nCount < nOutSize) {
        const qint32 nSymbol = tree.decodeChar(&reader);

        if (nSymbol < 0) return false;

        if (nSymbol < 256) {
            pOut[nCount++] = (quint8)nSymbol;
            window[r] = (quint8)nSymbol;
            r = (r + 1) & (LZ_N - 1);
        } else {
            qint32 nLength = nSymbol - 255 + LZ_THRESHOLD;

            if (nSymbol == (LZ_N_CHAR - 1)) {
                nLength += (qint32)reader.get(8);
            }

            if ((nLength <= 0) || (nLength > LZ_MAX_LENGTH)) return false;

            qint32 nPosValue = 0;
            qint32 nPosLen = 0;
            posLookup(reader.peek(9), &nPosValue, &nPosLen);
            reader.get(nPosLen);

            const qint32 nPosition = (nPosValue << 6) | (qint32)reader.get(6);

            if ((nPosition < 0) || (nPosition >= LZ_N)) return false;

            const qint32 i = (r - nPosition - 1) & (LZ_N - 1);

            for (qint32 k = 0; k < nLength; k++) {
                if (nCount >= nOutSize) break;

                const quint8 nByte = window[(i + k) & (LZ_N - 1)];
                pOut[nCount++] = nByte;
                window[r] = nByte;
                r = (r + 1) & (LZ_N - 1);
            }
        }

        // A well-formed chunk never reads outside its own compressed bytes.
        if (reader.bytesUsed() > nInSize) return false;
    }

    return (nCount == nOutSize) && (reader.bytesUsed() <= nInSize);
}

bool XLzpis2Decoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if ((!pDecompressState) || (!pDecompressState->pDeviceInput) || (!pDecompressState->pDeviceOutput) || (pDecompressState->nInputOffset < 0) ||
        (pDecompressState->nInputLimit < 0)) {
        return false;
    }

    const qint64 nInputSize = pDecompressState->nInputLimit;

    if ((nInputSize < (LZPIS2_MAGIC_SIZE + LZPIS2_CHUNK_HEADER_SIZE + 1)) || (nInputSize > LZPIS2_MAX_INPUT_SIZE)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);

    if (pDecompressState->bReadError || pDecompressState->bWriteError || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    XBinary::UNPACK_MEMORY_RESERVATION inputReservation;

    if (!inputReservation.acquire(pDecompressState->mapUnpackProperties, nInputSize)) {
        return false;
    }

    QByteArray baInput;
    baInput.resize((qint32)nInputSize);

    if (baInput.size() != (qint32)nInputSize) {
        pDecompressState->bReadError = true;
        return false;
    }

    {
        qint64 nDone = 0;

        while (nDone < nInputSize) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

            const qint64 nPortion = qMin<qint64>(0x10000, nInputSize - nDone);
            const qint32 nRead = XBinary::_readDevice(baInput.data() + nDone, (qint32)nPortion, pDecompressState);

            if (nRead <= 0) {
                pDecompressState->bReadError = true;
                return false;
            }

            nDone += nRead;
        }
    }

    const quint8 *pData = (const quint8 *)baInput.constData();

    if (memcmp(pData, "LZPIS2", (size_t)LZPIS2_MAGIC_SIZE) != 0) {
        return false;
    }

    // First pass: the chain must tile the container exactly, and it tells us
    // how large the output will be before a single byte is produced.
    qint64 nTotalUncompressed = 0;

    {
        qint64 nPos = LZPIS2_MAGIC_SIZE;

        while (nPos < nInputSize) {
            if ((nPos + LZPIS2_CHUNK_HEADER_SIZE) > nInputSize) return false;

            const qint64 nUnpacked = (qint64)pData[nPos] | ((qint64)pData[nPos + 1] << 8);
            const qint64 nPacked = (qint64)pData[nPos + 2] | ((qint64)pData[nPos + 3] << 8);

            if ((nUnpacked <= 0) || (nUnpacked > LZPIS2_MAX_CHUNK_SIZE) || (nPacked <= 0)) return false;
            if ((nPos + LZPIS2_CHUNK_HEADER_SIZE + nPacked) > nInputSize) return false;

            nTotalUncompressed += nUnpacked;

            if (nTotalUncompressed > LZPIS2_MAX_OUTPUT_SIZE) return false;

            nPos += LZPIS2_CHUNK_HEADER_SIZE + nPacked;
        }

        if (nPos != nInputSize) return false;
    }

    if (nTotalUncompressed <= 0) return false;

    if (pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        bool bConverted = false;
        const qint64 nDeclaredSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bConverted);
        if (!bConverted || (nDeclaredSize != nTotalUncompressed)) return false;
    }

    qint64 nOutputLimit = -1;

    if (!XBinary::getUnpackOutputLimit(pDecompressState->mapUnpackProperties, &nOutputLimit) ||
        !XBinary::isUnpackOutputSizeAllowed(pDecompressState->mapUnpackProperties, nTotalUncompressed)) {
        return false;
    }

    if ((nOutputLimit >= 0) && (nTotalUncompressed > nOutputLimit)) {
        return false;
    }

    quint8 chunkBuffer[4096];  // == LZPIS2_MAX_CHUNK_SIZE

    qint64 nPos = LZPIS2_MAGIC_SIZE;

    while (nPos < nInputSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const qint64 nUnpacked = (qint64)pData[nPos] | ((qint64)pData[nPos + 1] << 8);
        const qint64 nPacked = (qint64)pData[nPos + 2] | ((qint64)pData[nPos + 3] << 8);

        nPos += LZPIS2_CHUNK_HEADER_SIZE;

        if (!decodeChunk(pData + nPos, nPacked, chunkBuffer, nUnpacked)) {
            return false;
        }

        if (XBinary::_writeDevice((const char *)chunkBuffer, (qint32)nUnpacked, pDecompressState) != (qint32)nUnpacked) {
            return false;
        }

        nPos += nPacked;
    }

    return !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}
