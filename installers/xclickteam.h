/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XCLICKTEAM_H
#define XCLICKTEAM_H

#include <QList>
#include <QPointer>
#include <QSet>
#include <QSharedPointer>

#include "xbinary.h"

class XMaterializedUnpackGuard;

class XClickteam : public XBinary {
    Q_OBJECT

public:
    struct INTERNAL_INFO : public XBinary::INTERNAL_INFO {
        bool bIsValid = false;
        QString sVersion;
        qint64 nContainerOffset = -1;
    };

    struct FILE_ENTRY {
        QString sName;
        QByteArray baData;
    };

    struct UNPACK_CONTEXT {
        ~UNPACK_CONTEXT();

        QList<FILE_ENTRY> listEntries;
        qint64 nTotalOutput = 0;
        qint32 nCurrentIndex = 0;
        qint64 nCurrentOffset = 0;
        qint64 nSourceSize = 0;
        quint64 nDeviceGeneration = 0;
        QPointer<QIODevice> pSourceDevice;
        UNPACK_STATE *pOwnerState = nullptr;
        QByteArray baToken;
        XMaterializedUnpackGuard *pSourceGuard = nullptr;
        QList<XMaterializedUnpackGuard *> listCompanionGuards;
    };

    struct LIFETIME_STATE {
        ~LIFETIME_STATE();

        bool bOwnerAlive = true;
        bool bOperationInProgress = false;
        QSet<UNPACK_CONTEXT *> setContexts;
    };

    explicit XClickteam(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XClickteam() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct = nullptr) override;
    void setInternalInfo(void *pInternalInfo) override;
    FT getFileType() override;

    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

protected:
    bool isDeviceReplacementAllowed() const override;

private:
    INTERNAL_INFO _getInternalInfo(PDSTRUCT *pPdStruct);
    INTERNAL_INFO _detect(PDSTRUCT *pPdStruct);
    bool _buildEntries(UNPACK_CONTEXT *pContext, qint64 nContainerOffset, PDSTRUCT *pPdStruct);

    INTERNAL_INFO m_internalInfo;
    QSharedPointer<LIFETIME_STATE> m_pUnpackLifetimeState;
    bool m_bTrustedSnapshot = false;
};

#endif  // XCLICKTEAM_H
