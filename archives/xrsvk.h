/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XRSVK_H
#define XRSVK_H

#include "xarchive.h"

// "RSVKDATA" / "DLIBDATA" container.  The product that writes it is not named
// anywhere in the bytes; the samples are AIDA32 data files (aida32.dat) and an
// embedded 4.bin, all written by the same tool.
//
// Layout - the eight bytes that look like a magic are really two fields, the
// four-byte container tag and the tag of the FIRST data block:
//
//     +0x00  char[4]  "RSVK" or "DLIB"
//     +0x04  ...      the members' data blocks, back to back
//     ...
//     <central directory>
//     EOF-12 char[4]  "ECDR" or "DEND"
//     EOF-8  u32      (unused by the reader)
//     EOF-4  u32      file offset of the central directory
//
// Central directory entry, repeated until fewer than 28 bytes remain in front
// of the trailer:
//
//     +0x00  char[4]  "CFHS" or "FILE"
//     +0x04  i32      packed size hint (not the exact chain length)
//     +0x08  i32      uncompressed size
//     +0x0c  i32      number of data blocks the member is made of
//     +0x10  i32      file offset of the member's first block (>= 4)
//     +0x14  u16      MS-DOS time      +0x16  u16  MS-DOS date
//     +0x18  u32      attribute word
//     +0x1c  char[]   NUL-terminated name, e.g. "d:\aida32.da0"
//
// The entry's packed-size field does not agree with the real chain length, so
// this class measures each member by walking its block chain - which
// doubles as validation.
//
// Codec: XRSVKDecoder (Algos/xrsvkdecoder.*), HANDLE_METHOD_RSVK - a full
// block-sorting compressor (adaptive arithmetic coder + Fenwick structured
// model + RUNA/RUNB + MTF + BWT).  Nothing in the tree decodes it; the closest
// relative is HANDLE_METHOD_BZIP1, which shares the shape of the pipeline but
// not one field, constant or table.
class XRSVK final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDirOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;    // measured length of the block chain
        qint64 nUncompressedSize;
        qint32 nBlockCount;
        quint32 nAttributes;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };

    explicit XRSVK(QIODevice *pDevice = nullptr);
    ~XRSVK() override;

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
        qint64 nDirOffset;
        qint64 nDirSize;
        QList<MEMBER> listEntries;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
};

#endif  // XRSVK_H
