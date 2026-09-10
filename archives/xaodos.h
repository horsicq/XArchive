/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XAODOS_H
#define XAODOS_H

#include "xarchive.h"

// AO-DOS / MicroDOS floppy image for the Elektronika BK-0010/BK-0011M
// (PDP-11 compatible).  A raw 800 KiB sector dump with a fixed directory at
// byte 0x140; every member is a contiguous run of 512-byte blocks, so nothing
// in the container is compressed and the whole format is a filesystem walk.
class XAODOS final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nEntryOffset;
        qint64 nDataOffset;
        qint64 nDataSize;
        quint16 nEntryWord;
        quint16 nStartBlock;
        quint16 nBlockCount;
        quint16 nLoadAddress;
        bool bIsDirectory;
        QString sPath;
    };

    explicit XAODOS(QIODevice *pDevice = nullptr);
    ~XAODOS() override;

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
    QList<FPART_PROP> getAvailableFPARTProperties() override;

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
    // One raw 24-byte directory slot, before deleted entries are dropped and
    // before the parent-directory tree is resolved.
    struct ENTRY {
        qint64 nEntryOffset;
        quint16 nEntryWord;
        quint16 nStartBlock;
        quint16 nBlockCount;
        quint16 nLoadAddress;
        quint16 nSizeBytes;
        QByteArray baName;
    };

    struct CONTEXT {
        qint64 nInputSize;
        qint64 nTotalBlocks;
        qint64 nDirectoryOffset;
        qint64 nDirectorySize;
        qint32 nRawEntryCount;
        quint16 nDeclaredEntryCount;
        quint16 nFirstFreeBlock;
        quint16 nFreeBlockCount;
        QString sVersion;
        QList<MEMBER> listMembers;
    };

    // Resolves directory index -> output path, memoized, with a hop cap so a
    // corrupt parent cycle cannot recurse forever.
    struct TREE {
        QMap<quint32, quint32> mapParent;
        QMap<quint32, QString> mapLeaf;
        QMap<quint32, QString> mapPath;
        QSet<QString> stUsedPaths;

        bool resolve(quint32 nIndex, QString *pPath, qint32 nDepth);
        bool claimPath(const QString &sParent, const QString &sLeaf,
                       QString *pResult);
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString decodeKOI8Name(const QByteArray &baName);
    static QString readVersion(const QByteArray &baBootBlock);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XAODOS_H
