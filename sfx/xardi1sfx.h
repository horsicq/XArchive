/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XARDI1SFX_H
#define XARDI1SFX_H

#include "xarchive.h"

// ARDI self-extracting DISKETTE IMAGE (Daniel F Valot; the stub's own banner
// inside the payload reads "-- ARDI   Version 4.31 --" and "Character mode
// restorable diskette image").  Built by IMG2ARDI.EXE; the carrier is a 16-bit
// MZ+NE stub bound for DOS and OS/2 1.x that, when run, writes the payload back
// onto a physical floppy, or -- with its own "/D=filename.img" switch -- into a
// file the USER names on the command line.
//
// RELATIONSHIP TO FT_ARDI2_SFX.  The corpus labels "SFX ARDI 1" and "SFX ARDI
// 2" are handler indices, not generations, and the two are NOT one format in
// two versions.  They are two concurrent products of one toolkit that share an
// author, a brand and a single version counter: the ARDI 2 carriers measured
// here identify themselves as "Ardi unpacker Version 4.22" (2002) and "Ardi
// installer Version 4.33" / "4.34" (2006, 2009), while this one is "ARDI
// Version 4.31" (2003) -- i.e. the diskette container's version sits BETWEEN
// two installer-container versions in the same numbering, so neither container
// succeeds the other.  They also travel together: every ARDI 2 EMTINST carrier
// ships IMG2ARDI.EXE, the builder for THIS format, as one of its members.
// Structurally they share nothing at all -- no tag, no constant, no field, and
// two different EOF trailers -- so each needs its own reader.
//
// The file ends with a 51-byte trailer whose last 31 bytes are the literal
//
//     "ARDI-(C)1991-" <4 digits> "-Daniel Valot" 0x00
//
// and that is the only exact, position-fixed marker the format has.  It is the
// detection gate; everything else is arithmetic checked against it.
//
// The 0x33-byte record header sits at the END of the NE segment data, 0x82
// bytes past what the reference implementation treats as the archive start.
// It is NOT at a computable offset -- the two overlay definitions in use
// disagree by exactly those 0x82 bytes -- so it is LOCATED by searching for
// the 0x00000485 format constant and then accepted only when the record it
// implies ends precisely where the trailer begins:
//
//     +0x00  quint16  bytes per sector, 512 on every known build
//     +0x02  quint8   sectors per cluster        \
//     +0x03  quint16  reserved sectors            |  a verbatim copy of the
//     +0x05  quint8   number of FATs              |  image's DOS BPB, used
//     +0x06  quint16  root directory entries      |  by the stub to format
//     +0x08  quint16  total sectors  <- the size  |  the target diskette
//     +0x0A  quint8   media descriptor            |
//     +0x0B  quint16  sectors per FAT             |
//     +0x0D  quint16  sectors per track           |
//     +0x0F  quint16  heads                       |
//     +0x11  quint32  hidden sectors              |
//     +0x15  quint32  total sectors (32-bit)     /
//     +0x19  quint8[11]  drive geometry the BPB does not carry
//     +0x24  quint16  0x55AA
//     +0x26  quint32  compressed size, authoritative
//     +0x2A  quint32  0x00000485
//     +0x2E  quint8   0
//     +0x2F  quint32  CRC-32 of the disk image
//
// A single raw DEFLATE stream (zlib windowBits -15, no zlib or gzip wrapper)
// follows at +0x33 and runs for exactly the declared compressed size.  It
// inflates to a short prologue of [quint8 tag][quint32 length][length bytes]
// records terminated by a 0xFF tag, and then to the raw diskette image,
// "total sectors" * "bytes per sector" bytes long.  The prologue is only
// discoverable by inflating, so parsing decodes a bounded prefix of the stream
// to measure it, and the image is then published as a substream window of the
// one solid block.  The header CRC-32 covers the WINDOW, so it is published as
// the member's own result CRC and XDecompress verifies it.
//
// Two prologue record kinds are observed.  Tag 0x80 is the stub's message
// table, which is program text and not a file.  Tag 0x02 is the builder's
// free-text description of this image: [quint32 count][count NUL-terminated
// strings], which the message table names "Label text", "Message to user text"
// and "Technical data text" (the /PL, /PM and /PR print switches).  The first
// string is published as the member's FPART_PROP_INFO so nothing the container
// says about the image is dropped; the records themselves are never published
// as members, because they are not files.
//
// NAMES AND PATHS.  The container stores NO name and NO destination path for
// the image, and this is a property of the format rather than an omission by
// this reader: the stub's default action writes 2880 sectors to a physical
// diskette ("Insert a diskette in drive %s and press enter"), and its only
// file-producing mode takes the file name from the USER ("/D=filename.img
// convert ARDI to image file").  The two strings the container does carry are
// both unusable as names because both collide: the tag-0x02 label text has
// only two distinct values across the five carriers measured (three share
// "DFSee - 32-bit DOS version...", two share "DFSee - DFSDOS version 8.xx..."),
// and the image's FAT12 root-directory volume label is the identical string
// "DFSEEUSBDSK" on all five (the BPB label field at boot+0x2b is the unset
// placeholder "NO NAME    " on all five as well).  The name is therefore
// derived from the carrier as <carrier base name> + ".img" -- ".img" being the
// extension the format's own CLI uses -- exactly as the reference
// implementation does, and it is reported as DERIVED in the archive info
// string rather than presented as stored.  Exactly one member per carrier, so
// no collision is possible.
class XARDI1SFX final : public XArchive {
    Q_OBJECT

public:
    explicit XARDI1SFX(QIODevice *pDevice = nullptr);
    ~XARDI1SFX() override;

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
    struct HEADER {
        qint64 nRecordOffset;      // start of the 0x33-byte record header
        quint16 nBytesPerSector;   // 512
        quint16 nTotalSectors;     // 2880 on every known build (1.44 MiB)
        quint8 nMediaDescriptor;
        quint16 nSectorsPerTrack;
        quint16 nNumberOfHeads;
        qint64 nCompressedSize;    // declared, and checked against the trailer
        quint32 nImageCRC32;       // CRC-32 of the image, zlib polynomial
        qint64 nStreamOffset;      // nRecordOffset + 0x33
        qint64 nImageSize;         // nTotalSectors * nBytesPerSector
        QString sTrailerYear;      // the four digits inside the EOF trailer
    };

    struct CONTEXT {
        qint64 nInputSize;
        HEADER header;
        qint64 nPrologueSize;   // inflated bytes before the image, -1 unknown
        qint64 nBlockSize;      // nPrologueSize + nImageSize
        qint64 nTotalSize;      // end of the EOF trailer
        QString sImageName;     // DERIVED from the carrier; see the note above
        QString sLabel;         // the container's own description of the image
    };

    bool readHeader(HEADER *pHeader, PDSTRUCT *pPdStruct);
    bool acceptRecordAt(qint64 nRecordOffset, qint64 nStreamEnd, HEADER *pHeader, PDSTRUCT *pPdStruct);
    bool measurePrologue(const HEADER &header, qint64 *pnPrologueSize, QString *psLabel, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static ARCHIVERECORD imageRecord(const CONTEXT &context);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XARDI1SFX_H
