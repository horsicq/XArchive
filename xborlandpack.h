/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBORLANDPACK_H
#define XBORLANDPACK_H

#include "xarchive.h"

// Borland PACK/UNPACK container ("This is a packed file." + ":CM nn nnn:"),
// the .ARC shipped with Borland's DOS language products.  Unrelated to
// SEA/PKWARE ARC despite the extension; the whole container is ASCII text and
// carries no binary integer at all.  The per-member 4-hex-digit checksum is
// exposed for information only - its algorithm is not identified, so nothing
// in the class may be gated on it.
class XBorlandPack final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint16 nChecksum;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };

    explicit XBorlandPack(QIODevice *pDevice = nullptr);
    ~XBorlandPack() override;

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
        quint32 nMethod;
        quint32 nVersion;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(quint32 nMethod);
    static HANDLE_METHOD methodToHandleMethod(quint32 nMethod);
    static QString versionToString(quint32 nVersion);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XBORLANDPACK_H
