/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XRECOGNITA_H
#define XRECOGNITA_H

#include "xarchive.h"

// Recognita OCR distribution container (*.CMP).  The archive has no signature
// of its own: it is a bare chain of member headers, each immediately followed
// by one complete PKWARE Data Compression Library implode stream.
//
// Member header (25 bytes, little endian):
//   char   szName[13]   NUL terminated 8.3 name inside a fixed 13-byte field
//   qint32 nSize        uncompressed size
//   quint16 nDosTime
//   quint16 nDosDate
//   qint32 nNextOffset  absolute offset of the next member header
//
// The implode stream starts right after the header and runs to nNextOffset;
// the last member's nNextOffset is the file size, so the chain tiles the file
// exactly.  Every stream in the reference corpus starts with the DCL header
// bytes 00 06 (binary literals, 4 KiB dictionary) and the reference extractor
// requires exactly that pair, so it is part of the detector.
class XRecognita final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nPackedSize;
        qint64 nUncompressedSize;
        quint16 nDosTime;
        quint16 nDosDate;
        QString sFileName;
    };

    explicit XRecognita(QIODevice *pDevice = nullptr);
    ~XRecognita() override;

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
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool isValidRawName(const char *pRawName);
    static bool isValidDosDateTime(quint16 nDosDate, quint16 nDosTime);
    static QString rawNameToString(const char *pRawName, qint32 nIndex);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XRECOGNITA_H
