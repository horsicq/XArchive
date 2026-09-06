/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSLS_H
#define XSLS_H

#include "xarchive.h"

// "\x1fS/L?SOA_" single-file compressed container, as shipped with the
// WinSense DOS/Windows helpdesk product.  The wrapper is applied in place, so
// the packed files keep the extension of the plaintext they carry (.SL$ for
// the program overlays, and .DAT / .HLP / .GRP for the data files).
//
// The whole header is 13 bytes:
//
//   +0x00  9 bytes  1F 53 2F 4C 3F 53 4F 41 5F   ("\x1fS/L?SOA_")
//   +0x09  i32      uncompressed size (read SIGNED; a negative value is the
//                   only header-level reject U3 performs)
//   +0x0d           LZHUF bit stream to EOF
//
// There is no stored name, no timestamp and no checksum: the container holds
// exactly one member and U3 names it after the archive file itself, which is
// what this class reproduces.
class XSLS final : public XArchive {
    Q_OBJECT

public:
    explicit XSLS(QIODevice *pDevice = nullptr);
    ~XSLS() override;

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
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    QString memberName();
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSLS_H
