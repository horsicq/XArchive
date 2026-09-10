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
#ifndef XSQXDECODER_H
#define XSQXDECODER_H

#include "../xbinary.h"

// SQX (SQX-Software / Michael Bierhoff) member codec, methods 1..4.
//
// Facts that are easy to get wrong and are all exercised by the reference
// corpus:
//
//   * the method byte is at member-header body offset +5, NOT in the flags;
//   * the dictionary is 1 << (((flags >> 8) & 0xF) + 15), capped at 4 MiB;
//   * bits are MSB-first INSIDE LITTLE-ENDIAN 32-BIT WORDS - a plain MSB-first
//     byte reader desyncs immediately;
//   * there are THREE distance tables (48, 50 and 52 usable symbols) and the
//     one in force is chosen by the dictionary size, not by anything in the
//     stream;
//   * a block header opens with one bit: 0 selects the Huffman coder
//     implemented here, 1 selects a second "direct" coder that is not
//     implemented.  Methods 5 and up, and the b0 preprocessor filter, are not
//     implemented either; all of those fail cleanly instead of emitting
//     plausible garbage.
//
// Code lengths arrive in two different encodings - a deflate-style RLE over a
// 19-symbol pre-table, and a delta scheme (0 same, 10 +1, 111 abs4, 1100 -1,
// 1101 +2) - and both appear in the same archive.
//
// The Huffman table builder deliberately does its running-start arithmetic in
// 16 bits, exactly as the reference does: the completeness test compares
// against (1 << maxlen) & 0xFFFF.
//
// SOLIDITY.  Flag 0x04 means the member continues the previous member's LZ
// state: the 4 MiB window, the write cursor, the four recent distances and the
// last match are all carried over, and only a member WITHOUT the flag resets
// the cursor (never the window contents).  In the reference corpus essentially
// every member sets it, and 22 of the 332 members of one archive genuinely
// reach back into the preceding member's output.  This cannot be expressed
// through the 7z-shaped ISSOLID path - that one slices a record out of one
// decoded block - so solidity is handled INSIDE this decoder, the way Quantum
// does it: the reader hands over the whole archive plus a table of the
// preceding coded members, and decode() replays them to rebuild the window.
// Stored members are absent from the table because they never touch the LZ
// state.
class XSQXDecoder {
public:
    static const qint64 MAX_UNCOMPRESSED_SIZE = 0x40000000;
    static const qint32 ENTRY_SIZE = 28;
    static const qint32 TABLE_HEADER_SIZE = 12;

    // The member table published in FPART_PROP_COMPRESSPROPERTIES:
    //
    //   +0  u32 magic 'SQXT'
    //   +4  u32 entry count
    //   +8  u32 target entry index
    //   then one 28-byte entry per coded member, in archive order:
    //     +0  u64 data offset (inside the byte range handed to decode())
    //     +8  u64 packed size
    //     +16 u64 uncompressed size
    //     +24 u16 member flags
    //     +26 u8  filter selector ("b0")
    //     +27 u8  method
    static QByteArray startTable();
    static bool appendEntry(QByteArray *pbaTable, qint64 nDataOffset, qint64 nPackedSize, qint64 nUncompressedSize, quint16 nFlags, quint8 nFilter,
                            quint8 nMethod);
    static QByteArray finishTable(const QByteArray &baTable, qint32 nTargetIndex);

    // baPacked is the archive byte range the table's offsets are relative to.
    static bool decode(const QByteArray &baPacked, const QByteArray &baProperties, qint64 nUncompressedSize, QByteArray *pbaResult,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XSQXDECODER_H
