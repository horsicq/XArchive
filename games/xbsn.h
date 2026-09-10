/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBSN_H
#define XBSN_H

#include "xarchive.h"

// PhysTechSoft (PTS Ltd, Moscow) BSA archiver container, ".BSN".  Every
// multi-byte field is BIG-ENDIAN, which is unusual for a mid-1990s DOS format
// and is the single easiest thing to get wrong here.
//
// The archive is SOLID: a member's LZ window carries the preceding members'
// plaintext even though its Huffman tables and bit reader restart at its own
// first payload byte.  The class therefore replays the members it has already
// walked past and hands each record the 32 KiB of history its decoder needs
// (FPART_PROP_COMPRESSPROPERTIES).  It deliberately does NOT set
// FPART_PROP_ISSOLID: that flag routes XDecompress into the 7z/RAR solid-block
// cache, which decodes one shared block and slices sub-streams out of it - a
// model this format does not follow.
class XBSN final : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nAttributes;
        quint32 nDosDateTime;
        quint32 nCRC32;
        QString sFileName;
    };

    explicit XBSN(QIODevice *pDevice = nullptr);
    ~XBSN() override;

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
        quint16 nVersion;
        QList<MEMBER> listMembers;
        // Solid-window replay state.  nDictionaryIndex counts the members
        // already folded into baDictionary; -1 poisons the replay after a
        // failure so a partial dictionary can never be published as if it
        // were the real history.
        qint32 nDictionaryIndex;
        QByteArray baDictionary;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool ensureDictionary(CONTEXT *pContext, qint32 nIndex,
                          PDSTRUCT *pPdStruct);
    static bool isFolderMember(const MEMBER &member);
    static bool isStoredMember(const MEMBER &member);
    static QString methodToString(const MEMBER &member);
    static HANDLE_METHOD methodToHandleMethod(const MEMBER &member);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XBSN_H
