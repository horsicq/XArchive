/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSQZ_H
#define XSQZ_H

#include "xarchive.h"

// SQZ ("Squeeze It") is a linked sequence of independently stored or
// compressed members.  Methods 0 through 4 are decoded with strict member,
// output-size, checksum, and archive-chain bounds.
class XSQZ : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset = 0;
        qint64 nHeaderSize = 0;
        qint64 nDataOffset = 0;
        qint64 nCompressedSize = 0;
        qint64 nUncompressedSize = 0;
        quint8 nMethod = 0;
        quint8 nAttributes = 0;
        quint32 nCRC32 = 0;
        quint32 nDosDateTime = 0;
        QString sFileName;
    };

    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {
        bool bIsValid = false;
        qint64 nFileSize = 0;
        qint64 nArchiveSize = 0;
        quint8 nVersion = 0;
        QList<MEMBER> listMembers;
    };

    struct SQZ_UNPACK_CONTEXT {
        INTERNAL_INFO info;
    };

    explicit XSQZ(QIODevice *pDevice = nullptr);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    qint32 getType() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    OSNAME getOsName() override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    bool parseInternalInfo(INTERNAL_INFO *pInfo, PDSTRUCT *pPdStruct = nullptr);
    ARCHIVERECORD rawRecord(const MEMBER &member) const;
    static HANDLE_METHOD methodToHandle(quint8 nMethod);
    static QString reportedMethod(quint8 nMethod);
    static QString safeMemberName(const QByteArray &baName);
    static QDateTime dosDateTime(quint32 nValue);
};

#endif  // XSQZ_H
