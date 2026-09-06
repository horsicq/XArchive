/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include <limits>

#include "xaldusdecoder.h"

#include <QVector>
#include <QtEndian>

#include "xdcldecoder.h"

namespace {
const qint64 ALDUS_SUBHEADER_SIZE = 22;
const qint64 ALDUS_MAX_BLOCK_COUNT = 0x100000;
const qint64 ALDUS_MAX_BLOCK_SIZE = 0xffff;

// Gen1 LZW: TIFF/PDF variant.
const qint32 ALDUS_LZW_CLEAR = 256;
const qint32 ALDUS_LZW_EOD = 257;
const qint32 ALDUS_LZW_FIRST_FREE = 258;
const qint32 ALDUS_LZW_TABLE_SIZE = 4096;
const qint32 ALDUS_LZW_MIN_BITS = 9;
const qint32 ALDUS_LZW_MAX_BITS = 12;

// Gen3 LZSH: LHA -lh5- parameters.  The 13-bit window (np 14, pbit 4) is not a
// guess - the largest distance-table entry count anywhere in the corpus is 14,
// and reading that field with pbit 5 misparses every file.
const qint32 ALDUS_LZSH_NC = 510;
const qint32 ALDUS_LZSH_CBIT = 9;
const qint32 ALDUS_LZSH_NT = 19;
const qint32 ALDUS_LZSH_TBIT = 5;
const qint32 ALDUS_LZSH_NP = 14;
const qint32 ALDUS_LZSH_PBIT = 4;
const qint32 ALDUS_LZSH_ISPECIAL = 3;
const qint32 ALDUS_LZSH_THRESHOLD = 3;
const qint32 ALDUS_LZSH_MAX_BITLEN = 16;
const qint64 ALDUS_LZSH_BLOCK_PREFIX = 3;
const quint8 ALDUS_LZSH_METHOD_STORED = 0;
const quint8 ALDUS_LZSH_METHOD_LH5 = 1;

enum ALDUS_CODEC {
    ALDUS_CODEC_LZW = 0,
    ALDUS_CODEC_PKZP,
    ALDUS_CODEC_LZSH
};

// MSB-first bit reader.  Reading past the end is always a decoding error here:
// the even-length padding leaves spare bits at the tail of a block, never a
// short one, so a request that runs off the end means the stream is malformed.
class AldusBitReader {
public:
    AldusBitReader(const uchar *pData, qint64 nSize)
        : m_pData(pData), m_nBitCount(nSize * 8), m_nBitPosition(0),
          m_bError(false)
    {
    }

    quint32 read(qint32 nBits)
    {
        quint32 nResult = 0;
        if (nBits <= 0) return 0;
        if (m_bError || (nBits > 32) ||
            (nBits > m_nBitCount - m_nBitPosition)) {
            m_bError = true;
            return 0;
        }
        for (qint32 i = 0; i < nBits; ++i) {
            const qint64 nBit = m_nBitPosition + i;
            nResult = (nResult << 1) |
                      ((m_pData[nBit >> 3] >> (7 - (nBit & 7))) & 1U);
        }
        m_nBitPosition += nBits;
        return nResult;
    }

    bool hasError() const
    {
        return m_bError;
    }

    qint64 bitsLeft() const
    {
        return m_nBitCount - m_nBitPosition;
    }

private:
    const uchar *m_pData;
    qint64 m_nBitCount;
    qint64 m_nBitPosition;
    bool m_bError;
};

// Canonical MSB-first Huffman table.  LHA also codes "one symbol, zero bits",
// which is what the constant form represents.
class AldusHuffman {
public:
    AldusHuffman() : m_bConstant(false), m_nConstant(-1)
    {
        reset();
    }

    void setConstant(qint32 nSymbol)
    {
        reset();
        m_bConstant = true;
        m_nConstant = nSymbol;
    }

    bool build(const quint8 *pLengths, qint32 nCount)
    {
        reset();
        if (!pLengths || (nCount <= 0)) return false;
        qint32 nTotal = 0;
        for (qint32 i = 0; i < nCount; ++i) {
            const quint8 nLength = pLengths[i];
            if (nLength > ALDUS_LZSH_MAX_BITLEN) return false;
            if (nLength) {
                ++m_nCount[nLength];
                ++nTotal;
            }
        }
        if (!nTotal) return false;
        // Reject an over-subscribed code before it can be walked: a decoder
        // that trusts the lengths would otherwise resolve a code to a symbol
        // that the encoder never assigned.
        qint32 nLeft = 1;
        for (qint32 nLength = 1; nLength <= ALDUS_LZSH_MAX_BITLEN; ++nLength) {
            nLeft <<= 1;
            nLeft -= m_nCount[nLength];
            if (nLeft < 0) return false;
        }
        qint32 nOffsets[ALDUS_LZSH_MAX_BITLEN + 2] = {};
        for (qint32 nLength = 1; nLength <= ALDUS_LZSH_MAX_BITLEN; ++nLength) {
            nOffsets[nLength + 1] = nOffsets[nLength] + m_nCount[nLength];
        }
        m_listSymbols.resize(nTotal);
        for (qint32 i = 0; i < nCount; ++i) {
            const quint8 nLength = pLengths[i];
            if (nLength) m_listSymbols[nOffsets[nLength]++] = i;
        }
        return true;
    }

    qint32 decode(AldusBitReader *pReader) const
    {
        if (m_bConstant) return m_nConstant;
        qint32 nCode = 0;
        qint32 nFirst = 0;
        qint32 nIndex = 0;
        for (qint32 nLength = 1; nLength <= ALDUS_LZSH_MAX_BITLEN; ++nLength) {
            nCode |= qint32(pReader->read(1));
            if (pReader->hasError()) return -1;
            const qint32 nCount = m_nCount[nLength];
            if ((nCode - nFirst) < nCount) {
                return m_listSymbols.at(nIndex + (nCode - nFirst));
            }
            nIndex += nCount;
            nFirst = (nFirst + nCount) << 1;
            nCode <<= 1;
        }
        return -1;
    }

private:
    void reset()
    {
        m_bConstant = false;
        m_nConstant = -1;
        for (qint32 i = 0; i <= ALDUS_LZSH_MAX_BITLEN; ++i) m_nCount[i] = 0;
        m_listSymbols.clear();
    }

    bool m_bConstant;
    qint32 m_nConstant;
    qint32 m_nCount[ALDUS_LZSH_MAX_BITLEN + 1];
    QVector<qint32> m_listSymbols;
};

quint16 aldusCRC16Arc(const char *pData, qint64 nSize)
{
    quint16 nResult = 0;
    for (qint64 i = 0; i < nSize; ++i) {
        nResult ^= static_cast<quint8>(pData[i]);
        for (qint32 nBit = 0; nBit < 8; ++nBit) {
            nResult = (nResult & 1U) ? quint16((nResult >> 1) ^ 0xa001U)
                                     : quint16(nResult >> 1);
        }
    }
    return nResult;
}

bool aldusDecodeLzwBlock(const uchar *pData, qint64 nSize, qint64 nExpected,
                         QByteArray *pbaBlock)
{
    if (!pData || (nSize < 1) || (nExpected < 1)) return false;

    QVector<quint16> listPrefix(ALDUS_LZW_TABLE_SIZE, 0);
    QVector<quint8> listSuffix(ALDUS_LZW_TABLE_SIZE, 0);
    QVector<quint8> listStack(ALDUS_LZW_TABLE_SIZE, 0);

    QByteArray baResult;
    baResult.reserve(qint32(nExpected));

    const qint64 nBitCount = nSize * 8;
    qint64 nBitPosition = 0;
    qint32 nCodeBits = ALDUS_LZW_MIN_BITS;
    qint32 nNextCode = ALDUS_LZW_FIRST_FREE;
    qint32 nPreviousCode = -1;
    quint8 nPreviousFirst = 0;
    bool bEndSeen = false;

    while ((nBitPosition + nCodeBits) <= nBitCount) {
        quint32 nCode = 0;
        for (qint32 i = 0; i < nCodeBits; ++i) {
            const qint64 nBit = nBitPosition + i;
            nCode = (nCode << 1) |
                    ((pData[nBit >> 3] >> (7 - (nBit & 7))) & 1U);
        }
        nBitPosition += nCodeBits;

        if (nCode == quint32(ALDUS_LZW_CLEAR)) {
            nCodeBits = ALDUS_LZW_MIN_BITS;
            nNextCode = ALDUS_LZW_FIRST_FREE;
            nPreviousCode = -1;
            continue;
        }
        if (nCode == quint32(ALDUS_LZW_EOD)) {
            bEndSeen = true;
            break;
        }
        if (qint32(nCode) > nNextCode) return false;

        qint32 nStackSize = 0;
        qint32 nCurrent = qint32(nCode);
        if (nCurrent == nNextCode) {
            // KwKwK: the entry being defined right now, resolvable only
            // through the previous entry plus its own first character.
            if (nPreviousCode < 0) return false;
            listStack[nStackSize++] = nPreviousFirst;
            nCurrent = nPreviousCode;
        }
        while (nCurrent >= ALDUS_LZW_CLEAR) {
            if ((nCurrent >= ALDUS_LZW_TABLE_SIZE) ||
                (nStackSize >= ALDUS_LZW_TABLE_SIZE)) {
                return false;
            }
            listStack[nStackSize++] = listSuffix.at(nCurrent);
            nCurrent = listPrefix.at(nCurrent);
        }
        if (nStackSize >= ALDUS_LZW_TABLE_SIZE) return false;
        listStack[nStackSize++] = quint8(nCurrent);
        const quint8 nFirstCharacter = quint8(nCurrent);

        if (qint64(nStackSize) > (nExpected - baResult.size())) return false;
        while (nStackSize) {
            baResult.append(char(listStack.at(--nStackSize)));
        }

        if (nPreviousCode >= 0) {
            // A full table is not an error: the writer emits a Clear before it
            // needs a new code, and codes above the last defined entry can no
            // longer be referenced, so further definitions are simply dropped.
            if (nNextCode < ALDUS_LZW_TABLE_SIZE) {
                listPrefix[nNextCode] = quint16(nPreviousCode);
                listSuffix[nNextCode] = nFirstCharacter;
                ++nNextCode;
            }
        }
        nPreviousCode = qint32(nCode);
        nPreviousFirst = nFirstCharacter;

        // Early change: widen one code before the width is actually needed.
        if ((nCodeBits < ALDUS_LZW_MAX_BITS) &&
            ((nNextCode + 1) >= (1 << nCodeBits))) {
            ++nCodeBits;
        }
    }

    // Every block in the family carries an explicit EOD.  Requiring it keeps a
    // truncated block from being reported as a successful short decode.
    if (!bEndSeen || (baResult.size() != nExpected)) return false;
    *pbaBlock = baResult;
    return true;
}

bool aldusDecodePkzpBlock(const uchar *pData, qint64 nSize, qint64 nExpected,
                          QByteArray *pbaBlock)
{
    if (!pData || (nSize < 3) || (nExpected < 1)) return false;
    const QByteArray baBlock =
        QByteArray::fromRawData(reinterpret_cast<const char *>(pData),
                                qint32(nSize));
    QByteArray baResult;
    if (!XDclDecoder::decode(baBlock, &baResult, nExpected)) return false;
    if (baResult.size() != nExpected) return false;
    *pbaBlock = baResult;
    return true;
}

bool aldusReadPtLen(AldusBitReader *pReader, qint32 nCount, qint32 nBits,
                    qint32 nSpecial, AldusHuffman *pHuffman)
{
    const qint32 nEntries = qint32(pReader->read(nBits));
    if (pReader->hasError()) return false;
    if (nEntries == 0) {
        const qint32 nSymbol = qint32(pReader->read(nBits));
        if (pReader->hasError() || (nSymbol >= nCount)) return false;
        pHuffman->setConstant(nSymbol);
        return true;
    }
    if (nEntries > nCount) return false;

    QVector<quint8> listLengths(nCount, 0);
    qint32 i = 0;
    while (i < nEntries) {
        qint32 nLength = qint32(pReader->read(3));
        if (pReader->hasError()) return false;
        if (nLength == 7) {
            // Lengths above 6 are unary-extended by a run of 1 bits.
            while (true) {
                const quint32 nBit = pReader->read(1);
                if (pReader->hasError()) return false;
                if (!nBit) break;
                ++nLength;
                if (nLength > ALDUS_LZSH_MAX_BITLEN) return false;
            }
        }
        listLengths[i++] = quint8(nLength);
        if (i == nSpecial) {
            // The pre-tree reserves a two-bit skip after entry 2.
            qint32 nSkip = qint32(pReader->read(2));
            if (pReader->hasError()) return false;
            while (nSkip > 0) {
                if (i >= nCount) return false;
                listLengths[i++] = 0;
                --nSkip;
            }
        }
    }
    return pHuffman->build(listLengths.constData(), nCount);
}

bool aldusReadCLen(AldusBitReader *pReader, const AldusHuffman &preTable,
                   AldusHuffman *pHuffman)
{
    const qint32 nEntries = qint32(pReader->read(ALDUS_LZSH_CBIT));
    if (pReader->hasError()) return false;
    if (nEntries == 0) {
        const qint32 nSymbol = qint32(pReader->read(ALDUS_LZSH_CBIT));
        if (pReader->hasError() || (nSymbol >= ALDUS_LZSH_NC)) return false;
        pHuffman->setConstant(nSymbol);
        return true;
    }

    QVector<quint8> listLengths(ALDUS_LZSH_NC, 0);
    qint32 i = 0;
    while (i < nEntries) {
        const qint32 nSymbol = preTable.decode(pReader);
        if ((nSymbol < 0) || pReader->hasError()) return false;
        if (nSymbol <= 2) {
            qint32 nRun = 1;
            if (nSymbol == 1) {
                nRun = qint32(pReader->read(4)) + 3;
            } else if (nSymbol == 2) {
                nRun = qint32(pReader->read(ALDUS_LZSH_CBIT)) + 20;
            }
            if (pReader->hasError()) return false;
            // A zero run is allowed to overshoot the declared count and even
            // the table; the surplus entries are unreachable and dropped
            // rather than treated as corruption.
            while (nRun > 0) {
                if (i < ALDUS_LZSH_NC) listLengths[i] = 0;
                ++i;
                --nRun;
            }
        } else {
            if (i < ALDUS_LZSH_NC) listLengths[i] = quint8(nSymbol - 2);
            ++i;
        }
    }
    return pHuffman->build(listLengths.constData(), ALDUS_LZSH_NC);
}

// One container block holds one or two complete LHA sub-blocks.  The match
// window continues across the sub-blocks of a block but is reset for the next
// block, so the block's own output is the whole reachable history here.
bool aldusDecodeLh5Stream(const uchar *pData, qint64 nSize, qint64 nExpected,
                          QByteArray *pbaBlock)
{
    AldusBitReader reader(pData, nSize);
    QByteArray baResult;
    baResult.reserve(qint32(nExpected));

    AldusHuffman literalTable;
    AldusHuffman positionTable;
    qint32 nSymbolsLeft = 0;

    while (baResult.size() < nExpected) {
        if (nSymbolsLeft == 0) {
            nSymbolsLeft = qint32(reader.read(16));
            if (reader.hasError() || (nSymbolsLeft == 0)) return false;
            AldusHuffman preTable;
            if (!aldusReadPtLen(&reader, ALDUS_LZSH_NT, ALDUS_LZSH_TBIT,
                                ALDUS_LZSH_ISPECIAL, &preTable) ||
                !aldusReadCLen(&reader, preTable, &literalTable) ||
                !aldusReadPtLen(&reader, ALDUS_LZSH_NP, ALDUS_LZSH_PBIT, -1,
                                &positionTable)) {
                return false;
            }
        }
        --nSymbolsLeft;

        const qint32 nSymbol = literalTable.decode(&reader);
        if ((nSymbol < 0) || reader.hasError()) return false;
        if (nSymbol < 256) {
            baResult.append(char(quint8(nSymbol)));
            continue;
        }

        const qint32 nLength = nSymbol - 256 + ALDUS_LZSH_THRESHOLD;
        qint32 nDistanceBits = positionTable.decode(&reader);
        if ((nDistanceBits < 0) || (nDistanceBits > ALDUS_LZSH_MAX_BITLEN) ||
            reader.hasError()) {
            return false;
        }
        qint32 nDistanceBase = nDistanceBits;
        if (nDistanceBits > 0) {
            nDistanceBase = (1 << (nDistanceBits - 1)) +
                            qint32(reader.read(nDistanceBits - 1));
            if (reader.hasError()) return false;
        }
        const qint32 nDistance = nDistanceBase + 1;
        if ((nDistance > baResult.size()) ||
            (qint64(nLength) > (nExpected - baResult.size()))) {
            return false;
        }
        for (qint32 i = 0; i < nLength; ++i) {
            baResult.append(baResult.at(baResult.size() - nDistance));
        }
    }

    if (baResult.size() != nExpected) return false;
    *pbaBlock = baResult;
    return true;
}

bool aldusDecodeLzshBlock(const uchar *pData, qint64 nSize, qint64 nExpected,
                          QByteArray *pbaBlock)
{
    if (!pData || (nSize < ALDUS_LZSH_BLOCK_PREFIX) || (nExpected < 1)) {
        return false;
    }
    const quint16 nStoredCRC = qFromLittleEndian<quint16>(pData);
    const quint8 nMethod = pData[2];
    const uchar *pPayload = pData + ALDUS_LZSH_BLOCK_PREFIX;
    const qint64 nPayloadSize = nSize - ALDUS_LZSH_BLOCK_PREFIX;

    QByteArray baResult;
    if (nMethod == ALDUS_LZSH_METHOD_STORED) {
        if (nPayloadSize < nExpected) return false;
        baResult = QByteArray(reinterpret_cast<const char *>(pPayload),
                              qint32(nExpected));
    } else if (nMethod == ALDUS_LZSH_METHOD_LH5) {
        if (!aldusDecodeLh5Stream(pPayload, nPayloadSize, nExpected,
                                  &baResult)) {
            return false;
        }
    } else {
        return false;
    }

    // The block header's CRC-16/ARC covers the block plaintext.  It is the
    // only end-to-end check this generation offers, so a mismatch is a decode
    // failure rather than a warning.
    if (aldusCRC16Arc(baResult.constData(), baResult.size()) != nStoredCRC) {
        return false;
    }
    *pbaBlock = baResult;
    return true;
}

bool aldusDecodeContainer(const QByteArray &baPacked, qint64 nUncompressedSize,
                          QByteArray *pbaUnpacked, ALDUS_CODEC codec)
{
    if (!pbaUnpacked) return false;
    pbaUnpacked->clear();
    if ((nUncompressedSize < 1) ||
        (nUncompressedSize >
         qint64((std::numeric_limits<qint32>::max)()))) {
        return false;
    }
    if (qint64(baPacked.size()) < ALDUS_SUBHEADER_SIZE) return false;

    const uchar *pPayload =
        reinterpret_cast<const uchar *>(baPacked.constData());
    const qint64 nSubHeaderSize = qFromBigEndian<quint16>(pPayload);
    const qint64 nBlockSize = qFromBigEndian<quint16>(pPayload + 2);
    const qint64 nLastBlockSize = qFromBigEndian<quint16>(pPayload + 4);
    const qint64 nBlockCount = qFromBigEndian<quint32>(pPayload + 6);
    const qint64 nTableOffset = qFromBigEndian<quint32>(pPayload + 10);
    const qint64 nDataOffset = qFromBigEndian<quint32>(pPayload + 14);

    if ((nSubHeaderSize != ALDUS_SUBHEADER_SIZE) || (nBlockSize < 1) ||
        (nBlockSize > ALDUS_MAX_BLOCK_SIZE) || (nBlockCount < 1) ||
        (nBlockCount > ALDUS_MAX_BLOCK_COUNT) || (nLastBlockSize < 1) ||
        (nLastBlockSize > nBlockSize)) {
        return false;
    }
    // The sub-header's table and data offsets are absolute file offsets, so in
    // a payload slice only their difference is checkable - and it is exactly
    // the size of the block-length table.
    if ((nDataOffset - nTableOffset) != (2 * nBlockCount)) return false;
    if (((nBlockCount - 1) * nBlockSize + nLastBlockSize) !=
        nUncompressedSize) {
        return false;
    }

    const qint64 nTableRelative = ALDUS_SUBHEADER_SIZE;
    const qint64 nDataRelative = nTableRelative + 2 * nBlockCount;
    if (nDataRelative > qint64(baPacked.size())) return false;

    QVector<qint64> listBlockSizes(qint32(nBlockCount), 0);
    qint64 nTotalBlockSize = 0;
    for (qint64 i = 0; i < nBlockCount; ++i) {
        const qint64 nSize =
            qFromBigEndian<quint16>(pPayload + nTableRelative + 2 * i);
        // Every block is word-padded, so an odd or empty length means the
        // table is not an Aldus block table.
        if ((nSize < 2) || (nSize & 1)) return false;
        listBlockSizes[qint32(i)] = nSize;
        nTotalBlockSize += nSize;
    }
    // The caller hands over the payload up to the end of the block data; the
    // 18-byte zero trailer is deliberately outside the stream.
    if ((nDataRelative + nTotalBlockSize) != qint64(baPacked.size())) {
        return false;
    }

    QByteArray baResult;
    baResult.reserve(qint32(nUncompressedSize));
    qint64 nOffset = nDataRelative;
    for (qint64 i = 0; i < nBlockCount; ++i) {
        const qint64 nSize = listBlockSizes.at(qint32(i));
        const qint64 nExpected =
            (i == (nBlockCount - 1)) ? nLastBlockSize : nBlockSize;
        QByteArray baBlock;
        bool bBlock = false;
        if (codec == ALDUS_CODEC_LZW) {
            bBlock = aldusDecodeLzwBlock(pPayload + nOffset, nSize, nExpected,
                                         &baBlock);
        } else if (codec == ALDUS_CODEC_PKZP) {
            bBlock = aldusDecodePkzpBlock(pPayload + nOffset, nSize, nExpected,
                                          &baBlock);
        } else {
            bBlock = aldusDecodeLzshBlock(pPayload + nOffset, nSize, nExpected,
                                          &baBlock);
        }
        if (!bBlock || (baBlock.size() != nExpected)) return false;
        baResult.append(baBlock);
        nOffset += nSize;
    }

    if (baResult.size() != nUncompressedSize) return false;
    *pbaUnpacked = baResult;
    return true;
}
}  // namespace

bool XAldusDecoder::decodeLZW(const QByteArray &baPacked,
                              qint64 nUncompressedSize,
                              QByteArray *pbaUnpacked)
{
    return aldusDecodeContainer(baPacked, nUncompressedSize, pbaUnpacked,
                                ALDUS_CODEC_LZW);
}

bool XAldusDecoder::decodePKZP(const QByteArray &baPacked,
                               qint64 nUncompressedSize,
                               QByteArray *pbaUnpacked)
{
    return aldusDecodeContainer(baPacked, nUncompressedSize, pbaUnpacked,
                                ALDUS_CODEC_PKZP);
}

bool XAldusDecoder::decodeLZSH(const QByteArray &baPacked,
                               qint64 nUncompressedSize,
                               QByteArray *pbaUnpacked)
{
    return aldusDecodeContainer(baPacked, nUncompressedSize, pbaUnpacked,
                                ALDUS_CODEC_LZSH);
}
