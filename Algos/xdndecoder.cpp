/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xdndecoder.h"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <new>

namespace {
const qint32 DN_INPUT_BUFFER_SIZE = 0x4000;
const qint32 DN_OUTPUT_BUFFER_SIZE = 0x4000;
const qint32 DN_WINDOW_SIZE = 0x8000;
const qint32 DN_WINDOW_MASK = DN_WINDOW_SIZE - 1;
const qint32 DN_MAX_BITS = 15;
const qint32 DN_MAX_HUFFMAN_SYMBOLS = 288;
const qint32 DN_MAX_DYNAMIC_LENGTHS = 286 + 30;

const std::array<quint16, 29> DN_LENGTH_BASE = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27,
    31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
const std::array<quint8, 29> DN_LENGTH_EXTRA = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
    2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
const std::array<quint16, 30> DN_DISTANCE_BASE = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129,
    193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097,
    6145, 8193, 12289, 16385, 24577};
const std::array<quint8, 30> DN_DISTANCE_EXTRA = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6,
    6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
const std::array<quint8, 19> DN_CODE_LENGTH_ORDER = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

struct DN_STATE;

bool dnReadBits(DN_STATE *pState, qint32 nCount, quint32 *pValue);

struct DN_HUFFMAN {
    qint32 nMaxBits = 0;
    qint32 nSymbolCount = 0;
    std::array<quint16, DN_MAX_BITS + 1> anCounts = {};
    std::array<quint16, DN_MAX_HUFFMAN_SYMBOLS> anSymbols = {};

    bool build(const quint8 *pLengths, qint32 nCount, bool bAllowEmpty)
    {
        if (!pLengths || (nCount <= 0) || (nCount > DN_MAX_HUFFMAN_SYMBOLS)) return false;

        nMaxBits = 0;
        nSymbolCount = 0;
        anCounts.fill(0);
        anSymbols.fill(0);

        for (qint32 i = 0; i < nCount; ++i) {
            const qint32 nLength = pLengths[i];
            if ((nLength < 0) || (nLength > DN_MAX_BITS)) return false;
            if (nLength != 0) {
                ++anCounts[nLength];
                nMaxBits = (std::max)(nMaxBits, nLength);
                ++nSymbolCount;
            }
        }
        if (nSymbolCount == 0) return bAllowEmpty;

        // Reject over-subscribed trees. Incomplete trees are accepted because
        // the original installer accepts them and emits them for small files.
        qint32 nCodesLeft = 1;
        for (qint32 nLength = 1; nLength <= DN_MAX_BITS; ++nLength) {
            nCodesLeft = (nCodesLeft << 1) - anCounts[nLength];
            if (nCodesLeft < 0) return false;
        }

        std::array<quint16, DN_MAX_BITS + 1> anOffsets = {};
        for (qint32 nLength = 1; nLength < DN_MAX_BITS; ++nLength) {
            anOffsets[nLength + 1] = static_cast<quint16>(anOffsets[nLength] + anCounts[nLength]);
        }

        for (qint32 nSymbol = 0; nSymbol < nCount; ++nSymbol) {
            const qint32 nLength = pLengths[nSymbol];
            if (nLength != 0) {
                const quint16 nIndex = anOffsets[nLength]++;
                if (nIndex >= anSymbols.size()) return false;
                anSymbols[nIndex] = static_cast<quint16>(nSymbol);
            }
        }
        return true;
    }

    bool decode(DN_STATE *pState, quint32 *pSymbol) const;
};

struct DN_STATE {
    XBinary::DATAPROCESS_STATE *pProcessState = nullptr;
    XBinary::PDSTRUCT *pPdStruct = nullptr;
    quint32 nCompressedLeft = 0;
    quint32 nRawSize = 0;
    quint32 nProduced = 0;
    quint32 nBitBuffer = 0;
    qint32 nBitCount = 0;
    qint32 nInputPosition = 0;
    qint32 nInputSize = 0;
    qint32 nOutputUsed = 0;
    qint32 nWindowPosition = 0;
    bool bError = false;

    std::array<char, DN_INPUT_BUFFER_SIZE> abInput = {};
    std::array<char, DN_OUTPUT_BUFFER_SIZE> abOutput = {};
    std::array<quint8, DN_WINDOW_SIZE> abWindow = {};
    std::array<quint8, DN_MAX_DYNAMIC_LENGTHS> anCodeLengths = {};
};

bool dnPrepareProcessState(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct, quint32 *pCompressedSize, quint32 *pRawSize)
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

bool dnRefillInput(DN_STATE *pState)
{
    if (!pState || pState->bError || !pState->pProcessState || (pState->nCompressedLeft == 0) ||
        !XBinary::isPdStructNotCanceled(pState->pPdStruct)) {
        return false;
    }

    const qint32 nRequest = static_cast<qint32>((std::min)(static_cast<quint32>(DN_INPUT_BUFFER_SIZE), pState->nCompressedLeft));
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

bool dnReadByte(DN_STATE *pState, quint8 *pValue)
{
    if (!pState || !pValue || pState->bError || (pState->nCompressedLeft == 0) ||
        !XBinary::isPdStructNotCanceled(pState->pPdStruct)) {
        return false;
    }
    if ((pState->nInputPosition >= pState->nInputSize) && !dnRefillInput(pState)) return false;
    if ((pState->nInputPosition < 0) || (pState->nInputPosition >= pState->nInputSize)) {
        pState->bError = true;
        return false;
    }

    *pValue = static_cast<quint8>(pState->abInput[pState->nInputPosition++]);
    --pState->nCompressedLeft;
    return true;
}

bool dnReadBits(DN_STATE *pState, qint32 nCount, quint32 *pValue)
{
    if (!pState || !pValue || pState->bError || (nCount < 0) || (nCount > 16)) return false;

    while (pState->nBitCount < nCount) {
        quint8 nByte = 0;
        if (!dnReadByte(pState, &nByte)) return false;
        pState->nBitBuffer |= static_cast<quint32>(nByte) << pState->nBitCount;
        pState->nBitCount += 8;
    }

    const quint32 nMask = (nCount == 16) ? 0xffffU : ((static_cast<quint32>(1) << nCount) - 1);
    *pValue = pState->nBitBuffer & nMask;
    pState->nBitBuffer >>= nCount;
    pState->nBitCount -= nCount;
    return true;
}

bool DN_HUFFMAN::decode(DN_STATE *pState, quint32 *pSymbol) const
{
    if (!pState || !pSymbol || (nMaxBits <= 0) || (nMaxBits > DN_MAX_BITS) || (nSymbolCount <= 0)) return false;

    quint32 nCode = 0;
    quint32 nFirst = 0;
    quint32 nIndex = 0;
    for (qint32 nLength = 1; nLength <= nMaxBits; ++nLength) {
        quint32 nBit = 0;
        if (!dnReadBits(pState, 1, &nBit)) return false;
        nCode |= nBit;

        const quint32 nCount = anCounts[nLength];
        if ((nCode >= nFirst) && ((nCode - nFirst) < nCount)) {
            const quint32 nSymbolIndex = nIndex + (nCode - nFirst);
            if (nSymbolIndex >= static_cast<quint32>(nSymbolCount)) return false;
            *pSymbol = anSymbols[nSymbolIndex];
            return true;
        }

        nIndex += nCount;
        nFirst = (nFirst + nCount) << 1;
        nCode <<= 1;
    }
    return false;
}

bool dnFlushOutput(DN_STATE *pState)
{
    if (!pState || pState->bError || (pState->nOutputUsed < 0) || (pState->nOutputUsed > DN_OUTPUT_BUFFER_SIZE)) return false;
    if (pState->nOutputUsed == 0) return true;
    if (!XBinary::isPdStructNotCanceled(pState->pPdStruct) ||
        (XBinary::_writeDevice(pState->abOutput.data(), pState->nOutputUsed, pState->pProcessState) != pState->nOutputUsed)) {
        pState->bError = true;
        pState->pProcessState->bWriteError = true;
        return false;
    }
    pState->nOutputUsed = 0;
    return true;
}

bool dnEmitByte(DN_STATE *pState, quint8 nValue)
{
    if (!pState || pState->bError || (pState->nProduced >= pState->nRawSize)) return false;
    if (((pState->nProduced & 0xfffU) == 0) && !XBinary::isPdStructNotCanceled(pState->pPdStruct)) return false;
    if ((pState->nOutputUsed < 0) || (pState->nOutputUsed > DN_OUTPUT_BUFFER_SIZE)) return false;
    if ((pState->nOutputUsed == DN_OUTPUT_BUFFER_SIZE) && !dnFlushOutput(pState)) return false;
    if ((pState->nOutputUsed < 0) || (pState->nOutputUsed >= DN_OUTPUT_BUFFER_SIZE)) return false;

    pState->abWindow[pState->nWindowPosition] = nValue;
    pState->nWindowPosition = (pState->nWindowPosition + 1) & DN_WINDOW_MASK;
    pState->abOutput[pState->nOutputUsed++] = static_cast<char>(nValue);
    ++pState->nProduced;
    return true;
}

bool dnCopyMatch(DN_STATE *pState, quint32 nDistance, quint32 nLength)
{
    if (!pState || pState->bError || (nDistance == 0) || (nDistance > DN_WINDOW_SIZE) ||
        (nDistance > (std::min)(pState->nProduced, static_cast<quint32>(DN_WINDOW_SIZE))) ||
        (nLength > (pState->nRawSize - pState->nProduced))) {
        return false;
    }

    qint32 nSource = (pState->nWindowPosition - static_cast<qint32>(nDistance)) & DN_WINDOW_MASK;
    for (quint32 i = 0; i < nLength; ++i) {
        const quint8 nValue = pState->abWindow[nSource];
        nSource = (nSource + 1) & DN_WINDOW_MASK;
        if (!dnEmitByte(pState, nValue)) return false;
    }
    return true;
}

bool dnBuildFixedTrees(DN_HUFFMAN *pLiteralTree, DN_HUFFMAN *pDistanceTree)
{
    if (!pLiteralTree || !pDistanceTree) return false;

    std::array<quint8, 288> anLiteralLengths = {};
    std::array<quint8, 32> anDistanceLengths = {};
    for (qint32 i = 0; i <= 143; ++i) anLiteralLengths[i] = 8;
    for (qint32 i = 144; i <= 255; ++i) anLiteralLengths[i] = 9;
    for (qint32 i = 256; i <= 279; ++i) anLiteralLengths[i] = 7;
    for (qint32 i = 280; i <= 287; ++i) anLiteralLengths[i] = 8;
    anDistanceLengths.fill(5);
    return pLiteralTree->build(anLiteralLengths.data(), static_cast<qint32>(anLiteralLengths.size()), false) &&
           pDistanceTree->build(anDistanceLengths.data(), static_cast<qint32>(anDistanceLengths.size()), false);
}

bool dnBuildDynamicTrees(DN_STATE *pState, DN_HUFFMAN *pLiteralTree, DN_HUFFMAN *pDistanceTree)
{
    if (!pState || !pLiteralTree || !pDistanceTree) return false;

    quint32 nDistanceCount = 0;
    quint32 nCodeLengthCount = 0;
    quint32 nLiteralCount = 0;

    // Classic DN's inflater reads these fields in HDIST, HCLEN, HLIT order.
    // This is the sole format-level difference from an RFC 1951 dynamic block.
    if (!dnReadBits(pState, 5, &nDistanceCount) || !dnReadBits(pState, 4, &nCodeLengthCount) ||
        !dnReadBits(pState, 5, &nLiteralCount)) {
        return false;
    }
    ++nDistanceCount;
    nCodeLengthCount += 4;
    nLiteralCount += 257;
    if ((nDistanceCount > 30) || (nCodeLengthCount > 19) || (nLiteralCount > 286)) return false;

    std::array<quint8, 19> anCodeLengthLengths = {};
    for (quint32 i = 0; i < nCodeLengthCount; ++i) {
        quint32 nLength = 0;
        if (!dnReadBits(pState, 3, &nLength)) return false;
        anCodeLengthLengths[DN_CODE_LENGTH_ORDER[i]] = static_cast<quint8>(nLength);
    }

    DN_HUFFMAN codeLengthTree;
    if (!codeLengthTree.build(anCodeLengthLengths.data(), static_cast<qint32>(anCodeLengthLengths.size()), false)) return false;

    const quint32 nTotal = nLiteralCount + nDistanceCount;
    pState->anCodeLengths.fill(0);
    quint32 nIndex = 0;
    quint32 nPreviousLength = 0;
    while (nIndex < nTotal) {
        quint32 nSymbol = 0;
        if (!codeLengthTree.decode(pState, &nSymbol)) return false;
        if (nSymbol < 16) {
            pState->anCodeLengths[nIndex++] = static_cast<quint8>(nSymbol);
            nPreviousLength = nSymbol;
        } else {
            quint32 nRepeat = 0;
            quint32 nValue = 0;
            if (nSymbol == 16) {
                if (!dnReadBits(pState, 2, &nRepeat)) return false;
                nRepeat += 3;
                nValue = nPreviousLength;
            } else if (nSymbol == 17) {
                if (!dnReadBits(pState, 3, &nRepeat)) return false;
                nRepeat += 3;
                nPreviousLength = 0;
            } else if (nSymbol == 18) {
                if (!dnReadBits(pState, 7, &nRepeat)) return false;
                nRepeat += 11;
                nPreviousLength = 0;
            } else {
                return false;
            }
            if (nRepeat > (nTotal - nIndex)) return false;
            for (quint32 i = 0; i < nRepeat; ++i) pState->anCodeLengths[nIndex++] = static_cast<quint8>(nValue);
        }
    }

    if (pState->anCodeLengths[256] == 0) return false;
    return pLiteralTree->build(pState->anCodeLengths.data(), static_cast<qint32>(nLiteralCount), false) &&
           pDistanceTree->build(pState->anCodeLengths.data() + nLiteralCount, static_cast<qint32>(nDistanceCount), true);
}

bool dnDecodeHuffmanBlock(DN_STATE *pState, const DN_HUFFMAN &literalTree, const DN_HUFFMAN &distanceTree)
{
    if (!pState) return false;

    while (XBinary::isPdStructNotCanceled(pState->pPdStruct)) {
        quint32 nSymbol = 0;
        if (!literalTree.decode(pState, &nSymbol)) return false;
        if (nSymbol < 256) {
            if (!dnEmitByte(pState, static_cast<quint8>(nSymbol))) return false;
        } else if (nSymbol == 256) {
            return true;
        } else if (nSymbol <= 285) {
            const quint32 nLengthIndex = nSymbol - 257;
            quint32 nLengthExtra = 0;
            if (!dnReadBits(pState, DN_LENGTH_EXTRA[nLengthIndex], &nLengthExtra)) return false;
            const quint32 nLength = DN_LENGTH_BASE[nLengthIndex] + nLengthExtra;

            quint32 nDistanceSymbol = 0;
            if (!distanceTree.decode(pState, &nDistanceSymbol) || (nDistanceSymbol >= DN_DISTANCE_BASE.size())) return false;
            quint32 nDistanceExtra = 0;
            if (!dnReadBits(pState, DN_DISTANCE_EXTRA[nDistanceSymbol], &nDistanceExtra)) return false;
            const quint32 nDistance = DN_DISTANCE_BASE[nDistanceSymbol] + nDistanceExtra;
            if (!dnCopyMatch(pState, nDistance, nLength)) return false;
        } else {
            return false;
        }
    }
    return false;
}

bool dnDecodeStoredBlock(DN_STATE *pState)
{
    if (!pState) return false;
    pState->nBitBuffer = 0;
    pState->nBitCount = 0;

    quint32 nLength = 0;
    quint32 nComplement = 0;
    if (!dnReadBits(pState, 16, &nLength) || !dnReadBits(pState, 16, &nComplement) ||
        ((nLength ^ 0xffffU) != nComplement) || (nLength > (pState->nRawSize - pState->nProduced))) {
        return false;
    }
    for (quint32 i = 0; i < nLength; ++i) {
        quint32 nValue = 0;
        if (!dnReadBits(pState, 8, &nValue) || !dnEmitByte(pState, static_cast<quint8>(nValue))) return false;
    }
    return true;
}
}  // namespace

bool XDNDecoder::decompress(XBinary::DATAPROCESS_STATE *pProcessState, XBinary::PDSTRUCT *pPdStruct)
{
    quint32 nCompressedSize = 0;
    quint32 nRawSize = 0;
    if (!dnPrepareProcessState(pProcessState, pPdStruct, &nCompressedSize, &nRawSize) || (nCompressedSize == 0)) return false;

    std::unique_ptr<DN_STATE> pState(new (std::nothrow) DN_STATE);
    if (!pState) return false;
    pState->pProcessState = pProcessState;
    pState->pPdStruct = pPdStruct;
    pState->nCompressedLeft = nCompressedSize;
    pState->nRawSize = nRawSize;

    DN_HUFFMAN fixedLiteralTree;
    DN_HUFFMAN fixedDistanceTree;
    if (!dnBuildFixedTrees(&fixedLiteralTree, &fixedDistanceTree)) return false;

    bool bFinalBlock = false;
    do {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        quint32 nFinal = 0;
        quint32 nBlockType = 0;
        if (!dnReadBits(pState.get(), 1, &nFinal) || !dnReadBits(pState.get(), 2, &nBlockType)) return false;
        bFinalBlock = nFinal != 0;

        if (nBlockType == 0) {
            if (!dnDecodeStoredBlock(pState.get())) return false;
        } else if (nBlockType == 1) {
            if (!dnDecodeHuffmanBlock(pState.get(), fixedLiteralTree, fixedDistanceTree)) return false;
        } else if (nBlockType == 2) {
            DN_HUFFMAN literalTree;
            DN_HUFFMAN distanceTree;
            if (!dnBuildDynamicTrees(pState.get(), &literalTree, &distanceTree) ||
                !dnDecodeHuffmanBlock(pState.get(), literalTree, distanceTree)) {
                return false;
            }
        } else {
            return false;
        }
    } while (!bFinalBlock);

    // The final EOB may leave up to seven padding bits in its last byte, but
    // no additional byte is valid inside the member's declared packed extent.
    const bool bCodecExact = !pState->bError && (pState->nProduced == nRawSize) && (pState->nCompressedLeft == 0) &&
                             (pState->nInputPosition == pState->nInputSize) && dnFlushOutput(pState.get());
    return bCodecExact && !pState->bError && !pProcessState->bReadError && !pProcessState->bWriteError &&
           (pProcessState->nCountInput == nCompressedSize) && (pProcessState->nCountOutput == nRawSize) &&
           XBinary::isPdStructNotCanceled(pPdStruct);
}
