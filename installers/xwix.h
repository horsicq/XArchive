/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XWIX_H
#define XWIX_H

#include "xbinary.h"

class XMSI;
class XArchive;

/* WiX detector and installed-payload extractor. */

class XWiX : public XBinary {
    Q_OBJECT

public:
    struct INTERNAL_INFO : public XBinary::INTERNAL_INFO {
        bool bIsValid;
        QString sVersion;
    };

    struct UNPACK_CONTEXT {
        QPointer<QIODevice> pOuterSourceDevice;
        quint64 nOwnerDeviceGeneration;
        UNPACK_STATE *pOwnerState = nullptr;
        XMSI *pMSI;
        UNPACK_STATE state;
        XArchive *pSourceValidator;
        UNPACK_STATE sourceValidationState;
    };

    struct UNPACK_LIFETIME_STATE {
        UNPACK_LIFETIME_STATE() : bOperationInProgress(false), bOwnerAlive(true)
        {
        }
        ~UNPACK_LIFETIME_STATE();
        bool bOperationInProgress;
        bool bOwnerAlive;
        QSet<UNPACK_CONTEXT *> setContexts;
    };

    static bool deleteUnpackContext(UNPACK_CONTEXT *pContext);

    explicit XWiX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XWiX() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    virtual bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    virtual void *getInternalInfo(PDSTRUCT *pPdStruct = nullptr) override;
    virtual void setInternalInfo(void *pInternalInfo) override;

    virtual FT getFileType() override;

    virtual QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

protected:
    bool isDeviceReplacementAllowed() const override
    {
        return m_pUnpackLifetimeState && m_pUnpackLifetimeState->bOwnerAlive && !m_pUnpackLifetimeState->bOperationInProgress &&
               m_pUnpackLifetimeState->setContexts.isEmpty();
    }

private:
    INTERNAL_INFO _getInternalInfo(PDSTRUCT *pPdStruct);
    INTERNAL_INFO m_internalInfo;
    INTERNAL_INFO _detect(PDSTRUCT *pPdStruct);
    QSharedPointer<UNPACK_LIFETIME_STATE> m_pUnpackLifetimeState;
};

#endif  // XWIX_H
