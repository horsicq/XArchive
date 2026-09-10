/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSQ_H
#define XSQ_H

#include "xarchive.h"

// "SQ" squeezed single-file container (53 51 AC AE), as shipped for the .SQ_
// modules of the IBM LANAID / LAN utility distribution.
//
//   0x00  53 51 AC AE            magic
//   0x04  char szName[]          the original file name, ASCIIZ
//   ....  quint8 nMonth          1..12
//         quint8 nDay            1..31
//         quint8 nYear           years since 1980
//         quint8 nHour           0..23
//         quint8 nMinute         0..59
//         quint8 nSecond2        0..29, seconds / 2
//   ....  the packed stream, running to the end of the file
//
// Despite the name this is NOT the classic CP/M-DOS Squeeze format (0xFF76):
// no Huffman node table is stored anywhere.  The payload is an LZ77 over a
// 32 KiB window driven by an adaptive Huffman model; see XSQDecoder for the
// full description.  There is exactly one member and its uncompressed length
// is not recorded, so the length is discovered by decoding once.
class XSQ final : public XArchive {
    Q_OBJECT

public:
    explicit XSQ(QIODevice *pDevice = nullptr);
    ~XSQ() override;

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

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nStreamOffset;
        qint64 nStreamSize;
        qint64 nUncompressedSize;  // -1 until the stream has been measured
        QString sFileName;
        QDateTime dtModified;
    };

    // Header only: magic, the ASCIIZ name and the six date bytes.  Cheap enough
    // to run on every candidate during type detection.
    bool parseHeader(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    // parseHeader plus one full decode to learn the uncompressed length, which
    // the container does not store.
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
    static QString sanitizeName(const QByteArray &baRawName);
};

#endif  // XSQ_H
