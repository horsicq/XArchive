/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XINTEDUFT_H
#define XINTEDUFT_H

#include "xarchive.h"

// Russian edutainment resource pack (YUMKA / POLIGLOT / TEACHER / ORACLE
// *.DAT, *.6X6).  A flat archive with a leading index and no directory
// structure.
//
// Archive header:
//   quint32 nMagic   always 0x04072E7C
//   quint16 nCount   number of members, never 0
// followed by nCount variable length index entries:
//   qint32 nOffset      absolute offset of the member's data header
//   quint8 nNameLength  1..12
//   char   szName[nNameLength]
//
// Member data header (16 bytes at nOffset):
//   quint16 nVersion            2 in the whole reference corpus
//   quint16 nMethod             0 = stored, 8 = raw deflate
//   quint32 nCRC32              CRC-32 of the decompressed member
//   qint32  nCompressedSize
//   qint32  nUncompressedSize   equals nCompressedSize when stored
// The payload follows immediately.  Members are not required to tile the file
// and the index is not required to be sorted, so the detector leans on the
// magic plus a full walk of the index and of every member data header.
class XINTEDUFT final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nPackedSize;
        qint64 nUncompressedSize;
        quint32 nCRC32;
        quint16 nMethod;
        QString sFileName;
    };

    explicit XINTEDUFT(QIODevice *pDevice = nullptr);
    ~XINTEDUFT() override;

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
        qint64 nIndexSize;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString rawNameToString(const QByteArray &baRawName, qint32 nIndex);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XINTEDUFT_H
