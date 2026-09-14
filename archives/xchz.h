/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XCHZ_H
#define XCHZ_H

#include "xarchive.h"

// CHZ - the container of ChArc 1.1 / 1.2 (S.Chernivetsky, SP "Dialog", Moscow
// 1990), both as a bare ".chz" archive and inside its "ChSFX (small)" 16-bit
// DOS self-extractor.  The two are the SAME container: the self-extractor is
// a DOS MZ stub (1816 bytes in the v1.1 builder, 1850 in the v1.2 one) with
// the untouched chain appended behind it, which is why one reader answers for
// both and why the stub length is never assumed.
//
// THE CHAIN.  There is no archive header, no central directory and no count:
// a flat run of tagged records that ends on the last byte of the file.  Three
// tags exist and the first byte of each is what selects them:
//
//   "SChF"  a file.  0x18-byte header then the name then the payload:
//             0x00 char   tag[4]
//             0x04 qint32 nRecordSize   WHOLE record: 0x18 + name + payload
//             0x08 qint32 nUncompressedSize
//             0x0c quint32 nCheck       see NOT PUBLISHED below
//             0x10 quint16 nDosTime
//             0x12 quint16 nDosDate     TIME FIRST, then DATE
//             0x14 quint8  nMethod      0 = stored, 1 = the ChArc codec
//             0x15 quint8  (constant 0x11 on all 124 reference members)
//             0x16 quint16 nNameLength  1..0xffff, never 0
//             0x18 char    szName[nNameLength]   NOT NUL terminated
//   "SChD"  enter a subdirectory.  A 10-byte record - tag, four bytes this
//           reader does not interpret, a byte that MUST be zero and the name
//           length - followed by the name.
//   "SChd"  leave it again.  Four bytes and nothing else.
//
// THE TWO TIME WORDS ARE TIME-THEN-DATE.  Read the other way round the ten
// reference carriers land in 1988 and 2049; read this way the ChArc 1.2
// self-extractor stamps 1990-12-06 14:45:54 on CHARC.EXE, and its own
// CHARC.TXT says "Version 1.2 of 06 December 1990".  Nothing in the record
// flags the order, so a swapped reading produces plausible dates, not an
// error.
//
// CODEC.  Method 1 is HANDLE_METHOD_CHARC - an order-1 context-modelled LZ77
// with static per-context Huffman tables, described in Algos/xcharcdecoder.h.
// Method 0 is stored, and is only accepted when the payload length equals the
// declared decoded length, which is the reference implementation's own rule.
// Any other method value is listed with HANDLE_METHOD_UNKNOWN rather than
// guessed at.
//
// A SPLIT ARCHIVE IS LISTED UP TO THE CUT.  ChArc spans volumes by letting
// the last record of a volume declare a size the volume cannot hold
// ("RUSVOC1.BIN" in the reference set declares 383756 bytes with 22865 left).
// The complete records before it are real and are listed; the cut record is
// NOT, because its payload is not in this file.  The reference implementation
// behaves the same way - it emits what it walked and then reports failure.
//
// LOCATING THE CHAIN.  Offset 0 is tried first, which is the bare .chz case.
// Otherwise the carrier must be a plain 16-bit DOS MZ image - a PE, NE, LE or
// LX signature disqualifies it, because the ChSFX stub is 16-bit DOS only -
// and a bounded scan takes the first "SCh?" whose whole chain walks to the end
// of the file.  Over 73,838 files of F:\ARC and F:\tests that predicate fires
// on the twenty ChArc carriers (ten masters plus their ten staged copies) and
// on nothing else.
//
// NOT PUBLISHED.  The quint32 at 0x0c is a per-member check value; it is NOT
// CRC-32 and no table-driven CRC with any of the twelve common polynomials,
// either bit order, both init values and both final XORs reproduces it, on the
// one member that is stored and therefore needs no decoder at all.  ChArc 1.2
// computes it bitwise - charc.exe carries no lookup table - and it is
// unidentified, so this reader neither verifies it nor advertises it as a
// checksum.  The byte at 0x15 is likewise left alone: it is 0x11 on every
// reference member, which as a DOS attribute byte would mean READONLY |
// DIRECTORY on a plain file, so publishing it as attributes would be a
// fabrication.
class XCHZ final : public XArchive {
    Q_OBJECT

public:
    explicit XCHZ(QIODevice *pDevice = nullptr);
    ~XCHZ() override;

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
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nPackedSize;
        qint64 nUncompressedSize;
        quint16 nDosTime;
        quint16 nDosDate;
        quint8 nMethod;
        QString sFileName;
    };

    struct CONTEXT {
        qint64 nInputSize;
        qint64 nContainerOffset;
        qint64 nArchiveSize;
        bool bTruncated;
        QList<MEMBER> listMembers;
    };

    bool locateContainer(qint64 *pnContainerOffset, PDSTRUCT *pPdStruct);
    bool walkChain(qint64 nStart, qint64 nInputSize, QList<MEMBER> *pListMembers, bool *pbTruncated, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool isDosCarrier(PDSTRUCT *pPdStruct);
    static void fillRecordProperties(const MEMBER &member, QMap<FPART_PROP, QVariant> *pMapProperties);
    static HANDLE_METHOD memberHandleMethod(const MEMBER &member);
    static QString methodToString(quint8 nMethod);
    static bool isPlainName(const QByteArray &baRaw);
    static QString sanitizeName(const QByteArray &baRaw);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XCHZ_H
