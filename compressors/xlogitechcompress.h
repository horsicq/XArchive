/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XLOGITECHCOMPRESS_H
#define XLOGITECHCOMPRESS_H

#include "xarchive.h"

// Logitech Compress: the single-file compressor Logitech shipped its MouseWare
// and Logitech Scanner install media with, decompressed on the target by
// LGEXPAND.  Every packed file on those diskettes is one of these, and the DOS
// "last character of the extension replaced by a tilde" convention IS used:
// LMBCROSS.CU~ is the packed form of LMBCROSS.CUR, MOUSE.EX~ of MOUSE.EXE.
//
// Layout (all integers little endian), 8 bytes of header:
//
//   0x00  quint8   magic[2]      DA FA
//   0x02  quint8   missingChar   the character the '~' replaced
//   0x03  quint8   unknown       NOT a checksum - see below
//   0x04  quint32  rawSize       length of the plaintext
//   0x08  ...      payload       PKWARE DCL Implode stream, prelude included
//
// Byte 0x03 is left unreported.  Over the 251-file reference corpus it takes
// five values (0x09 x89, 0x27 x75, 0x28 x32, 0x29 x54, 0xff x1); 0x27/0x28/0x29
// each belong to exactly one of the three products in the corpus while 0x09
// spans all three, so it is neither a per-file attribute nor a version.  It was
// tested against the 8-bit sum and XOR of the plaintext and of the packed
// stream: 2/251, 1/251, 1/251 and 1/251 respectively, i.e. chance.  Nothing in
// it may be validated or reported.
//
// THE ORIGINAL FILE NAME IS NOT STORED - only its last character is.  The
// single record is therefore named after the container with the trailing '~'
// replaced by byte 0x02 VERBATIM, no case folding: that is exactly what the
// reference tool publishes, and it reproduces its name on all 251 files.  A
// container whose name does not end in '~', or whose missing character is not a
// legal file-name character, keeps the container's own name unchanged.
//
// The payload is a plain PKWARE DCL Implode ("blast") stream whose own two-byte
// prelude is part of the payload; the whole corpus writes 00 06 (uncoded
// literals, 4K window).  rawSize is redundant with the stream's end-of-stream
// code, and that redundancy is this format's ONLY integrity check - it has no
// CRC anywhere - so isValid() trial-decodes the stream and requires the two to
// agree exactly.  The 2-byte magic on its own is far too weak a gate.
class XLogitechCompress final : public XArchive {
    Q_OBJECT

public:
    explicit XLogitechCompress(QIODevice *pDevice = nullptr);
    ~XLogitechCompress() override;

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
        QString sFileName;
    };

    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        QList<MEMBER> listEntries;
    };

    // bVerifyPayload = true runs the bounded trial decode described above.  It
    // is the gate every entry point that has to be sure uses; getFileParts()
    // and the unpack path additionally need it because the reported plaintext
    // length has to be one a decoder can actually reproduce.
    bool parseContext(CONTEXT *pContext, bool bVerifyPayload,
                      PDSTRUCT *pPdStruct);
    static QString restoreFileName(const QString &sContainerName,
                                   quint8 nMissingChar);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XLOGITECHCOMPRESS_H
