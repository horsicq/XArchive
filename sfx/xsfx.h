/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSFX_H
#define XSFX_H

#include "xarchive.h"

class SubDevice;
struct XSFX_ZPAQ_SCAN_CACHE;
struct XSFX_FREEARC_SCAN_CACHE;

/* XSFX detects self-extracting archives in executable overlays and, for
 * explicitly selected resource-container families, in the mapped executable
 * image. It locates the archive and delegates the streaming unpack API to the
 * matching XArchive handler, presenting only the archive region through a
 * SubDevice. */

class XSFX : public XBinary {
    Q_OBJECT

public:
    struct INTERNAL_INFO : public XBinary::INTERNAL_INFO {
        bool bIsValid;
        bool bProvisional;
        bool bResourceIndeterminate;
        bool bAllowOpaqueZpaq;
        bool bUseOuterDevice;
        FT arcType;
        qint64 nArchiveOffset;
        qint64 nArchiveSize;
    };

    struct UNPACK_CONTEXT {
        QIODevice *pOuterSourceDevice = nullptr;
        quint64 nOwnerDeviceGeneration;
        UNPACK_STATE *pOwnerState = nullptr;
        INTERNAL_INFO info;
        SubDevice *pSubDevice;
        XArchive *pArchive;
        UNPACK_STATE innerState;
        // The public outer state never retains helper passwords. Keep a
        // sanitized retry map plus one detached credential copy private until
        // the provisional candidate is authenticated, then scrub it.
        QMap<UNPACK_PROP, QVariant> mapPrivateUnpackProperties;
        QString sPrivatePassword;
        QByteArray baPrivatePassword;
    };

    struct UNPACK_DEFERRED_CLEANUP {
        ~UNPACK_DEFERRED_CLEANUP();
        QSet<UNPACK_CONTEXT *> setContexts;
    };

    explicit XSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XSFX() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);

    // True when the device carries an LHarc or LArc self-extracting stub whose
    // embedded archive this class can locate WITHOUT any container identity -
    // the carrier is a raw DOS COM image, which has no signature and is
    // otherwise recognised only by a ".COM" file name.  It reads at most 1300
    // bytes and matches nothing else: over the 42,616-file reference corpus it
    // fires on 436 of 436 "LHARC SFX", 33 of the "SFX LHA" carriers and ZERO
    // files in the other 352 families.  Callers that gate the XSFX probe on a
    // detected executable type can use this as an additional entry so a stub
    // whose file name lost its extension is still reached.
    static bool isLhaSfxStubCarrier(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    virtual bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    virtual void *getInternalInfo(PDSTRUCT *pPdStruct = nullptr) override;
    virtual void setInternalInfo(void *pInternalInfo) override;

    virtual FT getFileType() override;
    virtual QString getArch() override;
    virtual MODE getMode() override;
    virtual QString getMIMEString() override;

    virtual QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

protected:
    explicit XSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress, FT requiredArcType);

private:
    struct SCAN_CANDIDATE_EVALUATOR;

    INTERNAL_INFO _getInternalInfo(PDSTRUCT *pPdStruct);
    INTERNAL_INFO m_internalInfo;
    INTERNAL_INFO _detect(PDSTRUCT *pPdStruct, XSFX_ZPAQ_SCAN_CACHE *pZpaqScanCache = nullptr, XSFX_FREEARC_SCAN_CACHE *pFreeArcScanCache = nullptr,
                          qint64 nMinimumArchiveOffset = -1);
    INTERNAL_INFO _detectScan(PDSTRUCT *pPdStruct, XSFX_ZPAQ_SCAN_CACHE *pZpaqScanCache, XSFX_FREEARC_SCAN_CACHE *pFreeArcScanCache, qint64 nMinimumArchiveOffset);
    bool _matchArchiveAt(qint64 nOffset, qint64 nSize, FT *pType, qint64 *pArchiveSize, PDSTRUCT *pPdStruct, XSFX_ZPAQ_SCAN_CACHE *pZpaqScanCache,
                         XSFX_FREEARC_SCAN_CACHE *pFreeArcScanCache, bool *pbProvisional, bool *pbResourceIndeterminate, bool *pbUseOuterDevice);
    XArchive *_createArchive(FT arcType, QIODevice *pDevice, bool bAllowOpaqueZpaq = false);
    FT m_requiredArcType;
};

#endif  // XSFX_H
