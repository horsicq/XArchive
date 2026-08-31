/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XANCIENT_H
#define XANCIENT_H

#include "xarchive.h"

// Native single-stream adapter for the BSD-licensed Ancient decompressor.
// Only formats with strong, format-specific signatures are exposed here;
// Ancient's deliberately heuristic fallback detectors are not used as generic
// archive probes.
class XAncient final : public XArchive
{
    Q_OBJECT

public:
    explicit XAncient(QIODevice *pDevice = nullptr, FT fileTypeHint = FT_UNKNOWN);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, FT fileTypeHint = FT_UNKNOWN,
                        PDSTRUCT *pPdStruct = nullptr);
    static FT detectFileType(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);

    FT getFileType() override;
    MODE getMode() override;
    qint32 getType() override;
    ENDIAN getEndian() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    OSNAME getOsName() override;
    QString getVersion() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState,
                    const QMap<UNPACK_PROP, QVariant> &mapProperties,
                    PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                       PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct STREAM_INFO {
        FT fileType = FT_UNKNOWN;
        qint64 nPackedSize = -1;
        qint64 nRawSize = -1;
        qint64 nImageSize = -1;
        qint64 nImageOffset = 0;
        qint64 nStreamOffset = 0;
        QString sMethod;
    };

    struct UNPACK_CONTEXT {
        STREAM_INFO info;
        QString sFileName;
    };

    static FT candidateFileType(const QByteArray &baHeader);
    static FT fileTypeForMethod(const QString &sMethod);
    static bool describe(const QByteArray &baPacked, FT fileTypeHint,
                         STREAM_INFO *pInfo);
    static QString outputName(QIODevice *pDevice, FT fileType);
    bool readPackedData(QByteArray *pData, PDSTRUCT *pPdStruct);

private:
    FT m_fileTypeHint;
};

#endif  // XANCIENT_H
