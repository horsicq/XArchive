/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XMIMEMAIL_H
#define XMIMEMAIL_H

#include "xarchive.h"

// One RFC 822/2045/2046 reader with three identities: an e-mail message
// (FT_EML), a MIME HTML web archive (FT_MHTML: the same envelope with a
// multipart/related body) and a Unix mailbox (FT_MBOX: messages separated by
// "From " lines).  Every leaf body part becomes one member, decoded from
// base64 / quoted-printable / 7bit / 8bit / binary; nested multiparts are
// recursed; mailbox messages become msg<N>/ folders holding message.eml and
// the parts.  Member naming (shared with the corpus generator):
//   Content-Disposition filename (RFC 2231 filename*= / RFC 2047 words decoded),
//   else Content-Type name=, else part<N>.<ext>; base name only; '<>:"/\|?*'
//   and controls become '_'; a repeated name gets the prefix part<N>_.
class XMimeMail final : public XArchive {
    Q_OBJECT

public:
    explicit XMimeMail(QIODevice *pDevice = nullptr, FT fileTypeHint = FT_UNKNOWN);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, FT fileTypeHint = FT_UNKNOWN, PDSTRUCT *pPdStruct = nullptr);
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
        QString sMethod;
        QString sContentType;
    };

    struct HEADER {
        QByteArray baName;   // lower-case
        QByteArray baValue;  // unfolded, trimmed
    };

    struct UNPACK_CONTEXT {
        FT fileType;
        QList<ITEM> listItems;

        UNPACK_CONTEXT() : fileType(FT_UNKNOWN)
        {
        }
    };

    struct WALK {
        QList<ITEM> *pItems;
        QSet<QString> setUsed;
        qint32 nIndex;
        QString sPrefix;
        PDSTRUCT *pPdStruct;

        WALK() : pItems(nullptr), nIndex(0), pPdStruct(nullptr)
        {
        }
    };

    static qint32 lineEnd(const QByteArray &baData, qint32 nPos, qint32 nEnd, qint32 *pnNext);
    static bool parseHeaders(const QByteArray &baData, qint32 nStart, qint32 nEnd, QList<HEADER> *pHeaders, qint32 *pnBodyStart, bool bStrict);
    static QByteArray headerValue(const QList<HEADER> &listHeaders, const char *pszName);
    static QByteArray mediaType(const QByteArray &baHeaderValue);
    static QString parameter(const QByteArray &baHeaderValue, const QByteArray &baName, bool *pbFound);
    static QString decodeCharset(const QByteArray &baBytes, const QByteArray &baCharset);
    static QString decodeEncodedWords(const QString &sText);
    static QByteArray decodeBase64(const QByteArray &baData);
    static QByteArray decodeQuotedPrintable(const QByteArray &baData);
    static QString sanitizeName(const QString &sName);
    static QString extensionFor(const QByteArray &baMediaType);
    static bool walkEntity(const QByteArray &baData, qint32 nStart, qint32 nEnd, qint32 nDepth, WALK *pWalk);
    static FT classify(const QByteArray &baData);
    static bool decode(const QByteArray &baSource, FT fileTypeHint, FT *pFileType, QList<ITEM> *pItems, PDSTRUCT *pPdStruct);
    bool readSource(QByteArray *pData, PDSTRUCT *pPdStruct);

private:
    FT m_fileTypeHint;
};

#endif  // XMIMEMAIL_H
