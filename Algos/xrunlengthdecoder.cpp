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
#include "xrunlengthdecoder.h"
#include "algo_utils.h"

namespace {
// Read one input byte, bounded by nInputLimit. RunLengthDecode has an explicit
// EOD byte, so reaching the boundary while a byte is requested is malformed.
int rleReadByte(XBinary::DATAPROCESS_STATE *pState)
{
    if ((pState->nInputLimit >= 0) && (pState->nCountInput >= pState->nInputLimit)) {
        pState->bReadError = true;
        return -1;
    }

    char c = 0;
    qint64 nRead = pState->pDeviceInput->read(&c, 1);
    if (nRead != 1) {
        pState->bReadError = true;
        return -1;
    }

    pState->nCountInput++;

    return static_cast<unsigned char>(c);
}
}  // namespace

XRunLengthDecoder::XRunLengthDecoder(QObject *parent) : QObject(parent)
{
}

// RunLengthDecode (PDF 7.4.5), a PackBits variant. Each length byte L governs the next run:
//   0..127  -> copy the following L+1 bytes literally.
//   129..255-> repeat the single following byte (257 - L) times.
//   128     -> EOD.
bool XRunLengthDecoder::decompress_pdf(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    bool bEnd = false;

    while (!bEnd && !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const int nLength = rleReadByte(pDecompressState);
        if (nLength < 0) {
            break;  // EOF / input limit
        }

        if (nLength == 128) {  // EOD
            bEnd = true;
            break;
        }

        if (nLength < 128) {
            // Literal run of (nLength + 1) bytes.
            const int nCount = nLength + 1;
            for (int i = 0; (i < nCount) && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct); ++i) {
                const int nByte = rleReadByte(pDecompressState);
                if (nByte < 0) {
                    break;
                }
                unsigned char c = static_cast<unsigned char>(nByte);
                if (XBinary::_writeDevice(reinterpret_cast<char *>(&c), 1, pDecompressState) != 1) return false;
            }
        } else {
            // Repeat run: the next byte is emitted (257 - nLength) times.
            const int nCount = 257 - nLength;
            const int nByte = rleReadByte(pDecompressState);
            if (nByte < 0) {
                break;
            }
            unsigned char c = static_cast<unsigned char>(nByte);
            for (int i = 0; (i < nCount) && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct); ++i) {
                if (XBinary::_writeDevice(reinterpret_cast<char *>(&c), 1, pDecompressState) != 1) return false;
            }
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    return bEnd && !(pDecompressState->bReadError || pDecompressState->bWriteError);
}
