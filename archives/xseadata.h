/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSEADATA_H
#define XSEADATA_H

#include "xarchive.h"

// SEA.DAT asset bundle - a chunked resource container whose records are tagged
// "TE2#".  The producing tool is unidentified; the format is named after the
// only file name it is ever shipped under.
//
// The file opens with a single quint32 magic 0x12213443 (bytes 43 34 21 12) and
// is then a chain of records, each of which is
//
//   +0x00 char   szTag[4]      "TE2#"
//   +0x04 qint32 nNextOffset   file offset of the record that follows
//   +0x08 qint32 nNameLength   1..255
//   +0x0c char   szName[nNameLength]
//   +0x0c+n quint8 always 0
//           payload
//
// There is no size field: the payload runs from just behind the name's NUL up
// to nNextOffset, i.e. its length is nNextOffset - previousNextOffset -
// nNameLength - 13 (13 = tag + the two ints + the NUL).  The chain ends at a
// record whose tag reads as 0 (or at end of file); the offsets must chain
// exactly, which is what makes the container self-checking.
//
// Members are stored verbatim (the payloads are intact PNG / TIFF / IFF-ILBM /
// TGA files), so HANDLE_METHOD_STORE covers the format and no new codec is
// introduced.
class XSeaData final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nDataOffset;
        qint64 nSize;
        QString sFileName;
    };

    explicit XSeaData(QIODevice *pDevice = nullptr);
    ~XSeaData() override;

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
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString rawNameToString(const QByteArray &baRawName, qint32 nIndex);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSEADATA_H
