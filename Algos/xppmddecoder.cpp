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
#include "xppmddecoder.h"
#include "xppmdrangedecoder.h"
#include "xppmdmodel.h"
#include "xppmd7model.h"

XPPMdDecoder::XPPMdDecoder(QObject *pParent) : QObject(pParent)
{
}

bool XPPMdDecoder::decompressPPMD8(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    QIODevice *pSourceDevice = pDecompressState->pDeviceInput;
    QIODevice *pDestDevice = pDecompressState->pDeviceOutput;

    // Read PPMd parameters (2 bytes for ZIP method 98)
    quint8 nParamByte1 = 0;
    quint8 nParamByte2 = 0;

    if (pSourceDevice->read((char *)&nParamByte1, 1) != 1) {
        return false;
    }

    if (pSourceDevice->read((char *)&nParamByte2, 1) != 1) {
        return false;
    }

    pDecompressState->nCountInput += 2;

    // Extract parameters from 2-byte header
    // The encoding format (from 7-Zip PpmdZip.cpp):
    // val = (Order - 1) + ((MemSizeMB - 1) << 4) + (Restor << 12)
    // Stored as 2 bytes little-endian
    quint16 nVal = nParamByte1 | (nParamByte2 << 8);
    quint8 nOrder = (nVal & 0x0F) + 1;             // Bits 0-3: Order - 1
    quint8 nMemSizeMB = ((nVal >> 4) & 0xFF) + 1;  // Bits 4-11: MemSizeMB - 1
    quint8 nRestor = (nVal >> 12);                 // Bits 12-15: Restor method

    quint32 nMemSize = ((quint32)nMemSizeMB) << 20;  // Memory in bytes

    // Validate parameters (matching PpmdZip.cpp from 7-Zip)
    if ((nOrder < XPPMdModel::MIN_ORDER) || (nOrder > XPPMdModel::MAX_ORDER)) {
        return false;
    }

    if (nRestor > 2) {
        return false;
    }

    // Initialize PPMd8 decoder using wrapper classes
    XPPMdModel model;

    if (!model.allocate(nMemSize)) {
        return false;
    }

    // Initialize 7-Zip's internal range decoder (hybrid solution)
    model.setInputStream(pSourceDevice);  // Set input stream for 7-Zip's internal decoder
    model.init(nOrder, nRestor);

    // Decompress
    const qint32 N_BUFFER_SIZE = 0x4000;
    char sBufferOut[N_BUFFER_SIZE];

    qint64 nUncompressedSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, -1).toLongLong();

    qint64 nDecompressed = 0;
    bool bSuccess = true;
    bool bEndOfStream = false;
    qint32 nIterations = 0;

    while (XBinary::isPdStructNotCanceled(pPdStruct) && !bEndOfStream) {
        qint32 nToDecompress = N_BUFFER_SIZE;

        // Limit to remaining size if known
        if (nUncompressedSize > 0) {
            qint64 nRemaining = nUncompressedSize - nDecompressed;
            if (nRemaining <= 0) {
                break;
            }
            nToDecompress = qMin((qint64)nToDecompress, nRemaining);
        }

        // Decompress chunk
        qint32 nActual = 0;
        for (qint32 i = 0; i < nToDecompress && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
            int nSymbol = model.decodeSymbol();

            if (nSymbol < 0) {
                // End of stream or error
                if (nUncompressedSize > 0 && nDecompressed < nUncompressedSize) {
                    bSuccess = false;
                }
                bEndOfStream = true;
                break;
            }

            sBufferOut[nActual++] = (char)nSymbol;
        }

        if (nActual > 0) {
            if (!XBinary::_writeDevice(sBufferOut, nActual, pDecompressState)) {
                bSuccess = false;
                break;
            }

            nDecompressed += nActual;
        }

        nIterations++;

        // Only break if we got fewer bytes than requested AND we hit the end of stream
        // Don't break just because we filled the buffer
        if (nActual < nToDecompress && bEndOfStream) {
            break;
        }
    }

    // Cleanup
    model.free();

    // Check if we got the expected amount of data
    if (nUncompressedSize > 0) {
        if (nDecompressed != nUncompressedSize) {
            return false;
        }
        // If we got the exact expected size, it's a success
        return true;
    }

    // If size is unknown, check if we successfully decoded something and there was no write error
    return bSuccess && (nDecompressed > 0) && !pDecompressState->bWriteError;
}

bool XPPMdDecoder::decompressPPMD7(XBinary::DATAPROCESS_STATE *pDecompressState, const QByteArray &baProperty, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    // 7z PPMd format: 5 bytes (order, mem0-3 little-endian)
    if (baProperty.size() != 5) {
        return false;
    }

    // Extract PPMd parameters from 5-byte property
    quint8 nOrder = (quint8)baProperty[0];
    quint32 nMemSize =
        ((quint8)baProperty[1]) | (((quint32)(quint8)baProperty[2]) << 8) | (((quint32)(quint8)baProperty[3]) << 16) | (((quint32)(quint8)baProperty[4]) << 24);

    // Validate parameters
    if ((nOrder < XPPMd7Model::MIN_ORDER) || (nOrder > XPPMd7Model::MAX_ORDER)) {
        return false;
    }

    if (nMemSize == 0) {
        return false;
    }

    // Initialize Ppmd7 model (PPMdH variant used by 7z)
    XPPMd7Model model;

    if (!model.allocate(nMemSize)) {
        return false;
    }

    pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    pDecompressState->pDeviceOutput->seek(0);

    // Set input stream to compressed data
    model.setInputStream(pDecompressState->pDeviceInput);
    model.init(nOrder);  // PPMdH (Ppmd7) only takes order parameter

    // Decompress symbol by symbol
    const qint32 N_BUFFER_SIZE = 0x4000;
    char sBufferOut[N_BUFFER_SIZE];

    qint64 nUncompressedSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, -1).toLongLong();
    qint64 nDecompressed = 0;
    bool bResult = true;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint32 nActual = 0;

        // Decode buffer
        for (qint32 i = 0; i < N_BUFFER_SIZE && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
            if (nUncompressedSize > 0 && nDecompressed >= nUncompressedSize) {
                break;  // Reached expected size
            }

            qint32 nSymbol = model.decodeSymbol();

            if (nSymbol < 0) {
                // End of stream or error
                if (nUncompressedSize > 0 && nDecompressed < nUncompressedSize) {
                    bResult = false;  // Unexpected end
                }
                break;
            }

            sBufferOut[nActual++] = (char)nSymbol;
            nDecompressed++;
        }

        // Write decoded data
        if (nActual > 0) {
            if (!XBinary::_writeDevice(sBufferOut, nActual, pDecompressState)) {
                bResult = false;
                break;
            }
        } else {
            // No more data
            break;
        }
    }

    model.free();

    // Verify size if known
    if (nUncompressedSize > 0) {
        if (nDecompressed != nUncompressedSize) {
            return false;
        }
        return true;
    }

    // If size is unknown, check if we successfully decoded something and there was no write error
    return bResult && (nDecompressed > 0) && !pDecompressState->bWriteError;
}
