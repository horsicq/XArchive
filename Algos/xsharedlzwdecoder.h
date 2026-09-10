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
#ifndef XSHAREDLZWDECODER_H
#define XSHAREDLZWDECODER_H

#include "../xbinary.h"

// The parameterised LZW that several 1980s/1990s containers share, in the exact
// shape the reference implementation uses.  Two details are load bearing and
// neither is guessable from a textbook description:
//
//  * THE BLOCK PADDING IS COUNTED IN CODES, NOT BITS.  When the code width
//    steps up, and again when a clear code arrives, the reader discards
//    whatever is left of the current group of eight codes at the width in
//    force, and then RESETS the code counter.  Padding relative to an absolute
//    bit position instead - the usual way to describe unix compress - desyncs
//    as soon as a stream pads twice.
//  * KwKwK APPENDS THE PREVIOUS PHRASE FIRST BYTE, not the low byte of the
//    previous code.  The two agree whenever the previous code was a literal,
//    which is why the mistake survives most small members and only shows up
//    once a stream is large enough to repeat a multi-byte phrase.
//
// bUnRle90 runs the decoded bytes through the ARC RLE90 filter (0x90 escape),
// which is how "crunched" members are stored.  Note the filter does NOT adopt
// an escaped literal 0x90 as the new run byte.
class XSharedLZWDecoder {
public:
    struct OPTIONS {
        qint32 nMaxBits;      // final code width, 9..16
        bool bHasClearCode;   // code 0x100 restarts the dictionary
        bool bHasEndCode;     // code 0x101 ends the stream
        bool bMsbFirst;       // bit order of the code reader
        bool bUnRle90;        // pipe the output through the ARC RLE90 filter
        bool bBlockPadding;   // pad to a group of eight codes on width change / clear
        // Bias on the free-slot count that triggers the width step.  Zero is
        // the usual "step when the next free slot no longer fits"; one steps a
        // single code earlier, which is what the CMP framing uses.  Getting
        // this wrong desyncs only after the first 512-code boundary, so short
        // members still decode and the bug hides.
        qint32 nWidthStepBias;

        OPTIONS()
            : nMaxBits(12), bHasClearCode(true), bHasEndCode(false), bMsbFirst(false), bUnRle90(false), bBlockPadding(false), nWidthStepBias(0)
        {
        }
    };

    static bool decode(const QByteArray &baPacked, const OPTIONS &options, qint64 nUncompressedSize, QByteArray *pbaResult,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);

    // ARC RLE90 on its own - ArcFS stores "packed" members this way.
    static bool unRle90(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult);
};

#endif  // XSHAREDLZWDECODER_H
