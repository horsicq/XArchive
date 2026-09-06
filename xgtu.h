/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XGTU_H
#define XGTU_H

#include "xarchive.h"

// OS/2 software-distribution kit (".CSD" corrective service diskettes and their
// ".001"/".002" continuation volumes) written by the IBM "GTU" packaging tool.
//
// The container has no magic string at all.  Its only global field is the u32 at
// +0, which is the total size of the file; everything else is a self-checking
// index.  The index starts at +4 and is a chain of records
//
//     +0x00 u32  offset of this member's data block
//     +0x04 u16  always 1
//     +0x06 u16  member kind, 0 or 1 (see XGTU::parseContext)
//     +0x08 u16  always 0
//     +0x0a u16  name length (never 0)
//     +0x0c      name[nameLength], obfuscated
//
// terminated by a record whose data-block offset equals the record's own file
// offset.  The name is stored under a running-difference cipher seeded with the
// name length (see XGTU::decodeName): plain[i] = enc[i] - key, key = plain[i].
//
// The data block at that offset repeats the same 12-byte record header and the
// same (still obfuscated) name verbatim - which is what makes a headerless
// format safely detectable - followed by a 28-byte information block
//
//     +0x00 u16  MS-DOS date        +0x02 u16  MS-DOS time
//     +0x04 u32  second timestamp   +0x08 u32  third timestamp
//     +0x0c i32  uncompressed size of the member
//     +0x10 i32  size rounded up to the volume's allocation unit
//     +0x14 u32  attribute word
//     +0x18 i32  number of already-delivered uncompressed bytes to skip
//
// and then a chain of compression frames, each  [i32 rawSize][i32 packedSize]
// followed by packedSize bytes.  rawSize is 65536 for every frame but the last
// one of a member.  The +0x18 field lets a member resume in the middle of the
// frame chain (the continuation volumes use it); those leading frames are
// stepped over without being decoded, so the part this class publishes starts
// at the first frame that actually contributes output.
//
// Every frame is one complete Okumura LZARI stream (LZSS over an adaptive
// binary arithmetic coder, N = 4096, F = 60, THRESHOLD = 2, N_CHAR = 314,
// M = 15) - the very same codec the AMPK family already carries, so no new
// codec is introduced: HANDLE_METHOD_GTU only adds the frame walk on top of
// XAMPKDecoder::decodeLZARI.
class XGTU final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nIndexOffset;        // record inside the index chain
        qint64 nBlockOffset;        // repeated header in the data area
        qint64 nInfoOffset;         // 28-byte information block
        qint64 nDataOffset;         // first frame that produces output
        qint64 nCompressedSize;     // bytes from nDataOffset to the last frame
        qint64 nUncompressedSize;   // information block +0x0c
        qint64 nAllocatedSize;      // information block +0x10
        qint64 nSkipSize;           // information block +0x18
        quint32 nAttributes;        // information block +0x14
        quint16 nKind;              // index record +0x06 (0 or 1)
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };

    explicit XGTU(QIODevice *pDevice = nullptr);
    ~XGTU() override;

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

    // Running-difference name cipher, seeded with the stored name length.
    static QByteArray decodeName(const QByteArray &baEncoded);

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nIndexOffset;
        qint64 nIndexSize;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool measureFrames(MEMBER *pMember, qint64 nInputSize,
                       PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XGTU_H
