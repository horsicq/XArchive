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
#ifndef XVMARCDECODER_H
#define XVMARCDECODER_H

#include "../xbinary.h"

// VM/CMS VMARC (John Fisher, Rice University) member codecs.
//
// The member content is written RAW EBCDIC with the CMS records simply
// concatenated: no record separators, no length prefixes and NO code-page
// translation.  The reference extractor builds its record sink with the
// translate flag OFF, and anything else corrupts every byte of the output.
//
// Two codecs:
//
//   ASIS (flags & 0x40) - big-endian u16 length-prefixed records, a zero
//   length ending the member.
//
//   LZW - 12-bit MSB-first codes over a 4096-entry trie with LEAF RECYCLING:
//   a round-robin rover hands out the first entry whose child count is zero,
//   and the entry's character is filled in LAZILY on the FOLLOWING iteration
//   (ch[pending] = ch[first node of the string just decoded]).  That deferred
//   write is exactly what makes the KwKwK case resolve for free - there is no
//   special case for it anywhere in the loop.
//
//   Symbol 0 is end-of-record and symbols 1..256 are data bytes value - 1.  A
//   single end-of-record finishes a fixed-format ('F') member; a variable
//   format ('V') member needs two CONSECUTIVE ones, because any data byte
//   resets the counter.
//
// Neither codec stores an output length, so the reader measures by decoding;
// run() reports both the produced bytes and the byte the member ends on, which
// is what lets the container walk find the next 80-byte-aligned header.
class XVMARCDecoder {
public:
    enum MODE {
        MODE_LZW = 0,
        MODE_STORED = 1
    };

    static const qint64 MAX_UNCOMPRESSED_SIZE = 0x20000000;

    // Properties as published in FPART_PROP_COMPRESSPROPERTIES:
    //   u16 LRECL (little endian), u8 isFixedFormat, u8 MODE
    static QByteArray packProperties(quint16 nLRECL, bool bFixed, quint8 nMode);
    static bool unpackProperties(const QByteArray &baProperties, quint16 *pnLRECL, bool *pbFixed, quint8 *pnMode);

    // baData is the whole container, nOffset the member's first data byte.
    // pbaOut may be null to measure only.  *pnEndOffset gets the byte the
    // member stops on.  Returns false when the member did not terminate
    // cleanly; the caller still keeps whatever was produced, as the reference
    // does.
    static bool run(const QByteArray &baData, qint64 nOffset, quint16 nLRECL, bool bFixed, quint8 nMode, QByteArray *pbaOut, qint64 *pnEndOffset,
                    XBinary::PDSTRUCT *pPdStruct = nullptr);

    static bool decode(const QByteArray &baPacked, const QByteArray &baProperties, qint64 nUncompressedSize, QByteArray *pbaResult,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XVMARCDECODER_H
