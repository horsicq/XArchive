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
#include "xlzhcxpdecoder.h"

#include <QVector>

namespace {
const qint32 LZHCXP_CODE_CLEAR = 0x200;
const qint32 LZHCXP_CODE_END = 0x201;
const qint32 LZHCXP_FIRST_FREE = 0x202;
const qint32 LZHCXP_MAX_CODE = 0x1000;
const qint32 LZHCXP_MIN_WIDTH = 10;
const qint32 LZHCXP_MAX_WIDTH = 12;

// Block-framed, LSB-first bit reader: [u8 length][length bytes] ... [0].
class LzhcxpBitReader {
public:
    explicit LzhcxpBitReader(const QByteArray &baInput)
        : m_pData(reinterpret_cast<const quint8 *>(baInput.constData())),
          m_nSize(baInput.size()), m_nPosition(0), m_nBlockLeft(0),
          m_nAccumulator(0), m_nBitCount(0)
    {
    }

    bool readBits(qint32 nWidth, qint32 *pnValue)
    {
        while (m_nBitCount < nWidth) {
            qint32 nByte = 0;
            if (!readByte(&nByte)) return false;
            m_nAccumulator |= quint32(nByte) << m_nBitCount;
            m_nBitCount += 8;
        }
        *pnValue = qint32(m_nAccumulator & ((1U << nWidth) - 1U));
        m_nAccumulator >>= nWidth;
        m_nBitCount -= nWidth;
        return true;
    }

private:
    bool readByte(qint32 *pnByte)
    {
        if (m_nBlockLeft == 0) {
            if (m_nPosition >= m_nSize) return false;
            m_nBlockLeft = qint32(m_pData[m_nPosition++]);
            // A zero-length block is the end-of-stream marker.
            if (m_nBlockLeft < 1) return false;
        }
        if (m_nPosition >= m_nSize) return false;
        *pnByte = qint32(m_pData[m_nPosition++]);
        --m_nBlockLeft;
        return true;
    }

    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    qint32 m_nBlockLeft;
    quint32 m_nAccumulator;
    qint32 m_nBitCount;
};
}  // namespace

bool XLzhcxpDecoder::decode(const QByteArray &baInput, qint64 nExpectedSize,
                            qint64 nMaxOutput, QByteArray *pbaResult)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if (baInput.isEmpty()) return false;

    LzhcxpBitReader reader(baInput);

    QVector<quint16> listPrefix(LZHCXP_MAX_CODE, 0);
    QVector<quint8> listSuffix(LZHCXP_MAX_CODE, 0);
    for (qint32 i = 0; i < 256; ++i) {
        listSuffix[i] = quint8(i);
    }
    QByteArray baStack(LZHCXP_MAX_CODE, 0);

    qint32 nWidth = LZHCXP_MIN_WIDTH;
    qint32 nNextFree = LZHCXP_FIRST_FREE;
    qint32 nLimit = 1 << LZHCXP_MIN_WIDTH;
    qint32 nPrevious = 0;
    qint32 nFirstCharacter = 0;
    bool bFinished = false;

    while (!bFinished) {
        // The widening test happens before the read, on the CURRENT next-free
        // code; there is no "early change" fudge here.
        if ((nLimit <= nNextFree) && (nWidth < LZHCXP_MAX_WIDTH)) {
            ++nWidth;
            nLimit = 1 << nWidth;
        }
        qint32 nCode = 0;
        if (!reader.readBits(nWidth, &nCode)) {
            // Exhausted input is the ordinary way these streams stop.
            break;
        }
        const qint32 nOriginalCode = nCode;

        if (nCode == LZHCXP_CODE_CLEAR) {
            nWidth = LZHCXP_MIN_WIDTH;
            nNextFree = LZHCXP_FIRST_FREE;
            nLimit = 1 << LZHCXP_MIN_WIDTH;
            qint32 nSeed = 0;
            if (!reader.readBits(nWidth, &nSeed)) return false;
            nPrevious = nSeed;
            nFirstCharacter = nSeed;
            if ((nMaxOutput >= 0) && (pbaResult->size() >= nMaxOutput)) {
                return false;
            }
            pbaResult->append(char(quint8(nSeed & 0xff)));
            continue;
        }
        if (nCode == LZHCXP_CODE_END) break;
        if ((nCode > 0xff) && (nCode < LZHCXP_CODE_CLEAR)) return false;

        qint32 nStackTop = 0;
        if (nCode >= nNextFree) {
            // KwKwK: the code that is not in the table yet.
            baStack[nStackTop++] = char(quint8(nFirstCharacter & 0xff));
            nCode = nPrevious;
        }
        while (nCode > 0xff) {
            if (nStackTop >= LZHCXP_MAX_CODE) return false;
            if (nCode >= LZHCXP_MAX_CODE) return false;
            baStack[nStackTop++] = char(listSuffix.at(nCode));
            nCode = qint32(listPrefix.at(nCode));
        }
        nFirstCharacter = nCode;
        if (nStackTop >= LZHCXP_MAX_CODE) return false;
        baStack[nStackTop++] = char(quint8(nCode & 0xff));

        if ((nMaxOutput >= 0) &&
            ((qint64(nStackTop) + pbaResult->size()) > nMaxOutput)) {
            return false;
        }
        while (nStackTop > 0) {
            --nStackTop;
            pbaResult->append(baStack.at(nStackTop));
        }

        if (nNextFree < LZHCXP_MAX_CODE) {
            listPrefix[nNextFree] = quint16(nPrevious);
            listSuffix[nNextFree] = quint8(nCode & 0xff);
            ++nNextFree;
        }
        nPrevious = nOriginalCode;
    }

    if ((nExpectedSize >= 0) && (pbaResult->size() != nExpectedSize)) {
        return false;
    }
    return true;
}
