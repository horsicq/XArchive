/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XARJSFX_H
#define XARJSFX_H

#include "xsfx.h"

class SubDevice;
class XARJ;

class XArjSFX : public XSFX {
    Q_OBJECT

public:
    struct ARJSFX_ENTRY {
        qint64 nArchiveOffset;
        qint64 nArchiveSize;
        qint32 nNumberOfRecords;
    };

    struct ARJSFX_UNPACK_CONTEXT {
        QList<ARJSFX_ENTRY> listEntries;
        QPointer<QIODevice> pOuterSourceDevice;
        quint64 nOwnerDeviceGeneration;
        UNPACK_STATE *pOwnerState;
        qint32 nEntryIndex;
        qint32 nEntryFirstRecord;
        SubDevice *pSubDevice;
        XARJ *pArchive;
        UNPACK_STATE innerState;
        QMap<UNPACK_PROP, QVariant> mapUnpackProperties;
    };

    struct ARJSFX_UNPACK_DEFERRED_CLEANUP {
        ~ARJSFX_UNPACK_DEFERRED_CLEANUP();
        QSet<ARJSFX_UNPACK_CONTEXT *> setContexts;
    };

    explicit XArjSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XArjSFX() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

protected:
    bool isDeviceReplacementAllowed() const override
    {
        return XSFX::isDeviceReplacementAllowed() && (!m_pArjUnpackOperationState || !*m_pArjUnpackOperationState) && m_setArjUnpackContexts.isEmpty();
    }

private:
    bool _scanArchives(QList<ARJSFX_ENTRY> *pList, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct);
    bool _bindEntry(ARJSFX_UNPACK_CONTEXT *pContext, qint32 nEntryIndex, PDSTRUCT *pPdStruct);
    bool _releaseEntry(ARJSFX_UNPACK_CONTEXT *pContext);

    QSharedPointer<bool> m_pArjUnpackOperationState;
    QSharedPointer<ARJSFX_UNPACK_DEFERRED_CLEANUP> m_pArjUnpackDeferredCleanup;
    QSet<ARJSFX_UNPACK_CONTEXT *> m_setArjUnpackContexts;
};

#endif  // XARJSFX_H
