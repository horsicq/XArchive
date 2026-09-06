/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 *
 * Headerless Unix-compress (LZW) decoder.
 *
 * The bit packing, the table-growth trigger and the group realignment are the
 * same as in Algos/xcompressdecoder.cpp; only the stream entry point differs
 * (a one-byte flags header instead of magic + flags).  The two files are kept
 * separate because XCompressDecoder is a device-streaming decoder wired to
 * DATAPROCESS_STATE, while this one decodes a whole bounded member payload.
 */
#include "xborlandpackdecoder.h"

#include <limits>

namespace {
const qint32 BORLANDPACK_LZW_CLEAR = 256;
const qint32 BORLANDPACK_LZW_FIRST = 257;
const qint32 BORLANDPACK_LZW_MINBITS = 9;
const qint32 BORLANDPACK_LZW_MAXBITS = 16;
// Bits 5 and 6 of the flags byte are reserved by the format and must be zero;
// a set bit there means the payload is not a compress stream at all.
const quint8 BORLANDPACK_LZW_FLAGS_RESERVED = 0x60U;
const quint8 BORLANDPACK_LZW_FLAGS_BLOCKMODE = 0x80U;
const quint8 BORLANDPACK_LZW_FLAGS_MAXBITS = 0x1fU;

// LSB-first bit reader over the code stream.  Bit 0 is the low bit of the
// first code byte, which is byte 1 of the payload (byte 0 is the flags byte).
class BorlandPackBitReader {
public:
    BorlandPackBitReader(const char *pData, qint64 nSize)
        : m_pData(pData), m_nTotalBits(nSize * 8), m_nBitPosition(0)
    {
    }

    qint32 readCode(qint32 nCodeBits)
    {
        if ((nCodeBits < 1) || (nCodeBits > BORLANDPACK_LZW_MAXBITS) ||
            (m_nBitPosition > m_nTotalBits - nCodeBits)) {
            return -1;
        }
        qint32 nCode = 0;
        for (qint32 i = 0; i < nCodeBits; ++i) {
            const qint64 nBit = m_nBitPosition + i;
            const quint8 nByte =
                static_cast<quint8>(m_pData[static_cast<qint64>(nBit >> 3)]);
            nCode |= static_cast<qint32>((nByte >> (nBit & 7)) & 1U) << i;
        }
        m_nBitPosition += nCodeBits;
        return nCode;
    }

    bool skipBits(qint64 nCount)
    {
        if ((nCount < 0) || (m_nBitPosition > m_nTotalBits - nCount)) {
            return false;
        }
        m_nBitPosition += nCount;
        return true;
    }

    qint64 bitsRead() const { return m_nBitPosition; }

private:
    const char *m_pData;
    qint64 m_nTotalBits;
    qint64 m_nBitPosition;
};

// compress writes codes in groups of eight.  When the code width changes, or a
// CLEAR resets the dictionary, the tail of the current group is padding and the
// next code starts at the following group boundary.  The group origin is the
// first code bit, i.e. immediately after the flags byte.
bool borlandPackAlignCodeGroup(BorlandPackBitReader *pReader, qint32 nCodeBits,
                               qint64 *pnGroupStartBits)
{
    if (!pReader || !pnGroupStartBits ||
        (nCodeBits < BORLANDPACK_LZW_MINBITS) ||
        (nCodeBits > BORLANDPACK_LZW_MAXBITS) ||
        (pReader->bitsRead() < *pnGroupStartBits)) {
        return false;
    }
    const qint64 nGroupBits = static_cast<qint64>(nCodeBits) * 8;
    const qint64 nUsedBits = pReader->bitsRead() - *pnGroupStartBits;
    const qint64 nSkipBits =
        (nGroupBits - (nUsedBits % nGroupBits)) % nGroupBits;
    if (!pReader->skipBits(nSkipBits)) return false;
    *pnGroupStartBits = pReader->bitsRead();
    return true;
}
}  // namespace

bool XBorlandPackDecoder::decode(const QByteArray &packed,
                                 qint64 nUncompressedSize, QByteArray *pOutput,
                                 XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput || (nUncompressedSize < 0) ||
        (nUncompressedSize > (std::numeric_limits<qint32>::max)()) ||
        (packed.size() < 1) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const quint8 nFlags = static_cast<quint8>(packed.at(0));
    if (nFlags & BORLANDPACK_LZW_FLAGS_RESERVED) return false;

    const qint32 nMaxBits =
        static_cast<qint32>(nFlags & BORLANDPACK_LZW_FLAGS_MAXBITS);
    const bool bBlockCompress =
        (nFlags & BORLANDPACK_LZW_FLAGS_BLOCKMODE) != 0;
    if ((nMaxBits < BORLANDPACK_LZW_MINBITS) ||
        (nMaxBits > BORLANDPACK_LZW_MAXBITS)) {
        return false;
    }

    const qint32 nMaxCode = 1 << nMaxBits;

    QVector<quint16> vectPrefix(nMaxCode, 0);
    QByteArray baSuffix(nMaxCode, 0);
    QByteArray baStack(nMaxCode, 0);
    quint16 *pPrefix = vectPrefix.data();
    quint8 *pSuffix = reinterpret_cast<quint8 *>(baSuffix.data());
    quint8 *pStack = reinterpret_cast<quint8 *>(baStack.data());
    for (qint32 i = 0; i < 256; ++i) pSuffix[i] = static_cast<quint8>(i);

    QByteArray baOutput;
    baOutput.reserve(static_cast<qint32>(nUncompressedSize));

    BorlandPackBitReader reader(packed.constData() + 1, packed.size() - 1);
    qint64 nGroupStartBits = 0;
    qint32 nNextCode = bBlockCompress ? BORLANDPACK_LZW_FIRST : 256;
    qint32 nCodeBits = BORLANDPACK_LZW_MINBITS;
    qint32 nMaxVal = 1 << nCodeBits;

    // The very first code addresses the pristine dictionary, so it can only be
    // a literal.  Accepting a forward code here would read uninitialised
    // prefix/suffix entries on malformed input.
    qint32 nOldCode = reader.readCode(nCodeBits);
    if ((nOldCode < 0) || (nOldCode >= 256)) {
        if ((nOldCode < 0) && (nUncompressedSize == 0)) {
            *pOutput = QByteArray();
            return true;
        }
        return false;
    }
    quint8 nFinChar = static_cast<quint8>(nOldCode);
    if (baOutput.size() >= nUncompressedSize) return false;
    baOutput.append(static_cast<char>(nFinChar));

    bool bResult = true;
    while (bResult) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            bResult = false;
            break;
        }

        qint32 nCode = reader.readCode(nCodeBits);
        if (nCode < 0) break;  // End of stream.

        if (bBlockCompress && (nCode == BORLANDPACK_LZW_CLEAR)) {
            do {
                if (!borlandPackAlignCodeGroup(&reader, nCodeBits,
                                               &nGroupStartBits)) {
                    bResult = false;
                    break;
                }
                nNextCode = BORLANDPACK_LZW_FIRST;
                nCodeBits = BORLANDPACK_LZW_MINBITS;
                nMaxVal = 1 << nCodeBits;
                nCode = reader.readCode(nCodeBits);
            } while (nCode == BORLANDPACK_LZW_CLEAR);

            if (!bResult || (nCode < 0) || (nCode >= 256)) {
                bResult = false;
                break;
            }
            nOldCode = nCode;
            nFinChar = static_cast<quint8>(nCode);
            if (baOutput.size() >= nUncompressedSize) {
                bResult = false;
                break;
            }
            baOutput.append(static_cast<char>(nFinChar));
            continue;
        }

        const qint32 nInCode = nCode;
        qint32 nStackTop = 0;

        // KwKwK: the encoder may reference the entry it is about to create.
        if (nCode >= nNextCode) {
            if ((nCode > nNextCode) || (nCode >= nMaxCode)) {
                bResult = false;
                break;
            }
            pStack[nStackTop++] = nFinChar;
            nCode = nOldCode;
        }

        while (nCode >= 256) {
            if ((nCode >= nNextCode) || (nCode >= nMaxCode) ||
                (nStackTop >= nMaxCode)) {
                bResult = false;
                break;
            }
            pStack[nStackTop++] = pSuffix[nCode];
            nCode = pPrefix[nCode];
        }
        if (!bResult) break;

        if ((nCode < 0) || (nCode >= 256) || (nStackTop >= nMaxCode)) {
            bResult = false;
            break;
        }
        nFinChar = pSuffix[nCode];
        pStack[nStackTop++] = nFinChar;

        if (baOutput.size() > nUncompressedSize - nStackTop) {
            bResult = false;
            break;
        }
        for (qint32 i = nStackTop - 1; i >= 0; --i) {
            baOutput.append(static_cast<char>(pStack[i]));
        }

        if (nNextCode < nMaxCode) {
            pPrefix[nNextCode] = static_cast<quint16>(nOldCode);
            pSuffix[nNextCode] = nFinChar;
            ++nNextCode;
            if ((nNextCode >= nMaxVal) && (nCodeBits < nMaxBits)) {
                if (!borlandPackAlignCodeGroup(&reader, nCodeBits,
                                               &nGroupStartBits)) {
                    bResult = false;
                    break;
                }
                ++nCodeBits;
                nMaxVal = 1 << nCodeBits;
            }
        }

        nOldCode = nInCode;
    }

    if (!bResult || (baOutput.size() != nUncompressedSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    *pOutput = baOutput;
    return true;
}
