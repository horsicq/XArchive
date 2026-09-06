/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XIRIXSA_H
#define XIRIXSA_H

#include "xarchive.h"

// SGI IRIX standalone-tools volume - the "sa" bootfile that lives in the
// root of an IRIX installation CD / miniroot and holds the per-CPU copies of
// the standalone programs (sash, fx, ide, mr).  The PROM reads the directory
// and boots one member by name.
//
// The whole container is one 512-byte block followed by the raw member images;
// nothing is compressed.
//
//   +0x00 u32be  0xACED1234  magic
//   +0x04 u32be  checksum    chosen so that the big-endian u32 sum of the
//                            complete 512-byte header block is 0
//   +0x08..0x1f  zero padding
//   +0x20        directory: exactly 20 fixed slots of 24 bytes
//
// Directory slot:
//   +0x00 char[16]  member name, NUL padded (NOT NUL terminated when it fills
//                   the field, so the full 16 bytes have to be read)
//   +0x10 u32be     first block of the member, in 512-byte units
//   +0x14 u32be     member size in bytes
//
// An all-zero slot is an unused one and is skipped; used slots are not
// required to be contiguous, so the walk covers all 20 and never stops early.
//
// Detection is the reference tool's: magic, the header block summing to zero,
// and the first slot starting at block 1.  Together those are far stronger
// than the 4-byte magic on its own and cannot fire on unrelated data.
class XIRIXSA final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        QString sFileName;
        qint64 nDataOffset;
        qint64 nSize;
        quint32 nBlock;
    };

    explicit XIRIXSA(QIODevice *pDevice = nullptr);
    ~XIRIXSA() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
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
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString rawNameToString(const char *pRawName, qint32 nIndex);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XIRIXSA_H
