/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XINSTALLFORGE_H
#define XINSTALLFORGE_H

#include <QStringList>

#include "xbinary.h"

class SubDevice;
class XArchive;

/* Detector + extractor for InstallForge installers. The MSVC PE stub stores its
 * payload in the overlay behind a 13-byte "IFSETUP_START" (+1) marker and an
 * 8-byte little-endian length, followed by either 7-Zip or a compressed TAR.
 * InstallForge obfuscates entry names as base64(UTF-16LE(name)); the wrapper
 * decodes those names before exposing archive records. */

class XInstallForge : public XBinary {
    Q_OBJECT

public:
    enum PAYLOAD {
        PAYLOAD_UNKNOWN = 0,
        PAYLOAD_7Z,
        PAYLOAD_BZIP2,
        PAYLOAD_GZIP
    };

    struct INTERNAL_INFO : public XBinary::INTERNAL_INFO {
        bool bIsValid;
        QString sVersion;
        qint64 nArchiveOffset;
        qint64 nArchiveSize;
        PAYLOAD payload;  // container format of the file payload (varies by compressor)
    };

    struct UNPACK_CONTEXT {
        QPointer<QIODevice> pOuterSourceDevice;
        quint64 nOwnerDeviceGeneration;
        UNPACK_STATE *pOwnerState = nullptr;
        SubDevice *pSubDevice;
        XArchive *pArchive;
        UNPACK_STATE innerState;
        QStringList listDecodedNames;
    };

    struct UNPACK_DEFERRED_CLEANUP {
        ~UNPACK_DEFERRED_CLEANUP();
        QSet<UNPACK_CONTEXT *> setContexts;
    };

    explicit XInstallForge(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XInstallForge() override;

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
    INTERNAL_INFO _getInternalInfo(PDSTRUCT *pPdStruct);
    INTERNAL_INFO m_internalInfo;
    INTERNAL_INFO _detect(PDSTRUCT *pPdStruct);
    QSharedPointer<bool> m_pUnpackOperationState;
    QSharedPointer<UNPACK_DEFERRED_CLEANUP> m_pUnpackDeferredCleanup;
    QSet<UNPACK_CONTEXT *> m_setUnpackContexts;
};

#endif  // XINSTALLFORGE_H
