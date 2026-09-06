/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xcorelltecdecoder.h"

#include <QVector>
#include <QtEndian>

#include <limits>

namespace {
const qint32 LTEC_NC = 510;        // 256 literals + 254 match lengths
const qint32 LTEC_CBIT = 9;        // width of the literal-table symbol count
const qint32 LTEC_NT = 19;         // pre-table alphabet
const qint32 LTEC_TBIT = 5;        // width of the pre-table symbol count
const qint32 LTEC_NP = 24;         // position alphabet ceiling (see header)
const qint32 LTEC_PBIT = 5;        // width of the position-table symbol count
const qint32 LTEC_THRESHOLD = 3;   // shortest encodable match
const qint32 LTEC_PT_SPECIAL = 3;  // pre-table index carrying the 2-bit run
const qint32 LTEC_MAX_CODE_LENGTH = 16;
// Only the two uninformative bits-of-nothing at the head of a block.
const qint32 LTEC_PRELUDE_BITS = 16;
// A block's plaintext size comes from u32 container fields, so this ceiling is
// the format's own limit and not a policy choice.
const qint64 LTEC_MAX_BLOCK_SIZE = Q_INT64_C(0xffffffff);
const qint64 LTEC_CANCEL_CHECK_INTERVAL = 0x10000;
// The last symbol of a block can need a few bits the block's own byte range
// does not cover: the encoder starts the next block on the byte that holds
// them (that overlap is exactly why every block opens with two stale bytes).
// Two bytes is the measured worst case over all 462 corpus blocks; the reader
// serves zeroes inside this margin and fails past it, so a truncated archive
// still cannot be decoded into plausible-looking output.
const qint64 LTEC_TAIL_SLACK_BITS = 16;

// MSB-first bit reader over the packed block.
class LtecBitReader {
public:
    explicit LtecBitReader(const QByteArray &data)
        : m_pData(reinterpret_cast<const quint8 *>(data.constData())),
          m_nSize(data.size()), m_nBitCount(qint64(data.size()) * 8),
          m_nBitPosition(0)
    {
    }

    bool readBits(qint32 nCount, quint32 *pValue)
    {
        if (!pValue || (nCount < 0) || (nCount > 24)) return false;
        if (m_nBitPosition > m_nBitCount + LTEC_TAIL_SLACK_BITS - nCount) {
            return false;
        }
        quint32 nResult = 0;
        for (qint32 i = 0; i < nCount; ++i) {
            quint32 nBit = 0;
            const qint64 nByteIndex = m_nBitPosition >> 3;
            if (nByteIndex < m_nSize) {
                nBit = (m_pData[nByteIndex] >> (7 - (m_nBitPosition & 7))) & 1U;
            }
            nResult = (nResult << 1) | nBit;
            ++m_nBitPosition;
        }
        *pValue = nResult;
        return true;
    }

    bool skipBits(qint32 nCount)
    {
        quint32 nDummy = 0;
        while (nCount > 24) {
            if (!readBits(24, &nDummy)) return false;
            nCount -= 24;
        }
        return readBits(nCount, &nDummy);
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nBitCount;
    qint64 m_nBitPosition;
};

// Canonical Huffman table.  bConstant covers LHA's "declared count is zero"
// shape, where a single symbol owns the whole alphabet and costs no bits.
struct LtecTree {
    quint32 nCount[LTEC_MAX_CODE_LENGTH + 1];
    quint32 nFirstCode[LTEC_MAX_CODE_LENGTH + 1];
    qint32 nFirstIndex[LTEC_MAX_CODE_LENGTH + 1];
    QVector<quint16> vSymbols;
    qint32 nMaxLength;
    bool bConstant;
    quint32 nConstantSymbol;
};

void ltecResetTree(LtecTree *pTree)
{
    for (qint32 i = 0; i <= LTEC_MAX_CODE_LENGTH; ++i) {
        pTree->nCount[i] = 0;
        pTree->nFirstCode[i] = 0;
        pTree->nFirstIndex[i] = 0;
    }
    pTree->vSymbols.clear();
    pTree->nMaxLength = 0;
    pTree->bConstant = false;
    pTree->nConstantSymbol = 0;
}

// Every table this format emits is Kraft-complete (checked over all 462
// blocks of the reference corpus).  Rejecting incomplete and over-subscribed
// tables therefore costs nothing on genuine input and turns a mis-parse into
// an immediate failure instead of plausible-looking wrong plaintext.
bool ltecBuildTree(const QVector<quint8> &vLengths, LtecTree *pTree)
{
    ltecResetTree(pTree);

    qint32 nMaxLength = 0;
    for (qint32 i = 0; i < vLengths.size(); ++i) {
        const quint8 nLength = vLengths.at(i);
        if (nLength > LTEC_MAX_CODE_LENGTH) return false;
        if (nLength > 0) {
            ++pTree->nCount[nLength];
            if (qint32(nLength) > nMaxLength) nMaxLength = qint32(nLength);
        }
    }
    if (nMaxLength == 0) return false;
    pTree->nMaxLength = nMaxLength;

    qint64 nLeft = 1;
    quint32 nCode = 0;
    qint32 nIndex = 0;
    for (qint32 nLength = 1; nLength <= nMaxLength; ++nLength) {
        nLeft = (nLeft << 1) - qint64(pTree->nCount[nLength]);
        if (nLeft < 0) return false;
        pTree->nFirstCode[nLength] = nCode;
        pTree->nFirstIndex[nLength] = nIndex;
        nCode = (nCode + pTree->nCount[nLength]) << 1;
        nIndex += qint32(pTree->nCount[nLength]);
    }
    if (nLeft != 0) return false;

    pTree->vSymbols.resize(nIndex);
    QVector<qint32> vNext(LTEC_MAX_CODE_LENGTH + 1, 0);
    for (qint32 nLength = 1; nLength <= nMaxLength; ++nLength) {
        vNext[nLength] = pTree->nFirstIndex[nLength];
    }
    for (qint32 i = 0; i < vLengths.size(); ++i) {
        const quint8 nLength = vLengths.at(i);
        if (nLength > 0) pTree->vSymbols[vNext[nLength]++] = quint16(i);
    }
    return true;
}

bool ltecDecodeSymbol(LtecBitReader *pReader, const LtecTree &tree,
                      quint32 *pSymbol)
{
    if (!pSymbol) return false;
    if (tree.bConstant) {
        *pSymbol = tree.nConstantSymbol;
        return true;
    }
    quint32 nCode = 0;
    for (qint32 nLength = 1; nLength <= tree.nMaxLength; ++nLength) {
        quint32 nBit = 0;
        if (!pReader->readBits(1, &nBit)) return false;
        nCode = (nCode << 1) | nBit;
        if (tree.nCount[nLength] && (nCode >= tree.nFirstCode[nLength]) &&
            (nCode - tree.nFirstCode[nLength] < tree.nCount[nLength])) {
            *pSymbol = tree.vSymbols.at(tree.nFirstIndex[nLength] +
                                        qint32(nCode - tree.nFirstCode[nLength]));
            return true;
        }
    }
    return false;
}

// Pre-table / position-table reader.  The 3-bit length escape at value 7 is
// extended by counting the following 1-bits AND consuming the terminating 0
// bit (LHA's fillbuf(c - 3)).  Dropping that consume still decodes the short
// tables perfectly while corrupting the long ones, so it is called out here.
bool ltecReadPtLen(LtecBitReader *pReader, qint32 nAlphabet, qint32 nCountBits,
                   qint32 nSpecialIndex, LtecTree *pTree)
{
    ltecResetTree(pTree);

    quint32 nNumber = 0;
    if (!pReader->readBits(nCountBits, &nNumber)) return false;
    if (nNumber == 0) {
        quint32 nSymbol = 0;
        if (!pReader->readBits(nCountBits, &nSymbol) ||
            (nSymbol >= quint32(nAlphabet))) {
            return false;
        }
        pTree->bConstant = true;
        pTree->nConstantSymbol = nSymbol;
        return true;
    }
    if (nNumber > quint32(nAlphabet)) return false;

    QVector<quint8> vLengths(nAlphabet, 0);
    qint32 i = 0;
    while (i < qint32(nNumber)) {
        quint32 nLength = 0;
        if (!pReader->readBits(3, &nLength)) return false;
        if (nLength == 7) {
            for (;;) {
                quint32 nBit = 0;
                if (!pReader->readBits(1, &nBit)) return false;
                if (!nBit) break;
                ++nLength;
                if (nLength > quint32(LTEC_MAX_CODE_LENGTH)) return false;
            }
        }
        vLengths[i++] = quint8(nLength);
        if (i == nSpecialIndex) {
            quint32 nZeroRun = 0;
            if (!pReader->readBits(2, &nZeroRun)) return false;
            while ((nZeroRun > 0) && (i < nAlphabet)) {
                vLengths[i++] = 0;
                --nZeroRun;
            }
        }
    }
    return ltecBuildTree(vLengths, pTree);
}

bool ltecReadCLen(LtecBitReader *pReader, const LtecTree &preTree,
                  LtecTree *pTree)
{
    ltecResetTree(pTree);

    quint32 nNumber = 0;
    if (!pReader->readBits(LTEC_CBIT, &nNumber)) return false;
    if (nNumber == 0) {
        quint32 nSymbol = 0;
        if (!pReader->readBits(LTEC_CBIT, &nSymbol) ||
            (nSymbol >= quint32(LTEC_NC))) {
            return false;
        }
        pTree->bConstant = true;
        pTree->nConstantSymbol = nSymbol;
        return true;
    }
    if (nNumber > quint32(LTEC_NC)) return false;

    QVector<quint8> vLengths(LTEC_NC, 0);
    qint32 i = 0;
    while (i < qint32(nNumber)) {
        quint32 nSymbol = 0;
        if (!ltecDecodeSymbol(pReader, preTree, &nSymbol)) return false;
        if (nSymbol <= 2) {
            quint32 nZeroRun = 0;
            if (nSymbol == 0) {
                nZeroRun = 1;
            } else if (nSymbol == 1) {
                if (!pReader->readBits(4, &nZeroRun)) return false;
                nZeroRun += 3;
            } else {
                if (!pReader->readBits(LTEC_CBIT, &nZeroRun)) return false;
                nZeroRun += 20;
            }
            // The run may be declared longer than the count leaves room for;
            // the surplus lands on entries the tail fill would zero anyway.
            while ((nZeroRun > 0) && (i < qint32(nNumber))) {
                vLengths[i++] = 0;
                --nZeroRun;
            }
        } else {
            if ((nSymbol - 2) > quint32(LTEC_MAX_CODE_LENGTH)) return false;
            vLengths[i++] = quint8(nSymbol - 2);
        }
    }
    return ltecBuildTree(vLengths, pTree);
}

// Decodes a solid block's plaintext up to nStopSize bytes.  The window is the
// plaintext produced so far - a match may reach back to the block's very
// first byte and never before it, which is what makes a flat buffer both
// correct and stricter than an LHA ring window pre-filled with spaces.
bool ltecDecodeBlock(const QByteArray &packed, qint64 nStopSize,
                     QByteArray *pOutput, XBinary::PDSTRUCT *pPdStruct)
{
    pOutput->clear();
    if (nStopSize == 0) return true;
    if (packed.isEmpty()) return false;

    LtecBitReader reader(packed);
    if (!reader.skipBits(LTEC_PRELUDE_BITS)) return false;

    LtecTree preTree;
    LtecTree literalTree;
    LtecTree positionTree;
    ltecResetTree(&preTree);
    ltecResetTree(&literalTree);
    ltecResetTree(&positionTree);

    QByteArray baResult;
    baResult.reserve(qint32(nStopSize));

    quint32 nBlockRemaining = 0;
    qint64 nNextCancelCheck = LTEC_CANCEL_CHECK_INTERVAL;
    while (qint64(baResult.size()) < nStopSize) {
        if (qint64(baResult.size()) >= nNextCancelCheck) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            nNextCancelCheck = qint64(baResult.size()) + LTEC_CANCEL_CHECK_INTERVAL;
        }
        if (nBlockRemaining == 0) {
            // A block restates all three tables in-stream after each run of
            // symbols; the 16-bit count is how many symbols the run holds.
            if (!reader.readBits(16, &nBlockRemaining) || (nBlockRemaining == 0)) {
                return false;
            }
            if (!ltecReadPtLen(&reader, LTEC_NT, LTEC_TBIT, LTEC_PT_SPECIAL,
                               &preTree) ||
                !ltecReadCLen(&reader, preTree, &literalTree) ||
                !ltecReadPtLen(&reader, LTEC_NP, LTEC_PBIT, -1, &positionTree)) {
                return false;
            }
        }
        --nBlockRemaining;

        quint32 nSymbol = 0;
        if (!ltecDecodeSymbol(&reader, literalTree, &nSymbol)) return false;
        if (nSymbol < 256) {
            baResult.append(char(quint8(nSymbol)));
            continue;
        }
        if (nSymbol >= quint32(LTEC_NC)) return false;

        const qint32 nMatchLength = qint32(nSymbol) - 256 + LTEC_THRESHOLD;
        quint32 nPositionSymbol = 0;
        if (!ltecDecodeSymbol(&reader, positionTree, &nPositionSymbol) ||
            (nPositionSymbol >= quint32(LTEC_NP))) {
            return false;
        }
        quint32 nDistance = 0;
        if (nPositionSymbol > 0) {
            quint32 nExtra = 0;
            if (!reader.readBits(qint32(nPositionSymbol) - 1, &nExtra)) {
                return false;
            }
            nDistance = (1U << (nPositionSymbol - 1)) + nExtra;
        }
        ++nDistance;

        const qint64 nCopyStart = qint64(baResult.size()) - qint64(nDistance);
        if (nCopyStart < 0) return false;
        // Overlapping matches are normal here, so the copy has to be
        // byte-by-byte: a memmove would read bytes this run has not written.
        qint32 nSource = qint32(nCopyStart);
        for (qint32 i = 0; i < nMatchLength; ++i) {
            baResult.append(baResult.at(nSource + i));
            if (qint64(baResult.size()) >= nStopSize) break;
        }
    }

    *pOutput = baResult;
    return qint64(pOutput->size()) >= nStopSize;
}
}  // namespace

qint32 XCorelLtecDecoder::positionAlphabetSize()
{
    return LTEC_NP;
}

QByteArray XCorelLtecDecoder::packProperties(qint64 nBlockSize,
                                             qint64 nOffsetInBlock)
{
    QByteArray baResult;
    if ((nBlockSize < 0) || (nBlockSize > LTEC_MAX_BLOCK_SIZE) ||
        (nOffsetInBlock < 0) || (nOffsetInBlock > LTEC_MAX_BLOCK_SIZE)) {
        return baResult;
    }
    quint32 nValues[2];
    nValues[0] = qToLittleEndian<quint32>(quint32(nBlockSize));
    nValues[1] = qToLittleEndian<quint32>(quint32(nOffsetInBlock));
    baResult.append(reinterpret_cast<const char *>(nValues), 8);
    return baResult;
}

bool XCorelLtecDecoder::decode(const QByteArray &packed,
                               qint64 nUncompressedSize,
                               const QByteArray &baProperties,
                               QByteArray *pOutput,
                               XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput) return false;
    pOutput->clear();
    if ((nUncompressedSize < 0) ||
        (nUncompressedSize > qint64((std::numeric_limits<qint32>::max)()))) {
        return false;
    }

    qint64 nBlockSize = nUncompressedSize;
    qint64 nOffsetInBlock = 0;
    if (!baProperties.isEmpty()) {
        if (baProperties.size() != 8) return false;
        const uchar *pProperties =
            reinterpret_cast<const uchar *>(baProperties.constData());
        nBlockSize = qint64(qFromLittleEndian<quint32>(pProperties));
        nOffsetInBlock = qint64(qFromLittleEndian<quint32>(pProperties + 4));
    }
    if ((nBlockSize < 0) || (nBlockSize > LTEC_MAX_BLOCK_SIZE) ||
        (nBlockSize > qint64((std::numeric_limits<qint32>::max)()))) {
        return false;
    }
    if ((nOffsetInBlock < 0) || (nOffsetInBlock > nBlockSize) ||
        (nUncompressedSize > nBlockSize - nOffsetInBlock)) {
        return false;
    }
    if (nUncompressedSize == 0) return true;

    // Only as much of the block as the member needs; the LZ prefix is
    // identical either way, so stopping early is not an approximation.
    const qint64 nStopSize = nOffsetInBlock + nUncompressedSize;
    QByteArray baBlock;
    if (!ltecDecodeBlock(packed, nStopSize, &baBlock, pPdStruct)) return false;
    if (qint64(baBlock.size()) < nStopSize) return false;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    *pOutput = baBlock.mid(qint32(nOffsetInBlock), qint32(nUncompressedSize));
    return qint64(pOutput->size()) == nUncompressedSize;
}

bool XCorelLtecDecoder::probe(const QByteArray &packed, qint64 nBlockSize,
                              qint64 nProbeSize, XBinary::PDSTRUCT *pPdStruct)
{
    if ((nBlockSize <= 0) || (nBlockSize > LTEC_MAX_BLOCK_SIZE) ||
        (nBlockSize > qint64((std::numeric_limits<qint32>::max)()))) {
        return false;
    }
    if (nProbeSize <= 0) return false;
    const qint64 nStopSize = qMin<qint64>(nProbeSize, nBlockSize);
    QByteArray baBlock;
    if (!ltecDecodeBlock(packed, nStopSize, &baBlock, pPdStruct)) return false;
    return qint64(baBlock.size()) >= nStopSize;
}
