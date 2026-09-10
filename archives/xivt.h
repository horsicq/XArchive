/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XIVT_H
#define XIVT_H

#include "xarchive.h"

// Microsoft Multimedia Viewer / MediaView title (".M20", also ".MVB"), the
// WinHelp-derived container.  The file magic is 3F 5F 04 01 - the same family
// as WinHelp's 3F 5F 03 00, one version on.
//
// File header (only two fields are used; the reference tool reads 40 bytes and
// ignores the rest):
//
//     +0x00 u32 magic 0x01045F3F
//     +0x04 i32 offset of the directory B-tree
//
// The directory is a B-tree whose header is 48 bytes - MediaView widened
// WinHelp's 38-byte BTREEHEADER by making MustBeZero/PageSplits/RootPage/
// MustBeNegOne/TotalPages 32-bit:
//
//     +0x00 u16  magic 0x293B      +0x02 u16 flags
//     +0x04 u16  page size (0x2000 in every known title)
//     +0x06 char structure[16]
//     +0x16 u32  must be zero      +0x1a u32 page splits
//     +0x1e u32  root page         +0x22 u32 must be -1
//     +0x26 u32  total pages       +0x2a u16 levels
//     +0x2c u32  total entries
//
// Pages follow immediately, each exactly one page size.  A leaf page opens with
//
//     +0x00 u16 unused bytes at the end of the page
//     +0x02 u16 entry count
//     +0x04 i32 previous page      +0x08 i32 next page
//
// and then that many entries of
//
//     u8 nameLength, name[nameLength], LEB128 fileOffset, LEB128 fileSize, u8 0
//
// After the entries the page is skipped by its "unused" count, which has to
// land exactly on the next page boundary - together with the entry total that
// is a complete structural check of the directory.
//
// An entry names an internal file at an absolute offset.  If it starts with
// "mszp" or "nszp" it is MSZIP: a 12-byte header (magic, u32 uncompressed size,
// u32 reserved) followed by blocks of
//
//     u16 uncompressedBlockSize, u16 compressedBlockSize, "CK", raw DEFLATE
//
// where compressedBlockSize counts the "CK" and each block inherits the
// previous 32 KiB as its dictionary - the identical scheme CAB uses, so this
// reuses the existing MSZIP block inflater and adds only the framing walk
// (HANDLE_METHOD_IVT).  Anything else is stored verbatim.
//
// Sixteen internal files hold the title's own indexes rather than content and
// are not published: AlinkInfo, AlinkList, AlinkLookup, GRPINF, KeywordInfo,
// KeywordList, KeywordLookup, STRINGS, TOCIDX, TOPICS, TitleInformation,
// URLTREE, charmap, ftindex, source.toc and stoplist.  Zero-length entries are
// skipped as well.  Internal names frequently start with '>' (topic streams are
// named ">00000001" and so on), so names are mapped through the same
// invalid-character substitution the reference tool applies.
class XIVT final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDataOffset;
        qint64 nStreamSize;        // bytes the directory declares
        qint64 nUncompressedSize;  // MSZIP header's size, or nStreamSize
        bool bCompressed;
        QString sInternalName;
        QString sFileName;  // sInternalName with invalid characters replaced
    };

    explicit XIVT(QIODevice *pDevice = nullptr);
    ~XIVT() override;

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
        qint64 nDirectoryOffset;
        qint64 nDirectorySize;
        qint32 nTotalEntries;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XIVT_H
