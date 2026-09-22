/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XJUGGLOR_H
#define XJUGGLOR_H

#include <QSet>

#include "xarchive.h"

// FlashJester Jugglor 2.x wraps zlib-compressed project files in a Delphi
// PE32 overlay. A fixed 220-byte EOF trailer authenticates the member region.
class XJugglor : public XArchive {
    Q_OBJECT

public:
    struct FILE_ENTRY {
        QString sName;
        qint64 nHeaderOffset = 0;
        qint64 nDataOffset = 0;
        qint64 nCompressedSize = 0;
        qint64 nUncompressedSize = 0;
        QDateTime mtDateTime;
    };

    struct UNPACK_CONTEXT {
        QList<FILE_ENTRY> listEntries;
        QIODevice *pSourceDevice = nullptr;
        UNPACK_STATE *pOwnerState = nullptr;
        QByteArray baToken;
        quint64 nDeviceGeneration = 0;
        qint64 nSourceSize = 0;
        qint32 nCurrentIndex = 0;
        qint64 nCurrentOffset = 0;
    };

    explicit XJugglor(QIODevice *pDevice = nullptr, bool bIsImage = false,
                      XADDR nModuleAddress = -1);
    ~XJugglor() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);

    FT getFileType() override;
    MODE getMode() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

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

private:
    bool _parse(QList<FILE_ENTRY> *pEntries, qint64 *pSourceSize,
                QString *pVersion, PDSTRUCT *pPdStruct);
    bool _isContextCurrent(const UNPACK_STATE *pState,
                           const UNPACK_CONTEXT *pContext);

    QSet<UNPACK_CONTEXT *> m_setContexts;
};

#endif  // XJUGGLOR_H
