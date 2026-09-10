/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XEA_H
#define XEA_H

#include "xarchive.h"

// Electronic Arts DOS distribution archive (".PEA", also shipped extensionless
// as DISK1 / DISK2 on the install floppies of Starflight 2, 688 Attack Sub,
// F-16 Combat Pilot, Cartooners, ...).
//
// There is no global header and no central directory: the file is a bare chain
// of 48-byte member headers, each immediately followed by its payload.
//
//   +0x00  u8    0x1a
//   +0x01  u16   'AE' little-endian, i.e. the ASCII pair "EA"
//   +0x03  char  name[12], NUL-padded ("main4.lbm")
// +0x0f u8 always 0 - the terminator the reference implementation forces before reading the name
//   +0x10  u32   timestamp/checksum word (NOT a DOS date: the high half is out
//                of range on most members, so it is published as an opaque
//                value and never turned into a QDateTime)
//   +0x14  u8    method: 0 = stored, 1 = 12-bit LZW
//   +0x15  i32   uncompressed size (unaligned, must be >= 0)
//   +0x19  i32   compressed size (unaligned, must be >= 0)
//   +0x1d  i32   constant 0x130 - the format's real discriminator
//   +0x21..0x2f  zero padding
//
// The 0x130 constant at +0x1d and the repeat of the 1A "EA" magic on every
// record are what make a truncated or spliced chain fail closed; the walk must
// land exactly on EOF.
class XEA final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nStamp;
        quint8 nMethod;
        QString sFileName;
    };

    explicit XEA(QIODevice *pDevice = nullptr);
    ~XEA() override;

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
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(quint8 nMethod);
    static HANDLE_METHOD methodToHandleMethod(quint8 nMethod);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XEA_H
