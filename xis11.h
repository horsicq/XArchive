/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XIS11_H
#define XIS11_H

#include "xarchive.h"

// InstallShield-family single-stream install file container.  These are the
// *.EX$ / *.DL$ / *.HL$ / *.$$$ / *.??$ files shipped on 16-bit InstallShield
// era distribution disks: the original file name is embedded and the payload
// is one headerless Unix-compress LZW stream.
//
// Archive header (13 bytes, little endian):
//   quint32 nMagic     always 0x8C135D65
//   quint32 nFormat    always 0x00010108
//   quint8  nVariant   1 or 2
//   quint32 nReserved  always 0
//
// Member header (12 bytes, unaligned, immediately after the archive header and
// then at every nNextOffset):
//   quint8  nFlags        0x0E in the whole reference corpus
//   qint32  nPackedSize   bytes of LZW payload, must be >= 0
//   qint32  nNextOffset   file offset of the next member header, 0 = last
//   quint16 nReserved
//   quint8  nNameLength   never 0
// followed by nNameLength name bytes and one separator byte, after which the
// LZW stream starts.  The chain must tile the file exactly: the last member
// ends at end of file, every other one ends exactly at its nNextOffset.
//
// Nothing stores the uncompressed size, so it is resolved by decoding the
// member once while listing.
class XIS11 final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nPackedSize;
        qint64 nUncompressedSize;  // -1 while unresolved
        quint8 nFlags;
        QString sFileName;
    };

    explicit XIS11(QIODevice *pDevice = nullptr);
    ~XIS11() override;

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
        quint8 nVariant;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bResolveSizes, PDSTRUCT *pPdStruct);
    bool resolveSize(MEMBER *pMember, PDSTRUCT *pPdStruct);
    static QString rawNameToString(const QByteArray &baRawName, qint32 nIndex);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XIS11_H
