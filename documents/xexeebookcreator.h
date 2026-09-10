/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XEXEEBOOKCREATOR_H
#define XEXEEBOOKCREATOR_H

#include "xarchive.h"

// Self-running Windows eBook produced by an MFC eBook compiler (one sample is
// stamped "Created with EBookGenerator").  The payload is the PE overlay: a
// hand-rolled MFC CArchive stream that opens with the class descriptors of
// CUpdateDir and CUpdateElem.
//
// Overlay header (43 bytes, little endian; byte packed, not word aligned):
//   quint16 0x0000, 0x0002        CArchive object/version tags
//   quint16 0xffff, 0x0001        new-class tag + schema of CUpdateDir
//   quint16 10                    class name length
//   char    "CUpdateDir"
//   quint16 nUnknown              not read by the reference extractor
//   quint16 nNumberOfFiles        at offset 22
//   quint16 0xffff, 0x0001        new-class tag + schema of CUpdateElem
//   quint16 11                    class name length
//   char    "CUpdateElem"
//   quint16 1
//
// Then nNumberOfFiles records of:
//   qint32 nCompressedSize        never negative
//   qint32 nUncompressedSize      never negative
//   quint8 data[nCompressedSize]  a complete zlib stream (78 9C / 78 DA)
//
// The container stores no member names at all; the reference extractor names
// them by their ordinal, and so does this class.
class XEXEEBookCreator final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        QString sFileName;
    };

    explicit XEXEEBookCreator(QIODevice *pDevice = nullptr);
    ~XEXEEBookCreator() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
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
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;
        qint64 nOverlayOffset;
        qint32 nNumberOfEntries;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XEXEEBOOKCREATOR_H
