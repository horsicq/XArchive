/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XFRONTPAGETHEME_H
#define XFRONTPAGETHEME_H

#include "xarchive.h"

// Microsoft FrontPage theme package (.elm), as produced by the FrontPage
// 98/2000 theme designer.  The whole container is plain text up to the first
// member and every member is STORED - there is no compression anywhere.
//
// Layout:
//   "<version>\n"              e.g. "3.0.2.1330" or "3.0.2.926"
//   "<count>\n"                decimal member count
//   count x "<name>,<size>\n"  the directory; size is the exact byte count
//   then, for every directory entry in order:
//       "<==MS-Theme==>" followed by <size> raw bytes
//
// The separator PRECEDES each member (it is not an infix), so a zero-length
// member - every theme carries one, "<themename>.utf8,0" - makes the marker
// appear twice in a row.  Treating the marker as a separator between blobs
// instead of a prefix of each blob is what breaks on those files.
//
// The last member ends exactly on EOF in all 55 corpus samples, and that is
// what isValid() requires: the sum of the directory sizes plus
// count * 14 marker bytes plus the text header must equal the file size,
// with the marker verified in front of every single member.  Because the
// header has no binary magic, this end-to-end arithmetic IS the signature.
class XFrontPageTheme final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;  // the "<==MS-Theme==>" marker
        qint64 nDataOffset;
        qint64 nSize;
        QString sFileName;
    };

    explicit XFrontPageTheme(QIODevice *pDevice = nullptr);
    ~XFrontPageTheme() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;
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
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN,
                             PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1,
                              PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState,
                    const QMap<UNPACK_PROP, QVariant> &mapProperties,
                    PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nDirectoryOffset;
        qint64 nDirectorySize;
        QString sVersion;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XFRONTPAGETHEME_H
