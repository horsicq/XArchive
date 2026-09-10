/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSWAG_H
#define XSWAG_H

#include "xarchive.h"

// SWAG (SourceWare Archive Group) Pascal snippet collection, ".SWG", as written
// by the GDSOFT "SWAG ARCHIVE PROJECT" packer (1994).  This is the compressed
// flavour of the collection; the uncompressed SWAGOLX packets are a completely
// different container handled by XSwagPacket.
//
// The file is an LHA level-0 multi-member chain whose method tag is "-sw1-"
// instead of one of the "-lhN-" ids, and whose extension area is a fixed
// 0xA7-byte block of SWAG reader metadata.  Per member:
//
//     +0x00 u8    header size, always nameLength + 0xBB
//     +0x01 u8    header checksum: low byte of the sum of the headerSize
//                 bytes starting at +0x02
//     +0x02 5     method id, always "-sw1-"
//     +0x07 i32   compressed size
//     +0x0B i32   uncompressed size
//     +0x0F u16   MS-DOS time
//     +0x11 u16   MS-DOS date
//     +0x13 u16   attribute word (0 in every sample)
//     +0x15 u32   ones-complement of the CRC-32 of the plaintext, i.e. the raw
//                 CRC register before the final inversion
//     +0x19 ..    SWAG reader metadata: length-prefixed source-archive name,
//                 category, author and keyword strings.  The framing of the
//                 individual strings drifts between packer builds (later ones
//                 leave uninitialised bytes between the fields), so only the
//                 source-archive name at +0x19 is published.
//     +0xBA u8    file name length, 1..12
//     +0xBB ..    file name (no path, plain 8.3 ASCII)
//     +hs+0 u16   CRC-16 of the plaintext (LHA/ARC reflected 0xA001, init 0)
//     +hs+2 ..    compressed data, `compressed size` bytes
//
// The next member starts right after the data, and the chain length comes from
// a 129-byte footer occupying the last bytes of the file:
//
//     +0x00  ShortString[60]  "SWAG ARCHIVE PROJECT (c) 1994 GDSOFT ..."
//     +0x3D  ShortString[65]  collection title, e.g. "GRAPHICS ROUTINES"
//     +0x7F  u16              member count
//
// Two of the 22 reference archives declare one member more than the data area
// actually holds (COMM.SWG, truncated in transit); the reference implementation reports those as errors
// yet still extracts every intact member, so the walk here stops cleanly at the
// first member that does not fit instead of rejecting the whole archive.
//
// The "-sw1-" codec is not a new one: it is plain LZHUF, the Okumura adaptive
// Huffman + 4 KiB LZSS scheme that LHA ships as "-lh1-" (N = 4096, F = 60,
// THRESHOLD = 2, N_CHAR = 314, ring pre-filled with spaces).  Verified byte
// exact on all 1213 members of the 22-file corpus against the reference implementation, so members are
// published as the existing HANDLE_METHOD_LZH1 and no codec is added.
//
// Note that the reference implementation additionally trims trailing NUL/TAB/LF/CR/space bytes and up to
// two 0x1A end-of-file markers off each decoded member before writing it.  That
// step is lossy and is deliberately NOT reproduced here: this class emits the
// full uncompressed size the header declares, of which the reference implementation's output is a strict
// prefix (3 to 5 bytes shorter on text members).
class XSWAG final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;       // start of the level-0 header
        qint64 nDataOffset;         // first byte of the compressed stream
        qint64 nCompressedSize;     // header +0x07
        qint64 nUncompressedSize;   // header +0x0B
        quint16 nDosTime;           // header +0x0F
        quint16 nDosDate;           // header +0x11
        quint16 nAttributes;        // header +0x13
        quint32 nCrc32;             // header +0x15, already un-inverted
        quint16 nCrc16;             // stored right in front of the data
        QString sFileName;
        QString sSourceArchive;     // metadata block +0x19
    };

    explicit XSWAG(QIODevice *pDevice = nullptr);
    ~XSWAG() override;

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
        qint64 nFooterOffset;
        qint32 nDeclaredCount;
        QString sCopyright;
        QString sTitle;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool parseHeader(const QByteArray &baHeader, MEMBER *pMember);
    static QString readShortString(const QByteArray &baBlock, qint32 nOffset,
                                   qint32 nCapacity);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSWAG_H
