/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XROMPAQ_H
#define XROMPAQ_H

#include "xarchive.h"

// Compaq ROMPAQ system-BIOS / option-ROM firmware image (the payload of the
// ROMPAQ and SoftPaq flash utilities).  The file has no ASCII magic at all:
// it opens with a 0x48-byte binary header whose only fixed markers are the
// version word 0x0100/0x0101 at offset 0x0A, a strictly alphanumeric 7-byte
// image name at 0x0C terminated by a NUL at 0x13, and three reserved fields
// that are always zero.  The extension of the distributed file is the ROM id
// word at offset 0x08 printed in hex (CPQ15010.B39 carries id 0x0B39), which
// is a strong corroborating signal but cannot be required: the id lives in the
// file and the name does not.
//
// Two body shapes exist.  A single-part image is one PKWARE Data Compression
// Library stream covering the whole ROM; a multi-part image is a chain of
// banks, each carrying a full copy of the 0x48-byte header followed by its own
// independent DCL stream, and the banks concatenate into one ROM image.  Both
// shapes yield exactly one output file, named by the 7-byte header name.
class XRomPaq final : public XArchive {
    Q_OBJECT

public:
    explicit XRomPaq(QIODevice *pDevice = nullptr);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;
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
        qint64 nStreamOffset;   // first byte handed to the decode chain
        qint64 nStreamSize;
        qint64 nUncompressedSize;
        qint32 nHandleMethod;
        quint32 nPartCount;     // 0 for a single-part image
        quint32 nHeaderChecksum;
        quint16 nRomId;
        quint16 nVersion;
        quint8 nMethod;
        QString sName;
        QString sDate;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XROMPAQ_H
