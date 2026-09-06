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
#ifndef XBZIP1DECODER_H
#define XBZIP1DECODER_H

#include "xbinary.h"

/*--
   Decoder for the ORIGINAL pre-bzip2 "bzip" stream ('B','Z','0',digit), the
   format written by Julian Seward's bzip 0.15/0.21 in 1996 and withdrawn a year
   later over the arithmetic-coding patents.  bzip2 kept the container's first
   three header bytes ('B','Z',version) and the BWT/MTF middle, but replaced the
   whole entropy back end with Huffman and added a 48-bit block magic, a
   cleartext per-block CRC and a cleartext origPtr.  None of that exists here:
   in bzip 0.21 the very first field of the first block is already inside the
   arithmetic coder, so XBZIP2Decoder cannot read one byte of this format and
   the two decoders share no code.

   Pipeline, in decode order:
     - 4 raw MSB-first header bytes 'B' 'Z' '0' ('0' + blockSize100k)
     - Moffat/Neal/Witten (DCC95) binary-renormalising arithmetic decoder,
       b = 26, started once per stream immediately after the header
     - per block: a 32-bit origPtr field sent through a fixed uniform 256-symbol
       byte model, sign-negated on the last block; then a fresh set of Fenwick
       structured models over the MTF alphabet
     - inverse move-to-front with a RUNA/RUNB bijective zero-run code
     - inverse Burrows-Wheeler using origPtr
     - the "spot" transform, a scattered -1 (mod 256) applied at a fixed,
       data-independent position sequence
     - inverse RLE1 (a run of >= 4 equal bytes is followed by an extra count
       byte); the final block ends with a 42 sentinel byte that is not output
     - after the last block, a 32-bit whole-file CRC-32 (poly 0x04C11DB7,
       MSB-first, init 0xFFFFFFFF, finally complemented), also sent through the
       byte model.  It is VERIFIED - a mismatch fails the decode rather than
       emitting the bytes.

   Written from the published format description; no bzip 0.21 code is used or
   derived from here, so this file stays inside the tree's MIT boundary.
--*/

class XBZIP1Decoder {
public:
    // Streaming: the uncompressed size of a bzip 0.21 stream cannot be known
    // before decoding it, so this is dispatched like HANDLE_METHOD_BZIP2 rather
    // than through xdecompress.cpp's whole-buffer family.
    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XBZIP1DECODER_H
