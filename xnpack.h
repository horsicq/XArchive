/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XNPACK_H
#define XNPACK_H

#include "xarchive.h"

// NPack: the compressed-file format of the Symantec / Norton installer, used
// across Norton Desktop for Windows, Norton AntiVirus and Norton Utilities for
// Windows (1993-1995).  The install media carry one container per file, named
// after the original with its final character replaced by '$' -- NDWCLOSE.WB$,
// NSFRAME.NS$, VNAVD.38$ -- so the member name cannot be recovered from the
// payload and is taken from the container's own file name, exactly as the
// reference extractor does.
//
// The container is five ASCII bytes, "MSTSM", and nothing else: no length, no
// name, no checksum, no method field.  Byte 5 onward is a single Stac LZS
// block (see Algos/xnpackdecoder.h for the bit grammar), so this class is a
// single-member archive whose one member always covers the rest of the file.
//
// Because there is no stored size and no stored checksum, isValid() cannot lean
// on a header cross-check.  It runs a bounded STRUCTURAL walk of the bit stream
// instead (Algos/xnpackdecoder.h, probeStream): the block must decode without a
// grammar error, must never reference the zero pre-fill of the 2048-byte
// window, and -- when the whole payload was walked -- must end on its stop code
// in the last byte of the file with nothing left over.  All 141 corpus samples
// satisfy that; random data behind an "MSTSM" prefix does not.
class XNPack final : public XArchive {
    Q_OBJECT

public:
    enum NPACK_TYPE {
        TYPE_UNKNOWN = 0,
        TYPE_NPACK
    };

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_NPACK_HEADER
    };

#pragma pack(push)
#pragma pack(1)
    struct NPACK_HEADER {
        char magic[5];  // 'M' 'S' 'T' 'S' 'M'
    };
#pragma pack(pop)

    explicit XNPack(QIODevice *pDevice = nullptr);
    ~XNPack() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    qint32 getType() override;
    QString typeIdToString(qint32 nType) override;
    ENDIAN getEndian() override;
    QString getArch() override;
    OSNAME getOsName() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    QString structIDToString(quint32 nID) override;
    QString structIDToFtString(quint32 nID) override;
    quint32 ftStringToStructID(const QString &sFtString) override;
    QList<XFHEADER> getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct) override;
    QList<XFRECORD> getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nStreamOffset;
        qint64 nStreamSize;
        // -1 when only a prefix of the payload was walked, which happens for
        // containers above the full-probe ceiling.  The corpus maximum is
        // 389149 bytes packed, so in practice this is always known.
        qint64 nUncompressedSize;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString();
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XNPACK_H
