/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XQIP1_H
#define XQIP1_H

#include "xarchive.h"

// Quarterdeck QIP install archive, "QD" member-stream variant (QRAM.QIP,
// QEMM386.QIP and friends).  This variant has NO package index at all: the file
// is nothing but member records laid end to end, each one
//
//   +0x00 u16   'QD' (0x4451)
//   +0x02 u16   0
//   +0x04 i32   compressed size of this member's stream
//   +0x08 u16   member sequence number, 1 for the first record
//   +0x0a u8    flag byte (0x21 throughout the reference corpus; it does not
//               select the codec - every member is compressed the same way)
//   +0x0b u16   MS-DOS time
//   +0x0d u16   MS-DOS date
//   +0x0f i32   uncompressed size
//   +0x13 char[13] member name, NUL padded (the last byte is forced to NUL, so
//               12 characters is the real ceiling)
//   +0x20       compressed stream, compressedSize bytes
//
// The stream is a complete PKWARE Data Compression Library "implode" stream,
// header included (the corpus is uniformly literal mode 0, 4 KiB dictionary),
// so this class introduces no codec of its own and rides
// HANDLE_METHOD_PKWARE_DCL_IMPLODE.
//
// The chain is walked to the end of the file; because each record states its
// own stream size, a container whose records tile the file exactly is a much
// stronger signal than the 2-byte magic, and that is what detection requires.
class XQIP1 final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint16 nSequence;
        quint16 nDosDate;
        quint16 nDosTime;
        quint8 nFlags;
        QString sFileName;
    };

    explicit XQIP1(QIODevice *pDevice = nullptr);
    ~XQIP1() override;

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
        qint64 nArchiveSize;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XQIP1_H
