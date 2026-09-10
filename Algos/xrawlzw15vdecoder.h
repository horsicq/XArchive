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
#ifndef XRAWLZW15VDECODER_H
#define XRAWLZW15VDECODER_H

#include <QByteArray>
#include <QtGlobal>

// "LZW15V" - Mark Nelson's variable-width LZW (9..15 bits) as it appears as a
// COMPLETELY HEADERLESS stream in DOS-era truncated-extension install files
// (.EX_, .DL_, .WO_, .HL_, .DA_, .SY_).  There is no magic, no size field, no
// checksum: the file IS the code stream and it starts with the first 9-bit
// code.
//
// BIT ORDER IS MSB-FIRST.  A byte is consumed whole and its bits are taken from
// bit 7 downwards; a code's first-read bit is its MOST significant one.
//
// Control codes (this is the part that differs between LZW dialects):
//
//   0x100  END      stop; this is the only end-of-stream signal
//   0x101  BUMP     widen the code width by one, capped at 15 bits
//   0x102  CLEAR    reset the dictionary: next code goes back to 0x103 and the
//                   width back to 9, and the code that FOLLOWS is a fresh
//                   9-bit literal, not an ordinary dictionary lookup
//
// The first assignable code is 0x103 and the table stops growing at 0x8000.
// Note the consequence of BUMP being explicit: the width never changes on its
// own, so a decoder that widens when nextCode hits (1 << width) - the habit of
// the Unix compress / GIF dialects - desynchronises immediately here.
//
// The stream opens with a bare 9-bit code that is emitted as a literal byte and
// becomes the initial previous-code; every CLEAR restarts that same opening.
//
// Reference: the reference implementation handler A141 ("Raw LZW15V", class xna, VMT 0x0053e8a8),
// whose worker calls the codec. The reference implementation itself detects the
// format with an 11-entry whitelist of literal 16-byte file prefixes, which is
// pure overfitting to its own corpus; XRawLzw15v instead gates on a strict full
// trial decode - see probe() below.
class XRawLzw15vDecoder {
public:
    enum {
        CODE_END = 0x100,
        CODE_BUMP = 0x101,
        CODE_CLEAR = 0x102,
        FIRST_CODE = 0x103,
        MAX_CODES = 0x8000,
        MIN_CODE_BITS = 9,
        MAX_CODE_BITS = 15
    };

    // Whole-stream decode, faithful to the original: no grammar checks beyond
    // memory safety, exactly as the reference decoder behaves.  Succeeds only
    // when exactly nUncompressedSize bytes come out.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaUnpacked);

    // Strict trial decode used for DETECTION of a headerless stream.  On top of
    // the plain decode it requires: the opening code is a literal (< 0x100),
    // BUMP never fires past 15 bits, a new code is never more than one past the
    // next assignable one, the next assignable code never outruns the current
    // width, the stream ends on an explicit 0x100, and every input byte is
    // consumed.  Writes the decoded length to pnOutputSize.
    static bool probe(const QByteArray &baPacked, qint64 nMaxOutput, qint64 *pnOutputSize);
};

#endif  // XRAWLZW15VDECODER_H
