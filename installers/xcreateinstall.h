/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XCREATEINSTALL_H
#define XCREATEINSTALL_H

#include <QSet>
#include <QSharedPointer>

#include "xbinary.h"

class XMaterializedUnpackGuard;

/* Detector + extractor for CreateInstall (Gentee-engine) installers. The
 * self-extracting PE carries a ".gentee" section plus the Gentee runtime strings
 * ("genteert.dll" / "gentee_init" / "lzge_decode") and stores its payload in a
 * "GEA" container. Store, lzge, PPMd7, solid groups, and sibling GEA volumes are
 * decoded here. */

class XCreateInstall : public XBinary {
    Q_OBJECT

public:
    struct INTERNAL_INFO : public XBinary::INTERNAL_INFO {
        bool bIsValid;
        QString sVersion;      // GEA container format version, if readable
        qint64 nGeaOffset;     // file offset of the last "GEA" container
        QString sGeaFileName;  // external main GEA volume when it is not embedded
    };

    struct FILE_ENTRY {
        QString sName;
        QByteArray baData;  // decoded content
    };

    struct UNPACK_CONTEXT {
        ~UNPACK_CONTEXT();
        QList<FILE_ENTRY> listEntries;
        QPointer<QIODevice> pSourceDevice;
        UNPACK_STATE *pOwnerState = nullptr;
        QByteArray baToken;
        quint64 nDeviceGeneration = 0;
        qint64 nSourceSize = 0;
        qint64 nCurrentOffset = 0;
        qint32 nCurrentIndex = 0;
        XMaterializedUnpackGuard *pSourceGuard = nullptr;
        QList<XMaterializedUnpackGuard *> listCompanionGuards;
    };

    explicit XCreateInstall(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XCreateInstall() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    virtual bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    virtual void *getInternalInfo(PDSTRUCT *pPdStruct = nullptr) override;
    virtual void setInternalInfo(void *pInternalInfo) override;

    virtual FT getFileType() override;

    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

protected:
    bool isDeviceReplacementAllowed() const override;

private:
    struct LIFETIME_STATE {
        bool bOperationInProgress = false;
        bool bOwnerAlive = true;
        QSet<UNPACK_CONTEXT *> setContexts;
        ~LIFETIME_STATE();
    };
    INTERNAL_INFO _getInternalInfo(PDSTRUCT *pPdStruct);
    INTERNAL_INFO m_internalInfo;
    INTERNAL_INFO _detect(PDSTRUCT *pPdStruct);
    QSharedPointer<LIFETIME_STATE> m_pUnpackLifetimeState;
    bool m_bTrustedSnapshot = false;
};

#endif  // XCREATEINSTALL_H
