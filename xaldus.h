/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XALDUS_H
#define XALDUS_H

#include "xarchive.h"

// Aldus/Adobe Setup install-disk container: exactly one compressed member per
// file, carrying the member's real name and size in a fixed header.  Three
// generations share the container and differ only in the per-block codec:
// "ALDUS LZW   1.00", "ALDUS PKZP  2.00" and "ADOBE LZSH  3.00".  The
// container fields are big-endian even though this is a DOS/Windows format,
// while the timestamps that follow them are little-endian.
class XAldus final : public XArchive {
    Q_OBJECT

public:
    enum GENERATION {
        GENERATION_UNKNOWN = 0,
        GENERATION_LZW = 1,   // "ALDUS LZW   1.00"
        GENERATION_PKZP = 2,  // "ALDUS PKZP  2.00"
        GENERATION_LZSH = 3   // "ADOBE LZSH  3.00"
    };

    explicit XAldus(QIODevice *pDevice = nullptr);
    ~XAldus() override;

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
        qint64 nHeaderSize;    // also the absolute payload offset
        qint64 nStreamOffset;  // == nHeaderSize
        qint64 nStreamSize;    // sub-header + block table + all block data
        qint64 nDataOffset;
        qint64 nUncompressedSize;
        qint64 nBlockSize;
        qint64 nLastBlockSize;
        qint64 nBlockCount;
        GENERATION generation;
        QString sFileName;
        QDateTime dtModified;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static GENERATION magicToGeneration(const QByteArray &baMagic);
    static QString generationToString(GENERATION generation);
    static HANDLE_METHOD generationToHandleMethod(GENERATION generation);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XALDUS_H
