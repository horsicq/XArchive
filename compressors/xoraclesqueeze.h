/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XORACLESQUEEZE_H
#define XORACLESQUEEZE_H

#include "xarchive.h"

// Oracle/R:BASE "squeezed" single-file container (76 FF).
//
//   0x00  quint16 nMagic             0xFF76, stored 76 FF
//   0x02  quint32 nUncompressedSize  the decoded length, in bytes
//   0x06  quint16 nChecksum          sum of the decoded bytes, truncated to 16 bit
//   0x08  char szName[]              the original file name, ASCIIZ
//   ....  quint16 nNumberOfNodes     1..256 decoding-tree nodes
//   ....  qint16 nChild[2] * nodes   the decoding tree
//   ....  the bit stream, running to the end of the file
//
// This is the classic CP/M-DOS Squeeze payload - a static Huffman tree over 257
// values followed by the 0x90 run-length stage, i.e. exactly SEA ARC method 4 -
// but the header is NOT the classic one: Oracle inserted a 32-bit decoded length
// between the magic and the checksum, so everything from the name onwards sits
// four bytes further into the file.  Feeding such a file to a classic Squeeze
// reader makes it read an empty name at 0x04 and start decoding four bytes early,
// which is why they used to surface as a zero-length member.
//
// The classic and the Oracle layout are told apart the only way they can be: a
// file that already parses as a classic Squeeze header is left to the classic
// reader.  See XOracleSqueeze::looksLikeClassicSqueeze.
class XOracleSqueeze final : public XArchive {
    Q_OBJECT

public:
    explicit XOracleSqueeze(QIODevice *pDevice = nullptr);
    ~XOracleSqueeze() override;

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
        qint64 nStreamOffset;  // the node count, i.e. where the ARC method 4 decoder starts
        qint64 nStreamSize;
        qint64 nUncompressedSize;
        quint16 nChecksum;
        QString sFileName;
    };

    // The whole header, including a structural check of the decoding tree.  The
    // tree is at most 1 KiB, so this is still cheap enough for type detection and
    // never needs a decode pass.
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    // ASCIIZ name at nOffset; returns the offset just past its terminator.
    static qint64 scanName(const QByteArray &baProbe, qint64 nOffset, QString *pName);
    // Node count plus tree at nOffset; returns the offset of the bit stream.
    static qint64 scanTree(const QByteArray &baProbe, qint64 nOffset, qint64 nInputSize);
    // True when the file is a plain CP/M Squeeze, whose name starts at 0x04.
    static bool looksLikeClassicSqueeze(const QByteArray &baProbe, qint64 nInputSize);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
    static QString sanitizeName(const QByteArray &baRawName);
};

#endif  // XORACLESQUEEZE_H
