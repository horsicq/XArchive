/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XJAM_H
#define XJAM_H

#include "xarchive.h"

#include <QSet>

// "JAM" game resource archive - an uncompressed container with a real
// directory TREE, as shipped with mid-90s NCAA college sports titles.
//
// Layout (little endian):
//   char  szMagic[3]          "JAM"
//   ...                       the root directory node starts at offset 3
//
// A directory node is:
//   qint32 nNumberOfFiles
//   nNumberOfFiles * 23 bytes:  char szName[15]; qint32 nOffset; qint32 nSize
//   qint32 nNumberOfSubdirectories
//   nNumberOfSubdirectories * 19 bytes: char szName[15]; qint32 nNodeOffset
//
// Name fields are NUL padded, every byte before the NUL is > 0x20, and at
// least one NUL is always present.  Member data is stored verbatim at
// nOffset - there are no compression fields at all - and the first member of
// the root node always begins on a 256-byte boundary.
//
// Subdirectory nodes are reached by absolute offset, so the walk carries a
// visited set: the reference reader does the same to keep a malformed archive
// from looping.
class XJAM : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDataOffset;
        qint64 nSize;
        QString sFileName;
    };

    explicit XJAM(QIODevice *pDevice = nullptr);
    ~XJAM() override;

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
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool walkNode(CONTEXT *pContext, qint64 nNodeOffset, const QString &sPrefix, QSet<qint64> *pSetVisited, qint32 nDepth, PDSTRUCT *pPdStruct);
    static QString rawNameToString(const char *pRawName, qint32 nIndex);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XJAM_H
