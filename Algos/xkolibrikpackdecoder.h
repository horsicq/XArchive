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
#ifndef XKOLIBRIKPACKDECODER_H
#define XKOLIBRIKPACKDECODER_H

#include "xbinary.h"

// KolibriOS kpack ("KPCK") container decoder.
//
// The payload is an LZMA1 stream with hard-wired properties lc=3 lp=0 pb=2 and
// no 13-byte LZMA header.  Its range coder is initialised from the FIRST four
// payload bytes read LITTLE-endian - kpack's own unpacker byte-swaps that dword
// in place before handing the buffer to a stock 5-byte initialiser, so no dummy
// byte is skipped.  Two optional x86 "call trick" filters are undone afterwards
// using a 5-byte trailer at the very end of the container.
//
// The whole container is consumed from nInputOffset (the 'KPCK' magic itself),
// because the filter parameters live at both ends of the file.
class XKolibriKPackDecoder : public QObject {
    Q_OBJECT

public:
    explicit XKolibriKPackDecoder(QObject *parent = nullptr);

    // Largest container / unpacked payload this decoder will buffer.  Both the
    // LZMA window and the call-trick pass need the complete output resident.
    static const qint64 KPACK_MAX_INPUT_SIZE = 0x10000000;   // 256 MiB
    static const qint64 KPACK_MAX_OUTPUT_SIZE = 0x20000000;  // 512 MiB
    static const qint64 KPACK_HEADER_SIZE = 12;

    // Header-only sanity check shared with XKolibriKPack::isValid().
    static bool checkHeader(const char *pHeader, qint64 nHeaderSize, qint64 nFileSize, quint32 *pnUnpackedSize, quint32 *pnFlags);

    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XKOLIBRIKPACKDECODER_H
