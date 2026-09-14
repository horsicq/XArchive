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
#ifndef XGENIUSLIBRARYDECODER_H
#define XGENIUSLIBRARYDECODER_H

#include "../xbinary.h"

// Framing of a compressed member of a "GENIUS LIBRARY" (.GPL) container.
//
// NO NEW CODEC: every block is one complete PKWARE DCL Implode stream, which
// XDclDecoder (Algos/xdcldecoder.*, the blast 1.3 adaptation already in the
// tree) decodes.  What this class adds is the member framing that the single
// whole-stream HANDLE_METHOD_PKWARE_DCL_IMPLODE cannot express, because a
// member is a CHAIN of such streams, each with its own prelude and its own
// end-of-stream code and each starting from a fresh dictionary:
//
//   member stream (FPART_PROP_COMPRESSEDSIZE bytes)
//     repeat until 8 bytes are left:
//       quint32  packed length of the block
//       ...      that many bytes: one complete DCL Implode stream
//     quint32  plaintext length of the whole member
//     quint32  CRC-32 of the plaintext
//
// The plaintext is cut into 4096-byte blocks, so all blocks but the last
// produce exactly 4096 bytes; the block's own end-of-stream code says where it
// ends, so the length is never needed and never stored.
//
// The trailing CRC-32 is the EDB88320 polynomial seeded with 0xFFFFFFFF and
// left UNFINALISED - the writer stores the running accumulator, not the
// customary complemented value - which is exactly what XBinary::_getCRC32
// returns.  Over the 433 members of the 44-file reference corpus the stored
// value matches on every compressed member, so it is enforced: a mismatch
// means the bytes are wrong and publishing them would be worse than failing.
//
// Stored members (method byte 0) never reach this class; they carry no block
// frames and no trailer at all and are published as HANDLE_METHOD_STORE.
class XGeniusLibraryDecoder {
public:
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XGENIUSLIBRARYDECODER_H
