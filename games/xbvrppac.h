/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBVRPPAC_H
#define XBVRPPAC_H

#include "xarchive.h"

// BVRP Software "PAC" installation-data container (banner
// "PAC - (c) BVRP Software 1990", binary sub-signature D6 A9 at 0x50).
// The directory is not a table: every 32-byte member header carries the
// ABSOLUTE offset of the next header and is immediately followed by its own
// payload, so the members form a singly-linked chain.  Every member observed
// in the family is compressed with LZHUF/lh1.
class XBvrpPac final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nCRC16;
        quint16 nAttributes;
        quint16 nDosDate;
        quint16 nDosTime;
        quint8 nMethod;
        QString sFileName;
    };

    explicit XBvrpPac(QIODevice *pDevice = nullptr);
    ~XBvrpPac() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN,
                             PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1,
                              PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState,
                    const QMap<UNPACK_PROP, QVariant> &mapProperties,
                    PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nFirstMemberOffset;
        quint16 nVersion;
        quint16 nDeclaredMemberCount;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(quint8 nMethod);
    static HANDLE_METHOD methodToHandleMethod(quint8 nMethod);
    static QDateTime dosDateTime(quint16 nDate, quint16 nTime);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XBVRPPAC_H
