/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBURN_H
#define XBURN_H

#include "xbinary.h"

class SubDevice;
class XArchive;
class XCab;

/* WiX Toolset Burn bundle detector and payload extractor.
 *
 * This reader supports the version-2 .wixburn section used by WiX v3 and v4,
 * a CAB UX container, and all attached CAB containers described by that
 * section and the embedded Burn manifest. Detached containers and external
 * payloads require Burn's caller-driven acquisition contract, which this
 * single-device archive API does not have. WiX v4 certificate-only payloads
 * are necessarily external as well. Unknown section/container formats are
 * deliberately rejected; released WiX writers/readers through v7 still define
 * only section version 2 and CAB format 1. Thus a successful open never
 * presents an incomplete or unverifiable archive. */
class XBurn : public XBinary {
    Q_OBJECT

public:
    enum CONTAINER_TYPE {
        CONTAINER_TYPE_UNKNOWN = 0,
        CONTAINER_TYPE_UX,
        CONTAINER_TYPE_ATTACHED
    };

    struct PAYLOAD_RECORD {
        CONTAINER_TYPE containerType;
        qint32 nContainerIndex;
        qint32 nInnerIndex;
        QString sSourcePath;
        QString sFilePath;
        QString sPublicPath;
        QString sPayloadId;
        QString sPackageId;
        QString sPackageType;
        qint64 nFileSize;
        QByteArray baHash;
    };

    struct CONTAINER_RECORD {
        CONTAINER_TYPE containerType;
        qint32 nContainerIndex;
        QString sId;
        qint64 nOffset;
        qint64 nSize;
        QByteArray baHash;
    };

    struct INTERNAL_INFO : public XBinary::INTERNAL_INFO {
        bool bIsValid;
        QString sVersion;
        QByteArray baBundleId;
        QList<CONTAINER_RECORD> listContainers;
        QList<PAYLOAD_RECORD> listPayloads;
    };

    struct CONTAINER_CONTEXT {
        SubDevice *pSubDevice;
        XCab *pCab;
        UNPACK_STATE state;
        bool bInitialized;
        qint64 nOuterOffset;
        qint64 nOuterSize;
    };

    struct UNPACK_CONTEXT {
        QPointer<QIODevice> pOuterSourceDevice;
        quint64 nOwnerDeviceGeneration;
        UNPACK_STATE *pOwnerState;
        QList<CONTAINER_CONTEXT *> listContainers;
        QList<PAYLOAD_RECORD> listPayloads;
        qint32 nCurrentPayload;
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

    explicit XBurn(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XBurn() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct = nullptr) override;
    void setInternalInfo(void *pInternalInfo) override;

    FT getFileType() override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

protected:
    bool isDeviceReplacementAllowed() const override
    {
        return m_pUnpackLifetimeState && m_pUnpackLifetimeState->bOwnerAlive && !m_pUnpackLifetimeState->bOperationInProgress &&
               m_pUnpackLifetimeState->setContexts.isEmpty();
    }

private:
    INTERNAL_INFO _getInternalInfo(PDSTRUCT *pPdStruct);
    INTERNAL_INFO _detect(PDSTRUCT *pPdStruct);

    INTERNAL_INFO m_internalInfo;
    QSharedPointer<UNPACK_LIFETIME_STATE> m_pUnpackLifetimeState;
};

#endif  // XBURN_H
