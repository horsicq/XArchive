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
#ifndef XTARX2DECODER_H
#define XTARX2DECODER_H

#include "../xbinary.h"

// TARX 2 (QNX "tarx") - fixed-key Blowfish in ECB over 8-byte blocks starting
// at file offset 0x10.  Underneath is a GZIP stream, and inside that a tar, so
// the whole container is Blowfish( gzip( tar ) ).
//
// KEY MATERIAL - THERE IS NO PASSPHRASE ANYWHERE.  The reference does not run a
// Blowfish key schedule at all:
//
//   1. it copies the STANDARD Blowfish initial S-boxes (the hex digits of pi, a
//      public constant - S1[0]=0xd1310ba6, S2[0]=0x4b7a70e9, S3[0]=0xe93d5a68,
//      S4[0]=0x3a39ce37) into the four boxes;
//   2. it copies 18 words into P[0..17].  THIS IS THE ONLY REAL SECRET: the
//      P-array ALREADY MIXED with the vendor's key, snapshotted so the
//      passphrase never appears in the binary;
//   3. it sets L,R = P[16],P[17] and regenerates the S-boxes the textbook way
//      (4 boxes x 128 iterations of encrypt(L,R) -> S[i][2j], S[i][2j+1]).
//
// Step 3 starting from P[16],P[17] is exactly the state an ordinary key
// schedule reaches once it has finished rewriting P, so this is a fixed-key
// Blowfish whose schedule was frozen half way through.  The 18 P-words are
// inlined below because they cannot be derived; the 1024 S-box words are the
// public constant.
//
// THE ENDIANNESS QUIRK: the ciphertext halves are loaded LITTLE-endian and the
// recovered plaintext halves are emitted BIG-endian.
//
//     for each 8-byte block c at 0x10, 0x18, ...:
//         L, R = u32le(c[0:4]), u32le(c[4:8])
//         blowfish_decrypt(L, R)
//         emit u32be(L), u32be(R)
//
// A trailing partial block (< 8 bytes) is DROPPED - the reference's refill does
// an exact 8-byte read and gives up on a short one.
//
// Because the plaintext was padded up to a multiple of 8 before encryption, the
// decrypted buffer carries up to SEVEN bytes of junk after the end of the gzip
// member (both reference samples do - 7 and 6 bytes).  inflate() stops at the
// member's own end marker, so that padding never reaches the tar.
//
// THE DECODER PRODUCES THE TAR, NOT THE GZIP.  Emitting the gzip and letting
// the filter chain unwrap it looks tidier but does not work: a decoded buffer
// that detects as FT_TAR_GZ is handed to the FT_GZIP reader, and that reader
// publishes its payload as an ARCHIVE_STREAM record with no byte extent, which
// XFilteredArchive::materializeLayer cannot decode.  Doing both steps in one
// pass keeps the container a single filter layer whose output is a plain tar -
// the same shape the RAR-under-XOR formats use.
class XTARX2Decoder {
public:
    static const qint64 HEADER_SIZE = 0x10;
    static const qint64 BLOCK_SIZE = 8;
    static const quint32 MAGIC0 = 0x7D957678;
    static const quint32 MAGIC1 = 0x45AD3B9C;

    static bool isValidHeader(const QByteArray &baHeader);

    // Decrypt nSize bytes (a multiple of BLOCK_SIZE) from pSource into pTarget.
    // The buffers may not overlap.  Working a chunk at a time is what keeps the
    // 43 MiB reference sample from needing a second whole-file buffer.
    static bool decryptChunk(const char *pSource, qint64 nSize, char *pTarget);

    // Dispatch entry point: decrypt, then inflate the gzip member the plaintext
    // holds.  nUncompressedSize is the size of the tar inside it.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XTARX2DECODER_H
