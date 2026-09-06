/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XMVA_H
#define XMVA_H

#include "xarchive.h"

// "mflh"/"mfen" multi-volume installer archive (.MVA is the first volume,
// .MVB the continuation ones).  The vendor is unidentified; the samples in the
// reference corpus ship Microsoft Multimedia Viewer titles and printer driver
// kits from 1997-1998.
//
// Container header (8 bytes, first volume only):
//
//     +0x00 char[4] "mflh"
//     +0x04 u32     1
//
// A continuation volume starts with the same "mflh" tag but the u32 at +4 is
// the tail of the previous volume's compressed stream instead of 1, and no
// member header follows - which is exactly the check that keeps .MVB files
// from being mis-detected (U3 rejects them the same way).
//
// Every member is one 346-byte header immediately followed by its data:
//
//     +0x000 char[4] "mfen"
//     +0x004 u16     1        (record version)
//     +0x006 u16     0x15a    (header size; the reader requires this value)
//     +0x008 u32     Unix mtime
//     +0x00c u32     second Unix timestamp (creation)
//     +0x010 char[260] full source path of the file, NUL padded
//     +0x114 i32     uncompressed size
//     +0x118 i32     compressed size
//     +0x11c u32     checksum word (algorithm unidentified - see below)
//     +0x120 u32     second checksum word
//     +0x124 u32     DOS/Win32 file attributes
//     ...            remaining bytes are zero in every known archive
//
// Members follow one another back to back: the next header sits at
// headerOffset + 0x15a + compressedSize.  There is no central directory and no
// end-of-archive marker; the chain simply runs to EOF.
//
// Compression is raw zlib (the payload starts 78 9C).  When the compressed
// size equals the uncompressed size the member is stored verbatim - U3 misses
// this case entirely: it always drives its inflate loop and therefore writes a
// zero-byte file for every stored member (15 of the 68 members in the
// reference corpus, including a 2.7 MB executable).  This class emits
// HANDLE_METHOD_STORE for those and HANDLE_METHOD_ZLIB otherwise, so no new
// codec is introduced.
//
// The two u32 words at +0x11c and +0x120 are checksums of some kind but match
// neither CRC-32 nor Adler-32 of the plaintext, so they are carried as
// informational values only and never used to gate extraction.
class XMVA final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nModificationTime;
        quint32 nCreationTime;
        quint32 nChecksum1;
        quint32 nChecksum2;
        quint32 nAttributes;
        bool bStored;
        QString sFileName;  // base name, as U3 reports it
        QString sFullPath;  // the stored 260-byte path field
    };

    explicit XMVA(QIODevice *pDevice = nullptr);
    ~XMVA() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

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
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XMVA_H
