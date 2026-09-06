/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XMAKESELF_H
#define XMAKESELF_H

#include "xarchive.h"

// Makeself shell self-extracting archive (".run", ".sh"), as produced by
// makeself.sh 1.x/2.x - VirtualBox's Guest Additions installers are the best
// known example.
//
// The file is an ordinary Bourne shell script with the payload appended
// verbatim.  Everything the extractor needs is written into the script as
// shell variables:
//
//     # This script was generated using Makeself 2.1.5
//     filesizes="7495680"                     total payload byte count
//     offset=`head -n 404 "$0" | wc -c | ...` where the payload starts
//
// so the payload begins at (file size - sum of the filesizes numbers).  Old
// releases split the payload across several numbers, hence the sum.
//
// The payload itself is one tar stream, optionally compressed.  Makeself
// supports gzip (the default), bzip2, compress and "none"; the compressor is
// not named in a machine-readable way, so it is identified by the magic at the
// payload offset - which is what U3 does too (FUN_00653690 scans the first
// 64 KiB for 1f 8b 08, "BZh1".."BZh9" or a tar header starting "./") and it is
// also the fallback here when the filesizes line is missing or does not agree
// with the magic.
//
// The archive publishes exactly one member, the tar stream, dispatched through
// codecs that already exist - HANDLE_METHOD_STORE for an uncompressed payload,
// HANDLE_METHOD_DEFLATE for gzip (the raw deflate data after the gzip header,
// with the trailer's ISIZE as the plaintext length), HANDLE_METHOD_BZIP2 for
// bzip2, HANDLE_METHOD_COMPRESS for a .Z payload and HANDLE_METHOD_XZ for xz.
// No new codec, and no HANDLE_METHOD of this family's own.  U3 behaves the same
// way for a gzip payload: it hands the file to its GZIP handler and writes out
// the tar; only for an uncompressed payload does it go on to list the tar's own
// members, which here is left to the generic chain that re-detects the .tar.
class XMakeself final : public XArchive {
    Q_OBJECT

public:
    enum PAYLOAD_KIND {
        PAYLOAD_KIND_UNKNOWN = 0,
        PAYLOAD_KIND_TAR,       // "none": the tar stream itself
        PAYLOAD_KIND_GZIP,      // 1f 8b 08
        PAYLOAD_KIND_BZIP2,     // "BZh" + '1'..'9'
        PAYLOAD_KIND_COMPRESS,  // 1f 9d
        PAYLOAD_KIND_XZ         // fd '7' 'z' 'X' 'Z' 00
    };

    explicit XMakeself(QIODevice *pDevice = nullptr);
    ~XMakeself() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    OSNAME getOsName() override;
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
        qint64 nScriptSize;      // bytes of shell script before the payload
        qint64 nPayloadOffset;   // == nScriptSize
        qint64 nPayloadSize;     // to the end of the file
        qint64 nStreamOffset;    // payload offset, past a gzip header
        qint64 nStreamSize;
        qint64 nUncompressedSize;  // -1 when the codec cannot report it
        PAYLOAD_KIND payloadKind;
        QString sVersion;
        QString sLabel;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static PAYLOAD_KIND classifyPayload(const QByteArray &baMagic);
    // Steps over the variable-length gzip header (FEXTRA / FNAME / FCOMMENT /
    // FHCRC) so the member can be dispatched as raw deflate.
    static qint64 gzipHeaderSize(const QByteArray &baPayloadHead);
    static QString extractShellValue(const QByteArray &baScript,
                                     const char *pKey);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
    HANDLE_METHOD handleMethod(const CONTEXT &context) const;
    static QString methodName(PAYLOAD_KIND payloadKind);
};

#endif  // XMAKESELF_H
