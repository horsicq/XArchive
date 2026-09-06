/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XDTPACKED_H
#define XDTPACKED_H

#include "xarchive.h"

// "DT" single-file compressor used by the Delrina installers (WinFax PRO,
// Delrina Communications Suite).  Every packed file on those diskettes is one
// of these: the DOS "last extension character replaced by an underscore"
// convention is NOT used, the container simply replaces the file in place.
//
// Layout (all integers little endian), 41 bytes of header:
//
//   0x00  char     magic[2]      "DT"
//   0x02  quint16  version       always 0x0002
//   0x04  quint16  flags         always 0x0001
//   0x06  quint8   junk[23]      UNINITIALISED WRITER MEMORY - see below
//   0x1d  quint32  rawSize       length of the plaintext
//   0x21  quint8   junk[4]       more uninitialised writer memory
//   0x25  quint16  dosDate
//   0x27  quint16  dosTime
//   0x29  ...      payload       PKWARE DCL Implode stream, prelude included
//
// Bytes 0x06..0x1c and 0x21..0x24 are NOT a name and NOT a checksum: across the
// 160-file reference corpus they hold recognisable fragments of the packer's own
// address space - DOS environment strings ("PROMPT=$P$G\0TMP="), 8086 code
// ("8b d3 8b 1f 0b db 75 e6"), error message text ("File %s ") - and two files
// with completely different contents can carry byte-identical junk.  Nothing in
// them may be validated or reported.
//
// THE ORIGINAL FILE NAME IS STORED NOWHERE, so the single record is named after
// the container file itself, which is how these arrive on the install media.
//
// The payload is a plain PKWARE DCL Implode ("blast") stream whose own two-byte
// prelude is part of the payload; the whole corpus writes 00 06 (uncoded
// literals, 4K window).  rawSize is redundant with the stream's end-of-stream
// code, and that redundancy is this format's ONLY integrity check - it has no
// CRC anywhere - so isValid() trial-decodes the stream and requires the two to
// agree exactly.  The 6-byte magic on its own is far too weak a gate.
class XDTPacked final : public XArchive {
    Q_OBJECT

public:
    explicit XDTPacked(QIODevice *pDevice = nullptr);
    ~XDTPacked() override;

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
    struct MEMBER {
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };

    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        quint16 nVersion;
        QList<MEMBER> listEntries;
    };

    // bVerifyPayload = true runs the bounded trial decode described above.  It
    // is the gate every entry point that has to be sure uses; getFileParts()
    // and the unpack path additionally need it because the reported plaintext
    // length has to be one a decoder can actually reproduce.
    bool parseContext(CONTEXT *pContext, bool bVerifyPayload,
                      PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XDTPACKED_H
