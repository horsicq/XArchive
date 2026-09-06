/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XADFARCHIVE_H
#define XADFARCHIVE_H

#include "xarchive.h"

#include <QDateTime>

// Reader for standard 880/1760 KiB AmigaDOS DOS\0 (OFS) and DOS\1 (FFS)
// floppy images. U3 archive 10 derives the root from DD/HD geometry.
// Member data is fragmented on disk, so records are exposed
// through XArchive's index-paired archive-stream contract.
class XADFArchive final : public XArchive
{
    Q_OBJECT

public:
    explicit XADFArchive(QIODevice *pDevice = nullptr);
    ~XADFArchive() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    FT getFileType() override;
    MODE getMode() override;
    QString getMIMEString() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState,
                    const QMap<UNPACK_PROP, QVariant> &mapProperties,
                    PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                       PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct DATA_BLOCK {
        qint32 nBlock = -1;
        qint32 nPayloadOffset = 0;
        qint32 nPayloadSize = 0;
    };

    struct MEMBER {
        QString sPath;
        qint32 nHeaderBlock = -1;
        qint64 nSize = 0;
        qint64 nStoredSize = 0;
        bool bIsDirectory = false;
        QList<DATA_BLOCK> listDataBlocks;
        QDateTime mtDateTime;
        quint32 nProtection = 0;
    };

    struct CONTEXT {
        bool bOFS = false;
        qint32 nRootBlock = -1;
        qint32 nBlockCount = 0;
        qint64 nImageSize = 0;
        QString sVolumeName;
        QList<MEMBER> listMembers;
    };

    struct PARSER;

    bool readBlock(qint32 nBlock, QByteArray *pBlock,
                   PDSTRUCT *pPdStruct);
    bool parseImage(CONTEXT *pContext, PDSTRUCT *pPdStruct);
};

#endif  // XADFARCHIVE_H
