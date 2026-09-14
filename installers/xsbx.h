/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSBX_H
#define XSBX_H

#include "xarchive.h"

// SBX - "SpinnerBaker eXtractor" - self-extracting archive / SBSETUP installer.
//
// The carrier is an ordinary Windows executable (PE32 in eleven of the fifteen
// reach-set samples, NE in the other four) and the container is its overlay:
// a bare chain of member records with NO archive header, NO central directory
// and NO count.  The chain simply runs to the last byte of the file, which is
// the only thing that closes it - a record whose arithmetic walks past EOF, or
// stops short of it, is not an SBX chain.
//
// One record is a 14-byte fixed header, the name, the decoded size, and the
// packed bytes:
//
//   0x00  quint32 0x00314253      "SB1\0" - the container tag AND its version
//   0x04  qint32  nRecordSize     WHOLE record: 0x12 + nNameLength + packed
//   0x08  quint16 nDosDate        DOS date  (year - 1980 << 9 | month << 5 | day)
//   0x0a  quint16 nDosTime        DOS time  (hour << 11 | minute << 5 | sec/2)
//   0x0c  quint8  nAttributes     DOS attribute byte (0x20 / 0x22 in the corpus)
//   0x0d  quint8  nNameLength     1..255, never 0
//   0x0e  char    szName[nNameLength]      NOT NUL terminated
//   +     qint32  nUncompressedSize        must be >= 0
//   +4    packed bytes, nRecordSize - nNameLength - 0x12 of them
//
// THE TWO TIME WORDS ARE DATE-THEN-TIME, the opposite of the (time, date)
// order MS-DOS uses in its own directory entries and that ZIP/ARJ/LHA inherit.
// Read in the familiar order every member of the reach set lands between 2012
// and 2074; read date first they land between 1997 and 2000, which is when
// these installers were built.  Nothing in the record flags the order, so a
// swapped reading produces plausible dates rather than an error.
//
// CODEC.  Every member, including a two-byte one, is a plain Yoshizaki LZHUF
// stream - LZSS over an adaptive Huffman tree - with no framing, no block
// structure and no method field: dist_variant 1, F = 0x3c, THRESHOLD = 2, no
// end symbol (the stored decoded size is the only stop condition), MAX_FREQ
// 0x8000, and a 0x2000-byte ring prefilled with 0x20.  That is exactly
// XLZHUFDecoder::getOptions(1, 1, 0, false, false, false), i.e. the same
// parameter set XZTCDecoder uses, so HANDLE_METHOD_SBX_LZHUF needs no new
// codec - only a dispatch.  The neighbouring parameter sets are NOT harmless
// guesses: F = 0x3d with THRESHOLD = 3 gives the same 314-symbol alphabet and
// decodes the first hundred bytes of a text member perfectly before the
// one-byte length bias desynchronises it, and a 0x00-filled ring corrupts only
// the head of a member.  Both produce output of exactly the declared size.
//
// LOCATING THE CHAIN.  For a PE carrier the chain starts exactly at the PE
// overlay (eleven of eleven), so that offset is tried first and costs one
// memory-map walk.  NE carriers have no dependable overlay calculation - the
// same reason XIS3SFXArchive gives - so the fallback is a bounded scan for the
// record tag that accepts a candidate only when the whole chain from it lands
// on the last byte of the file.  Four of the fifteen samples carry a stray
// "SB1\0" inside the stub's own data; every one of them fails that walk on the
// first record.
//
// NOT CLAIMED.  A bare .SB archive (the same chain at file offset 0, with no
// executable in front of it) is deliberately refused: the carrier must be an
// MZ image and the chain must start behind it.  No such file exists in either
// corpus, and the detection site this reader is wired into only runs for PE and
// NE carriers anyway, so accepting one would be untested code.
class XSBX final : public XArchive {
    Q_OBJECT

public:
    explicit XSBX(QIODevice *pDevice = nullptr);
    ~XSBX() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

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
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nPackedSize;
        qint64 nUncompressedSize;
        quint16 nDosDate;
        quint16 nDosTime;
        quint8 nAttributes;
        QString sFileName;
    };

    struct CONTEXT {
        qint64 nInputSize;
        qint64 nContainerOffset;
        qint64 nArchiveSize;
        QList<MEMBER> listMembers;
    };

    bool locateContainer(qint64 *pnContainerOffset, PDSTRUCT *pPdStruct);
    bool readMember(qint64 nOffset, qint64 nInputSize, MEMBER *pMember, PDSTRUCT *pPdStruct);
    bool walkChain(qint64 nStart, qint64 nInputSize, QList<MEMBER> *pListMembers, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static void fillRecordProperties(const MEMBER &member, QMap<FPART_PROP, QVariant> *pMapProperties);
    static HANDLE_METHOD memberHandleMethod(const MEMBER &member);
    static QString sanitizeName(const QByteArray &baRaw);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSBX_H
