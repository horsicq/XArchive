/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XARCV4_H
#define XARCV4_H

#include "xarchive.h"

// ARCV version 4.00 installer data archive (Eschalon Setup / EDI Install
// lineage).  Unrelated to the ARCV 1.10 layout handled by
// XLegacyStoreArchive: v4 has a fixed 0x79C-byte archive header followed by a
// chain of "FILE"/"DATA" chunk pairs terminated by "EOFM".
//
// Members are either stored (method 0) or compressed (method 2).  Method 2 is
// NOT the Yoshizaki LZHUF that ARCV 1.10 and ARCV 2.00 use -- it is a distinct
// adaptive-Huffman + LZ77 scheme with a 3245-symbol alphabet, a 32 KiB window
// and LSB-first bit packing; see XARCV4Decoder (Algos/xarcv4decoder.h) for the
// codec and the tree rules it has to reproduce byte for byte.  A member whose
// payload is only the leading fragment of a split installation volume stays
// HANDLE_METHOD_UNKNOWN whatever its method says: the rest of the stream is
// not in this file.
class XARCV4 final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;      // start of the FILE chunk
        qint64 nHeaderSize;        // FILE chunk + the DATA chunk header
        qint64 nDataOffset;        // first payload byte
        qint64 nCompressedSize;    // DATA record size (authoritative)
        qint64 nUncompressedSize;  // FILE originalSize
        quint64 nMTime;            // Windows FILETIME, both sub-variants
        quint32 nCRC32;            // CRC-32 of the UNCOMPRESSED member
        quint32 nMethod;
        quint32 nFileVersionMS;
        quint32 nFileVersionLS;
        bool bSpanned;  // payload continues on the next installation volume
        QString sGroupName;
        QString sFileName;
    };

    explicit XARCV4(QIODevice *pDevice = nullptr);
    ~XARCV4() override;

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
        qint64 nFirstMemberOffset;
        quint16 nSubVariant;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(quint32 nMethod, bool bSpanned);
    static HANDLE_METHOD methodToHandleMethod(quint32 nMethod, bool bSpanned);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XARCV4_H
