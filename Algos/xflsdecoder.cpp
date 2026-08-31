/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xflsdecoder.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace {
const qint32 FLS_INPUT_BUFFER_SIZE = 0x4000;
const qint32 FLS_OUTPUT_BUFFER_SIZE = 0x4000;
const qint32 FLS_ENTRY_COUNT = 0x163f;
const qint32 FLS_END_CODE = 0x163f;
const qint32 FLS_CODE_COUNT = 0x1640;
const qint32 FLS_LITERAL_COUNT = 0x100;
const qint32 FLS_CLASS_COUNT = 6;
const qint32 FLS_MAX_PHRASE_LENGTH = 250;
const qint32 FLS_MAX_EXTENSION_LENGTH = 30;
const qint32 FLS_REPLACEMENT_SCAN_LIMIT = FLS_ENTRY_COUNT * 64;

const std::array<quint16, FLS_CLASS_COUNT> FLS_CLASS_BASE = {0x000, 0x080, 0x0c0, 0x140, 0x240, 0x640};
const std::array<quint16, FLS_CLASS_COUNT> FLS_CLASS_END = {0x080, 0x0c0, 0x140, 0x240, 0x640, 0x1640};
const std::array<quint8, FLS_CLASS_COUNT> FLS_CLASS_BITS = {7, 6, 7, 8, 10, 12};

struct FLS_STATE {
    XBinary::DATAPROCESS_STATE *pProcessState = nullptr;
    XBinary::PDSTRUCT *pPdStruct = nullptr;
    quint32 nCompressedLeft = 0;
    quint32 nRawSize = 0;
    quint32 nProduced = 0;
    quint32 nCurrentByte = 0;
    qint32 nBitsLeft = 0;
    qint32 nInputPosition = 0;
    qint32 nInputSize = 0;
    qint32 nOutputUsed = 0;
    quint16 nLastSlot = 0xff;
    quint16 nPreviousEntry = 0;
    quint16 nPreviousLength = 0;
    quint16 nTokenCounter = 0x1f3;
    bool bHavePrevious = false;
    bool bError = false;

    std::array<char, FLS_INPUT_BUFFER_SIZE> abInput = {};
    std::array<char, FLS_OUTPUT_BUFFER_SIZE> abOutput = {};
    std::array<std::array<quint8, FLS_MAX_PHRASE_LENGTH>, FLS_ENTRY_COUNT> aanPhrases = {};
    std::array<quint8, FLS_ENTRY_COUNT> anPhraseLengths = {};
    std::array<quint16, FLS_ENTRY_COUNT> anParents = {};
    std::array<quint16, FLS_ENTRY_COUNT> anReferenceCounts = {};
    std::array<quint8, FLS_ENTRY_COUNT> anAges = {};
    std::array<quint16, FLS_ENTRY_COUNT> anCodeToEntry = {};
    std::array<quint16, FLS_ENTRY_COUNT> anEntryToCode = {};
    std::array<quint8, FLS_CODE_COUNT> anCodeClasses = {};
    std::array<quint8, FLS_CODE_COUNT> anCodeScores = {};
    std::array<quint16, FLS_CLASS_COUNT> anScanCursors = {};
    std::array<quint8, FLS_CLASS_COUNT> anClassOrder = {};
};

bool flsPrepareProcessState(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct, quint32 *pCompressedSize, quint32 *pRawSize)
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

void flsInitializeDictionary(FLS_STATE *pState)
{
    if (!pState) return;

    pState->anParents.fill(0xffff);
    for (qint32 i = 0; i < FLS_ENTRY_COUNT; ++i) {
        pState->anCodeToEntry[i] = static_cast<quint16>(i);
        pState->anEntryToCode[i] = static_cast<quint16>(i);
        if (i < FLS_LITERAL_COUNT) {
            pState->aanPhrases[i][0] = static_cast<quint8>(i);
            pState->anPhraseLengths[i] = 1;
        }
    }

    for (qint32 nClass = 0; nClass < FLS_CLASS_COUNT; ++nClass) {
        std::fill(pState->anCodeClasses.begin() + FLS_CLASS_BASE[nClass], pState->anCodeClasses.begin() + FLS_CLASS_END[nClass],
                  static_cast<quint8>(nClass));
    }

    // Each replacement cursor begins immediately before the class it scans.
    // Cursor one deliberately wraps from 0xffff to code zero on first use.
    pState->anScanCursors = {0, 0xffff, 0x007f, 0x00bf, 0x013f, 0x023f};
}

bool flsRefillInput(FLS_STATE *pState)
{
    if (!pState || pState->bError || !pState->pProcessState || (pState->nCompressedLeft == 0) ||
        !XBinary::isPdStructNotCanceled(pState->pPdStruct)) {
        return false;
    }

    const qint32 nRequest = static_cast<qint32>((std::min)(static_cast<quint32>(FLS_INPUT_BUFFER_SIZE), pState->nCompressedLeft));
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

bool flsReadByte(FLS_STATE *pState, quint8 *pValue)
{
    if (!pState || !pValue || pState->bError || (pState->nCompressedLeft == 0) ||
        !XBinary::isPdStructNotCanceled(pState->pPdStruct)) {
        return false;
    }
    if ((pState->nInputPosition >= pState->nInputSize) && !flsRefillInput(pState)) return false;
    if ((pState->nInputPosition < 0) || (pState->nInputPosition >= pState->nInputSize)) {
        pState->bError = true;
        return false;
    }

    *pValue = static_cast<quint8>(pState->abInput[pState->nInputPosition++]);
    --pState->nCompressedLeft;
    return true;
}

bool flsReadBits(FLS_STATE *pState, qint32 nCount, quint32 *pValue)
{
    if (!pState || !pValue || pState->bError || (nCount < 0) || (nCount > 16)) return false;

    quint32 nResult = 0;
    while (nCount != 0) {
        if (pState->nBitsLeft == 0) {
            quint8 nByte = 0;
            if (!flsReadByte(pState, &nByte)) return false;
            pState->nCurrentByte = nByte;
            pState->nBitsLeft = 8;
        }

        const qint32 nTake = (std::min)(nCount, pState->nBitsLeft);
        const qint32 nShift = pState->nBitsLeft - nTake;
        const quint32 nMask = (static_cast<quint32>(1) << nTake) - 1;
        nResult = (nResult << nTake) | ((pState->nCurrentByte >> nShift) & nMask);
        pState->nBitsLeft -= nTake;
        nCount -= nTake;
    }

    *pValue = nResult;
    return true;
}

bool flsReadClassOrder(FLS_STATE *pState)
{
    quint32 nFirst = 0;
    quint32 nSecond = 0;
    if (!flsReadBits(pState, 3, &nFirst) || !flsReadBits(pState, 3, &nSecond) || (nFirst >= FLS_CLASS_COUNT) ||
        (nSecond >= FLS_CLASS_COUNT) || (nFirst == nSecond)) {
        return false;
    }

    pState->anClassOrder[0] = static_cast<quint8>(nFirst);
    pState->anClassOrder[1] = static_cast<quint8>(nSecond);
    qint32 nOutput = 2;
    for (qint32 i = 0; i < FLS_CLASS_COUNT; ++i) {
        if ((i != static_cast<qint32>(nFirst)) && (i != static_cast<qint32>(nSecond))) {
            pState->anClassOrder[nOutput++] = static_cast<quint8>(i);
        }
    }
    return nOutput == FLS_CLASS_COUNT;
}

bool flsReadCode(FLS_STATE *pState, quint16 *pCode)
{
    if (!pState || !pCode) return false;

    quint32 nSelector = 0;
    if (!flsReadBits(pState, 2, &nSelector)) return false;
    if (nSelector >= 2) {
        quint32 nExtra = 0;
        if (!flsReadBits(pState, 1, &nExtra)) return false;
        nSelector = nSelector * 2 - 2 + nExtra;
    }
    if (nSelector >= FLS_CLASS_COUNT) return false;

    const qint32 nClass = pState->anClassOrder[nSelector];
    if ((nClass < 0) || (nClass >= FLS_CLASS_COUNT)) return false;
    quint32 nValue = 0;
    if (!flsReadBits(pState, FLS_CLASS_BITS[nClass], &nValue)) return false;
    nValue += FLS_CLASS_BASE[nClass];
    if (nValue >= FLS_CODE_COUNT) return false;
    *pCode = static_cast<quint16>(nValue);
    return true;
}

quint16 flsAdvanceScan(FLS_STATE *pState, qint32 nGroup)
{
    quint16 nValue = static_cast<quint16>(pState->anScanCursors[nGroup] + 1);
    if (nValue == FLS_CLASS_END[nGroup - 1]) nValue = FLS_CLASS_BASE[nGroup - 1];
    pState->anScanCursors[nGroup] = nValue;
    return nValue;
}

bool flsSwapCodes(FLS_STATE *pState, quint16 nLeft, quint16 nRight)
{
    if (!pState || (nLeft >= FLS_ENTRY_COUNT) || (nRight >= FLS_ENTRY_COUNT)) return false;

    const quint16 nLeftEntry = pState->anCodeToEntry[nLeft];
    const quint16 nRightEntry = pState->anCodeToEntry[nRight];
    if ((nLeftEntry >= FLS_ENTRY_COUNT) || (nRightEntry >= FLS_ENTRY_COUNT)) return false;
    pState->anCodeToEntry[nLeft] = nRightEntry;
    pState->anCodeToEntry[nRight] = nLeftEntry;
    pState->anEntryToCode[nRightEntry] = nLeft;
    pState->anEntryToCode[nLeftEntry] = nRight;
    return true;
}

bool flsInsertPhrase(FLS_STATE *pState, quint16 nCurrentEntry, const quint8 *pCurrent, qint32 nCurrentLength)
{
    if (!pState || !pCurrent || (nCurrentEntry >= FLS_ENTRY_COUNT) || (nCurrentLength <= 0) ||
        (nCurrentLength > FLS_MAX_PHRASE_LENGTH) || (pState->nPreviousEntry >= FLS_ENTRY_COUNT) ||
        (pState->nPreviousLength >= FLS_MAX_PHRASE_LENGTH) ||
        (pState->anPhraseLengths[pState->nPreviousEntry] != pState->nPreviousLength) ||
        (pState->anReferenceCounts[pState->nPreviousEntry] == 0xffff)) {
        return false;
    }
    ++pState->anReferenceCounts[pState->nPreviousEntry];

    quint16 nCandidate = pState->nLastSlot;
    qint32 nIterations = 0;
    for (;;) {
        ++nCandidate;
        if (nCandidate == FLS_ENTRY_COUNT) nCandidate = FLS_LITERAL_COUNT;

        if (pState->anAges[nCandidate] != 0) {
            --pState->anAges[nCandidate];
        } else if ((pState->anReferenceCounts[nCandidate] == 0) && (nCandidate != nCurrentEntry)) {
            break;
        }

        if (++nIterations > FLS_REPLACEMENT_SCAN_LIMIT) return false;
        if (((nIterations & 0xfff) == 0) && !XBinary::isPdStructNotCanceled(pState->pPdStruct)) return false;
    }

    const quint16 nOldParent = pState->anParents[nCandidate];
    if (nOldParent != 0xffff) {
        if ((nOldParent >= FLS_ENTRY_COUNT) || (pState->anReferenceCounts[nOldParent] == 0)) return false;
        --pState->anReferenceCounts[nOldParent];
    }

    const qint32 nExtensionLength =
        (std::min)((std::min)(nCurrentLength, FLS_MAX_EXTENSION_LENGTH), FLS_MAX_PHRASE_LENGTH - static_cast<qint32>(pState->nPreviousLength));
    const qint32 nNewLength = static_cast<qint32>(pState->nPreviousLength) + nExtensionLength;
    if ((nExtensionLength <= 0) || (nNewLength <= 0) || (nNewLength > FLS_MAX_PHRASE_LENGTH)) return false;

    std::memcpy(pState->aanPhrases[nCandidate].data(), pState->aanPhrases[pState->nPreviousEntry].data(), pState->nPreviousLength);
    std::memcpy(pState->aanPhrases[nCandidate].data() + pState->nPreviousLength, pCurrent, nExtensionLength);
    pState->anParents[nCandidate] = pState->nPreviousEntry;
    pState->anPhraseLengths[nCandidate] = static_cast<quint8>(nNewLength);
    pState->nLastSlot = nCandidate;

    const quint16 nCurrentCode = pState->anEntryToCode[nCandidate];
    if (nCurrentCode >= FLS_CLASS_BASE[5]) {
        const quint16 nReplacementCode = flsAdvanceScan(pState, 1);
        if (!flsSwapCodes(pState, nReplacementCode, nCurrentCode)) return false;
    }
    return true;
}

bool flsAdaptCode(FLS_STATE *pState, quint16 nEntry, qint32 nPhraseLength)
{
    if (!pState || (nEntry >= FLS_ENTRY_COUNT) || (nPhraseLength <= 0) || (nPhraseLength > FLS_MAX_PHRASE_LENGTH)) return false;
    const quint16 nCurrentCode = pState->anEntryToCode[nEntry];
    if (nCurrentCode >= FLS_ENTRY_COUNT) return false;

    const qint32 nClass = pState->anCodeClasses[nCurrentCode];
    if ((nClass < 0) || (nClass >= FLS_CLASS_COUNT)) return false;
    if (nClass == 1) {
        qint32 nScore = pState->anCodeScores[nCurrentCode] + 2;
        if (nPhraseLength == 1) nScore += 2;
        pState->anCodeScores[nCurrentCode] = static_cast<quint8>((std::min)(nScore, 20));
        return true;
    }

    const qint32 nTargetClass = (nClass == 0) ? 5 : nClass;
    if ((nTargetClass <= 0) || (nTargetClass >= FLS_CLASS_COUNT)) return false;
    if (nTargetClass < 5) pState->anCodeScores[nCurrentCode] = static_cast<quint8>(nTargetClass * 2 - 2);

    quint16 nReplacementCode = 0;
    qint32 nIterations = 0;
    do {
        nReplacementCode = flsAdvanceScan(pState, nTargetClass);
        if (nReplacementCode >= FLS_ENTRY_COUNT) return false;
        if (pState->anCodeScores[nReplacementCode] == 0) break;
        --pState->anCodeScores[nReplacementCode];
        if (++nIterations > FLS_CODE_COUNT * 21) return false;
    } while (true);

    return flsSwapCodes(pState, nReplacementCode, nCurrentCode);
}

bool flsFlushOutput(FLS_STATE *pState)
{
    if (!pState || pState->bError || (pState->nOutputUsed < 0) || (pState->nOutputUsed > FLS_OUTPUT_BUFFER_SIZE)) return false;
    if (pState->nOutputUsed == 0) return true;
    if (!XBinary::isPdStructNotCanceled(pState->pPdStruct) ||
        (XBinary::_writeDevice(pState->abOutput.data(), pState->nOutputUsed, pState->pProcessState) != pState->nOutputUsed)) {
        pState->bError = true;
        return false;
    }
    pState->nOutputUsed = 0;
    return true;
}

bool flsEmitPhrase(FLS_STATE *pState, const quint8 *pData, qint32 nSize)
{
    if (!pState || !pData || pState->bError || (nSize <= 0) ||
        (static_cast<quint32>(nSize) > (pState->nRawSize - pState->nProduced))) {
        return false;
    }

    qint32 nPosition = 0;
    while (nPosition < nSize) {
        if (pState->nOutputUsed == FLS_OUTPUT_BUFFER_SIZE && !flsFlushOutput(pState)) return false;
        const qint32 nCopy = (std::min)(nSize - nPosition, FLS_OUTPUT_BUFFER_SIZE - pState->nOutputUsed);
        std::memcpy(pState->abOutput.data() + pState->nOutputUsed, pData + nPosition, nCopy);
        pState->nOutputUsed += nCopy;
        nPosition += nCopy;
    }
    pState->nProduced += static_cast<quint32>(nSize);
    return true;
}
}  // namespace

bool XFLSDecoder::decompress(XBinary::DATAPROCESS_STATE *pProcessState, XBinary::PDSTRUCT *pPdStruct)
{
    quint32 nCompressedSize = 0;
    quint32 nRawSize = 0;
    if (!flsPrepareProcessState(pProcessState, pPdStruct, &nCompressedSize, &nRawSize) || (nCompressedSize == 0)) return false;

    std::unique_ptr<FLS_STATE> pState(new (std::nothrow) FLS_STATE);
    if (!pState) return false;
    pState->pProcessState = pProcessState;
    pState->pPdStruct = pPdStruct;
    pState->nCompressedLeft = nCompressedSize;
    pState->nRawSize = nRawSize;
    flsInitializeDictionary(pState.get());

    quint8 nTag = 0;
    if (!flsReadByte(pState.get(), &nTag) || (nTag != 0x53)) return false;

    bool bEndSeen = false;
    while (!pState->bError && XBinary::isPdStructNotCanceled(pPdStruct)) {
        ++pState->nTokenCounter;
        if (pState->nTokenCounter == 0x1f4) {
            if (!flsReadClassOrder(pState.get())) {
                pState->bError = true;
                break;
            }
            pState->nTokenCounter = 0;
        }

        quint16 nCode = 0;
        if (!flsReadCode(pState.get(), &nCode)) {
            pState->bError = true;
            break;
        }
        if (nCode == FLS_END_CODE) {
            bEndSeen = true;
            break;
        }
        if ((nCode >= FLS_ENTRY_COUNT) || (pState->nProduced >= pState->nRawSize)) {
            pState->bError = true;
            break;
        }

        const quint16 nEntry = pState->anCodeToEntry[nCode];
        if (nEntry >= FLS_ENTRY_COUNT) {
            pState->bError = true;
            break;
        }
        const qint32 nLength = pState->anPhraseLengths[nEntry];
        if ((nLength <= 0) || (nLength > FLS_MAX_PHRASE_LENGTH) ||
            (static_cast<quint32>(nLength) > (pState->nRawSize - pState->nProduced)) ||
            !flsEmitPhrase(pState.get(), pState->aanPhrases[nEntry].data(), nLength)) {
            pState->bError = true;
            break;
        }

        if (pState->anAges[nEntry] < 0x32) pState->anAges[nEntry] = static_cast<quint8>(pState->anAges[nEntry] + 5);
        if (pState->bHavePrevious && (pState->nPreviousLength < FLS_MAX_PHRASE_LENGTH) &&
            !flsInsertPhrase(pState.get(), nEntry, pState->aanPhrases[nEntry].data(), nLength)) {
            pState->bError = true;
            break;
        }

        pState->nPreviousEntry = nEntry;
        pState->nPreviousLength = static_cast<quint16>(nLength);
        pState->bHavePrevious = true;
        if (!flsAdaptCode(pState.get(), nEntry, nLength)) {
            pState->bError = true;
            break;
        }
    }

    // The end code may leave up to seven padding bits in its final byte, but
    // no additional byte is valid inside the member's declared packed extent.
    const bool bCodecExact = bEndSeen && !pState->bError && (pState->nProduced == nRawSize) && (pState->nCompressedLeft == 0) &&
                             (pState->nInputPosition == pState->nInputSize) && flsFlushOutput(pState.get());
    return bCodecExact && !pState->bError && !pProcessState->bReadError && !pProcessState->bWriteError &&
           (pProcessState->nCountInput == nCompressedSize) && (pProcessState->nCountOutput == nRawSize) &&
           XBinary::isPdStructNotCanceled(pPdStruct);
}
