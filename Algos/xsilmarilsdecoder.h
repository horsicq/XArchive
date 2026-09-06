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
#ifndef XSILMARILSDECODER_H
#define XSILMARILSDECODER_H

#include <QByteArray>
#include <QtGlobal>

// Silmarils (Ishar / Robinson's Requiem / Transarctica era) game-resource
// container, byte-run method - the one selected by the container's flag byte
// 0x81.  Reverse engineered from the 176-file ARC4 Silmarils_FT corpus; no
// existing HANDLE_METHOD decodes it (checked against RUNLENGTH/PDF,
// COMPACT_PRO_RLE, ARC_PACK, EMT_RLE and SPIS_RLE, all of which use a
// different escape convention).
//
// The stream is a flat sequence of two token shapes, byte aligned, with no
// end marker - it simply runs to the declared plaintext size:
//
//   c  < 0x80 : literal run  - c raw bytes follow and are copied out
//   c >= 0x80 : byte run     - the NEXT byte is repeated (c & 0x7f) times
//
// Note the counts are exact: neither token has the "+1" bias that PackBits and
// PDF /RunLengthDecode use, and 0x80 (a zero-length run) never occurs.
//
// IMPORTANT - the declared size is six bytes larger than the plaintext.  The
// container's 24-bit size field counts the six-byte header as well, so the
// caller must pass nUncompressedSize = storedSize - 6.  Evidence: over the 41
// byte-run members of the corpus, literals + runs came to storedSize - 6 in
// every single file (never storedSize, never a varying delta), all 41 resulting
// lengths are exact multiples of 8 (the resource unit of this data) whereas the
// storedSize values are not, and for the four members that happen to be stored
// as pure literal runs the encoder emitted six bytes of slack past the end,
// which this decoder discards by stopping on the size.
//
// A trailing partial token is therefore legal: the decoder stops the moment the
// plaintext is complete and the caller checks that the input was consumed.
class XSilmarilsDecoder {
public:
    // Decodes one complete byte-run stream.  Succeeds only when exactly
    // nUncompressedSize bytes come out AND every byte of baPacked is accounted
    // for (the tail of an over-long final literal run counts as consumed).
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaUnpacked);

    // Structural probe for detection: same grammar walk, but it only measures.
    // Returns true when the stream produces exactly nUncompressedSize bytes and
    // ends exactly on the last input byte.
    static bool probe(const QByteArray &baPacked, qint64 nUncompressedSize);
};

#endif  // XSILMARILSDECODER_H
