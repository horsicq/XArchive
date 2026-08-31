/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xspisrledecoder.h"

#include "algo_utils.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>

namespace {
const quint8 SPIS_RLE_ESCAPE = 0x94;
const qint32 SPIS_RLE_BUFFER_SIZE = 0x10000;

class SpisRleOutput
{
public:
    SpisRleOutput(char *pOutput, XBinary::DATAPROCESS_STATE *pState,
                  XBinary::PDSTRUCT *pPdStruct, qint64 nRawSize)
        : m_pOutput(pOutput),
          m_pState(pState),
          m_pPdStruct(pPdStruct),
          m_nRawSize(nRawSize)
    {
    }

    bool flush()
    {
        if (m_nOutputUsed == 0) return true;
        if (XBinary::_writeDevice(m_pOutput, m_nOutputUsed, m_pState) !=
            m_nOutputUsed) {
            return false;
        }
        m_nOutputUsed = 0;
        return true;
    }

    bool emitByte(quint8 nValue)
    {
        if ((m_pState->nCountOutput < 0) || (m_nOutputUsed < 0) ||
            (m_pState->nCountOutput > m_nRawSize - m_nOutputUsed) ||
            (m_pState->nCountOutput + m_nOutputUsed >= m_nRawSize)) {
            return false;
        }
        m_pOutput[m_nOutputUsed++] = static_cast<char>(nValue);
        return (m_nOutputUsed < SPIS_RLE_BUFFER_SIZE) || flush();
    }

    bool emitRepeat(quint8 nValue, qint32 nCount)
    {
        if ((nCount < 0) || (m_pState->nCountOutput < 0) ||
            (m_nOutputUsed < 0) ||
            (m_pState->nCountOutput > m_nRawSize - m_nOutputUsed) ||
            (static_cast<qint64>(nCount) >
             m_nRawSize - m_pState->nCountOutput - m_nOutputUsed)) {
            return false;
        }
        while (nCount > 0) {
            if (!XBinary::isPdStructNotCanceled(m_pPdStruct)) return false;
            const qint32 nSpace = SPIS_RLE_BUFFER_SIZE - m_nOutputUsed;
            const qint32 nChunk = (std::min)(nSpace, nCount);
            memset(m_pOutput + m_nOutputUsed, nValue,
                   static_cast<size_t>(nChunk));
            m_nOutputUsed += nChunk;
            nCount -= nChunk;
            if ((m_nOutputUsed == SPIS_RLE_BUFFER_SIZE) && !flush()) {
                return false;
            }
        }
        return true;
    }

private:
    char *m_pOutput = nullptr;
    XBinary::DATAPROCESS_STATE *m_pState = nullptr;
    XBinary::PDSTRUCT *m_pPdStruct = nullptr;
    qint64 m_nRawSize = 0;
    qint32 m_nOutputUsed = 0;
};
}

XSPISRLEDecoder::XSPISRLEDecoder(QObject *pParent) : QObject(pParent)
{
}

bool XSPISRLEDecoder::decompress(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pDeviceInput || !pState->pDeviceOutput || (pState->nInputOffset < 0) || (pState->nInputLimit < 0) ||
        !pState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        return false;
    }

    bool bRawSizeOK = false;
    const qint64 nRawSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bRawSizeOK);
    if (!bRawSizeOK || (nRawSize < 0) || !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nRawSize)) return false;

    char *pInput = new (std::nothrow) char[SPIS_RLE_BUFFER_SIZE];
    char *pOutput = new (std::nothrow) char[SPIS_RLE_BUFFER_SIZE];
    if (!pInput || !pOutput) {
        delete[] pInput;
        delete[] pOutput;
        return false;
    }

    Algo_utils::prepareState(pState);
    bool bResult = !pState->bReadError && !pState->bWriteError;
    bool bEscape = false;
    quint8 nLast = 0;
    SpisRleOutput output(pOutput, pState, pPdStruct, nRawSize);

    while (bResult && (pState->nCountInput < pState->nInputLimit) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nReadSize = Algo_utils::getReadChunkSize(pState, SPIS_RLE_BUFFER_SIZE);
        const qint32 nRead = XBinary::_readDevice(pInput, nReadSize, pState);
        if (nRead <= 0) {
            bResult = false;
            break;
        }

        for (qint32 i = 0; bResult && (i < nRead); ++i) {
            const quint8 nValue = static_cast<quint8>(pInput[i]);
            if (!bEscape) {
                if (nValue == SPIS_RLE_ESCAPE) {
                    bEscape = true;
                } else {
                    bResult = output.emitByte(nValue);
                    nLast = nValue;
                }
            } else {
                bEscape = false;
                if (nValue >= 2) {
                    bResult = output.emitRepeat(nLast, static_cast<qint32>(nValue) - 1);
                } else if (nValue == 0) {
                    // Confirmed from the GP-Install expander: count zero is the
                    // literal-escape form; count one emits no additional byte.
                    bResult = output.emitByte(SPIS_RLE_ESCAPE);
                }
                // The original decoder assigns last=b after every token.  A
                // repeat token therefore leaves 0x94 as the next last value;
                // runs over 255 bytes must restart with a fresh literal.
                nLast = SPIS_RLE_ESCAPE;
            }
        }
    }

    if (bResult) bResult = !bEscape && output.flush();
    bResult = bResult && !pState->bReadError && !pState->bWriteError && (pState->nCountInput == pState->nInputLimit) &&
              (pState->nCountOutput == nRawSize) && XBinary::isPdStructNotCanceled(pPdStruct);

    delete[] pInput;
    delete[] pOutput;
    return bResult;
}
