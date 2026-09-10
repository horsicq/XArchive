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
#ifndef XLARCPFX_H
#define XLARCPFX_H

#include "games/xgamestorearchive_p.h"

// LArc "PFX" self-extracting Atari ST program (Markus Fritze, PFX 1.13P and
// later).  The carrier is an ordinary GEMDOS executable:
//
//   +0    u16be  0x601a
//   +2    u32be  TEXT size          the decompressor stub
//   +6    u32be  DATA size          the LHA member, header included
//   +10   u32be  BSS size
//   +14   u32be  symbol table size
//
// The last long word of TEXT is the fixed marker DE AD FA CE, so the member
// header begins at exactly 28 + TEXT and the compressed bytes end at
// 28 + TEXT + DATA.  What follows is a level-0 LHA header whose additive
// checksum verifies, always with method "-lz5-".
//
// Two fields the stub writes are NOT usable and are the reason the ordinary
// XLHA reader cannot take these files:
//
//   * the compressed-size field is LARGER than the bytes physically present
//     (measured on all 27 known carriers), so XLHA's extent check at
//     xlha.cpp:288 rejects the member; and
//   * the CRC-16 field does not match the member (mismatched on 21 of 21
//     distinct payloads), so it must not be published as a checksum.
//
// Seventeen of the 27 also store name length 0 - no file name at all - which
// xlha.cpp:298 rejects independently.  The member is then named after the
// carrier, which is what PFX packs: every payload decodes to a complete GEMDOS
// program of exactly the declared uncompressed size.
//
// This reader deliberately claims ONLY the carriers whose compressed-size field
// overruns the bytes present.  Every well-formed LArc/LHarc self-extractor -
// 1,119 of the 1,146 in the reference "SFX LHA" family - keeps the extent its
// header declares and stays on the existing XSFX/XLHA path untouched.
class XLArcPfx final : public XGameStoreArchiveBase {
    Q_OBJECT

public:
    explicit XLArcPfx(QIODevice *pDevice = nullptr);

    using XGameStoreArchiveBase::isValid;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    // The stored name is a build-time source path on the packer's own machine
    // ("E:\\RELEASE\\SBCNV8_0.TOS") and one carrier holds a GEM alert string in
    // the field instead.  Accept only a strict, printable base name.
    static bool decodeStoredName(const QByteArray &baField, QString *pName);

    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XLARCPFX_H
