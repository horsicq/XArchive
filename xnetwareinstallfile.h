/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XNETWAREINSTALLFILE_H
#define XNETWAREINSTALLFILE_H

#include "xarchive.h"

// Novell NetWare installation "packed file" (NWUNPACK) container.
//
// The whole file is a chain of tagged chunks, each one
//
//     u32 nTotalSize     // includes these four bytes
//     u8  nKeyLength     // strlen(key) + 1
//     u8  key[nKeyLength - 1]
//     u8  value[nTotalSize - 5 - (nKeyLength - 1)]
//
// laid end to end from offset 0 to EOF with no padding.  Every sample carries
// "NetWareFileInfo" (a fixed 14-byte signature block), "NetWareFile" (the
// member's directory entry) and "PackedData" (the compressed bytes); optional
// "VeRsIoN=" and "CoPyRiGhT=" chunks may sit in between.
//
// One member per container.  The PackedData codec - format version 0x01,
// method 0x0A, an LZ77+Huffman variant with a ~4K window that NetWare1
// "Packed File" containers use as well - has NOT been reverse engineered, so
// this class lists the member and refuses extraction.  It never publishes a
// partial or raw-copied member.
class XNetWareInstallFile final : public XArchive {
    Q_OBJECT

public:
    explicit XNetWareInstallFile(QIODevice *pDevice = nullptr);
    ~XNetWareInstallFile() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    qint32 getType() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;

    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    // Everything the listing needs, parsed once from the chunk chain.
    struct CONTEXT {
        qint64 nInputSize = 0;
        qint64 nStreamOffset = 0;  // first byte of the compressed stream
        qint64 nStreamSize = 0;
        qint64 nUncompressedSize = 0;
        quint8 nFormatVersion = 0;  // PackedData value[0]
        quint8 nMethod = 0;         // PackedData value[1]
        quint8 nRecordVersion = 0;  // NetWareFile value[0]
        quint8 nRecordSubVersion = 0;
        QString sFileName;
        QString sDigest;
        QString sInfo;
        QDateTime dtStamp;
    };

    struct UNPACK_CONTEXT {
        CONTEXT context;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);

    static bool parseChunkChain(const QByteArray &baSource, CONTEXT *pContext);
    static bool parseFileRecord(const QByteArray &baValue, CONTEXT *pContext);
    static bool isValidMemberName(const QByteArray &baName);
    static QDateTime parseStamp(const QByteArray &baStamp);
};

#endif  // XNETWAREINSTALLFILE_H
