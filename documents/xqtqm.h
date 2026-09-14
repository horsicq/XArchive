/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XQTQM_H
#define XQTQM_H

#include "xarchive.h"

// Qt compiled translation (.qm): the 16-byte magic followed by tagged blocks
// (Contexts, Hashes, Messages, NumerusRules, Dependencies, Language).  The
// Messages block is parsed and published as ONE synthesised Qt Linguist
// `.ts` member (context/name, message/source, comment, translation and
// numerusform).  Layout knowledge only; the parser and the writer are our own.
class XQtQM final : public XArchive {
    Q_OBJECT

public:
    explicit XQtQM(QIODevice *pDevice = nullptr);

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
    struct MESSAGE {
        QString sContext;
        QString sSource;
        QString sComment;
        QStringList listTranslations;
    };

    struct CATALOG {
        QString sLanguage;
        QStringList listDependencies;
        QList<MESSAGE> listMessages;  // in Hashes-block order when that block exists, else block order
        qint32 nBlocks;

        CATALOG() : nBlocks(0)
        {
        }
    };

    struct UNPACK_CONTEXT {
        QString sName;
        QByteArray baText;
        qint32 nMessages;
        QString sLanguage;

        UNPACK_CONTEXT() : nMessages(0)
        {
        }
    };

    static bool parseCatalog(const QByteArray &baSource, CATALOG *pCatalog, PDSTRUCT *pPdStruct);
    static bool parseMessages(const QByteArray &baBlock, QList<MESSAGE> *pList, QList<qint32> *pOffsets, PDSTRUCT *pPdStruct);
    static QString normalizeLanguage(const QString &sLanguage);
    static QString languageFromFileName(const QString &sFileName);
    static QString utf16BE(const QByteArray &baData);
    static QString xmlEscape(const QString &sText);
    static QByteArray renderTS(const CATALOG &catalog);
    bool readSource(QByteArray *pData, PDSTRUCT *pPdStruct);
    QString memberName();
};

#endif  // XQTQM_H
