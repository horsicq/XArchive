/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSOFTPAQ2_H
#define XSOFTPAQ2_H

#include "xarchive.h"

// Compaq (later HP) SoftPaq self-extracting distribution EXE (sp####.exe).
//
// The container is a DOS extractor stub - itself PKLITE compressed, so the file
// opens with the stock "MZ" header followed by the "PKLITE Copr. 1990-9x PKWARE
// Inc." banner at +0x1e - with the payload and a "[FIT]" directory appended
// behind it.
//
// The directory is anchored by a 37-byte locator record that can sit anywhere in
// the file (it is emitted right behind the stub, around +0x8ce0 in the reference
// corpus) and identifies itself by repeating its own file offset:
//
//   +0x00 char   szTag[8]    "[FIT]" 00 01 00
//   +0x10 qint32 nSelfOffset must equal the record's own offset
//   +0x14 qint32 nDirOffset  first stored-entry record
//   +0x18 qint32 nSplit      first compressed-entry record
//
// [nDirOffset, nSplit) is a table of 23-byte STORED entries and
// [nSplit, fileSize) a table of 38-byte COMPRESSED entries; both tile their
// range exactly.
//
// STORED entry (23 bytes) - the stub's own resources:
//   +0x00 char   szName[8]   space padded
//   +0x08 quint8 always 0
//   +0x09 char   szExt[4]    carries its own leading '.', space padded
//   +0x0d quint8 always 0
//   +0x0e qint32 nSize
//   +0x12 qint32 nOffset
//   +0x16 quint8 nPublished  0 = the extractor stub itself, not a member
//
// COMPRESSED entry (38 bytes) - the payload proper:
//   +0x00 char   szName[8]
//   +0x08 quint8 always 0
//   +0x09 char   szExt[4]
//   +0x0d quint8 always 0
//   +0x0e quint16 nMethod    0 = stored, 6 = PKWARE DCL implode
//   +0x10 quint16 nDosTime   +0x12 quint16 nDosDate
//   +0x14 quint32 nCRC32     CRC-32 of the PACKED stream, not of the member
//   +0x18 qint32  nUncompressedSize
//   +0x1c qint32  nCompressedSize
//   +0x20 quint16 nAttributes
//   +0x22 qint32  nOffset
//
// Method 6 is a plain PKWARE Data Compression Library implode stream (literal
// mode + dictionary-bits prelude included), so no new codec is introduced:
// HANDLE_METHOD_PKWARE_DCL_IMPLODE and HANDLE_METHOD_STORE cover the format.
class XSoftPaq2 final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nCRC32;
        bool bHasCRC;
        quint16 nMethod;
        quint16 nAttributes;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };

    explicit XSoftPaq2(QIODevice *pDevice = nullptr);
    ~XSoftPaq2() override;

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
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nLocatorOffset;
        qint64 nDirectoryOffset;
        qint64 nSplitOffset;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString rawNameToString(const char *pName, const char *pExt, qint32 nIndex);
    static HANDLE_METHOD methodToHandleMethod(quint16 nMethod);
    static QString methodToString(quint16 nMethod);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSOFTPAQ2_H
