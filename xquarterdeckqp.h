/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XQUARTERDECKQP_H
#define XQUARTERDECKQP_H

#include "xarchive.h"

// Quarterdeck QIP install package, "QP" variant (.QIP / .QIF; DESQview,
// QEMM, DESQview/X 1993-1996).
//
// Layout:  a 16-byte file header ("QP", u16 file count, u32 index size,
// u16 version 2, six zero bytes), then a fixed-stride index of
// count * 16 bytes (u32 record offset + 12-byte name field), then a chain
// of "QD" records.  Two record kinds share the "QD" magic and are told
// apart by the u16 at +0x02: type 1 is a destination-directory record with
// a u32 name length, type 0 is a file record with a fixed 36-byte header
// (13-byte 8.3 name field included) followed by a complete PKWARE DCL
// Implode stream carrying its own two-byte prelude.
//
// The index is what makes the format self-validating: for every one of the
// 232 corpus samples the index size equals count*16, every index offset is
// exactly a type-0 record offset, every index name equals that record's
// name, and the last member's payload ends precisely on EOF.  isValid()
// enforces all four, so a stray "QP" prefix cannot be accepted.
//
// Directory records are parsed and skipped, not applied to the member
// names.  They are install-time destinations, some of them relative
// ("..\\.", "."), and the reference extractor writes every member flat.
class XQuarterdeckQP final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nCRC32;
        quint16 nSequence;
        quint16 nDosTime;
        quint16 nDosDate;
        quint8 nAttributes;
        QString sFileName;
        QString sPath;
    };

    explicit XQuarterdeckQP(QIODevice *pDevice = nullptr);
    ~XQuarterdeckQP() override;

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
        qint64 nIndexOffset;
        qint64 nIndexSize;
        qint32 nVersion;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XQUARTERDECKQP_H
