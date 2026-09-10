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
#ifndef XGENTEEDECODER_H
#define XGENTEEDECODER_H

#include "../xbinary.h"

// The Gentee installer payload: one BLOCK CHAIN, and every block is
//
//     quint32 nDecodedSize
//     <bit stream>
//
// where the bit stream is read MSB-first and stops the moment nDecodedSize
// bytes have been produced.  Nothing stores the block's packed length, so a
// block can only be skipped by decoding it, and the chain can only be walked
// from its start.
//
// The codec is an LZ77 over a 0x8000 window driven by THREE ADAPTIVE Huffman
// trees.  A tree starts as a Huffman tree over the weights 1..n (symbol i
// weighs i + 1) and after every decoded symbol the leaf's weight is raised by
// one along the path to the root, swapping a node with its uncle whenever the
// uncle is no longer heavier; when the root reaches 0x200 every weight is
// halved.  The trees are therefore only correct if every preceding symbol of
// the same stream has been decoded, which is what makes the container solid.
//
//   * main tree, 0x112 symbols: below 0x100 a literal; 0x100 + n is a match
//     whose length is n + 3, and n == 0x11 escapes into the length tree
//     (0xed symbols) for lengths 0x11 + s + 3.
//   * distance tree, 0x22 symbols: below 0x1e a base from the 30-entry table
//     plus that entry's extra bits (read LOW BIT FIRST); 0x1e..0x21 select one
//     of the four most recent distances instead.  Every distance is then moved
//     to the front of that four-entry list.
//   * the match source is the window read from nWindowPos - nLength - nDistance,
//     not from nWindowPos - nDistance.
//
// The payload as a whole is
//
//     block  : the Gentee runtime image (its decoder state is DROPPED after it)
//     20 raw bytes: quint16 at +14 must be zero, otherwise the archive is not
//                   in the plain form this decoder handles
//     block  : two of them, skipped
//     block* : the command chain, decoded with the SAME state from the 20-byte
//              header onwards.  quint16 at +0 is the command tag and the
//              record body starts at +3.  Tag 0x87f0 ends the archive, 0x87f4
//              is a member:
//                  +0x00 quint32 attributes
//                  +0x04 qint32  decoded size
//                  +0x08 FILETIME
//                  +0x19 quint8  0 = the payload follows STORED, else it is
//                                the next block of the member chain
//                  +0x1f asciiz  name
//
// Member data blocks share ONE decoder state of their own, so member k can
// only be decoded by replaying members 0..k-1 - the AIN arrangement.  decode()
// therefore takes the payload from its very first byte and a member index.
class XGenteeDecoder {
public:
    struct MEMBER {
        QString sFileName;
        qint64 nSize;
        qint64 nDataOffset;  // payload-relative; for a stored member the bytes themselves
        qint64 nDataEnd;     // payload-relative end of this member's data
        quint32 nAttributes;
        quint64 nFileTime;
        bool bStored;
    };

    // Walks the whole payload.  A chain that stops early (a truncated
    // download) still reports the members that were completely decoded.
    static bool scan(const QByteArray &baPayload, QList<MEMBER> *plistMembers, qint64 *pnArchiveSize, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // baPacked starts at the payload's first byte and must reach at least the
    // end of the requested member's data.
    static bool decode(const QByteArray &baPacked, qint64 nMemberIndex, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);

    static QByteArray indexToProperty(qint64 nMemberIndex);
    static bool propertyToIndex(const QByteArray &baProperty, qint64 *pnMemberIndex);

    static bool isPayloadHeader(const char *pData, qint64 nSize);
};

#endif  // XGENTEEDECODER_H
