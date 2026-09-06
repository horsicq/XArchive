/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XASCEND_H
#define XASCEND_H

#include <QDateTime>

#include "xarchive.h"

// Ascend for Windows installer compressed file (Franklin Quest Co.,
// 1991-1993).  A single-member container with no magic at all: the first
// twelve bytes are the ORIGINAL file's modification time as six little-endian
// words, and everything from offset 12 to EOF is a stock PKWARE Data
// Compression Library stream (its own two-byte header included).  Neither the
// member name nor its unpacked size is stored, so the size has to be
// discovered by a trial decode and the name is reconstructed from the
// archive's own file name.
class XAscend final : public XArchive {
    Q_OBJECT

public:
    explicit XAscend(QIODevice *pDevice = nullptr);
    ~XAscend() override;

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
        quint8 nLiteralMode;
        quint8 nDictionaryBits;
        QDateTime dtModified;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString memberName(QIODevice *pDevice);
    static QString methodToString(quint8 nLiteralMode, quint8 nDictionaryBits);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XASCEND_H
