/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XHUFF_H
#define XHUFF_H

#include "xarchive.h"

// "HUF" multi-file Huffman archive (the producing tool is not identifiable from
// the container; every known specimen holds a C-library source tree).
//
// Layout - everything is little endian:
//
//   +0x00 u16  0x01BD               magic
//   +0x02 u16  memberCount          never 0
//   +0x04 u16  symbolCount          1..256
//   +0x06 u32  directoryOffset      > 9 and inside the file
//   +0x0a      symbol table, symbolCount bytes, most frequent symbol first
//              (it may contain 0x00 - the NUL that terminates a member name is
//              an ordinary coded symbol)
//   ...        the shared Huffman tree, as a bit stream, ending exactly where
//              the directory starts
//   dirOff     memberCount records of 13 bytes:
//                  +0x00 u32  offset of this member's Huffman-coded NAME
//                  +0x04 u32  uncompressed size
//                  +0x08 u32  offset of this member's Huffman-coded DATA
//                  +0x0c u8   flag byte (0 or 1 in the reference corpus; it
//                             does not change how a member is decoded)
//
// There is ONE tree for the whole archive and it is stored in the header, so a
// member's own bytes are not a self-contained stream: XHUFF hands the tree over
// to the decompressor as FPART_PROP_COMPRESSPROPERTIES (see
// XHuffDecoder::packTree) and publishes only the coded data as the member's
// stream.  Names are coded with the same tree and are decoded during parsing.
//
// Neither the names nor the data carry an end marker - a stream simply stops
// after the expected number of symbols - so a member's stream size is not
// stored anywhere.  What is published as the compressed size is the distance to
// the next stream that starts after it, which is exact for every archive whose
// streams do not overlap and is in any case only an upper bound handed to a
// decoder that stops on its own.
class XHUFF final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nIndexOffset;       // directory record
        qint64 nNameOffset;        // coded name
        qint64 nDataOffset;        // coded data
        qint64 nStreamSize;        // bound of the coded data
        qint64 nUncompressedSize;  // record +0x04
        quint8 nFlags;             // record +0x0c
        QString sFileName;
    };

    explicit XHUFF(QIODevice *pDevice = nullptr);
    ~XHUFF() override;

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
        qint64 nDirectoryOffset;
        qint64 nDirectorySize;
        qint64 nTreeOffset;
        qint64 nTreeSize;
        qint32 nSymbolCount;
        QByteArray baProperty;  // packed symbol table + tree bits
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XHUFF_H
