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
#ifndef XTELEDESKDECODER_H
#define XTELEDESKDECODER_H

#include "../xbinary.h"

// Sydex TeleDisk (.TD0) - the payload transform, the sector data codec and the
// raw-image builder.  The container itself is XTeleDiskArchive's business; what
// lives here is everything that turns the bytes after the 12-byte file header
// into the reconstructed disk image.
//
// THE TRANSFORM IS CHOSEN BY SIGNATURE **AND** VERSION.  There are three of
// them and picking the wrong one decodes plausible-looking garbage rather than
// failing:
//
//   "TD"                  the payload is stored verbatim;
//   "td", version 10..19  blocked 12-bit LSB-FIRST LZW.  A block opens with a
//                         u16 counter that is decremented by THREE per code
//                         read; the dictionary is reset at the start of every
//                         block, freezes at 4096 entries and has NO clear code;
//   "td", version 20..21  Okumura LZHUF, and the decisive detail is that it is
//                         ONE CONTINUOUS STREAM ACROSS THE WHOLE ARCHIVE - not
//                         one stream per track and not one per member.  That is
//                         why this decoder is pull-based: the image builder
//                         takes a few bytes at a time out of a single stream
//                         whose state has to survive between reads.
//
// The tree's own LZHUF core (HANDLE_METHOD_LZH1, struct Lzhuf in
// Algos/xlzhdecoder.cpp) could not be reused: it lives in an anonymous
// namespace inside that translation unit and its only entry point decodes a
// whole member against a KNOWN output length, while TeleDisk has no length to
// give it and needs to stop at a genuine end of input.  XLZHUFDecoder is
// whole-buffer for the same reason.  The pull-based copy below is therefore a
// third instance of the codec rather than an oversight.
//
// CRC16: polynomial 0xA097, MSB first, init 0, NO reflection and NO final xor.
// It is applied to the file header, to the comment block, to a track header's
// first three bytes (low byte only) and to a sector's DECODED data.
//
// SECTOR DATA CODEC (methods 0..2, everything else is an error):
//   0  stored, the payload must already be exactly the sector size;
//   1  u16 count + a 2-byte pattern repeated that many times, 4 bytes per run;
//   2  a run list: {u8 code, u8 count}; code 0 means `count` literal bytes
//      follow, otherwise a block of code*2 bytes follows and is repeated
//      `count` times.
// Both run codings must land exactly on the sector size.
//
// IMAGE LAYOUT - the three rules that a "sensible" builder gets wrong:
//   * sectors are written IN FILE ORDER.  No sorting by sector number, no
//     seeking to a computed LBA, no gap filling for missing sectors;
//   * every sector occupies max(size, 512) bytes, so a 128/256-byte sector is
//     padded out to 512;
//   * nSectors == 0xFF ends the archive and is tested BEFORE the track header
//     CRC, and whatever follows the terminator is ignored (21 of the 23
//     reference samples carry trailing bytes).
// One further quirk is reproduced verbatim: the single track/sector pair
// (nSectors 0x13, sector number 0x76, flag 0x40) is decoded and CRC-checked but
// NOT written out.
//
// PARTIAL OUTPUT IS THE CONTRACT.  On any error the reference extractor keeps
// the bytes it has already produced and reports a failure alongside them, so
// buildImage() returns the partial image together with a bComplete flag; a
// caller that throws partial output away will not match the reference on the
// samples that fail part way through.
class XTeleDeskDecoder {
public:
    struct INFO {
        quint8 nVersion;        // 10..21
        bool bCompressed;       // signature "td" rather than "TD"
        bool bAdvancedCodec;    // "td" with version >= 20, i.e. LZHUF not LZW
        bool bComplete;         // the 0xFF terminator was reached
        qint64 nImageSize;      // bytes produced, partial output included
    };

    static const qint64 HEADER_SIZE = 12;

    static quint16 crc16(const char *pData, qint64 nSize, quint16 nInit);

    // The 12-byte file header test the reference detector performs, header CRC
    // included.
    static bool isValidHeader(const QByteArray &baHeader);

    // Rebuild the image.  pbaImage may be null to measure only.  Returns false
    // when the header is unusable; a truncated or corrupt payload still returns
    // true with pInfo->bComplete false and the partial image in place.
    static bool buildImage(const QByteArray &baFile, QByteArray *pbaImage, INFO *pInfo, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Dispatch entry point.  baPacked is the WHOLE FILE, because the payload
    // transform starts at offset 12 and one member covers every track.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XTELEDESKDECODER_H
