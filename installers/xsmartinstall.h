/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSMARTINSTALL_H
#define XSMARTINSTALL_H

#include <QSet>
#include <QSharedPointer>

#include "xbinary.h"

class XMaterializedUnpackGuard;

/* Detector + extractor for Smart Install Maker installers. The Delphi PE stub
 * stores its installer payload in the overlay, prefixed with the literal tag
 * "Smart Install Maker v. <version>". A 36-byte footer at EOF holds four u64
 * region pointers + a method/magic dword. method==0 = "None" (store); method==1
 * = compressed. The compressed payload is a CAB image with its four-byte
 * "MSCF" signature omitted: "Normal" is MSZIP and "Maximal" is LZX. */

class XSmartInstall : public XBinary {
    Q_OBJECT

public:
    struct INTERNAL_INFO : public XBinary::INTERNAL_INFO {
        bool bIsValid;
        QString sVersion;
        bool bExtractable;  // store, MSZIP ("Normal"), or LZX ("Maximal")
        qint32 nMethod;     // 0 = store, 1 = compressed
        qint64 nDataStart;
    };

    struct FILE_ENTRY {
        QString sName;
        qint32 nNameIndex;
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

    explicit XSmartInstall(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XSmartInstall() override;

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

#endif  // XSMARTINSTALL_H
