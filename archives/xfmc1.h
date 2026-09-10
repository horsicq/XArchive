/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XFMC1_H
#define XFMC1_H

#include "xarchive.h"

// "Form Master" .CMP archive (FMC1) - a DOS multi-member container.
//
//   char szMagic[4]   "FMC1", once, at offset 0
//
// then a flat chain of members, each a 24-byte record immediately followed by
// its payload:
//
//   char    szName[12]        original name, NUL terminated when shorter than
//                             the field; the bytes after the NUL are stale and
//                             must not be validated
//   quint16 nDosTime
//   quint16 nDosDate
//   quint16 nReserved         0x0020 throughout the reference corpus
//   qint16  nZero             always 0 - the reference detector gates on it
//   qint32  nCompressedSize   payload length, header excluded
//
// The UNCOMPRESSED SIZE IS NOT STORED anywhere: the reference decompressor is
// given an unlimited output budget and simply runs until the payload is spent.
// XFMC1 therefore walks the token stream at parse time to count the output
// bytes, and hands the member to the generic chain with that size.
//
// The payload is Haruhiko Okumura's LZSS - 4 KiB ring, F = 18, THRESHOLD = 2,
// LSB-first flag byte per eight tokens, ring pre-filled with spaces - which the
// tree already carries as HANDLE_METHOD_AMPK_LZSS.  No new codec: the two
// decoders were compared token for token over the whole family and agree byte
// for byte.
class XFMC1 : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nDosDateTime;
        QString sFileName;
    };

    explicit XFMC1(QIODevice *pDevice = nullptr);
    ~XFMC1() override;

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
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString rawNameToString(const char *pRawName, qint32 nIndex);
    static bool isValidRawName(const char *pRawName);
    static qint64 measureLzss(const QByteArray &baPacked);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);

    bool m_bContextCached;
    bool m_bContextValid;
    CONTEXT m_context;
    QIODevice *m_pCachedDevice;
    qint64 m_nCachedSize;
    QByteArray m_baCachedPrefix;
};

#endif  // XFMC1_H
