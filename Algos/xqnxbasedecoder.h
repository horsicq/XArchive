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
#ifndef XQNXBASEDECODER_H
#define XQNXBASEDECODER_H

#include <QByteArray>
#include <QtGlobal>

// The compressed payload of a QNX Neutrino boot image (.ifs / .boot /
// .altboot), i.e. everything after the startup code: a chain of independent
// UCL NRV2B streams, each introduced by a BIG-endian u16 giving the byte length
// of that stream, and terminated by a length word of 0.
//
// The length word is only a hint - each stream carries its own end-of-stream
// marker (a match offset that decodes to 0xffffffff), which is what actually
// stops a block - but it is checked here so a truncated or non-QNX buffer is
// rejected instead of being decoded into noise.
//
// The codec itself is plain UCL NRV2B with the 8-bit bit source (ucl
// nrv2b_decompress_8), reproduced here rather than reached through
// XUCLDecoder because that class only exposes a whole-stream
// DATAPROCESS_STATE entry point and this container needs the streams walked
// one at a time with a shared output history.
//
// Bit source (MSB first, sentinel bit trick): the state is a 32-bit word that
// is doubled on every bit; when its low byte reaches 0 the next input byte is
// loaded into the low byte and the word is doubled and incremented, which puts
// a stop bit under the 8 payload bits.  The bit handed back is always bit 8.
//
// Per iteration:
//   * while getbit()  -> copy one literal byte
//   * m_off = 1; do { m_off = m_off*2 + getbit(); } until getbit()
//   * m_off == 2      -> reuse the previous match offset
//     otherwise       -> m_off = (m_off - 3)*256 + nextByte(); 0xffffffff ends
//                        the stream; else ++m_off becomes the new previous
//   * m_len = getbit()*2 + getbit() + 1; when that is 1 the length is gamma
//     coded: do { m_len = m_len*2 + getbit(); } until getbit(); m_len += 3
//   * m_len += (m_off > 0xd00)
//   * copy m_len bytes from output[-m_off]
//
// Reference: the reference implementation handler A458 ("QNX Base", class dcb, VMT 0x006132d0),
// worker 0x00613190 -> block walk 0x00612cd0 -> 0x00612ae0 -> codec 0x00612830
// with its variant selector held at 0 (NRV2B) and a 64 KiB output window.
class XQNXBaseDecoder {
public:
    // Walk the whole block chain.  Stops as soon as nStopAfter bytes of output
    // exist (pass -1 for "decode everything"); the terminating zero length word
    // is then not required.  nMaxOutput is a hard ceiling on the produced size.
    static bool decodeImage(const QByteArray &baPacked, qint64 nMaxOutput, qint64 nStopAfter, QByteArray *pbaUnpacked);

    // Convenience wrapper for the unpack chain: decode just far enough to
    // return [nOffset, nOffset + nSize) of the decompressed image.
    static bool decodeRange(const QByteArray &baPacked, qint64 nOffset, qint64 nSize, qint64 nMaxOutput, QByteArray *pbaUnpacked);
};

#endif  // XQNXBASEDECODER_H
