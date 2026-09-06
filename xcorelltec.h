/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XCORELLTEC_H
#define XCORELLTEC_H

#include "xarchive.h"

// Corel / LEAD Technologies "LTEC" installer archive (SETUP.LTA, CorelDRAW
// 4-6 era, 1993-1996).
//
// Layout:  "LTEC", a u32 whose meaning is not established (it is neither the
// archive size, the plaintext size nor any block size - see getFileParts()),
// one zero byte, then a directory that runs straight into the payload:
//
//     u16 record length  = 14 + strlen(name) + 1
//     u32 block offset   - start of the member's block, relative to the
//                          first byte after the directory
//     u32 offset in block- where the member starts inside that block's
//                          PLAINTEXT
//     u32 member size    - plaintext bytes
//     char name[]        - NUL-terminated, 8.3 upper case
//
// There is no directory terminator and no member count.  The directory ends
// where the first block begins, and the first block always sits at block
// offset 0, so the last record is the one whose successor would no longer be
// a well-formed record - which is only safe to decide because the two offset
// fields are fully redundant: consecutive members of one block satisfy
// offset == previous offset + previous size exactly, and the first member of
// a block restarts at zero.  isValid() enforces that chain, so a stray
// "LTEC" cannot be walked into a plausible directory.
//
// The blocks are SOLID.  A member is a byte range inside its block's
// plaintext, and one block routinely carries a dozen members, so a member's
// bytes cannot be produced without decoding the block up to that member's
// end.  Each record therefore publishes the BLOCK as its stream and hands the
// codec the (block size, offset in block) pair through
// FPART_PROP_COMPRESSPROPERTIES; XCorelLtecDecoder returns the slice.
// FPART_PROP_COMPRESSEDSIZE is consequently the block's packed size, shared
// by every member of that block - the format stores no per-member packed
// size, and inventing one by division would be a lie.
class XCorelLtec final : public XArchive {
    Q_OBJECT

public:
    struct BLOCK {
        qint64 nFileOffset;         // absolute offset of the packed block
        qint64 nCompressedSize;     // packed bytes the directory accounts for
        // Bytes the codec actually has to see.  Blocks OVERLAP by up to two
        // bytes: the recorded offset of the next block is the encoder's
        // flushed-byte count, two behind its bit register, so the last
        // symbols of a block live in the first two bytes of the next one -
        // which is also why every block opens with two stale bytes the
        // decoder throws away.  Measured worst case over all corpus blocks is
        // exactly +16 bits.  Feeding only nCompressedSize decodes the tail of
        // most blocks wrongly while the rest of the file still looks right.
        qint64 nStreamSize;
        qint64 nUncompressedSize;   // plaintext bytes
    };

    struct MEMBER {
        qint64 nRecordOffset;
        qint32 nBlockIndex;
        qint64 nOffsetInBlock;
        qint64 nSize;
        QString sFileName;
    };

    explicit XCorelLtec(QIODevice *pDevice = nullptr);
    ~XCorelLtec() override;

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
        qint64 nArchiveSize;
        qint64 nDirectoryOffset;
        qint64 nDirectorySize;
        quint32 nHeaderValue;
        QList<BLOCK> listBlocks;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bProbeStream, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XCORELLTEC_H
