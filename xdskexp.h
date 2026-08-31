/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XDSKEXP_H
#define XDSKEXP_H

#include "xarchive.h"

// Disk eXPress stores a partial or complete floppy image as independently
// compressed tracks after a 512-byte AS header. Executable packages place the
// header immediately after the MZ-declared DOS image and a four-byte stub CRC.
class XDskExp final : public XArchive {
    Q_OBJECT

public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_DSKEXP_HEADER
    };

#pragma pack(push)
#pragma pack(1)
    struct DSKEXP_HEADER {
        char signature[2];
        quint8 majorVersion;
        quint8 minorVersion;
        quint8 release;
        quint8 diskType;
        quint32 dataCRC;
        quint8 compressionMethod;
        quint8 lastCylinder;
        quint8 lastHead;
        quint8 unknown;
        quint8 flags;
        char reserved[0x121];
        quint32 headerCRC;
        char description[200];
        quint32 descriptionCRC;
    };
#pragma pack(pop)

    explicit XDskExp(QIODevice *pDevice = nullptr);
    ~XDskExp() override;

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    FT getFileType() override;
    MODE getMode() override;
    QString getMIMEString() override;
    qint32 getType() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    bool isSigned() override;
    OSNAME getOsName() override;
    QString getOsVersion() override;
    QString getVersion() override;
    bool isEncrypted() override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN,
                             PDSTRUCT *pPdStruct = nullptr) override;

    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

    QString structIDToString(quint32 nID) override;
    QString structIDToFtString(quint32 nID) override;
    quint32 ftStringToStructID(const QString &sFtString) override;
    QList<XFHEADER> getXFHeaders(const XFSTRUCT &xfStruct,
                                 PDSTRUCT *pPdStruct) override;
    QList<XFRECORD> getXFRecords(FT fileType, quint32 nStructID,
                                 const XLOC &xLoc) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1,
                              PDSTRUCT *pPdStruct = nullptr) override;

    DSKEXP_HEADER readHeader(qint64 nOffset);

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

private:
    struct TRACK_RECORD {
        qint64 nOffset;
        qint32 nSize;
    };

    struct CONTEXT {
        bool bExecutable;
        qint64 nStubEnd;
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nInputSize;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        qint32 nTrackSize;
        qint32 nNumberOfTracks;
        qint32 nCylinders;
        qint32 nSectorsPerTrack;
        quint8 nMajorVersion;
        quint8 nMinorVersion;
        quint8 nDiskType;
        quint8 nCompressionMethod;
        quint8 nFlags;
        quint32 nDataCRC;
        QString sDescription;
        QList<TRACK_RECORD> listTracks;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool writeRange(qint64 nOffset, qint64 nSize,
                    DATAPROCESS_STATE *pDirectState,
                    QIODevice *pOutputLifetime,
                    PDSTRUCT *pPdStruct);
    static QString decodeDescription(const QByteArray &baDescription);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);

    INTERNAL_INFO m_internalInfo;
};

#endif  // XDSKEXP_H
