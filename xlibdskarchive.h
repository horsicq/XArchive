/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XLIBDSKARCHIVE_H
#define XLIBDSKARCHIVE_H

#include "xarchive.h"

// Native single-record reader for strongly identified legacy sector images.
class XLibDskArchive final : public XArchive
{
    Q_OBJECT

public:
    explicit XLibDskArchive(QIODevice *pDevice = nullptr);
    ~XLibDskArchive() override;

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

private:
    struct CONTEXT {
        QByteArray rawImage;
        QString sName;
        QString sDriver;
        QString sInfo;
        qint64 nRawSize = 0;
    };

    static QString identifyDriver(QIODevice *pDevice);
};

#endif  // XLIBDSKARCHIVE_H
