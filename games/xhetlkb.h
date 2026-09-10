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
#ifndef XHETLKB_H
#define XHETLKB_H

#include "xgamestorearchive_p.h"

// Humongous Entertainment "TLKB" speech pack (SCUMM HE .TLK / .HE2).
//
// The WHOLE FILE is obfuscated with a constant 0x69 XOR; de-XORed it is an
// ordinary SCUMM IFF-style chunk tree with BIG-endian sizes that INCLUDE the
// 8-byte chunk header:
//
//   TLKB  <size = the entire file>
//     TALK  <size>            one per spoken line
//       HSHD  0x18            sound header, 16-byte payload
//       SDAT  <size>          unsigned 8-bit mono PCM
//
// Verified on the three reference files: the root size equals the file size
// exactly and the walk reaches EOF with no malformed node (529, 529 and 313
// TALK chunks). Every HSHD carries 11000 in the LE u16 at payload +6 - the
// sample rate - and every SDAT opens on 0x80, which is silence for unsigned
// 8-bit samples.
//
// A member is one chunk's PAYLOAD, so extraction yields HSHD+SDAT: the bytes
// that are actually in the container, still self-describing because HSHD keeps
// the rate. Nothing is synthesised - in particular no WAV header is fabricated,
// because that would be handing the caller bytes the archive does not contain.
//
// The 0x69 XOR is the whole codec, and it is length-preserving, so the member
// is published as HANDLE_METHOD_XOR_69 rather than STORE. STORE would "succeed"
// and write out obfuscated bytes as though they were the file.
class XHETLKB final : public XGameStoreArchiveBase {
    Q_OBJECT

public:
    using INTERNAL_INFO = XGameStoreArchiveBase::INTERNAL_INFO;

    explicit XHETLKB(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;

    static quint32 readBE32(const uchar *pData);
};

#endif  // XHETLKB_H
