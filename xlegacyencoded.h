/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XLEGACYENCODED_H
#define XLEGACYENCODED_H

#include "xarchive.h"

// Checksum-verifying readers for historical printable transport encodings.
// BinHex exposes its Macintosh data/resource forks; xbtoa exposes the decoded
// byte stream.  Decoding is bounded and retained behind XArchive's normal
// source-authentication and transactional-output machinery.
class XLegacyEncoded final : public XArchive
{
    Q_OBJECT

public:
    explicit XLegacyEncoded(QIODevice *pDevice = nullptr,
                            FT fileTypeHint = FT_UNKNOWN);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, FT fileTypeHint = FT_UNKNOWN,
                        PDSTRUCT *pPdStruct = nullptr);
    static FT detectFileType(QIODevice *pDevice,
                             PDSTRUCT *pPdStruct = nullptr);

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
    struct ITEM {
        QString sName;
        QByteArray baData;
        QString sMethod;
        // A CRC-unverified member (a tolerantly-identified truncated or
        // damaged fork) is listed but never emitted unless the caller
        // explicitly disables CRC enforcement.  nDeclaredSize is the header's
        // declared fork size when it exceeds the recovered bytes.
        bool bVerified = true;
        qint64 nDeclaredSize = 0;
    };

    struct UNPACK_CONTEXT {
        FT fileType = FT_UNKNOWN;
        QList<ITEM> listItems;
    };

    static constexpr qint64 MAX_ENCODED_SIZE = 128LL * 1024LL * 1024LL;
    static constexpr qint64 MAX_DECODED_SIZE = 128LL * 1024LL * 1024LL;

    static quint16 crc16CCITT(const char *pData, qint64 nSize);
    static bool decodeBinHex(const QByteArray &baSource,
                             QList<ITEM> *pItems);
    static bool decodeBtoa(const QByteArray &baSource, const QString &sName,
                           QList<ITEM> *pItems);
    static bool decode(const QByteArray &baSource, FT fileTypeHint,
                       const QString &sName, FT *pFileType,
                       QList<ITEM> *pItems);
    bool readSource(QByteArray *pData, PDSTRUCT *pPdStruct);

private:
    FT m_fileTypeHint;
};

#endif  // XLEGACYENCODED_H
