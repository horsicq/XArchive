/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSECONDNATURE_H
#define XSECONDNATURE_H

#include "xarchive.h"

// Second Nature Software Inc. screen-saver resource archive
// (.SNX picture module, .REF reference/credits module, .BMX splash module;
// Second Nature Software, Portland OR, 1994-1999).
//
// Every file opens with the 32-byte banner
//   "Second Nature Software Inc. " + a three-letter kind + NUL
// where kind is "SNX", "REF" or "BMX".  In .BMX the banner is stored
// plainly; in .SNX and .REF the whole header block is stored as its ONE'S
// COMPLEMENT, so the banner reads AC 9A 9C 90 ... on disk.  The
// complement covers only the header/directory region - member payloads
// are always plain, which is why the JPEG streams are visible unaltered
// in a raw dump of a .REF.
//
// Directory layouts (all little-endian, all name fields 13 bytes wide and
// NUL-terminated with stale bytes after the terminator):
//
//   BMX  0x20 u16 member count, then count * 21-byte entries at 0x22:
//        char name[13], u32 offset, u32 size.
//   REF  0x20 u16 text-member count, 0x22 u16 image-member count, then
//        (n1 + n2) * 31-byte entries at 0x24: char name[13], u16 kind,
//        u32 offset, u32 size, u16 rect[4] (the on-screen placement
//        rectangle).  The table is padded to six slots, so member data
//        starts at 222 on every known sample.
//   SNX  0x20 title[64], 0x60 caption[128], 0xE0 description[128], then
//        at 0x160 u16 sequence id, u16 width, u16 height, followed by
//        three 21-byte slots at 0x166 (same shape as BMX).  Slot 0 is the
//        picture, slot 2 usually names the companion .REF with a zero
//        offset/size.  Member data starts at 421.
//
// Members are STORED - there is no compression anywhere in the format.
// Text members are emitted verbatim.  Image members are ".JIF" files: a
// small Second Nature picture header followed by a complete JFIF stream.
// The reference extractor drops that header and writes the JPEG, so this
// class publishes the member stream starting at the SOI marker.  The
// header comes in two sizes - 42 bytes in .REF/.BMX and 554 bytes in .SNX
// (the extra 512 bytes hold the 128x96 preview block) - so its length is
// located rather than assumed, after the header itself is identified by
// its leading 0x18 byte and the "\x18\x30\x00\x31" tag at +16.
//
// Verified byte-for-byte against the reference extractor on 128 of 129
// members across all 71 corpus files; the odd one out is a truncated
// 518-byte .REF where the reference extractor aborts before creating its
// last member and this class still lists it.
class XSecondNature final : public XArchive {
    Q_OBJECT

public:
    enum KIND {
        KIND_UNKNOWN = 0,
        KIND_SNX,
        KIND_REF,
        KIND_BMX
    };

    struct MEMBER {
        qint64 nRecordOffset;      // directory entry offset
        qint64 nMemberOffset;      // payload offset as stored in the directory
        qint64 nMemberSize;        // payload size as stored in the directory
        qint64 nStreamOffset;      // emitted stream start (JIF header skipped)
        qint64 nStreamSize;        // emitted stream size
        quint16 nKind;             // REF: member kind word
        bool bImage;               // a Second Nature .JIF picture
        QString sFileName;
    };

    explicit XSecondNature(QIODevice *pDevice = nullptr);
    ~XSecondNature() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

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
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN,
                             PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1,
                              PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState,
                    const QMap<UNPACK_PROP, QVariant> &mapProperties,
                    PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nDirectoryOffset;
        qint64 nDirectorySize;
        KIND kind;
        bool bComplemented;
        quint16 nSequenceId;
        quint16 nWidth;
        quint16 nHeight;
        QString sTitle;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    QByteArray readHeaderBlock(qint64 nOffset, qint64 nSize, bool bComplemented,
                               PDSTRUCT *pPdStruct);
    bool resolveStream(MEMBER *pMember, qint64 nInputSize, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSECONDNATURE_H
