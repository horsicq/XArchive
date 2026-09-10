/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XRCF_H
#define XRCF_H

#include "xarchive.h"

// "RCF" installer archive (DISK1.RCF and friends, vendor unidentified).
//
// Header (10 bytes):
//   quint8 nLength   3
//   char   szTag[3]  "RCF" with 0xA5 added to every byte -> F7 E8 EB
//   quint8 nLength   3
//   char   szVer[3]  "1.0" verbatim
//   quint16 nZero    0
// The first member's payload starts at offset 10, i.e. the two PKWARE DCL
// header bytes of member 0 sit at offsets 10 and 11 and are also what the
// reference detector range-checks (literal mode 0/1, dictionary 4/5/6).
//
// Directory (at the very end of the file):
//   quint16 nCount   the last two bytes of the file
//   entry[nCount]    17 bytes each, immediately before that count:
//       quint8 nNameLength   1..12
//       char   szName[12]    NUL padded
//       qint32 nPackedSize
//
// Member payloads are stored back to back from offset 10 in directory order,
// each one a complete PKWARE DCL "implode" stream carrying its own two byte
// header.  The uncompressed size is not stored anywhere, so this class measures
// each stream once while parsing and publishes the result as
// FPART_PROP_UNCOMPRESSEDSIZE; extraction then rides the existing
// HANDLE_METHOD_PKWARE_DCL_IMPLODE and no new codec is introduced.
class XRCF final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDataOffset;
        qint64 nPackedSize;
        qint64 nRawSize;
        QString sFileName;
    };

    explicit XRCF(QIODevice *pDevice = nullptr);
    ~XRCF() override;

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
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bHeaderOnly, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XRCF_H
