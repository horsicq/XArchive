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
#include "xasciihexdecoder.h"
#include "algo_utils.h"

namespace {
// Read one input byte, bounded by nInputLimit. Returns -1 on limit/EOF WITHOUT flagging a read error,
// because an ASCIIHex stream may legally end at EOF (no '>' marker) in damaged/streamed PDFs.
int hexReadByte(XBinary::DATAPROCESS_STATE *pState)
{
    if ((pState->nInputLimit >= 0) && (pState->nCountInput >= pState->nInputLimit)) {
        return -1;
    }

    char c = 0;
    qint64 nRead = pState->pDeviceInput->read(&c, 1);
    if (nRead != 1) {
        return -1;
    }

    pState->nCountInput++;

    return static_cast<unsigned char>(c);
}

int hexDigitValue(unsigned char c)
{
    if ((c >= '0') && (c <= '9')) return c - '0';
    if ((c >= 'A') && (c <= 'F')) return (c - 'A') + 10;
    if ((c >= 'a') && (c <= 'f')) return (c - 'a') + 10;
    return -1;
}
}  // namespace

XASCIIHexDecoder::XASCIIHexDecoder(QObject *parent) : QObject(parent)
{
}

// ASCIIHexDecode (PDF 7.4.2): hex digits, whitespace ignored, terminated by '>'. Each pair of digits
// yields one byte; a trailing odd digit is treated as if followed by '0'. Liberal on malformed input.
bool XASCIIHexDecoder::decompress_pdf(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput ||
        (pDecompressState->nInputOffset < 0) || (pDecompressState->nInputLimit < -1)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    bool bHaveHigh = false;
    int nHigh = 0;
    bool bEnd = false;

    while (!bEnd && !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const int nCh = hexReadByte(pDecompressState);
        if (nCh < 0) {
            break;  // EOF / input limit
        }

        const unsigned char c = static_cast<unsigned char>(nCh);

        if (c == '>') {  // EOD marker
            bEnd = true;
            break;
        }

        // Whitespace (NUL, TAB, LF, FF, CR, SP) is ignored per spec.
        if ((c == 0) || (c == '\t') || (c == '\n') || (c == '\f') || (c == '\r') || (c == ' ')) {
            continue;
        }

        const int nValue = hexDigitValue(c);
        if (nValue < 0) {
            continue;  // ignore stray non-hex bytes (be liberal with damaged PDFs)
        }

        if (!bHaveHigh) {
            nHigh = nValue;
            bHaveHigh = true;
        } else {
            unsigned char nByte = static_cast<unsigned char>((nHigh << 4) | nValue);
            if (XBinary::_writeDevice(reinterpret_cast<char *>(&nByte), 1, pDecompressState) != 1) {
                return false;
            }
            bHaveHigh = false;
        }
    }

    // A dangling high nibble at EOD/EOF is completed with an implicit low nibble of 0.
    if (bHaveHigh && !pDecompressState->bWriteError && !pDecompressState->bReadError &&
        XBinary::isPdStructNotCanceled(pPdStruct)) {
        unsigned char nByte = static_cast<unsigned char>(nHigh << 4);
        if (XBinary::_writeDevice(reinterpret_cast<char *>(&nByte), 1, pDecompressState) != 1) {
            return false;
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    return !(pDecompressState->bReadError || pDecompressState->bWriteError);
}
