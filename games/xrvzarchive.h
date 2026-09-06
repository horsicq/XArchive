/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XRVZARCHIVE_H
#define XRVZARCHIVE_H

#include "xarchive.h"

#include <QVector>

// GameCube-only reader for Dolphin RVZ version 1 images.  RVZ has no native
// member directory, so a valid image is exposed as one reconstructed ISO
// stream.  Wii RVZ images deliberately remain unsupported here: recreating
// their encrypted/hash sectors requires a separate Wii partition pipeline.
class XRVZArchive final : public XArchive
{
    Q_OBJECT

public:
    explicit XRVZArchive(QIODevice *pDevice = nullptr);
    ~XRVZArchive() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    FT getFileType() override;
    MODE getMode() override;
    qint32 getType() override;
    ENDIAN getEndian() override;
    QString getArch() override;
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
    struct RAW_DATA_ENTRY {
        qint64 nOffset = 0;
        qint64 nSize = 0;
        qint64 nAlignedOffset = 0;
        qint64 nLogicalSize = 0;
        qint64 nOutputSkip = 0;
        qint64 nOutputSize = 0;
        quint32 nGroupIndex = 0;
        quint32 nGroupCount = 0;
    };

    struct GROUP_ENTRY {
        qint64 nDataOffset = 0;
        quint32 nDataSize = 0;
        quint32 nPackedSize = 0;
        bool bCompressed = false;
    };

    struct CONTEXT {
        QByteArray baDiscHeader;
        QVector<RAW_DATA_ENTRY> listRawData;
        QVector<GROUP_ENTRY> listGroups;
        qint64 nSourceSize = 0;
        qint64 nIsoSize = 0;
        quint32 nChunkSize = 0;
        quint32 nCompression = 0;
        QString sFileName;
        QString sReportedMethod;
    };

    static bool rangeWithin(qint64 nTotalSize, qint64 nOffset,
                            qint64 nSize);
    static bool rangesOverlap(qint64 nOffset1, qint64 nSize1,
                              qint64 nOffset2, qint64 nSize2);
    static bool addChecked(qint64 nLeft, qint64 nRight, qint64 *pnResult);
    static bool readBE64(const uchar *pData, qint64 *pnResult);
    static bool decompressZstd(const QByteArray &baCompressed,
                               qint64 nExpectedSize, QByteArray *pResult,
                               PDSTRUCT *pPdStruct);
    static bool decodeTable(const QByteArray &baCompressed,
                            qint64 nExpectedSize, quint32 nCompression,
                            QByteArray *pResult, PDSTRUCT *pPdStruct);
    static bool decodeRvzPacked(const QByteArray &baPacked,
                                qint64 nExpectedSize,
                                qint64 nDataOffset, QByteArray *pResult,
                                PDSTRUCT *pPdStruct);
    static bool parseContext(QIODevice *pDevice, CONTEXT *pContext,
                             PDSTRUCT *pPdStruct);
    static bool decodeGroup(QIODevice *pDevice, const CONTEXT &context,
                            const GROUP_ENTRY &group,
                            qint64 nExpectedSize, qint64 nDataOffset,
                            QByteArray *pResult, PDSTRUCT *pPdStruct);
    static bool writeStage(QIODevice *pStage, const char *pData,
                           qint64 nSize,
                           const QSharedPointer<OUTPUT_BUDGET> &spBudget,
                           PDSTRUCT *pPdStruct);
    static bool writeZeroes(QIODevice *pStage, qint64 nSize,
                            const QSharedPointer<OUTPUT_BUDGET> &spBudget,
                            PDSTRUCT *pPdStruct);
};

#endif  // XRVZARCHIVE_H
