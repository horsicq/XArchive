/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XMATERIALIZEDUNPACKGUARD_H
#define XMATERIALIZEDUNPACKGUARD_H

#include <memory>
#include <new>

#include <QFile>
#include <QList>
#include "xarchive.h"

// Materialized handlers decode into private memory during initUnpack(), then
// serve those cached bytes later.  Keep the same complete source identity and
// content authentication used by streaming archives for the whole lifetime of
// that cached context.  The UNPACK_STATE below is private and has a stable heap
// address, so copied public states cannot authorize or release the session.
class XMaterializedUnpackGuard {
public:
    static XMaterializedUnpackGuard *bind(QIODevice *pDevice, XBinary::PDSTRUCT *pPdStruct = nullptr, bool bOwnDevice = false)
    {
        if (!pDevice) return nullptr;
        // Establish the lifetime guard before XArchive's constructor or
        // source binding can invoke caller-controlled QIODevice callbacks.
        // A scoped smart pointer would retain a dangling address if such a callback
        // destroys an owned device and would delete it a second time while
        // unwinding the failed bind.
        QIODevice *guardedDevice = pDevice;
        QIODevice *guardedOwnedDevice = bOwnDevice ? pDevice : nullptr;
        std::unique_ptr<XMaterializedUnpackGuard> pResult;
        try {
            pResult.reset(new XMaterializedUnpackGuard(guardedDevice));
        } catch (const std::bad_alloc &) {
            if (guardedOwnedDevice) delete guardedOwnedDevice;
            return nullptr;
        }
        if (bOwnDevice) {
            pResult->m_ownedDevice.track(guardedOwnedDevice);
        }
        if (!guardedDevice || !pResult->m_archive.bindUnpackSource(&pResult->m_state, pPdStruct)) {
            return nullptr;
        }
        pResult->m_bBound = true;
        return pResult.release();
    }

    static XMaterializedUnpackGuard *openFile(const QString &sFileName, XBinary::PDSTRUCT *pPdStruct = nullptr)
    {
        std::unique_ptr<QFile> pFile(new (std::nothrow) QFile(sFileName));
        if (!pFile || !pFile->open(QIODevice::ReadOnly)) return nullptr;
        return bind(pFile.release(), pPdStruct, true);
    }

    ~XMaterializedUnpackGuard()
    {
        if (m_bBound) {
            m_archive.releaseUnpackSource(&m_state);
            m_bBound = false;
        }
    }

    bool validateAndFinalize(XBinary::PDSTRUCT *pPdStruct = nullptr)
    {
        if (!m_bBound || m_bFinalized || !m_archive.validateAndFinalizeUnpackSource(&m_state, pPdStruct)) return false;
        m_bFinalized = true;
        return true;
    }

    bool isCurrent(XBinary::PDSTRUCT *pPdStruct = nullptr)
    {
        return m_bBound && m_bFinalized && m_archive.isUnpackSourceCurrent(&m_state, pPdStruct);
    }

    QIODevice *device()
    {
        return m_archive.getDevice();
    }

    static bool areCurrent(XMaterializedUnpackGuard *pPrimary, const QList<XMaterializedUnpackGuard *> &listCompanions, XBinary::PDSTRUCT *pPdStruct = nullptr)
    {
        if (!pPrimary || !pPrimary->isCurrent(pPdStruct)) return false;
        for (XMaterializedUnpackGuard *pGuard : listCompanions) {
            if (!pGuard || !pGuard->isCurrent(pPdStruct)) return false;
        }
        return true;
    }

private:
    class TRACKED_DEVICE_OWNER {
    public:
        TRACKED_DEVICE_OWNER() = default;

        ~TRACKED_DEVICE_OWNER()
        {
            // QObject clears QPointer synchronously.  If a source callback
            // already destroyed the device, there is nothing left to own.
            QIODevice *pDevice = m_pDevice;
            m_pDevice = nullptr;
            delete pDevice;
        }

        void track(QIODevice *pDevice)
        {
            m_pDevice = pDevice;
        }

    private:
        Q_DISABLE_COPY(TRACKED_DEVICE_OWNER)
        QIODevice *m_pDevice = nullptr;
    };

    explicit XMaterializedUnpackGuard(QIODevice *pDevice) : m_archive(pDevice)
    {
    }

    Q_DISABLE_COPY(XMaterializedUnpackGuard)

    // Keep ownership before the archive so reverse member destruction retains
    // the original cleanup order: release the session, destroy XArchive, then
    // delete a still-live owned device.
    TRACKED_DEVICE_OWNER m_ownedDevice;
    XArchive m_archive;
    XBinary::UNPACK_STATE m_state = {};
    bool m_bBound = false;
    bool m_bFinalized = false;
};

#endif  // XMATERIALIZEDUNPACKGUARD_H
