/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xasymetrixdecoder.h"

#include <QtEndian>

#include <array>
#include <limits>

namespace {
const qint64 ASYMDEC_BLOCK_HEADER_SIZE = 6;
const qint64 ASYMDEC_BLOCK_UNPACKED_SIZE = 4096;
// The corpus never exceeds 4096 packed bytes per block; the wider ceiling only
// keeps a hypothetical expanding block readable while still bounding the
// allocation that a malformed length field can ask for.
const qint64 ASYMDEC_MAX_BLOCK_SIZE = 0x10000;
const quint16 ASYMDEC_METHOD_STORED = 0;
const quint16 ASYMDEC_METHOD_IMPLODE = 1;

// Bounds-oriented PKWARE Data Compression Library decoder, adapted from Mark
// Adler's zlib-licensed blast 1.3 algorithm.  Each Asymetrix block is a
// complete DCL stream: it opens with the literal-mode and dictionary-bit
// selector bytes (always 00 05 here) and closes with the 519 end code, so the
// blocks are decoded independently of one another.
const int ASYMDEC_DCL_MAX_BITS = 13;

struct AsymDclHuffman {
    std::array<short, ASYMDEC_DCL_MAX_BITS + 1> count;
    std::array<short, 256> symbol;
};

bool asymDclConstruct(AsymDclHuffman *pTable, const uchar *pRepeat,
                      int nRepeatCount)
{
    if (!pTable || !pRepeat || nRepeatCount <= 0) return false;
    std::array<short, 256> lengths = {};
    int nSymbols = 0;
    for (int i = 0; i < nRepeatCount; ++i) {
        const int nRun = (pRepeat[i] >> 4) + 1;
        const int nLength = pRepeat[i] & 15;
        if (nLength > ASYMDEC_DCL_MAX_BITS ||
            nSymbols > int(lengths.size()) - nRun) {
            return false;
        }
        for (int j = 0; j < nRun; ++j) lengths[nSymbols++] = short(nLength);
    }
    pTable->count.fill(0);
    pTable->symbol.fill(0);
    for (int i = 0; i < nSymbols; ++i) ++pTable->count[lengths[i]];
    if (pTable->count[0] == nSymbols) return false;
    int nLeft = 1;
    for (int nLength = 1; nLength <= ASYMDEC_DCL_MAX_BITS; ++nLength) {
        nLeft = (nLeft << 1) - pTable->count[nLength];
        if (nLeft < 0) return false;
    }
    std::array<short, ASYMDEC_DCL_MAX_BITS + 1> offsets = {};
    offsets[1] = 0;
    for (int nLength = 1; nLength < ASYMDEC_DCL_MAX_BITS; ++nLength) {
        offsets[nLength + 1] = offsets[nLength] + pTable->count[nLength];
    }
    for (int i = 0; i < nSymbols; ++i) {
        const int nLength = lengths[i];
        if (nLength) pTable->symbol[offsets[nLength]++] = short(i);
    }
    return true;
}

struct AsymDclTables {
    AsymDclHuffman literal;
    AsymDclHuffman length;
    AsymDclHuffman distance;
    bool bValid = false;

    AsymDclTables()
    {
        static const uchar literalLengths[] = {
            11,  124, 8,   7,   28,  7,   188, 13,  76,  4,   10,  8,   12,
            10,  12,  10,  8,   23,  8,   9,   7,   6,   7,   8,   7,   6,
            55,  8,   23,  24,  12,  11,  7,   9,   11,  12,  6,   7,   22,
            5,   7,   24,  6,   11,  9,   6,   7,   22,  7,   11,  38,  7,
            9,   8,   25,  11,  8,   11,  9,   12,  8,   12,  5,   38,  5,
            38,  5,   11,  7,   5,   6,   21,  6,   10,  53,  8,   7,   24,
            10,  27,  44,  253, 253, 253, 252, 252, 252, 13,  12,  45,  12,
            45,  12,  61,  12,  45,  44,  173};
        static const uchar lengthLengths[] = {2, 35, 36, 53, 38, 23};
        static const uchar distanceLengths[] = {2, 20, 53, 230, 247, 151, 248};
        bValid = asymDclConstruct(&literal, literalLengths,
                                  int(sizeof(literalLengths))) &&
                 asymDclConstruct(&length, lengthLengths,
                                  int(sizeof(lengthLengths))) &&
                 asymDclConstruct(&distance, distanceLengths,
                                  int(sizeof(distanceLengths)));
    }
};

class AsymDclBits {
public:
    AsymDclBits(const uchar *pBytes, int nSize) : m_pBytes(pBytes), m_nSize(nSize)
    {
    }

    bool read(int nCount, int *pnValue)
    {
        if (!pnValue || nCount < 0 || nCount > 16) return false;
        while (m_nBitCount < nCount) {
            if (m_nPosition >= m_nSize) return false;
            m_nBuffer |= quint32(m_pBytes[m_nPosition++]) << m_nBitCount;
            m_nBitCount += 8;
        }
        *pnValue = nCount ? int(m_nBuffer & ((1U << nCount) - 1U)) : 0;
        m_nBuffer >>= nCount;
        m_nBitCount -= nCount;
        return true;
    }

private:
    const uchar *m_pBytes = nullptr;
    int m_nSize = 0;
    int m_nPosition = 0;
    quint32 m_nBuffer = 0;
    int m_nBitCount = 0;
};

bool asymDclSymbol(AsymDclBits *pBits, const AsymDclHuffman &table,
                   int *pnSymbol)
{
    if (!pBits || !pnSymbol) return false;
    int nCode = 0, nFirst = 0, nIndex = 0;
    for (int nLength = 1; nLength <= ASYMDEC_DCL_MAX_BITS; ++nLength) {
        int nBit = 0;
        if (!pBits->read(1, &nBit)) return false;
        // DCL stores its canonical codes inverted, which is why the incoming
        // bit is complemented before it joins the code being assembled.
        nCode |= nBit ^ 1;
        const int nCount = table.count[nLength];
        if (nCode < nFirst + nCount) {
            const int nSymbolIndex = nIndex + nCode - nFirst;
            if (nSymbolIndex < 0 || nSymbolIndex >= int(table.symbol.size())) {
                return false;
            }
            *pnSymbol = table.symbol[nSymbolIndex];
            return true;
        }
        nIndex += nCount;
        nFirst = (nFirst + nCount) << 1;
        nCode <<= 1;
    }
    return false;
}

// Explodes one block into exactly nExpectedSize bytes, appending to pbaOutput.
// The exact-size requirement is the format's only per-block integrity check:
// the block's unpacked length is implied, not stored.
bool asymDclExplode(const uchar *pPacked, int nPackedSize, qint64 nExpectedSize,
                    QByteArray *pbaOutput, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaOutput || !pPacked || nPackedSize < 3 || nExpectedSize < 0 ||
        nExpectedSize > (std::numeric_limits<int>::max)()) {
        return false;
    }
    static const AsymDclTables tables;
    if (!tables.bValid) return false;

    AsymDclBits bits(pPacked, nPackedSize);
    int nLiteralMode = 0, nDictionaryBits = 0;
    if (!bits.read(8, &nLiteralMode) || !bits.read(8, &nDictionaryBits) ||
        nLiteralMode < 0 || nLiteralMode > 1 || nDictionaryBits < 4 ||
        nDictionaryBits > 6) {
        return false;
    }
    static const int baseLength[16] = {3,  2,  4,  5,  6,  7,   8,   9,
                                       10, 12, 16, 24, 40, 72,  136, 264};
    static const int extraLength[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                        1, 2, 3, 4, 5, 6, 7, 8};

    const qint32 nBase = pbaOutput->size();
    for (;;) {
        if (((pbaOutput->size() - nBase) & 0x3fff) == 0 &&
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        int nIsMatch = 0;
        if (!bits.read(1, &nIsMatch)) return false;
        if (!nIsMatch) {
            int nLiteral = 0;
            if ((nLiteralMode &&
                 !asymDclSymbol(&bits, tables.literal, &nLiteral)) ||
                (!nLiteralMode && !bits.read(8, &nLiteral)) || nLiteral < 0 ||
                nLiteral > 255 ||
                (pbaOutput->size() - nBase) >= nExpectedSize) {
                return false;
            }
            pbaOutput->append(char(nLiteral));
            continue;
        }
        int nLengthSymbol = 0;
        if (!asymDclSymbol(&bits, tables.length, &nLengthSymbol) ||
            nLengthSymbol < 0 || nLengthSymbol >= 16) {
            return false;
        }
        int nExtra = 0;
        if (!bits.read(extraLength[nLengthSymbol], &nExtra)) return false;
        int nLength = baseLength[nLengthSymbol] + nExtra;
        if (nLength == 519) break;  // end of block
        int nDistanceSymbol = 0;
        if (!asymDclSymbol(&bits, tables.distance, &nDistanceSymbol) ||
            nDistanceSymbol < 0 || nDistanceSymbol >= 64) {
            return false;
        }
        const int nLowBits = (nLength == 2) ? 2 : nDictionaryBits;
        int nDistanceLow = 0;
        if (!bits.read(nLowBits, &nDistanceLow)) return false;
        const int nDistance = (nDistanceSymbol << nLowBits) + nDistanceLow + 1;
        // The window never reaches back before this block: each block opens a
        // fresh DCL stream, so the match source is bounded by nBase.
        if (nDistance <= 0 || nDistance > (pbaOutput->size() - nBase) ||
            nLength > nExpectedSize - (pbaOutput->size() - nBase)) {
            return false;
        }
        while (nLength-- > 0) {
            pbaOutput->append(pbaOutput->at(pbaOutput->size() - nDistance));
        }
    }
    return (pbaOutput->size() - nBase) == nExpectedSize;
}
}  // namespace

bool XAsymetrixDecoder::decode(const QByteArray &baPacked,
                               qint64 nUncompressedSize,
                               QByteArray *pbaUnpacked,
                               XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaUnpacked || nUncompressedSize < 0 ||
        nUncompressedSize > (std::numeric_limits<qint32>::max)()) {
        return false;
    }

    QByteArray baResult;
    baResult.reserve(static_cast<qint32>(nUncompressedSize));

    const uchar *pData = reinterpret_cast<const uchar *>(baPacked.constData());
    const qint64 nTotalSize = baPacked.size();
    qint64 nOffset = 0;
    qint64 nProduced = 0;

    while (nProduced < nUncompressedSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (nOffset > nTotalSize - ASYMDEC_BLOCK_HEADER_SIZE) return false;

        const quint16 nMethod = qFromLittleEndian<quint16>(pData + nOffset);
        const quint32 nBlockSize =
            qFromLittleEndian<quint32>(pData + nOffset + 2);
        const qint64 nPayloadOffset = nOffset + ASYMDEC_BLOCK_HEADER_SIZE;
        if ((static_cast<qint64>(nBlockSize) > ASYMDEC_MAX_BLOCK_SIZE) ||
            (static_cast<qint64>(nBlockSize) > nTotalSize - nPayloadOffset)) {
            return false;
        }

        const qint64 nWanted = qMin<qint64>(ASYMDEC_BLOCK_UNPACKED_SIZE,
                                            nUncompressedSize - nProduced);
        if (nMethod == ASYMDEC_METHOD_STORED) {
            // A stored block must be exactly the implied unpacked length.  This
            // is what refuses the 6-zero-byte end-of-volume trailer, i.e. a
            // member whose remaining blocks are on the next disk.
            if (static_cast<qint64>(nBlockSize) != nWanted) return false;
            baResult.append(baPacked.constData() + nPayloadOffset,
                            static_cast<qint32>(nBlockSize));
        } else if (nMethod == ASYMDEC_METHOD_IMPLODE) {
            if (!asymDclExplode(pData + nPayloadOffset,
                                static_cast<int>(nBlockSize), nWanted,
                                &baResult, pPdStruct)) {
                return false;
            }
        } else {
            return false;
        }

        nProduced += nWanted;
        nOffset = nPayloadOffset + static_cast<qint64>(nBlockSize);
    }

    // The producing class sizes the stream to the chain exactly, so anything
    // left over means the two disagree about the framing.
    if ((nOffset != nTotalSize) || (baResult.size() != nUncompressedSize)) {
        return false;
    }

    *pbaUnpacked = baResult;
    return true;
}
