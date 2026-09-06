/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef XSETTLERSFTDECODER_H
#define XSETTLERSFTDECODER_H

#include "xbinary.h"

// Resource decoder for the Settlers / Serf City ".PA" archive (XSettlersFT).
//
// The container itself only stores (size, offset) pairs, but its members are
// not interchangeable blobs: two of the five member kinds are images in a
// game-specific encoding that is meaningless without the archive's own 256
// colour palette (itself one of the members).  classify() reproduces exactly
// the test U3 applies - a bounded trial decode of the row-RLE - and decode()
// turns the two image kinds into the Windows bitmaps U3 writes.  The three
// remaining kinds are byte-for-byte copies and are published as
// HANDLE_METHOD_STORE instead of reaching this decoder at all.
class XSettlersFTDecoder : public QObject {
    Q_OBJECT

public:
    enum KIND {
        KIND_NONE = 0,
        KIND_BIN = 1,      // anything unrecognised: stored verbatim
        KIND_XMI = 2,      // IFF "FORM" chunk (XMIDI music): stored verbatim
        KIND_BITMAP = 3,   // flat 8bpp image, becomes a paletted BMP
        KIND_MASK = 4,     // row-RLE sprite, becomes a 32bpp BMP
        KIND_PALETTE = 5   // the 768-byte palette: stored verbatim
    };

    explicit XSettlersFTDecoder(QObject *parent = nullptr);

    // Decides what a member is and how many bytes the conversion produces.
    // baPalette may be empty: the palette only supplies colour values and
    // never steers the decision, so classification stays available before the
    // palette member has been located.
    static KIND classify(const QByteArray &baEntry, qint64 *pnOutputSize);

    // Converts one KIND_BITMAP or KIND_MASK member.  Returns false for every
    // other kind - those never carry this handle method.
    static bool decode(const QByteArray &baPacked, const QByteArray &baPalette,
                       qint64 nUncompressedSize, QByteArray *pbaUnpacked,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);

    static const qint32 PALETTE_SIZE = 0x300;
};

#endif  // XSETTLERSFTDECODER_H
