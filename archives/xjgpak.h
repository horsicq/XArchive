/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XJGPAK_H
#define XJGPAK_H

#include "xarchive.h"

// JGPAK - the distribution container JGsoft (Jan Goyvaerts) shipped with
// HelpScribble and its Delphi / C++Builder HelpContext property-editor packs.
//
// Layout (all integers little endian):
//
//   +0x00  "JGPAK" 00 01                       7-byte signature
//   +0x07  u32 length + bytes                  product description
//          u32 length + bytes                  version string
//          u8                                  flag, 0 in every known archive
//          i32                                 number of members
//   then, back to back, one directory record per member:
//          u8  nameLength
//          .   name[nameLength]                no path component, no NUL
//          u16 MS-DOS time                     +0x00 of the 24-byte tail
//          u16 MS-DOS date                     +0x02
//          i32 uncompressed size               +0x04
//          i32 absolute file offset of data    +0x08
//          i32 compressed size                 +0x0c
//          u32 CRC-32 of the member            +0x10
//          u32 second checksum, unverified     +0x14
//   and finally the member payloads, in directory order, starting immediately
//   after the directory.
//
// The directory is self-checking: member N's data offset equals member N-1's
// offset plus its compressed size, the first equals the byte after the
// directory, and the last member ends exactly at end of file.  XJGPAK::isValid
// enforces all of that, which is what makes the 7-byte signature safe.
//
// Every member is compressed - there is no stored method - with stock Yoshizaki
// LZHUF: adaptive Huffman over 314 symbols (256 literals plus 256..313 standing
// for match lengths 3..60) followed by the classic 12-bit position code.  That
// is bit-for-bit LHA -lh1-, which this tree already decodes, so members are
// routed through the existing HANDLE_METHOD_LZH1 and no new codec is added.
// (the reference implementation runs the same engine over an 8 KiB ring primed with 0x20 while
// HANDLE_METHOD_LZH1 uses the canonical 4 KiB ring; the 12-bit distances can
// never tell the two apart, and all 87 members of the corpus decode
// identically either way.)
//
// Reference: handler "JGPAK" (class adb, entry A487), detector at VA
// 0x0061ea50, worker at 0x0061eb80.
class XJGPAK final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;      // directory record (name length byte)
        qint64 nDataOffset;        // payload, absolute
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nCrc32;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };

    explicit XJGPAK(QIODevice *pDevice = nullptr);
    ~XJGPAK() override;

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
        qint64 nDirectoryEnd;
        QString sDescription;
        QString sVersion;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XJGPAK_H
