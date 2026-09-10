/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XHFE_H
#define XHFE_H

#include "xarchive.h"

#include "Algos/xhfedecoder.h"

// HxC Floppy Emulator HFE v1 disk image (SDCard HxC, the DSKA00nn.HFE files).
//
// The container stores no files and no sectors: each track is the raw MFM bit
// cell stream of one revolution, both sides interleaved in 256-byte chunks,
// LSB first.  This class publishes exactly one member - the flat sector image
// recovered from that flux - and leaves the guest file system to the tree's
// existing disk/file-system path, the same split the reference implementation
// uses (its HFE handler decodes to a raw image and then re-enters the generic
// partition/file-system chain).
//
// Header layout, the track look-up table and the MFM recovery are documented
// on XHFEDecoder (Algos/xhfedecoder.h).  Detection follows the reference
// detector exactly: "HXCPICFE", revision byte 0, a non-zero track count and a
// side count of 1 or 2 - plus, here, a successful decode of cylinder 0, so a
// file that only carries the magic cannot be mistaken for a readable image.
class XHFE final : public XArchive {
    Q_OBJECT

public:
    explicit XHFE(QIODevice *pDevice = nullptr);
    ~XHFE() override;

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
        XHFEDecoder::GEOMETRY geometry;
        QString sImageName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
};

#endif  // XHFE_H
