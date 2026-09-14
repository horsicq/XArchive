/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XARDI2SFX_H
#define XARDI2SFX_H

#include "xarchive.h"

// ARDI self-extracting INSTALLER (Daniel F Valot; the LX stub identifies itself
// as "ARDI (C)Daniel F Valot 1991-<year>" and "Ardi unpacker Version 4.22" or
// "Ardi installer Version 4.33" / "4.34").  The carrier is a 32-bit OS/2 LX
// executable and the container is a chain of length-suffixed blocks running
// from the end of the LX image to 50 bytes before end of file.
//
// RELATIONSHIP TO FT_ARDI1_SFX.  The corpus labels "SFX ARDI 1" and "SFX ARDI
// 2" are handler indices, not generations.  The two are one author's toolkit
// and share a single version counter -- this container's versions 4.22 (2002),
// 4.33 (2006) and 4.34 (2009) bracket the diskette container's 4.31 (2003), so
// neither succeeds the other -- and they travel together: the EMTINST carriers
// of THIS format ship IMG2ARDI.EXE, the builder for that one, as a member.
// Structurally they share nothing: no tag, no constant, no field, and two
// different EOF trailers.  Each needs its own reader.
//
// The last 50 bytes are a fixed trailer:
//
//     +0x00  char[25]  "Copyright Daniel F Valot "
//     +0x19  quint32   build check value, not a member CRC
//     +0x1D  quint16   builder counter; NOT the block count -- measured
//                      against every known carrier and it does not match
//     +0x1F  char[19]  "TSHTSH - 1991-" <4 digits> " "
//
// That literal pair is NECESSARY but NOT SUFFICIENT, and the difference is not
// academic: the same 50 bytes are the author's copyright watermark on his
// ordinary products too.  Eleven of the ninety-five members inside the
// carriers measured here (MAHJONPM.EXE, SAMEPM.EXE, SHISENPM.EXE, EMT4PM.EXE,
// EMT4OS2.EXE, EMT4WARP.EXE, IMG2ARDI.EXE) carry the exact trailer and no
// chain at all.  Detection therefore requires the trailer AND a complete
// backward walk to the sentinel, which is what isValid() does.
//
// The chain is walked BACKWARDS exactly as the reference implementation does:
// the quint32 four bytes in front of the trailer is the last block's total
// length, a block is
//
//     [quint32 tag][payload][quint32 total length]
//
// and stepping back by the length lands on the tag, whose predecessor's length
// word sits four bytes earlier.  The walk ends when the length slot holds
// 0x98765432, the sentinel that marks the start of the chain.
//
// Tags: 0x12345677 opens a member's header block and 0x12345678 its data
// block; they always occur in that order and the pair is adjacent once the
// non-member blocks are dropped.  0x11221122 is the installer's DESTINATION
// DIRECTORY, read below.  0x97979797 (product title), 0x12121212 (installation
// blurb), 0x13131313 (licence text) and 0x98989898 (four opaque bytes) are
// skipped, and any other tag ends the walk rather than being guessed at.
//
// DESTINATION PATH.  Block 0x11221122 holds one NUL-terminated path -- the
// directory the installer offers as the default install target.  It is present
// exactly once in every carrier measured and its value varies per product
// ("\OS2\APPS\MAHJONGG", "\OS2\APPS\SAME", "\OS2\APPS\SHISENPM",
// "\OS2\APPS\EMTTOOLS" twice).  The reference implementation walks past it;
// this reader does not.  It is published as the archive-level information
// string and as FPART_PROP_PREFIX on every record, and it is deliberately NOT
// prefixed onto member names, for three reasons: it is an ARCHIVE-level value
// rather than a per-member one, it is an absolute OS/2 path, and the stub
// treats it as a default the user overrides at run time ("Enter or select the
// path to install to:").  Every member is written flat into whichever single
// directory is chosen, so the extracted layout stays flat and matches what the
// stub produces.  This is the same handling, and the same property, that
// XQuarterdeckQP uses for its install-destination record.
//
// A header block is [quint32 tag][quint32 time_t][name '!' kind description
// 0x00] and must be between 9 and 0x1008 bytes long.  The stored name is the
// text before the first '!'; it is a bare 8.3 name with no path component of
// its own.  The names are published exactly as stored -- nothing is stripped,
// no suffix is invented, and they are unique inside every carrier measured.
//
// A data block is [quint32 tag] followed by a raw DEFLATE stream (zlib
// windowBits -15) for the rest of the block.  The inflated length is recorded
// NOWHERE, so no uncompressed size is published: the deflate stream is
// self-terminating and a declared size here would be this reader's guess.
class XARDI2SFX final : public XArchive {
    Q_OBJECT

public:
    explicit XARDI2SFX(QIODevice *pDevice = nullptr);
    ~XARDI2SFX() override;

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
    struct BLOCK {
        qint64 nOffset;   // the tag word
        qint64 nSize;     // tag plus payload, WITHOUT the trailing length word
        quint32 nTag;
    };

    struct MEMBER {
        QString sName;
        QString sDescription;
        qint64 nStreamOffset;
        qint64 nStreamSize;
        quint32 nUnixTime;
        bool bHasTime;
    };

    struct CONTEXT {
        qint64 nInputSize;
        qint64 nSentinelOffset;  // the 0x98765432 word that opens the chain
        qint64 nTotalSize;
        QString sTrailerYear;
        QString sInstallPath;    // block 0x11221122, the stored destination
        QList<MEMBER> listMembers;
    };

    bool readTrailer(QString *psYear, PDSTRUCT *pPdStruct);
    bool walkChain(QList<BLOCK> *pListBlocks, BLOCK *pPathBlock, qint64 *pnSentinelOffset, PDSTRUCT *pPdStruct);
    bool readMemberHeader(const BLOCK &block, MEMBER *pMember, PDSTRUCT *pPdStruct);
    bool readInstallPath(const BLOCK &block, QString *psPath, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static ARCHIVERECORD recordAt(const CONTEXT &context, qint32 nIndex);
    static qint64 recordOffset(const CONTEXT &context, qint32 nIndex);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XARDI2SFX_H
