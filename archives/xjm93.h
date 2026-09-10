/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XJM93_H
#define XJM93_H

#include "xarchive.h"

// "JM93" single-file compressor (.CMP).  An in-house tool - the tag is the
// author's initials plus the year - with no vendor string anywhere in the
// container; the only thing it carries besides the payload is the name of the
// file it was made from.
//
// Header, 73 bytes, little endian:
//   +0x00 char[4]   "JM93"
//   +0x04 quint8    always 0 (the terminator of the tag / a format revision)
//   +0x05 char[60]  original file name, NUL padded
//   +0x41 quint32   compressed size, i.e. everything behind the header
//   +0x45 quint32   uncompressed size
//   +0x49           the payload
//
// The payload is not a private codec: it is one complete PKWARE DCL implode
// ("blast") stream, header byte pair and 519 end-of-stream length code
// included, so this class only adds the container walk and reuses
// HANDLE_METHOD_PKWARE_DCL_IMPLODE for the decoding.
class XJM93 final : public XArchive {
    Q_OBJECT

public:
    explicit XJM93(QIODevice *pDevice = nullptr);
    ~XJM93() override;

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
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString rawNameToString(const char *pRawName);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XJM93_H
