/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XFIZ_H
#define XFIZ_H

#include "xarchive.h"

// FIZ - Maximus BBS (Lanius Corp) distribution/installer archive, DOS, early
// 1990s.  Extension .FIZ; the shipped set is CTL/HLP/LANG/MISC/MEX/RIP/SYS*.
//
// There is no central directory and no global header at all: the file is a
// bare chain of members, each introduced by a fixed 20-byte header followed
// immediately by the (not NUL-terminated) name and then the payload.  Every
// header byte is accounted for:
//
//   +0x00  "FIZ" 0x1a     magic, repeated on every member
//   +0x04  u8   method    0 = stored, 1 = LHA -lh5-
//   +0x05  u8   nameLen   1..12 (8.3 name, no path component ever appears)
//   +0x06  u16  crc16     CRC-16/ARC of the UNPACKED member
//   +0x08  u32  origSize
//   +0x0c  u32  compSize  payload bytes that follow the name
//   +0x10  u16  dosTime
//   +0x12  u16  dosDate
//   +0x14  name[nameLen], then compSize payload bytes
//
// The walk is self-validating and must land exactly on EOF, which is what
// makes the two-byte method/nameLen gate safe to rely on.
class XFiz final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint16 nCRC16;
        quint8 nMethod;
        QString sFileName;
        QDateTime dtModified;
    };

    explicit XFiz(QIODevice *pDevice = nullptr);
    ~XFiz() override;

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
        qint64 nFirstMemberOffset;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(quint8 nMethod);
    static HANDLE_METHOD methodToHandleMethod(quint8 nMethod);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XFIZ_H
