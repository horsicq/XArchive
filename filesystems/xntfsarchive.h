/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * SPDX-License-Identifier: MIT
 */
#ifndef XNTFSARCHIVE_H
#define XNTFSARCHIVE_H
#include "xarchive.h"

// Read-only, bounded NTFS user-file extraction. Accepts a raw volume or an MBR
// disk containing NTFS primary partitions; no filesystem is mounted.
class XNTFSArchive : public XArchive {
public:
    explicit XNTFSArchive(QIODevice *device = nullptr);
    XBinary *createInstance(QIODevice *device, bool image = false, XADDR address = -1) override;
    bool isValid(PDSTRUCT *pd = nullptr) override;
    FT getFileType() override;
    QString getFileFormatExt() override;
    QString getMIMEString() override;
    QString getVersion() override;
    QList<PM_INFO> unpackImplemented() override;
    bool initUnpack(UNPACK_STATE *state, const QMap<UNPACK_PROP, QVariant> &properties, PDSTRUCT *pd = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *state, PDSTRUCT *pd = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *state, QIODevice *output, PDSTRUCT *pd = nullptr) override;
    bool moveToNext(UNPACK_STATE *state, PDSTRUCT *pd = nullptr) override;
    bool finishUnpack(UNPACK_STATE *state, PDSTRUCT *pd = nullptr) override;
    QIODevice *ntfsDataDevice();
private:
    // Only the disk reader can supply a different logical view. The immutable
    // disk allocation plan is authenticated through the original source's
    // normal XArchive snapshot, including its complete backing-device chain.
    friend class XVirtualDiskArchive;
    XNTFSArchive(QIODevice *identityDevice, QIODevice *logicalDevice);
    QIODevice *m_logicalDevice = nullptr;
    bool m_mapped = false;
};
#endif
