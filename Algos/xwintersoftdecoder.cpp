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
#include "xwintersoftdecoder.h"

namespace {

// --- LZW15V ---------------------------------------------------------------
const qint32 N_END_OF_STREAM = 0x100;
const qint32 N_BUMP_CODE = 0x101;
const qint32 N_FLUSH_CODE = 0x102;
const qint32 N_FIRST_CODE = 0x103;
const qint32 N_TABLE_CAP = 0x8000;
const qint32 N_MIN_CODE_BITS = 9;
const qint32 N_MAX_CODE_BITS = 15;
const qint32 N_FIRST_BUMP = 0x1ff;

// --- AHUFF ----------------------------------------------------------------
const qint32 N_ROOT = 0;
const qint32 N_MAX_WEIGHT = 0x8000;
const qint32 N_A_EOS = 0x100;
const qint32 N_A_ESC = 0x101;
const qint32 N_SYMBOLS = 0x102;
const qint32 N_NODES = 0x204;  // 2 * N_SYMBOLS, one spare pair over the 0x203 a
                               // full alphabet can actually reach

// a 32-bit accumulator filled MSB first from a limited byte
// supply. A code that runs off the end is NOT zero-padded - it is an end of
// stream, which is what stops the decoder on a member whose encoder omitted the
// explicit 0x100.
class Lzw15vBits {
public:
    Lzw15vBits(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nBuffer(0), m_nCount(0), m_bEof(false)
    {
    }

    bool get(qint32 nWidth, qint32 *pnValue)
    {
        if (nWidth == 0) {
            *pnValue = 0;
            return true;
        }
        if (m_bEof) return false;
        while (m_nCount < nWidth) {
            if (m_nPosition >= m_nSize) {
                m_bEof = true;
                return false;
            }
            m_nBuffer = (quint32)(m_nBuffer + ((quint32)m_pData[m_nPosition] << (24 - m_nCount)));
            ++m_nPosition;
            m_nCount += 8;
        }
        *pnValue = (qint32)(m_nBuffer >> (32 - nWidth));
        m_nBuffer = (quint32)(m_nBuffer << nWidth);
        m_nCount -= nWidth;
        return true;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint32 m_nBuffer;
    qint32 m_nCount;
    bool m_bEof;
};

// Owns the code width, because the width changes only on an explicit BUMP or
// FLUSH and never as a side effect of the table filling up.
class Lzw15vReader {
public:
    Lzw15vReader(const quint8 *pData, qint64 nSize) : m_bits(pData, nSize), m_nWidth(N_MIN_CODE_BITS), m_nBump(N_FIRST_BUMP)
    {
    }

    // The reference implementation. The auto-bump is a safety net for a foreign encoder: a
    // Nelson-compatible one is always exactly one code short of tripping it.
    bool next(qint32 nNextCode, qint32 *pnCode)
    {
        if (m_nBump < nNextCode) widen();

        return m_bits.get(m_nWidth, pnCode);
    }

    void widen()
    {
        if (m_nWidth < N_MAX_CODE_BITS) {
            ++m_nWidth;
            // Pinned above the table cap at the top width so no later bump, and
            // no auto-bump, can push the width past 15.
            m_nBump = (m_nWidth == N_MAX_CODE_BITS) ? N_TABLE_CAP : ((1 << m_nWidth) - 1);
        }
    }

    void reset()
    {
        m_nWidth = N_MIN_CODE_BITS;
        m_nBump = N_FIRST_BUMP;
    }

private:
    Lzw15vBits m_bits;
    qint32 m_nWidth;
    qint32 m_nBump;
};

// The reference implementation mode 1: one byte at a time, MSB first.
class AhuffBits {
public:
    AhuffBits(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nCurrent(0), m_nCount(0)
    {
    }

    bool bit(qint32 *pnBit)
    {
        if (m_nCount == 0) {
            if (m_nPosition >= m_nSize) return false;
            m_nCurrent = m_pData[m_nPosition];
            ++m_nPosition;
            m_nCount = 8;
        }
        *pnBit = (qint32)((m_nCurrent >> 7) & 1);
        m_nCurrent = (quint8)(m_nCurrent << 1);
        --m_nCount;

        return true;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint8 m_nCurrent;
    qint32 m_nCount;
};

// Nelson AHUFF's tree, kept as parallel arrays sorted by descending weight.
// The parent link is deliberately NOT part of a swap - a node keeps the parent
// belonging to its SLOT, not to its contents, which is what makes the sibling
// property repair work in place.
class AhuffTree {
public:
    AhuffTree()
    {
        for (qint32 i = 0; i < N_SYMBOLS; ++i) m_listLeaf[i] = -1;
        for (qint32 i = 0; i < N_NODES; ++i) {
            m_listWeight[i] = 0;
            m_listParent[i] = 0;
            m_listChild[i] = 0;
            m_listIsLeaf[i] = false;
        }

        m_listWeight[0] = 2;
        m_listChild[0] = 1;
        m_listIsLeaf[0] = false;
        m_listParent[0] = -1;

        m_listWeight[1] = 1;
        m_listChild[1] = N_A_EOS;
        m_listIsLeaf[1] = true;
        m_listParent[1] = 0;
        m_listLeaf[N_A_EOS] = 1;

        m_listWeight[2] = 1;
        m_listChild[2] = N_A_ESC;
        m_listIsLeaf[2] = true;
        m_listParent[2] = 0;
        m_listLeaf[N_A_ESC] = 2;

        m_nFree = 3;
    }

    bool isLeaf(qint32 nNode) const
    {
        return m_listIsLeaf[nNode];
    }

    qint32 child(qint32 nNode) const
    {
        return m_listChild[nNode];
    }

    // Split the lightest node - always the ESCAPE leaf - to make room for a
    // symbol seen for the first time.
    bool add(qint32 nSymbol)
    {
        if ((nSymbol < 0) || (nSymbol >= N_SYMBOLS)) return false;
        if ((m_nFree + 1) >= N_NODES) return false;

        const qint32 nLight = m_nFree - 1;
        const qint32 nNew = m_nFree;
        const qint32 nZero = m_nFree + 1;
        m_nFree += 2;

        m_listWeight[nNew] = m_listWeight[nLight];
        m_listParent[nNew] = nLight;
        m_listChild[nNew] = m_listChild[nLight];
        m_listIsLeaf[nNew] = m_listIsLeaf[nLight];
        setLeafSlot(m_listChild[nNew], nNew);

        m_listChild[nLight] = nNew;
        m_listIsLeaf[nLight] = false;

        m_listChild[nZero] = nSymbol;
        m_listIsLeaf[nZero] = true;
        m_listWeight[nZero] = 0;
        m_listParent[nZero] = nLight;
        m_listLeaf[nSymbol] = nZero;

        return true;
    }

    void update(qint32 nSymbol)
    {
        if (m_listWeight[N_ROOT] == N_MAX_WEIGHT) rebuild();
        if ((nSymbol < 0) || (nSymbol >= N_SYMBOLS)) return;

        qint32 nCurrent = m_listLeaf[nSymbol];
        while (nCurrent != -1) {
            if ((nCurrent < 0) || (nCurrent >= N_NODES)) return;
            ++m_listWeight[nCurrent];

            qint32 nNew = nCurrent;
            while (nNew > N_ROOT) {
                if (m_listWeight[nNew - 1] >= m_listWeight[nCurrent]) break;
                --nNew;
            }
            if (nCurrent != nNew) {
                swapNodes(nCurrent, nNew);
                nCurrent = nNew;
            }
            nCurrent = m_listParent[nCurrent];
        }
    }

private:
    void setLeafSlot(qint32 nSymbol, qint32 nNode)
    {
        // The source is always a leaf in practice, but a corrupt stream must not
        // be able to turn a node index into a write outside m_listLeaf.
        if ((nSymbol >= 0) && (nSymbol < N_SYMBOLS)) m_listLeaf[nSymbol] = nNode;
    }

    void relink(qint32 nNode, qint32 nTarget)
    {
        if (m_listIsLeaf[nNode]) {
            setLeafSlot(m_listChild[nNode], nTarget);
        } else {
            const qint32 nChild = m_listChild[nNode];
            if ((nChild >= 0) && ((nChild + 1) < N_NODES)) {
                m_listParent[nChild] = nTarget;
                m_listParent[nChild + 1] = nTarget;
            }
        }
    }

    void swapNodes(qint32 nFirst, qint32 nSecond)
    {
        relink(nFirst, nSecond);
        relink(nSecond, nFirst);

        const qint32 nWeight = m_listWeight[nFirst];
        const qint32 nChild = m_listChild[nFirst];
        const bool bIsLeaf = m_listIsLeaf[nFirst];

        m_listWeight[nFirst] = m_listWeight[nSecond];
        m_listChild[nFirst] = m_listChild[nSecond];
        m_listIsLeaf[nFirst] = m_listIsLeaf[nSecond];

        m_listWeight[nSecond] = nWeight;
        m_listChild[nSecond] = nChild;
        m_listIsLeaf[nSecond] = bIsLeaf;
        // Parents stay with the slots.
    }

    // Nelson RebuildTree: pack the leaves to the end with halved weights, then
    // rebuild the internal nodes back down to the root. Only fires once the
    // root weight reaches 0x8000, so it is the least travelled path here.
    void rebuild()
    {
        qint32 nDestination = m_nFree - 1;
        for (qint32 i = m_nFree - 1; i >= N_ROOT; --i) {
            if (m_listIsLeaf[i]) {
                m_listWeight[nDestination] = (m_listWeight[i] + 1) >> 1;
                m_listParent[nDestination] = m_listParent[i];
                m_listChild[nDestination] = m_listChild[i];
                m_listIsLeaf[nDestination] = m_listIsLeaf[i];
                --nDestination;
            }
        }

        qint32 nSource = m_nFree - 2;
        while (nDestination >= N_ROOT) {
            if ((nSource < 0) || ((nSource + 1) >= N_NODES)) return;

            const qint32 nWeight = m_listWeight[nSource] + m_listWeight[nSource + 1];
            m_listWeight[nDestination] = nWeight;
            m_listIsLeaf[nDestination] = false;

            qint32 nSlot = nDestination + 1;
            while ((nSlot < N_NODES) && (nWeight < m_listWeight[nSlot])) ++nSlot;
            --nSlot;

            for (qint32 i = nDestination; i < nSlot; ++i) {
                m_listWeight[i] = m_listWeight[i + 1];
                m_listParent[i] = m_listParent[i + 1];
                m_listChild[i] = m_listChild[i + 1];
                m_listIsLeaf[i] = m_listIsLeaf[i + 1];
            }

            m_listWeight[nSlot] = nWeight;
            m_listChild[nSlot] = nSource;
            m_listIsLeaf[nSlot] = false;

            nSource -= 2;
            --nDestination;
        }

        for (qint32 i = m_nFree - 1; i >= N_ROOT; --i) relink(i, i);
    }

    qint32 m_listLeaf[N_SYMBOLS];
    qint32 m_listWeight[N_NODES];
    qint32 m_listParent[N_NODES];
    qint32 m_listChild[N_NODES];
    bool m_listIsLeaf[N_NODES];
    qint32 m_nFree;
};

}  // namespace

bool XWintersoftDecoder::decodeLZW15V(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    QVector<qint32> listPrefix(N_TABLE_CAP, 0);
    QByteArray baSuffix(N_TABLE_CAP, (char)0);
    quint8 *pSuffix = (quint8 *)baSuffix.data();
    for (qint32 i = 0; i < 256; ++i) pSuffix[i] = (quint8)i;

    qint32 nNextCode = N_FIRST_CODE;
    Lzw15vReader reader((const quint8 *)baPacked.constData(), baPacked.size());

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);

    // The stream opens with a bare code that is emitted as a literal and becomes
    // the initial previous-code; it is NOT range checked, exactly as the reference implementation does not
    // range check it.
    qint32 nCode = 0;
    if (reader.next(nNextCode, &nCode)) {
        qint32 nOldCode = nCode;
        qint32 nCharacter = nCode & 0xff;
        baOut.append((char)(quint8)(nCode & 0xff));

        QByteArray baStack;
        const qint64 nOutputLimit = nUncompressedSize + N_TABLE_CAP;
        qint32 nCheck = 0;
        bool bStop = false;

        while (!bStop) {
            ++nCheck;
            if ((nCheck & 0xffff) == 0) {
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            }

            if (!reader.next(nNextCode, &nCode)) break;
            if (nCode == N_END_OF_STREAM) break;

            if (nCode == N_BUMP_CODE) {
                reader.widen();
                continue;
            }

            if (nCode == N_FLUSH_CODE) {
                nNextCode = N_FIRST_CODE;
                reader.reset();
                if (!reader.next(nNextCode, &nCode)) break;
                // After a flush the next code restarts the string, so it has to
                // be a literal; anything else is a broken stream.
                if (nCode > 0xff) break;
                nOldCode = nCode;
                nCharacter = nCode;
                baOut.append((char)(quint8)nCode);
                continue;
            }

            if (nCode >= N_TABLE_CAP) break;  // unreachable at 15 bits, bounded anyway

            const qint32 nNewCode = nCode;
            qint32 nCurrent = nCode;
            baStack.resize(0);

            if (nCurrent >= nNextCode) {
                // KwKwK: the code is the entry being defined right now.
                baStack.append((char)(quint8)nCharacter);
                nCurrent = nOldCode;
            }

            qint32 nGuard = 0;
            while (nCurrent > 0xff) {
                ++nGuard;
                if ((nCurrent >= N_TABLE_CAP) || (nGuard > N_TABLE_CAP)) {
                    bStop = true;
                    break;
                }
                baStack.append((char)pSuffix[nCurrent]);
                nCurrent = listPrefix[nCurrent];
            }
            if (bStop) break;

            nCharacter = nCurrent & 0xff;
            baStack.append((char)(quint8)nCharacter);
            for (qint32 i = baStack.size() - 1; i >= 0; --i) baOut.append(baStack.at(i));

            if (nNextCode < N_TABLE_CAP) {
                listPrefix[nNextCode] = nOldCode;
                pSuffix[nNextCode] = (quint8)nCharacter;
                ++nNextCode;
            }
            nOldCode = nNewCode;

            if ((qint64)baOut.size() > nOutputLimit) break;
        }
    }

    *pbaResult = baOut;

    return (qint64)baOut.size() == nUncompressedSize;
}

bool XWintersoftDecoder::decodeAHUFF(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    AhuffBits bits((const quint8 *)baPacked.constData(), baPacked.size());
    AhuffTree tree;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);
    qint32 nCheck = 0;

    while ((qint64)baOut.size() < nUncompressedSize) {
        ++nCheck;
        if ((nCheck & 0xffff) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }

        qint32 nNode = N_ROOT;
        qint32 nDepth = 0;
        bool bStop = false;

        while (!tree.isLeaf(nNode)) {
            ++nDepth;
            if (nDepth > N_NODES) {
                bStop = true;
                break;
            }
            nNode = tree.child(nNode);
            qint32 nBit = 0;
            if (!bits.bit(&nBit)) {
                bStop = true;
                break;
            }
            nNode += nBit;
            if ((nNode < 0) || (nNode >= N_NODES)) {
                bStop = true;
                break;
            }
        }
        if (bStop) break;

        qint32 nSymbol = tree.child(nNode);
        if (nSymbol == N_A_EOS) break;

        if (nSymbol == N_A_ESC) {
            // A byte never seen before: eight raw bits, then the escape leaf is
            // split so the byte gets a leaf of its own.
            nSymbol = 0;
            for (qint32 i = 0; i < 8; ++i) {
                qint32 nBit = 0;
                if (!bits.bit(&nBit)) {
                    bStop = true;
                    break;
                }
                nSymbol = nSymbol * 2 + nBit;
            }
            if (bStop) break;
            if (!tree.add(nSymbol)) break;
        }

        baOut.append((char)(quint8)(nSymbol & 0xff));
        tree.update(nSymbol);
    }

    *pbaResult = baOut;

    return (qint64)baOut.size() == nUncompressedSize;
}
