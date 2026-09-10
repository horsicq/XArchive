/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSETTLERSFT_H
#define XSETTLERSFT_H

#include "xarchive.h"

// Data archive of Blue Byte's "The Settlers" / "Serf City: Life is Feudal"
// (SPAE.PA, SPAD.PA, SPAU.PA, SPADD.PA - one file per language).
//
// There is no magic and no names.  The whole container is
//
//     +0x00 u32   total size of the file, which is the anchor that makes a
//                 headerless format identifiable at all
//     +0x04 u32   number of table entries
//     +0x08 ..    that many 8-byte entries, [u32 size][u32 offset]
//
// and everything after the table is raw resource data.  Entries with offset 0
// are unused slots and produce nothing; the remaining ones are numbered by
// their table position, 1-based, which is the only name a member ever gets.
//
// The members are not interchangeable blobs.  One of them is the archive's
// 256-colour palette (the first entry whose size is exactly 768) and two of
// the five member kinds are images that only mean something together with it:
//
//     KIND_MASK    row-RLE sprite      -> "Masks/NNNN.bmp"     (32bpp BMP)
//     KIND_BITMAP  flat 8bpp image     -> "Bitmaps/NNNN.bmp"   (paletted BMP)
//     KIND_XMI     IFF "FORM" chunk    -> "XMIDI/NNNN.xmi"     (stored)
//     KIND_PALETTE the 768-byte table  -> "Palettes/NNNN.pal"  (stored)
//     KIND_BIN     everything else     -> "NNNN.bin"           (stored)
//
// The kind is decided by a bounded trial decode of the row-RLE exactly as the reference implementation
// does it (see XSettlersFTDecoder::classify), so the naming and the conversion
// can never disagree.  The three stored kinds carry HANDLE_METHOD_STORE; only
// the two image kinds carry HANDLE_METHOD_SETTLERS_FT, with the palette handed
// to the decoder as FPART_PROP_COMPRESSPROPERTIES.
//
// Verified against the reference implementation on the whole five-file corpus: 12412 of 12412 members
// byte identical, with the same member set and the same names.
class XSettlersFT final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDataOffset;
        qint64 nDataSize;
        qint64 nUncompressedSize;  // == nDataSize for the stored kinds
        qint32 nIndex;             // 1-based table position, the member's name
        qint32 nKind;              // XSettlersFTDecoder::KIND
        QString sFileName;
    };

    explicit XSettlersFT(QIODevice *pDevice = nullptr);
    ~XSettlersFT() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

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
        qint64 nTableSize;
        qint32 nEntryCount;
        QByteArray baPalette;
        QList<MEMBER> listMembers;
    };

    // bClassify reads each member and runs the trial decode so the kinds, the
    // names and the produced sizes are known.  isValid() passes false: the
    // table arithmetic plus the presence of a palette entry already identifies
    // the container, and a probe must not read every resource in the file.
    bool parseContext(CONTEXT *pContext, bool bClassify, PDSTRUCT *pPdStruct);
    void fillMemberProperties(const CONTEXT &context, const MEMBER &member,
                              QMap<FPART_PROP, QVariant> *pmapProperties);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSETTLERSFT_H
