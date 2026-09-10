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
#ifndef XLZHUFDECODER_H
#define XLZHUFDECODER_H

#include "../xbinary.h"

// LZHUF - Haruyasu Yoshizaki's LZSS over an adaptive (dynamic) Huffman tree -
// in the parameterised shape the reference implementation uses, so that the
// families that embed it can share one decoder instead of one copy each.
//
// The common skeleton is always the same:
//
//   * bits are consumed MSB first from a 16-bit window of a 32-bit shift
//     register; getByte() takes the top eight;
//   * the alphabet is 0x00..0xFF for literals plus one symbol per match length;
//     the tree starts with every leaf at frequency one and is re-weighted after
//     every symbol, bubbling the node up past every lighter node;
//   * a match distance is a byte run through LHA's classic position tables
//     d_code[256] / d_len[16], plus d_len[i>>4]-k further raw bits.
//
// What the embedders disagree about is small, undocumented, and each difference
// silently produces plausible-looking garbage rather than an error, so every one
// of them is an OPTIONS field rather than a hardcoded constant:
//
//  * MATCH LENGTH BIAS.  The textbook rule is length = code - 0xFF + THRESHOLD.
//    Zoom does NOT add THRESHOLD: its length is exactly code - 255, so the first
//    match symbol means a one-byte match, not a three-byte one.  Getting this
//    wrong costs two bytes on the very first match and then desynchronises
//    everything after it, which reads as "the format is almost right".
//  * DISTANCE BIAS.  LZSS normally copies from (r - dist - 1), because the
//    encoder stored the position of the byte before the match.  Zoom uses
//    (r - dist) with NO -1 adjustment; ZTC uses the -1.  A one-byte skew in the
//    window is invisible for runs of equal bytes and destroys everything else.
//  * WINDOW SIZE AND PRESET.  ZTC rides a 0x2000-byte ring masked with 0x1FFF
//    (deliberately wider than the encoder's 4 KiB window, which is harmless)
//    preset to 0x20; Zoom uses a 0x1000-byte ring preset to 0x00.  The preset
//    only shows through in the first few hundred bytes of a member, so a wrong
//    preset corrupts the head of a file and nothing else.
//  * END SYMBOL.  ZTC has none and is driven purely by the stored uncompressed
//    size; Zoom terminates on symbol 0x13C, the top of its alphabet.  A third
//    variant (nEOFCode 0x100 with bShiftAboveEOF) terminates on 0x100 and shifts
//    every higher symbol down by one.
//  * ALPHABET SIZE.  Derivable as 0x100 - (THRESHOLD - F) [+1 for an end symbol]
//    for the reference-implementation variants, but Zoom's 317 does not follow
//    from its F/THRESHOLD, so nNChar is stored explicitly and never derived
//    behind the caller's back.
//  * MAX_FREQ BEHAVIOUR.  The textbook decoder halves every weight and rebuilds
//    the tree; Zoom's simply stops updating from then on.  Neither is reached by
//    the current corpora, but they are not interchangeable.
class XLZHUFDecoder {
public:
    struct OPTIONS {
        // 0 -> position = d_code[i]<<5 | (x & 0x1F), extra = d_len[i>>4]-3  (2 KiB)
        // 1 -> position = d_code[i]<<6 | (x & 0x3F), extra = d_len[i>>4]-2  (4 KiB)
        // 2 -> position = d_code[i]<<7 | (x & 0x7F), extra = d_len[i>>4]-1  (8 KiB)
        qint32 nDistVariant;
        qint32 nNChar;        // alphabet size; T = 2*nNChar-1, R = T-1
        qint32 nRingSize;     // power of two; positions are masked with nRingSize-1
        qint32 nRingFill;     // byte the ring starts filled with (0x20 or 0x00)
        qint32 nMaxFreq;      // 0x8000 or 0xD000
        qint32 nEOFCode;      // symbol that ends the stream, -1 for none
        bool bShiftAboveEOF;  // every symbol above nEOFCode moves down by one
        qint32 nLengthBias;   // length = symbol - 0xFF + nLengthBias (THRESHOLD, or 0)
        qint32 nMatchBias;    // source = (r - distance - nMatchBias) & mask
        bool bReconstruct;    // at nMaxFreq: halve and rebuild, else stop updating

        OPTIONS()
            : nDistVariant(1), nNChar(314), nRingSize(0x2000), nRingFill(0x20), nMaxFreq(0x8000), nEOFCode(-1), bShiftAboveEOF(false), nLengthBias(2),
              nMatchBias(1), bReconstruct(true)
        {
        }
    };

    // ZTC (Zortech C / Symantec C++ distribution archive) members: distance
    // variant 1, F = 60, THRESHOLD = 2, no end symbol, MAX_FREQ 0x8000, ring
    // 0x2000 preset to 0x20, N_CHAR 314 (T = 627, R = 626).
    static OPTIONS getZTCOptions();

    // Zoom (Amiga floppy imager) chunks: verbatim Yoshizaki LZHUF over a 4096
    // byte zero-filled ring, N_CHAR 317 (T = 633, R = 632), end symbol 0x13C,
    // length = symbol - 255 and distance used as-is.  Feed it the chunk payload
    // and the record's "length after the LZHUF stage" field.
    static OPTIONS getZoomOptions();

    // The reference implementation's own setup call, in its argument order, for
    // the other families that share this codec.  f_sel: 0->F=0x20, 1->F=0x3C,
    // 2->F=0x3D, else F=0x5A.  thr_sel: 1->THRESHOLD 3, else 2.  bHasEOF adds
    // one symbol and makes 0x100 the terminator with everything above it shifted
    // down.  bBigFreq picks MAX_FREQ 0xD000 over 0x8000.  bFillZero presets the
    // ring to 0x00 instead of 0x20.  The ARC RLE90 post filter that call also
    // carries is NOT applied here - run XSharedLZWDecoder::unRle90 on the result.
    static OPTIONS getOptions(qint32 nDistVariant, qint32 nFSel, qint32 nThrSel, bool bHasEOF, bool bBigFreq, bool bFillZero);

    // Returns false unless exactly nUncompressedSize bytes came out.  Without an
    // end symbol the decoder is driven by nUncompressedSize and a final match is
    // truncated to fit; with one, overrunning nUncompressedSize is an error.
    static bool decode(const QByteArray &baPacked, const OPTIONS &options, qint64 nUncompressedSize, QByteArray *pbaResult,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XLZHUFDECODER_H
