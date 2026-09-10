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
#ifndef XZTCDECODER_H
#define XZTCDECODER_H

#include "../xbinary.h"

// ZTC - the member codec of the Zortech C / Symantec C++ distribution archive
// ("<n>.ZTC").  The container - the ten byte file header, the member chain and
// its name checksum - is XZTCArchive's business; this class starts at a
// member's stored payload and ends with the member's bytes.
//
// TRAP - THE MEMBER PAYLOAD IS PAGED, AND THE PAGE SUMS ARE INLINE.  It is not
// a flat codec stream: it is a run of pages, each min(0x1000, remaining - 4)
// payload bytes followed by a FOUR BYTE little-endian plain sum of just those
// bytes, where `remaining` starts at the compressed size and loses the four
// check bytes before the page length is chosen.  The sums have to be stripped
// and the payloads concatenated BEFORE the codec sees anything.  Feeding the
// raw payload to LZHUF instead produces the first 4096 bytes of the member
// correctly and then garbage, because the codec swallows the first check field
// as data - so a short member decodes perfectly and hides the bug, and a long
// one is corrupt from 0x1000 on while still ending at the right length.
//
// depagePayload() is that step; decode() is depage plus XLZHUFDecoder::decode()
// with XLZHUFDecoder::getZTCOptions(), which is the whole pipeline a dispatch
// needs.  A record publishes the PAGED payload extent, because that is what
// actually exists in the file, so decode() is fed exactly those bytes.
class XZTCDecoder {
public:
    enum {
        // Payload bytes per page, and the size of the little-endian sum that
        // follows each one.
        PAGE_SIZE = 0x1000,
        PAGE_CHECK_SIZE = 4
    };

    // Strip the 4 KiB page / 4-byte sum framing described above. bVerifySums
    // rejects a page whose stored sum disagrees; with it off the payload is
    // still de-framed, which is the only way a member with a damaged sum can
    // still be recovered.
    static bool depagePayload(const QByteArray &baPaged, QByteArray *pbaResult, bool bVerifySums = true);

    // The whole path for one member: depagePayload() followed by LZHUF with
    // XLZHUFDecoder::getZTCOptions(). baPacked is the member's paged payload.
    // Returns false unless exactly nUncompressedSize bytes came out.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XZTCDECODER_H
