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
#include "xpakleodecoder.h"

namespace {
const qint32 LEO_CAPACITY = 0x8000;
const qint32 LEO_FIRST_FREE = 0x103;
const qint32 LEO_CODE_EOF = 0x100;
const qint32 LEO_CODE_WIDEN = 0x101;
const qint32 LEO_CODE_CLEAR = 0x102;
const qint32 LEO_MAX_WIDTH = 15;

// Refill one byte, hand out bits MSB first.  -1 means the stream is exhausted.
class LeoBitReader {
public:
    LeoBitReader(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nCurrent(0), m_nCount(0)
    {
    }

    qint32 get(qint32 nWidth)
    {
        qint32 nValue = 0;
        for (qint32 i = 0; i < nWidth; ++i) {
            if (m_nCount == 0) {
                if (m_nPosition >= m_nSize) return -1;
                m_nCurrent = m_pData[m_nPosition];
                ++m_nPosition;
                m_nCount = 8;
            }
            nValue = (nValue << 1) | ((m_nCurrent >> 7) & 1);
            m_nCurrent = (quint8)((m_nCurrent << 1) & 0xff);
            --m_nCount;
        }

        return nValue;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint8 m_nCurrent;
    qint32 m_nCount;
};
}  // namespace

bool XPAKLEODecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > MAX_UNCOMPRESSED_SIZE)) return false;

    LeoBitReader reader((const quint8 *)baPacked.constData(), baPacked.size());

    QByteArray baPrefix(LEO_CAPACITY * (qint32)sizeof(quint16), (char)0);
    QByteArray baSuffix(LEO_CAPACITY, (char)0);
    quint16 *pPrefix = (quint16 *)baPrefix.data();
    quint8 *pSuffix = (quint8 *)baSuffix.data();
    for (qint32 i = 0; i < 256; ++i) pSuffix[i] = (quint8)i;

    QByteArray baStack(LEO_CAPACITY, (char)0);
    quint8 *pStack = (quint8 *)baStack.data();

    QByteArray baOut;
    qint64 nLeft = nUncompressedSize;
    bool bDone = false;

    while (!bDone) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        // entry point, and where a clear code comes back to
        qint32 nCode = reader.get(9);
        if ((nCode < 0) || (nCode == LEO_CODE_EOF) || (nCode > 0xff)) break;

        baOut.append((char)(quint8)nCode);
        if (nLeft > 0) --nLeft;
        qint32 nFirst = nCode;
        qint32 nPrevious = nCode;
        qint32 nFree = LEO_FIRST_FREE;
        qint32 nWidth = 9;

        bool bRestart = false;
        while (!bRestart) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

            qint32 nCurrent = -1;
            while (true) {  // swallow every 0x101
                if (nLeft == 0) {
                    bDone = true;
                    break;
                }
                nCurrent = reader.get(nWidth);
                if ((nCurrent < 0) || (nCurrent == LEO_CODE_EOF)) {
                    bDone = true;
                    break;
                }
                if (nCurrent != LEO_CODE_WIDEN) break;
                if (nWidth < LEO_MAX_WIDTH) ++nWidth;
            }
            if (bDone) break;

            if (nCurrent == LEO_CODE_CLEAR) {
                bRestart = true;
                break;
            }

            qint32 nStackSize = 0;
            qint32 c = nCurrent;
            if (nCurrent >= nFree) {  // KwKwK
                pStack[nStackSize++] = (quint8)nFirst;
                c = nPrevious;
            }
            bool bBad = false;
            while (c > 0xff) {
                if ((c >= LEO_CAPACITY) || (nStackSize >= LEO_CAPACITY)) {
                    bBad = true;
                    break;
                }
                pStack[nStackSize++] = pSuffix[c];
                c = pPrefix[c];
            }
            if (bBad) {
                bDone = true;
                break;
            }
            if (nStackSize >= LEO_CAPACITY) {
                bDone = true;
                break;
            }
            pStack[nStackSize++] = (quint8)c;
            nFirst = c;

            for (qint32 i = nStackSize - 1; i >= 0; --i) {
                baOut.append((char)pStack[i]);
                if (nLeft > 0) --nLeft;
            }

            if (nFree < LEO_CAPACITY) {
                pPrefix[nFree] = (quint16)nPrevious;
                pSuffix[nFree] = (quint8)nFirst;
                ++nFree;
            }
            nPrevious = nCurrent;

            if (baOut.size() > nUncompressedSize) {
                bDone = true;
                break;
            }
        }
    }

    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
