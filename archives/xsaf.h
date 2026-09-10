/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSAF_H
#define XSAF_H

#include "xarchive.h"

// Stac Electronics SAF archive (.SAF), the installer container shipped with
// Stacker and its companion tools.
//
// Header: the ASCII banner "SAF, (c)1992, ..." terminated by 1A 00.  Two
// banner lengths exist and both are fixed:
//   "SAF, (c)1992, Version 1.00 "                   -> 1A 00 at offset 0x1b,
//                                                      members start at 0x1d
//   "SAF, (c)1992, Stac Electronics, Version 1.00 " -> 1A 00 at offset 0x2d,
//                                                      members start at 0x2f
//
// Member header (35 bytes, little endian), members packed back to back to EOF:
//   char   szName[14]     NUL terminated, DOS 8.3
//   qint32 nRawSize
//   qint32 nPackedSize
//   quint16 nDosDate
//   quint16 nDosTime
//   quint8  nUnknown
//   quint8  nMethod       3 = one stream, otherwise a chunk chain
//   quint32 nCrc          CRC-32 (IEEE, the standard pre/post inverted
//                         form) of the *unpacked* member; verified byte for
//                         byte on all 54 members of the reference corpus
//   quint8  nReserved[3]
// followed by nPackedSize bytes of payload.
class XSAF final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nPackedSize;
        qint64 nRawSize;
        quint32 nMethod;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };

    explicit XSAF(QIODevice *pDevice = nullptr);
    ~XSAF() override;

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
        qint64 nFirstMemberOffset;
        QString sBanner;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bHeaderOnly, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
    static QByteArray methodProperty(quint32 nMethod);
};

#endif  // XSAF_H
