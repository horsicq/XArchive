/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XEALIB_H
#define XEALIB_H

#include "xarchive.h"

// EALIB library (.LIB): the Electronic Arts DOS-era resource container used by
// SEAL Team (1993), LHX and friends.
//
// Header (7 bytes, little endian):
//   char   szMagic[5]       "EALIB"
//   quint16 nNumberOfFiles  never 0
//
// Directory (nNumberOfFiles + 1 entries of 18 bytes, at offset 7).  The extra
// trailing entry is a sentinel that only carries the end offset of the last
// member:
//   char   szName[13]       NUL padded, byte 12 is always the terminator
//   quint8 nMethod          0/3 stored, 1 LZSS, 4 PKWARE DCL implode
//   quint32 nOffset         absolute file offset of the member data
//
// Member extents are implicit: the next entry's offset minus this one's, which
// is why the sentinel exists.  The first entry's offset always equals
// 7 + (nNumberOfFiles + 1) * 18 - the directory and the payload tile the file.
//
// Compressed members (method 1 and 4) start with a little endian quint32
// uncompressed size, so the codec stream begins four bytes into the extent.
//   method 1: Okumura LZSS, N=4096, F=18, THRESHOLD=3, LSB-first flag bits,
//             ring pre-set to 0x00 (NOT 0x20 - that is what makes it different
//             from HANDLE_METHOD_AMPK_LZSS, which pre-sets spaces).
//   method 4: a plain PKWARE Data Compression Library implode stream, decoded
//             by the shared HANDLE_METHOD_PKWARE_DCL_IMPLODE path.
//   method 2: never produced by any known packer and rejected by the reference
//             extractor; the class refuses the archive rather than guessing.
class XEALIB final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint8 nMethod;
        QString sFileName;
    };

    explicit XEALIB(QIODevice *pDevice = nullptr);
    ~XEALIB() override;

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
    static XBinary::HANDLE_METHOD methodToHandleMethod(quint8 nMethod);
    static QString methodToString(quint8 nMethod);
};

#endif  // XEALIB_H
