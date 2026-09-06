/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XIGF1_H
#define XIGF1_H

#include "xarchive.h"

// Compressed member file of the "IGF" Windows installer (unidentified vendor,
// 1995-96).  These ship on the distribution media under a chopped extension -
// SETUP.EX_, CTL3DV2.DL_, VTUTOR.IC_, README.TX_ - and each file holds exactly
// one member: this is a per-file compressor, not a multi-member archive.
//
// Fixed 0x38 byte header, little endian:
//
//     +0x00 u16  0xecdb  magic
//     +0x02 u16  0x0200  format version (the sibling IGF variant U3 handles
//                        through the same worker rejects 0xffff here)
//     +0x04 u16  0x0020  constant in every known file
//     +0x06 u16  installer-internal group/disk word
//     +0x08 u32  0
//     +0x0c u32  installer-internal file id
//     +0x10 u32  Unix time the file was packed
//     +0x14 u32  Unix time of the original file (this is the member's mtime)
//     +0x18 u32  0
//     +0x1c i32  uncompressed size          (must be >= 0)
//     +0x20 u32  checksum of the plaintext  (algorithm unknown; U3 does not
//                                            verify it and neither does this)
//     +0x24 i32  compressed size            (must be >= 0)
//     +0x28 u32  checksum of the packed data (likewise unverified)
//     +0x2c u32  offset of the compressed data (must be > 0)
//     +0x30 u32  ~offset, the one's complement of +0x2c - a 32-bit self check
//     +0x34 i32  compressed size again, must equal +0x24
//     +0x38      NUL-terminated original file name, lower case, then padding
//                up to the data offset
//
// The complement word plus the duplicated size is what makes the two-byte magic
// safe: U3's detector (FUN_0053f4b0) checks exactly those relations.
//
// The payload is LHA static Huffman: NC = 510 with a 12-bit direct table,
// NT = 19 / TBIT = 5 / i_special = 3, np = 13 and pbit = 4, THRESHOLD = 3, so a
// 4 KiB dictionary - i.e. plain "-lh4-", which XLZHDecoder already implements.
// U3 runs it through its shared LHA core (FUN_004e4dd0) with exactly those two
// parameters and bounds the output by the +0x1c size instead of relying on the
// 0x1fe end symbol, which is how this class drives it too.  No new codec.
class XIGF1 final : public XArchive {
    Q_OBJECT

public:
    struct RECORD {
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nPackTime;      // +0x10, Unix seconds
        quint32 nFileTime;      // +0x14, Unix seconds
        quint32 nPlainChecksum; // +0x20
        quint32 nPackedChecksum;// +0x28
        quint16 nVersion;       // +0x02
        quint16 nGroup;         // +0x06
        quint32 nFileId;        // +0x0c
        QString sFileName;
    };

    explicit XIGF1(QIODevice *pDevice = nullptr);
    ~XIGF1() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

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
        QList<RECORD> listRecords;  // always exactly one entry
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XIGF1_H
