/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSPISSFX_H
#define XSPISSFX_H

#include "xbinary.h"

class SubDevice;
class XSPIS;

// GP-Install/TCompress executable carrier. Payloads are either an exact PE
// overlay chain of [u32 size][SPIS blob] records or complete SPIS resources.
// The public stream is the flattened member sequence across every blob.
class XSpisSFX final : public XBinary {
    Q_OBJECT

public:
    struct BLOB {
        qint64 nOffset = 0;
        qint64 nSize = 0;
        qint32 nRecords = 0;
        QString sHint;
    };

    struct LOCATION {
        qint32 nBlob = -1;
        qint32 nRecord = -1;
    };

    struct INTERNAL_INFO : XBinary::INTERNAL_INFO {
        bool bIsValid = false;
        bool bOverlayChain = false;
        qint64 nFileSize = 0;
        QList<BLOB> listBlobs;
        QList<LOCATION> listLocations;
    };

    struct UNPACK_CONTEXT {
        INTERNAL_INFO info;
        QPointer<QIODevice> pOuterSourceDevice;
        quint64 nOwnerDeviceGeneration = 0;
        UNPACK_STATE *pOwnerState = nullptr;
        SubDevice *pSubDevice = nullptr;
        XSPIS *pArchive = nullptr;
        UNPACK_STATE innerState = {};
        qint32 nBoundBlob = -1;
        QMap<UNPACK_PROP, QVariant> mapUnpackProperties;
    };

    struct UNPACK_DEFERRED_CLEANUP {
        ~UNPACK_DEFERRED_CLEANUP();
        QSet<UNPACK_CONTEXT *> setContexts;
    };

    explicit XSpisSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XSpisSFX() override;

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
        return (!m_pUnpackOperationState || !*m_pUnpackOperationState) && m_setUnpackContexts.isEmpty();
    }

private:
    bool discover(INTERNAL_INFO *pInfo, PDSTRUCT *pPdStruct);
    bool appendBlob(INTERNAL_INFO *pInfo, qint64 nOffset, qint64 nSize, const QString &sHint, PDSTRUCT *pPdStruct);
    bool bindLocation(UNPACK_CONTEXT *pContext, qint32 nGlobalIndex, PDSTRUCT *pPdStruct);
    bool releaseBlob(UNPACK_CONTEXT *pContext);
    bool isOwnContext(const UNPACK_STATE *pState) const;
    static QString resourceToken(bool bIsName, quint32 nID, const QString &sName);

private:
    INTERNAL_INFO m_internalInfo;
    QSharedPointer<bool> m_pUnpackOperationState;
    QSharedPointer<UNPACK_DEFERRED_CLEANUP> m_pUnpackDeferredCleanup;
    QSet<UNPACK_CONTEXT *> m_setUnpackContexts;
};

#endif  // XSPISSFX_H
