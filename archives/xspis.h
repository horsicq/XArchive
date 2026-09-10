/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSPIS_H
#define XSPIS_H

#include "xarchive.h"

// SPIS is GP-Install's bounded payload container. It supports a single named
// stream or a directory of independently compressed members. Every member is
// authenticated with the format's 32-bit sum of decompressed bytes before it
// is published to the caller. TCompress permits applications to replace the
// ID, so this reader intentionally recognizes only the default SPIS\x1a form.
class XSPIS final : public XArchive {
    Q_OBJECT

public:
    enum METHOD {
        METHOD_NON = 0,
        METHOD_RLE,
        METHOD_LZH,
        METHOD_CUS,
        METHOD_LH5,
        METHOD_INVALID
    };

    struct MEMBER {
        qint64 nHeaderOffset = 0;
        qint64 nHeaderSize = 0;
        qint64 nDataOffset = 0;
        qint64 nPackedSize = 0;
        qint64 nRawSize = 0;
        quint32 nChecksum = 0;
        // the archive-level flags word, carried per member because it decides
        // both the checksum rule and whether the payload is XOR masked
        quint32 nFlags = 0;
        quint32 nDosDateTime = 0;
        quint16 nAttributes = 0;
        // set when the archive ends before this member's declared payload does;
        // only ever the last member, and never the only one
        bool bTruncated = false;
        METHOD method = METHOD_INVALID;
        QString sName;
    };

    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {
        bool bIsValid = false;
        qint64 nFileSize = 0;
        quint8 nArchiveType = 0;
        quint32 nTotalRawSize = 0;
        QList<MEMBER> listMembers;
    };

    struct SPIS_UNPACK_CONTEXT {
        INTERNAL_INFO info;
    };

    explicit XSPIS(QIODevice *pDevice = nullptr);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct = nullptr) override;
    void setInternalInfo(void *pInternalInfo) override;

    FT getFileType() override;
    MODE getMode() override;
    qint32 getType() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    OSNAME getOsName() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    bool parseInternalInfo(INTERNAL_INFO *pInfo, PDSTRUCT *pPdStruct = nullptr);
    ARCHIVERECORD rawRecord(const MEMBER &member) const;
    static METHOD tagMethod(const QByteArray &baTag);
    static HANDLE_METHOD handleMethod(METHOD method);
    static QString methodName(METHOD method);
    static QString safeMemberName(const QByteArray &baName);
    static QString payloadExtension(const QByteArray &baPrefix);
    static QDateTime dosDateTime(quint32 nValue);

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XSPIS_H
