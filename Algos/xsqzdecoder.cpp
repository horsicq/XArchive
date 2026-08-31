/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xsqzdecoder.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace {
const qint32 SQZ_INPUT_BUFFER_SIZE = 0x2000;
const qint32 SQZ_OUTPUT_BUFFER_SIZE = 0x4000;
const qint32 SQZ_WINDOW_SIZE = 0x8000;
const qint32 SQZ_WINDOW_MASK = SQZ_WINDOW_SIZE - 1;
const qint32 SQZ_NT = 19;
const qint32 SQZ_NC = 0x1ff;
const qint32 SQZ_NP = 0x1f;
const qint32 SQZ_MAX_BITS = 16;
const qint32 SQZ_MAX_PADDING_BYTES = 2;

// SQZ methods 3/4 map C symbols 0x100..0x11f through these tables.
const quint8 SQZ_LENGTH_EXTRA[32] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6};
const quint16 SQZ_LENGTH_BASE[32] = {
    0,   1,   2,   3,   4,   5,   6,   7,   8,   10,  12,  14,  16,  20,  24,  28,
    32,  40,  48,  56,  64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 384, 448};

// Methods 2/4 use the distance tables embedded in SQZ.EXE.  Index 31 is not
// part of the 31-symbol P alphabet and is intentionally omitted.
const quint8 SQZ_DISTANCE_EXTRA[31] = {
    0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6,
    6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
const quint16 SQZ_DISTANCE_BASE[31] = {
    0,   1,   2,   3,   4,    5,    7,    9,    13,   17,   25,   33,   49,   65,   97,   129,
    193, 257, 385, 513, 769,  1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

struct SQZ_STATE;

bool sqzReadBits(SQZ_STATE *pState, qint32 nBits, quint32 *pValue);

struct SQZ_HUFFMAN {
    bool bConstant = false;
    quint16 nConstant = 0;
    qint32 nSymbolCount = 0;
    std::array<quint16, SQZ_MAX_BITS + 1> anCounts = {};
    std::array<quint32, SQZ_MAX_BITS + 1> anFirstCode = {};
    std::array<quint16, SQZ_MAX_BITS + 1> anFirstSymbol = {};
    std::array<quint16, SQZ_NC> anSymbols = {};

    bool setConstant(quint32 nSymbol, qint32 nCount)
    {
        if ((nCount <= 0) || (nCount > SQZ_NC) || (nSymbol >= static_cast<quint32>(nCount))) return false;
        bConstant = true;
        nConstant = static_cast<quint16>(nSymbol);
        nSymbolCount = nCount;
        return true;
    }

    bool build(const quint8 *pLengths, qint32 nCount)
    {
        if (!pLengths || (nCount <= 0) || (nCount > SQZ_NC)) return false;

        bConstant = false;
        nConstant = 0;
        nSymbolCount = 0;
        anCounts.fill(0);
        anFirstCode.fill(0);
        anFirstSymbol.fill(0);
        anSymbols.fill(0);

        for (qint32 i = 0; i < nCount; ++i) {
            const qint32 nLength = pLengths[i];
            if ((nLength < 0) || (nLength > SQZ_MAX_BITS)) return false;
            if (nLength != 0) ++anCounts[nLength];
        }

        // SQZ.EXE's table builder requires the 16-bit canonical code space to
        // be exactly filled.  Constant alphabets use the separate n==0 form.
        qint32 nCodesLeft = 1;
        for (qint32 nLength = 1; nLength <= SQZ_MAX_BITS; ++nLength) {
            nCodesLeft = (nCodesLeft << 1) - anCounts[nLength];
            if (nCodesLeft < 0) return false;
        }
        if (nCodesLeft != 0) return false;

        quint32 nCode = 0;
        quint32 nSymbolIndex = 0;
        for (qint32 nLength = 1; nLength <= SQZ_MAX_BITS; ++nLength) {
            nCode = (nCode + anCounts[nLength - 1]) << 1;
            anFirstCode[nLength] = nCode;
            anFirstSymbol[nLength] = static_cast<quint16>(nSymbolIndex);

            for (qint32 nSymbol = 0; nSymbol < nCount; ++nSymbol) {
                if (pLengths[nSymbol] == nLength) {
                    if (nSymbolIndex >= static_cast<quint32>(nCount)) return false;
                    anSymbols[nSymbolIndex++] = static_cast<quint16>(nSymbol);
                }
            }
        }

        nSymbolCount = nCount;
        return (nSymbolIndex > 0) && (nSymbolIndex <= static_cast<quint32>(nCount));
    }

    bool decode(SQZ_STATE *pState, quint32 *pSymbol) const
    {
        if (!pState || !pSymbol || (nSymbolCount <= 0)) return false;
        if (bConstant) {
            *pSymbol = nConstant;
            return nConstant < nSymbolCount;
        }

        quint32 nCode = 0;
        for (qint32 nLength = 1; nLength <= SQZ_MAX_BITS; ++nLength) {
            quint32 nBit = 0;
            if (!sqzReadBits(pState, 1, &nBit)) return false;
            nCode = (nCode << 1) | nBit;

            const quint32 nFirst = anFirstCode[nLength];
            const quint32 nCount = anCounts[nLength];
            if ((nCount != 0) && (nCode >= nFirst) && ((nCode - nFirst) < nCount)) {
                const quint32 nIndex = anFirstSymbol[nLength] + (nCode - nFirst);
                if ((nIndex >= anSymbols.size()) || (anSymbols[nIndex] >= nSymbolCount)) return false;
                *pSymbol = anSymbols[nIndex];
                return true;
            }
        }

        return false;
    }
};

struct SQZ_STATE {
    XBinary::DATAPROCESS_STATE *pProcessState = nullptr;
    XBinary::PDSTRUCT *pPdStruct = nullptr;
    qint32 nMethod = 0;
    quint32 nRawSize = 0;
    quint32 nProduced = 0;
    quint32 nBlockRemaining = 0;
    quint32 nCompressedLeft = 0;
    quint32 nBitBuffer = 0;
    qint32 nBitCount = 0;
    qint32 nPaddingBytes = 0;
    qint32 nInputPosition = 0;
    qint32 nInputSize = 0;
    qint32 nOutputUsed = 0;
    qint32 nWindowPosition = 0;
    bool bError = false;
    std::array<char, SQZ_INPUT_BUFFER_SIZE> abInput = {};
    std::array<char, SQZ_OUTPUT_BUFFER_SIZE> abOutput = {};
    std::array<quint8, SQZ_WINDOW_SIZE> abWindow = {};
    std::array<quint8, SQZ_NC> anCodeLengths = {};
    SQZ_HUFFMAN cTree;
    SQZ_HUFFMAN pTree;
};

bool sqzPrepareProcessState(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct, quint32 *pCompressedSize, quint32 *pRawSize)
{
    if (!pState || !pCompressedSize || !pRawSize || !pState->pDeviceInput || !pState->pDeviceOutput || (pState->nInputOffset < 0) ||
        (pState->nInputLimit < 0)) {
        return false;
    }

    const qint64 nMax32 = (std::numeric_limits<quint32>::max)();
    if (pState->nInputLimit > nMax32) return false;

    bool bRawSizeOK = false;
    const qint64 nRawSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bRawSizeOK);
    if (!pState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE) || !bRawSizeOK || (nRawSize < 0) || (nRawSize > nMax32) ||
        !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nRawSize)) {
        return false;
    }

    const qint64 nMax64 = (std::numeric_limits<qint64>::max)();
    if ((pState->nProcessedOffset < 0) || (pState->nProcessedLimit < -1) ||
        ((pState->nProcessedLimit != -1) && (pState->nProcessedOffset > (nMax64 - pState->nProcessedLimit)))) {
        return false;
    }

    pState->bReadError = false;
    pState->bWriteError = false;
    pState->nCountInput = 0;
    pState->nCountOutput = 0;

    if (!pState->pDeviceInput->isReadable() || !pState->pDeviceInput->seek(pState->nInputOffset) ||
        (pState->pDeviceInput->pos() != pState->nInputOffset)) {
        pState->bReadError = true;
    }
    if (!pState->pDeviceOutput->isWritable() || (!pState->pDeviceOutput->isSequential() && !pState->pDeviceOutput->seek(0)) ||
        (pState->pDeviceOutput->pos() != 0)) {
        pState->bWriteError = true;
    }
    if (pState->bReadError || pState->bWriteError || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    *pCompressedSize = static_cast<quint32>(pState->nInputLimit);
    *pRawSize = static_cast<quint32>(nRawSize);
    return true;
}

bool sqzRefillInput(SQZ_STATE *pState)
{
    if (!pState || pState->bError || !pState->pProcessState || (pState->nCompressedLeft == 0) ||
        !XBinary::isPdStructNotCanceled(pState->pPdStruct)) {
        return false;
    }

    const qint32 nRequest = static_cast<qint32>((std::min)(static_cast<quint32>(SQZ_INPUT_BUFFER_SIZE), pState->nCompressedLeft));
    const qint32 nRead = XBinary::_readDevice(pState->abInput.data(), nRequest, pState->pProcessState);
    if ((nRead <= 0) || (nRead > nRequest)) {
        pState->bError = true;
        pState->pProcessState->bReadError = true;
        return false;
    }

    pState->nInputPosition = 0;
    pState->nInputSize = nRead;
    return true;
}

bool sqzReadByte(SQZ_STATE *pState, quint8 *pValue)
{
    if (!pState || !pValue || pState->bError || !XBinary::isPdStructNotCanceled(pState->pPdStruct)) return false;

    if (pState->nCompressedLeft == 0) {
        if (++pState->nPaddingBytes > SQZ_MAX_PADDING_BYTES) {
            pState->bError = true;
            return false;
        }
        *pValue = 0;
        return true;
    }

    if ((pState->nInputPosition >= pState->nInputSize) && !sqzRefillInput(pState)) return false;
    if ((pState->nInputPosition < 0) || (pState->nInputPosition >= pState->nInputSize)) {
        pState->bError = true;
        return false;
    }

    *pValue = static_cast<quint8>(pState->abInput[pState->nInputPosition++]);
    --pState->nCompressedLeft;
    return true;
}

bool sqzEnsureBits(SQZ_STATE *pState, qint32 nBits)
{
    if (!pState || pState->bError || (nBits < 0) || (nBits > SQZ_MAX_BITS)) return false;
    while (pState->nBitCount < nBits) {
        quint8 nByte = 0;
        if (!sqzReadByte(pState, &nByte)) return false;
        pState->nBitBuffer = (pState->nBitBuffer << 8) | nByte;
        pState->nBitCount += 8;
    }
    return true;
}

bool sqzPeekBits(SQZ_STATE *pState, qint32 nBits, quint32 *pValue)
{
    if (!pValue || !sqzEnsureBits(pState, nBits)) return false;
    if (nBits == 0) {
        *pValue = 0;
        return true;
    }
    *pValue = (pState->nBitBuffer >> (pState->nBitCount - nBits)) & ((1U << nBits) - 1U);
    return true;
}

bool sqzReadBits(SQZ_STATE *pState, qint32 nBits, quint32 *pValue)
{
    if (!pValue || !sqzPeekBits(pState, nBits, pValue)) return false;
    pState->nBitCount -= nBits;
    if (pState->nBitCount == 0) {
        pState->nBitBuffer = 0;
    } else {
        pState->nBitBuffer &= (1U << pState->nBitCount) - 1U;
    }
    return true;
}

bool sqzReadPtLengths(SQZ_STATE *pState, qint32 nSymbols, qint32 nBitWidth, qint32 nSpecial, SQZ_HUFFMAN *pTree)
{
    if (!pState || !pTree || (nSymbols <= 0) || (nSymbols > SQZ_NC)) return false;
    pState->anCodeLengths.fill(0);

    quint32 nEncoded = 0;
    if (!sqzReadBits(pState, nBitWidth, &nEncoded)) return false;
    if (nEncoded == 0) {
        quint32 nConstant = 0;
        return sqzReadBits(pState, nBitWidth, &nConstant) && pTree->setConstant(nConstant, nSymbols);
    }
    if (nEncoded > static_cast<quint32>(nSymbols)) return false;

    qint32 i = 0;
    while (i < static_cast<qint32>(nEncoded)) {
        quint32 nLookAhead = 0;
        if (!sqzPeekBits(pState, 16, &nLookAhead)) return false;
        qint32 nLength = static_cast<qint32>(nLookAhead >> 13);
        if (nLength == 7) {
            quint32 nMask = 0x1000;
            while ((nMask != 0) && ((nLookAhead & nMask) != 0)) {
                nMask >>= 1;
                ++nLength;
            }
        }
        if (nLength > SQZ_MAX_BITS) return false;

        quint32 nDiscard = 0;
        const qint32 nConsumed = (nLength < 7) ? 3 : (nLength - 3);
        if (!sqzReadBits(pState, nConsumed, &nDiscard)) return false;
        pState->anCodeLengths[i++] = static_cast<quint8>(nLength);

        if (i == nSpecial) {
            quint32 nZeros = 0;
            if (!sqzReadBits(pState, 2, &nZeros) || (nZeros > (nEncoded - static_cast<quint32>(i)))) return false;
            for (quint32 j = 0; j < nZeros; ++j) pState->anCodeLengths[i++] = 0;
        }
    }

    return pTree->build(pState->anCodeLengths.data(), nSymbols);
}

bool sqzReadCLengths(SQZ_STATE *pState, const SQZ_HUFFMAN &ptTree, SQZ_HUFFMAN *pTree)
{
    if (!pState || !pTree) return false;
    pState->anCodeLengths.fill(0);

    quint32 nEncoded = 0;
    if (!sqzReadBits(pState, 9, &nEncoded)) return false;
    if (nEncoded == 0) {
        quint32 nConstant = 0;
        return sqzReadBits(pState, 9, &nConstant) && pTree->setConstant(nConstant, SQZ_NC);
    }
    if (nEncoded > SQZ_NC) return false;

    qint32 i = 0;
    while (i < static_cast<qint32>(nEncoded)) {
        quint32 nSymbol = 0;
        if (!ptTree.decode(pState, &nSymbol) || (nSymbol >= SQZ_NT)) return false;

        if (nSymbol <= 2) {
            quint32 nRun = 1;
            if (nSymbol == 1) {
                if (!sqzReadBits(pState, 4, &nRun)) return false;
                nRun += 3;
            } else if (nSymbol == 2) {
                nRun = 20;
                quint32 nPart = 0;
                do {
                    if (!sqzReadBits(pState, 7, &nPart)) return false;
                    nRun += nPart;
                    if (nRun > (nEncoded - static_cast<quint32>(i))) return false;
                } while (nPart == 0x7f);
            }
            if (nRun > (nEncoded - static_cast<quint32>(i))) return false;
            i += static_cast<qint32>(nRun);
        } else {
            const quint32 nLength = nSymbol - 2;
            if ((nLength == 0) || (nLength > SQZ_MAX_BITS)) return false;
            pState->anCodeLengths[i++] = static_cast<quint8>(nLength);
        }
    }

    return pTree->build(pState->anCodeLengths.data(), SQZ_NC);
}

bool sqzReadBlock(SQZ_STATE *pState)
{
    quint32 nBlockSize = 0;
    if (!sqzReadBits(pState, 14, &nBlockSize) || (nBlockSize == 0)) return false;

    SQZ_HUFFMAN ptTree;
    SQZ_HUFFMAN cTree;
    SQZ_HUFFMAN pTree;
    if (!sqzReadPtLengths(pState, SQZ_NT, 5, 3, &ptTree) || !sqzReadCLengths(pState, ptTree, &cTree) ||
        !sqzReadPtLengths(pState, SQZ_NP, 5, -1, &pTree)) {
        return false;
    }

    pState->nBlockRemaining = nBlockSize;
    pState->cTree = cTree;
    pState->pTree = pTree;
    return true;
}

bool sqzDecodeC(SQZ_STATE *pState, quint32 *pValue)
{
    if (!pState || !pValue) return false;
    if ((pState->nBlockRemaining == 0) && !sqzReadBlock(pState)) return false;

    quint32 nSymbol = 0;
    if (!pState->cTree.decode(pState, &nSymbol) || (nSymbol >= SQZ_NC)) return false;
    --pState->nBlockRemaining;

    if (nSymbol <= 0xff) {
        *pValue = nSymbol;
        return true;
    }

    if (pState->nMethod < 3) {
        if (nSymbol < 0x1c0) {
            *pValue = nSymbol;
            return true;
        }
        quint32 nExtra = 0;
        if (!sqzReadBits(pState, 1, &nExtra)) return false;
        *pValue = 0x1c0 + ((nSymbol - 0x1c0) << 1) + nExtra;
        return true;
    }

    if ((nSymbol < 0x100) || (nSymbol > 0x11f)) return false;
    const qint32 nIndex = static_cast<qint32>(nSymbol - 0x100);
    if (SQZ_LENGTH_EXTRA[nIndex] == 0) {
        *pValue = nSymbol;
        return true;
    }

    quint32 nExtra = 0;
    if (!sqzReadBits(pState, SQZ_LENGTH_EXTRA[nIndex], &nExtra)) return false;
    *pValue = 0x100U + SQZ_LENGTH_BASE[nIndex] + nExtra;
    return true;
}

bool sqzDecodeDistance(SQZ_STATE *pState, quint32 *pDistance)
{
    if (!pState || !pDistance) return false;
    quint32 nSymbol = 0;
    if (!pState->pTree.decode(pState, &nSymbol) || (nSymbol >= SQZ_NP)) return false;

    quint32 nDistance = 0;
    if ((pState->nMethod == 1) || (pState->nMethod == 3)) {
        if (nSymbol < 2) {
            nDistance = nSymbol;
        } else {
            // The compact table has 16 entries (symbols 0..15).
            if (nSymbol > 15) return false;
            const qint32 nExtraBits = static_cast<qint32>(nSymbol - 1);
            quint32 nExtra = 0;
            if (!sqzReadBits(pState, nExtraBits, &nExtra)) return false;
            nDistance = (1U << nExtraBits) + nExtra;
        }
    } else {
        const qint32 nIndex = static_cast<qint32>(nSymbol);
        quint32 nExtra = 0;
        if (!sqzReadBits(pState, SQZ_DISTANCE_EXTRA[nIndex], &nExtra)) return false;
        nDistance = SQZ_DISTANCE_BASE[nIndex] + nExtra;
    }

    if (nDistance >= SQZ_WINDOW_SIZE) return false;
    *pDistance = nDistance;
    return true;
}

bool sqzFlushOutput(SQZ_STATE *pState)
{
    if (!pState || pState->bError || (pState->nOutputUsed < 0) || (pState->nOutputUsed > SQZ_OUTPUT_BUFFER_SIZE)) return false;
    if (pState->nOutputUsed == 0) return true;
    if (!XBinary::isPdStructNotCanceled(pState->pPdStruct) ||
        (XBinary::_writeDevice(pState->abOutput.data(), pState->nOutputUsed, pState->pProcessState) != pState->nOutputUsed)) {
        pState->bError = true;
        return false;
    }
    pState->nOutputUsed = 0;
    return true;
}

bool sqzEmitByte(SQZ_STATE *pState, quint8 nValue)
{
    if (!pState || pState->bError || (pState->nProduced >= pState->nRawSize)) return false;
    pState->abWindow[pState->nWindowPosition] = nValue;
    pState->nWindowPosition = (pState->nWindowPosition + 1) & SQZ_WINDOW_MASK;
    pState->abOutput[pState->nOutputUsed++] = static_cast<char>(nValue);
    ++pState->nProduced;
    return (pState->nOutputUsed < SQZ_OUTPUT_BUFFER_SIZE) || sqzFlushOutput(pState);
}

bool sqzFinalizeInput(SQZ_STATE *pState)
{
    if (!pState || pState->bError || (pState->nCompressedLeft > SQZ_MAX_PADDING_BYTES)) return false;
    while (pState->nCompressedLeft != 0) {
        quint8 nIgnored = 0;
        if (!sqzReadByte(pState, &nIgnored)) return false;
    }
    return (pState->nInputPosition == pState->nInputSize) &&
           (pState->pProcessState->nCountInput == pState->pProcessState->nInputLimit);
}
}  // namespace

bool XSQZDecoder::decompress(XBinary::DATAPROCESS_STATE *pProcessState, qint32 nMethod, XBinary::PDSTRUCT *pPdStruct)
{
    if ((nMethod < 1) || (nMethod > 4)) return false;

    quint32 nCompressedSize = 0;
    quint32 nRawSize = 0;
    if (!sqzPrepareProcessState(pProcessState, pPdStruct, &nCompressedSize, &nRawSize)) return false;
    if (nRawSize == 0) {
        return (nCompressedSize == 0) && !pProcessState->bReadError && !pProcessState->bWriteError &&
               XBinary::isPdStructNotCanceled(pPdStruct);
    }
    if (nCompressedSize == 0) return false;

    std::unique_ptr<SQZ_STATE> pState(new (std::nothrow) SQZ_STATE);
    if (!pState) return false;
    pState->pProcessState = pProcessState;
    pState->pPdStruct = pPdStruct;
    pState->nMethod = nMethod;
    pState->nRawSize = nRawSize;
    pState->nCompressedLeft = nCompressedSize;
    pState->abWindow.fill(0);
    std::fill(pState->abWindow.begin() + 0x7fc0, pState->abWindow.end(), static_cast<quint8>(0x20));

    while ((pState->nProduced < nRawSize) && !pState->bError && XBinary::isPdStructNotCanceled(pPdStruct)) {
        quint32 nCode = 0;
        if (!sqzDecodeC(pState.get(), &nCode)) {
            pState->bError = true;
            break;
        }

        if (nCode <= 0xff) {
            if (!sqzEmitByte(pState.get(), static_cast<quint8>(nCode))) pState->bError = true;
            continue;
        }

        if (nCode <= 0xfd) {
            pState->bError = true;
            break;
        }
        const quint32 nLength = nCode - 0xfd;
        if ((nLength < 3) || (nLength > 514)) {
            pState->bError = true;
            break;
        }

        quint32 nDistance = 0;
        if (!sqzDecodeDistance(pState.get(), &nDistance)) {
            pState->bError = true;
            break;
        }
        qint32 nSource = (pState->nWindowPosition - static_cast<qint32>(nDistance) - 1) & SQZ_WINDOW_MASK;
        // SQZ members are terminated by their declared raw size.  The final
        // match is allowed to cross that boundary; the original extractor
        // copies only the remaining bytes.
        const quint32 nCopyLength = (std::min)(nLength, nRawSize - pState->nProduced);
        for (quint32 i = 0; i < nCopyLength; ++i) {
            const quint8 nValue = pState->abWindow[nSource];
            nSource = (nSource + 1) & SQZ_WINDOW_MASK;
            if (!sqzEmitByte(pState.get(), nValue)) {
                pState->bError = true;
                break;
            }
        }
    }

    // A block's token count is an upper decoding bound, not an end marker for
    // the member.  SQZ.EXE stops as soon as the declared output size has been
    // produced, and valid archives can therefore leave tokens in the final
    // block.  The packed extent and output size are still enforced exactly.
    const bool bCodecExact = !pState->bError && (pState->nProduced == nRawSize) && sqzFinalizeInput(pState.get()) && sqzFlushOutput(pState.get());
    return bCodecExact && !pState->bError && !pProcessState->bReadError && !pProcessState->bWriteError &&
           (pProcessState->nCountOutput == nRawSize) && XBinary::isPdStructNotCanceled(pPdStruct);
}
