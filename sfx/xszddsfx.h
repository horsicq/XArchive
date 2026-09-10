/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSZDDSFX_H
#define XSZDDSFX_H

#include "xsfx.h"

class SubDevice;
class XSZDD;

class XSzddSFX : public XSFX {
    Q_OBJECT

public:
    struct SZDDSFX_ENTRY {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nStreamSize;
        qint64 nUncompressedSize;
        quint8 nMissingChar;
        QString sName;
    };

    struct SZDDSFX_UNPACK_CONTEXT {
        QList<SZDDSFX_ENTRY> listEntries;
        QPointer<QIODevice> pOuterSourceDevice;
        quint64 nOwnerDeviceGeneration;
        UNPACK_STATE *pOwnerState;
        SubDevice *pSubDevice;
        XSZDD *pArchive;
        UNPACK_STATE innerState;
        QMap<UNPACK_PROP, QVariant> mapUnpackProperties;
    };

    struct SZDDSFX_UNPACK_DEFERRED_CLEANUP {
        ~SZDDSFX_UNPACK_DEFERRED_CLEANUP();
        QSet<SZDDSFX_UNPACK_CONTEXT *> setContexts;
    };

    explicit XSzddSFX(QIODevice *pDevice = nullptr, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XSzddSFX() override;

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
        return (!m_pSzddUnpackOperationState || !*m_pSzddUnpackOperationState) && m_setSzddUnpackContexts.isEmpty();
    }

private:
    bool _scanStreams(QList<SZDDSFX_ENTRY> *pList, PDSTRUCT *pPdStruct);
    QList<QString> _recoverNames(qint32 nExpectedCount, qint64 nFirstHeaderOffset, PDSTRUCT *pPdStruct);
    bool _bindEntry(SZDDSFX_UNPACK_CONTEXT *pContext, qint32 nIndex, PDSTRUCT *pPdStruct);
    bool _releaseEntry(SZDDSFX_UNPACK_CONTEXT *pContext);

    QSharedPointer<bool> m_pSzddUnpackOperationState;
    QSharedPointer<SZDDSFX_UNPACK_DEFERRED_CLEANUP> m_pSzddUnpackDeferredCleanup;
    QSet<SZDDSFX_UNPACK_CONTEXT *> m_setSzddUnpackContexts;
};

#endif  // XSZDDSFX_H
