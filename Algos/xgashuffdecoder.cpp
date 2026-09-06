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
#include "xgashuffdecoder.h"

#include "algo_utils.h"

namespace {

// MSB-first bit reader over a flat buffer.
class GasBitReader {
public:
    GasBitReader(const quint8 *pData, qint64 nSize, qint64 nByteOffset) : m_pData(pData), m_nSize(nSize), m_nBitPos(nByteOffset * 8), m_bOverread(false)
    {
    }

    bool isOverread() const
    {
        return m_bOverread;
    }

    qint64 bitPos() const
    {
        return m_nBitPos;
    }

    void setBitPos(qint64 nBitPos)
    {
        m_nBitPos = nBitPos;
    }

    qint64 bitsLeft() const
    {
        return (m_nSize * 8) - m_nBitPos;
    }

    quint32 readBit()
    {
        if (m_nBitPos >= (m_nSize * 8)) {
            m_bOverread = true;
            return 0;
        }

        const quint8 nByte = m_pData[m_nBitPos >> 3];
        const quint32 nResult = (quint32)((nByte >> (7 - (m_nBitPos & 7))) & 1);
        m_nBitPos++;

        return nResult;
    }

    // The container fills its multi-bit fields starting at the least
    // significant bit, while the bits themselves arrive most significant first
    // inside each byte.
    quint32 readBitsLsbFirst(qint32 nCount)
    {
        quint32 nResult = 0;

        for (qint32 i = 0; i < nCount; i++) {
            nResult |= (readBit() << i);
        }

        return nResult;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nBitPos;
    bool m_bOverread;
};

quint32 gasReadU32(const quint8 *pData)
{
    return (quint32)pData[0] | ((quint32)pData[1] << 8) | ((quint32)pData[2] << 16) | ((quint32)pData[3] << 24);
}

quint32 gasReadU16(const quint8 *pData)
{
    return (quint32)pData[0] | ((quint32)pData[1] << 8);
}

// Walks the tree from the root: rejects cycles, out-of-range links, leaves that
// repeat a byte value and internal nodes that are not fully connected.
bool gasCheckTree(const std::vector<XGasHuffDecoder::GAS_NODE> &listNodes, quint32 nRootIndex, qint32 *pnLeafCount)
{
    const qint32 nRecords = (qint32)listNodes.size();

    if ((nRecords <= 0) || ((qint32)nRootIndex >= nRecords)) {
        return false;
    }

    if (listNodes[nRootIndex].bLeaf) {
        // A single-symbol tree would give the payload no bits at all; every
        // sample of this family has a branching root.
        return false;
    }

    std::vector<quint8> listVisited((size_t)nRecords, 0);
    std::vector<quint8> listSymbolSeen(256, 0);
    std::vector<quint16> listStack;
    listStack.push_back((quint16)nRootIndex);

    qint32 nLeaves = 0;
    qint32 nInternal = 0;

    while (!listStack.empty()) {
        const quint16 nIndex = listStack.back();
        listStack.pop_back();

        if ((qint32)nIndex >= nRecords) {
            return false;
        }

        if (listVisited[nIndex]) {
            // A proper tree reaches every node exactly once.
            return false;
        }

        listVisited[nIndex] = 1;

        if (listNodes[nIndex].bLeaf) {
            if (listSymbolSeen[listNodes[nIndex].nSymbol]) {
                return false;
            }

            listSymbolSeen[listNodes[nIndex].nSymbol] = 1;
            nLeaves++;
        } else {
            nInternal++;

            if (((qint32)listNodes[nIndex].nChild1 >= nRecords) || ((qint32)listNodes[nIndex].nChild0 >= nRecords)) {
                return false;
            }

            if (listNodes[nIndex].nChild1 == listNodes[nIndex].nChild0) {
                return false;
            }

            listStack.push_back(listNodes[nIndex].nChild1);
            listStack.push_back(listNodes[nIndex].nChild0);

            if ((qint32)listStack.size() > nRecords) {
                return false;
            }
        }
    }

    if (nLeaves != (nInternal + 1)) {
        return false;
    }

    if (nLeaves < 2) {
        return false;
    }

    if (pnLeafCount) {
        *pnLeafCount = nLeaves;
    }

    return true;
}

}  // namespace

XGasHuffDecoder::XGasHuffDecoder(QObject *parent) : QObject(parent)
{
}

bool XGasHuffDecoder::checkStream(const char *pData, qint64 nSize, qint64 nFullSize, quint32 *pnUncompressedSize, quint32 *pnNodeCount, quint32 *pnRootIndex,
                                  std::vector<GAS_NODE> *pListNodes, qint64 *pnPayloadBitOffset)
{
    if (pnUncompressedSize) *pnUncompressedSize = 0;
    if (pnNodeCount) *pnNodeCount = 0;
    if (pnRootIndex) *pnRootIndex = 0;
    if (pnPayloadBitOffset) *pnPayloadBitOffset = 0;

    if ((!pData) || (nSize <= (GAS_HEADER_SIZE + 2)) || (nFullSize < nSize) || (nFullSize > GAS_MAX_INPUT_SIZE)) {
        return false;
    }

    const bool bComplete = (nSize == nFullSize);

    const quint8 *pBytes = (const quint8 *)pData;

    const quint32 nUncompressedSize = gasReadU32(pBytes);
    const quint32 nNodeCount = gasReadU16(pBytes + 4);
    const quint32 nRootIndex = gasReadU16(pBytes + 6);

    if ((nUncompressedSize == 0) || ((qint64)nUncompressedSize > GAS_MAX_OUTPUT_SIZE)) {
        return false;
    }

    if ((nNodeCount < 3) || (nNodeCount > (quint32)GAS_MAX_NODES)) {
        return false;
    }

    if (nRootIndex > nNodeCount) {
        return false;
    }

    const qint32 nRecords = (qint32)nNodeCount + 1;

    // Cheap size sanity before touching the bit stream: a Huffman stream can
    // never be more than one byte per symbol plus the node table, and it needs
    // at least one bit per emitted symbol.
    const qint64 nMaxTreeBytes = ((qint64)nRecords * 21 + 7) / 8;

    if ((nFullSize - GAS_HEADER_SIZE) > ((qint64)nUncompressedSize + nMaxTreeBytes + 16)) {
        return false;
    }

    std::vector<GAS_NODE> listNodes;
    listNodes.reserve((size_t)nRecords);

    GasBitReader reader(pBytes, nSize, GAS_HEADER_SIZE);

    qint32 nInternal = 0;

    for (qint32 i = 0; i < nRecords; i++) {
        GAS_NODE node = {};

        if (reader.readBit()) {
            node.bLeaf = true;
            node.nSymbol = (quint8)reader.readBitsLsbFirst(8);
        } else {
            const quint32 nRaw1 = reader.readBitsLsbFirst(10);
            const quint32 nRaw0 = reader.readBitsLsbFirst(10);

            // Both link fields carry 2 * index + 1; an even field means this is
            // not a node table.
            if (((nRaw1 & 1) == 0) || ((nRaw0 & 1) == 0)) {
                return false;
            }

            const quint32 nChild1 = (nRaw1 - 1) >> 1;
            const quint32 nChild0 = (nRaw0 - 1) >> 1;

            if (((qint32)nChild1 >= nRecords) || ((qint32)nChild0 >= nRecords)) {
                return false;
            }

            node.bLeaf = false;
            node.nChild1 = (quint16)nChild1;
            node.nChild0 = (quint16)nChild0;
            nInternal++;
        }

        if (reader.isOverread()) {
            return false;
        }

        listNodes.push_back(node);
    }

    // Every well-formed member of this family stores exactly nNodeCount / 2
    // internal records; the remaining records are leaves plus one spare.
    if (nInternal != (qint32)(nNodeCount / 2)) {
        return false;
    }

    qint32 nLeafCount = 0;

    if (!gasCheckTree(listNodes, nRootIndex, &nLeafCount)) {
        return false;
    }

    const qint64 nPayloadBitOffset = reader.bitPos();
    const qint64 nPayloadBits = (nFullSize * 8) - nPayloadBitOffset;

    // The shortest possible code is one bit per symbol.
    if (nPayloadBits < (qint64)nUncompressedSize) {
        return false;
    }

    // Bounded trial decode - the only real detection test a headerless format
    // can offer.
    {
        const qint64 nProbe = qMin<qint64>((qint64)nUncompressedSize, (qint64)GAS_PROBE_BYTES);
        qint64 nDone = 0;

        while (nDone < nProbe) {
            quint32 nCurrent = nRootIndex;
            qint32 nGuard = 0;
            bool bExhausted = false;

            while (!listNodes[nCurrent].bLeaf) {
                nCurrent = reader.readBit() ? listNodes[nCurrent].nChild1 : listNodes[nCurrent].nChild0;

                if (reader.isOverread()) {
                    bExhausted = true;
                    break;
                }

                nGuard++;

                if (nGuard > nRecords) {
                    return false;
                }
            }

            if (bExhausted) {
                // Running out inside a complete container means the tree does
                // not fit the stream.  On a truncated probe it only means the
                // prefix ended, which is acceptable once enough symbols came
                // out cleanly.
                if (bComplete || (nDone < (qint64)GAS_MIN_PROBE_BYTES)) {
                    return false;
                }

                break;
            }

            nDone++;
        }
    }

    if (pnUncompressedSize) *pnUncompressedSize = nUncompressedSize;
    if (pnNodeCount) *pnNodeCount = nNodeCount;
    if (pnRootIndex) *pnRootIndex = nRootIndex;
    if (pnPayloadBitOffset) *pnPayloadBitOffset = nPayloadBitOffset;
    if (pListNodes) *pListNodes = listNodes;

    return true;
}

bool XGasHuffDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if ((!pDecompressState) || (!pDecompressState->pDeviceInput) || (!pDecompressState->pDeviceOutput) || (pDecompressState->nInputOffset < 0) ||
        (pDecompressState->nInputLimit < 0)) {
        return false;
    }

    const qint64 nInputSize = pDecompressState->nInputLimit;

    if ((nInputSize <= (GAS_HEADER_SIZE + 2)) || (nInputSize > GAS_MAX_INPUT_SIZE)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);

    if (pDecompressState->bReadError || pDecompressState->bWriteError || !XBinary::isPdStructNotCanceled(pPdStruct)) {
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

    quint32 nUncompressedSize = 0;
    quint32 nNodeCount = 0;
    quint32 nRootIndex = 0;
    qint64 nPayloadBitOffset = 0;
    std::vector<GAS_NODE> listNodes;

    if (!checkStream(baInput.constData(), baInput.size(), baInput.size(), &nUncompressedSize, &nNodeCount, &nRootIndex, &listNodes, &nPayloadBitOffset)) {
        return false;
    }

    if (pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        bool bConverted = false;
        const qint64 nDeclaredSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bConverted);
        if (!bConverted || (nDeclaredSize != (qint64)nUncompressedSize)) return false;
    }

    qint64 nOutputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(pDecompressState->mapUnpackProperties, &nOutputLimit) ||
        !XBinary::isUnpackOutputSizeAllowed(pDecompressState->mapUnpackProperties, (qint64)nUncompressedSize)) {
        return false;
    }

    if ((nOutputLimit >= 0) && ((qint64)nUncompressedSize > nOutputLimit)) {
        return false;
    }

    QByteArray baOutput;
    baOutput.resize((qint32)nUncompressedSize);

    if (baOutput.size() != (qint32)nUncompressedSize) {
        pDecompressState->bWriteError = true;
        return false;
    }

    {
        // checkStream() already parsed and validated the node table, so the
        // payload starts at the bit offset it reported.
        GasBitReader reader((const quint8 *)baInput.constData(), baInput.size(), 0);
        reader.setBitPos(nPayloadBitOffset);

        const qint32 nRecords = (qint32)listNodes.size();
        quint8 *pOut = (quint8 *)baOutput.data();

        for (qint64 i = 0; i < (qint64)nUncompressedSize; i++) {
            if ((i & 0xFFFF) == 0) {
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            }

            quint32 nCurrent = nRootIndex;
            qint32 nGuard = 0;

            while (!listNodes[nCurrent].bLeaf) {
                nCurrent = reader.readBit() ? listNodes[nCurrent].nChild1 : listNodes[nCurrent].nChild0;

                if (reader.isOverread()) {
                    return false;
                }

                nGuard++;

                if (nGuard > nRecords) {
                    return false;
                }
            }

            pOut[i] = listNodes[nCurrent].nSymbol;
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    if (XBinary::_writeDevice(baOutput.data(), baOutput.size(), pDecompressState) != baOutput.size()) {
        return false;
    }

    return !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}
