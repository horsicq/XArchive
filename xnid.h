/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XNID_H
#define XNID_H

#include "xarchive.h"

// "NI" install-set volume (DISK001.NID, RAID001.DAT, Z.PAC).  A DOS-era
// installer data volume; the vendor is not identifiable from the bytes.
//
// Header (0x78 bytes, little endian):
//   char   szMagic[2]       "NI"
//   quint8 nVersion[2]      0x15 0x01
//   quint16 nChecksum       not verified by the reference extractor
//   quint16 nNumberOfFiles  never 0
//   quint8 nReserved[0x70]  zero in every sample
//
// Directory (nNumberOfFiles entries of 29 bytes, at 0x78):
//   quint8  nMethod         always 1; the reference extractor stops the listing
//                           at the first entry that is not 1 (the continuation
//                           entries of a following volume carry 2)
//   qint32  nOffset         data offset, ONE BASED - subtract 1
//   quint32 nFolderKey      opaque; files sharing a key share an output folder,
//                           numbered "Folder1", "Folder2", ... in the order the
//                           keys are first seen
//   char    szName[11]      FCB style, 8 + 3, space padded
//   quint8  nAttributes     DOS attribute byte
//   quint16 nTime, nDate    DOS timestamp
//   qint32  nSize           uncompressed size
//
// A member is a chain of five-byte-framed blocks (see XNIDDecoder); the member
// extent runs from its data offset to the end of the block carrying the
// last-block flag.  Members whose chain leaves the file (a set split across
// volumes) are still listed and fail at extraction, exactly as the reference
// extractor does.
class XNID final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nFolderKey;
        qint32 nFolderIndex;
        quint16 nTime;
        quint16 nDate;
        bool bSingleStoredBlock;
        QString sFileName;
    };

    explicit XNID(QIODevice *pDevice = nullptr);
    ~XNID() override;

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
    static QString fcbNameToString(const char *pRawName, qint32 nIndex);
    static bool isValidFcbName(const char *pRawName);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);

    bool measureMember(MEMBER *pMember, qint64 nInputSize, PDSTRUCT *pPdStruct);
};

#endif  // XNID_H
