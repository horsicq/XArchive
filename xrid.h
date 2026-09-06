/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XRID_H
#define XRID_H

#include "xarchive.h"

// "RID" multi-member installer archive (OS/2, 1998-99).  Carries the OS2YOU /
// PM2YOU remote-console family and its LANTERM / TERM2 / DRIVERS packages; the
// extension is always .RID and the installer reads the file front to back.
//
// The container is HEADERLESS: there is no magic, no central directory and no
// count.  The file is a bare chain of members, each one a 43-byte header
// followed by a chain of framed payload blocks that ends in a 0xff terminator,
// and the last member's terminator must land exactly on EOF.
//
//   +0x00  u16   writer tag - varies per member, only ever non-zero
//   +0x02  19    zero bytes  (the reserved area; a single stray byte is a
//                             mis-parse and is rejected)
//   +0x15  u8    DOS attributes, 0x00 or 0x20 (archive bit) only
//   +0x16  u16   DOS time
//   +0x18  u16   DOS date, never zero
//   +0x1a  u32   uncompressed size, <= 0x00ffffff
//   +0x1e  13    8.3 name, NUL terminated INSIDE the field.  The bytes behind
//                the terminator are stale writer-buffer content ("ZIP.DLL\0EXE"
//                is a real field), so they must never be read or trimmed.
//   +0x2b        block chain (see XRidDecoder)
//
// Detection therefore cannot rest on a signature: it walks the whole chain,
// requires every header to pass all of the above and the walk to land exactly
// on EOF, and additionally trial-decodes the first member's payload.
class XRID final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint16 nTag;
        quint16 nDosDate;
        quint16 nDosTime;
        quint8 nAttributes;
        qint32 nNumberOfBlocks;
        QString sFileName;
    };

    explicit XRID(QIODevice *pDevice = nullptr);
    ~XRID() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

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

    bool parseContext(CONTEXT *pContext, bool bDeepCheck, PDSTRUCT *pPdStruct);
    static bool isValidNameField(const QByteArray &baField, QString *pName);
    static QString methodToString(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XRID_H
