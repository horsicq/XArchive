/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XIGF2_H
#define XIGF2_H

#include "xarchive.h"

// SETUP.IGF - the outer container of the "IGF" DOS/Windows installer.  It holds
// the installer's working files (the ~igfXXXX.TMP payload container and the
// .ifd script) under their original absolute DOS paths.
//
// Header (40 bytes, little endian):
//   quint16 nMagic          0x1324
//   quint8  nUnknown[6]
//   qint32  nFileSize       equals the size of the file
//   qint32  nUnknown        greater than zero
//   quint8  nUnknown[16]
//   quint32 nDirectoryOffset
//   quint32 nDirectoryOffsetNot   the ones complement of the field above
//
// Record (56 bytes, little endian), starting at nDirectoryOffset and repeated
// until the terminator:
//   quint16 nMagic          0xECDB
//   quint16 nTag            0xFFFF ends the walk
//   quint8  nUnknown[24]
//   qint32  nRawSize        at +0x1c
//   quint32 nRawCrc         at +0x20
//   qint32  nPackedSize     at +0x24
//   quint32 nPackedCrc      at +0x28
//   quint8  nUnknown[12]
// followed by a NUL terminated name and padding up to the next multiple of
// four *strictly past* the terminator, then nPackedSize bytes of payload.
//
// The payload is LHA static Huffman with a 4 KiB dictionary - literal/length
// alphabet of 510 symbols in a 12 bit table, 19 entry pre-table at 5 bits, 13
// distance codes at 4 bits - i.e. exactly what HANDLE_METHOD_LZH4 decodes.
class XIGF2 final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nRecordOffset;
        qint64 nDataOffset;
        qint64 nPackedSize;
        qint64 nRawSize;
        quint32 nUnixTime;
        QString sFileName;
    };

    explicit XIGF2(QIODevice *pDevice = nullptr);
    ~XIGF2() override;

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
        qint64 nDirectoryOffset;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bHeaderOnly, PDSTRUCT *pPdStruct);
    // pLayoutRaw points at one of the two record layouts defined in the
    // implementation file; it is passed opaquely so the layout table can stay
    // private to that file.
    bool walkDirectory(const CONTEXT *pContext, const void *pLayoutRaw, QList<MEMBER> *pListMembers, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XIGF2_H
