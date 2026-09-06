/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xbsndecoder.h"

#include <QVector>

#include <limits>

namespace {
const qint32 BSN_NC = 510;          // literal/length alphabet (256 + 254 lengths)
const qint32 BSN_CBIT = 9;          // width of the literal-table symbol count
const qint32 BSN_NT = 19;           // pre-table alphabet
const qint32 BSN_TBIT = 5;          // width of the pre-table symbol count
const qint32 BSN_NP = 16;           // position alphabet -> 15-bit distances
const qint32 BSN_PBIT = 5;          // width of the position-table symbol count
const qint32 BSN_THRESHOLD = 3;     // shortest encodable match
const qint32 BSN_PT_SPECIAL = 3;    // pre-table index carrying the 2-bit zero run
const qint32 BSN_MAX_CODE_LENGTH = 16;
const qint32 BSN_WINDOW_SIZE = 32768;
const qint32 BSN_WINDOW_MASK = BSN_WINDOW_SIZE - 1;
// Member sizes are u32 in the container, so this ceiling is the format's own
// limit rather than a policy choice; it only keeps the allocation below from
// being driven past what the header could ever legitimately declare.
const qint64 BSN_MAX_OUTPUT = Q_INT64_C(0xffffffff);
const qint64 BSN_CANCEL_CHECK_INTERVAL = 0x10000;

// MSB-first bit reader.  Every read is bounds-checked against the packed
// buffer: a truncated member must fail, never read past the payload into the
// next member's header.
class BsnBitReader
{
public:
    explicit BsnBitReader(const QByteArray &data)
        : m_pData(reinterpret_cast<const quint8 *>(data.constData())),
          m_nBitCount(qint64(data.size()) * 8), m_nBitPosition(0)
    {
    }

    bool readBits(qint32 nCount, quint32 *pValue)
    {
        if (!pValue || (nCount < 0) || (nCount > 24) ||
            (m_nBitPosition > m_nBitCount - nCount)) {
            return false;
        }
        quint32 nResult = 0;
        for (qint32 i = 0; i < nCount; ++i) {
            const quint32 nBit =
                (m_pData[m_nBitPosition >> 3] >> (7 - (m_nBitPosition & 7))) & 1U;
            nResult = (nResult << 1) | nBit;
            ++m_nBitPosition;
        }
        *pValue = nResult;
        return true;
    }

    qint64 consumedSize() const { return (m_nBitPosition + 7) / 8; }

private:
    const quint8 *m_pData;
    qint64 m_nBitCount;
    qint64 m_nBitPosition;
};

// Canonical Huffman decode table.  bConstant covers LHA's "n == 0" shape,
// where the block declares a single symbol and spends no bits on it; that
// path is not theoretical here - the position table takes it in this corpus.
struct BsnTree
{
    quint32 nCount[BSN_MAX_CODE_LENGTH + 1];
    quint32 nFirstCode[BSN_MAX_CODE_LENGTH + 1];
    qint32 nFirstIndex[BSN_MAX_CODE_LENGTH + 1];
    QVector<quint16> vSymbols;
    qint32 nMaxLength;
    bool bConstant;
    quint32 nConstantSymbol;
};

void bsnResetTree(BsnTree *pTree)
{
    for (qint32 i = 0; i <= BSN_MAX_CODE_LENGTH; ++i) {
        pTree->nCount[i] = 0;
        pTree->nFirstCode[i] = 0;
        pTree->nFirstIndex[i] = 0;
    }
    pTree->vSymbols.clear();
    pTree->nMaxLength = 0;
    pTree->bConstant = false;
    pTree->nConstantSymbol = 0;
}

// Every table this format emits is Kraft-complete (verified over 1292 blocks
// of the reference corpus).  Rejecting incomplete and over-subscribed tables
// therefore costs nothing on genuine input and turns a mis-parse into an
// immediate failure instead of a plausible-looking wrong plaintext.
bool bsnBuildTree(const QVector<quint8> &vLengths, BsnTree *pTree)
{
    bsnResetTree(pTree);

    qint32 nMaxLength = 0;
    for (qint32 i = 0; i < vLengths.size(); ++i) {
        const quint8 nLength = vLengths.at(i);
        if (nLength > BSN_MAX_CODE_LENGTH) return false;
        if (nLength > 0) {
            ++pTree->nCount[nLength];
            if (nLength > nMaxLength) nMaxLength = nLength;
        }
    }
    if (nMaxLength == 0) return false;
    pTree->nMaxLength = nMaxLength;

    qint64 nLeft = 1;
    quint32 nCode = 0;
    qint32 nIndex = 0;
    for (qint32 nLength = 1; nLength <= nMaxLength; ++nLength) {
        nLeft = (nLeft << 1) - pTree->nCount[nLength];
        if (nLeft < 0) return false;
        pTree->nFirstCode[nLength] = nCode;
        pTree->nFirstIndex[nLength] = nIndex;
        nCode = (nCode + pTree->nCount[nLength]) << 1;
        nIndex += qint32(pTree->nCount[nLength]);
    }
    if (nLeft != 0) return false;

    pTree->vSymbols.resize(nIndex);
    QVector<qint32> vNext(BSN_MAX_CODE_LENGTH + 1, 0);
    for (qint32 nLength = 1; nLength <= nMaxLength; ++nLength) {
        vNext[nLength] = pTree->nFirstIndex[nLength];
    }
    for (qint32 i = 0; i < vLengths.size(); ++i) {
        const quint8 nLength = vLengths.at(i);
        if (nLength > 0) pTree->vSymbols[vNext[nLength]++] = quint16(i);
    }
    return true;
}

bool bsnDecodeSymbol(BsnBitReader *pReader, const BsnTree &tree,
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
        if (tree.nCount[nLength] &&
            (nCode >= tree.nFirstCode[nLength]) &&
            (nCode - tree.nFirstCode[nLength] < tree.nCount[nLength])) {
            *pSymbol = tree.vSymbols.at(
                tree.nFirstIndex[nLength] +
                qint32(nCode - tree.nFirstCode[nLength]));
            return true;
        }
    }
    return false;
}

// Pre-table / position-table reader.  The 3-bit length escape at value 7 is
// extended by counting following 1-bits AND consuming the terminating 0 bit
// (LHA's fillbuf(c - 3)).  Dropping that consume yields Kraft-invalid tables
// on about two thirds of the members while the short ones still decode
// perfectly - a very convincing partial success, so it is called out here.
bool bsnReadPtLen(BsnBitReader *pReader, qint32 nAlphabet, qint32 nCountBits,
                  qint32 nSpecialIndex, BsnTree *pTree)
{
    bsnResetTree(pTree);

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
                if (nLength > quint32(BSN_MAX_CODE_LENGTH)) return false;
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
    return bsnBuildTree(vLengths, pTree);
}

bool bsnReadCLen(BsnBitReader *pReader, const BsnTree &preTree, BsnTree *pTree)
{
    bsnResetTree(pTree);

    quint32 nNumber = 0;
    if (!pReader->readBits(BSN_CBIT, &nNumber)) return false;
    if (nNumber == 0) {
        quint32 nSymbol = 0;
        if (!pReader->readBits(BSN_CBIT, &nSymbol) ||
            (nSymbol >= quint32(BSN_NC))) {
            return false;
        }
        pTree->bConstant = true;
        pTree->nConstantSymbol = nSymbol;
        return true;
    }
    if (nNumber > quint32(BSN_NC)) return false;

    QVector<quint8> vLengths(BSN_NC, 0);
    qint32 i = 0;
    while (i < qint32(nNumber)) {
        quint32 nSymbol = 0;
        if (!bsnDecodeSymbol(pReader, preTree, &nSymbol)) return false;
        if (nSymbol <= 2) {
            quint32 nZeroRun = 0;
            if (nSymbol == 0) {
                nZeroRun = 1;
            } else if (nSymbol == 1) {
                if (!pReader->readBits(4, &nZeroRun)) return false;
                nZeroRun += 3;
            } else {
                if (!pReader->readBits(BSN_CBIT, &nZeroRun)) return false;
                nZeroRun += 20;
            }
            while ((nZeroRun > 0) && (i < qint32(nNumber))) {
                vLengths[i++] = 0;
                --nZeroRun;
            }
        } else {
            if ((nSymbol - 2) > quint32(BSN_MAX_CODE_LENGTH)) return false;
            vLengths[i++] = quint8(nSymbol - 2);
        }
    }
    return bsnBuildTree(vLengths, pTree);
}
}  // namespace

qint32 XBSNDecoder::windowSize()
{
    return BSN_WINDOW_SIZE;
}

bool XBSNDecoder::decode(const QByteArray &packed, qint64 nUncompressedSize,
                         const QByteArray &baHistory, QByteArray *pOutput,
                         XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput) return false;
    pOutput->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > BSN_MAX_OUTPUT) ||
        (nUncompressedSize > qint64((std::numeric_limits<qint32>::max)()))) {
        return false;
    }
    if (nUncompressedSize == 0) return packed.isEmpty();
    if (packed.isEmpty()) return false;

    // The dictionary starts as LHA's 0x20 fill and is then overwritten, at its
    // END, by the tail of the preceding plaintext.  With w_pos parked at 0 a
    // distance d resolves to window[SIZE - d], which is history[n - d] for
    // d <= n and stays 0x20 beyond it - exactly the solid semantics the
    // corpus was validated against.
    QByteArray baWindow(BSN_WINDOW_SIZE, ' ');
    const qint32 nHistorySize =
        qMin<qint32>(BSN_WINDOW_SIZE, qint32(qMin<qint64>(
                         baHistory.size(), BSN_WINDOW_SIZE)));
    if (nHistorySize > 0) {
        memcpy(baWindow.data() + (BSN_WINDOW_SIZE - nHistorySize),
               baHistory.constData() + (baHistory.size() - nHistorySize),
               size_t(nHistorySize));
    }
    quint8 *pWindow = reinterpret_cast<quint8 *>(baWindow.data());
    qint32 nWindowPosition = 0;

    QByteArray baResult;
    baResult.reserve(qint32(nUncompressedSize));

    BsnBitReader reader(packed);
    BsnTree preTree = {};
    BsnTree literalTree = {};
    BsnTree positionTree = {};
    bsnResetTree(&preTree);
    bsnResetTree(&literalTree);
    bsnResetTree(&positionTree);

    quint32 nBlockRemaining = 0;
    qint64 nNextCancelCheck = BSN_CANCEL_CHECK_INTERVAL;
    while (baResult.size() < nUncompressedSize) {
        if (baResult.size() >= nNextCancelCheck) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            nNextCancelCheck = baResult.size() + BSN_CANCEL_CHECK_INTERVAL;
        }
        if (nBlockRemaining == 0) {
            // A member may hold several blocks; each one restates all three
            // tables in-stream, immediately after the 16-bit symbol count.
            if (!reader.readBits(16, &nBlockRemaining) ||
                (nBlockRemaining == 0)) {
                return false;
            }
            if (!bsnReadPtLen(&reader, BSN_NT, BSN_TBIT, BSN_PT_SPECIAL,
                              &preTree) ||
                !bsnReadCLen(&reader, preTree, &literalTree) ||
                !bsnReadPtLen(&reader, BSN_NP, BSN_PBIT, -1, &positionTree)) {
                return false;
            }
        }
        --nBlockRemaining;

        quint32 nSymbol = 0;
        if (!bsnDecodeSymbol(&reader, literalTree, &nSymbol)) return false;
        if (nSymbol < 256) {
            baResult.append(char(quint8(nSymbol)));
            pWindow[nWindowPosition] = quint8(nSymbol);
            nWindowPosition = (nWindowPosition + 1) & BSN_WINDOW_MASK;
            continue;
        }

        const qint32 nMatchLength =
            qint32(nSymbol) - 256 + BSN_THRESHOLD;
        quint32 nPositionSymbol = 0;
        if (!bsnDecodeSymbol(&reader, positionTree, &nPositionSymbol) ||
            (nPositionSymbol >= quint32(BSN_NP))) {
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
        // A genuine stream never overruns the declared member size, and a
        // distance can never exceed the dictionary.  Both are hard errors:
        // silently clamping either one would emit wrong bytes.
        if ((nDistance > quint32(BSN_WINDOW_SIZE)) ||
            (qint64(baResult.size()) + nMatchLength > nUncompressedSize)) {
            return false;
        }

        qint32 nCopyPosition =
            (nWindowPosition - qint32(nDistance)) & BSN_WINDOW_MASK;
        for (qint32 i = 0; i < nMatchLength; ++i) {
            const quint8 nByte = pWindow[nCopyPosition];
            nCopyPosition = (nCopyPosition + 1) & BSN_WINDOW_MASK;
            baResult.append(char(nByte));
            pWindow[nWindowPosition] = nByte;
            nWindowPosition = (nWindowPosition + 1) & BSN_WINDOW_MASK;
        }
    }

    if ((baResult.size() != nUncompressedSize) ||
        (reader.consumedSize() > packed.size()) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    *pOutput = baResult;
    return true;
}
