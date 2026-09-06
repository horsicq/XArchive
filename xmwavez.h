/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XMWAVEZ_H
#define XMWAVEZ_H

#include "xarchive.h"

// IBM Mwave packed file (.Z), as shipped on the Mwave DSP driver and utility
// disks for the IBM Aptiva / ThinkPad Mwave adapters (1993-1995).
//
// Layout - a fixed 23-byte header followed by a plain Unix compress LZW stream:
//
//   +0x00  2  1F 9D          the ordinary Unix-compress magic
//   +0x02  2  checksum       u16 LE over the member (algorithm not identified;
//                            it is neither CRC-16/ARC, CRC-16/CCITT nor a byte
//                            sum of either the packed or the plain member, so
//                            it is published as an opaque value and never used
//                            as an acceptance gate)
//   +0x04  2  DOS date       u16 LE, 1993-09-20 .. 1995-11-29 across the corpus
//   +0x06  2  DOS time       u16 LE, constantly 0x0020 (00:01:00) in the corpus
//   +0x08 15  original name  ASCIIZ 8.3 name in a fixed 15-byte field; the tail
//                            after the terminator holds stale bytes from a
//                            previously written name ("Q44.DSP\0DSP\0...") and
//                            must not be read
//   +0x17  1  compress flags bit 7 = block-compress, bits 0..4 = maxbits;
//                            0x8C (block mode, 12 bits) on every known sample
//   +0x18  .. LZW code stream, LSB-first, running to EOF
//
// The magic and the flags byte are therefore NOT adjacent: the three bytes
// XCompressDecoder expects at its entry point do not exist contiguously
// anywhere in the container.  Everything from +0x17 on is a byte-exact
// headerless .Z payload, so the class re-attaches 1F 9D through a thin read-only
// view (MwaveZStream in the .cpp) and hands that to the shared Unix-compress
// decoder rather than duplicating the LZW codec.
//
// One member per container; the stored name is the extracted name.
class XMwaveZ final : public XArchive {
    Q_OBJECT

public:
    explicit XMwaveZ(QIODevice *pDevice = nullptr);
    ~XMwaveZ() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    qint32 getType() override;
    QString typeIdToString(qint32 nType) override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    OSNAME getOsName() override;
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
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                       PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

    enum MWAVEZ_TYPE {
        TYPE_UNKNOWN = 0,
        TYPE_Z
    };

private:
    struct HEADER {
        quint16 nChecksum;
        quint16 nDosDate;
        quint16 nDosTime;
        quint8 nFlags;
        QString sFileName;
    };

    struct CONTEXT {
        qint64 nInputSize = 0;
        qint64 nStreamOffset = 0;    // always MWAVEZ_HEADER_SIZE
        qint64 nCompressedSize = 0;  // payload bytes the LZW decoder consumed
        qint64 nUncompressedSize = 0;
        HEADER header = {};
    };

    // Header-only recognition. Cheap, and strict enough that a plain Unix
    // compress .Z cannot be claimed: it would need a NUL-terminated valid DOS
    // 8.3 name in its LZW data at +0x08 plus a legal flags byte at +0x17 plus a
    // legal DOS date/time.
    bool readHeader(HEADER *pHeader, PDSTRUCT *pPdStruct);
    // Header parse plus a full LZW pass to learn both stream sizes.
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);

    static bool isValidDosName(const QByteArray &baName);
};

#endif  // XMWAVEZ_H
