/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XNSIS_H
#define XNSIS_H

#include <QString>

#include "xbinary.h"

class XArchive;

class XNSIS : public XBinary {
    Q_OBJECT

public:
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_HEADER,
        STRUCTID_FILEENTRY,
    };

    struct INTERNAL_INFO : public XBinary::INTERNAL_INFO {
        bool bIsValid;
        qint64 nSignatureOffset;
        QString sSignature;
        QString sVersion;
        bool bIsUnicode;
    };

    // NSIS compression method (matches 7-Zip NMethodType layout)
    enum NMETHOD {
        NMETHOD_COPY = 0,
        NMETHOD_DEFLATE,
        NMETHOD_BZIP2,
        NMETHOD_LZMA
    };

    // NSIS command layout variants (matches 7-Zip ENsisType)
    enum NSISTYPE {
        NSISTYPE_NSIS2 = 0,
        NSISTYPE_NSIS3,
        NSISTYPE_PARK1,
        NSISTYPE_PARK2,
        NSISTYPE_PARK3
    };

    struct NSIS_HEADER {
        quint32 nFlags;
        quint32 nHeaderSize;
        quint32 nArchiveSize;
    };

    // A single extractable file (built from an EW_EXTRACTFILE / EW_WRITEUNINSTALLER instruction)
    struct FILE_ENTRY {
        QString sFileName;            // reduced (relative) name
        QString sPath;                // reduced SetOutPath ($OUTDIR) directory this file is extracted to
        quint32 nPos;                 // position of the file inside the data block
        quint32 nSize;                // uncompressed size (if known)
        bool bSizeDefined;            // whether nSize is known before extraction
        bool bIsCompressed;           // non-solid: this block is compressed
        quint32 nCompressedSize;      // non-solid: size of the compressed block (without the 4-byte header)
        bool bCompressedSizeDefined;  // whether nCompressedSize came from a valid block header
        quint32 nMTimeLow;
        quint32 nMTimeHigh;
        bool bIsEmptyFile;
        bool bIsUninstaller;  // built from EW_WRITEUNINSTALLER (may be a patched stub)
        quint32 nPatchSize;   // nonzero when the uninstaller data is a patch for the installer stub
    };

    // Order file entries by ascending data-block position (std::stable_sort comparator).
    static bool _fileEntryPosLess(const FILE_ENTRY &a, const FILE_ENTRY &b);

    struct UNPACK_CONTEXT {
        QPointer<QIODevice> pOuterSourceDevice;
        quint64 nOwnerDeviceGeneration;
        UNPACK_STATE *pOwnerState = nullptr;
        XArchive *pSourceValidator;
        UNPACK_STATE sourceValidationState;
        // ---- first header ----
        qint64 nFirstHeaderOffset;  // offset of the 0x1C-byte first header
        qint64 nDataStreamOffset;   // nFirstHeaderOffset + 0x1C
        quint32 nFlags;
        quint32 nHeaderSize;  // uncompressed header size
        quint32 nArchiveSize;

        // ---- compression ----
        NMETHOD method;
        XBinary::HANDLE_METHOD compressMethod;  // mirror of method for reporting/tests
        bool bIsSolid;
        bool bFilterFlag;  // 7-Zip-modified NSIS with BCJ filter
        bool bHeaderIsCompressed;
        quint32 nNonSolidStartOffset;

        // ---- parsed header ----
        QByteArray baHeader;      // decompressed header (blocks + strings)
        quint32 nStringsPos;      // string table offset inside baHeader
        quint32 nNumStringChars;  // string table size (in chars)
        bool bIsUnicode;
        bool bIs64Bit;
        qint32 nNsisType;  // NSISTYPE
        bool bIsNsis225;

        // ---- items ----
        QList<FILE_ENTRY> listEntries;

        // ---- solid decoded stream (header + all files) ----
        QByteArray baSolid;
        bool bSolidDecoded;

        // ---- fields kept for reporting / test compatibility ----
        qint64 nDataOffset;  // = nDataStreamOffset
        qint64 nDataSize;    // size of the compressed data region
    };

    struct UNPACK_LIFETIME_STATE {
        UNPACK_LIFETIME_STATE() : bOperationInProgress(false), bOwnerAlive(true)
        {
        }
        ~UNPACK_LIFETIME_STATE();
        bool bOperationInProgress;
        bool bOwnerAlive;
        QSet<UNPACK_CONTEXT *> setContexts;
    };

    static void deleteUnpackContext(UNPACK_CONTEXT *pContext);

    explicit XNSIS(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XNSIS() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    virtual FT getFileType() override;
    virtual bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    virtual void *getInternalInfo(PDSTRUCT *pPdStruct = nullptr) override;
    virtual void setInternalInfo(void *pInternalInfo) override;

    virtual QString structIDToString(quint32 nID) override;
    virtual QString structIDToFtString(quint32 nID) override;
    virtual quint32 ftStringToStructID(const QString &sFtString) override;

    virtual QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

protected:
    bool isDeviceReplacementAllowed() const override
    {
        return m_pUnpackLifetimeState && m_pUnpackLifetimeState->bOwnerAlive && !m_pUnpackLifetimeState->bOperationInProgress &&
               m_pUnpackLifetimeState->setContexts.isEmpty();
    }

private:
    INTERNAL_INFO _getInternalInfo(PDSTRUCT *pPdStruct);
    INTERNAL_INFO m_internalInfo;
    INTERNAL_INFO _analyse(PDSTRUCT *pPdStruct);
    bool _findFirstHeader(qint64 nSignatureOffset, qint64 nTotalSize, UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct);

    // opening / decoding
    bool _open(UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool _detectMethod(UNPACK_CONTEXT *pContext, const quint8 *pSig, qint64 nSigSize);
    bool _decodeHeader(UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool _decodeSolidStream(UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct);

    // low-level decompressors: decode one raw block
    bool _decodeBlock(NMETHOD method, bool bFilterFlag, const quint8 *pSrc, qint64 nSrcSize, qint64 nOutHint, bool bOutHintKnown, qint64 nOutputLimit,
                      QByteArray *pResult, PDSTRUCT *pPdStruct);
    bool _lzmaDecode(const quint8 *pSrc, qint64 nSrcSize, qint64 nOutHint, bool bOutHintKnown, qint64 nOutputLimit, QByteArray *pResult, PDSTRUCT *pPdStruct);
    bool _inflateRaw(const quint8 *pSrc, qint64 nSrcSize, qint64 nOutHint, bool bOutHintKnown, qint64 nOutputLimit, QByteArray *pResult, PDSTRUCT *pPdStruct);
    bool _bzip2Decode(const quint8 *pSrc, qint64 nSrcSize, qint64 nOutHint, bool bOutHintKnown, qint64 nOutputLimit, QByteArray *pResult, PDSTRUCT *pPdStruct);

    // header parse
    bool _parseHeader(UNPACK_CONTEXT *pContext);
    void _detectNsisType(UNPACK_CONTEXT *pContext, quint32 nEntriesOffset, quint32 nEntriesNum);
    bool _readEntries(UNPACK_CONTEXT *pContext, quint32 nEntriesOffset, quint32 nEntriesNum, QStringList *pPrefixes);
    void _sortItems(UNPACK_CONTEXT *pContext);

    // string helpers (operate on pContext->baHeader)
    QString _readStringRaw(const UNPACK_CONTEXT *pContext, quint32 nStrPos) const;
    void _appendVar(QString *pRes, quint32 nIndex, const UNPACK_CONTEXT *pContext) const;
    void _appendShellString(QString *pRes, unsigned nIndex1, unsigned nIndex2, const UNPACK_CONTEXT *pContext) const;
    qint32 _getVarIndex(const UNPACK_CONTEXT *pContext, quint32 nStrPos) const;
    qint32 _getVarIndex(const UNPACK_CONTEXT *pContext, quint32 nStrPos, quint32 *pResOffset) const;
    bool _isAbsolutePathVar(const UNPACK_CONTEXT *pContext, quint32 nStrPos) const;
    bool _isGoodString(const UNPACK_CONTEXT *pContext, quint32 nStrPos) const;

    quint32 _getCmd(const UNPACK_CONTEXT *pContext, quint32 nCmd) const;

    // Reconstruct a patched uninstaller: apply the NSIS patch stream onto the installer's PE stub.
    static bool _uninstallerPatch(const QByteArray &baPatch, QByteArray *pDest);
    QSharedPointer<UNPACK_LIFETIME_STATE> m_pUnpackLifetimeState;
};

#endif  // XNSIS_H
