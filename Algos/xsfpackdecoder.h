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
#ifndef XSFPACKDECODER_H
#define XSFPACKDECODER_H

#include "../xbinary.h"

// SFPack (.sfpack) - a SoundFont 2 compressor.
//
// SFPACK IS NOT AN ARCHIVE.  It does not hold files: it holds the pieces of ONE
// .sf2 (RIFF/sfbk) and the reference extractor REBUILDS that single file.  The
// reassembly is part of the codec, not a courtesy: the sample data is laid out
// afresh, so the dwStart / dwEnd / dwStartloop / dwEndloop fields of every shdr
// record in the decompressed pdta have to be rewritten, and 46 zero bytes are
// appended after every sample.  Emitting the pieces separately would produce
// something no synthesiser can load.
//
// Container
//   +0x00  4    "SFPK"
//   +0x04  u16  version 0x0100
//   +0x06  u16  flags.  Bit 2 means ENCRYPTED and the reference refuses it
//                (status 7); bits 0/1 announce an embedded .txt/.lic blob,
//                which lives in the skipped region below.  Observed: 0x0008.
//   +0x08  i32  the size of the .sf2 that will be produced.  DO NOT TRUST IT:
//                5 of the 7 corpus files declare a size 468..1170 bytes larger
//                than what the reference actually writes, so the reader
//                measures the real size instead (see measure()).
//   +0x0c  u32  zero
//   +0x10       chunk("INFO")   the RIFF INFO list payload
//               chunk("pdta")   the RIFF pdta list payload
//               i32 skipLen ; skipLen bytes skipped (the .txt / .lic blob)
//               i32 tableBytes (>0, multiple of 4)
//               tableBytes/4 * i32  absolute file offset of each sample stream
//
//   chunk(tag): +0x00 u32 A - a byte count measured from +0x04, so the next
//               chunk starts at chunkStart + 4 + A and the compressed payload
//               is A-8 bytes long; +0x04 4 tag; +0x08 u32 unpacked length;
//               +0x0c the codec-1 payload.
//
// CODEC 1 - chunk LZW.  Textbook LZW with NO CONTROL CODES AT ALL: the first
// free code is 0x100, so the 256 literals are the whole initial alphabet.  Two
// details are load bearing: the width grows when free+1 == 1<<width, one code
// EARLY compared with a textbook coder, and when free reaches 0xfff the entire
// table is thrown away, the width drops back to 9 and the next code is a fresh
// 9-bit literal (a full reset, not a partial clear).  Bits are MSB first out of
// a one-byte refill.
//
// CODEC 2 - the sample codec.  A lossless predictive audio coder over 32-bit
// accumulators emitting 16-bit little-endian PCM.  Its bit reader REFILLS FROM
// 32-BIT LITTLE-ENDIAN WORDS AND HANDS OUT BITS MSB FIRST (bit 31 down) - a
// plain MSB-first byte reader desynchronises immediately.  On top of it:
//   gamma(k)   q leading zero bits (the terminating 1 is consumed), then k raw
//              bits r; value = (q << k) | r
//   sgamma(k)  v = gamma(k+1); zigzag: v even -> v>>1, v odd -> ~(v>>1)
//   uval()     k = gamma(2); return gamma(k)
// Header: N = uval() (block size, <= 0x800), maxOrder = uval() (< N),
// dc = uval() - 33000, hist = max(maxOrder, 3).  Then commands = gamma(2):
// 9 ends the stream, 5 sets intmode = gamma(0), 6 sets shift = gamma(2), 8 sets
// the block length n = uval(), 7 is a block of n zeros, and 0..4 are coded
// blocks with Rice parameter k = gamma(3) and predictor orders 0..3 or an
// explicit order-N filter (order = gamma(3), coefficients sgamma(5), prediction
// (32 + sum) >> 5 arithmetic).
// THE PREDICTOR HISTORY IS SNAPSHOTTED BEFORE THE SHIFT AND INTEGRATION STEPS -
// the predictor works in the residual domain, so taking the history after
// integration silently produces plausible but wrong audio.
//
// Reassembly: smplWords = sum over non-ROM samples of (dwEnd - dwStart + 23),
// where the 23 is the 46 appended zero bytes.  A sample whose sfSampleType
// (shdr +0x2c) has bit 15 set is ROM: it is skipped entirely and its shdr
// record is left alone.  The file is then
//   "RIFF" u32(infoLen + 2*smplWords + pdtaLen + 0x30) "sfbk"
//   "LIST" u32(infoLen + 4) "INFO" <info>
//   "LIST" u32(2*smplWords + 0x0c) "sdta" "smpl" u32(2*smplWords) <samples>
//   "LIST" u32(pdtaLen + 4) "pdta" <patched pdta>
class XSFPACKDecoder {
public:
    static const qint64 MAX_UNCOMPRESSED_SIZE = 0x40000000;
    static const qint32 SHDR_RECORD_SIZE = 0x2e;

    struct HEADER {
        qint64 nInfoOffset;       // first byte of the INFO codec-1 payload
        qint64 nInfoPackedSize;
        qint64 nInfoSize;         // unpacked INFO length
        qint64 nPdtaOffset;
        qint64 nPdtaPackedSize;
        qint64 nPdtaSize;
        qint64 nTableOffset;      // the i32 sample stream offsets
        qint32 nSampleCount;
        qint64 nDeclaredSize;     // header +8; unreliable, see above
        qint64 nStructureSize;    // end of the offsets table
        quint16 nFlags;
    };

    // Cheap: no decompression, only the chunk headers and the offset table.
    // baBuffer may be a PREFIX of the file; nFileSize is the real file size.
    // When the prefix is too short the call fails and *pnNeeded (if given) is
    // set to the number of leading bytes required, so the caller can read that
    // much and try again.
    static bool parseHeader(const QByteArray &baBuffer, qint64 nFileSize, HEADER *pHeader, qint64 *pnNeeded = nullptr);

    // codec 1
    static bool decodeChunk(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // codec 2; one sample stream that starts at nOffset inside baFile
    static bool decodeSample(const QByteArray &baFile, qint64 nOffset, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // The exact size of the .sf2 that decode() will produce.  Only the pdta
    // chunk is decompressed, so baBuffer needs to cover no more than
    // header.nStructureSize bytes of the file.
    static bool measure(const QByteArray &baBuffer, const HEADER &header, qint64 *pnSize, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // The whole rebuild.  baPacked is the WHOLE .sfpack file.  A negative
    // nUncompressedSize skips the final size check.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XSFPACKDECODER_H
