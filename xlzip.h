/* Copyright (c) 2017-2025 hors<horsicq@gmail.com>
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
#ifndef XLZIP_H
#define XLZIP_H

#include "xarchive.h"

class XLzip : public XArchive {
    Q_OBJECT

public:
    enum LZIP_TYPE {
        TYPE_UNKNOWN = 0,
        TYPE_LZ
    };

    // Lzip file format (RFC 1952 inspired):
    // Magic: "LZIP" (4 bytes: 0x4C, 0x5A, 0x49, 0x50)
    // Version: 1 byte
    // Dict size code: 1 byte (encodes dictionary size as 2^(n+16) - 35)
    // Compressed data
    // CRC32: 4 bytes
    // Size: 8 bytes (uncompressed size mod 2^32)

#pragma pack(push)
#pragma pack(1)
    struct LZIP_HEADER {
        char magic[4];         // "LZIP" (0x4C, 0x5A, 0x49, 0x50)
        quint8 nVersion;       // Format version
        quint8 nDictSizeCode;  // Dictionary size code
    };
#pragma pack(pop)

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_LZIP_HEADER,
        STRUCTID_MEMBER_HEADER,
        STRUCTID_COMPRESSED_DATA,
        STRUCTID_FOOTER
    };

    explicit XLzip(QIODevice *pDevice = nullptr);
    ~XLzip();

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice);
    virtual MODE getMode() override;
    virtual qint32 getType() override;
    virtual QString typeIdToString(qint32 nType) override;
    virtual QString getFileFormatExt() override;
    virtual FT getFileType() override;
    virtual QString getFileFormatExtsString() override;
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    virtual QString getMIMEString() override;
    virtual ENDIAN getEndian() override;
    virtual OSNAME getOsName() override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString structIDToString(quint32 nID) override;
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;
    virtual quint64 getNumberOfRecords(PDSTRUCT *pPdStruct) override;
    virtual QList<RECORD> getRecords(qint32 nLimit, PDSTRUCT *pPdStruct) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;
    virtual qint64 getNumberOfArchiveRecords(PDSTRUCT *pPdStruct) override;

    // Streaming unpacking API
    virtual bool initUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

    LZIP_HEADER _read_LZIP_HEADER(qint64 nOffset);
    quint32 _getDictionarySize(quint8 nDictSizeCode);

private:
    // Format-specific unpacking context
    struct LZIP_UNPACK_CONTEXT {
        qint64 nHeaderSize;        // Size of LZIP header (6 bytes min)
        qint64 nCompressedSize;    // Size of compressed data
        qint64 nUncompressedSize;  // Size of uncompressed data
        quint32 nCRC32;            // CRC32 of uncompressed data
    };
};

#endif  // XLZIP_H
