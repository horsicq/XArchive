/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSOLARISBOOTARCHIVE_H
#define XSOLARISBOOTARCHIVE_H

#include "xarchive.h"

// Solaris x86 boot compressed container ("TL"/"TG" blocked Deflate).
//
// Layout, all little-endian:
//   +0x000  19 9E 'T' 'L'          descriptor magic
//   +0x004  u32 nGroupSize         total size of the "TG" group that follows
//   +0x008  zero fill up to +0x200 (the descriptor occupies a whole sector)
//   +0x200  19 9E 'T' 'G'          group magic
//   +0x204  u32 nNumberOfBlocks
//   +0x208  u32 nBlockSizeMax      always 0x8000 in the wild
//   +0x20c  nNumberOfBlocks x { u32 unpacked, u32 packed, u32 offset }
//                                  offset is relative to +0x200 and the first
//                                  one equals the header size (12 + 12*n), so
//                                  the table is self-describing and contiguous
//
// Every block is a self-terminating raw Deflate (RFC 1951) stream; the packed
// length carries one byte of slack past the final block, so the decoder must
// not demand that the whole extent be consumed.  All blocks but the last
// inflate to exactly nBlockSizeMax.  Concatenating them yields one SVR4
// "070702" (newc/CRC) cpio image, which is what the container is: a single
// member, exposed whole so the existing XCPIO recursion can descend into it.
//
// The container is padded out to a 512-byte sector boundary, and the two boot
// floppy images in the corpus carry the rest of a 1.44 MB medium behind it, so
// trailing bytes are an overlay and never a rejection reason.
class XSolarisBootArchive final : public XArchive {
    Q_OBJECT

public:
    struct BLOCK {
        qint64 nCompressedOffset;    // absolute offset in the container
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
    };

    explicit XSolarisBootArchive(QIODevice *pDevice = nullptr);
    ~XSolarisBootArchive() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    qint32 getType() override;
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
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    // Exactly one member per container: the concatenated cpio image.
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;        // descriptor sector + group
        qint64 nGroupOffset;        // always SOLARIS_BOOT_GROUP_OFFSET
        qint64 nGroupSize;          // the "TL" length field, cross-checked
        qint64 nUncompressedSize;   // sum over the block table
        qint64 nBlockSizeMax;
        QString sFileName;
        QList<BLOCK> listBlocks;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    QString buildMemberName();
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSOLARISBOOTARCHIVE_H
