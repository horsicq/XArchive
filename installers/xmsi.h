/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XMSI_H
#define XMSI_H

#include "xbinary.h"

class SubDevice;
class XArchive;
class QIODevice;

/* Detector + extractor for Windows Installer (MSI) databases.  MSI tables are
 * decoded far enough to map File rows to Media cabinets, so the streaming
 * archive API exposes installed files rather than implementation-only OLE
 * streams. */

class XMSI : public XBinary {
    Q_OBJECT

public:
    struct INTERNAL_INFO : public XBinary::INTERNAL_INFO {
        bool bIsValid;
        QString sVersion;  // "database" / "patch" / "transform"
    };

    struct CAB_CONTEXT {
        QIODevice *pDevice;
        XArchive *pArchive;
        UNPACK_STATE state;
    };

    struct PAYLOAD_ENTRY {
        qint32 nCabinetIndex;
        qint32 nCabinetRecordIndex;
        qint32 nSequence;
        qint32 nTableOrder;
        qint64 nSize;
        QString sName;
        QString sExternalPath;
        QByteArray baExternalFingerprint;
    };

    struct UNPACK_CONTEXT {
        QPointer<QIODevice> pOuterSourceDevice;
        quint64 nOwnerDeviceGeneration;
        UNPACK_STATE *pOwnerState = nullptr;
        bool bPayloadMode;
        SubDevice *pSubDevice;
        XArchive *pArchive;
        UNPACK_STATE innerState;
        XArchive *pSourceValidator;
        UNPACK_STATE sourceValidationState;
        QList<CAB_CONTEXT *> listCabinets;
        QList<PAYLOAD_ENTRY> listEntries;
    };

    struct UNPACK_DEFERRED_CLEANUP {
        ~UNPACK_DEFERRED_CLEANUP();
        QSet<UNPACK_CONTEXT *> setContexts;
    };

    explicit XMSI(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XMSI() override;

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

    // Advanced Installer bootstrappers can carry a CAB next to the embedded
    // MSI inside their own container.  Supplying it here avoids temporary files
    // and keeps normal external-CAB resolution restricted to the MSI directory.
    void setExternalCabinetData(const QMap<QString, QByteArray> &mapData);

protected:
    bool isDeviceReplacementAllowed() const override
    {
        return (!m_pUnpackOperationState || !*m_pUnpackOperationState) && m_setUnpackContexts.isEmpty();
    }

private:
    INTERNAL_INFO _getInternalInfo(PDSTRUCT *pPdStruct);
    INTERNAL_INFO m_internalInfo;
    INTERNAL_INFO _detect(PDSTRUCT *pPdStruct);
    QMap<QString, QByteArray> m_mapExternalCabinetData;
    QSharedPointer<bool> m_pUnpackOperationState;
    QSharedPointer<UNPACK_DEFERRED_CLEANUP> m_pUnpackDeferredCleanup;
    QSet<UNPACK_CONTEXT *> m_setUnpackContexts;
};

#endif  // XMSI_H
