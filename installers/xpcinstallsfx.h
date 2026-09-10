/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XPCINSTALLSFX_H
#define XPCINSTALLSFX_H

#include <QHash>

#include "xarchive.h"

// PC-Install self-extracting installer: a PE32 or NE setup stub whose overlay
// carries the same "[20/20]" container that XPCInstall reads out of a plain
// DISK1.BND floppy volume.  The two shapes are NOT interchangeable, which is
// why this is a separate reader:
//
//   * A .BND starts its record chain at file offset 16, behind a sixteen-byte
//     volume header.  The self-extracting overlay starts it at
//     <overlay offset> + 8, behind an eight-byte "[20/20]\0" head tag, so
//     XPCInstall::parseContext() rejects every one of these on its
//     "first record offset must be 16" gate.
//   * The trailer's stored first/last record offsets are absolute file
//     offsets and are stale on stubs that were re-linked after the payload was
//     appended, so the head tag - not the trailer - is the authoritative
//     anchor.
//   * A record payload here is either a member GROUP (one 0x3a prologue whose
//     +0x10 word is the member count, then that many 0xa8 info blocks each
//     followed by its PKWARE DCL stream) or a raw staging file the setup
//     engine consumes as-is (SETUP.CFG, the .PIF shortcuts, a spawn helper).
//     XPCInstall handles only the count==1 group and rejects the rest.
//
// The container stores no reliable uncompressed size (roughly half the info
// blocks leave that field zero), so the raw size is discovered by decoding,
// exactly as XPCInstall does.
class XPCInstallSFX final : public XArchive {
    Q_OBJECT

public:
    struct ENTRY {
        qint64 nRecordOffset;   // outer 0x114 record
        qint64 nGroupOffset;    // record payload
        qint64 nGroupSize;
        qint64 nInfoOffset;     // 0xa8 member info block; -1 for a raw record
        qint64 nDataOffset;
        qint64 nDataSize;
        qint64 nDeclaredRawSize;  // 0 when the builder left the field blank
        quint32 nAttributes;
        quint16 nDosDate;
        quint16 nDosTime;
        bool bStored;
        QString sFileName;
        QString sSourceName;
    };

    explicit XPCInstallSFX(QIODevice *pDevice = nullptr);
    ~XPCInstallSFX() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

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
        qint64 nHeadOffset;         // the eight-byte "[20/20]\0" tag
        qint64 nFirstRecordOffset;  // nHeadOffset + 8
        qint64 nTrailerOffset;
        QList<ENTRY> listEntries;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool isHeadTagAt(qint64 nOffset, PDSTRUCT *pPdStruct);
    bool walkRecords(CONTEXT *pContext, qint64 nStartOffset,
                     PDSTRUCT *pPdStruct);
    bool parseGroup(qint64 nGroupOffset, qint64 nGroupSize,
                    qint64 nRecordOffset, const QString &sSourceName,
                    QList<ENTRY> *pListEntries, PDSTRUCT *pPdStruct);
    qint64 resolveUncompressedSize(const ENTRY &entry, PDSTRUCT *pPdStruct);
    void fillEntryProperties(const ENTRY &entry,
                             QMap<FPART_PROP, QVariant> *pMapProperties,
                             PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);

    // Discovering a stream's raw size costs a full decode.  Memoise it per
    // stream offset so a listing followed by an extraction does not decompress
    // the whole overlay again for every call.
    QHash<qint64, qint64> m_mapUncompressedSizes;
};

#endif  // XPCINSTALLSFX_H
