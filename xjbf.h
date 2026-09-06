/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XJBF_H
#define XJBF_H

#include "xarchive.h"

// Headerless DOS archiver whose only known samples are the "JBF.1" data files
// shipped with the 1993/1994 game "The Jelly Bean Factory"; nothing in the
// bytes names the tool that wrote them.
//
// Members are concatenated from offset 0 with no per-member header at all.  The
// directory is a plain trailer:
//
//     [member 0 data][member 1 data]...[dir entry 0]...[dir entry n-1][u8 n]
//
// The very last byte of the file is the member count, so the directory starts
// at fileSize - (count * 31 + 1).  Each 31-byte entry is
//
//     +0x00  char[13]  8.3 name, NUL padded
//     +0x0d  u32       compressed size
//     +0x11  u32       uncompressed size
//     +0x15  u32       absolute offset of the member's data
//     +0x19  u16       integrity word.  U3 wraps the output stream in a 16-bit
//                      checksum filter seeded with 0x3456 and then compares
//                      this field against the one's complement of the result
//                      (FUN_00623970).  The filter's update rule was not
//                      recovered, so this class neither verifies nor publishes
//                      the field (publishing it as FPART_PROP_UNCOMPRESSEDCRC
//                      would make the console compare it against a CRC-32).
//     +0x1b  u16       MS-DOS time
//     +0x1d  u16       MS-DOS date
//
// Every member is one complete LZHUF stream, the same sub-variant the "!HZL"
// compressor uses (see Algos/xhzldecoder.h), so this class publishes
// HANDLE_METHOD_HZL and adds no codec of its own.
//
// Detection: the container has no magic, so the class validates the trailer
// arithmetic (the directory must tile the file exactly: offsets ascending,
// every member inside the data area, and the last member must end at the
// directory).  U3 additionally keys on the fixed eight bytes at +0 - which are
// just the first bytes of the first member's LZHUF stream - and the same
// constant is kept here as a cheap first filter, spelled out as
// JBF_LEAD_SIGNATURE.
class XJBF final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDirOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint16 nChecksum;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };

    explicit XJBF(QIODevice *pDevice = nullptr);
    ~XJBF() override;

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
        qint64 nDirOffset;
        qint64 nDirSize;
        QList<MEMBER> listEntries;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
};

#endif  // XJBF_H
