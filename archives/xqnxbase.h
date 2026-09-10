/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XQNXBASE_H
#define XQNXBASE_H

#include "xarchive.h"

// QNX Neutrino boot image ("QNX Base"): the .ifs / .boot / .altboot files an
// x86 QNX 6 install CD boots from.  The whole file is one bootable blob; the
// archive part of it is the image filesystem buried in the middle.
//
// Layout, all little-endian:
//
//   +0x000  boot prefix, always
//              EB 4C "DDDD" 00 00 00 00 00 01 00 00 00 00
//           (a jmp over the QNX IPL's boot record; the four 'D's and the
// following two dwords are the constant part the reference implementation keys on)
//   +0x400  struct startup_header  (0x3f8 and 0x3d0 in other builds; it is
//                                   found by scanning, see below)
//              +0x00 u32  signature, 0x00ff7eeb
//              +0x06 u8   flags1; bit pattern 0x0d means "compressed, UCL"
//              +0x20 i32  startup_size    - length of the startup code that
//                                           follows the header
//              +0x24 u32  stored_size     - size of the whole image
//              +0x2c i32  imagefs_size    - UNCOMPRESSED size of the image
//                                           filesystem
//              +0x30 u32  preboot_size    - the offset of this header itself.
//                                           That self-reference, together with
//                                           the signature, is what identifies
//                                           the header, so it is located by a
//                                           bounded 4-byte-aligned scan of the
//                                           first 64 KiB rather than by trying
// fixed offsets. The reference implementation only ever tries
//                                           0x400 and 0x3f8 and therefore fails
//                                           on the QNX 6.1 images, which put it
//                                           at 0x3d0.
//   preboot_size + startup_size
//           the compressed image filesystem: a chain of UCL NRV2B streams,
//           each preceded by a BIG-endian u16 length, terminated by a zero
//           length word (see XQNXBaseDecoder).
//
// The decompressed blob is a QNX image filesystem:
//
//   +0x00  "imagefs" + u8 flags (0 or 4)
//   +0x08  u32 image_size, equal to the decompressed length
//   +0x10  u32 dir_offset - start of the directory, relative to the blob
//
// and the directory is a run of variable-length records
//
//   +0x00  u16 size of this record        +0x02 u16 extattr_offset
//   +0x04  u32 ino                        +0x08 u32 mode
//   +0x0c  u32 gid                        +0x10 u32 uid
//   +0x14  u32 mtime (Unix seconds)
//   +0x18  u32 file offset inside the blob   (regular files)
//   +0x1c  u32 file size                     (regular files)
//   +0x20  NUL-terminated path, e.g. "proc/boot/procnto"
//
// ended by a record whose size word is 0.  Records whose mode says directory
// (0x4000) or symlink (0xa000) carry no payload and are stepped over, exactly
// as the reference implementation does - the published members are the regular files only, and the
// directory structure comes back from the slashes in their paths.
//
// Reference: the reference implementation handler A458 ("QNX Base", class dcb, VMT slots
// 0x006132d0 / 0x006132f0); detector 0x00612c90, worker 0x00613190, header
// probe 0x00613120, block walk 0x00612cd0, image-filesystem walk 0x00612d70.
class XQNXBase final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nImageOffset;       // payload offset inside the decompressed blob
        qint64 nUncompressedSize;  // payload size
        qint64 nRecordOffset;      // directory record inside the blob
        quint32 nMode;
        quint32 nMTime;
        QString sFileName;
    };

    explicit XQNXBase(QIODevice *pDevice = nullptr);
    ~XQNXBase() override;

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
    struct HEADER {
        qint64 nHeaderOffset;      // preboot_size: 0x400 or 0x3f8
        qint64 nStartupSize;       // startup_header +0x20
        qint64 nStoredSize;        // startup_header +0x24
        qint64 nImageFsSize;       // startup_header +0x2c
        qint64 nCompressedOffset;  // nHeaderOffset + nStartupSize
        quint8 nFlags1;            // startup_header +0x06
        quint16 nVersion;          // startup_header +0x04
    };

    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nCompressedOffset;
        qint64 nCompressedSize;
        qint64 nImageFsSize;
        HEADER header;
        QList<MEMBER> listMembers;
    };

    bool readHeader(HEADER *pHeader, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool parseImage(const QByteArray &baImage, QList<MEMBER> *pListMembers);
    static QByteArray packMemberProperty(qint64 nImageOffset, qint64 nImageFsSize);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XQNXBASE_H
