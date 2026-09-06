/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xinfogramespakdecoder.h"

#include <limits>

namespace
{
const qint32 IPK_MAXBITS = 16;      // longest Shannon-Fano code observed / legal
const qint32 IPK_NSYMBOLS = 64;     // both trees are exactly 64 symbols wide
const qint32 IPK_DICT_BITS = 6;     // 4K window: 6 distance bits read verbatim
const qint32 IPK_MIN_MATCH = 2;     // NOT 3 - see the header comment
const qint32 IPK_LENGTH_ESCAPE = 63;  // length symbol 63 takes 8 extra bits
const qint64 IPK_MAX_OUTPUT = 0x10000000;  // 256 MB sanity cap

// LSB-first bit reader.  Never reads past the end of the packed buffer: a
// truncated stream fails instead of decoding zero bits.
class IpkBits
{
public:
    IpkBits(const quint8 *pData, qint64 nSize)
        : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nBuffer(0),
          m_nCount(0)
    {
    }

    bool read(qint32 nNeed, quint32 *pValue)
    {
        if (!pValue || nNeed < 0 || nNeed > 8) return false;
        while (m_nCount < nNeed) {
            if (m_nPosition >= m_nSize) return false;
            m_nBuffer |= quint32(m_pData[m_nPosition]) << m_nCount;
            ++m_nPosition;
            m_nCount += 8;
        }
        *pValue = m_nBuffer & ((quint32(1) << nNeed) - 1);
        m_nBuffer >>= nNeed;
        m_nCount -= nNeed;
        return true;
    }

    qint64 position() const { return m_nPosition; }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint32 m_nBuffer;
    qint32 m_nCount;
};

// Canonical prefix code, ordered by (length, symbol) ascending.  The bits of a
// code arrive least-significant-byte-first but most-significant-bit-first
// within the code, and each bit is inverted - the PKWARE convention.
struct IpkTree
{
    qint32 nCount[IPK_MAXBITS + 1];
    qint32 nSymbol[IPK_NSYMBOLS];
};

// A tree is stored as: one count byte (pairs - 1), then that many pair bytes.
// Each pair byte carries (bit length - 1) in the low nibble and
// (repeat count - 1) in the high nibble; the runs must cover all 64 symbols
// exactly.  That exact-cover rule is the format's cheapest self-check.
bool ipkReadTree(const quint8 *pData, qint64 nSize, qint64 *pPosition,
                 IpkTree *pTree)
{
    if (!pData || !pPosition || !pTree) return false;
    qint64 nPosition = *pPosition;
    if ((nPosition < 0) || (nPosition >= nSize)) return false;

    const qint32 nPairs = qint32(pData[nPosition]) + 1;
    ++nPosition;

    quint8 lengths[IPK_NSYMBOLS];
    qint32 nFilled = 0;
    for (qint32 i = 0; i < nPairs; ++i) {
        if (nPosition >= nSize) return false;
        const quint8 nPair = pData[nPosition];
        ++nPosition;
        const qint32 nLength = qint32(nPair & 0x0fU) + 1;
        const qint32 nRepeat = qint32((nPair >> 4) & 0x0fU) + 1;
        if (nFilled + nRepeat > IPK_NSYMBOLS) return false;
        for (qint32 j = 0; j < nRepeat; ++j) {
            lengths[nFilled] = quint8(nLength);
            ++nFilled;
        }
    }
    if (nFilled != IPK_NSYMBOLS) return false;

    for (qint32 i = 0; i <= IPK_MAXBITS; ++i) pTree->nCount[i] = 0;
    for (qint32 i = 0; i < IPK_NSYMBOLS; ++i) {
        pTree->nCount[lengths[i]]++;
    }

    qint32 offsets[IPK_MAXBITS + 2];
    offsets[0] = 0;
    offsets[1] = 0;
    for (qint32 nLength = 1; nLength <= IPK_MAXBITS; ++nLength) {
        offsets[nLength + 1] = offsets[nLength] + pTree->nCount[nLength];
    }
    for (qint32 i = 0; i < IPK_NSYMBOLS; ++i) {
        pTree->nSymbol[offsets[lengths[i]]] = i;
        offsets[lengths[i]]++;
    }

    *pPosition = nPosition;
    return true;
}

bool ipkDecodeSymbol(IpkBits *pBits, const IpkTree *pTree, qint32 *pSymbol)
{
    if (!pBits || !pTree || !pSymbol) return false;
    qint32 nCode = 0;
    qint32 nFirst = 0;
    qint32 nIndex = 0;
    for (qint32 nLength = 1; nLength <= IPK_MAXBITS; ++nLength) {
        quint32 nBit = 0;
        if (!pBits->read(1, &nBit)) return false;
        nCode |= qint32(nBit ^ 1U);
        const qint32 nCount = pTree->nCount[nLength];
        if ((nCode - nFirst) < nCount) {
            const qint32 nPosition = nIndex + (nCode - nFirst);
            if ((nPosition < 0) || (nPosition >= IPK_NSYMBOLS)) return false;
            *pSymbol = pTree->nSymbol[nPosition];
            return true;
        }
        nIndex += nCount;
        nFirst += nCount;
        nFirst <<= 1;
        nCode <<= 1;
    }
    return false;
}
}  // namespace

bool XInfogramesPakDecoder::decode(const QByteArray &packed,
                                   qint64 nExpectedSize, QByteArray *pOutput,
                                   qint64 *pConsumedSize,
                                   XBinary::PDSTRUCT *pPdStruct)
{
    if (pConsumedSize) *pConsumedSize = 0;
    if (!pOutput) return false;
    pOutput->clear();
    if ((nExpectedSize < 0) || (nExpectedSize > IPK_MAX_OUTPUT)) return false;
    if (packed.isEmpty()) return nExpectedSize == 0;

    const quint8 *pData = reinterpret_cast<const quint8 *>(packed.constData());
    const qint64 nSize = packed.size();

    IpkTree lengthTree;
    IpkTree distanceTree;
    qint64 nPosition = 0;
    if (!ipkReadTree(pData, nSize, &nPosition, &lengthTree)) return false;
    if (!ipkReadTree(pData, nSize, &nPosition, &distanceTree)) return false;

    QByteArray output;
    output.reserve(qint32(nExpectedSize));

    IpkBits bits(pData + nPosition, nSize - nPosition);
    qint64 nProduced = 0;
    qint32 nCheck = 0;

    while (nProduced < nExpectedSize) {
        if (((++nCheck) & 0x3ff) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }

        quint32 nFlag = 0;
        if (!bits.read(1, &nFlag)) return false;

        if (nFlag) {
            quint32 nLiteral = 0;
            if (!bits.read(8, &nLiteral)) return false;
            output.append(char(quint8(nLiteral)));
            ++nProduced;
            continue;
        }

        quint32 nLow = 0;
        if (!bits.read(IPK_DICT_BITS, &nLow)) return false;
        qint32 nDistanceCode = 0;
        if (!ipkDecodeSymbol(&bits, &distanceTree, &nDistanceCode)) return false;
        const qint64 nDistance =
            (qint64(nDistanceCode) << IPK_DICT_BITS) + qint64(nLow) + 1;

        qint32 nLengthCode = 0;
        if (!ipkDecodeSymbol(&bits, &lengthTree, &nLengthCode)) return false;
        qint64 nLength = nLengthCode;
        if (nLengthCode == IPK_LENGTH_ESCAPE) {
            quint32 nExtra = 0;
            if (!bits.read(8, &nExtra)) return false;
            nLength += qint64(nExtra);
        }
        nLength += IPK_MIN_MATCH;

        // Copies that reach in front of the member read the zero-filled
        // window.  This is not a corruption guard: real archives depend on it
        // (a leading run of zeroes is coded as a back-reference), so the
        // substitution has to be exact rather than an early reject.
        for (qint64 i = 0; i < nLength; ++i) {
            const qint64 nSource = qint64(output.size()) - nDistance;
            if (nSource >= 0) {
                output.append(output.at(qint32(nSource)));
            } else {
                output.append(char(0));
            }
            ++nProduced;
            if (nProduced >= nExpectedSize + 0x1000) return false;
        }
    }

    // The final copy may overshoot; the declared size is authoritative.
    if (qint64(output.size()) < nExpectedSize) return false;
    output.resize(qint32(nExpectedSize));

    *pOutput = output;
    if (pConsumedSize) *pConsumedSize = nPosition + bits.position();
    return true;
}
