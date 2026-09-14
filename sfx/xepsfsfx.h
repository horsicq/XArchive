/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XEPSFSFX_H
#define XEPSFSFX_H

#include "xarchive.h"

// Eschalon Setup 3 self-extractor, "EPSF" generation (Eschalon Development
// Inc. / Robert Salesas -- the same lineage as FT_ARCV, FT_ARCV2 and FT_ARCV4).
// NOT Encapsulated PostScript, and unrelated to FT_EPFS_ARCHIVE (East Point
// Software), whose tag is the transposed "EPFS".
//
// The carrier is a 32-bit Delphi PE stub and everything else sits in the PE
// OVERLAY, which begins with an 18-byte header:
//
//     +0x00  char[4]  "EPSF"
//     +0x04  quint16  version, 3 on every known build
//     +0x06  quint32  unpacked size of the installer runtime
//     +0x0A  quint32  packed size of the installer runtime
//     +0x0E  quint32  32-bit sum of the runtime's UNPACKED bytes
//
// and is followed by three consecutive regions:
//
//   1. the installer runtime, SETUPMN.DLL, packed with the ARCV 4.00 method-2
//      codec (XARCV4Decoder) and exactly the declared packed size long;
//   2. a second stream in the same codec carrying the setup script.  Nothing
//      records its unpacked size -- the reference extractor skips it by
//      DECODING it and throwing the output away -- so it cannot be decoded
//      here and is published as a region, never as a member;
//   3. a complete ARCV 4.00 container, present on the carriers that ship a
//      product payload and absent on the ones that only carry the runtime.
//
// Region 3 is byte-for-byte the FT_ARCV4 container, so this reader does not
// re-derive it: it locates the "ARCV" + 0x0400 header behind region 2, runs
// XARCV4 over a SubDevice pinned there and republishes the records it yields
// with their stream offsets shifted into carrier coordinates.  Every ARCV4
// gate, name, timestamp, method and CRC is therefore whatever FT_ARCV4 says,
// and a later fix to that reader reaches this one unchanged.
//
// The runtime's header checksum is a plain 32-bit sum of the unpacked bytes.
// No CRC_TYPE describes that, so it is parsed and used for nothing rather than
// published under a CRC identity that would make XDecompress verify the wrong
// function.
class XEPSFSFX final : public XArchive {
    Q_OBJECT

public:
    explicit XEPSFSFX(QIODevice *pDevice = nullptr);
    ~XEPSFSFX() override;

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
    struct HEADER {
        qint64 nHeaderOffset;  // == the PE overlay offset on every carrier
        quint16 nVersion;
        qint64 nRuntimeSize;          // unpacked SETUPMN.DLL
        qint64 nRuntimePackedSize;    // packed SETUPMN.DLL, authoritative
        quint32 nRuntimeCheckSum;     // 32-bit sum of the unpacked bytes
    };

    struct CONTEXT {
        qint64 nInputSize;
        HEADER header;
        qint64 nRuntimeOffset;  // header end
        qint64 nScriptOffset;   // runtime end
        qint64 nScriptSize;     // up to the payload archive, or to end of file
        qint64 nArchiveOffset;  // -1 when the carrier ships no payload archive
        qint64 nArchiveSize;
        qint64 nTotalSize;
        bool bArchiveRejected;  // an "ARCV" tag was there but would not parse
        QList<ARCHIVERECORD> listRecords;
    };

    bool readHeader(HEADER *pHeader, PDSTRUCT *pPdStruct);
    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool collectArchiveRecords(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static ARCHIVERECORD runtimeRecord(const CONTEXT &context);
    static ARCHIVERECORD recordAt(const CONTEXT &context, qint32 nIndex);
    static qint64 recordOffset(const CONTEXT &context, qint32 nIndex);
    static qint32 recordCount(const CONTEXT &context);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XEPSFSFX_H
