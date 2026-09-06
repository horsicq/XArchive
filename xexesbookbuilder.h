/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XEXESBOOKBUILDER_H
#define XEXESBOOKBUILDER_H

#include "xarchive.h"

// SbookBuilder 2 (Jan Verhoeven, jansfreeware.com) "self-running Sbook": a
// Delphi MZP stub with the whole web-site tree appended behind the PE image.
//
// The container is the PE overlay.  Its last four bytes are the overlay's own
// file offset, which is how the stub finds itself and how this class finds the
// container without having to trust the section table.
//
// Overlay header (13 bytes):
//
//   +0x00  u32 5                     length prefix of the tag below
//   +0x04  "Sbook"                   ("Ebook" is also accepted by the original)
//   +0x09  u32                       total size of all members once inflated
//
// then one record per member:
//
//   +0x00  u32   member count on the FIRST record, 0 on every later one
//   +0x04  u32   length of the source path (1..0x400)
//   +0x08  char[length]  the path the author built the book from, e.g.
//                        "d:\!!praia\!upload\ebooks\myws\sbook\tile.gif"
//   ...    u32   timestamp-shaped word, not validated by the original
//   ...    "EC2\0"      per-member codec tag
//   ...    the chunk chain
//
// and the list ends on a record whose count and path length are both 0; the
// four bytes after that terminator are the overlay pointer.
//
// A chunk chain is a run of  [u32 compressedSize][compressedSize bytes]  where
// each chunk is its OWN complete zlib stream (the stub links "inflate 1.0.4
// Copyright 1995-1996 Mark Adler").  Every chunk inflates to exactly 16384
// bytes except the last one of a member, which is what ends the chain - there
// is no member length field anywhere, so the member boundaries only exist once
// the chunks have been inflated.  A member that is an exact multiple of 16384
// bytes long therefore ends with a chunk that inflates to 0 bytes.
//
// Reference: U3.unp.exe handler S144 ("EXE SBookBuilder", class xvb, VMT slots
// 0x007793a0 / 0x007793c0); detector 0x00778e70, worker 0x00778ec0, its
// length-prefixed string reader 0x0041d500 and its zlib entry point 0x0042f260.
class XEXESBookBuilder final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;      // start of the member's record
        qint64 nDataOffset;        // first chunk of the chain
        qint64 nCompressedSize;    // bytes the chain occupies
        qint64 nUncompressedSize;  // bytes the chain inflates to
        quint32 nStamp;            // the word in front of the "EC2" tag
        QString sFileName;
    };

    explicit XEXESBookBuilder(QIODevice *pDevice = nullptr);
    ~XEXESBookBuilder() override;

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
        qint64 nOverlayOffset;
        qint64 nArchiveSize;      // end of the overlay pointer at EOF
        qint64 nTotalSize;        // overlay header +0x09
        qint32 nDeclaredCount;    // first record's count word
        char cTagFirst;           // 'S' or 'E'
        QList<MEMBER> listMembers;
    };

    // Locates the appended container: the trailing self-pointer first, the
    // section table second.  Returns -1 when neither candidate carries the tag.
    qint64 findOverlay(qint64 nInputSize, PDSTRUCT *pPdStruct);
    bool checkOverlayTag(qint64 nOffset, qint64 nInputSize, char *pcTagFirst, PDSTRUCT *pPdStruct);
    qint64 sectionTableOverlay(qint64 nInputSize, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    // Walks one chunk chain, inflating as it goes: chunk boundaries are the
    // only thing that says where a member ends.
    bool measureChain(qint64 nOffset, qint64 nInputSize, qint64 *pnCompressedSize, qint64 *pnUncompressedSize, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XEXESBOOKBUILDER_H
