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
#ifndef XULEADDECODER_H
#define XULEADDECODER_H

#include "../xbinary.h"

// ULEAD ("U_LEAD CORP.") - a block table followed by per-block LZW.
//
// The member is split into 0x4000-byte blocks, each compressed independently,
// and a u16 PACKED size per block precedes the payloads.  A block whose packed
// size equals its plain size is STORED - that is the only marker, there is no
// per-block method byte, so a block that happens to compress to exactly its
// own length would be read as stored.  The producer evidently makes sure that
// cannot happen by storing it instead.
//
// The codec is the shared parameterised LZW at 12 bits with a clear code, an
// end code, MSB-first codes and the one-code-early width step.  That is the
// same option set CMP uses at 16 bits, so the two differ only in code width.
//
// Every block starts with the clear code and the encoder emits another one when
// the dictionary fills, which lands 3838 codes later - at bit 43231 of a block
// that stays at 12 bits.  Both are worth knowing: a block whose first code is
// not 0x100, or whose second clear arrives early, is a damaged payload rather
// than a layout this decoder has misread.
//
// The whole file is handed to this decoder rather than just the payload region
// because the block table lives in the header and a record cannot describe two
// disjoint ranges.
class XULEADDecoder {
public:
    struct HEADER {
        qint64 nDataOffset;      // first block payload
        qint64 nTableOffset;     // the u16-per-block packed sizes
        qint32 nBlockCount;
        qint32 nLastBlockSize;
        qint64 nUncompressedSize;
        QString sFileName;       // empty for the layout that stores no name
        qint32 nLayout;          // 1 or 2
    };

    static bool parseHeader(const QByteArray &baFile, qint64 nFileSize, HEADER *pHeader);
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XULEADDECODER_H
