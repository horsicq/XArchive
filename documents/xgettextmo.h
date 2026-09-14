/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XGETTEXTMO_H
#define XGETTEXTMO_H

#include "xarchive.h"

// GNU gettext message catalog (.mo): the 0x950412de (LE) / 0xde120495 (BE)
// header, the original/translation string tables, msgctxt (0x04 separator),
// msgid_plural (NUL separator) and NUL-separated plural translations.  The
// catalog is published as ONE synthesised `<stem>.po` member whose text
// follows msgunfmt's layout (header entry, then entries in table order,
// C-escaped strings, 79-column wrapping).  Format knowledge only; the parser
// and the writer are our own.
class XGettextMO final : public XArchive {
    Q_OBJECT

public:
    explicit XGettextMO(QIODevice *pDevice = nullptr);

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
    struct ENTRY {
        bool bHasContext;
        QByteArray baContext;
        QByteArray baId;
        bool bHasPlural;
        QByteArray baPlural;
        QList<QByteArray> listStr;

        ENTRY() : bHasContext(false), bHasPlural(false)
        {
        }
    };

    struct CATALOG {
        bool bBigEndian;
        quint32 nRevision;
        QList<ENTRY> listEntries;
        QByteArray baCharset;  // from the header entry's Content-Type

        CATALOG() : bBigEndian(false), nRevision(0)
        {
        }
    };

    struct UNPACK_CONTEXT {
        QString sName;
        QByteArray baText;
        qint32 nEntries;

        UNPACK_CONTEXT() : nEntries(0)
        {
        }
    };

    static bool parseCatalog(const QByteArray &baSource, CATALOG *pCatalog, PDSTRUCT *pPdStruct);
    static QByteArray renderPO(const CATALOG &catalog);
    static void writeField(QByteArray *pOut, const QByteArray &baKeyword, const QByteArray &baValue, bool bUtf8);
    bool readSource(QByteArray *pData, PDSTRUCT *pPdStruct);
    QString memberName();
};

#endif  // XGETTEXTMO_H
