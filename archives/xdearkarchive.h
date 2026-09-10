/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 */
#ifndef XDEARKARCHIVE_H
#define XDEARKARCHIVE_H

#include "xarchive.h"

class QFile;
class QTemporaryDir;
class XZip;

// Bounded archive adapter for the project legacy decoder. The decoder writes
// only to a private ZIP; XZip then supplies the normal streaming, CRC,
// output-budget, and transactional-publication behavior.
class XDearkArchive : public XArchive {
    Q_OBJECT

public:
    explicit XDearkArchive(QIODevice *pDevice = nullptr);
    ~XDearkArchive() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    FT getFileType() override;
    MODE getMode() override;
    QString getMIMEString() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct DEARK_UNPACK_CONTEXT {
        QTemporaryDir *pTemporaryDir;
        QFile *pZipFile;
        XZip *pInnerArchive;
        UNPACK_STATE innerState;
        QString sModule;

        DEARK_UNPACK_CONTEXT();
        ~DEARK_UNPACK_CONTEXT();
    };

};

#endif  // XDEARKARCHIVE_H
