/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XGAMOS_H
#define XGAMOS_H

#include "xarchive.h"

// "GAMOS PACKED FILE" - the resource container of the Russian studio Gamos
// (Snake, Wonderland/WS, ...).  A game ships one .GPF holding its scripts and
// blocks plus companion containers in the same format that carry the display
// data for one video mode (.EGA / .VGA) or the music (.SND).
//
// Fixed 33-byte header:
//
//   +0x00  1A "GAMOS PACKED FILE"   (0x12 bytes)
//   +0x12  u8  0
//   +0x13  u8  1
//   +0x14  u16 0
//   +0x16  u8  1
//   +0x17  u16 number of members, never 0
//   +0x19  8 bytes, zero in every known file
//
// then exactly that many 22-byte directory records, back to back:
//
//   +0x00  char[13]  NUL-padded 8.3 name
//   +0x0d  u8        compression method
//   +0x0e  i32       absolute file offset of the member's data
//   +0x12  u16       stored (compressed) size
//   +0x14  u16       uncompressed size
//
// The directory ends exactly where the first member's data begins, so the whole
// container is header, directory, payload - there is no separate index.  Both
// size fields are 16-bit, which caps a member at 64 KiB.
//
// Method 1 is an LZSS variant that is close to Okumura's but not compatible
// with any of the ones already in this tree, so it gets its own dispatch arm
// (HANDLE_METHOD_GAMOS):
//
//   * 4 KiB ring pre-filled with 0x20, write cursor starts at 0 - NOT at
//     N - F, which is what LPAK/AMPK/SZDD do;
//   * one flag byte per eight tokens, consumed least-significant bit first,
//     bit set = literal;
//   * a match is two bytes b1, b2 with position = ((b2 & 0x0f) << 8) | b1 and
//     length = (b2 >> 4) + 3 - the two nibbles of b2 are the other way round
//     from HANDLE_METHOD_LPAK_LZSS and HANDLE_METHOD_AMPK_LZSS;
//   * the stream is driven by the COMPRESSED byte count, not by the output
//     size: it stops when the input is exhausted, and the +0x14 field is only
//     verified afterwards.
//
// Method 2 is stored: the member is the +0x12 bytes at the record's offset, and
// its +0x14 field is 0 (the original reports the stored size as the member size
// in that case).
//
// Reference: the reference implementation handler A561 ("Gamos", class hgb, VMT slots
// 0x0065cac0 / 0x0065cae0); detector 0x0065c770, worker 0x0065c850, method
// dispatch 0x0065c7e0, LZSS codec 0x0065c440.
class XGamos final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint8 nMethod;
        QString sFileName;
    };

    explicit XGamos(QIODevice *pDevice = nullptr);
    ~XGamos() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nDirectoryOffset;
        qint64 nDirectorySize;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static XBinary::HANDLE_METHOD methodOf(const MEMBER &member);
    static QString reportedMethodOf(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XGAMOS_H
