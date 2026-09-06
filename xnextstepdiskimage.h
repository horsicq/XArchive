/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XNEXTSTEPDISKIMAGE_H
#define XNEXTSTEPDISKIMAGE_H

#include "xarchive.h"

// NeXTSTEP ".diskimage" - the container the NeXT installer shipped its
// distribution floppies in.  There is no magic string: the file is a 46-byte
// big-endian geometry block followed by the raw sector image, and the geometry
// block is what identifies it, because all of its fields have to agree with
// each other.
//
//   +0x00 u32be  format version (2 in everything shipped)
//   +0x04 u16be  bytes per sector          (512)
//   +0x06 u32be  cylinders                 (80)
//   +0x0a u32be  heads                     (2)
//   +0x0e u32be  drive type / step rate
//   +0x12 u32be  drive attributes
//   +0x16 u32be  image size in bytes       (0x168000 = 1 474 560)
//   +0x1a u32be  block offset of the image (1)
//   +0x1e u32be  block size                (512)
//   +0x22 u16be  bytes per sector, repeated(512)
//   +0x24 u32be  sectors per track         (18)
//   +0x28 u16be  gap / format byte word
//   +0x2a u32be  total sectors             (0xb40 = 2880)
//   +0x2e        the raw sector image starts here
//
// The reference tool keys on image size, total sectors and cylinder count; this
// class additionally requires the geometry to multiply out exactly
// (cylinders * heads * sectorsPerTrack == totalSectors, totalSectors *
// bytesPerSector == imageSize), which is what makes a container with no magic
// safe to detect.
//
// Nothing is compressed.  The single member this class publishes is the raw
// sector image, so the generic chain can go on and recognise what is inside it
// (a NeXT disk label "dlV3" on the m68k media, an x86 boot sector on the Intel
// media, a UFS filesystem behind either).
class XNextStepDiskImage final : public XArchive {
    Q_OBJECT

public:
    explicit XNextStepDiskImage(QIODevice *pDevice = nullptr);
    ~XNextStepDiskImage() override;

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
        qint64 nImageOffset;
        qint64 nImageSize;
        quint32 nCylinders;
        quint32 nHeads;
        quint32 nSectorsPerTrack;
        quint32 nTotalSectors;
        quint32 nBytesPerSector;
        quint32 nVersion;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XNEXTSTEPDISKIMAGE_H
