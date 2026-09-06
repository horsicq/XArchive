/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XPACKIT_H
#define XPACKIT_H

#include "xarchive.h"

// "PACKIT by MJP" - the self-describing bundle format used by the Blue Wave
// Offline Mail Reader installers (BWAVEDOS.INS / BWAVE386.INS / BWAVEOS2.INS)
// and by a handful of other DOS/OS-2 setup payloads (INSTALL.DAT).
//
//   +0x00  "PACKIT by MJP" 0D 0A 1A            16-byte signature
//
// then a flat chain of members, each
//
//   +0x00  u16  0x00FF                         record tag
//   +0x02  i32  member size
//   +0x06  i32  member size, repeated
//   +0x0a  u16  MS-DOS date
//   +0x0c  u16  MS-DOS time
//   +0x0e  u32  CRC-32 of the member, stored as the running register
//               (init 0xFFFFFFFF, no final complement)
//   +0x12  u8   name length, never 0
//   +0x13  .    name[nameLength]
//   .      u8   0x00 terminator
//   .      .    member bytes, uncompressed
//
// and a u16 0xFFFF where the next tag would be.  Nothing in the container is
// compressed: the duplicated size field is the format's only "method" and both
// copies must agree.
//
// Members are stored back to back, so a truncated archive still holds every
// member before the cut.  XPACKIT publishes those plus, when the last member
// runs past end of file, that member clamped to the bytes that survive - the
// reference reader writes exactly those bytes before it reports the archive as
// damaged, and the stored CRC-32 then no longer matches.
//
// Reference: U3 handler "PACKIT" (class rra, entry A231), detector at VA
// 0x00562ed0, worker at 0x00563050, member copier at 0x00562f40.
class XPACKIT final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nDataOffset;
        qint64 nSize;
        quint32 nCrc32Register;  // init 0xFFFFFFFF, NOT complemented
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };

    explicit XPACKIT(QIODevice *pDevice = nullptr);
    ~XPACKIT() override;

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
        bool bTruncated;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XPACKIT_H
