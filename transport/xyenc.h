/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XYENC_H
#define XYENC_H

#include "xarchive.h"

// yEnc (yEncode 1.2) transport decoder.  Each `=ybegin` ... `=yend` block is
// decoded with the 8-bit `byte - 42` mapping and the `=` escape; multi-part
// posts (`=ypart begin= end=`) are joined into one member when their parts
// arrive in order and cover the declared size.  Trailer `pcrc32=`/`crc32=`
// values are verified whenever present and a mismatch fails closed.
class XYEnc final : public XArchive {
    Q_OBJECT

public:
    explicit XYEnc(QIODevice *pDevice = nullptr);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
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
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct ITEM {
        QString sName;
        QByteArray baData;
        qint32 nParts;
        bool bCrcVerified;  // a pcrc32=/crc32= trailer covered every byte and matched

        ITEM() : nParts(0), bCrcVerified(false)
        {
        }
    };

    struct UNPACK_CONTEXT {
        QList<ITEM> listItems;
    };

    static bool parseAttributes(const QByteArray &baLine, qint32 nPrefix, QMap<QByteArray, QByteArray> *pMap);
    static bool decode(const QByteArray &baSource, QList<ITEM> *pItems, PDSTRUCT *pPdStruct);
    bool readSource(QByteArray *pData, PDSTRUCT *pPdStruct);
};

#endif  // XYENC_H
