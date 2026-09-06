/* Copyright (c) 2026 hors<horsicq@gmail.com>
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
#ifndef XNPACKDECODER_H
#define XNPACKDECODER_H

#include "xbinary.h"

/*--
   NPack ("MSTSM") payload decoder.

   NPack is the compressed-file format of the Symantec / Norton installer
   (Norton Desktop for Windows, Norton AntiVirus, Norton Utilities for Windows,
   1993-1995): every file on the install disks is stored as one MSTSM container
   whose name has its last character replaced by '$' (NDWCLOSE.WB$, VNAVD.38$).

   The container is a 5-byte ASCII magic and NOTHING else - no size, no name, no
   checksum, no method byte.  Everything after byte 5 is a single Stac LZS
   ("Stacker") block, MSB-first, exactly one block per file:

     0 <8 bits>                 literal byte
     1 0 <11 bits>              match, distance = code (code 0 is invalid)
     1 1 <7 bits>               match, distance = code; code 0 is the STOP code
     match length, prefix code: 00 -> 2   01 -> 3   10 -> 4
                                1100 -> 5 1101 -> 6 1110 -> 7
                                1111 -> 8 + sum of 4-bit groups, the last group
                                          being the first one below 15
     history window 2048 bytes, zero-filled at the start of the block

   Verified byte-exact against the reference extractor on all 141 corpus files.
   Measured over that corpus: the block always terminates with the stop code in
   the FINAL byte of the file (zero trailing bytes after re-aligning), the
   largest distance seen is 1942, the largest match length 60466 (a match may
   legitimately be far longer than the window), and no match ever reaches back
   past the start of the output.  The last property is what lets probeStream()
   reject a bogus stream cheaply: a real NPack file never reads the zero
   pre-fill of the window, so a match whose distance exceeds the bytes produced
   so far is treated as a structural error here even though the reference
   decoder would quietly return zeroes.

   The unpacked size is not recorded anywhere, so it is only knowable by
   decoding; decompress() therefore streams its output and never preallocates.
--*/
class XNPackDecoder {
public:
    // "MSTSM".  The stream handed to decompress() starts AFTER this.
    static const qint64 NPACK_MAGIC_SIZE = 5;
    static const qint32 NPACK_WINDOW_SIZE = 2048;

    struct PROBE_RESULT {
        qint64 nConsumed;   // payload bytes consumed, rounded up to a byte boundary
        qint64 nProduced;   // plaintext bytes the walk accounted for
        bool bStopCode;     // the block's stop code was reached
        bool bOutputCapped; // the walk stopped on nMaxOutput, not on the data
    };

    // Structure-only walk of an LZS block: it tracks sizes, not bytes, because
    // nothing in the bit grammar depends on the plaintext.  Returns false on a
    // structural error (invalid 11-bit distance 0, a match reaching back past
    // the start of the output, or the input ending mid-token).
    static bool probeStream(const char *pPayload, qint64 nPayloadSize, qint64 nMaxOutput, PROBE_RESULT *pResult);

    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XNPACKDECODER_H
