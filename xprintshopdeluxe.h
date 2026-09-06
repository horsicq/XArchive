/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XPRINTSHOPDELUXE_H
#define XPRINTSHOPDELUXE_H

#include "xarchive.h"

// Broderbund "The Print Shop Deluxe" (Windows/DOS) install file.
//
// Every file on the distribution media is a SINGLE compressed member wrapped in
// a fixed 80-byte prologue.  The media keeps the DOS "last extension character
// replaced" convention (TRIBUNE.TT_, PSDWIN.HL$, PSD.PB$, ...), so the container
// name is useless and the real name only lives inside the prologue.
//
// Layout (little endian):
//
//   0x00  char    name[76]   original 8.3 name, NUL terminated, NUL padded
//   0x4C  quint32 totalSize  size of THIS file, prologue included
//   0x50  ...     stream     one complete PKWARE DCL Implode ("blast") stream
//
// Notes that matter for the parser:
//
//  * totalSize is the CONTAINER size, not the plaintext size.  It equals
//    QIODevice::size() for every one of the 88 reference files, which is what
//    makes it a usable integrity gate rather than a size hint.
//  * The PLAINTEXT LENGTH IS STORED NOWHERE.  It only falls out of the DCL
//    end-of-stream code, so parseContext() takes a bScanSize flag and recovers
//    it with XDclDecoder::scan() on the paths that need it.  decPkwareDcl()
//    takes the output length as an INPUT, so a member whose length could not be
//    measured is reported as HANDLE_METHOD_UNKNOWN - a zero there would silently
//    write an empty file at exit 0 instead of failing.
//  * There is no magic number of any kind.  isValid() therefore leans on the
//    shape of the whole prologue: a non-empty printable 8.3 name, a strictly
//    NUL-filled remainder of the 76-byte field, totalSize == file size, and a
//    legal DCL prelude at 0x50.  Every reference file writes prelude 00 06
//    (binary literals, 4K window); the gate accepts the full legal DCL range
//    because the stream format does.
class XPrintShopDeluxe final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        // False until XDclDecoder::scan() has recovered the plaintext length.
        bool bUncompressedSizeKnown;
        QString sFileName;
    };

    explicit XPrintShopDeluxe(QIODevice *pDevice = nullptr);
    ~XPrintShopDeluxe() override;

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
        qint64 nFirstMemberOffset;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bScanSize, PDSTRUCT *pPdStruct);
    bool scanMemberSize(MEMBER *pMember, PDSTRUCT *pPdStruct);
    static QString methodToString(const MEMBER &member);
    static HANDLE_METHOD methodToHandleMethod(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XPRINTSHOPDELUXE_H
