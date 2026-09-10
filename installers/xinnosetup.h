/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XINNOSETUP_H
#define XINNOSETUP_H

#include <QString>
#include <QBuffer>

#include "xbinary.h"

class XArchive;
class QFile;
struct XInnoSetupTestAccess;

class XInnoSetup : public XBinary {
    Q_OBJECT

public:
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_HEADER,
    };
    struct INTERNAL_INFO : public XBinary::INTERNAL_INFO {
        bool bIsValid;
        qint64 nSignatureOffset;
        QString sVersion;
    };

    // Real InnoSetup offset table parsed from rDlPtS magic
    struct OFFSET_TABLE {
        bool bIsValid;
        qint64 nTableOffset;  // Absolute offset of the rDlPtS magic
        quint32 nRevision;
        quint16 nLoaderVersion;  // Internal legacy markers or 2/4/5/6/7 for fixed-pointer tables
        quint64 nTotalSize;
        qint64 nExeOffset;
        quint32 nExeUncompressedSize;
        quint32 nExeChecksum;
        qint64 nHeaderOffset;  // Absolute offset of setup-0.bin
        qint64 nDataOffset;    // Absolute offset of setup-1.bin/data stream
    };

    enum INNO_COMPRESSION {
        INNO_COMPRESSION_UNKNOWN = -1,
        INNO_COMPRESSION_STORE = 0,
        INNO_COMPRESSION_ZLIB,
        INNO_COMPRESSION_BZIP2,
        INNO_COMPRESSION_LZMA1,
        INNO_COMPRESSION_LZMA2,
    };

    enum INNO_CHECKSUM {
        INNO_CHECKSUM_UNKNOWN = 0,
        INNO_CHECKSUM_ADLER32,
        INNO_CHECKSUM_CRC32,
        INNO_CHECKSUM_MD5,
        INNO_CHECKSUM_SHA1,
        INNO_CHECKSUM_SHA256,
    };

    enum INNO_ENCRYPTION {
        INNO_ENCRYPTION_NONE = 0,
        INNO_ENCRYPTION_ARC4_MD5,
        INNO_ENCRYPTION_ARC4_SHA1,
        INNO_ENCRYPTION_XCHACHA20,
    };

    struct INNO_VERSION {
        bool bIsValid;
        bool bUnicode;
        bool bWin16;
        bool bLegacyShortId;
        bool bIsx;
        quint16 nMajor;
        quint16 nMinor;
        quint16 nPatch;
        quint16 nRevision;
    };

    struct HEADER_INFO {
        bool bIsValid;
        qint32 nFileCount;
        qint32 nDataEntryCount;
        qint32 nHeaderEndOffset;
        qint32 nCountCount;
        qint32 anCounts[17];
        INNO_COMPRESSION compression;
        INNO_ENCRYPTION encryption;
        bool bEncryptionUsed;
        QByteArray baPasswordTest;
        QByteArray baPasswordSalt;
        qint32 nKdfIterations;
        QByteArray baEncryptionBaseNonce;
    };

    struct SLICE_SOURCE {
        quint32 nSlice;
        qint64 nDataOffset;
        QFile *pFile;
        XArchive *pValidator;
        UNPACK_STATE validationState;
    };

    // Data entry from Block Stream 2 (data locations)
    struct DATA_ENTRY {
        quint32 nFirstSlice;
        quint32 nLastSlice;
        qint64 nChunkStartOffset;  // Offset of the zlb chunk in the data stream
        qint64 nChunkSubOffset;    // Offset within the decompressed solid chunk
        qint64 nOriginalSize;
        qint64 nChunkCompressedSize;
        QByteArray baChecksum;
        INNO_CHECKSUM checksumType;
        INNO_COMPRESSION compression;
        quint64 nFileTime;
        quint32 nFileVersionMS;
        quint32 nFileVersionLS;
        quint16 nFlags;
        bool bCallInstructionOptimized;
        bool bChunkEncrypted;
        bool bChunkCompressed;
    };

    // File entry from Block Stream 1 (file metadata)
    struct FILE_ENTRY {
        QString sDestName;      // Destination path (e.g., {app}\file.txt)
        qint32 nLocationEntry;  // Index into DATA_ENTRY array (-1 if none)
    };

    // Decompressed chunk cache for solid compression
    struct CHUNK_CACHE {
        quint32 nFirstSlice;
        quint32 nLastSlice;
        qint64 nChunkOffset;  // Slice-relative or embedded absolute offset
        qint64 nChunkCompressedSize;
        INNO_COMPRESSION compression;
        bool bEncrypted;
        QByteArray baDecompressedData;  // Full decompressed chunk content
        UNPACK_MEMORY_RESERVATION memoryReservation;
    };

    struct UNPACK_CONTEXT {
        QPointer<QIODevice> pOuterSourceDevice;
        quint64 nOwnerDeviceGeneration;
        UNPACK_STATE *pOwnerState = nullptr;
        XArchive *pSourceValidator;
        UNPACK_STATE sourceValidationState;
        QList<ARCHIVERECORD> listAllRecords;
        bool bIsRealFormat;  // true = real InnoSetup, false = synthetic ISDF
        INNO_VERSION version;
        qint64 nSignatureOffset;
        qint64 nDataStreamOffset;  // Absolute offset of data overlay (zlb chunks)
        QList<DATA_ENTRY> listDataEntries;
        QList<FILE_ENTRY> listFileEntries;
        QList<qint32> listRecordDataEntryIndexes;
        QMap<quint32, SLICE_SOURCE *> mapSliceSources;
        INNO_ENCRYPTION encryption;
        quint8 nEncryptionUse;
        QByteArray baEncryptionKey;
        QByteArray baEncryptionBaseNonce;
        QString sPassword;
        quint32 nAnsiCodePageOverride;
        quint32 nAnsiCodePage;
        bool bHasPasswordBytes;
        QByteArray baPasswordBytes;
        CHUNK_CACHE chunkCache;  // Cached decompressed chunk
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

    explicit XInnoSetup(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1);
    ~XInnoSetup() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    virtual FT getFileType() override;
    virtual bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    virtual void *getInternalInfo(PDSTRUCT *pPdStruct = nullptr) override;
    virtual void setInternalInfo(void *pInternalInfo) override;

    virtual QString structIDToString(quint32 nID) override;
    virtual QString structIDToFtString(quint32 nID) override;
    virtual quint32 ftStringToStructID(const QString &sFtString) override;

    // Streaming unpacking API
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
    friend struct XInnoSetupTestAccess;

    INTERNAL_INFO _getInternalInfo(PDSTRUCT *pPdStruct);
    INTERNAL_INFO m_internalInfo;
    INTERNAL_INFO _analyse(PDSTRUCT *pPdStruct);
    QList<ARCHIVERECORD> _parseSyntheticFileEntries(qint64 nSignatureOffset, PDSTRUCT *pPdStruct);

    // Real InnoSetup format parsing
    OFFSET_TABLE _findOffsetTable(PDSTRUCT *pPdStruct);
    OFFSET_TABLE _decodeLegacyOffsetTable(qint64 nTableOffset, const QByteArray &baExpectedId,
                                          quint16 nLoaderVersion, qint64 nFileSize,
                                          PDSTRUCT *pPdStruct);
    QByteArray _readLegacyBlock(qint64 nOffset, qint64 nLimit, qint64 *pnConsumed, PDSTRUCT *pPdStruct);
    bool _readNextLegacySetupBlock(qint64 *pnCursor, qint64 nSetup0End,
                                   QByteArray *pBlock, PDSTRUCT *pPdStruct);
    QByteArray _readBlockStream(qint64 nOffset, qint64 *pnConsumed, PDSTRUCT *pPdStruct, bool b64BitStoredSize, const QByteArray &baCryptKey = QByteArray(),
                                const QByteArray &baCryptNonce = QByteArray(), const INNO_VERSION *pVersion = nullptr);
    QByteArray _stripCRCChunks(const QByteArray &baData, bool *pbValid);
    QByteArray _decompressZlib(const QByteArray &baData, qint64 nExpectedSize = -1);
    QByteArray _decompressLZMA1(const QByteArray &baData);
    QList<DATA_ENTRY> _parseDataEntries(const QByteArray &baBlock2, const INNO_VERSION &version, bool bRev2, qint32 nExpectedCount, INNO_COMPRESSION headerCompression);
    // bUnicode selects UTF-16 WideString (Inno >= 5.3.0 Unicode builds, all 6.x) vs single-byte
    // AnsiString (Inno < 5.3.0, e.g. 5.1.x) parsing of the setup-0 file-entry array.
    QList<FILE_ENTRY> _parseFileEntries(const QByteArray &baBlock1, const HEADER_INFO &headerInfo, const INNO_VERSION &version, bool bRev2 = false,
                                        quint32 nAnsiCodePageOverride = 0, quint32 *pnAnsiCodePage = nullptr);
    QList<FILE_ENTRY> _parseFileEntriesAnsi(const QByteArray &baBlock1, const HEADER_INFO &headerInfo, const INNO_VERSION &version, quint32 nAnsiCodePageOverride = 0,
                                            quint32 *pnAnsiCodePage = nullptr);
    bool _parseLegacySetup0(UNPACK_CONTEXT *pContext, const OFFSET_TABLE &offsetTable, const INNO_VERSION &version, qint32 nVersionIdSize, PDSTRUCT *pPdStruct);
    bool _parseISXDataFallback(UNPACK_CONTEXT *pContext, const OFFSET_TABLE &offsetTable, const INNO_VERSION &version, PDSTRUCT *pPdStruct);
    bool _probeZlibMember(qint64 nOffset, qint64 nLimit, qint64 *pnCompressedSize, qint64 *pnOriginalSize, quint32 *pnAdler32, PDSTRUCT *pPdStruct);
    bool _finalizeRealRecords(UNPACK_CONTEXT *pContext, const QList<DATA_ENTRY> &listDataEntries, const QList<FILE_ENTRY> &listFileEntries,
                              qint64 nDataStreamOffset, const INNO_VERSION &version, PDSTRUCT *pPdStruct);
    bool _parseRealInnoSetup(UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool _prepareSliceSources(UNPACK_CONTEXT *pContext, const QList<DATA_ENTRY> &listDataEntries, PDSTRUCT *pPdStruct);
    bool _areSliceSourcesCurrent(const UNPACK_CONTEXT *pContext, quint32 nFirstSlice, quint32 nLastSlice, PDSTRUCT *pPdStruct) const;
    bool _readDataChunk(UNPACK_CONTEXT *pContext, const DATA_ENTRY &entry, QByteArray *pCompressedData, PDSTRUCT *pPdStruct);
    QByteArray _decompressDataChunk(UNPACK_CONTEXT *pContext, const DATA_ENTRY &entry, qint64 nOutputLimit, const QMap<UNPACK_PROP, QVariant> &mapProperties,
                                    PDSTRUCT *pPdStruct);
    static INNO_VERSION _parseVersionId(const QByteArray &baVersionId);
    static HEADER_INFO _parseHeaderInfo(const QByteArray &baBlock1, const INNO_VERSION &version);
    static QString _readWideString(const QByteArray &baData, qint32 nOffset, qint32 *pnNewOffset);
    static QString _readAnsiString(const QByteArray &baData, qint32 nOffset, qint32 *pnNewOffset);
    static QByteArray _readAnsiBytes(const QByteArray &baData, qint32 nOffset, qint32 *pnNewOffset);
    static QString _readSetupString(const QByteArray &baData, qint32 nOffset, qint32 *pnNewOffset, bool bUnicode);
    static QString _decodeWindows1252(const char *pData, qint32 nSize);
    static bool _decodeAnsiCodePage(const QByteArray &baData, quint32 nCodePage, QString *psResult);
    static QString _getOptionalDestinationPath(const QString &sDestination);
    static void _setDestinationProperties(ARCHIVERECORD *pRecord, const QString &sDestination);
    static bool _deriveEncryptionKey(const QString &sPassword, const QByteArray &baSalt, qint32 nIterations, QByteArray *pKey, PDSTRUCT *pPdStruct);
    static QByteArray _encodeLegacyPassword(const QString &sPassword, bool bUnicode, quint32 nAnsiCodePage = 1252, bool *pbOk = nullptr);
    static bool _arcFourCrypt(QByteArray *pData, const QByteArray &baPassword, const QByteArray &baSalt, INNO_ENCRYPTION encryption);
    static bool _xChaCha20Crypt(QByteArray *pData, const QByteArray &baKey, const QByteArray &baNonce);
    QSharedPointer<UNPACK_LIFETIME_STATE> m_pUnpackLifetimeState;
};

#endif  // XINNOSETUP_H
