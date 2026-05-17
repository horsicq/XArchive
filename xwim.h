/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef XWIM_H
#define XWIM_H

#include "xarchive.h"

class XWIM : public XArchive {
    Q_OBJECT

public:
    virtual QList<QString> getSearchSignatures() override;
    virtual XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_WIM_HEADER
    };

    struct RESOURCE_INFO {
        quint64 nPackSize;
        quint64 nOffset;
        quint64 nUnpackSize;
        quint8 nFlags;
    };

    struct WIM_HEADER {
        quint32 nHeaderSize;
        quint32 nVersion;
        quint32 nFlags;
        quint32 nChunkSize;
        QByteArray baGuid;
        quint16 nPartNumber;
        quint16 nNumberOfParts;
        quint32 nNumberOfImages;
        quint32 nBootIndex;
        RESOURCE_INFO offsetTableResource;
        RESOURCE_INFO xmlResource;
        RESOURCE_INFO bootMetadataResource;
        RESOURCE_INFO integrityResource;
    };

    enum HEADER_FLAG {
        HEADER_FLAG_COMPRESSION = 1 << 1,
        HEADER_FLAG_READONLY = 1 << 2,
        HEADER_FLAG_SPANNED = 1 << 3,
        HEADER_FLAG_RESOURCE_ONLY = 1 << 4,
        HEADER_FLAG_METADATA_ONLY = 1 << 5,
        HEADER_FLAG_WRITE_IN_PROGRESS = 1 << 6,
        HEADER_FLAG_REPARSE_POINT_FIXUP = 1 << 7,
        HEADER_FLAG_XPRESS = 1 << 17,
        HEADER_FLAG_LZX = 1 << 18,
        HEADER_FLAG_LZMS = 1 << 19,
        HEADER_FLAG_XPRESS2 = 1 << 21
    };

    explicit XWIM(QIODevice *pDevice = nullptr);
    ~XWIM() override;

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    virtual FT getFileType() override;
    virtual QString getMIMEString() override;
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    virtual MODE getMode() override;
    virtual qint32 getType() override;
    virtual ENDIAN getEndian() override;
    virtual OSNAME getOsName() override;
    virtual QString getVersion() override;
    virtual QString getArch() override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;

    virtual QString structIDToString(quint32 nID) override;
    virtual QString structIDToFtString(quint32 nID) override;
    virtual quint32 ftStringToStructID(const QString &sFtString) override;
    // virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    virtual QList<PM_INFO> unpackImplemented() override;
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QList<FPART_PROP> getAvailableFPARTProperties() override;

    WIM_HEADER readWIMHeader(qint64 nOffset = 0);
    RESOURCE_INFO readResourceInfo(qint64 nOffset);
    QString compressionMethodToString();

private:
    static const qint32 WIM_SIGNATURE_SIZE = 8;
    static const qint32 WIM_HEADER_SIZE_OLD = 0x60;
    static const qint32 WIM_HEADER_SIZE_NEW = 0xD0;
    static const qint32 WIM_RESOURCE_SIZE = 0x18;
    static const qint32 WIM_STREAM_INFO_SIZE = 50;
    static const qint32 WIM_HASH_SIZE = 20;
    static const qint32 WIM_DIR_ENTRY_SIZE = 0x66;

    enum RESOURCE_FLAG {
        RESOURCE_FLAG_METADATA = 1 << 1,
        RESOURCE_FLAG_COMPRESSED = 1 << 2,
        RESOURCE_FLAG_SOLID = 1 << 4
    };

    struct STREAM_INFO {
        RESOURCE_INFO resourceInfo;
        quint16 nPartNumber;
        quint32 nRefCount;
        QByteArray baHash;
    };

    struct WIM_RECORD {
        QString sFileName;
        bool bIsFolder;
        qint64 nStreamOffset;
        qint64 nStreamSize;
        qint64 nUncompressedSize;
        HANDLE_METHOD handleMethod;
        QDateTime mtDateTime;
    };

    struct WIM_UNPACK_CONTEXT {
        QList<WIM_RECORD> listRecords;
    };

    bool _isSupportedVersion(quint32 nVersion, quint32 nHeaderSize) const;
    void _appendResourcePart(QList<FPART> *pListResult, quint32 nFileParts, const RESOURCE_INFO &resourceInfo, const QString &sName, qint32 nLimit);
    bool _isResourceStored(const RESOURCE_INFO &resourceInfo) const;
    QByteArray _readStoredResource(const RESOURCE_INFO &resourceInfo, PDSTRUCT *pPdStruct);
    QList<STREAM_INFO> _readStreamInfoList(const WIM_HEADER &header, PDSTRUCT *pPdStruct);
    bool _parseMetadata(const QByteArray &baMetadata, const QList<STREAM_INFO> &listStreams, QList<WIM_RECORD> *pListRecords);
    bool _parseMetadataDir(const QByteArray &baMetadata, qint64 nOffset, const QString &sParent, const QMap<QByteArray, STREAM_INFO> &mapStreams,
                           QList<WIM_RECORD> *pListRecords, QSet<qint64> *pStVisited);
    WIM_RECORD _createRecordFromMetadataItem(const QByteArray &baMetadata, qint64 nOffset, const QString &sParent, const QMap<QByteArray, STREAM_INFO> &mapStreams);
    QString _readUTF16LEString(const QByteArray &baData, qint64 nOffset, qint32 nSize);
    static bool _isEmptyHash(const QByteArray &baHash);
};

#endif  // XWIM_H
