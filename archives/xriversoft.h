/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XRIVERSOFT_H
#define XRIVERSOFT_H

#include "xarchive.h"

// RiverSoft Data Library (".RDL"), the resource container of RiverSoft's
// mid-1990s DOS/Windows games (MISC.RDL, SOUND.RDL, IMAGE.RDL).
//
// Fixed 32-byte header, then the member data, then the directory at the very
// end of the file:
//
//     +0x00 23   "RiverSoft Data Library" 0x1A
//     +0x17 u16  1
//     +0x19 u8   1
//     +0x1A u16  number of directory entries
//     +0x1C u32  sum of every member's uncompressed size
//     +0x20 ..   reserved, zero (the first member starts at 0x40 in all known
//                archives, but nothing in the format demands it)
//
// The directory occupies the last count * 0x15 bytes, one 21-byte record per
// member and nothing else - there is no terminator and no count repeated at
// the end, which is why the entry size has to be exact:
//
//     +0x00 13   Turbo Pascal ShortString[12] file name, length 1..12
//     +0x0D u32  file offset of the member's compressed data
//     +0x11 u16  compressed size
//     +0x13 u16  uncompressed size
//
// Both sizes are 16 bit, so no member can exceed 64 KiB - which is exactly the
// chunk size the reference implementation's decompressor works in.
//
// Every member is one PKWARE Data Compression Library "implode" stream (the
// blast/explode format: literal flag byte, dictionary-size byte 4..6, then the
// three fixed Huffman tables with the 519 end code).  That codec is already in
// the library, so members are published as the existing
// HANDLE_METHOD_PKWARE_DCL_IMPLODE and nothing new is added.
//
// Verified against the reference implementation on the whole three-file corpus: 1069 of 1069 members
// byte identical, same names, same set.
class XRiverSoft final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDirOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        QString sFileName;
    };

    explicit XRiverSoft(QIODevice *pDevice = nullptr);
    ~XRiverSoft() override;

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
        qint64 nDirectoryOffset;
        qint64 nDirectorySize;
        qint32 nEntryCount;
        quint32 nTotalUncompressedSize;  // header +0x1C
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static void fillMemberProperties(const MEMBER &member,
                                     QMap<FPART_PROP, QVariant> *pmap);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XRIVERSOFT_H
