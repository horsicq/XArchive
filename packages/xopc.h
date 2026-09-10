/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XOPC_H
#define XOPC_H

#include "xarchive.h"

// OS2Point distribution package (.OPC) - the packaging format of the OS/2
// FidoNet point system "OS2Point" 1.2 / 2.0.
//
// The file is an 80-byte fixed ASCII banner followed by an ordinary PKZIP
// archive whose every byte has been shifted by +0x67:
//
//   0x00  "OS2POINT v1.2:" / "OS2POINT v2.0:" then a padded description
//   0x4F  0x1A                                (banner terminator)
//   0x50  the obfuscated ZIP; its first four bytes read B7 B2 6A 6B,
//         which is 'P' 'K' 0x03 0x04 each plus 0x67.
//
// The shift is length preserving and position independent, so the ZIP
// structures are read through a de-obfuscating window and every member's data
// extent maps 1:1 onto a file extent.  Members therefore stay in place and are
// published with HANDLE_METHOD_OPC, which subtracts 0x67 and then applies the
// member's own ZIP method (0 stored / 8 deflate) - the method byte travels in
// FPART_PROP_COMPRESSPROPERTIES.
class XOPC final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDataOffset;  // absolute file offset of the member's packed data
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint16 nZipMethod;
        QString sFileName;
    };

    explicit XOPC(QIODevice *pDevice = nullptr);
    ~XOPC() override;

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
        qint64 nZipSize;
        QString sBanner;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    // Reads nSize bytes at nZipOffset inside the embedded ZIP and undoes the
    // +0x67 shift.  Returns an empty array on any short read.
    QByteArray readDeobfuscated(qint64 nZipOffset, qint64 nSize, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
    static QString sanitizeName(const QByteArray &baRawName);
    static QByteArray methodProperty(quint16 nZipMethod);
    static QString reportedMethod(quint16 nZipMethod);
};

#endif  // XOPC_H
