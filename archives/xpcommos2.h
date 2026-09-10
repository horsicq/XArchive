/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XPCOMMOS2_H
#define XPCOMMOS2_H

#include "xarchive.h"

// IBM Personal Communications for OS/2 (PCOMM 4.x) install-diskette packed
// file: one member, the last character of whose 8.3 name the packer replaced
// by '_' (SAMPLEUP.XLS ships as SAMPLEUP.XL_).
//
// The container is completely headerless - no magic, no name, no length, no
// checksum - so the whole file is a byte-oriented LZ77 token stream and
// recognition has to be a trial decode.  The stream is blocked: every block
// but the last produces exactly 16384 plaintext bytes, two 0xE0 bytes close
// the file, and the token walk must consume the input exactly.  Those
// invariants are the detector; see XPCommOS2Decoder for the token layout.
//
// The plaintext size only exists once the stream has been walked, so
// initUnpack() runs the walk (which allocates nothing) and publishes the
// result as FPART_PROP_UNCOMPRESSEDSIZE.  The payload is a genuine byte range
// of this device, so the record is decoded through the shared
// HANDLE_METHOD_PCOMM_OS2 route rather than being materialized here.
class XPCommOS2 final : public XArchive {
    Q_OBJECT

public:
    explicit XPCommOS2(QIODevice *pDevice = nullptr);
    ~XPCommOS2() override;

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
    OSNAME getOsName() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    // Exactly one member per container, and it covers the whole device.
    struct CONTEXT {
        qint64 nInputSize = 0;
        qint64 nUncompressedSize = 0;
        QString sFileName;
    };

    struct UNPACK_CONTEXT {
        CONTEXT context;
    };

    bool probeGate(PDSTRUCT *pPdStruct);
    bool readSource(QByteArray *pbaSource, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    QString sourceMemberName();
};

#endif  // XPCOMMOS2_H
