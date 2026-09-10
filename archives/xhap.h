/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XHAP_H
#define XHAP_H

#include "xarchive.h"

// HAP archive (.HAP), the DOS archiver by Harri Hirvola.
//
// Archive header (15 bytes):
//   quint8 nMagic[4]     91 33 48 46
//   quint8 nZero[11]     all zero - the reference extractor checks every one
//
// Then a flat chain of members, each introduced by a 40-byte header:
//   quint8 nMagic[4]     8E 68 4A 57
//   qint32 nCompressed   never negative
//   quint32 nCrc         0 when the producer stored no checksum
//   quint8 nZero[4]      offset 12
//   quint8 nFlag         offset 16, always 0
//   quint8 nAttributes   offset 17, the DOS attribute byte
//   quint16 nTime, nDate offsets 18 and 20
//   qint32 nUncompressed offset 22, never negative
//   char   szName[13]    offset 26, NUL padded; the reference extractor forces
//                        byte 38 to NUL, so the usable name is 12 characters
//   quint8 nMethod       offset 39: 0x15 stored, 0x16 compressed
//
// The member payload follows the header and is nCompressed bytes long; the next
// header sits right behind it, and the chain ends at end of file.
//
// A 0x15 member is a verbatim copy and requires nCompressed == nUncompressed.
// A 0x16 member is HAP's own order-4 PPM stream over a 16-bit arithmetic coder
// (see XHAPDecoder); no other decoder in the tree covers it.
class XHAP final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nCrc;
        quint16 nTime;
        quint16 nDate;
        quint8 nMethod;
        QString sFileName;
    };

    explicit XHAP(QIODevice *pDevice = nullptr);
    ~XHAP() override;

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
    static bool isValidEntryHeader(const uchar *pHeader);
    static QString rawNameToString(const char *pRawName, qint32 nIndex);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
    static XBinary::HANDLE_METHOD methodToHandleMethod(quint8 nMethod);
};

#endif  // XHAP_H
