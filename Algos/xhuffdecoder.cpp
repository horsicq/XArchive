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
#include "xhuffdecoder.h"

namespace {
// Byte-wise LSB-first bit reader.  The original keeps a "bits used" counter
// that starts at 8, so the very first request pulls a byte in; that is exactly
// what this reproduces.
class HuffBitReader {
public:
    explicit HuffBitReader(const QByteArray &baInput)
        : m_pData(reinterpret_cast<const quint8 *>(baInput.constData())),
          m_nSize(baInput.size()), m_nPosition(0), m_nUsed(8), m_nByte(0)
    {
    }

    bool readBit(qint32 *pnBit)
    {
        if (m_nUsed == 8) {
            if (m_nPosition >= m_nSize) return false;
            m_nByte = m_pData[m_nPosition++];
            m_nUsed = 0;
        }
        *pnBit = (m_nByte & (1U << m_nUsed)) ? 1 : 0;
        ++m_nUsed;
        return true;
    }

    qint64 bytesUsed() const
    {
        return m_nPosition;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    qint32 m_nUsed;
    quint8 m_nByte;
};
}  // namespace

QByteArray XHuffDecoder::packTree(const QByteArray &baSymbols,
                                  const QByteArray &baTreeBits)
{
    QByteArray baResult;
    if ((baSymbols.size() <= 0) || (baSymbols.size() > MAX_SYMBOLS)) {
        return baResult;
    }
    const quint32 nCount = quint32(baSymbols.size());
    baResult.append(char(quint8(nCount & 0xff)));
    baResult.append(char(quint8((nCount >> 8) & 0xff)));
    baResult.append(baSymbols);
    baResult.append(baTreeBits);
    return baResult;
}

bool XHuffDecoder::unpackTree(const QByteArray &baProperty,
                              QByteArray *pbaSymbols, QByteArray *pbaTreeBits)
{
    if (!pbaSymbols || !pbaTreeBits || (baProperty.size() < 2)) return false;
    const qint32 nCount = qint32(quint8(baProperty.at(0))) |
                          (qint32(quint8(baProperty.at(1))) << 8);
    if ((nCount <= 0) || (nCount > MAX_SYMBOLS)) return false;
    if (baProperty.size() < (2 + nCount)) return false;
    *pbaSymbols = baProperty.mid(2, nCount);
    *pbaTreeBits = baProperty.mid(2 + nCount);
    return true;
}

bool XHuffDecoder::buildTree(const QByteArray &baTreeBits,
                             const QByteArray &baSymbols,
                             QVector<NODE> *pListNodes, qint64 *pnBytesUsed)
{
    if (!pListNodes) return false;
    pListNodes->clear();
    if (pnBytesUsed) *pnBytesUsed = 0;
    if ((baSymbols.size() <= 0) || (baSymbols.size() > MAX_SYMBOLS)) {
        return false;
    }

    HuffBitReader reader(baTreeBits);

    // Explicit stack instead of recursion: the shape below is the original's
    // pre-order walk (child0's whole subtree, then child1), just without the
    // 256-deep call chain a degenerate tree would produce.
    struct PENDING {
        qint32 nParent;
        qint32 nWhich;
    };
    QVector<PENDING> stackPending;
    PENDING rootPending = {-1, 0};
    stackPending.append(rootPending);

    qint32 nSymbolIndex = 0;
    while (!stackPending.isEmpty()) {
        const PENDING pending = stackPending.takeLast();
        if (pListNodes->size() >= MAX_NODES) return false;
        const qint32 nIndex = pListNodes->size();
        NODE node = {-1, -1, 0};
        pListNodes->append(node);
        if (pending.nParent >= 0) {
            if (pending.nWhich == 0) {
                (*pListNodes)[pending.nParent].nChild0 = nIndex;
            } else {
                (*pListNodes)[pending.nParent].nChild1 = nIndex;
            }
        }

        qint32 nBit = 0;
        if (!reader.readBit(&nBit)) return false;
        if (nBit == 0) {
            if (nSymbolIndex >= baSymbols.size()) return false;
            (*pListNodes)[nIndex].nSymbol =
                quint8(baSymbols.at(nSymbolIndex));
            ++nSymbolIndex;
        } else {
            PENDING child1 = {nIndex, 1};
            PENDING child0 = {nIndex, 0};
            stackPending.append(child1);
            stackPending.append(child0);
        }
    }

    // Every table entry has to end up in the tree; a partial match means the
    // BD 01 at offset 0 was a coincidence.
    if (nSymbolIndex != baSymbols.size()) return false;
    if (pnBytesUsed) *pnBytesUsed = reader.bytesUsed();
    return true;
}

bool XHuffDecoder::decodeSymbols(const QVector<NODE> &listNodes,
                                 const QByteArray &baInput, qint64 nCount,
                                 qint64 nLimit, QByteArray *pbaResult)
{
    if (!pbaResult || listNodes.isEmpty()) return false;
    pbaResult->clear();
    if (nCount == 0) return true;
    if ((nCount > 0) && (nLimit >= 0) && (nCount > nLimit)) return false;

    HuffBitReader reader(baInput);
    qint64 nProduced = 0;
    while (true) {
        qint32 nNode = 0;
        while (listNodes.at(nNode).nChild0 != -1) {
            qint32 nBit = 0;
            if (!reader.readBit(&nBit)) return false;
            const qint32 nNext = nBit ? listNodes.at(nNode).nChild0
                                      : listNodes.at(nNode).nChild1;
            if ((nNext < 0) || (nNext >= listNodes.size())) return false;
            nNode = nNext;
        }
        const quint8 nSymbol = listNodes.at(nNode).nSymbol;
        if (nCount < 0) {
            // Name mode: the NUL terminator is consumed but not stored.
            if (nSymbol == 0) return true;
            if ((nLimit >= 0) && (nProduced >= nLimit)) return false;
            pbaResult->append(char(nSymbol));
            ++nProduced;
        } else {
            pbaResult->append(char(nSymbol));
            ++nProduced;
            if (nProduced == nCount) return true;
        }
    }
}

bool XHuffDecoder::decodeWithProperty(const QByteArray &baProperty,
                                      const QByteArray &baInput,
                                      qint64 nUncompressedSize,
                                      QByteArray *pbaResult)
{
    if (!pbaResult || (nUncompressedSize < 0)) return false;
    QByteArray baSymbols;
    QByteArray baTreeBits;
    if (!unpackTree(baProperty, &baSymbols, &baTreeBits)) return false;
    QVector<NODE> listNodes;
    if (!buildTree(baTreeBits, baSymbols, &listNodes, nullptr)) return false;
    if (!decodeSymbols(listNodes, baInput, nUncompressedSize, -1, pbaResult)) {
        return false;
    }
    return (pbaResult->size() == nUncompressedSize);
}
