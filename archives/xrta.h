/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XRTA_H
#define XRTA_H

#include "xarchive.h"

// RTA container, the archive the Pocket Soft RTPatch tooling ships its update
// kits in.  The reference implementation gives the bare container and the
// self-extracting carrier the same handler, and so does this reader: XRTA
// always parses from offset 0 of its own device, and the executable carrier is
// reached by XSFX (FT_RTA), which hands it a SubDevice starting at the
// container.  On every carrier of the reach set the container begins at exactly
// the DOS image end, i.e. the MZ overlay offset, so no signature sweep is
// needed to find it.
//
// Layout - a 4-byte magic followed by a flat chain of records and a
// zero-length name acting as the end marker:
//
//   0x0000  char    szMagic[4]      "KJd\0"
//   then, repeated:
//       quint8  nNameLength         0 ends the archive
//       char    szName[nNameLength] member name, NOT NUL terminated
//       quint8  nExtraLength        second declared string
//       char    szExtra[nExtraLength]
//       quint8  nAttributes         DOS attribute byte (0x20 = ARCHIVE)
//       quint16 nDosDate
//       quint16 nDosTime
//       qint32  nUnpackedSize
//       qint32  nPackedSize
//       char    baStream[nPackedSize]
//
// Every member stream is one complete RTPatch adaptive Huffman/LZSS stream -
// the same codec HANDLE_METHOD_RTPATCH already dispatches to - so this reader
// adds a container and no codec.  A stream opens with the 16-bit 0xB59C codec
// magic, an 8-bit raw-literal flag, and the mandatory 0xFF reserved byte; those
// four bytes are exactly the decoder's own invariants and are required of every
// member before the archive is accepted.
//
// THE SECOND STRING.  It is empty in all 35 members of the reach set and the
// reference reads it and throws it away; only the first string reaches the
// record it builds.  Its meaning is therefore NOT established.  A non-empty one
// is accepted when it is printable and published verbatim as the member's info
// property; it never contributes to the file name.
//
// NAMES are taken verbatim from the first string, with '\\' normalized to '/'.
// Nothing is stripped or folded, so two distinct members cannot collapse onto
// one output path; a name the host filesystem cannot represent rejects the
// container instead.
class XRTA final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nPackedSize;
        qint64 nUnpackedSize;
        quint8 nAttributes;
        quint16 nDosDate;
        quint16 nDosTime;
        QString sFileName;
        QString sExtra;
    };

    explicit XRTA(QIODevice *pDevice = nullptr);
    ~XRTA() override;

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
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    QList<FPART_PROP> getAvailableFPARTProperties() override;
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
    static void fillRecordProperties(const MEMBER &member, QMap<FPART_PROP, QVariant> *pMapProperties);
    static HANDLE_METHOD memberHandleMethod(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
    static bool decodeName(const uchar *pData, qint32 nSize, QString *pName);
    static bool decodeExtra(const uchar *pData, qint32 nSize, QString *pExtra);
};

#endif  // XRTA_H
