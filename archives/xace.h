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
#ifndef XACE_H
#define XACE_H

#include "xarchive.h"

class XACE : public XArchive {
    Q_OBJECT
public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    virtual QList<QString> getSearchSignatures() override;
    virtual XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_HEADER,
        STRUCTID_RECORD,
    };

    // ACE compression techniques (TECH.TYPE)
    enum CTYPE {
        CTYPE_STORED = 0,      // Stored (no compression)
        CTYPE_LZ_HUFFMAN = 1,  // LZ77 + Huffman (ACE 1.x)
        CTYPE_BLOCKED = 2,     // ACE 2.x blocked stream, not a delta method
    };

    // ACE head_type values
    enum HEADTYPE {
        HEADTYPE_ARCHIVE = 0,   // Archive header
        HEADTYPE_FILE = 1,      // File header
        HEADTYPE_RECOVERY = 2,  // Recovery record
    };

    // Generic and archive-header HEAD_FLAGS bits.
    static const quint16 FLAG_ADDSIZE = 0x0001;
    static const quint16 FLAG_COMMENT = 0x0002;
    static const quint16 ARCHFLAG_V20FORMAT = 0x0100;
    static const quint16 ARCHFLAG_SFX = 0x0200;
    static const quint16 ARCHFLAG_LIMITSFXJR = 0x0400;
    static const quint16 ARCHFLAG_MULTIVOLUME = 0x0800;
    static const quint16 ARCHFLAG_AV = 0x1000;
    static const quint16 ARCHFLAG_RECOVERY = 0x2000;
    static const quint16 ARCHFLAG_LOCK = 0x4000;
    static const quint16 ARCHFLAG_SOLID = 0x8000;

    // head_flags bits for file header
    static const quint16 FILEFLAG_ADDSIZE = FLAG_ADDSIZE;
    static const quint16 FILEFLAG_COMMENT = FLAG_COMMENT;
    static const quint16 FILEFLAG_SPLIT_BEFORE = 0x1000;
    static const quint16 FILEFLAG_SPLIT_AFTER = 0x2000;
    static const quint16 FILEFLAG_PASSWORD = 0x4000;
    static const quint16 FILEFLAG_SOLID = 0x8000;

    // ACE magic at offset 7 of archive header block: "**ACE**"
    static const quint8 MAGIC[7];
    static const qint32 MAGIC_OFFSET = 7;  // Offset within archive block where magic starts

    explicit XACE(QIODevice *pDevice = nullptr);
    void setFileNameCodePage(const QString &sCodePage);
    QString getFileNameCodePage() const;

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    virtual FT getFileType() override;
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual QString getMIMEString() override;
    virtual QString getVersion() override;
    virtual QString getArch() override;
    virtual MODE getMode() override;
    virtual ENDIAN getEndian() override;

    virtual QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString structIDToString(quint32 nID) override;
    virtual QString structIDToFtString(quint32 nID) override;
    virtual quint32 ftStringToStructID(const QString &sFtString) override;
    virtual QList<XFHEADER> getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct) override;
    virtual QList<XFRECORD> getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc) override;
    // virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct BLOCK_INFO {
        qint64 nOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        quint32 nAddSize;
        quint16 nHeadCRC;
        quint16 nHeadSize;
        quint16 nHeadFlags;
        quint8 nHeadType;

        // Main-header fields.
        quint8 nVersionExtract;
        quint8 nVersionCreated;
        quint8 nHostCreated;
        quint8 nVolumeNumber;
        quint32 nTimeCreated;
        quint16 nMainReserved1;
        quint16 nMainReserved2;
        quint32 nMainReserved;
        quint8 nAVSize;
        qint64 nAVOffset;

        // File-header fields.
        quint32 nPackedSize;
        quint32 nUnpackedSize;
        quint32 nFileTime;
        quint32 nAttributes;
        quint32 nFileCRC;
        quint8 nTechType;
        quint8 nTechQuality;
        quint16 nTechParameter;
        quint16 nReserved;
        quint16 nFileNameSize;
        qint64 nFileNameOffset;
        QString sFileName;

        // Optional compressed main/file comment.
        quint16 nCommentSize;
        qint64 nCommentOffset;

        // ACE 1.x recovery-header fields.
        quint32 nRecoveryRelativeStart;
        quint32 nRecoveryBlockCount;
        quint32 nRecoveryClusterSize;
        quint16 nRecoveryCRC;
    };

    struct UNPACK_CONTEXT {
        QList<BLOCK_INFO> listFileBlocks;
        quint16 nArchiveFlags;
        qint64 nArchiveSize;
        quint8 nVolumeNumber;
    };

    bool _readBlock(qint64 nOffset, BLOCK_INFO *pInfo, PDSTRUCT *pPdStruct = nullptr);
    bool _isRawAce1Main(const BLOCK_INFO &info) const;
    bool _collectBlocks(QList<BLOCK_INFO> *pListBlocks, PDSTRUCT *pPdStruct = nullptr);
    static qint64 _blockEnd(const BLOCK_INFO &info);

    // Returns total header size (common header: 4 bytes + head_size bytes)
    // Returns -1 on error
    qint64 _getBlockHeaderSize(qint64 nOffset);
    // Returns true if offset has a valid file block (head_type == 1)
    bool _isFileBlock(qint64 nOffset);
    // Returns true if offset looks like a valid ACE block
    bool _isValidBlock(qint64 nOffset);
    // Returns compressed size field from a file block (0 if not a file block or no data)
    quint32 _getCompressedSize(qint64 nOffset);
    // Returns filename from a file block
    QString _getFileName(qint64 nOffset);

private:
    INTERNAL_INFO m_internalInfo;
    QString m_sFileNameCodePage;
};

#endif  // XACE_H
