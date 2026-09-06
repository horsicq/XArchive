/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XIZPACK_H
#define XIZPACK_H

#include "xarchive.h"

// IzPack (com.izforge.izpack) installer pack file - the "packN" members that
// live inside an IzPack installer JAR.
//
// The container is a Java object-serialization stream (AC ED 00 05) written by
// ObjectOutputStream, but it is NOT general Java serialization: IzPack writes a
// fixed script, so only one narrow path has to be understood.
//
//   AC ED 00 05                     stream magic, version 5
//   77 04 <int32be count>           TC_BLOCKDATA: number of PackFile records
//   then, `count` times:
//     TC_OBJECT ('s') + either a full TC_CLASSDESC ('r') for the first record
//     or TC_REFERENCE ('q' 00 7E 00 00) for every later one, followed by the
//     PackFile field values in declaration order, then the member's bytes as a
//     run of TC_BLOCKDATA ('w' + u8 len) / TC_BLOCKDATALONG ('z' + u32be len)
//     chunks - the ObjectOutputStream's own 1024-byte framing.
//
// The class descriptor is the version oracle: the serialVersionUID together
// with the declared field count selects one of seven PackFile layouts (v1..v7),
// which is exactly how U3's rcb handler (0061a700 / 0061ac80) picks the record
// shape.  Nothing else in the stream states a version.
//
// Records whose offsetInPreviousPack is not -1 hold no bytes in this pack and
// are omitted, as are pack200-compressed JAR members (their payload is a
// Pack200 archive, not the file).  A trailing section after the last record
// (the installer's parsable/executable lists) is reported as an overlay.
class XIzPack final : public XArchive {
    Q_OBJECT

public:
    struct RECORD {
        QString sFileName;      // targetPath, e.g. "$INSTALL_PATH/readme.txt"
        QString sSourcePath;    // v5..v7 only; empty otherwise
        bool bIsDirectory;
        qint64 nUncompressedSize;
        qint64 nMTime;        // Java milliseconds since the epoch, 0 if absent
        qint64 nRecordOffset;  // the record's TC_OBJECT tag
        qint64 nRecordSize;
        qint64 nStreamOffset;  // first block-data tag, or the payload itself
        qint64 nStreamSize;
        HANDLE_METHOD handleMethod;
    };

    explicit XIzPack(QIODevice *pDevice = nullptr);
    ~XIzPack() override;

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
        qint64 nArchiveSize;   // end of the last record
        qint64 nHeaderSize;    // 10: magic + the record-count block
        qint32 nVersion;       // 1..7
        qint32 nDeclaredCount;
        QList<RECORD> listRecords;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString versionToString(qint32 nVersion);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XIZPACK_H
