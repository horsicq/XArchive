/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XQSETUP_H
#define XQSETUP_H

#include "xarchive.h"

// Pantaray "QSetup Installation Suite" self-extractor.
//
// The carrier is a PE32 stub and the container lives in the PE overlay.  It
// opens with two length-prefixed ANSI strings:
//
//   quint32 nLength1, nLength1 bytes  -- the builder's separator list ("||")
//   quint32 nLength2, nLength2 bytes  -- the product string list; it always
//                                        opens with "|http:"
//
// nLength1 is 1 or 2, and those two strings plus the "|http:" opening are what
// the reference implementation recognises the format by.
//
// Behind them come the member records, each
//
//   quint32 nRecordSize
//   nRecordSize bytes of ONE complete zlib (RFC 1950) member
//
// and the next record starts at nRecordSize bytes past the size word.  The
// decoded stream does NOT start with the file: it starts with a NUL-terminated
// header line
//
//   |<name>|<seconds since 1980-01-01>|
//
// and the file body is everything behind that NUL.  A name may carry a
// trailing '*' (the builder's "run this after install" mark), which is not
// part of the name.  Nothing anywhere declares the decoded size, so the member
// table only exists once every record has been inflated: listing costs a full
// unpack, the same arrangement XGentee uses.
//
// Because the member's own header line sits INSIDE the compressed stream, a
// record is published as a one-member solid folder - FPART_PROP_ISSOLID with
// FPART_PROP_SUBSTREAMOFFSET at the first byte behind the NUL - which is the
// only route in XDecompress that decodes a whole stream and then emits a
// window of it.  Publishing the record as a plain HANDLE_METHOD_ZLIB member
// would prepend "|name|time|\0" to every extracted file.
//
// The chain ends on a 0x4a-byte trailer whose dword at +0x0c is 4A3B2C1D and
// whose dword at +0x46 is 0x4a.  That trailer also repeats the container
// offset (+0x04) and the member count (+0x08); both are checked against the
// walk, so a desynchronised walk is rejected instead of publishing garbage.
class XQSetup final : public XArchive {
    Q_OBJECT

public:
    explicit XQSetup(QIODevice *pDevice = nullptr);
    ~XQSetup() override;

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
    struct MEMBER {
        QString sFileName;   // the installed name: index key stripped
        QString sKeyedName;  // the container's raw "NNNNN#name" form
        qint64 nStreamOffset;       // first byte of the zlib member
        qint64 nStreamSize;         // its exact length, footer included
        qint64 nTotalUnpackedSize;  // header line + NUL + body
        qint64 nSubstreamOffset;    // first body byte inside the decoded stream
        qint64 nUncompressedSize;   // body only
        quint32 nSeconds1980;       // 0 when the record carried no usable stamp
    };

    struct CONTEXT {
        qint64 nInputSize;
        qint64 nContainerOffset;
        qint64 nRecordsOffset;
        qint64 nArchiveSize;
        QList<MEMBER> listMembers;
    };

    bool locateContainer(qint64 *pnContainerOffset, qint64 *pnRecordsOffset, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool readMember(MEMBER *pMember, qint32 nIndex, PDSTRUCT *pPdStruct);
    static void fillMemberProperties(const CONTEXT &context, qint32 nIndex, QMap<FPART_PROP, QVariant> *pMapProperties);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XQSETUP_H
