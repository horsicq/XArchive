/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XGOB2_H
#define XGOB2_H

#include "xarchive.h"

// LucasArts GOB resource archive, version 20 - the container used by Jedi
// Knight: Dark Forces II and Mysteries of the Sith (JK1.GOB, Res1hi.gob, ...).
//
// This is NOT the Dark Forces "GOB\n" container that XGob reads: the magic is
// "GOB " (with a trailing space), a version word follows it, and the directory
// sits at the FRONT of the file instead of the end.
//
//   0x00  char    szMagic[4]      "GOB "
//   0x04  quint32 nVersion        0x14 (20)
//   0x08  quint32 nIndexOffset    0x0C - the offset of the entry count
//   0x0C  qint32  nNumberOfFiles
//   0x10  entry[nNumberOfFiles]
//
// One entry is 0x88 bytes:
//   0x00  qint32 nOffset          absolute file offset of the member data
//   0x04  qint32 nSize            member length
//   0x08  char   szName[0x80]     NUL padded, '\' separated relative path
//
// Every member is stored verbatim, so extraction rides HANDLE_METHOD_STORE and
// no codec is involved.
class XGOB2 final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDataOffset;
        qint64 nSize;
        QString sFileName;
    };

    explicit XGOB2(QIODevice *pDevice = nullptr);
    ~XGOB2() override;

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
        qint32 nVersion;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString rawNameToString(const char *pRawName, qint32 nIndex);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XGOB2_H
