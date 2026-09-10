/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBZIP1_H
#define XBZIP1_H

#include "xarchive.h"

// bzip 0.21 by Julian Seward -- the withdrawn pre-bzip2 compressor ('BZ0').
// It is a single-stream compressor, not a multi-member archive: a 4-byte ASCII
// header followed by one opaque entropy-coded bit stream that runs to EOF.
// The pipeline is RLE1 + Burrows-Wheeler + move-to-front + ADAPTIVE ARITHMETIC
// CODING; bzip2 replaced the last stage with Huffman when the arithmetic-coding
// patents forced the 1996 withdrawal.  No decoder for the arithmetic stage
// exists in this tree (and the original GPL source is unobtainable), so this
// class is deliberately LIST-ONLY: it identifies the container, names its single
// member and reports its extent, and lets extraction fail closed rather than
// emit bytes it cannot vouch for.
class XBZIP1 final : public XArchive {
    Q_OBJECT

public:
    enum BZIP1_TYPE {
        TYPE_UNKNOWN = 0,
        TYPE_BZ1
    };

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_BZIP1_HEADER
    };

    struct BZIP1_HEADER {
        char magic[3];     // 'B' 'Z' '0'  ('0' is the version tag; bzip2 writes 'h')
        quint8 blockSize;  // ASCII '1'..'9' == blockSize100k
    };

    explicit XBZIP1(QIODevice *pDevice = nullptr);
    ~XBZIP1() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;
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
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN,
                             PDSTRUCT *pPdStruct = nullptr) override;
    QString structIDToString(quint32 nID) override;
    QString structIDToFtString(quint32 nID) override;
    quint32 ftStringToStructID(const QString &sFtString) override;
    QList<XFHEADER> getXFHeaders(const XFSTRUCT &xfStruct,
                                 PDSTRUCT *pPdStruct) override;
    QList<XFRECORD> getXFRecords(FT fileType, quint32 nStructID,
                                 const XLOC &xLoc) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1,
                              PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

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
        qint64 nStreamOffset;
        qint64 nStreamSize;
        quint32 nBlockSize100k;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(quint32 nBlockSize100k);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XBZIP1_H
