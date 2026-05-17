/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
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
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    enum LZIP_TYPE {
        TYPE_UNKNOWN = 0,
        TYPE_LZ
    };

    // Lzip file format (RFC 1952 inspired):
    // Magic: "LZIP" (4 bytes: 0x4C, 0x5A, 0x49, 0x50)
    // Version: 1 byte
    // Dict size code: 1 byte (encodes dictionary size as 1 << (n & 0x1F))
    // Compressed data
    // CRC32: 4 bytes
    // Data size: 8 bytes (uncompressed data size)
    // Member size: 8 bytes (total member size including header and footer)

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

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    MODE getMode() override;
    qint32 getType() override;
    QString typeIdToString(qint32 nType) override;
    QString getFileFormatExt() override;
    FT getFileType() override;
    QString getFileFormatExtsString() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    QString getMIMEString() override;
    ENDIAN getEndian() override;
    OSNAME getOsName() override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    QString structIDToString(quint32 nID) override;
    QString structIDToFtString(quint32 nID) override;
    quint32 ftStringToStructID(const QString &sFtString) override;
    // QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;
    QList<XFHEADER> getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct) override;
    QList<XFRECORD> getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    // Streaming unpacking API
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

    LZIP_HEADER _read_LZIP_HEADER(qint64 nOffset);
    quint32 _getDictionarySize(quint8 nDictSizeCode);

private:
    // Format-specific unpacking context
    struct LZIP_UNPACK_CONTEXT {
        qint64 nHeaderSize;        // Size of LZIP header (6 bytes min)
        qint64 nCompressedSize;    // Size of compressed data
        qint64 nUncompressedSize;  // Size of uncompressed data
        quint32 nCRC32;            // CRC32 of uncompressed data
        quint8 nDictSizeCode;      // Dictionary size code from header
    };
};

#endif  // XLZIP_H
