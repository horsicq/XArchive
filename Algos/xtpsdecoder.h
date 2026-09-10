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
#ifndef XTPSDECODER_H
#define XTPSDECODER_H

#include "../xbinary.h"

// TPS member codec - a framing layer over plain Okumura LZHUF.
//
// THE FRAMING IS NOT PART OF THE COMPRESSED STREAM.  Every payload byte on disk
// is stored XOR 0x80 and the payload is cut into 0x4000-byte blocks, each
// followed by ONE PLAIN check byte.  That trailing byte is a MOD-256 ADDITIVE
// CHECKSUM of the de-XORed block, not a flag: reading it as a flag byte, or
// feeding it to the codec, desynchronises the Huffman tree a couple of hundred
// bytes in and produces plausible garbage.
//
// The block counter starts at 4, charged for the uncompressed-size dword that
// sits at directory offset +0x12, so BLOCK 0 CARRIES ONLY 0x3ffc DATA BYTES and
// every later block carries 0x4000.  The four size bytes are also the first
// four bytes fed to the block-0 checksum.  After the last block's check byte
// comes one plain 0x01 end-of-member marker.
//
// The codec underneath is the reference implementation's shared parameterised
// LZHUF called as the reference implementation(dist_variant 1, F 0x3c, THRESHOLD 2, no EOF
// symbol, MAX_FREQ 0x8000, ring 0x2000 prefilled with 0x20) - i.e. plain LHA
// "-lh1-" with a 4 KiB match window - so it rides XLZHUFDecoder::getOptions().
class XTPSDecoder {
public:
    static const qint64 MAX_UNCOMPRESSED_SIZE = 0x40000000;
    static const qint64 BLOCK_SIZE = 0x4000;

    // On-disk length of a member whose coded payload is nCodedSize bytes:
    // the coded bytes, one check byte per block, and the 0x01 end marker.
    // ceil((nCodedSize + 4) / 0x4000) blocks - the +4 is the size dword.
    static qint64 storedSize(qint64 nCodedSize);

    // Undo the XOR, verify every block checksum and the end marker.  The whole
    // of baStored must be consumed exactly.
    static bool dechunk(const QByteArray &baStored, quint32 nUncompressedSize, QByteArray *pbaResult);

    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XTPSDECODER_H
