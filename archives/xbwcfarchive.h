/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBWCFARCHIVE_H
#define XBWCFARCHIVE_H

#include "xarchive.h"

// BWCF - the McAfee / Network Associates ".SET" distribution container
// (MCAFDOS.SET, SCN31401.SET, VNT30301.SET, Zac61001.set ...).  Despite the
// shared "BW" prefix in the corpus folder names this is NOT a relative of BWF:
// BWF has no magic, a 22-byte fixed record and a PKWARE DCL payload, while this
// one leads with a literal "BWCF" tag and carries LZHUF.  The reference tool
// catalogues them as two unrelated archive handlers, and nothing in either
// layout is shared.
//
// File header, 0x56 bytes:
//   0x00   4  "BWCF"
//   0x04   1  u8 version, 1 or 2
//   0x05  81  product description, NUL padded ("McAfee ScanPM", "ZAC Agent
//             6.1.0 Setup", ...)
//
// Records follow from 0x56.  The two versions differ only in how the name is
// carried:
//   version 1:  a fixed 0x10C-byte field holding a NUL terminated 8.3 name.
//   version 2:  "MFTS" + u8 tag 0x02, then TWO length-prefixed (u8) strings -
//               the file name FIRST and the destination directory SECOND.  The
//               directory is the one that matters: Clean.dat alone occurs in
//               four different directories of one archive, so publishing the
//               bare name would collide.  The directory already ends in a
//               backslash, and the published path is directory + name.
//
// Both versions then carry the same 17-byte descriptor:
//   +0x00  2  u16 MS-DOS time
//   +0x02  2  u16 MS-DOS date
//   +0x04  4  i32 UNCOMPRESSED size
//   +0x08  4  i32 block size - the advance from the end of this descriptor to
//             the next record
//   +0x0C  4  i32 reserved, must be zero
//   +0x10  1  u8  method: 1 = LZHUF, 3 = stored.  Every other value is left
//             unclaimed; the reference refuses them too.
//
// The block itself opens with an i32 repeat of the uncompressed size, so the
// payload proper starts at +4 and runs to the end of the block.  For a stored
// member the block size must be exactly uncompressed + 4.
//
// The compressed members are plain Yoshizaki LZHUF in precisely the shape
// XLZHUFDecoder::getOptions(1, 1, 0, false, false, false) already produces for
// SBX, ARNI and ZTC - dist variant 1, F = 0x3C, THRESHOLD = 2, no end symbol,
// MAX_FREQ 0x8000, 0x2000-byte ring prefilled with 0x20, driven purely by the
// stored output length.  No codec is added for this format; HANDLE_METHOD_BWCF_LZHUF
// exists only so the reported method names the right family, exactly as ARNI's
// arm does next to SBX's.
class XBWCFArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint8 nMethod;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };

    explicit XBWCFArchive(QIODevice *pDevice = nullptr);
    ~XBWCFArchive() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    qint32 getType() override;
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
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        quint8 nVersion;
        QString sDescription;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(quint8 nMethod);
    static HANDLE_METHOD methodToHandleMethod(quint8 nMethod);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XBWCFARCHIVE_H
