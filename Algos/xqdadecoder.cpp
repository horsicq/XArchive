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
#include "xqdadecoder.h"

#include <QtEndian>

#include <limits>

namespace {
const qint32 QDA_ALPHABET_SIZE = 0x100;
const qint32 QDA_SKIP_BIAS = 0x7f;
const qint32 QDA_STACK_SIZE = 0x100;
const qint32 QDA_STACK_LIMIT = 0xfe;
}  // namespace

bool XQDADecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > (qint64)(std::numeric_limits<qint32>::max)())) return false;

    QByteArray baOut((qint32)nUncompressedSize, (char)0);
    if (baOut.size() != (qint32)nUncompressedSize) return false;

    const quint8 *pData = (const quint8 *)baPacked.constData();
    const qint64 nSize = baPacked.size();
    quint8 *pOut = (quint8 *)baOut.data();
    qint64 nProduced = 0;
    qint64 nPosition = 0;

    quint8 nLeft[QDA_ALPHABET_SIZE];
    quint8 nRight[QDA_ALPHABET_SIZE];
    quint8 nStack[QDA_STACK_SIZE];
    // The right table is NOT reset per block; only the left one is.
    for (qint32 i = 0; i < QDA_ALPHABET_SIZE; ++i) nRight[i] = 0;

    while (nPosition < nSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        for (qint32 i = 0; i < QDA_ALPHABET_SIZE; ++i) nLeft[i] = (quint8)i;
        qint32 nCode = 0;

        for (;;) {
            if (nPosition >= nSize) return false;
            qint32 nControl = pData[nPosition];
            ++nPosition;
            if (nControl > QDA_SKIP_BIAS) {
                nCode += nControl - QDA_SKIP_BIAS;
                nControl = 0;
            }
            if (nCode == QDA_ALPHABET_SIZE) break;

            for (qint32 i = 0; i <= nControl; ++i) {
                if ((nCode > (QDA_ALPHABET_SIZE - 1)) || (nPosition >= nSize)) return false;
                nLeft[nCode] = pData[nPosition];
                ++nPosition;
                if ((quint8)nCode != nLeft[nCode]) {
                    if (nPosition >= nSize) return false;
                    nRight[nCode] = pData[nPosition];
                    ++nPosition;
                }
                ++nCode;
            }
            // The completion test is made twice on purpose - once right after
            // the skip above and once here; a table that fills on its last
            // entry run never reaches another control byte.
            if (nCode == QDA_ALPHABET_SIZE) break;
        }

        if ((nSize - nPosition) < 4) return false;
        qint32 nBlockLength = (qint32)qFromLittleEndian<quint32>(pData + nPosition);
        nPosition += 4;

        qint32 nStackDepth = 0;
        for (;;) {
            quint8 nSymbol = 0;
            if (nStackDepth == 0) {
                if (nBlockLength == 0) break;
                --nBlockLength;
                if (nPosition >= nSize) return false;
                nSymbol = pData[nPosition];
                ++nPosition;
            } else {
                --nStackDepth;
                nSymbol = nStack[nStackDepth];
            }

            if (nSymbol == nLeft[nSymbol]) {
                // The reference emits first and only then notices it has passed
                // the declared length, so an overlong member is a failed member
                // rather than a truncated one.
                if (nProduced >= nUncompressedSize) return false;
                pOut[nProduced] = nSymbol;
                ++nProduced;
            } else {
                if (nStackDepth > QDA_STACK_LIMIT) return false;
                nStack[nStackDepth] = nRight[nSymbol];
                nStack[nStackDepth + 1] = nLeft[nSymbol];
                nStackDepth += 2;
            }
        }
    }

    if (nProduced != nUncompressedSize) return false;

    *pbaResult = baOut;

    return true;
}
