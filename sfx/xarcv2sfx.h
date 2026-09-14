/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XARCV2SFX_H
#define XARCV2SFX_H

#include "xarchive.h"

// Eschalon Setup ARCV 2.00 SELF-EXTRACTOR: an MZ/NE carrier (SETUP.EXE built
// by the Eschalon Setup authoring tool, Eschalon Development / Robert Salesas)
// followed by a CHAIN of complete ARCV 2.00 containers, the last of which ends
// exactly at end of file.
//
// The chain is what keeps this out of FT_ARCV2.  A built setup carries the
// installer runtime (CTL3DV2.DLL, SETUPMN.EXE, SETUP.STX and the script text)
// in the first container and the product payload in the second; the two are
// written back to back with no index in front of them, so the FT_ARCV2 walk --
// which is entitled to demand that the member walk land exactly on end of
// file -- stops at the first container's end and rejects the whole stream.
// Reading only one of the two would silently drop half the archive, which is
// why this is a chain reader and not an offset parameter on FT_ARCV2.
//
// The member format is byte-identical to FT_ARCV2 and the payload codec is the
// same ARCV LZHUF, so nothing about the members is re-derived here.
//
// The per-member CRC is calculated over the *stored* bytes, so it
// authenticates the container stream and must not be advertised as a checksum
// of the unpacked output.
class XARCV2SFX final : public XArchive {
    Q_OBJECT

public:
    // Some writers run a prefix-XOR filter over every payload.  Nothing in the
    // container records this, but it is uniform per archive, so it is probed
    // once and stored here.  The two scrambled writers differ only in the seed
    // the filter starts from: the Release Edition of the authoring tool uses
    // 0x56, the Trial Edition 0xab.  No sample of this carrier is scrambled,
    // but the writer that produces the carrier is the same one that produces
    // the bare .ARV, so the probe is carried over rather than assumed away.
    enum SCRAMBLE {
        SCRAMBLE_NONE = 0,
        SCRAMBLE_DELTA,
        SCRAMBLE_DELTA_TRIAL
    };

    struct MEMBER {
        qint32 nArchiveIndex;      // which container in the chain
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nDataSize;          // payload bytes present in THIS container
        qint64 nCompressedSize;    // member total; 0 in a split-head block
        qint64 nUncompressedSize;
        quint32 nFlags;
        quint32 nAttributes;
        quint32 nDosDateTime;
        quint32 nFileVersionMS;
        quint32 nFileVersionLS;
        quint32 nPackedCRC32;
        QString sFileName;
    };

    explicit XARCV2SFX(QIODevice *pDevice = nullptr);
    ~XARCV2SFX() override;

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
        qint64 nStubOffset;        // first byte of the first container
        qint64 nFirstMemberOffset;
        qint32 nArchiveCount;
        quint32 nDiskNumber;
        SCRAMBLE scramble;
        bool bScrambleProbed;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bProbeScramble,
                      PDSTRUCT *pPdStruct);
    bool parseChain(CONTEXT *pContext, qint64 nStubOffset,
                    PDSTRUCT *pPdStruct);
    qint64 nextCandidateOffset(qint64 nFrom, PDSTRUCT *pPdStruct);
    bool isExecutableCarrier(PDSTRUCT *pPdStruct);
    SCRAMBLE probeScramble(const QList<MEMBER> &listMembers, bool *pbProbed,
                           PDSTRUCT *pPdStruct);
    bool streamEndsAtDeclaredLength(const MEMBER &member, SCRAMBLE scramble,
                                    PDSTRUCT *pPdStruct);
    // Byte the prefix-XOR filter starts from for a given reading.  Meaningless
    // for SCRAMBLE_NONE, where the filter is not run at all.
    static quint8 scrambleSeed(SCRAMBLE scramble);
    static bool isStored(quint32 nFlags);
    static bool isCompressed(quint32 nFlags);
    static bool isSplitFragment(quint32 nFlags);
    static QString methodToString(quint32 nFlags, SCRAMBLE scramble);
    static HANDLE_METHOD methodToHandleMethod(quint32 nFlags,
                                              SCRAMBLE scramble);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XARCV2SFX_H
