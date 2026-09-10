/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XKWAJSFX_H
#define XKWAJSFX_H

#include "xsfx.h"

class SubDevice;
class XKWAJ;

class XKwajSFX : public XSFX {
    Q_OBJECT

public:
    struct KWAJSFX_ENTRY {
        qint64 nResourceOffset;
        qint64 nResourceSize;
        quint32 nType;
        quint32 nID;
        bool bKwaj;
        QString sName;
        QString sGroupKey;
    };

    struct KWAJSFX_UNPACK_CONTEXT {
        QList<KWAJSFX_ENTRY> listEntries;
        QPointer<QIODevice> pOuterSourceDevice;
        quint64 nOwnerDeviceGeneration;
        UNPACK_STATE *pOwnerState;
        SubDevice *pSubDevice;
        XKWAJ *pArchive;
        UNPACK_STATE innerState;
        QMap<UNPACK_PROP, QVariant> mapUnpackProperties;
    };

    struct KWAJSFX_UNPACK_DEFERRED_CLEANUP {
        ~KWAJSFX_UNPACK_DEFERRED_CLEANUP();
        QSet<KWAJSFX_UNPACK_CONTEXT *> setContexts;
    };

    explicit XKwajSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XKwajSFX() override;

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
        return XSFX::isDeviceReplacementAllowed() && (!m_pKwajUnpackOperationState || !*m_pKwajUnpackOperationState) && m_setKwajUnpackContexts.isEmpty();
    }

private:
    bool _buildResourceEntries(QList<KWAJSFX_ENTRY> *pList, PDSTRUCT *pPdStruct);
    bool _bindEntry(KWAJSFX_UNPACK_CONTEXT *pContext, qint32 nIndex, PDSTRUCT *pPdStruct);
    bool _releaseEntry(KWAJSFX_UNPACK_CONTEXT *pContext);
    bool _isOwnContext(const UNPACK_STATE *pState) const;

    QSharedPointer<bool> m_pKwajUnpackOperationState;
    QSharedPointer<KWAJSFX_UNPACK_DEFERRED_CLEANUP> m_pKwajUnpackDeferredCleanup;
    QSet<KWAJSFX_UNPACK_CONTEXT *> m_setKwajUnpackContexts;
};

#endif  // XKWAJSFX_H
