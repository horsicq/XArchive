/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSEVENZIPSFX_H
#define XSEVENZIPSFX_H

#include "xbinary.h"

class SubDevice;
class XArchive;

/* Detector + extractor for 7-Zip self-extracting archives (7z.sfx / 7zCon.sfx
 * GUI/console modules and the 7zSD.sfx installer SFX). The PE stub is followed
 * by a 7z archive in the overlay, optionally preceded by a ";!@Install@!UTF-8!"
 * config block. Detection keys on the overlay marker; extraction delegates to
 * the XArchive 7z handler on the located archive region. */

class XSevenZipSFX : public XBinary {
    Q_OBJECT

public:
    struct INTERNAL_INFO : public XBinary::INTERNAL_INFO {
        bool bIsValid;
        QString sVersion;
        qint64 nArchiveOffset;
        qint64 nArchiveSize;
    };

    struct UNPACK_CONTEXT {
        QPointer<QIODevice> pOuterSourceDevice;
        quint64 nOwnerDeviceGeneration;
        UNPACK_STATE *pOwnerState = nullptr;
        SubDevice *pSubDevice;
        XArchive *pArchive;
        UNPACK_STATE innerState;
    };

    struct UNPACK_DEFERRED_CLEANUP {
        ~UNPACK_DEFERRED_CLEANUP();
        QSet<UNPACK_CONTEXT *> setContexts;
    };

    explicit XSevenZipSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XSevenZipSFX() override;

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

#endif  // XSEVENZIPSFX_H
