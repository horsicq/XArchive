/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSWAGPACKET_H
#define XSWAGPACKET_H

#include "xarchive.h"

// SWAG snippet packet produced by SWAGOLX.EXE (GDSOFT, 1993), the offline
// packer used to ship the SourceWare Archive Group Pascal collection on BBS
// networks (extension .SWG).  This is NOT the LHA-based ".SWG" packet that
// deark's swg module handles; there is no LHA header anywhere in the file.
//
// Layout is a flat 128-byte block grid with no central directory:
//   file header  (1 block)  banner[48] ' ' count[5] "         " title[65]
//   per snippet             header block[128] + data blocks, all counted by
//                           the header's own block field (header included).
//
// Everything is fixed-width, space-padded ASCII; nothing is compressed.  The
// snippet text uses character 0xE3 as its line separator (the SWAG reader's
// on-screen newline), so the extracted member is the stored bytes with 0xE3
// mapped to CR - a length-preserving translation, which is why the record is
// still published as HANDLE_METHOD_STORE while unpackCurrent() emits the
// converted text.  The text ends at the first 0x1A and the trailing block
// padding (spaces) is dropped.
class XSwagPacket final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nBlockCount;   // includes the member's own header block
        qint64 nRegionSize;   // (nBlockCount - 1) * 128
        qint64 nTextSize;     // after the 0x1A cut and the trailing-space trim
        QString sFileName;
        QString sIndex;
        QString sDate;
        QString sTime;
        QString sAuthor;
        QString sContributor;
        QString sSubject;
        QString sKeyword;
    };

    explicit XSwagPacket(QIODevice *pDevice = nullptr);
    ~XSwagPacket() override;

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
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                       PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nFirstMemberOffset;
        QString sPacketTitle;
        QList<MEMBER> listMembers;
    };

    // bComputeText makes the walk materialize every member's data region so
    // the trimmed text length is known.  isValid() deliberately walks headers
    // only: the block chain is fully self-describing, so a probe never has to
    // read the payload of a multi-megabyte file.
    bool parseContext(CONTEXT *pContext, bool bComputeText,
                      PDSTRUCT *pPdStruct);
    QByteArray decodeMember(const MEMBER &member, PDSTRUCT *pPdStruct);
    static qint64 trimmedTextSize(const QByteArray &baRegion);
    static QDateTime memberDateTime(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSWAGPACKET_H
