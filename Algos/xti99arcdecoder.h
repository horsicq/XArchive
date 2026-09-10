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
#ifndef XTI99ARCDECODER_H
#define XTI99ARCDECODER_H

#include "../xbinary.h"

// Member extraction for a TI99 ARC (.ARK) container whose payload is LZW
// compressed - flag bit 1 of the TIFILES / FIAD wrapper.
//
// THE REASON THIS CODEC EXISTS AT ALL is that the archive's CATALOGUE LIVES
// INSIDE THE COMPRESSED STREAM.  A member is therefore not a byte range of the
// file, and the ordinary "one record maps to one (offset, size, method)
// triple" shape cannot express it: the whole payload has to be expanded and
// then sliced.  12 of the 13 reference files are built this way, so this is
// the normal case, not an exotic one.  When the payload is NOT compressed the
// reader does not come here - a member really is a byte range then, and it
// goes out as HANDLE_METHOD_SCL_SECTORS (prefix-then-copy) instead.
//
// The LZW is the shared parameterised one (XSharedLZWDecoder) with the parameter
// block the reference dispatcher passes for this family:
//
//     maxBits 12, CLEAR code, END code, MSB-FIRST bit order,
//     no RLE90 filter, no block padding, width-step bias 0
//
// MSB-first is the trap here.  Most of the families that share this LZW read
// codes LSB-first; reading this one LSB-first still decodes a few hundred
// plausible bytes before it desyncs, which looks like a truncated archive
// rather than a wrong bit order.
//
// The properties blob (FPART_PROP_COMPRESSPROPERTIES) carries what the file
// itself cannot say, all little endian:
//
//   +0x00 u32  plainSize     bytes of the expanded stream the member needs
//   +0x04 u32  memberOffset  member start inside the expanded stream
//   +0x08 u32  memberSize    member length inside the expanded stream
//   +0x0c u32  prefixSize    length of the synthesised TIFILES header
//   +0x10      prefix        that header, emitted verbatim before the data
class XTI99ARCDecoder {
public:
    // 64 MiB. The corpus tops out near 100 KiB; the cap only exists so a
    // crafted stream cannot ask for an unbounded buffer.
    static const qint64 MAX_PLAIN_SIZE = 0x4000000;
    // The catalogue is a chain of at most 0x400 sectors of 0x100 bytes, so
    // expanding this much is always enough to walk it - and it keeps the
    // detector cheap on files that are not this format at all.
    static const qint64 PROBE_SIZE = 0x40100;

    // Expand up to nPlainSize bytes of the payload.  A stream that ends early
    // yields a short buffer and still returns true; the caller decides whether
    // the range it wanted is present, because the probe deliberately asks for
    // more than any real archive produces.
    static bool expand(const QByteArray &baPayload, qint64 nPlainSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);

    static QByteArray buildProperties(qint64 nPlainSize, qint64 nMemberOffset, qint64 nMemberSize, const QByteArray &baPrefix);

    static bool decode(const QByteArray &baPacked, const QByteArray &baProperties, qint64 nUncompressedSize, QByteArray *pbaResult,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XTI99ARCDECODER_H
