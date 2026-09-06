/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * MIT License
 */
#ifndef XALZARCHIVE_H
#define XALZARCHIVE_H

#include "xarchive.h"

// Native port of the recovered U3 archive handler; see port provenance.
class XAlzArchive final : public XArchive
{
    Q_OBJECT
public:
    explicit XAlzArchive(QIODevice *pDevice = nullptr);
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
    struct CONTEXT { QList<ARCHIVERECORD> items; };
    static bool parse(QIODevice *device, QList<ARCHIVERECORD> *items, const QMap<UNPACK_PROP, QVariant> &properties, PDSTRUCT *pd);
};

#endif
