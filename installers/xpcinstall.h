/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XPCINSTALL_H
#define XPCINSTALL_H

#include <QHash>

#include "xarchive.h"

// PC-Install ("[20/20]") disk-set installer container, shipped as DISK1.BND /
// DISK2.BND floppy sets by a mid-1990s Windows 3.1/95 setup builder.  Detect It
// Easy already calls this data blob "PCInstall", so the class keeps that name.
//
// The container has no head magic: the first sixteen bytes are the volume
// header block (zero-filled on every plain .BND) and the only invariant tag is
// a sixteen-byte trailer at EOF-16.  Detection is therefore a tail probe plus a
// full chain walk; there is nothing cheaper that is also safe.
//
// Every payload is a raw PKWARE DCL "implode" stream and the container stores
// no uncompressed size anywhere, so the raw size has to be discovered by
// decoding.  That is deliberately kept out of isValid()/getFileFormatSize().
class XPCInstall final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nMemberHeaderOffset;
        qint64 nMemberHeaderSize;
        qint64 nDataOffset;
        qint64 nDataSize;              // bytes physically present in this volume
        qint64 nDeclaredCompressedSize;  // whole stream length; -1 when unknown
        quint32 nAttributes;
        quint16 nRecordType;
        quint16 nDosDate;
        quint16 nDosTime;
        bool bComplete;  // the whole stream lives inside this volume
        QString sFileName;
        QString sSourceName;
        QString sVolumeLinkName;
    };

    explicit XPCInstall(QIODevice *pDevice = nullptr);
    ~XPCInstall() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN,
                             PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1,
                              PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState,
                    const QMap<UNPACK_PROP, QVariant> &mapProperties,
                    PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nFirstRecordOffset;
        qint64 nLastRecordOffset;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    qint64 resolveUncompressedSize(const MEMBER &member, PDSTRUCT *pPdStruct);
    void fillMemberProperties(const MEMBER &member,
                              QMap<FPART_PROP, QVariant> *pMapProperties,
                              PDSTRUCT *pPdStruct);
    static QString methodToString(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);

    // Discovering a member's raw size costs a full decode.  Memoise it per
    // stream offset so a listing followed by an extraction (or several
    // getFileParts() calls from the memory-map modes) does not decompress the
    // whole volume again each time.
    QHash<qint64, qint64> m_mapUncompressedSizes;
};

#endif  // XPCINSTALL_H
