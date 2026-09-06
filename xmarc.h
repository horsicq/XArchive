/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XMARC_H
#define XMARC_H

#include "xarchive.h"

// "MARC" version 3 resource archive (.mar), the resource-pack container used by
// MSN Explorer / the MSN client (msn###.mar, ui.mar, themedef.mar, mail.mar).
//
// Header (12 bytes, little endian):
//   char   szMagic[4]        "MARC"
//   quint32 nVersion         always 3
//   qint32  nNumberOfEntries
//
// Directory (nNumberOfEntries * 68 bytes, immediately behind the header):
//   char   szName[56]  NUL terminated; the reference extractor forces byte 55
//                      to NUL, so 55 characters is the effective maximum
//   qint32 nSize
//   quint32 nCRC32     CRC-32 of the member payload
//   qint32 nOffset
//
// Nothing is compressed: every member is a verbatim slice of the file, so
// HANDLE_METHOD_STORE covers the whole format and no new codec is introduced.
// Zero-length members are legal and common (they share the offset of the next
// member).
class XMARC final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDataOffset;
        qint64 nSize;
        quint32 nCRC32;
        QString sFileName;
    };

    explicit XMARC(QIODevice *pDevice = nullptr);
    ~XMARC() override;

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
        qint64 nDirectoryOffset;
        qint64 nDirectorySize;
        qint32 nNumberOfEntries;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString rawNameToString(const char *pRawName, qint32 nIndex);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XMARC_H
