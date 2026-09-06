/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 *
 * Headerless 12-bit block-mode Unix-compress (LZW) decoder for the
 * InstallShield-family *.??$ container handled by XIS11.
 *
 * The algorithm is the same one Algos/xborlandpackdecoder.cpp implements, but
 * that decoder consumes a flags byte at offset 0 and needs the uncompressed
 * size up front.  This stream carries neither: the parameters are implicit and
 * decoding simply runs until the member's bounded input is exhausted.
 */
#include "xis11decoder.h"

#include <limits>

namespace {
const qint32 IS11_LZW_CLEAR = 256;
const qint32 IS11_LZW_FIRST = 257;
const qint32 IS11_LZW_MINBITS = 9;
const qint32 IS11_LZW_MAXBITS = 12;
const qint32 IS11_LZW_MAXCODE = 1 << IS11_LZW_MAXBITS;
// A 12-bit dictionary cannot legitimately blow a member up by more than a few
// hundred times; 64 MiB is far beyond anything the corpus produces and keeps a
// corrupt stream from exhausting memory.
const qint64 IS11_DEFAULT_OUTPUT_LIMIT = 64 * 1024 * 1024;

// LSB-first bit reader over the whole packed member.
class Is11BitReader {
public:
    Is11BitReader(const char *pData, qint64 nSize) : m_pData(pData), m_nTotalBits(nSize * 8), m_nBitPosition(0)
    {
    }

    qint32 readCode(qint32 nCodeBits)
    {
        if ((nCodeBits < 1) || (nCodeBits > IS11_LZW_MAXBITS) || (m_nBitPosition > m_nTotalBits - nCodeBits)) {
            return -1;
        }
        qint32 nCode = 0;
        for (qint32 i = 0; i < nCodeBits; ++i) {
            const qint64 nBit = m_nBitPosition + i;
            const quint8 nByte = static_cast<quint8>(m_pData[static_cast<qint64>(nBit >> 3)]);
            nCode |= static_cast<qint32>((nByte >> (nBit & 7)) & 1U) << i;
        }
        m_nBitPosition += nCodeBits;
        return nCode;
    }

    bool skipBits(qint64 nCount)
    {
        if ((nCount < 0) || (m_nBitPosition > m_nTotalBits - nCount)) return false;
        m_nBitPosition += nCount;
        return true;
    }

    qint64 bitsRead() const
    {
        return m_nBitPosition;
    }

private:
    const char *m_pData;
    qint64 m_nTotalBits;
    qint64 m_nBitPosition;
};

// compress emits codes in groups of eight.  A width change or a CLEAR abandons
// the rest of the current group, so the next code starts on the following group
// boundary.  The group origin is bit 0 of the member.
bool is11AlignCodeGroup(Is11BitReader *pReader, qint32 nCodeBits, qint64 *pnGroupStartBits)
{
    if (!pReader || !pnGroupStartBits || (nCodeBits < IS11_LZW_MINBITS) || (nCodeBits > IS11_LZW_MAXBITS) || (pReader->bitsRead() < *pnGroupStartBits)) {
        return false;
    }
    const qint64 nGroupBits = static_cast<qint64>(nCodeBits) * 8;
    const qint64 nUsedBits = pReader->bitsRead() - *pnGroupStartBits;
    const qint64 nSkipBits = (nGroupBits - (nUsedBits % nGroupBits)) % nGroupBits;
    if (!pReader->skipBits(nSkipBits)) return false;
    *pnGroupStartBits = pReader->bitsRead();
    return true;
}
}  // namespace

bool XIS11Decoder::decodeStream(const QByteArray &packed, QByteArray *pOutput, qint64 nMaxOutput, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (nMaxOutput < 0) nMaxOutput = IS11_DEFAULT_OUTPUT_LIMIT;
    if (nMaxOutput > static_cast<qint64>((std::numeric_limits<qint32>::max)())) {
        nMaxOutput = static_cast<qint64>((std::numeric_limits<qint32>::max)());
    }
    if (packed.isEmpty()) return false;

    QVector<quint16> vectPrefix(IS11_LZW_MAXCODE, 0);
    QByteArray baSuffix(IS11_LZW_MAXCODE, 0);
    QByteArray baStack(IS11_LZW_MAXCODE, 0);
    quint16 *pPrefix = vectPrefix.data();
    quint8 *pSuffix = reinterpret_cast<quint8 *>(baSuffix.data());
    quint8 *pStack = reinterpret_cast<quint8 *>(baStack.data());
    for (qint32 i = 0; i < 256; ++i) pSuffix[i] = static_cast<quint8>(i);

    QByteArray baOutput;

    Is11BitReader reader(packed.constData(), packed.size());
    qint64 nGroupStartBits = 0;
    qint32 nNextCode = IS11_LZW_FIRST;
    qint32 nCodeBits = IS11_LZW_MINBITS;
    qint32 nMaxVal = 1 << nCodeBits;

    // The first code addresses the pristine dictionary, so only a literal is
    // meaningful there.
    qint32 nOldCode = reader.readCode(nCodeBits);
    if ((nOldCode < 0) || (nOldCode >= 256)) return false;
    quint8 nFinChar = static_cast<quint8>(nOldCode);
    baOutput.append(static_cast<char>(nFinChar));

    bool bResult = true;
    while (bResult) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            bResult = false;
            break;
        }

        qint32 nCode = reader.readCode(nCodeBits);
        if (nCode < 0) break;  // End of the bounded member.

        if (nCode == IS11_LZW_CLEAR) {
            if (!is11AlignCodeGroup(&reader, nCodeBits, &nGroupStartBits)) {
                bResult = false;
                break;
            }
            nNextCode = IS11_LZW_FIRST;
            nCodeBits = IS11_LZW_MINBITS;
            nMaxVal = 1 << nCodeBits;
            nCode = reader.readCode(nCodeBits);
            if (nCode < 0) break;
            // A CLEAR immediately after a CLEAR terminates the stream, exactly
            // as in the reference extractor.
            if (nCode == IS11_LZW_CLEAR) break;
            if (nCode >= 256) {
                bResult = false;
                break;
            }
            nOldCode = nCode;
            nFinChar = static_cast<quint8>(nCode);
            if (baOutput.size() >= nMaxOutput) {
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
            if ((nCode > nNextCode) || (nCode >= IS11_LZW_MAXCODE)) {
                bResult = false;
                break;
            }
            pStack[nStackTop++] = nFinChar;
            nCode = nOldCode;
        }

        while (nCode >= 256) {
            if ((nCode >= nNextCode) || (nCode >= IS11_LZW_MAXCODE) || (nStackTop >= IS11_LZW_MAXCODE)) {
                bResult = false;
                break;
            }
            pStack[nStackTop++] = pSuffix[nCode];
            nCode = pPrefix[nCode];
        }
        if (!bResult) break;

        if ((nCode < 0) || (nCode >= 256) || (nStackTop >= IS11_LZW_MAXCODE)) {
            bResult = false;
            break;
        }
        nFinChar = pSuffix[nCode];
        pStack[nStackTop++] = nFinChar;

        if (baOutput.size() > nMaxOutput - nStackTop) {
            bResult = false;
            break;
        }
        for (qint32 i = nStackTop - 1; i >= 0; --i) {
            baOutput.append(static_cast<char>(pStack[i]));
        }

        if (nNextCode < IS11_LZW_MAXCODE) {
            pPrefix[nNextCode] = static_cast<quint16>(nOldCode);
            pSuffix[nNextCode] = nFinChar;
            ++nNextCode;
            if ((nNextCode >= nMaxVal) && (nCodeBits < IS11_LZW_MAXBITS)) {
                if (!is11AlignCodeGroup(&reader, nCodeBits, &nGroupStartBits)) {
                    bResult = false;
                    break;
                }
                ++nCodeBits;
                nMaxVal = 1 << nCodeBits;
            }
        }

        nOldCode = nInCode;
    }

    if (!bResult || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    *pOutput = baOutput;
    return true;
}

bool XIS11Decoder::decode(const QByteArray &packed, qint64 nUncompressedSize, QByteArray *pOutput, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput || (nUncompressedSize < 0) || (nUncompressedSize > static_cast<qint64>((std::numeric_limits<qint32>::max)()))) {
        return false;
    }
    QByteArray baOutput;
    if (!decodeStream(packed, &baOutput, nUncompressedSize, pPdStruct)) return false;
    if (baOutput.size() != nUncompressedSize) return false;
    *pOutput = baOutput;
    return true;
}
