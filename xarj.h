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
#ifndef XARJ_H
#define XARJ_H

#include "xarchive.h"

class XARJ : public XArchive {
    Q_OBJECT
public:
    virtual QList<QString> getSearchSignatures() override;
    virtual XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_HEADER,
        STRUCTID_RECORD,
    };

    // ARJ compression methods
    enum CMETHOD {
        CMETHOD_STORED = 0,              // Stored (no compression)
        CMETHOD_COMPRESSED_MOST = 1,     // Compressed most
        CMETHOD_COMPRESSED = 2,          // Compressed
        CMETHOD_COMPRESSED_FASTER = 3,   // Compressed faster
        CMETHOD_COMPRESSED_FASTEST = 4,  // Compressed fastest
        CMETHOD_NO_COMPRESSION_1 = 5,    // No compression (type 5)
        CMETHOD_NO_COMPRESSION_2 = 6,    // No compression (type 6)
    };

    // ARJ file types
    enum FTYPE {
        FTYPE_BINARY = 0,     // Binary file
        FTYPE_7BIT_TEXT = 1,  // 7-bit text file
        FTYPE_CHAPTER = 2,    // Chapter label
        FTYPE_DIRECTORY = 3,  // Directory
        FTYPE_VOLUME = 4,     // Volume label
    };

    static const quint16 MARKER = 0xEA60;        // 0x60 0xEA in file (LE)
    static const qint32 FIXED_HEADER_SIZE = 30;  // Size of fixed part of basic header

    explicit XARJ(QIODevice *pDevice = nullptr);

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

    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString structIDToString(quint32 nID) override;
    virtual QString structIDToFtString(quint32 nID) override;
    virtual quint32 ftStringToStructID(const QString &sFtString) override;
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    static QString cmethodToString(CMETHOD cmethod);

private:
    // Returns the total number of bytes to skip past the entry header
    // (marker + basic_hdr_size_field + basic_header + CRC32 + extended headers)
    // Returns -1 on error
    qint64 _getEntryHeaderSize(qint64 nOffset);
    // Returns true if bytes at nOffset look like a valid ARJ entry header
    bool _isValidEntry(qint64 nOffset);
    // Returns the null-terminated filename from within the basic header data
    // (basic header data begins at nOffset+4; filename is at +30 within that)
    QString _getFileName(qint64 nOffset);
};

#endif  // XARJ_H

