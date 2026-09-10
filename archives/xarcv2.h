/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XARCV2_H
#define XARCV2_H

#include "xarchive.h"

// Eschalon Setup archive, ARCV version 2.00 -- a chain of "BLCK" member
// records (Eschalon Development / Robert Salesas; Windows 3.x SETUP.ARV and
// the spanned volumes SETUP.A02/.A03/.A04).  This is a different container
// from ARCV 1.10, whose single file descriptor plus "CHNK" segment is handled
// by FT_ARCV; the version word at +4 keeps the two apart.
//
// The per-member CRC is calculated over the *stored* bytes, so it
// authenticates the container stream and must not be advertised as a checksum
// of the unpacked output.
class XARCV2 final : public XArchive {
    Q_OBJECT

public:
    // Some writers run a prefix-XOR filter over every payload.  Nothing in the
    // container records this, but it is uniform per archive, so it is probed
    // once and stored here.  The two scrambled writers differ only in the seed
    // the filter starts from: the Release Edition of the authoring tool uses
    // 0x56, the Trial Edition 0xab (its own SETUP.STX says "OEM=Eschalon Trial
    // Edition", but that is inside the payload, i.e. readable only after the
    // seed is already known).  So the seed has to be probed like the filter
    // itself.
    enum SCRAMBLE {
        SCRAMBLE_NONE = 0,
        SCRAMBLE_DELTA,
        SCRAMBLE_DELTA_TRIAL
    };

    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nDataSize;          // payload bytes present in THIS volume
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

    explicit XARCV2(QIODevice *pDevice = nullptr);
    ~XARCV2() override;

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
        quint32 nVolumeFlags;
        quint32 nDiskNumber;
        SCRAMBLE scramble;
        bool bScrambleProbed;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bProbeScramble,
                      PDSTRUCT *pPdStruct);
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

#endif  // XARCV2_H
