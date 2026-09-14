/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XESPARCHIVE_H
#define XESPARCHIVE_H

#include "xarchive.h"

// ESP - "Extension Sort Packer" (GyikSoft & MikroLab, 1996-97), a DOS archiver
// whose containers ship either bare as ".esp" or behind the 16-bit DOS
// self-extracting stub that its ESP2EXE.COM builds.  ONE reader answers for both
// because the container is byte-identical in the two shapes and the stub length
// is never assumed - three of the five reference carriers carry WWPACK's "WWP "
// tag at MZ+0x1c where the other two carry ESP's own "ESP!" and "ESP ", so the
// stub is not even constant for one builder.
//
// UNRELATED to FT_EPSF_SFX (Eschalon Setup, whose tag is the same four letters
// transposed) and to FT_AIN, whose codec this shares but whose container it does
// not.
//
//   ARCHIVE HEADER, 10 bytes at the container offset.  The first six bytes are
//   documented by the archiver's own TECHNOTE.TXT, which ships inside the
//   reference carriers; the directory offset behind them is not.
//     +0  char[4]  "ESP>"
//     +4  u8       format version: 0x15 ESP 1.5, 0x16 ESP 1.6+, 0x17 ESP 1.9+
//                  written with the newer encryption (/ME2), 0x19 ESP 1.9+
//                  written with the 24-bit multimedia model (/MM2)
//     +5  u8       method and password flags: bits 0..2 the method (0..4, 4
//                  meaning stored), bit 3 multimedia-compressed (the filter
//                  below), bit 4 unused and MUST be clear, bit 5 last fragment,
//                  bit 6 password used - listed but refused here, bit 7 not the
//                  first fragment
//     +6  i32      offset of the DIRECTORY, relative to the container, > 9
//
// Everything after the header is ONE SOLID stream that runs to the directory
// offset, and the directory itself is a SECOND stream that runs to the end of
// the file.  Both are the LZH of Algos/xaindecoder.h - the same blocked
// 0x110/0xfe Huffman pair over a 0x8000 window that AIN uses, bit for bit - so
// no codec is introduced here.
//
// SCRAMBLED STREAMS.  From format version 0x16 up, every byte a stream reads is
// XORed with a four-byte key that rotates right by eight after each byte.  The
// key is 0x4B697947, so the keystream is the repeating text "GyiK" - the
// archiver is GyikSoft's.  The two streams each start the key from scratch, and
// a STORED archive is not scrambled at all: the mask lives in the codec's byte
// fill, not in the file, and stored members never reach it.
//
// DIRECTORY RECORD, 28 bytes decoded from that second stream
//     +0  i32      SHARED PREFIX - see below.  0 <= this <= the size at +24
//     +4  u16      parent directory, 0xffff for the root; otherwise a BYTE
//                  OFFSET into the path buffer described below, not an index
//     +6  char[13] NUL-padded 8.3 name
//     +0x13 u8     DOS attributes; 0x10 marks a directory record
//     +0x14 u16    DOS time
//     +0x16 u16    DOS date
//     +0x18 i32    original size
//
// THE PATH BUFFER.  A directory record's full path is appended, NUL included,
// to a 0x4000-byte buffer, and the offset at which it landed is what later
// records name in their parent field.  The buffer therefore has to be rebuilt
// byte for byte while walking, or every nested name comes out wrong - the
// offsets are not recomputable from the record order alone.
//
// THE SHARED PREFIX is what makes this reader materialize.  A member's content
// is its predecessor's first N bytes followed by size-N bytes taken from the
// solid stream, where N is the record's +0 field.  The predecessor's first N
// bytes are themselves partly inherited, so a member is NOT a contiguous run of
// the decoded stream and cannot be published as a device extent with a skip
// count.  The reference implementation keeps exactly one rolling buffer: after
// emitting a member it leaves that buffer holding the member's first N' bytes,
// where N' is the NEXT record's prefix field, refilling it from the stream as
// it goes.  This reader replays that rolling buffer instead of guessing, which
// is why the fill amount is measured against the buffer's previous length and
// not against the current record's prefix.
//
// THE FILTER (flags bit 3) is the archiver's /MM "multimedia" model for 8-bit
// sound and 256-colour images: a cumulative byte delta over the stream, with one
// accumulator for the whole archive and only the freshly decoded bytes passing
// through it - inherited prefix bytes are already filtered.  Applying it once
// over the whole decoded stream is therefore equivalent, and that is what is
// done here.  From format version 0x19 the same bit means /MM2 instead, the
// 24-bit model, which splits the member into three channels; no reference
// carrier uses it and it is NOT guessed at - such an archive is listed and its
// members are refused rather than published wrong.
class XESPArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nSharedPrefix;
        qint64 nSize;
        qint64 nContentOffset;  // into CONTEXT::baContent, -1 when not materialized
        quint16 nDosDate;
        quint16 nDosTime;
        quint8 nAttributes;
        bool bIsFolder;
        QString sFileName;
    };

    explicit XESPArchive(QIODevice *pDevice = nullptr);
    ~XESPArchive() override;

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

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nContainerOffset;
        qint64 nArchiveSize;
        qint64 nDirectoryOffset;  // absolute
        quint8 nVersion;
        quint8 nFlags;
        quint8 nMethod;
        quint8 nFilter;
        bool bEncrypted;
        bool bScrambled;
        bool bMaterialized;
        QList<MEMBER> listMembers;
        QByteArray baContent;
    };

    struct UNPACK_CONTEXT {
        CONTEXT context;
    };

    bool isDosCarrier(PDSTRUCT *pPdStruct);
    bool locateContainer(qint64 *pnContainerOffset, PDSTRUCT *pPdStruct);
    bool readHeader(qint64 nContainerOffset, CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool readDirectory(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool materialize(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, bool bMaterialize, PDSTRUCT *pPdStruct);

    static void descramble(QByteArray *pbaData);
    static void applyDeltaFilter(QByteArray *pbaData);
    static QString publishedName(const QByteArray &baRawPath);
    static QString methodToString(quint8 nMethod);
};

#endif  // XESPARCHIVE_H
