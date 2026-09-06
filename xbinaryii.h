/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBINARYII_H
#define XBINARYII_H

#include "xarchive.h"

// Apple II Binary II (Gary B. Little, 1986) - the .BNY/.BXY/.SDK transfer
// container.  There is no central directory and no codec: the file IS a chain
// of 128-byte member headers, each immediately followed by its stored payload,
// with the next header rounded up to the following 128-byte boundary.  The
// header's "number of files to follow" byte counts down to zero and is the
// only terminator, so it doubles as the structural cross-check of the walk.
class XBinaryII final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        // Bytes actually present on disk.  Not the same as nDeclaredSize:
        // a ProDOS directory member declares an EOF but stores nothing.
        qint64 nStoredSize;
        qint64 nDeclaredSize;
        quint32 nAuxType;
        quint32 nBlockCount;
        quint16 nFileType;
        quint16 nStorageType;
        quint16 nAccess;
        quint16 nNativeFileType;
        quint8 nOSType;
        quint8 nDataFlags;
        quint8 nVersion;
        quint8 nFilesToFollow;
        bool bIsFolder;
        QDateTime dtModified;
        QDateTime dtCreated;
        QString sFileName;
    };

    explicit XBinaryII(QIODevice *pDevice = nullptr);
    ~XBinaryII() override;

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
        quint8 nVersion;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool parseHeader(const QByteArray &baHeader, qint64 nHeaderOffset,
                            MEMBER *pMember);
    static QString describeMember(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XBINARYII_H
