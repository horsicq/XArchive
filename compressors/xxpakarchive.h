/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XXPAKARCHIVE_H
#define XXPAKARCHIVE_H

#include "xarchive.h"

// "XPAK" single-file compressed container.
//
// NAME COLLISION, read before grepping: the class XPAK in XArchive/games/xpak.h
// is the Quake PACK resource format ('PACK' magic) and has nothing to do with
// this one; XPAKLEOArchive is PAKLEO (.PLL).  This class is the container whose
// magic literally is 'XPAK', hence the doubled X.
//
//   0x00 char     szMagic[4]   "XPAK"
//   0x04 quint32  nFileSize    LITTLE endian, the size of this whole file
//   0x08 char     szName[13]   member name, NUL padded, printable ASCII
//   0x15 quint32  nRawSize     LITTLE endian, uncompressed size of the member
//   0x19 quint8   params[10]   codec parameter block
//   0x23 coded stream, runs to EOF
//
// THE CODEC IS NOT IMPLEMENTED.  The container above is fully recovered and
// verified against all 65 samples, so the archive lists correctly - name, both
// sizes and the extents are all read straight out of the header - but the
// stream itself is not decoded, and a member therefore fails to extract with
// "Unknown compression method".  That is deliberate and it is the reason this
// class publishes HANDLE_METHOD_XPAK rather than leaving the record at the
// HANDLE_METHOD_STORE that an unset field would mean: a record that named no
// method would be run through the STORE path and would hand the caller the
// still-compressed bytes as though they were the file.  Listing a member the
// decoder cannot produce is honest; inventing its contents is not.
//
// What is known about the stream, measured over the 65-file corpus, so the next
// person does not have to redo it:
//   - params[] is byte-identical in every sample: 09 FF FE 00 80 00 04 00 20 FF.
//     It is published as FPART_PROP_COMPRESSPROPERTIES so that a future decoder
//     receives it from the record instead of re-reading the container.
//   - The coded stream is bit-oriented with P(1) = 0.4740 and a byte histogram
//     that is monotonic in popcount, matching independent bits; it is therefore
//     entropy-coded but NOT arithmetic-coded.
//   - It carries no transmitted Huffman table (no low-entropy region anywhere,
//     including immediately after params[]) and shows no periodicity at any
//     period <= 40, so the codes are variable-length rather than fixed-width.
//   - LZW, fixed-field LZSS and the LZHUF/adaptive-Huffman family were each
//     implemented and tested against the exact bit-budget oracle (decode must
//     yield nRawSize bytes consuming exactly the stream) and all three are
//     excluded.  The surviving hypothesis is LZ77 over a static Huffman table
//     built into the original decoder, which cannot be recovered from the
//     container because the table is not in it.
class XXPAKArchive final : public XArchive {
    Q_OBJECT

public:
    explicit XXPAKArchive(QIODevice *pDevice = nullptr);
    ~XXPAKArchive() override;

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
        qint64 nDeclaredSize;  // header field, NOT trusted for extents
        qint64 nStreamOffset;
        qint64 nStreamSize;  // what is actually present in the device
        qint64 nRawSize;
        bool bTruncated;  // nDeclaredSize > nInputSize
        QString sFileName;
        QByteArray baParams;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
    // ISSUE-25 fixed: the console prints archive-level FPART_PROP_INFO now, so
    // the truncation warning goes back to that channel and the method column
    // names only the codec. The warning itself is set in initUnpack().
};

#endif  // XXPAKARCHIVE_H
