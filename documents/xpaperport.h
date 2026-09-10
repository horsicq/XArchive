/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XPAPERPORT_H
#define XPAPERPORT_H

#include "xarchive.h"

// Visioneer PaperPort desktop / document file (.MAX, ~DESKTOP.NNN).
//
// File header: "ViG" then one of 'C' 'D' 'E' 'F', a byte, then 0x1A.  The
// object database root offset is a qint32 at offset 164.
//
// Objects are chained through 32-byte chunk headers:
//   char   szMagic[2]  "VZ"
//   qint32 nSize       payload size, counted from the end of this header
//   ...
//   quint16 nType      at +26: 0x8000 root, 0x4000 item, 0x1000 image
//
// The root payload starts with a quint16 page count repeated at +10, then one
// 12-byte entry per page whose qint32 at +4 points at the item object; the
// item payload carries the image object's offset as a qint32 at +34.
//
// A page is not a stored file - it is an image object whose pixels live in a
// grid of independently coded tiles - so each member is published as the
// assembled page and rendered to a Windows BMP by XPaperPortDecoder, which is
// what the reference extractor writes as "1.bmp", "2.bmp", ...
class XPaperPort final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nObjectOffset;  // image object payload start
        qint64 nObjectSize;    // image object payload size
        qint64 nOutputSize;    // size of the rendered BMP
        qint32 nWidth;
        qint32 nHeight;
        qint32 nBitsPerPixel;
        QString sFileName;
    };

    explicit XPaperPort(QIODevice *pDevice = nullptr);
    ~XPaperPort() override;

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
        quint8 nVariant;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool readChunk(qint64 nOffset, qint64 *pnSize, quint16 *pnType, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XPAPERPORT_H
