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
#ifndef XVMSPCSIDECODER_H
#define XVMSPCSIDECODER_H

#include "xbinary.h"

// OpenVMS DCX, the codec behind a PCSI$COMPRESSED kit.  The WHOLE FILE is the
// unit of work: the coding tables live in their own blob near the front and
// every record after them is decoded against those tables, so a member cannot
// be described by one contiguous range and the decoder is handed the container
// itself.
//
// File header (0x50 bytes are read; the ASCII banner IS the magic)
//   +0x00  "OpenVMS DCX PCSI Compressed File"
//   +0x38  u32  size of the table blob
//   +0x40  i32  record count
//
// The table blob sits at file offset 0x200:
//   +0x00  u32  size, equal to the header's copy, at least 0x14
//   +0x04  u32  0
//   +0x08  u32  0x5BF5A3A7          DCX magic
//   +0x0c  u32  0
//   +0x10  u16  context count
//   +0x12  u16  0x14                header length
// followed by that many variable-length context blocks.  Each block is
// EXPANDED INTO A FIXED 0x440-BYTE SLOT, zero filled first, because the codec
// indexes the three tables by node number and by symbol with no bounds of its
// own; the fixed slot is what makes those indices safe:
//   +0x00  u16  block length on disk
//   +0x02  u8   first symbol
//   +0x03  u8   last symbol (>= first)
//   +0x06  u16  0x000c              header length
//   +0x08  u16  node table offset   (> 0x0c, and at most 0x40 leaf-bitmap bytes)
//   +0x0a  u16  context map offset  (0 = this context maps every symbol to 0)
//   the leaf bitmap goes to slot[0x000..0x040), the node table to
//   slot[0x040..0x240) and the map to slot[0x240 + firstSymbol*2 ...], exactly
//   (last - first + 1) u16 entries.  Every u16 in the map area must name an
//   existing context; validating that once here is what lets the inner loop
//   follow a map entry without a range test.
//
// TWO TRAPS, both silent:
//
//   * the table blob does NOT run straight into the records.  The reader
//     position after the last context block is rounded UP to the next 0x200
//     boundary first.  Starting the records at the blob's true end decodes
//     garbage from the first bit.
//   * every record begins with a 2-BYTE HEADER that the reference decoder
//     reads and throws away.  Feeding those two bytes to the bit reader
//     desynchronises record 1 onward while leaving record 0 correct, which is
//     exactly the failure that looks like a codec bug and is not one.
//
// The coding itself is a per-context binary tree walked LSB-first, one bit per
// step: a 1 bit moves to the odd sibling, a leaf emits the node table's byte
// and switches to the context the map names for that symbol, an interior node
// jumps to child*2, and an interior node of 0 ends the record.  All records
// concatenate into one output.
class XVMSPCSIDecoder {
public:
    // The 32-byte banner; a cheap gate that costs no walk.
    static bool isBannerValid(const QByteArray &baHeader);

    // Walks tables and records without storing a byte, so the caller can learn
    // the output size before committing to an allocation that reaches tens of
    // megabytes on real kits.
    static bool measure(const QByteArray &baFile, qint64 *pnUncompressedSize, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // baPacked is the whole container, not a member range.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XVMSPCSIDECODER_H
