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
#include "xascii85decoder.h"

// Internal helpers for ASCII85 decoding
namespace {
static int _ascii85_readByte(XBinary::DECOMPRESS_STATE *st)
{
    char c;
    qint64 r = st->pDeviceInput->read(&c, 1);
    if (r != 1) {
        st->bReadError = true;
        return -1;
    }
    st->nCountInput++;
    return (unsigned char)c;
}

static void _ascii85_writeBytes(XBinary::DECOMPRESS_STATE *st, const unsigned char *buf, int n)
{
    if (n > 0) XBinary::_writeDevice((char *)buf, n, st);
}
}  // namespace

XASCII85Decoder::XASCII85Decoder(QObject *parent) : QObject(parent)
{
}

// ASCII85 (Adobe variant) decoder for PDF streams.
// Supports optional PostScript style start marker "<~" (ignored if present) and terminates on standard '~>'.
// Implements zero tuple shortcut 'z', whitespace skipping, and partial tuple handling per Adobe spec.
// Robustness: ignores malformed characters outside valid range, guards against accumulator overflow, and honors cancellation.
bool XASCII85Decoder::decompress_pdf(XBinary::DECOMPRESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) return false;

    if (pDecompressState->nInputOffset > 0) pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    if (pDecompressState->pDeviceOutput) pDecompressState->pDeviceOutput->seek(0);

    // (Moved helper lambdas to file-scope static functions above.)

    // Detect optional opening marker <~ (Adobe style) but it's not required in PDF.
    // Peek first two bytes if at beginning of provided range.
    if (pDecompressState->nCountInput == 0 && pDecompressState->nInputLimit != 0) {
        qint64 pos = pDecompressState->pDeviceInput->pos();
        char marker[2];
        qint64 r = pDecompressState->pDeviceInput->read(marker, 2);
        if (r == 2) {
            if (!(marker[0] == '<' && marker[1] == '~')) {
                // Not a marker: rewind to allow normal processing
                pDecompressState->pDeviceInput->seek(pos);
            } else {
                pDecompressState->nCountInput += 2;
            }
        } else {
            if (r > 0) {
                // Not enough data
                pDecompressState->pDeviceInput->seek(pos);
            }
        }
    }

    unsigned char tuple[4];
    quint64 accum = 0;  // Use 64-bit to safely detect overflow (> 0xFFFFFFFF)
    int count = 0;      // Number of collected base85 digits (0..5)
    bool end = false;

    while (!end && !pDecompressState->bReadError && XBinary::isPdStructNotCanceled(pPdStruct) &&
           (pDecompressState->nInputLimit < 0 || pDecompressState->nCountInput < pDecompressState->nInputLimit)) {
        int ch = _ascii85_readByte(pDecompressState);
        if (ch < 0) break;  // read error or EOF
        unsigned char c = (unsigned char)ch;

        // Whitespace (space, tab, CR, LF, formfeed) are ignored
        if (c <= ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f') {
            continue;
        }

        if (c == '~') {  // EOD marker start
            int c2 = _ascii85_readByte(pDecompressState);
            if (c2 >= 0 && (unsigned char)c2 == '>') {
                end = true;
                break;
            } else {
                // Malformed end marker; treat as end anyway.
                end = true;
                break;
            }
        } else if (c == 'z') {  // Shortcut for 0x00000000, only allowed when no digits collected
            if (count != 0) {
                // Invalid placement; ignore per robustness principle.
                continue;
            }
            unsigned char zeros[4] = {0, 0, 0, 0};
            _ascii85_writeBytes(pDecompressState, zeros, 4);
            continue;
        } else if (c < '!' || c > 'u') {
            // Out of range character – skip (could set error flag, but being liberal aids damaged PDFs)
            continue;
        } else {
            // Accumulate digit
            accum = accum * 85 + (quint32)(c - '!');
            count++;
            if (count == 5) {
                if (accum > 0xFFFFFFFFULL) {  // Overflow shouldn't happen for valid data
                    pDecompressState->bReadError = true;
                    break;
                }
                tuple[0] = (unsigned char)((accum >> 24) & 0xFF);
                tuple[1] = (unsigned char)((accum >> 16) & 0xFF);
                tuple[2] = (unsigned char)((accum >> 8) & 0xFF);
                tuple[3] = (unsigned char)(accum & 0xFF);
                _ascii85_writeBytes(pDecompressState, tuple, 4);
                accum = 0;
                count = 0;
            }
        }
    }

    // Handle partial tuple at EOF/EOD: count 2..4 digits => produce (count - 1) bytes.
    if (!pDecompressState->bWriteError && !pDecompressState->bReadError) {
        if (count == 1) {
            // Single leftover digit is invalid – ignore.
        } else if (count > 1) {
            for (int i = count; i < 5; ++i) accum = accum * 85 + 84;  // Pad with 'u'
            if (accum > 0xFFFFFFFFULL) {
                pDecompressState->bReadError = true;
            } else {
                unsigned char tail[4];
                tail[0] = (unsigned char)((accum >> 24) & 0xFF);
                tail[1] = (unsigned char)((accum >> 16) & 0xFF);
                tail[2] = (unsigned char)((accum >> 8) & 0xFF);
                tail[3] = (unsigned char)(accum & 0xFF);
                _ascii85_writeBytes(pDecompressState, tail, count - 1);
            }
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    return !(pDecompressState->bReadError || pDecompressState->bWriteError);
}
