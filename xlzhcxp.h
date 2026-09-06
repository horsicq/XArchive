/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XLZHCXP_H
#define XLZHCXP_H

#include "xarchive.h"

// Single-file "LZ" container (U3 calls the family LZHCXP).
//
//   +0x00 u16  'LZ'
//   +0x02      the payload, as a chain of blocks: [u8 length][length bytes],
//              ended by a zero-length block
//
// That is the whole header.  There is no member name, no uncompressed size, no
// checksum and no timestamp - the file is its own single member, and the only
// way to learn the decompressed size is to decompress.
//
// Because there is nothing to validate structurally, detection follows the
// original's rules and then TRIAL DECODES:
//
//   * u16 at +0 is 'LZ'
//   * the first block length is not 0 and the first payload byte is 0
//   * the first 10-bit code is exactly 0x200 (CLEAR) and the second code is a
//     literal - this is what the (u16 at +4) & 0xC03 == 2 test in the original
//     amounts to once the bit packing is written out
//   * a first block length other than 0xFF means the payload is a single block,
//     so the file must be exactly length + 4 bytes and the byte after the block
//     must be the zero-length terminator
//   * the LZW stream then has to decode without hitting an invalid code
//
// The codec is XLzhcxpDecoder (LZW with 0x200 = CLEAR, 0x201 = END, dictionary
// from 0x202, 10..12 bit codes); see that header for the details.
class XLZHCXP final : public XArchive {
    Q_OBJECT

public:
    explicit XLZHCXP(QIODevice *pDevice = nullptr);
    ~XLZHCXP() override;

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
        qint64 nStreamOffset;
        qint64 nStreamSize;
        qint64 nUncompressedSize;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XLZHCXP_H
