/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XTARMA_H
#define XTARMA_H

#include <QSet>
#include <QSharedPointer>

#include "xbinary.h"

class XMaterializedUnpackGuard;

/* Detector + extractor for Tarma InstallMate installers.
 *
 * Tarma InstallMate (Setup Utility) marks its PE with two dedicated sections,
 * ".tsustub" (the loader) and ".tsuarch" (the packed payload), and stores a
 * "TIZ" container inside .tsuarch: at section raw + 0x10 the bytes are
 * "tiz" + <generation digit> + "z" + 00 + <version word>. The v9 "tiz3z"
 * container is a *standard raw LZMA1* stream (props at +0x48, stream at +0x4D)
 * and "tiz2z" is zlib (stream at +0x48). Their decoded inner is a sequence of
 * "tzf3" blocks. File records in the leading tin9 database provide real names
 * and sizes, which are paired with payload blocks in the executable or sibling
 * DiskNNNN.tiz volumes. */

class XTarma : public XBinary {
    Q_OBJECT

public:
    struct INTERNAL_INFO : public XBinary::INTERNAL_INFO {
        bool bIsValid;
        QString sVersion;
        bool bIsLegacy;     // legacy tiz1 overlay layout
        bool bExtractable;  // tiz2z (zlib), tiz3z (LZMA), or legacy tiz1
        qint64 nContainerOffset;
    };

    struct FILE_ENTRY {
        QString sName;
        QByteArray baData;
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

    explicit XTarma(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XTarma() override;

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
    bool _buildEntries(UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct);
    QSharedPointer<LIFETIME_STATE> m_pUnpackLifetimeState;
    bool m_bTrustedSnapshot = false;
};

#endif  // XTARMA_H
