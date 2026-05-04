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
#include "algo_utils.h"
#include <QBuffer>

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
    ret = LzmaDec_Allocate(&state, (Byte *)properties, nPropSize, Algo_utils::lzmaAlloc());

    if (ret != 0) {  // S_OK
        return false;
    }

    LzmaDec_Init(&state);
    bool bResult = Algo_utils::decompressLZMA(&state, pDecompressState, pPdStruct);
    LzmaDec_Free(&state, Algo_utils::lzmaAlloc());

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

    pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    pDecompressState->pDeviceOutput->seek(0);

    CLzmaDec state = {};
    SRes ret = LzmaProps_Decode(&state.prop, (Byte *)baProperty.constData(), baProperty.size());

    if (ret != 0) {
        qDebug("[LZMA] LzmaProps_Decode FAILED: %d", ret);
        return false;
    }

    LzmaDec_Construct(&state);
    ret = LzmaDec_Allocate(&state, (Byte *)baProperty.constData(), baProperty.size(), Algo_utils::lzmaAlloc());

    if (ret != 0) {
        qDebug("[LZMA] LzmaDec_Allocate FAILED: %d", ret);
        return false;
    }

    LzmaDec_Init(&state);
    bool bResult = Algo_utils::decompressLZMA(&state, pDecompressState, pPdStruct);
    LzmaDec_Free(&state, Algo_utils::lzmaAlloc());

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
    SRes ret = Lzma2Dec_Allocate(&state, (Byte)propByte, Algo_utils::lzmaAlloc());

    if (ret != 0) {  // S_OK
        return false;
    }

    Lzma2Dec_Init(&state);
    bool bResult = Algo_utils::decompressLZMA2(&state, pDecompressState, pPdStruct);
    Lzma2Dec_Free(&state, Algo_utils::lzmaAlloc());

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
    SRes ret = Lzma2Dec_Allocate(&state, (Byte)baProperty[0], Algo_utils::lzmaAlloc());

    if (ret != 0) {  // S_OK
        return false;
    }

    Lzma2Dec_Init(&state);
    bool bResult = Algo_utils::decompressLZMA2(&state, pDecompressState, pPdStruct);
    Lzma2Dec_Free(&state, Algo_utils::lzmaAlloc());

    return bResult;
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
        if (!Algo_utils::xzReadVarInt(baBH, nBHPos, nCompressedSize64)) {
            return false;
        }
    }
    if (bHasUncompressedSize) {
        if (!Algo_utils::xzReadVarInt(baBH, nBHPos, nUncompressedSize64)) {
            return false;
        }
    }

    // Parse filter chain: find LZMA2 props byte and optional BCJ x86 filter
    bool bHasBCJX86 = false;
    quint8 nLZMA2PropsByte = 0;

    for (qint32 nFilter = 0; nFilter < nNumFilters && nFilter < 4; nFilter++) {
        quint64 nFilterID = 0;
        quint64 nPropSize = 0;
        if (!Algo_utils::xzReadVarInt(baBH, nBHPos, nFilterID)) {
            return false;
        }
        if (!Algo_utils::xzReadVarInt(baBH, nBHPos, nPropSize)) {
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
            Algo_utils::applyBCJX86Decode(baIntermediate);

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
