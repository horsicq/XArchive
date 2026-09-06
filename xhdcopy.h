/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XHDCOPY_H
#define XHDCOPY_H

#include "xarchive.h"

// HD-COPY disk image (Oliver Fromme's HD-COPY 1.x/2.x, ".IMG").
//
// The container is a compressed floppy image, not a file archive: it holds one
// member, the reconstructed raw sector image, which the rest of the toolchain
// can then read as a FAT volume.  See Algos/xhdcopydecoder.h for the header
// layout and the per-track RLE.
//
// Detection follows the reference tool exactly, and it is unusually tight for a
// two-byte magic:
//
//   * 0xff 0x18 at +0
//   * label length at +2 is 0..11, and the label field is padded with spaces
//     from the declared length to +0x0d (or is entirely NUL when the length is
//     0) - a real constraint, since it rules out arbitrary binary there
//   * last cylinder at +0x0e is 79..83
//   * sectors per track at +0x0f is one of 9, 10, 15, 17, 18, 20, 21
//
// and on top of those this class walks the block chain and requires it to
// consume the file to the last byte, which no accidental 0xff 0x18 match will
// do.
class XHDCopy final : public XArchive {
    Q_OBJECT

public:
    explicit XHDCopy(QIODevice *pDevice = nullptr);
    ~XHDCopy() override;

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
        qint64 nImageSize;
        qint64 nTrackCount;
        qint64 nTrackSize;
        qint64 nUsedTracks;
        quint8 nLastCylinder;
        quint8 nSectorsPerTrack;
        QString sLabel;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XHDCOPY_H
