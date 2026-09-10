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
#ifndef XKBOOMDECODER_H
#define XKBOOMDECODER_H

#include "../xbinary.h"

// Codec of the KBOOM 1.1 single-member container.  An LZW, but not the usual
// one: the dictionary is an explicit TRIE with sibling lists, and it is never
// cleared - once all 8191 slots are taken the coder RECYCLES a childless node,
// sweeping a cursor round the table.  There is no clear code and no end code;
// the header's declared size is the only terminator.
//
//  * codes are a fixed 13 bits, taken from the top of a 32-bit accumulator
//    that is refilled 16 bits at a time from little-endian words;
//  * a code expands by walking parent links to the root, so the phrase comes
//    out backwards and is written down from a fixed offset in a scratch
//    buffer;
//  * after each phrase, up to FOUR of its leading bytes extend the trie from
//    the node the PREVIOUS code named - which is why several dictionary
//    entries can be created per token, and why the previous phrase's length
//    takes part in the cap that stops a phrase exceeding 100 bytes.
//
// Read out of the reference decompressor at VA 0x00575690.  All 55 reference
// files decode; 49 are byte-identical to that implementation and the other 6
// differ only in that it emits ONE byte more than the header declares, which
// this decoder drops.
class XKBoomDecoder {
public:
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XKBOOMDECODER_H
