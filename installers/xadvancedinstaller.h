/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XADVANCEDINSTALLER_H
#define XADVANCEDINSTALLER_H

#include "xbinary.h"

class XMSI;
class XArchive;

/* Detector + extractor for Advanced Installer (Caphyon) output. Two shapes:
 *  - the bootstrapper .exe, marked by the 10-byte EOF trailer "ADVINSTSFX";
 *  - the authored .msi, an MSI database carrying the "aicustact.dll" custom
 *    action + the "AI_" property namespace + "Advanced Installer" strings.
 * MSI payload extraction is delegated to XMSI. For an ExeInside bootstrapper,
 * the embedded MSI and CAB records are decoded first; an external bootstrapper
 * is resolved only to the exact, safe sibling MSI named by its footer. */

class XAdvancedInstaller : public XBinary {
    Q_OBJECT

public:
    enum SUBTYPE {
        SUBTYPE_UNKNOWN = 0,
        SUBTYPE_EXE,
        SUBTYPE_MSI
    };

    struct INTERNAL_INFO : public XBinary::INTERNAL_INFO {
        bool bIsValid;
        SUBTYPE subType;
        QString sVersion;
    };

    struct UNPACK_CONTEXT {
        QPointer<QIODevice> pOuterSourceDevice;
        quint64 nOwnerDeviceGeneration = 0;
        UNPACK_STATE *pOwnerState = nullptr;
        QPointer<QIODevice> pOwnedDevice;
        XMSI *pMSI = nullptr;
        UNPACK_STATE innerState = {};
        XArchive *pSourceValidator = nullptr;
        UNPACK_STATE sourceValidationState = {};
        bool bInnerInitialized = false;
    };

    struct UNPACK_DEFERRED_CLEANUP {
        ~UNPACK_DEFERRED_CLEANUP();
        QSet<UNPACK_CONTEXT *> setContexts;
    };

    explicit XAdvancedInstaller(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XAdvancedInstaller() override;

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
        return (!m_pUnpackOperationState || !*m_pUnpackOperationState) && m_setUnpackContexts.isEmpty();
    }

private:
    struct EXE_FOOTER {
        bool bIsValid;
        qint64 nFooterOffset;
        qint64 nMarkerOffset;
        quint32 nMode;
        quint32 nExternalNameOffset;
        quint32 nNumberOfFiles;
        quint32 nFormatVersion;
        quint32 nMetadataEndOffset;
        quint32 nInfoOffset;
        quint32 nFileDataOffset;
    };

    struct EXE_FILE {
        quint32 nType;
        quint32 nIndex;
        quint32 nXorFlag;
        quint32 nSize;
        quint32 nDataOffset;
        QString sName;
    };

    INTERNAL_INFO _getInternalInfo(PDSTRUCT *pPdStruct);
    INTERNAL_INFO m_internalInfo;
    INTERNAL_INFO _detect(PDSTRUCT *pPdStruct);
    EXE_FOOTER _readExeFooter(PDSTRUCT *pPdStruct);
    EXE_FOOTER _parseExeMarker(qint64 nSize, qint64 nFooterPrefixSize, const QByteArray &baMarker, qint64 nMarkerOffset, PDSTRUCT *pPdStruct);
    bool _readExeFiles(const EXE_FOOTER &footer, QList<EXE_FILE> *pListFiles, qint64 *pnMetadataEnd, PDSTRUCT *pPdStruct);
    bool _validateExeFooter(const EXE_FOOTER &footer, QList<EXE_FILE> *pListFiles, qint64 *pnMetadataEnd, PDSTRUCT *pPdStruct);
    QString _readUTF16Name(qint64 nOffset, quint32 nNumberOfCharacters, qint64 nLimit, PDSTRUCT *pPdStruct);
    QByteArray _readExeFile(const EXE_FILE &file, PDSTRUCT *pPdStruct);
    QString _readExternalMSIName(const EXE_FOOTER &footer, qint64 nMetadataEnd, PDSTRUCT *pPdStruct);
    QIODevice *_openExternalMSI(const QString &sName);
    bool _initMSIDelegate(UNPACK_STATE *pState, QIODevice *pSourceDevice, QIODevice *pOwnedDevice, XArchive *pSourceValidator, UNPACK_STATE *pSourceValidationState,
                          const QMap<QString, QByteArray> &mapExternalCabinets, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct);
    static bool _deleteUnpackContext(UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct);
    QSharedPointer<bool> m_pUnpackOperationState;
    QSharedPointer<UNPACK_DEFERRED_CLEANUP> m_pUnpackDeferredCleanup;
    QSet<UNPACK_CONTEXT *> m_setUnpackContexts;
};

#endif  // XADVANCEDINSTALLER_H
