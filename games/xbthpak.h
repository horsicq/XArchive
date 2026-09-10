/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBTHPAK_H
#define XBTHPAK_H

#include "xarchive.h"

// "PAK" single-member container written by the Park Place Productions /
// Swfte installer (INSTBTH.EXE, "Beat The House for Windows", 1995).  The
// 8-byte header stores only the final character of the original file
// extension and the uncompressed size; there is no member name, no compressed
// size, no CRC and no end marker, so the payload runs from offset 8 to EOF and
// the block chain landing exactly on EOF is the only structural anchor there
// is.  The member name has to be rebuilt from the device file name, the same
// way the MS-DOS truncated-extension convention (.BM_ -> .BMP) intends.
class XBTHPAK final : public XArchive {
    Q_OBJECT

public:
    explicit XBTHPAK(QIODevice *pDevice = nullptr);
    ~XBTHPAK() override;

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
        qint64 nPayloadOffset;
        qint64 nPayloadSize;
        qint64 nUncompressedSize;
        qint32 nBlockCount;
        quint8 nExtensionCharacter;
        // True when the candidate was too large to walk end to end and only a
        // bounded prefix of the block chain was proved in bounds.
        bool bPartialScan;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
    static QString methodToString();
};

#endif  // XBTHPAK_H
