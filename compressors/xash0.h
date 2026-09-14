/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XASH0_H
#define XASH0_H

#include "xarchive.h"

// Nintendo ASH0: the single-file compressor behind the Wii System Menu's
// resources (and Animal Crossing: City Folk, My Pokemon Ranch).  One member,
// twelve bytes of header, no directory.  Format understanding derived from
// ASH0-tools by Garhoogin and NinjaCheetah (MIT) and from the ASH Extractor
// 0.1 disassembly of the Wii System Menu decoder by crediar.
//
// Layout (all integers big endian):
//
//   0x00  quint8   magic[4]    'A' 'S' 'H' '0'
//   0x04  quint32  sizeWord    bits 23..0 = plaintext length (1 .. 0xFFFFFF)
//                              bits 31..24 = top byte, see below
//   0x08  quint32  distOffset  absolute offset of the distance bit stream
//   0x0C  ...      symbol stream (Huffman tree + literal/length codes), up to
//                  distOffset
//   dist  ...      distance stream (Huffman tree + distance codes), to EOF
//
// No name, no timestamp, no checksum, no member count, no end marker.  The
// declared length is the ONLY termination and the ONLY integrity check the
// format has.
//
// The top byte of +4: the System Menu decoder masks the word with 0x00FFFFFF
// and never looks at it again; every known encoder writes 0.  It encodes
// nothing - not the tree widths, not flags - so it is accepted whatever it is,
// never validated, and only reported in FPART_PROP_INFO.
//
// Distance width: the System Menu and City Folk use 11-bit distance leaves,
// Pokemon Ranch 15-bit ones, and the file does not say which.  The decoder
// auto-detects it (wider first, accept the first "tight" decode - see
// xash0decoder.h) and the width it settles on is published as
// FPART_PROP_WINDOWSIZE (2048 / 32768) and in the reported method string
// "ASH0 9/11" or "ASH0 9/15", so the extraction path can try it first.  Should
// that property be dropped the decoder simply re-runs the same search - there
// is no substituted profile that could decode silently wrong.
//
// distOffset must lie in [0x10, size - 4]: the symbol stream needs one whole
// word after the header and the distance stream one whole word before EOF.
// 4-alignment is NOT required (ashcomp always aligns, Nintendo's own encoder
// is unknown and the System Menu never checks).
//
// The whole container is the STREAM part (offset 0, the file's length): the
// codec needs the size word at +4 and the offset at +8, the way XCompressZ
// publishes its stream from offset 0 with the magic inside.
//
// isValid() is a full trial decode with width detection.  Four magic bytes and
// a 24-bit length are far too weak a gate: a scanner over unrelated data that
// happened to start with "ASH0" would otherwise list a member and then write
// garbage at exit 0.  The trial is bounded by the format itself (16 MiB - 1 of
// output, tables of at most 512 KiB) and by an expansion-ratio guard (1032 x
// packed + slack, the two-leaf-tree maximum) checked before any allocation;
// it has no timing budget and no machine-speed dependency.  Containers over
// ASH0_MAX_PACKED_SIZE are refused before being read: a 16 MiB plaintext of
// incompressible bytes packs to about 18 MiB (nine-bit literals), so nothing
// real comes close.
//
// The original file name is not stored.  The member takes the container's name
// with one trailing ".ash" removed (case-insensitive: "foo.ash" -> "foo",
// "foo.arc.ash" -> "foo.arc"), the container's name verbatim otherwise, and
// "ash0_data" when the device has no name.  A container literally named
// ".ash" keeps that name - the member name is never empty.  Nothing is
// appended: the payload is often a U8 archive but not always, and its real
// type is detected recursively anyway.
class XASH0 final : public XArchive {
    Q_OBJECT

public:
    explicit XASH0(QIODevice *pDevice = nullptr);
    ~XASH0() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    qint32 getType() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
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
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nUncompressedSize;
        qint64 nDistOffset;
        quint8 nTopByte;
        qint32 nDistBits;  // 11 or 15 once verified, 0 after the cheap parse
        bool bTight;
        bool bAmbiguous;
        QString sFileName;
    };

    // bVerifyPayload = true runs the trial decode described above and fills
    // nDistBits.  isValid(), getFileParts() and the unpack path use it: the
    // reported plaintext length and width have to be ones the decoder can
    // actually reproduce.  getFileFormatSize() uses the cheap header parse.
    bool parseContext(CONTEXT *pContext, bool bVerifyPayload,
                      PDSTRUCT *pPdStruct);
    static QString memberName(const QString &sContainerName);
    static QString reportedMethod(qint32 nDistBits);
    static qint64 windowSize(qint32 nDistBits);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XASH0_H
