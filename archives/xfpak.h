/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XFPAK_H
#define XFPAK_H

#include "xarchive.h"

// FoxPro Distribution Kit archives.  Unlike the generic single-device FPAK
// reader, this archive session can join a lead .PAK with its .PA1, .PA2, ...
// siblings when the source is a QFile.  Companion reads never escape this
// session and every repeated FPPF header is revalidated before extraction.
class XFpakArchive : public XArchive
{
    Q_OBJECT

public:
    explicit XFpakArchive(QIODevice *pDevice = nullptr);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

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
    QString getVersion() override;
    QList<QString> getSearchSignatures() override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
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
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct SEGMENT {
        QString sPath;  // Empty means the archive's caller-owned device.
        qint64 nVolumeSize = 0;
        qint64 nHeaderOffset = 0;
        qint64 nHeaderSize = 0;
        qint64 nDataOffset = 0;
        qint64 nDataSize = 0;
        qint64 nPackedSize = 0;
        qint64 nRawSize = 0;
        quint32 nCRC32 = 0;
        quint16 nDosTime = 0;
        quint16 nDosDate = 0;
        // The FPPF compression method and PKZIP general-purpose flags select the
        // Implode profile. They MUST reach the decoder: a stream decoded with the
        // wrong dictionary width desyncs into plausible garbage rather than
        // failing, and only the member CRC-32 catches it.
        quint16 nMethod = 6;
        quint16 nFlags = 0;
        QString sFileName;
        QByteArray baPinnedHeader;
    };

    struct MEMBER {
        QList<SEGMENT> listSegments;
        qint64 nCompressedSize = 0;
        qint64 nPackedSize = 0;
        qint64 nRawSize = 0;
        quint32 nCRC32 = 0;
        quint16 nDosTime = 0;
        quint16 nDosDate = 0;
        quint16 nMethod = 6;
        quint16 nFlags = 0;
        QString sFileName;
        bool bComplete = false;
    };

    struct VOLUME {
        bool bLead = false;
        // the chain stopped before the end of the volume and the segments below
        // are only the part that could still be walked
        bool bTruncated = false;
        quint16 nVersion = 0;
        qint64 nPackedSize = 0;
        qint64 nRawSize = 0;
        qint64 nFileSize = 0;
        // how far the chain actually reached; equals nFileSize unless bTruncated
        qint64 nParsedSize = 0;
        QList<SEGMENT> listSegments;
    };

    struct UNPACK_CONTEXT {
        qint64 nSourceSize = 0;
        qint64 nArchiveSize = 0;
        quint16 nVersion = 0;
        QList<MEMBER> listMembers;
    };

    enum ASSEMBLY_STATUS {
        ASSEMBLY_MALFORMED = 0,
        ASSEMBLY_INCOMPLETE,
        ASSEMBLY_COMPLETE
    };

    // bAllowTruncatedTail keeps the segments walked so far when the chain stops
    // making sense before the end of the volume.  It is set only for the volume
    // the caller opened itself: a continuation volume must still account for
    // every one of its bytes, because a short one silently corrupts the
    // assembly of the split members that follow it.
    bool readVolume(QIODevice *pDevice, const QString &sPath,
                    bool bRequireContinuation, bool bAllowTruncatedTail,
                    VOLUME *pVolume, PDSTRUCT *pPdStruct);
    bool buildContext(UNPACK_CONTEXT *pContext,
                      PDSTRUCT *pPdStruct);
    ASSEMBLY_STATUS assemble(const QList<SEGMENT> &listSegments,
                             qint64 nExpectedPacked, qint64 nExpectedRaw,
                             QList<MEMBER> *pComplete,
                             MEMBER *pPartial) const;
    bool readMemberData(const MEMBER &member, QByteArray *pData,
                        PDSTRUCT *pPdStruct);
    ARCHIVERECORD memberRecord(const MEMBER &member,
                               qint32 nIndex) const;
};

#endif  // XFPAK_H
