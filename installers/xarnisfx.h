/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XARNISFX_H
#define XARNISFX_H

#include "xarchive.h"

// ARNI self-extracting installer - the container the mIRC setup stub carries.
//
// The carrier is a 32-bit Windows executable and the container does NOT live in
// the overlay: it is the contents of one RCDATA resource (named "MIRCALL" in
// two of the five reference carriers and numbered 4 in the other three), so the
// PE overlay offset - the first thing every other SFX reader here tries - is
// the END of the file and points at nothing.  That is why this reader locates
// the chain by its own tag instead.
//
// The container is a flat chain of records with no archive header, no member
// count, no directory and NO NAMES:
//
//   0x00  quint32 "ARNI"          the record tag, 41 52 4E 49
//   0x04  qint32  nDecodedSize    decoded length, 0 < n < 0x1000000
//   0x08  packed bytes            LZHUF, self-terminating at nDecodedSize
//
// and the chain closes on a ten byte end record:
//
//   "ARNI" "ARNI" 0D 0A          i.e. 41 52 4E 49 41 52 4E 49 0D 0A
//
// A RECORD DOES NOT STORE ITS PACKED LENGTH.  The decoder stops when it has
// produced nDecodedSize bytes, and the next record's tag begins at the very
// next byte, so a member's packed extent is the distance to the next record
// header.  Measured on the five reference carriers, running the decoder over
// each member consumed EXACTLY that many input bytes for all 54 of them - the
// encoder leaves no slack - so the scan and a full decode agree.  Scanning is
// what this reader does, because it makes listing cost one pass over the
// carrier instead of a full decompression.
//
// The four-byte tag is short enough to appear by chance, so a candidate is only
// a header when the dword behind it is a plausible decoded size (the reference
// implementation's own bound, 0 < n < 0x1000000) or the second "ARNI" of the
// end record; the reference additionally refuses a tag followed by "NG",
// because "ARNING" is the tail of the English word WARNING, and that rule is
// kept here.  A walk is only accepted when it lands on the end record, which is
// what makes a stray tag inside the stub harmless.
//
// CODEC.  Every member is a plain Yoshizaki LZHUF stream - LZSS over an
// adaptive Huffman tree - with no framing and no method field: dist variant 1,
// F = 0x3c, THRESHOLD = 2, no end symbol (the stored decoded size is the only
// stop condition), MAX_FREQ 0x8000, and a 0x2000-byte ring prefilled with 0x20.
// That is exactly XLZHUFDecoder::getOptions(1, 1, 0, false, false, false), the
// same parameter set HANDLE_METHOD_ZTC and HANDLE_METHOD_SBX_LZHUF already use,
// so HANDLE_METHOD_ARNI_LZHUF needs a dispatch and not a new decoder.
//
// MEMBER NAMES.  The container stores none - the reference implementation
// publishes "File_0.bin", "File_1.bin", ... - but the carrier does: the stub
// writes its files with one `sprintf("%s\\%s", destination, name)` per record
// from a string pool of bare file names that sits in its data segment, in the
// SAME ORDER as the records.  This reader recovers that pool and publishes the
// real installed names when, and only when, the carrier bytes ahead of the
// container contain EXACTLY ONE maximal run of plain file-name strings whose
// length equals the member count and whose entries are all distinct; anything
// else falls back to the reference's own "File_<n>.bin".  On the five reference
// carriers the rule fires every time and the pairing is corroborated by the
// decoded content: .exe members start "MZ", .hlp members start 3F 5F 03 00, and
// .txt/.ini members are text - 54 of 54.  The carrier that ships no mlink32.exe
// has ten names and ten records and everything still lines up, which is what
// makes the pairing positional rather than a coincidence of one build.
//
// NOT CLAIMED.  A bare chain at file offset 0, with no executable in front of
// it, is refused: the carrier must start "MZ".  No such file exists in either
// reference corpus and accepting one would be untested code.
class XArniSFX final : public XArchive {
    Q_OBJECT

public:
    explicit XArniSFX(QIODevice *pDevice = nullptr);
    ~XArniSFX() override;

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

    QList<FPART_PROP> getAvailableFPARTProperties() override;
    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nPackedSize;
        qint64 nUncompressedSize;
        QString sFileName;
    };

    struct CONTEXT {
        qint64 nInputSize;
        qint64 nContainerOffset;
        qint64 nArchiveSize;
        QList<MEMBER> listMembers;
    };

    static bool classifyHeader(const QByteArray &baHeader, bool *pbTerminator, qint64 *pnUncompressedSize);
    static bool isPlainFileName(const QByteArray &baToken);
    static QList<QString> collectNameTable(const QByteArray &baStub, qint32 nCount);

    qint64 findHeader(qint64 nFrom, qint64 nInputSize, PDSTRUCT *pPdStruct);
    bool walkChain(qint64 nStart, qint64 nInputSize, QList<MEMBER> *pListMembers, qint64 *pnChainEnd, PDSTRUCT *pPdStruct);
    bool locateContainer(qint64 *pnContainerOffset, PDSTRUCT *pPdStruct);
    void applyNames(qint64 nContainerOffset, QList<MEMBER> *pListMembers, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static void fillRecordProperties(const MEMBER &member, QMap<FPART_PROP, QVariant> *pMapProperties);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XARNISFX_H
