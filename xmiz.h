/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XMIZ_H
#define XMIZ_H

#include "xarchive.h"

// MIZ - "DKCL"/"SBRW" single-file compressed installer member, as shipped by
// the Intuit QuickBooks / QuickOrder DOS+Windows installers (early/mid 1990s).
// Extension .MIZ; exactly one payload file per container, no directory.
//
// Layout, fully accounted for (little endian throughout):
//
//   +0x00  "DKCL"            magic
//   +0x04  u16  0x0001       container version; U3 accepts only 1
//   +0x06  "SBRW"            second magic
//   +0x0a  u16  nameLen      length of the name FIELD, NUL included
//   +0x0c  char name[nameLen]   8.3 name, NUL-terminated inside the field
//        (then, at 0x0c + nameLen:)
//   +0x00  u16  dosTime
//   +0x02  u16  dosDate
//   +0x04  i32  uncompressedSize   U3 rejects a negative value
//   +0x08  i32  compressedSize     U3 rejects a negative value
//   +0x0c  payload[compressedSize]
//          "MJDK"            4-byte trailer, checked after decompression
//
// The payload is a plain PKWARE Data Compression Library ("implode"/blast)
// stream: it carries its own two-byte preamble (literal mode 0/1, dictionary
// exponent 4/5/6), so no codec parameters live in the MIZ header and the
// already-present HANDLE_METHOD_PKWARE_DCL_IMPLODE decodes it unchanged.
class XMiz final : public XArchive {
    Q_OBJECT

public:
    explicit XMiz(QIODevice *pDevice = nullptr);
    ~XMiz() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
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
        qint64 nHeaderSize;   // magic + name field + the 12-byte record
        qint64 nDataOffset;   // == nHeaderSize
        qint64 nFooterOffset; // "MJDK"
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint16 nVersion;
        QString sFileName;
        QDateTime dtModified;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XMIZ_H
