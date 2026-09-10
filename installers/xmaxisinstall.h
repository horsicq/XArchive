/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XMAXISINSTALL_H
#define XMAXISINSTALL_H

#include "xarchive.h"

// Maxis DOS install-disk archive (INSTALL.MXS and the extension-less "._"
// data files shipped with SimCity 2000, SimFarm, SimEarth, SimHealth and
// Unnatural Selection, 1993-94).
//
// There is no global header, no magic and no central directory: the file is a
// bare chain of members, each of which is
//
//     +0x00  u32  record size    (everything after this 17-byte header)
//     +0x04  char name[13]       NUL-terminated DOS 8.3 name; the bytes after
//                                the terminator are uninitialised writer stack
//                                junk and must never be validated
//     +0x11  u32  original size   <- also the LZHUF stream's own size prefix
//     +0x15  ...  LZHUF bitstream
//
// so a member occupies 17 + recordSize bytes and the walk has to land exactly
// on EOF.  That walk plus a bounded trial decode is the whole isValid: nothing
// else in the file identifies the format.
//
// The payload is stock Yoshizaki/Okumura LZHUF - LZSS with a 4 KiB ring
// pre-filled with spaces, F=60, THRESHOLD=2, 314 adaptive-Huffman symbols and
// the canonical 64-entry position table - i.e. byte-for-byte what LHA calls
// -lh1-, which is why this class reuses HANDLE_METHOD_LZH1 instead of
// spending a new one.  The stream begins at +0x15, AFTER the u32 that LZHUF's
// encoder writes in front of the coded bits; that u32 is the member's
// uncompressed size and is published as such.
class XMaxisInstall final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        QString sFileName;
    };

    explicit XMaxisInstall(QIODevice *pDevice = nullptr);
    ~XMaxisInstall() override;

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
        qint64 nFirstMemberOffset;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool probeMember(const MEMBER &member, PDSTRUCT *pPdStruct);
    static HANDLE_METHOD memberHandleMethod(const MEMBER &member);
    static QString memberMethodString(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XMAXISINSTALL_H
