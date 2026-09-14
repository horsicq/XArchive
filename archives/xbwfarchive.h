/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBWFARCHIVE_H
#define XBWFARCHIVE_H

#include "xarchive.h"

// BWF - the Beame & Whiteside BW-Connect distribution file (BWNDISK.BWF,
// BWWTCPD.BWF, BWV220A.BWF ...), a flat chain of members with NO magic and no
// central directory.  Each record is exactly 22 bytes followed by its payload:
//
//   +0x00   1  u8  record tag, always 0x01
//   +0x01  13  name; byte +0x0D is NOT part of the name - the reference reader
//              overwrites it with NUL before reading the field as a C string,
//              so the name is at most the 12 bytes at +0x01..+0x0C.  That byte
//              really does carry stale data (values 0x1B, 0x20, 0x33, 0x42,
//              0x46, 0x57, 0x79, 0x8B, 0x8F, 0x90, 0x95 all occur across the
//              146-file corpus, 350 records of 1288), but so do the bytes
//              BEFORE it: 496 of 1288 records carry a nonzero byte after the
//              terminating NUL inside +0x01..+0x0C alone (e.g. 49 42 4D 2E 53
//              54 55 00 8F FF 33 ED = "IBM.STU" + junk).  What keeps the junk
//              out of the name is therefore the NUL scan, NOT the field width:
//              every 12-character name in the corpus has +0x0D == 0, so a
//              NUL-terminated read of 13 bytes would yield the same 1288 names.
//              The 12-byte cap is kept only because it is what the reference
//              reader enforces.
//   +0x0E   4  u32 MS-DOS packed date/time, (date << 16) | time
//   +0x12   4  i32 PACKED size of the payload that follows
//   +0x16   n  payload, n = the packed size
//
// The next record starts at +0x16 + n and the chain lands exactly on EOF; there
// is no terminator, so that exact tiling is most of what makes a magic-less
// format safe to claim.
//
// The payload is a plain PKWARE Data Compression Library "implode" stream, the
// codec HANDLE_METHOD_PKWARE_DCL_IMPLODE already carries - no new decoder is
// needed and none is added here.  What the container does NOT store is the
// plaintext length; exactly like Ascend backup volumes it only falls out of the
// DCL end-of-stream code, so parseContext() takes a bScanSizes flag and the
// lengths are recovered with XDclDecoder::scan().  VERIFIED over all 1288
// members of the 146-file corpus: the declared packed size is precisely the
// bitstream boundary the decoder stops at, so a mismatch means the record chain
// and the payload disagree and the member must not be extracted.
class XBWFArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        // False until XDclDecoder::scan() has recovered the plaintext length.
        // A member whose length is unknown must report HANDLE_METHOD_UNKNOWN:
        // decPkwareDcl() takes the output length as an INPUT, so handing it a
        // zero would silently write a truncated file instead of failing.
        bool bUncompressedSizeKnown;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };

    explicit XBWFArchive(QIODevice *pDevice = nullptr);
    ~XBWFArchive() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

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
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bScanSizes, PDSTRUCT *pPdStruct);
    bool scanMemberSize(MEMBER *pMember, PDSTRUCT *pPdStruct);
    static QString methodToString(const MEMBER &member);
    static HANDLE_METHOD methodToHandleMethod(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XBWFARCHIVE_H
