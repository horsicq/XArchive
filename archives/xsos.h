/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSOS_H
#define XSOS_H

#include "xarchive.h"

// "SOS" - the loader/filesystem the Amiga demo group behind the Sanity
// Operating System put on their release disks.  The container is an 880 KiB
// floppy image (.adf): an ordinary AmigaDOS bootblock, so the machine boots it,
// with the group's own flat directory written straight over the rest of the
// disk instead of an AmigaDOS filesystem.
//
// Bootblock (offset 0):
//
//   +0x00  "DOS" + u8 filesystem flag, 0..5
//   +0x04  u32 bootblock checksum        +0x08 u32 rootblock
//   +0x10  "SOS1"     <- the only thing that says this is an SOS disk
//
// The image size must be a whole number of 512-byte sectors.
//
// The directory has no pointer anywhere in the bootblock.  It is found by
// probing the first 32 bytes of sectors 1..30 for a record whose name is
// exactly "loader"; that record is the first entry of the directory, and the
// records run back to back from there.  Each is 32 bytes:
//
//   +0x00  u32 BIG-endian  absolute byte offset of the file in the image
//   +0x04  u32 BIG-endian  file size in bytes
//   +0x08  char[24]        NUL-padded name ("mod", "part2.pp", "SINE.soslibrary")
//
// and the list ends on the first record with a zero offset or an empty name.
// Nothing in the container is compressed - the members are byte ranges of the
// image, so this class needs no codec at all (several members are individually
// packed with PowerPacker, which is what the ".pp" names mean, but that is the
// members' own business and the original does not touch it either).
//
// Reference: the reference implementation handler A009 ("SOS", class zaa, VMT slots 0x00429550 /
// 0x00429570); bootblock check 0x00429020, worker 0x00429110 (its param_1 is
// the path prefix and is always 0 here), member copy 0x0041d690.
class XSOS final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nDataOffset;
        qint64 nSize;
        QString sFileName;
    };

    explicit XSOS(QIODevice *pDevice = nullptr);
    ~XSOS() override;

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

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nDirectoryOffset;
        qint64 nDirectorySize;
        quint8 nDosFlag;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSOS_H
