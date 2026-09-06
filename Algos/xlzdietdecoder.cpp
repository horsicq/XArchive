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
#include "xlzdietdecoder.h"

#include <QtEndian>

#include <cstring>

namespace {
const qint32 LZDIET_MAX_BITS = 10;
const qint32 LZDIET_MAX_CODES = 1 << LZDIET_MAX_BITS;  // 1024
const qint32 LZDIET_CLEAR_CODE = 0x100;
const qint32 LZDIET_END_CODE = 0x101;
const qint32 LZDIET_FIRST_CODE = 0x102;

class LzDietLzw {
public:
    LzDietLzw(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPos(0), m_nAccumulator(0), m_nBitCount(0)
    {
    }

    // Decodes one chunk, appending at most nMaxOutput bytes to *pbaOutput.
    // Returns false only on a malformed stream; running out of input, hitting
    // the end code and hitting the output limit are all normal terminations.
    bool run(QByteArray *pbaOutput, qint64 nMaxOutput)
    {
        quint16 nPrefix[LZDIET_MAX_CODES];
        quint8 nSuffix[LZDIET_MAX_CODES];
        quint8 nStack[LZDIET_MAX_CODES + 1];

        for (qint32 i = 0; i < LZDIET_MAX_CODES; i++) {
            nPrefix[i] = 0;
            nSuffix[i] = (i < 256) ? (quint8)i : 0;
        }

        m_nBits = 9;
        m_nMaxCode = 0x1ff;
        m_nNextFree = LZDIET_FIRST_CODE;

        qint32 nOldCode = 0;
        qint32 nFirstChar = 0;
        qint64 nWritten = 0;

        while (true) {
            if (nWritten >= nMaxOutput) return true;

            qint32 nCode = 0;
            if (!readCode(&nCode)) return true;  // input exhausted: end of chunk

            if (nCode == LZDIET_END_CODE) return true;

            if (nCode == LZDIET_CLEAR_CODE) {
                m_nBits = 9;
                m_nMaxCode = 0x1ff;
                m_nNextFree = LZDIET_FIRST_CODE;
                if (!readCode(&nCode)) return false;
                if ((nCode == LZDIET_CLEAR_CODE) || (nCode == LZDIET_END_CODE)) return true;
                if (nCode > 0xff) return false;
                nOldCode = nCode;
                nFirstChar = nCode;
                pbaOutput->append((char)(quint8)nCode);
                nWritten++;
                continue;
            }

            qint32 nStackTop = 0;
            qint32 nWork = nCode;
            if (m_nNextFree <= nCode) {
                // KwKwK: the code being read is the one about to be defined.
                nStack[nStackTop++] = (quint8)nFirstChar;
                nWork = nOldCode;
            }
            while (nWork > 0xff) {
                if ((nWork >= LZDIET_MAX_CODES) || (nStackTop >= LZDIET_MAX_CODES)) return false;
                nStack[nStackTop++] = nSuffix[nWork];
                nWork = nPrefix[nWork];
            }
            if (nStackTop >= LZDIET_MAX_CODES + 1) return false;
            nStack[nStackTop++] = (quint8)nWork;
            nFirstChar = nWork;

            while (nStackTop > 0) {
                nStackTop--;
                pbaOutput->append((char)nStack[nStackTop]);
                nWritten++;
                if (nWritten >= nMaxOutput) return true;
            }

            if (m_nNextFree < LZDIET_MAX_CODES) {
                nPrefix[m_nNextFree] = (quint16)nOldCode;
                nSuffix[m_nNextFree] = (quint8)nWork;
                m_nNextFree++;
            }
            nOldCode = nCode;
        }
    }

private:
    bool readCode(qint32 *pnCode)
    {
        // The width grows before the code that needs it is fetched.
        if (m_nMaxCode < m_nNextFree) {
            m_nBits++;
            m_nMaxCode = (m_nBits == LZDIET_MAX_BITS) ? LZDIET_MAX_CODES : ((1 << m_nBits) - 1);
        }
        while (m_nBitCount < m_nBits) {
            if (m_nPos >= m_nSize) return false;
            m_nAccumulator |= ((quint32)m_pData[m_nPos]) << m_nBitCount;
            m_nPos++;
            m_nBitCount += 8;
        }
        *pnCode = (qint32)(m_nAccumulator & (quint32)((1u << m_nBits) - 1u));
        m_nAccumulator >>= m_nBits;
        m_nBitCount -= m_nBits;
        return true;
    }

    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPos;
    quint32 m_nAccumulator;
    qint32 m_nBitCount;
    qint32 m_nBits;
    qint32 m_nMaxCode;
    qint32 m_nNextFree;
};
}  // namespace

bool XLZDIETDecoder::decode(const QByteArray &baContainer, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();

    if ((nUncompressedSize < 0) || (nUncompressedSize > (qint64)0x7fffffff)) return false;
    if (baContainer.size() < LZDIET_HEADER_SIZE) return false;
    if (memcmp(baContainer.constData(), "lZdIeT", 6) != 0) return false;

    const quint8 *pContainer = (const quint8 *)baContainer.constData();
    const qint64 nContainerSize = baContainer.size();

    if ((qint64)qFromLittleEndian<qint32>(pContainer + 6) != nUncompressedSize) return false;

    pbaResult->reserve((qint32)nUncompressedSize);

    for (qint32 i = 0; i < LZDIET_MAX_CHUNKS; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if ((qint64)pbaResult->size() >= nUncompressedSize) break;

        const quint8 *pEntry = pContainer + LZDIET_TABLE_OFFSET + ((qint64)i * LZDIET_ENTRY_SIZE);
        const qint64 nChunkOffset = (qint64)qFromLittleEndian<qint32>(pEntry);
        const qint64 nChunkSize = (qint64)qFromLittleEndian<quint16>(pEntry + 4);

        if (nChunkOffset == -1) break;
        if ((nChunkOffset < LZDIET_HEADER_SIZE) || (nChunkSize < LZDIET_CHUNK_PREAMBLE)) return false;

        const qint64 nStreamOffset = nChunkOffset + LZDIET_CHUNK_PREAMBLE;
        const qint64 nStreamSize = nChunkSize - LZDIET_CHUNK_PREAMBLE;
        if ((nStreamOffset > nContainerSize) || (nStreamSize > nContainerSize - nStreamOffset)) return false;

        LzDietLzw decoder(pContainer + nStreamOffset, nStreamSize);
        if (!decoder.run(pbaResult, nUncompressedSize - (qint64)pbaResult->size())) return false;
    }

    return ((qint64)pbaResult->size() == nUncompressedSize) && XBinary::isPdStructNotCanceled(pPdStruct);
}
