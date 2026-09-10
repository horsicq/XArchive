/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XGLU_H
#define XGLU_H

#include "xarchive.h"

// ".GLU" BBS / FidoNet network distribution container.
//
// HEADERLESS and completely self-describing by structure only: the file is a
// back-to-back chain of members, each of which is
//
//   char  szName[]   the original file name, NUL terminated (no length field)
//   ...              one LZW code stream, terminated by its own END code
//
// and the next member's name starts at the very next BYTE (the code stream is
// drained a whole byte at a time, so the boundary is always byte aligned).
//
// The code stream is Mark Nelson's variable-width LZW as the reference implementation implements it at
// 0x00519f60 - MSB-first bits, 9..15 bit codes, 0x100 = END, 0x101 = widen,
// 0x102 = clear, first assignable code 0x103 - which the tree already carries
// as HANDLE_METHOD_RAW_LZW15V / XRawLzw15vDecoder.  No new codec.
//
// The reference implementation "detects" this format with a hard-coded whitelist of six literal 16-byte
// file prefixes taken from its own samples, which generalises to nothing.
// XGLU instead requires the WHOLE file to tile: a plausible name, then a code
// stream that parses under the strict LZW grammar, repeated until the last
// stream's END code lands exactly on the last byte.
class XGLU : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nNameOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        QString sFileName;
    };

    explicit XGLU(QIODevice *pDevice = nullptr);
    ~XGLU() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

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
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);

    // Cache: parsing means decoding the whole container, and the listing path
    // calls parseContext repeatedly.
    bool m_bContextCached;
    bool m_bContextValid;
    CONTEXT m_context;
    QIODevice *m_pCachedDevice;
    qint64 m_nCachedSize;
    QByteArray m_baCachedPrefix;
};

#endif  // XGLU_H
