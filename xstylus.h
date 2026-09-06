/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSTYLUS_H
#define XSTYLUS_H

#include "xarchive.h"

// Compressed dictionary container of the Stylus / PROMT family of machine
// translation products (the ".#SD" files that sit next to the engine and hold
// one specialist English<->Russian dictionary each).  The bytes identify the
// container, not the product: nothing inside names Stylus, so the association
// comes from U3's own handler table.
//
// A single stream behind a 16-byte header, no member list and no names:
//
//     +0x00 4   "DP" 1A 07          container magic
//     +0x04 u16 1                   container version
//     +0x06 u8  3                   stream kind
//     +0x07 3   "SDC"               payload tag (matched case-insensitively)
//     +0x0A u16 0
//     +0x0C u32 CRC-32 of the decompressed dictionary
//     +0x10 ..  compressed stream to the end of the file
//
// The stream is LZSS over a 4 KiB window with every input byte XORed by 0xB5;
// see XStylusDecoder for why the existing SZDD LZSS cannot be reused (zero
// filled ring, +18 position bias, and the XOR).  The container stores no
// uncompressed size, so this class measures the stream once when it has to
// publish one rather than reporting an unknown length.
//
// The decompressed member has no stored name either.  U3 names it after the
// archive with the extension replaced by ".sdc", and this class does the same
// so extraction lands on the same file name.
class XStylus final : public XArchive {
    Q_OBJECT

public:
    explicit XStylus(QIODevice *pDevice = nullptr);
    ~XStylus() override;

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
        qint64 nCompressedSize;
        // -1 when the stream was not measured (probe path) or is too large to
        // measure; the decoder does not need it, it only makes the listing
        // report a real size.
        qint64 nUncompressedSize;
        quint32 nCrc32;
        QString sFileName;
    };

    // bMeasure runs one full decode to learn the plaintext length.  isValid()
    // and getFileFormatSize() deliberately pass false: the 10-byte magic is
    // enough to decide, and a probe must not decompress megabytes.
    bool parseContext(CONTEXT *pContext, bool bMeasure, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSTYLUS_H
