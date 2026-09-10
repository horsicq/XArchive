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
#ifndef XVMSDATABASEDECODER_H
#define XVMSDATABASEDECODER_H

#include "xbinary.h"

// The TLV primitives of a VMS DataBase / PCSI$ kit, plus the one "codec" the
// container has: a member's bytes are NOT one range.  They are a run of 0x04
// OCTET STRING chunks inside the member's 0xA3 element, and the file is their
// contents concatenated and cut to the length the attribute record declares.
// Reassembling them is what this decoder does; nothing is compressed.
//
// Tag reading is deliberately not standard BER.  The reference reader always
// takes TWO bytes - a tag byte and a length-form byte - and only then decides:
//
//   form <  0x80   definite, that many content bytes
//   form == 0x80   CONSTRUCTED with indefinite length; the content ends at an
//                  end-of-contents marker, which is a tag byte of 0x00
//   form == 0x81   one more byte holds the length and it must be >= 0x80
//   form == 0x82   two more BIG-ENDIAN bytes hold it and it must be >= 0x100
//
// The >= tests on the long forms are not decoration: they are what stops a
// stream of ordinary data from parsing as a plausible element chain.
class XVMSDataBaseDecoder {
public:
    struct TAG {
        quint8 nTag;
        qint64 nLength;
        bool bConstructed;
        qint32 nHeaderSize;
    };

    // Parses one element header out of at most four bytes of lookahead.
    static bool parseTag(const quint8 *pData, qint64 nAvailable, TAG *pTag);

    // baPacked runs from the first byte inside the member's 0xA3 element up to
    // and INCLUDING that element's end-of-contents marker, so the chunk walk
    // stops on a tag it can see rather than on running out of buffer.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XVMSDATABASEDECODER_H
