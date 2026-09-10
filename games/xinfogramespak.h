/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XINFOGRAMESPAK_H
#define XINFOGRAMESPAK_H

#include "xarchive.h"

// Infogrames / Alone in the Dark engine .PAK resource container (AITD 1-3,
// Time Gate, Jack in the Dark; also shipped by the same engine's tooling as
// LISTANIM/LISTTRAK/LISTLIFE/ITD_RESS).
//
// The format is HEADERLESS: the file opens directly on a u32 offset table.
// Slot 0 is always 0 and slot 1 is the byte size of the table itself, which
// doubles as the offset of the first record.
//
// The table is an INDEX MAP, not a member list.  It has one slot per resource
// id the engine may ask for, and ids the archive never fills point at whatever
// record the writer last had in hand - usually a placeholder, but not always,
// and the dead slots are not confined to the end: 85_pubalbzogaqtfsrp_MK052500
// parks slots 15..20 and 23..30 on one record while slots 21 and 22 carry real
// new offsets in between.  Counting slots therefore counts a record once per
// id that aliases it; on the 367-archive reference corpus that inflates 13180
// members into 60 identical copies of one 1206-byte record for the smallest
// archive alone.  Membership comes from the RECORD CHAIN instead, and the
// table is used only to name each member (its resource id) and to validate:
// every non-zero slot has to land on a chain record.
//
// The chain starts at table[1] and tiles the file to the last byte.  A member
// is an optional 16-byte crumb, then a 16-byte header - u32 additional-
// descriptor size, u32 packed size, u32 unpacked size, u8 method, u8 info,
// u16 inline-descriptor size - then both descriptors, then the payload; the
// next member begins where that payload ends.  The inline descriptor, when
// present, is 0x49 <size> followed by a NUL-padded 8.3 name.
//
// Methods: 0 = stored, 4 = raw deflate (zlib -15), 1 = the PKZIP-implode
// variant in XInfogramesPakDecoder.  The crumb in front of a record is a
// PKZIP local file header cut off after two CRC bytes, so the checksum it
// carries is the LOW HALF of the CRC32 of THAT record's unpacked member - a
// genuine checksum, but only 16 of the 32 bits, so it is used for validation
// here and deliberately not published as FPART_PROP_RESULTCRC (the framework
// would compare it against a full CRC32 and fail every member).  Six corpus
// archives close with a 16-byte "PK\1\2" crumb - the head of a central
// directory record that was cut off with everything behind it - which ends
// the chain.
class XInfogramesPak final : public XArchive {
    Q_OBJECT

public:
    struct ENTRY {
        // Resource id: the first offset-table slot that names this record, or
        // -1 when no slot does.  It is NOT the position in the chain.
        qint32 nIndex;
        // Start of the 16-byte PKZIP crumb in front of the record, or -1.
        qint64 nCrumbOffset;
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint8 nMethod;
        qint32 nDescriptorSize;
        qint32 nExtraSize;
        bool bHasCrc;
        quint16 nCrc16;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sStoredName;
        QString sFileName;
    };

    explicit XInfogramesPak(QIODevice *pDevice = nullptr);
    ~XInfogramesPak() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;
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
        qint64 nTableSize;
        qint64 nArchiveSize;
        // Start of the trailing PKZIP crumb that ends the chain, or -1 when
        // the last member runs right up to the final byte.
        qint64 nFooterOffset;
        QList<ENTRY> listEntries;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool trialDecode(const ENTRY &entry, PDSTRUCT *pPdStruct);
    static QString methodToString(quint8 nMethod);
    static HANDLE_METHOD methodToHandleMethod(quint8 nMethod);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XINFOGRAMESPAK_H
