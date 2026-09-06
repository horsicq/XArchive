/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XIBMZPAK_H
#define XIBMZPAK_H

#include "xarchive.h"

// IBM "-ZPAK" distribution archive, as found on IBM AntiVirus / IBM OS/2 LAN
// product diskettes.  The packed members keep the DOS "last extension character
// replaced by an underscore" convention (AV.IN_, NAT.EX_, ...), so the file name
// carries no usable extension of its own.
//
// Layout (all integers little endian):
//
//   0x00  char     magic[5]      "-ZPAK"
//   0x05  quint8   zero          always 0x00
//   0x06  quint16  version       always 0x0001
//   0x08  ...      payload       member streams, back to back, no padding
//   ...   ENTRY    directory[N]  one 88-byte entry per member, in stream order
//   EOF-2 quint16  count         N, the number of members
//
//   ENTRY = char name[80] (NUL terminated, NUL padded to the full 80 bytes)
//         | quint32 packedSize | quint16 dosDate | quint16 dosTime
//
// The DIRECTORY IS AT THE END and the streams carry no per-member header, so the
// member offsets only come out of a running sum of the directory's packedSize
// fields.  That sum landing exactly on the first directory byte is the format's
// only self-check and this class treats a mismatch as a reject.
//
// Every member is one complete PKWARE DCL Implode ("blast") stream, prelude
// bytes included - packedSize counts those two bytes.  In the 1774-file
// reference corpus the prelude is always 00 06 (binary literals, 4K window).
//
// The PLAINTEXT LENGTH IS STORED NOWHERE.  It only falls out of the DCL
// end-of-stream code, so parseContext() takes a bScanSizes flag and recovers it
// with XDclDecoder::scan() exclusively on the paths that need it; a member whose
// size could not be measured is reported as HANDLE_METHOD_UNKNOWN, because
// decPkwareDcl() takes the output length as an input and a zero there would
// silently write an empty file instead of failing.
class XIBMZPak final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDirectoryOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        // False until XDclDecoder::scan() has recovered the plaintext length.
        bool bUncompressedSizeKnown;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };

    explicit XIBMZPak(QIODevice *pDevice = nullptr);
    ~XIBMZPak() override;

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
        qint64 nDirectoryOffset;
        qint64 nFirstMemberOffset;
        quint16 nVersion;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bScanSizes, PDSTRUCT *pPdStruct);
    bool scanMemberSize(MEMBER *pMember, PDSTRUCT *pPdStruct);
    static QString methodToString(const MEMBER &member);
    static HANDLE_METHOD methodToHandleMethod(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XIBMZPAK_H
