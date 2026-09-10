/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XHZL_H
#define XHZL_H

#include "xarchive.h"

// "!HZL" single-file compressor.  The only samples that carry it are the
// Dr.Web SpIDer guard payloads (SPID32.EX0, SPIDEA.HL0, SPIDEF.CN0 ... - the
// last character of the extension is replaced by '0' on disk), so the product
// that wrote it is not identifiable from the bytes alone.
//
// The container is a flat 12-byte header followed by one LZHUF stream:
//
//     +0x00  char[4]  "!HZL"
//     +0x04  i32      size of the plaintext (must be >= 0)
//     +0x08  char[4]  the original file extension INCLUDING its dot, e.g.
//                     ".EXE" / ".HLP" / ".CNT"; byte +0x08 is always '.'
//     +0x0c  ...      LZHUF stream, runs to end of file
//
// There is no name, no CRC, no timestamp and no terminator: the stream is
// decoded until the declared plaintext length is reached.  The published name
// is therefore the container's own base name with the stored extension put
// back on, exactly as the reference implementation does.
//
// Codec: XHZLDecoder (Algos/xhzldecoder.*), N = 8192, F = 60, THRESHOLD = 2,
// N_CHAR = 314, no stop code, ring prefilled with 0x20 and started at 0.  The
// JBF archiver reuses the same stream, which is why HANDLE_METHOD_HZL - not a
// second method - is what XJBF publishes as well.
class XHZL final : public XArchive {
    Q_OBJECT

public:
    struct HEADER {
        qint64 nHeaderSize;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        QString sExtension;  // ".EXE" etc, including the dot
        QString sFileName;
    };

    explicit XHZL(QIODevice *pDevice = nullptr);
    ~XHZL() override;

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
        QList<HEADER> listEntries;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
};

#endif  // XHZL_H
