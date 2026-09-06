/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XAIXBFF_H
#define XAIXBFF_H

#include "xarchive.h"

// IBM AIX backup file format (BFF) written by `backup -i` / bffcreate, the
// "backup by name" stream used by installp filesets and PTF packages.  Every
// structural field is little-endian even though the member payloads are
// big-endian AIX/XCOFF32 content; that mismatch is a documented quirk of the
// writer, not a parsing mistake.  Members are either stored verbatim or packed
// with the classic SysV `pack` Huffman codec in its headerless form.
class XAIXBFF final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nMode;
        quint32 nATime;
        quint32 nMTime;
        quint32 nCTime;
        quint16 nMagic;
        bool bIsFolder;
        QString sFileName;
    };

    explicit XAIXBFF(QIODevice *pDevice = nullptr);
    ~XAIXBFF() override;

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
        qint64 nTerminatorOffset;
        qint64 nFirstMemberOffset;
        quint32 nBackupTime;
        QString sVolumeLabel;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(quint16 nMagic);
    static HANDLE_METHOD methodToHandleMethod(quint16 nMagic);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XAIXBFF_H
