/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSTORK_H
#define XSTORK_H

#include "xarchive.h"

// "Stork" DOS install / disk-set archive (INSTALL.STK, INSTALL.HD!).
//
// The container is headerless: it is a bare chain of member records that tiles
// the file exactly, with no global header, count or trailer.  Every record is
//
//   +0x00 quint8 nNameLength   1..12
//   +0x01 char   szName[12]    NOT NUL terminated, only nNameLength is valid
//   +0x0d qint32 nPackedSize   payload bytes that follow the 18-byte record
//   +0x11 char   cTerminator   always '$'
//
// followed by nPackedSize bytes of payload; the next record starts immediately
// behind them.  The uncompressed size is NOT stored anywhere - the payload is a
// self-terminating stream and the producer simply never wrote the size down.
//
// The payload is a plain PKWARE Data Compression Library implode stream: the
// two bytes that open every member (literal mode 0/1, dictionary bits 4..6) are
// the DCL prelude, which is also what the reference detector tests.  So no new
// codec is introduced - the class walks the stream once at parse time with a
// size-only decoder (measurePayload) to recover the uncompressed length, and
// then hands the member to the shared HANDLE_METHOD_PKWARE_DCL_IMPLODE path
// with that exact length.
//
// Because nothing here is a magic number, detection is anchored the same way
// the reference implementation anchors it: the archive must open with the
// 10-byte member "@ASSOC.SAV" (the Stork runtime's own association table), the
// whole chain must tile the file exactly, and the first payload must actually
// decode.
class XStork final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;  // -1 when the payload did not decode
        QString sFileName;
    };

    explicit XStork(QIODevice *pDevice = nullptr);
    ~XStork() override;

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

    // Size-only walk of one PKWARE DCL implode stream.  Returns the number of
    // bytes the stream would produce, or -1 when it is not a valid stream.  No
    // output buffer is materialised: a match only needs its distance checked
    // against the number of bytes produced so far.
    static qint64 measurePayload(const QByteArray &baPacked);

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString rawNameToString(const char *pRawName, qint32 nLength, qint32 nIndex);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSTORK_H
