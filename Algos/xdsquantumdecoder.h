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
#ifndef XDSQUANTUMDECODER_H
#define XDSQUANTUMDECODER_H

#include <QByteArray>
#include <QList>

#include "xbinary.h"

// Decoder for the SOLID Quantum stream of a David Stafford "DS" .PAK archive
// (Q.EXE, the compressor Microsoft later licensed for CAB compression type 2).
//
// The whole archive body is ONE arithmetic-coded stream: the 9 adaptive models,
// the LZ window and the coder registers all run continuously across the members,
// so member N can only be produced by replaying members 0..N-1 first.  That is
// exactly what decode() does, which is why it is handed the full member size
// table rather than a single size.
//
// Two on-disk variants exist and differ in more than framing:
//   * "new" (header version byte >= 0x17): the CAB shape.  A 7-symbol selector
//     encodes literal-bank / match-length class, match extra bits are read as
//     RAW bits out of the bit buffer, models rescale with the shifts-left(4/50)
//     halve-then-reorder rule, and every member is followed by a 16-bit
//     in-stream checksum that MUST be consumed to keep the stream aligned.
//   * "old" (version byte < 0x17): a 5-symbol selector that only says
//     literal-vs-match, the match LENGTH is decoded first from its own 29-slot
//     model and selects one of three position models (len 3, len 4, len >= 5),
//     the extra bits go through the arithmetic coder against a flat model
//     instead of the raw bit buffer, model rescale is a plain halve with no
//     reordering, several models start from a decaying weight table rather than
//     a uniform one, symbols come out reversed (real = entries - sym - 1), and
//     there is no in-stream checksum (the member CRC lives in the directory).
//
// Ported from the recovered handler and validated
// byte-exact against the reference implementation over all 19 files of the reference corpus, both
// variants, window orders 10..18.
class XDSQuantumDecoder {
public:
    // baProperties, little endian, describes the member being asked for:
    //   quint8  nWindowBits    10..21, from header offset 6
    //   quint8  bOldVariant    1 when the header version byte is < 0x17
    //   quint32 nMemberIndex   index of the wanted member
    //   quint32 nMemberCount   number of members in the archive
    //   quint32 nSizes[nMemberCount]  uncompressed size of every member, in order
    // baPacked is the whole compressed body (everything after the directory).
    // nUncompressedSize must equal nSizes[nMemberIndex].
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, const QByteArray &baProperties, QByteArray *pbaUnpacked,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Builds the property blob above.
    static QByteArray createProperties(qint32 nWindowBits, bool bOldVariant, qint32 nMemberIndex, const QList<qint64> &listSizes);
};

#endif  // XDSQUANTUMDECODER_H
