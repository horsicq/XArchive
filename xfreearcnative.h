/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * MIT License
 */
#ifndef XFREEARCNATIVE_H
#define XFREEARCNATIVE_H

#include "xbinary.h"

// Native control-block reader for FreeArc. XExternalArchive retains the
// optional helper for method chains outside the native decoder's scope.
class XFreeArcNative {
public:
    enum RESULT { INVALID, UNSUPPORTED, RESOURCE_LIMIT, CANCELED, READY };
    struct ENTRY {
        QString name;
        qint64 size = 0;
        quint32 time = 0;
        quint32 crc = 0;
        bool folder = false;
        qint32 block = -1;
        qint64 offset = 0;
        QString stagedPath;
    };
    RESULT open(QIODevice *device, const XBinary::OUTPUT_POLICY &policy, XBinary::PDSTRUCT *pd);
    bool materialize(const QString &directory, XBinary::PDSTRUCT *pd);
    const QList<ENTRY> &entries() const { return m_entries; }

private:
    struct BLOCK {
        qint64 offset = 0;
        qint64 packed = 0;
        qint64 unpacked = 0;
        QByteArray method;
    };
    bool chargeMetadata(qint64 amount, qint64 liveBufferBytes = 0);
    qint64 m_memoryLimit = 0;
    qint64 m_metadataCharge = 0;
    QPointer<QIODevice> m_device;
    QList<ENTRY> m_entries;
    QList<BLOCK> m_blocks;
    XBinary::OUTPUT_POLICY m_policy = {};
};

#endif
