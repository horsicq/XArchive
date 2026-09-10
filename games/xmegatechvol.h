/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XMEGATECHVOL_H
#define XMEGATECHVOL_H

#include "xarchive.h"

// Megatech Software game resource volume (".VOL").
//
// There is no magic and no per-member metadata at all: the file opens with a
// flat table of little-endian u32 absolute file offsets and nothing else.
//
//     +0x00 u32  offset of member 0 - which is also the byte size of the table
//     +0x04 u32  offset of member 1
//     ...        one entry per member, strictly non-decreasing
//                a member's size is table[i + 1] - table[i]
//                the last used entry equals the file size (the end sentinel)
//                every entry after it is 0 (the table is zero padded to its
//                declared length, 0x80 or 0x100 bytes in the known corpus)
//
// Because table[0] doubles as the table length the container is self-checking,
// and that is the whole of the reference implementation's detector:
//
//   * table[0] >= 8 and table[0] % 4 == 0 and table[0] <= file size;
//   * walking i from 0, entries must be non-negative, <= file size and
//     non-decreasing, until one entry equals the file size and is either the
//     last entry or is followed by a 0 - that terminates the walk;
//   * every remaining entry must be 0.
//
// Members are stored verbatim, so no codec is involved: the parts are published
// with HANDLE_METHOD_STORE and no HANDLE_METHOD of this family's own is needed.
//
// The container carries no names either. The reference implementation
// numbers the non-empty members from 1 and picks the extension by peeking at
// the first u32 of the member's data, which is what this class reproduces:
// "Crea" (Creative Voice File) -> .voc, 47 50 48 1d ("GPH\x1d") -> .gph,
// anything else -> .bin.  Zero-length gaps (repeated offsets, common in this
// corpus) are skipped entirely and do not consume a number.
class XMegatechVOL final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nTableIndex;   // index into the raw offset table
        qint64 nDataOffset;   // table[i]
        qint64 nDataSize;     // table[i + 1] - table[i]
        QString sFileName;    // "<n>.<ext>", n counting non-empty members
    };

    explicit XMegatechVOL(QIODevice *pDevice = nullptr);
    ~XMegatechVOL() override;

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
        qint64 nTableSize;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XMEGATECHVOL_H
