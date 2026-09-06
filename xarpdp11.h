/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XARPDP11_H
#define XARPDP11_H

#include "xarchive.h"

// UNIX V7 / 2BSD PDP-11 "ar" library archive: the pre-"!<arch>" binary variant
// whose global header is the single 16-bit word 0177545.  Microsoft XENIX-86
// kept the same container while filling it with Intel OMF objects, so member
// payloads are not necessarily PDP-11 a.out.
//
// This is deliberately a separate class from X_Ar and not a variant of it: the
// two record layers share nothing.  X_Ar's record is 60 ASCII bytes with a
// 0x60 0x0A terminator and decimal fields; this one is 26 packed binary bytes
// with PDP middle-endian longs and no terminator at all.
class XArPdp11 final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nSize;  // exact payload size; never the even-padded stride
        quint32 nMTime;
        quint16 nMode;
        quint8 nUid;
        quint8 nGid;
        QString sFileName;
    };

    explicit XArPdp11(QIODevice *pDevice = nullptr);
    ~XArPdp11() override;

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
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool isZeroTail(qint64 nOffset, qint64 nSize, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XARPDP11_H
