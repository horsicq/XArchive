/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSMSIPAKARCHIVE_H
#define XSMSIPAKARCHIVE_H

#include "games/xgamestorearchive_p.h"

// Two containers of the same lineage: both wrap their payload in a PKWARE Data
// Compression Library ("implode"/blast) stream, which XFU already decodes as
// HANDLE_METHOD_PKWARE_DCL_IMPLODE, so neither brings a new codec.
//
// FT_SMSIPAK - "SMSIPAK " distribution media, the .PAK volumes of the DOS/Win16
// setup sets the reference corpus files under SMSIPAK.  Fourteen-byte header:
//
//   +0   8    "SMSIPAK "
//   +8   u16  0x1a07, the format stamp
//   +10  u32  offset at which the member area ends - NOT the file size: the
//             volume index follows it
//
// then a flat chain of members, each a 34-byte record and its payload:
//
//   +0   12   member name, NUL padded (8.3, no path)
//   +12  u8   always overwritten with a terminator by the reference reader
//   +13  u16  MS-DOS date
//   +15  u16  MS-DOS time
//   +17  u32  CRC-32 of the member's plaintext
//   +21  u16  attribute word (0, 1 or 2 across the corpus - not structural)
//   +23  u8   method: 1 = PKWARE DCL, 2 = stored
//   +24  i32  plaintext size
//   +28  i32  stored size
//   +32  u16  zero
//
// After the last member sits a volume index - u32 zero, u16 member count, then
// one u32 file offset per member - which is what tells the archive end from the
// cross-volume remainder some volumes carry.  The reference tool stops at the
// first record whose method is not 1, so it loses every stored member and
// everything behind it; both are read here.
//
// FT_PSN_COMPRESS - "PSNcompress", the per-file wrapper 3M's Post-it Software
// Notes setup ships its payload in.  A 65-byte ASCII header and one DCL stream:
//
//   +0   53   "PSNcompress-Copyright<AE> 3M Company ALL RIGHTS RESERVED"
//   +53  4    "0001"
//   +61  8    plaintext size as upper-case hex, or "FFFFFFFF" when the writer
//             did not know it - then the size comes from the trial decode
//   +65       the DCL stream
//
// Nothing in that header names the payload, so the single member takes the
// archive's own file name verbatim, which is what the reference publishes too.
class XSmsiPakArchive final : public XGameStoreArchiveBase
{
    Q_OBJECT

public:
    explicit XSmsiPakArchive(QIODevice *pDevice = nullptr,
                             FT fileType = FT_UNKNOWN);

    using XGameStoreArchiveBase::isValid;
    static FT detectFileType(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    static bool isValid(QIODevice *pDevice, FT fileType,
                        PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

private:
    bool scanSmsiPak(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                     PDSTRUCT *pPdStruct);
    bool scanPsnCompress(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                         PDSTRUCT *pPdStruct);
    bool scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                    PDSTRUCT *pPdStruct) override;
};

#endif  // XSMSIPAKARCHIVE_H
