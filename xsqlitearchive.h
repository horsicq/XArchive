/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * MIT License
 */
#ifndef XSQLITEARCHIVE_H
#define XSQLITEARCHIVE_H

#include "xarchive.h"

// U3 A597 SQLite table-text export, not an SQL dump or database engine.
class XSQLiteArchive final : public XArchive
{
    Q_OBJECT
public:
    explicit XSQLiteArchive(QIODevice *pDevice = nullptr);
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
        QString name;
        QString table;
        QByteArray bytes;
        QString error;
        quint32 root = 0;
    };
    struct CONTEXT { QList<ITEM> items; };
    static bool readSource(QIODevice *pDevice, QByteArray *pBytes, PDSTRUCT *pPdStruct);
    static bool parse(const QByteArray &bytes, QList<ITEM> *pItems, bool render, PDSTRUCT *pPdStruct, const OUTPUT_POLICY *pPolicy = nullptr);
};

#endif
