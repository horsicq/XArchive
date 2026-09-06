/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XLSZ_H
#define XLSZ_H

#include "xarchive.h"

// Delrina WinFax PRO / WinFax Lite ".LSZ" install-library archive.
//
// Layout (all integers little endian, no alignment padding anywhere):
//
//   0x00  quint32  magic     0xFFFFF037   (bytes 37 F0 FF FF)
//   0x04  quint16  version   0x0300       (bytes 00 03)
//   0x06  RECORD   records[] one 51-byte record + its payload, back to back,
//                            repeated until end of file.
//
//   RECORD (51 bytes):
//     +0x00  quint32  tag         0xFFFF037F or 0x00000000
//     +0x04  char     name[13]    NUL terminated, NUL padded (8.3, max 12)
//     +0x11  quint16  reserved    always 0
//     +0x13  qint32   uncompressedSize
//     +0x17  qint32   compressedSize   (bytes of payload that follow)
//     +0x1b  quint32  checksum    0 for most members, non-zero for a few
//     +0x1f  quint16  dosTime
//     +0x21  quint16  dosDate
//     +0x23  quint16  attributes  0x0000 or 0x0020 (DOS archive bit)
//     +0x25  quint16  method      1 = stored, 2 = PKWARE DCL Implode, 6 = empty
//     +0x27  quint8   reserved[12] always 0
//
// The payload of the record starts at recordOffset + 51 and is exactly
// compressedSize bytes long; the next record starts right after it.  There is
// NO directory, NO member count and NO terminator record: the chain ending
// exactly on end-of-file is the format's only integrity check, and this class
// treats any slack as a reject.
//
// METHODS.  Method 2 is one complete PKWARE DCL Implode ("blast") stream, its
// two prelude bytes included in compressedSize - so it is handed to the
// already-present HANDLE_METHOD_PKWARE_DCL_IMPLODE unchanged, no new codec.
// Method 1 is stored (compressedSize == uncompressedSize).  Method 6 is a
// GENUINELY EMPTY member: both sizes are 0 and no payload follows.  Eleven of
// the 48 reference archives are directory-only listings built entirely out of
// method 6 records, so a correct reader emits zero-byte files for them; that is
// the format, not a truncation.  Method 6 is reported as HANDLE_METHOD_STORE
// over a zero-length stream, which is what the generic chain needs to create
// the empty file.
//
// Member names are not unique: 32_yubawjmdpqxkvzvd_USTBUS.LSZ carries the same
// name twice, so callers must not assume the listing is a set.
class XLSZ final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nChecksum;
        quint16 nDosDate;
        quint16 nDosTime;
        quint16 nAttributes;
        quint16 nMethod;
        QString sFileName;
    };

    explicit XLSZ(QIODevice *pDevice = nullptr);
    ~XLSZ() override;

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
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool parseRecord(const QByteArray &baRecord, MEMBER *pMember);
    static QString methodToString(const MEMBER &member);
    static HANDLE_METHOD methodToHandleMethod(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XLSZ_H
