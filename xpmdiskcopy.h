/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XPMDISKCOPY_H
#define XPMDISKCOPY_H

#include "xarchive.h"

// OS/2 "PM Diskcopy" floppy image (.IMG).
//
// The container is a 47-byte preamble followed by the raw sector image, and
// nothing is compressed or encoded:
//
//   +0x00  11 bytes  "PM Diskcopy"   (exactly 11 - the NUL that follows in a
//                                     hex dump is already the low byte of the
//                                     BPB's bytes-per-sector field)
//   +0x0b  36 bytes  a verbatim DOS BPB copied off the image's own boot
//                    sector: u16 bytesPerSector, u8 sectorsPerCluster,
//                    u16 reservedSectors, u8 numberOfFATs, u16 rootEntries,
//                    u16 totalSectors16, u8 mediaDescriptor, u16 sectorsPerFAT,
//                    u16 sectorsPerTrack, u16 heads, u32 hidden,
//                    u32 totalSectors32, then drive/serial bytes
//   +0x2f  ...       the disk image itself, starting at its boot sector and
//                    truncated after the last used sector
//
// U3 validates the copied BPB rather than the string alone, and this class
// mirrors that gate exactly: bytesPerSector must be 512/1024/2048/4096, the
// media descriptor must have its top nibble set, the FAT count must be 1 or 2,
// and sectorsPerCluster must be one of the values in U3's bitmap at 0x425ba0
// (1..16, 32, 64, 128).  On top of that the payload must be a whole number of
// sectors, which every sample in the corpus is.
//
// The single member is the raw image, published STORED; the FAT inside it is
// then a job for the file-system reader, exactly as U3 does it.
class XPMDiskcopy final : public XArchive {
    Q_OBJECT

public:
    explicit XPMDiskcopy(QIODevice *pDevice = nullptr);
    ~XPMDiskcopy() override;

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
        qint64 nDataOffset;
        qint64 nDataSize;
        quint32 nBytesPerSector;
        quint32 nSectorsPerCluster;
        quint32 nNumberOfFATs;
        quint32 nMediaDescriptor;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    QString memberName();
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XPMDISKCOPY_H
