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
#ifndef XZXZIPDECODER_H
#define XZXZIPDECODER_H

#include "../xbinary.h"

// ZXZIP (the ZX Spectrum "ZIP" archiver) member codecs, plus the Hobeta
// wrapper - the two cannot be separated, because a member is not emitted as
// its own bytes: the reference writes a 17-byte HOBETA HEADER built from the
// directory entry, then the data, then zero padding up to the entry's sector
// count.  Every method therefore goes through this decoder, stored members
// included.
//
// METHODS
//   0  store
//   1  UNREACHABLE.  No file in the reference corpus uses it and the reference
//      implementation of it is a transliteration of a Z80 depacker whose
//      behaviour cannot be checked against anything, so it is refused cleanly
//      here rather than guessed at.
//   2  stock PKZIP "Shrink": 9..13 bit LZW where code 0x100 is an escape,
//      escape+1 widens the code and escape+2 does a PARTIAL clear that frees
//      only the codes with no children.
//   3  ZXZIP LZH: an 8 KiB ring, LSB-first bits, flag bit 1 = literal, three
//      static table sets picked by the sub-method byte and the output size
//      (A when sub == 0, B when the output is under 0x1600 bytes, else C, and
//      only C Huffman-codes its literals).
//
// THE TREE DECODER INVERTS EVERY CODE BIT relative to how the tree is built: a
// stream bit of 0 takes the child stored for code bit 1.  The tables are
// canonical (ascending length, then ascending symbol) so the inversion is not
// visible in the table data, and getting it wrong decodes a plausible-looking
// but wrong stream.
//
// UNPACKED SIZE.  For type 'B'/'b' (BASIC) the size is the u16 at entry +0x09
// plus 4, otherwise the u16 at +0x0b.  That is rounded up to 256 bytes, and if
// the rounded value disagrees with sectors*256 then BOTH the size and the
// padded size become sectors*256.
class XZXZIPDecoder {
public:
    static const qint64 MAX_UNCOMPRESSED_SIZE = 0x01000000;
    static const qint32 ENTRY_SIZE = 0x16;
    static const qint32 HOBETA_SIZE = 0x11;

    enum METHOD {
        METHOD_STORE = 0,
        METHOD_ZX_UNSUPPORTED = 1,
        METHOD_SHRINK = 2,
        METHOD_LZH = 3
    };

    // Properties as published in FPART_PROP_COMPRESSPROPERTIES: the member's
    // 0x16-byte directory entry, VERBATIM.  Everything the codec needs is in
    // it - the method at +0x14, the sub-method at +0x15, the type letter and
    // the two sizes that give the unpacked length, and bytes 0..12, which are
    // copied straight into the Hobeta header.
    static QByteArray packProperties(const QByteArray &baEntry);

    // *pnDataSize is the decoded byte count, *pnPaddedSize the zero-padded one;
    // the emitted member is HOBETA_SIZE + *pnPaddedSize bytes long.
    static bool memberSize(const QByteArray &baEntry, qint64 *pnDataSize, qint64 *pnPaddedSize);

    static QByteArray hobetaHeader(const QByteArray &baEntry);

    static bool decode(const QByteArray &baPacked, const QByteArray &baProperties, qint64 nUncompressedSize, QByteArray *pbaResult,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XZXZIPDECODER_H
