/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XDMAPACKED_H
#define XDMAPACKED_H

#include "xarchive.h"

// DMA packed file (".PK$"): the single-file compressor Dynamic Microprocessor
// Associates shipped its DOS pcANYWHERE install diskettes with.  Every packed
// file on those disks is one of these - AWHOST.EXE, AWREMOTE.EXE, AWVGA.DRV,
// BTRIEVE.EXE and so on, each in its own AW*.PK$ container.
//
// Layout (all integers little endian), 0x22 bytes of header:
//
//   0x00  quint8[4]  magic       64 6D 10 11 ('d','m',0x10,0x11)
//   0x04  char[14]   fileName    NUL terminated; the bytes behind the NUL are
//                                uninitialised writer memory, not padding, so
//                                the name ends at the FIRST NUL and the rest of
//                                the field is never read
//   0x12  qint32     rawSize     length of the plaintext
//   0x16  quint16    dosDate     DOS date, then DOS time - in THAT order, which
//   0x18  quint16    dosTime     is what makes all three stamps of the corpus
//                                (1989-02-08 14:32:20, 1990-07-12 17:21:48,
//                                1991-12-20 04:50:00, one per release) legal
//                                dates; read the other way round two of them
//                                name day 0
//   0x1A  char[6]    "PAKPAK"
//   0x20  quint16    0x2A00      writer version; constant over the corpus
//   0x22  ...        payload     PKWARE DCL Implode stream, prelude included
//
// THE NAME IN THE HEADER IS THE TRUE INSTALLED NAME (AW.TRM, AWDVR_0.OVL,
// VPCAW.386) and is published verbatim; the container's own name is never used.
//
// The payload is a plain PKWARE DCL Implode ("blast") stream whose own two-byte
// prelude is part of the payload; the whole corpus writes 00 06 (uncoded
// literals, 4K window), and the stream ends at the last byte of the file in all
// 48 reference files.  rawSize is redundant with the stream's end-of-stream
// code and this format has NO checksum anywhere, so that redundancy is the only
// integrity check there is - isValid() trial-decodes the stream and requires
// the two to agree exactly.  The reference implementation gates on the 4-byte
// magic, rawSize >= 0 and the 0x2A00 word alone; that is too weak to claim a
// file with, which is why the trial decode is added here.
class XDMAPacked final : public XArchive {
    Q_OBJECT

public:
    explicit XDMAPacked(QIODevice *pDevice = nullptr);
    ~XDMAPacked() override;

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
    struct MEMBER {
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };

    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        QList<MEMBER> listEntries;
    };

    // bVerifyPayload = true runs the bounded trial decode described above.  It
    // is the gate every entry point that has to be sure uses; getFileParts()
    // and the unpack path additionally need it because the reported plaintext
    // length has to be one a decoder can actually reproduce.
    bool parseContext(CONTEXT *pContext, bool bVerifyPayload,
                      PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XDMAPACKED_H
