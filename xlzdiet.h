/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XLZDIET_H
#define XLZDIET_H

#include "xarchive.h"

// "lZdIeT" chunked LZ container (seen only as .PAD data files).
//
// It is a SINGLE-FILE container, not a multi-member archive: one payload, whose
// name the container does not store, split into up to 250 independently coded
// chunks so a reader can seek into the middle of the plaintext.
//
// Header (0x5FC bytes, little endian):
//   char   szMagic[6]        "lZdIeT"
//   qint32 nUncompressedSize at 0x06, always > 0
//   ...                      (unused bytes, zero in every sample)
//   at 0x20: { qint32 nChunkOffset; quint16 nChunkSize; } x 250
//            terminated by nChunkOffset == -1; the table exactly fills the
//            header (0x20 + 250 * 6 == 0x5FC)
//
// The first chunk therefore always starts at 0x5FC, which is the check the
// reference extractor makes, and nChunkSize counts a 6-byte per-chunk preamble
// that sits in front of the code stream.
//
// Every chunk is a self-contained LZW stream (LSB-first, clear 0x100, end
// 0x101, 9 bits growing to 10); see XLZDIETDecoder for the codec.  The whole
// container is handed to that decoder as one buffer, because the chunk table -
// not the member - is what describes the payload.
class XLZDIET final : public XArchive {
    Q_OBJECT

public:
    explicit XLZDIET(QIODevice *pDevice = nullptr);
    ~XLZDIET() override;

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
    QList<FPART_PROP> getAvailableFPARTProperties() override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;       // header + every chunk
        qint64 nUncompressedSize;  // from the header
        qint32 nNumberOfChunks;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XLZDIET_H
