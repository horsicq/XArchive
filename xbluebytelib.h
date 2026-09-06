/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBLUEBYTELIB_H
#define XBLUEBYTELIB_H

#include "xarchive.h"

// Blue Byte engine resource library (Battle Isle / Battle Isle 2 "The Legacy of
// the Titan" / History Line era .LIB/.DAT/.AFX/.SFX/.CGA/.EGA/.TDY/.PMP asset
// containers).  There is no magic: offset 0 is a variable little-endian offset
// of a directory that lives at the END of the file, so the class is recognised
// structurally, not by signature.  Every member is stored verbatim; the
// four-character tags at member starts (TPWM, INFO, ILBM, BBHDF, 3DOB, ADLX,
// ...) are inner payload formats and must be left compressed here.
class XBlueByteLib final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nDataOffset;
        qint64 nSize;
        QByteArray baRawName;
        QString sFileName;
    };

    explicit XBlueByteLib(QIODevice *pDevice = nullptr);
    ~XBlueByteLib() override;

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
        qint64 nDirectoryOffset;
        qint64 nDataOffset;
        qint32 nNumberOfEntries;
        bool bHasPreHeader;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString rawNameToString(const QByteArray &baRawName,
                                   qint32 nIndex);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XBLUEBYTELIB_H
