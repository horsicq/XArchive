/* Copyright (c) 2025 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xlzmadecoder.h"

static void *SzAlloc(ISzAllocPtr, size_t size)
{
    return malloc(size);
}

static void SzFree(ISzAllocPtr, void *address)
{
    free(address);
}

static ISzAlloc g_Alloc = {SzAlloc, SzFree};

static bool _decompressLZMACommon(CLzmaDec *pState, XBinary::DECOMPRESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    const qint32 N_BUFFER_SIZE = 0x4000;

    char bufferIn[N_BUFFER_SIZE];
    char bufferOut[N_BUFFER_SIZE];

    ELzmaStatus lastStatus = LZMA_STATUS_NOT_FINISHED;
    qint32 nLoopCount = 0;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint32 nBufferSize = qMin((qint32)(pDecompressState->nInputLimit - pDecompressState->nCountInput), N_BUFFER_SIZE);
        qint32 nSize = XBinary::_readDevice(bufferIn, nBufferSize, pDecompressState);

        // qDebug("_decompressLZMACommon() loop %d: read %d bytes, nCountInput=%lld, nInputLimit=%lld", 
               // nLoopCount++, nSize, pDecompressState->nCountInput, pDecompressState->nInputLimit);
        nLoopCount++;

        // Process available input
        qint64 nPos = 0;
        bool bContinueReading = true;

        while (bContinueReading && nPos < nSize && XBinary::isPdStructNotCanceled(pPdStruct)) {
            ELzmaStatus status;
            SizeT inProcessed = nSize - nPos;
            SizeT outProcessed = N_BUFFER_SIZE;

            SRes ret = LzmaDec_DecodeToBuf(pState, (Byte *)bufferOut, &outProcessed, (Byte *)(bufferIn + nPos), &inProcessed, LZMA_FINISH_ANY, &status);

            if (ret != 0) {  // Check for decompression error
                // qDebug("_decompressLZMACommon() FAILED: LzmaDec_DecodeToBuf returned %d", ret);
                // Dump first bytes of input for debugging
                // qDebug("  Input chunk size: %d bytes at nPos=%lld", nSize, nPos);
                // if (nSize >= 8) {
                //     qDebug("  First 8 bytes (hex): %02x %02x %02x %02x %02x %02x %02x %02x", 
                //            (unsigned char)bufferIn[0], (unsigned char)bufferIn[1], 
                //            (unsigned char)bufferIn[2], (unsigned char)bufferIn[3],
                //            (unsigned char)bufferIn[4], (unsigned char)bufferIn[5],
                //            (unsigned char)bufferIn[6], (unsigned char)bufferIn[7]);
                // }
                return false;
            }

            nPos += inProcessed;

            if (outProcessed > 0) {
                if (!XBinary::_writeDevice((char *)bufferOut, (qint32)outProcessed, pDecompressState)) {
                    // qDebug("_decompressLZMACommon() FAILED: _writeDevice returned false");
                    return false;
                }
            }

            lastStatus = status;

            // qDebug("_decompressLZMACommon() inner loop: inProcessed=%zu, outProcessed=%zu, status=%d", 
            //        inProcessed, outProcessed, status);

            if (status == LZMA_STATUS_FINISHED_WITH_MARK) {
                // Decompression completed successfully
                // qDebug("_decompressLZMACommon() LZMA_STATUS_FINISHED_WITH_MARK received");
                bContinueReading = false;
                break;
            }

            // If we couldn't process any input, stop
            if (inProcessed == 0) {
                // qDebug("_decompressLZMACommon() no input processed, stopping");
                break;
            }
        }

        // If we got stream end mark, stop reading more
        if (lastStatus == LZMA_STATUS_FINISHED_WITH_MARK) {
            break;
        }

        // If no data was read, we're done
        if (nSize == 0) {
            // qDebug("_decompressLZMACommon() no data read, exiting");
            break;
        }
    }

    return true;
}

static bool _decompressLZMA2Common(CLzma2Dec *pState, XBinary::DECOMPRESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    const qint32 N_BUFFER_SIZE = 0x4000;

    char bufferIn[N_BUFFER_SIZE];
    char bufferOut[N_BUFFER_SIZE];

    ELzmaStatus lastStatus = LZMA_STATUS_NOT_FINISHED;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint32 nBufferSize = qMin((qint32)(pDecompressState->nInputLimit - pDecompressState->nCountInput), N_BUFFER_SIZE);
        qint32 nSize = XBinary::_readDevice(bufferIn, nBufferSize, pDecompressState);

        // Process available input
        qint64 nPos = 0;
        bool bContinueReading = true;

        while (bContinueReading && nPos < nSize && XBinary::isPdStructNotCanceled(pPdStruct)) {
            ELzmaStatus status;
            SizeT inProcessed = nSize - nPos;
            SizeT outProcessed = N_BUFFER_SIZE;

            SRes ret = Lzma2Dec_DecodeToBuf(pState, (Byte *)bufferOut, &outProcessed, (Byte *)(bufferIn + nPos), &inProcessed, LZMA_FINISH_ANY, &status);

            if (ret != 0) {  // Check for decompression error
                return false;
            }

            nPos += inProcessed;

            if (outProcessed > 0) {
                if (!XBinary::_writeDevice((char *)bufferOut, (qint32)outProcessed, pDecompressState)) {
                    return false;
                }
            }

            lastStatus = status;

            if (status == LZMA_STATUS_FINISHED_WITH_MARK) {
                // Decompression completed successfully
                bContinueReading = false;
                break;
            }

            // If we couldn't process any input, stop
            if (inProcessed == 0) {
                break;
            }
        }

        // If we got stream end mark, stop reading more
        if (lastStatus == LZMA_STATUS_FINISHED_WITH_MARK) {
            break;
        }

        // If no data was read, we're done
        if (nSize == 0) {
            break;
        }
    }

    return true;
}

XLZMADecoder::XLZMADecoder(QObject *parent) : QObject(parent)
{
}

bool XLZMADecoder::decompress(XBinary::DECOMPRESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    if (pDecompressState->nInputLimit < 4) {
        return false;
    }

    pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    pDecompressState->pDeviceOutput->seek(0);

    qint32 nPropSize = 0;
    char header1[4] = {};
    quint8 properties[32] = {};

    XBinary::_readDevice(header1, sizeof(header1), pDecompressState);
    nPropSize = header1[2];  // TODO Check

    if (!nPropSize || nPropSize >= 30) {
        return false;
    }

    XBinary::_readDevice((char *)properties, nPropSize, pDecompressState);

    CLzmaDec state = {};
    SRes ret = LzmaProps_Decode(&state.prop, (Byte *)properties, nPropSize);

    if (ret != 0) {  // S_OK
        return false;
    }

    LzmaDec_Construct(&state);
    ret = LzmaDec_Allocate(&state, (Byte *)properties, nPropSize, &g_Alloc);

    if (ret != 0) {  // S_OK
        return false;
    }

    LzmaDec_Init(&state);
    bool bResult = _decompressLZMACommon(&state, pDecompressState, pPdStruct);
    LzmaDec_Free(&state, &g_Alloc);

    return bResult;
}

bool XLZMADecoder::decompress(XBinary::DECOMPRESS_STATE *pDecompressState, const QByteArray &baProperty, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        // qDebug("XLZMADecoder::decompress() FAILED: null pointer check");
        return false;
    }

    if (baProperty.size() <= 0 || baProperty.size() >= 30) {
        // qDebug("XLZMADecoder::decompress() FAILED: invalid baProperty size: %d", baProperty.size());
        return false;
    }

    // qDebug("XLZMADecoder::decompress() called with baProperty.size()=%d, nInputLimit=%lld", 
    //        baProperty.size(), pDecompressState->nInputLimit);
    
    // Dump LZMA properties
    // if (baProperty.size() >= 5) {
    //     qDebug("  LZMA Properties (hex): %02x %02x %02x %02x %02x",
    //            (unsigned char)baProperty[0], (unsigned char)baProperty[1],
    //            (unsigned char)baProperty[2], (unsigned char)baProperty[3],
    //            (unsigned char)baProperty[4]);
    // }

    pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    pDecompressState->pDeviceOutput->seek(0);

    CLzmaDec state = {};
    SRes ret = LzmaProps_Decode(&state.prop, (Byte *)baProperty.constData(), baProperty.size());

    if (ret != 0) {  // S_OK
        // qDebug("XLZMADecoder::decompress() FAILED: LzmaProps_Decode returned %d", ret);
        return false;
    }

    LzmaDec_Construct(&state);
    ret = LzmaDec_Allocate(&state, (Byte *)baProperty.constData(), baProperty.size(), &g_Alloc);

    if (ret != 0) {  // S_OK
        // qDebug("XLZMADecoder::decompress() FAILED: LzmaDec_Allocate returned %d", ret);
        return false;
    }

    LzmaDec_Init(&state);
    bool bResult = _decompressLZMACommon(&state, pDecompressState, pPdStruct);
    LzmaDec_Free(&state, &g_Alloc);

    // qDebug("XLZMADecoder::decompress() result: %s, decompressed size: %lld", 
    //        bResult ? "TRUE" : "FALSE", pDecompressState->nCountOutput);

    return bResult;
}

bool XLZMADecoder::decompressLZMA2(XBinary::DECOMPRESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    if (pDecompressState->nInputLimit < 1) {
        return false;
    }

    pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    pDecompressState->pDeviceOutput->seek(0);

    // Read LZMA2 properties (1 byte)
    char propByte = 0;
    qint32 nPropsRead = XBinary::_readDevice(&propByte, 1, pDecompressState);

    if (nPropsRead != 1) {
        return false;
    }

    // LZMA2 state
    CLzma2Dec state = {};
    SRes ret = Lzma2Dec_Allocate(&state, (Byte)propByte, &g_Alloc);

    if (ret != 0) {  // S_OK
        return false;
    }

    Lzma2Dec_Init(&state);
    bool bResult = _decompressLZMA2Common(&state, pDecompressState, pPdStruct);
    Lzma2Dec_Free(&state, &g_Alloc);

    return bResult;
}

bool XLZMADecoder::decompressLZMA2(XBinary::DECOMPRESS_STATE *pDecompressState, const QByteArray &baProperty, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    if (baProperty.size() != 1) {
        return false;
    }

    pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    pDecompressState->pDeviceOutput->seek(0);

    // LZMA2 state
    CLzma2Dec state = {};
    SRes ret = Lzma2Dec_Allocate(&state, (Byte)baProperty[0], &g_Alloc);

    if (ret != 0) {  // S_OK
        return false;
    }

    Lzma2Dec_Init(&state);
    bool bResult = _decompressLZMA2Common(&state, pDecompressState, pPdStruct);
    Lzma2Dec_Free(&state, &g_Alloc);

    return bResult;
}
