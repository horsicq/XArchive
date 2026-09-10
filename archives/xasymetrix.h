/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XASYMETRIX_H
#define XASYMETRIX_H

#include "xarchive.h"

// Asymetrix ToolBook Setup disk-set archive (.001 .. .NNN, Windows 3.1 era).
// A set is spread over several floppy volumes.  Only volume 1 carries the
// member directory; every later volume is pure continuation data behind the
// same 0x2C-byte header, which is why per-file tools report those volumes as
// empty archives.  XArchive hands a class a single QIODevice, so this reader
// lists the whole set directory but unpacks only the members whose complete
// block chain lives inside the file it was given - a member that starts on a
// later volume, or that runs off the end of this one, is listed and refused
// rather than written out truncated.
class XAsymetrix final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nDataOffset;         // valid only when nVolume == container volume
        qint64 nStreamSize;         // exact byte length of the block chain, 0 when refused
        qint64 nUncompressedSize;
        quint32 nCRC32;
        quint16 nVolume;            // volume on which this member's data begins
        quint16 nDosDate;
        quint16 nDosTime;
        quint16 nAttributes;
        bool bComplete;             // the whole chain is present in this file
        QString sFileName;
    };

    explicit XAsymetrix(QIODevice *pDevice = nullptr);
    ~XAsymetrix() override;

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
        qint64 nDataOffset;         // first block header of the data area
        qint64 nDirectorySize;      // 0 on a continuation volume
        quint16 nVolume;
        quint16 nDeclaredCount;     // member count of the whole set
        qint32 nCompleteCount;
        QString sSetName;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool measureMemberChain(const CONTEXT &context, qint64 nDataOffset,
                            qint64 nRegionEnd, qint64 nUncompressedSize,
                            qint64 *pnStreamSize, PDSTRUCT *pPdStruct);
    QString memberInfoString(const CONTEXT &context, const MEMBER &member) const;
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XASYMETRIX_H
