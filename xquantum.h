/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XQUANTUM_H
#define XQUANTUM_H

#include "xarchive.h"

// Quantum archive (.PAK / .001) - David Stafford's Q.EXE, the compressor
// Microsoft later licensed as CAB compression type 2.  Borland shipped it as
// the volume format of several installer sets (OWL, PVCS, WATCOM .001 volumes).
//
// Header (8 bytes, little endian):
//   char   szMagic[2]     "DS"
//   quint8 nZero          always 0
//   quint8 nVersion       never 0; < 0x17 selects the OLD stream shape
//   quint16 nNumberOfFiles never 0
//   quint8 nWindowBits    10..21, the LZ window order
//   quint8 nLevel         packer effort, does not affect decoding
//
// Directory, immediately after the header, one record per file:
//   varlen nNameSize      1 byte, or 2 when bit 7 is set: ((b & 0x7f) << 8) | b2
//   char   szName[nNameSize]
//   varlen nExtraSize     a second string (packer version tag); skipped
//   char   szExtra[nExtraSize]
//   quint32 nUncompressedSize
//   quint16 nDosTime
//   quint16 nDosDate
//   quint16 nCRC          OLD variant only
//
// Everything after the directory is ONE solid arithmetic-coded stream: models,
// LZ window and coder registers run continuously across the members, so a
// member can only be produced by replaying the members in front of it.  That is
// why every record publishes the whole body as its stream and hands the size
// table over as FPART_PROP_COMPRESSPROPERTIES.
class XQuantum final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        QString sFileName;
        qint64 nUncompressedSize;
        quint16 nDosTime;
        quint16 nDosDate;
        quint16 nCRC;
        bool bHasCRC;
    };

    explicit XQuantum(QIODevice *pDevice = nullptr);
    ~XQuantum() override;

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
        qint64 nStreamOffset;
        qint64 nStreamSize;
        qint32 nWindowBits;
        qint32 nVersion;
        qint32 nLevel;
        bool bOldVariant;
        QList<MEMBER> listMembers;
        QList<qint64> listSizes;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    QByteArray propertiesForMember(const CONTEXT &context, qint32 nIndex);
    static QString rawNameToString(const QByteArray &baName, qint32 nIndex);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XQUANTUM_H
