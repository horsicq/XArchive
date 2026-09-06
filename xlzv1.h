/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XLZV1_H
#define XLZV1_H

#include "xarchive.h"

// "LZV1" single-file compressed container (.LZV).
//
// One payload, no member name, no stored unpacked size and no CRC: a 12-byte
// header followed by one LZW code stream that runs to EOF.
//
//   0x00 char   szMagic[4]  "LZV1"
//   0x04 quint8 fixed[6]    5D 19 01 AD 00 00 - constant in every known file
//   0x0A quint16 nMaxCodes  BIG endian dictionary ceiling, 0x100 < n < 0x4000
//   0x0C code stream
//
// The ten fixed bytes are the whole detector; nMaxCodes is a decoder parameter,
// which is why the entire container - header included - is handed to
// XLZV1Decoder as the stream.
//
// Because nothing in the container records the unpacked size, this class does
// NOT publish FPART_PROP_UNCOMPRESSEDSIZE: the size is only knowable by running
// the decoder, and listing an archive must not silently decompress it.
class XLZV1 final : public XArchive {
    Q_OBJECT

public:
    explicit XLZV1(QIODevice *pDevice = nullptr);
    ~XLZV1() override;

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
        qint64 nStreamSize;  // the whole file: the decoder needs the header too
        qint32 nMaxCodes;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XLZV1_H
