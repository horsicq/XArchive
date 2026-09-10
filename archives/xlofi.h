/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XLOFI_H
#define XLOFI_H

#include "xarchive.h"

// Solaris compressed lofi disk image - the file `lofiadm -C lzma` produces and
// `lofiadm -a` mounts.  The container holds exactly one logical object, the raw
// disk image, cut into fixed-size segments that are indexed and individually
// LZMA compressed.
//
// The byte layout, the index convention (n entries describe n-1 segments) and
// the per-segment framing are documented on XLOFIDecoder in
// Algos/xlofidecoder.h, which owns the parse so the class and the codec cannot
// drift apart.
//
// The archive publishes a single member whose name is derived from the
// container's own file name, because nothing inside the format records one: the
// only thing stored is the image.
//
// Reference: handler "LOFI" (class yfa, entry A431). Its slot 1 is the
// generic raw-image worker, which is why the reference implementation shows the mounted file system
// rather than the image: the image is handed straight back to the format
// detector.
class XLOFI final : public XArchive {
    Q_OBJECT

public:
    explicit XLOFI(QIODevice *pDevice = nullptr);
    ~XLOFI() override;

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
        qint64 nSegmentSize;
        qint64 nSegmentCount;
        qint64 nDataOffset;
        qint64 nDataSize;
        qint64 nImageSize;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XLOFI_H
