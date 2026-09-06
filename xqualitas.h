/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XQUALITAS_H
#define XQUALITAS_H

#include "xarchive.h"

// Qualitas 386MAX / BlueMAX install-disk archive.  The disk number is the file
// extension (386MAX.1, 386MAX.2, ...); every disk is a self-contained archive
// whose directory lists the WHOLE product, so the members that live on later
// disks are present in the directory of disk 1 as well.
//
// The container is HEADERLESS - there is no magic - so detection rests entirely
// on the arithmetic of the fixed header and the shape of the first directory
// record, exactly as the reference extractor does it.
//
// Header (14 bytes, little endian):
//   quint32 nChecksum        not verified by the reference extractor
//   quint16 nHeaderSize      always 0x000E
//   quint16 nDirectorySize   directory bytes; first payload sits at 14 + this
//   quint16 nUnknown8
//   quint16 nNumberOfFiles   never 0
//   quint16 nUnknown12
//
// Directory (starts at 0x0E, one variable-length record per file):
//   qint32  nNextRecord      absolute offset of the record that follows
//   quint16 nUnknown4
//   qint32  nDataOffset      absolute offset of the member payload
//   quint16 nDosTime
//   quint16 nDosDate
//   qint32  nUncompressedSize
//   qint32  nCompressedSize  INCLUDING the 4-byte CRC in front of the payload
//   quint16 nAttributes      DOS attribute word
//   quint8  nDiskNumber      1 == "on this disk"; higher values end the walk
//   quint8  nMethod          0 or 1; both are PKWARE DCL implode streams
//   char    szName[]         ASCIIZ, immediately after the 26 fixed bytes
//
// Member payload: quint32 nCrcComplement (the one's complement of a CRC-32 over
// the plaintext) followed by nCompressedSize - 4 bytes of PKWARE Data
// Compression Library implode (lit=0, dict=6 in every sample), which is the
// already-present HANDLE_METHOD_PKWARE_DCL_IMPLODE - no new codec.
//
// A record whose disk number is not 1, or whose method byte is above 1, ENDS
// the directory walk rather than being skipped: that is what the reference
// extractor does, and it is what keeps a disk-1 image from advertising members
// whose bytes live on a disk this file does not contain.
class XQualitas final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nDataOffset;       // payload start, i.e. the CRC word
        qint64 nStreamOffset;     // nDataOffset + 4, the implode stream
        qint64 nCompressedSize;   // stream size, CRC word excluded
        qint64 nUncompressedSize;
        quint16 nDosTime;
        quint16 nDosDate;
        quint16 nAttributes;
        quint8 nMethod;
        QString sFileName;
    };

    explicit XQualitas(QIODevice *pDevice = nullptr);
    ~XQualitas() override;

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
    QList<FPART_PROP> getAvailableFPARTProperties() override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nDirectorySize;
        qint32 nNumberOfFiles;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString rawNameToString(const QByteArray &baRawName, qint32 nIndex);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XQUALITAS_H
