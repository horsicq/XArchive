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
#ifndef XZPAKDECODER_H
#define XZPAKDECODER_H

#include "../xbinary.h"

// ZPAK - the member codec of ZSoft's archive format.  The six byte header and
// the fixed size directory are XZPAKArchive's business; this class starts at a
// member's stored payload.
//
// THE MAGIC PICKS THE CODEC FOR EVERY MEMBER; it is not a per-entry field.
//
// "zpk2" is plain PKWARE DCL ("implode", XDclDecoder), header byte 0 the
// literal mode and byte 1 the 4/5/6 dictionary-size code - it needs no wrapper
// at all and is not handled here.
//
// "zpak" is 12-bit LZW with a chunk layer wrapped round it, and BOTH halves of
// that sentence are traps:
//
// TRAP 1 - THE CHUNK LAYER IS FRAMING, NOT BLOCKING.  The payload is a 0xCA
// marker byte, then `compressedSize - 2` bytes of [u16 little-endian length]
// [that many bytes] records, then a 0x00 trailer.  The chunk payloads
// CONCATENATE INTO ONE CONTINUOUS LZW STREAM - the codec does not reset, flush
// or pad at a chunk boundary, so decoding chunk by chunk produces a correct
// first 4096-byte chunk and then nothing usable.  unchunk() strips the layer.
//
// TRAP 2 - THE LZW SETTINGS ARE NOT THE ONES THE CALL SITE SUGGESTS.  The
// reference implementation configures its shared LZW here with four arguments
// and leaves six on uninitialised stack, so the bit order and the width-step
// bias cannot be read off the code at all; they were pinned down against a
// member that exists in both a "zpak" and a "zpk2" copy of the same archive.
// The answer is GIF-style: LSB first, widths 9..12, 0x100 CLEAR, 0x101 END,
// first free code 0x102, no block padding, step bias 0.  ULEAD's handler makes
// a textually IDENTICAL call and wants MSB-first with bias 1, so copying the
// options across from it is a mistake that still decodes short members.
class XZPAKDecoder {
public:
    enum {
        // First byte of a framed "zpak" payload.
        CHUNK_MARKER = 0xca
    };

    // Strip the 0xCA / [u16 length][data] / 0x00 framing of a "zpak" payload.
    static bool unchunk(const QByteArray &baPayload, QByteArray *pbaResult);

    // The whole path for a "zpak" member: unchunk() followed by XSharedLZWDecoder
    // with the options above. Returns false unless exactly nUncompressedSize
    // bytes came out.
    static bool decodeLZW(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XZPAKDECODER_H
