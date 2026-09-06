/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XAMPK_H
#define XAMPK_H

#include "xarchive.h"

// AMPK: big-endian Amiga multi-file archive (container revisions 2, 3 and 4)
// found on German Amiga-Magazin cover disks.  The container carries a real
// directory tree: file records are listed with their '/'-joined path, while the
// directory records themselves are only path prefixes and are not listed as
// members (an empty directory therefore leaves no trace, which is also what the
// original unpacker does).
class XAMPK final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nDataSize;          // bytes actually stored in the container
        qint64 nDeclaredPackedSize;  // the record's compressedSize field
        qint64 nUncompressedSize;
        quint8 nMethod;
        quint8 nAttributes;  // AmigaDOS protection bits; informational
        QString sFileName;
    };

    explicit XAMPK(QIODevice *pDevice = nullptr);
    ~XAMPK() override;

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
        qint64 nFirstMemberOffset;
        quint8 nVersion;
        quint16 nDeclaredDirectoryCount;
        quint16 nDeclaredFileCount;
        quint32 nDeclaredUncompressedSize;
        quint32 nDeclaredDataSize;
        qint32 nDirectoryCount;
        // False when the record walk stopped before the end of the file or a
        // header cross-check failed: the archive is damaged and only the
        // members collected so far can be trusted.
        bool bComplete;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(quint8 nMethod);
    static HANDLE_METHOD methodToHandleMethod(quint8 nMethod);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XAMPK_H
