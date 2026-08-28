/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef XEXTERNALARCHIVE_H
#define XEXTERNALARCHIVE_H

#include <QDeadlineTimer>

#include "xarchive.h"

// Streaming adapter for archive/codec backends installed with PeaZip.  Helper
// processes only ever see private temporary input/output paths; caller-owned
// output devices remain behind XArchive's transactional publication boundary.
class XExternalArchive : public XArchive {
public:
    enum BACKEND {
        BACKEND_UNKNOWN = 0,
        BACKEND_ZPAQ,
        BACKEND_BCM,
        BACKEND_LPAQ8,
        BACKEND_PEA,
        BACKEND_FREEARC
    };

    enum EXTERNAL_FAILURE {
        EXTERNAL_FAILURE_NONE = 0,
        EXTERNAL_FAILURE_ARCHIVE_REJECTED,
        EXTERNAL_FAILURE_PASSWORD,
        EXTERNAL_FAILURE_TIMEOUT,
        EXTERNAL_FAILURE_RESOURCE_LIMIT,
        EXTERNAL_FAILURE_CANCELED,
        EXTERNAL_FAILURE_INFRASTRUCTURE
    };

    explicit XExternalArchive(QIODevice *pDevice, BACKEND backend);

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState,
                    const QMap<UNPACK_PROP, QVariant> &mapProperties,
                    PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                       PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

    // XSFX gives all provisional candidates in one operation the same
    // monotonic deadline. Ordinary direct archive use leaves this at Forever
    // and receives the configured per-helper timeout.
    static qint64 getConfiguredHelperTimeoutMs();
    static QDeadlineTimer createHelperDeadline();
    void setHelperDeadline(const QDeadlineTimer &deadline);
    void clearHelperDeadline();
    EXTERNAL_FAILURE getLastExternalFailure() const;
    bool isDeferredArchiveMaterialized(const UNPACK_STATE *pState) const;

    // Authenticates/materializes a metadata-only FreeArc/ZPAQ context without
    // publishing a member. Used for empty and directory-only provisional SFX
    // candidates, whose normal streaming API has no file body to trigger it.
    bool verifyDeferredArchive(UNPACK_STATE *pState,
                               PDSTRUCT *pPdStruct = nullptr);

protected:
    BACKEND getExternalBackend() const;
    void setLastExternalFailure(EXTERNAL_FAILURE failure);

private:
    struct EXTERNAL_UNPACK_CONTEXT;
    bool _materializeDeferredArchive(EXTERNAL_UNPACK_CONTEXT *pContext,
                                     UNPACK_STATE *pState,
                                     PDSTRUCT *pPdStruct);

    BACKEND m_backend;
    QDeadlineTimer m_helperDeadline;
    EXTERNAL_FAILURE m_lastExternalFailure;
};

#endif  // XEXTERNALARCHIVE_H
