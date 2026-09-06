/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSW_H
#define XSW_H

#include "xarchive.h"

// SGI IRIX inst(1M) / swmgr software distribution image ("product image").
//
// Header (13 bytes, ASCII, at offset 0):
//   "im001" 'V' DDD 'P' DD    12 bytes, e.g. "im001V630P00"
//   quint8  0                 byte 12 is always zero
// The member stream begins at offset 13.
//
// Member (repeated to EOF):
//   quint16be nNameLength     1..255
//   char      szName[]        an absolute IRIX path without a leading '/',
//                             e.g. "usr/relnotes/shader_eoe/ch1.z"
//   quint8    data[]          the file body, stored verbatim
//
// Member sizes live in the companion ".idb" index, which is a separate file and
// is normally absent, so the boundaries have to be recovered from the image
// itself.  The reference extractor scans for the next plausible name header -
// a 16 bit big endian length in 5..255 whose name starts with one of the eight
// IRIX top level directories and contains no character that is illegal in a
// filename - and derives each member's size from the distance to the next hit
// (or to EOF for the last one).  Nothing in the image is compressed by the
// container: members that hold compress(1) or pack(1) streams keep them, which
// is exactly what the reference extractor writes out.
class XSW final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nSize;
        QString sFileName;
    };

    explicit XSW(QIODevice *pDevice = nullptr);
    ~XSW() override;

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
        QString sVersion;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bHeaderOnly, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSW_H
