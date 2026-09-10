/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XARTIPACK_H
#define XARTIPACK_H

#include "xarchive.h"

// Artisoft "ARTIPACK" installer container (LANtastic 5.x INSTALL.PAK /
// LANtastic-for-TCP-IP INSTALLT.PAK), header version word 0x0100.
//
// A fixed 32-byte header, one contiguous run of member payloads, and a trailing
// fixed-width central directory that ends exactly at EOF.  Every member is
// compressed with stock LZHUF.C (Okumura/Yoshizaki) - the same algorithm XFU
// already implements as HANDLE_METHOD_LZH1 - and each payload is prefixed by
// LZHUF's own 4-byte original-size dword, which is INCLUDED in the directory's
// compressed size.  The bitstream handed to the decoder is therefore
// dataOffset+4 / compressedSize-4.
class XArtiPack final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;      // directory record (34 bytes)
        qint64 nDataOffset;        // payload, starts with the LZHUF size dword
        qint64 nCompressedSize;    // payload size, dword included
        qint64 nStreamOffset;      // nDataOffset + 4: the LZHUF bitstream
        qint64 nStreamSize;        // nCompressedSize - 4
        qint64 nUncompressedSize;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
    };

    explicit XArtiPack(QIODevice *pDevice = nullptr);
    ~XArtiPack() override;

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
        qint64 nArchiveSize;
        qint64 nDirectoryOffset;
        qint64 nDirectorySize;
        qint64 nFirstMemberOffset;
        quint16 nVersion;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XARTIPACK_H
