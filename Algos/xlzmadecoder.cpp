/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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
#include <QBuffer>

static void *SzAlloc(ISzAllocPtr, size_t size)
{
    return malloc(size);
}

static void SzFree(ISzAllocPtr, void *address)
{
    free(address);
}

static ISzAlloc g_Alloc = {SzAlloc, SzFree};

static bool _decompressLZMACommon(CLzmaDec *pState, XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    qint32 _nBufferSize = XBinary::getBufferSize(pPdStruct);

    char *bufferIn = new char[_nBufferSize];
    char *bufferOut = new char[_nBufferSize];

    ELzmaStatus lastStatus = LZMA_STATUS_NOT_FINISHED;
    qint32 nLoopCount = 0;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint32 nBufferSize = qMin((qint32)(pDecompressState->nInputLimit - pDecompressState->nCountInput), _nBufferSize);
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
            SizeT outProcessed = _nBufferSize;

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
                delete[] bufferIn;
                delete[] bufferOut;
                return false;
            }

            nPos += inProcessed;

            if (outProcessed > 0) {
                if (!XBinary::_writeDevice((char *)bufferOut, (qint32)outProcessed, pDecompressState)) {
                    // qDebug("_decompressLZMACommon() FAILED: _writeDevice returned false");
                    delete[] bufferIn;
                    delete[] bufferOut;
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

    delete[] bufferIn;
    delete[] bufferOut;

    return true;
}

static bool _decompressLZMA2Common(CLzma2Dec *pState, XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    qint32 _nBufferSize = XBinary::getBufferSize(pPdStruct);

    char *bufferIn = new char[_nBufferSize];
    char *bufferOut = new char[_nBufferSize];

    ELzmaStatus lastStatus = LZMA_STATUS_NOT_FINISHED;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint32 nBufferSize = qMin((qint32)(pDecompressState->nInputLimit - pDecompressState->nCountInput), _nBufferSize);
        qint32 nSize = XBinary::_readDevice(bufferIn, nBufferSize, pDecompressState);

        // Process available input
        qint64 nPos = 0;
        bool bContinueReading = true;

        while (bContinueReading && nPos < nSize && XBinary::isPdStructNotCanceled(pPdStruct)) {
            ELzmaStatus status;
            SizeT inProcessed = nSize - nPos;
            SizeT outProcessed = _nBufferSize;

            SRes ret = Lzma2Dec_DecodeToBuf(pState, (Byte *)bufferOut, &outProcessed, (Byte *)(bufferIn + nPos), &inProcessed, LZMA_FINISH_ANY, &status);

            if (ret != 0) {  // Check for decompression error
                delete[] bufferIn;
                delete[] bufferOut;
                return false;
            }

            nPos += inProcessed;

            if (outProcessed > 0) {
                if (!XBinary::_writeDevice((char *)bufferOut, (qint32)outProcessed, pDecompressState)) {
                    delete[] bufferIn;
                    delete[] bufferOut;
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

    delete[] bufferIn;
    delete[] bufferOut;

    return true;
}

XLZMADecoder::XLZMADecoder(QObject *parent) : QObject(parent)
{
}

bool XLZMADecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
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

bool XLZMADecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, const QByteArray &baProperty, XBinary::PDSTRUCT *pPdStruct)
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

bool XLZMADecoder::decompressLZMA2(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
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

bool XLZMADecoder::decompressLZMA2(XBinary::DATAPROCESS_STATE *pDecompressState, const QByteArray &baProperty, XBinary::PDSTRUCT *pPdStruct)
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

// XZ variable-length integer decoder
static bool _xzReadVarInt(const QByteArray &ba, qint32 &nPos, quint64 &nValue)
{
    nValue = 0;
    qint32 nShift = 0;
    while (nPos < ba.size()) {
        quint8 nByte = (quint8)ba.at(nPos++);
        nValue |= ((quint64)(nByte & 0x7F)) << nShift;
        if (!(nByte & 0x80)) {
            return true;
        }
        nShift += 7;
        if (nShift > 63) {
            return false;
        }
    }
    return false;
}

// BCJ x86 reverse filter (decompression side)
// Algorithm from xz-utils liblzma/simple/x86.c:
//   forward (encoding): stored = rel_addr + (now_pos + i + 1)
//   reverse (decoding): rel_addr = stored - (now_pos + i + 1), now_pos = 0
static void _applyBCJX86Decode(QByteArray &ba)
{
    qint32 nSize = ba.size();
    if (nSize < 5) {
        return;
    }
    for (qint32 i = 0; i <= nSize - 5;) {
        quint8 nOpcode = (quint8)ba.at(i);
        if (nOpcode != 0xE8 && nOpcode != 0xE9) {
            i++;
            continue;
        }
        // Validity check: MSB of the stored 4-byte value must be 0x00 or 0xFF
        quint8 nMSB = (quint8)ba.at(i + 4);
        if (nMSB != 0x00 && nMSB != 0xFF) {
            i++;
            continue;
        }
        // Read stored 4-byte LE value
        quint32 nStored = (quint32)(quint8)ba[i + 1] | ((quint32)(quint8)ba[i + 2] << 8) | ((quint32)(quint8)ba[i + 3] << 16) | ((quint32)(quint8)ba[i + 4] << 24);
        // Reverse: rel_addr = stored - (now_pos + i + 5)  with now_pos = 0
        quint32 nRelAddr = nStored - (quint32)(i + 5);
        ba[i + 1] = (char)(nRelAddr & 0xFF);
        ba[i + 2] = (char)((nRelAddr >> 8) & 0xFF);
        ba[i + 3] = (char)((nRelAddr >> 16) & 0xFF);
        ba[i + 4] = (char)((nRelAddr >> 24) & 0xFF);
        i += 5;
    }
}

bool XLZMADecoder::decompressXZ(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    QIODevice *pDevice = pDecompressState->pDeviceInput;
    qint64 nOffset = pDecompressState->nInputOffset;
    qint64 nTotalSize = pDecompressState->nInputLimit;

    if (nTotalSize < 28) {  // stream header(12) + minimal block header(4) + stream footer(12)
        return false;
    }

    // --- 1. Validate XZ stream header (12 bytes) ---
    pDevice->seek(nOffset);
    QByteArray baStreamHeader = pDevice->read(12);
    if (baStreamHeader.size() != 12) {
        return false;
    }
    static const quint8 XZ_MAGIC[6] = {0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00};
    if (memcmp(baStreamHeader.constData(), XZ_MAGIC, 6) != 0) {
        return false;
    }

    // --- 2. Parse block header (starts at offset 12) ---
    qint64 nBlockHeaderOffset = nOffset + 12;
    pDevice->seek(nBlockHeaderOffset);

    // header_size byte: actual block header size = (header_size + 1) * 4
    char nHdrSizeBuf = 0;
    if (pDevice->read(&nHdrSizeBuf, 1) != 1) {
        return false;
    }
    quint8 nHeaderSizeByte = (quint8)nHdrSizeBuf;
    qint32 nActualHeaderSize = (nHeaderSizeByte + 1) * 4;
    if (nActualHeaderSize < 4 || nActualHeaderSize > 1024) {
        return false;
    }

    pDevice->seek(nBlockHeaderOffset);
    QByteArray baBH = pDevice->read(nActualHeaderSize);
    if (baBH.size() != nActualHeaderSize) {
        return false;
    }

    quint8 nBlockFlags = (quint8)baBH.at(1);
    qint32 nNumFilters = (nBlockFlags & 0x03) + 1;
    bool bHasCompressedSize = (nBlockFlags & 0x40) != 0;
    bool bHasUncompressedSize = (nBlockFlags & 0x80) != 0;

    qint32 nBHPos = 2;
    quint64 nCompressedSize64 = 0;
    quint64 nUncompressedSize64 = 0;

    if (bHasCompressedSize) {
        if (!_xzReadVarInt(baBH, nBHPos, nCompressedSize64)) {
            return false;
        }
    }
    if (bHasUncompressedSize) {
        if (!_xzReadVarInt(baBH, nBHPos, nUncompressedSize64)) {
            return false;
        }
    }

    // Parse filter chain: find LZMA2 props byte and optional BCJ x86 filter
    bool bHasBCJX86 = false;
    quint8 nLZMA2PropsByte = 0;

    for (qint32 nFilter = 0; nFilter < nNumFilters && nFilter < 4; nFilter++) {
        quint64 nFilterID = 0;
        quint64 nPropSize = 0;
        if (!_xzReadVarInt(baBH, nBHPos, nFilterID)) {
            return false;
        }
        if (!_xzReadVarInt(baBH, nBHPos, nPropSize)) {
            return false;
        }
        if (nFilterID == 0x21) {  // LZMA2
            if (nPropSize == 1 && nBHPos < baBH.size()) {
                nLZMA2PropsByte = (quint8)baBH.at(nBHPos);
            }
        } else if (nFilterID == 0x04) {  // BCJ x86
            bHasBCJX86 = true;
        }
        nBHPos += (qint32)nPropSize;
    }

    // --- 3. Locate compressed block data ---
    qint64 nDataOffset = nBlockHeaderOffset + nActualHeaderSize;
    qint64 nDataSize = 0;
    if (bHasCompressedSize) {
        nDataSize = (qint64)nCompressedSize64;
    } else {
        // Generous estimate; LZMA2 end-of-stream marker stops decompression naturally
        nDataSize = nTotalSize - 12 - nActualHeaderSize - 12;
        if (nDataSize <= 0) {
            return false;
        }
    }

    pDevice->seek(nDataOffset);
    QByteArray baCompressed = pDevice->read(nDataSize);
    if (baCompressed.isEmpty()) {
        return false;
    }

    QBuffer compressedBuffer(&baCompressed);
    compressedBuffer.open(QIODevice::ReadOnly);

    QByteArray baPropByte;
    baPropByte.append((char)nLZMA2PropsByte);

    bool bDecompressResult = false;

    if (bHasBCJX86) {
        // Decompress LZMA2 to intermediate buffer, then apply BCJ x86 reverse
        QByteArray baIntermediate;
        QBuffer intermediateBuffer(&baIntermediate);
        intermediateBuffer.open(QIODevice::WriteOnly);

        XBinary::DATAPROCESS_STATE lzma2State = {};
        lzma2State.pDeviceInput = &compressedBuffer;
        lzma2State.pDeviceOutput = &intermediateBuffer;
        lzma2State.nInputOffset = 0;
        lzma2State.nInputLimit = baCompressed.size();
        lzma2State.nProcessedOffset = 0;
        lzma2State.nProcessedLimit = -1;

        bDecompressResult = XLZMADecoder::decompressLZMA2(&lzma2State, baPropByte, pPdStruct);
        intermediateBuffer.close();

        if (bDecompressResult) {
            _applyBCJX86Decode(baIntermediate);

            qint64 nWriteFrom = pDecompressState->nProcessedOffset;
            qint64 nWriteLimit = pDecompressState->nProcessedLimit;
            if (nWriteLimit == -1) {
                nWriteLimit = baIntermediate.size();
            }
            qint64 nToWrite = qMin(nWriteLimit, (qint64)(baIntermediate.size() - nWriteFrom));
            if (nToWrite > 0 && nWriteFrom >= 0) {
                pDecompressState->pDeviceOutput->seek(0);
                pDecompressState->pDeviceOutput->write(baIntermediate.constData() + nWriteFrom, nToWrite);
                pDecompressState->nCountOutput = nToWrite;
            }
        }
    } else {
        // Pure LZMA2 — stream directly to output device
        XBinary::DATAPROCESS_STATE lzma2State = {};
        lzma2State.pDeviceInput = &compressedBuffer;
        lzma2State.pDeviceOutput = pDecompressState->pDeviceOutput;
        lzma2State.nInputOffset = 0;
        lzma2State.nInputLimit = baCompressed.size();
        lzma2State.nProcessedOffset = pDecompressState->nProcessedOffset;
        lzma2State.nProcessedLimit = pDecompressState->nProcessedLimit;

        bDecompressResult = XLZMADecoder::decompressLZMA2(&lzma2State, baPropByte, pPdStruct);

        if (bDecompressResult) {
            pDecompressState->nCountInput = lzma2State.nCountInput;
            pDecompressState->nCountOutput = lzma2State.nCountOutput;
        }
    }

    compressedBuffer.close();

    return bDecompressResult;
}
