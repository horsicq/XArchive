/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * SPDX-License-Identifier: MIT
 */
#ifndef XVIRTUALDISKARCHIVE_H
#define XVIRTUALDISKARCHIVE_H

#include "xarchive.h"

// Raw-disk export is the default. UNPACK_PROP_DISK_FILESYSTEM selects bounded
// NTFS guest-file extraction through a seekable allocation view; nothing is mounted.
class XVirtualDiskArchive : public XArchive {
public:
    enum KIND { KIND_VHD, KIND_VDI, KIND_QCOW2, KIND_VHDX };
    explicit XVirtualDiskArchive(QIODevice *pDevice = nullptr, KIND kind = KIND_VHD);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    FT getFileType() override;
    QString getFileFormatExt() override;
    QString getMIMEString() override;
    QString getVersion() override;
    QList<PM_INFO> unpackImplemented() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    KIND m_kind;
};

#endif
