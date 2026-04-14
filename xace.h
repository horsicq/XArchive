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
    virtual QList<QString> getSearchSignatures() override;
    virtual XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_HEADER,
        STRUCTID_RECORD,
    };

    // ACE compression types (low nibble of tech_info)
    enum CTYPE {
        CTYPE_STORED = 0,          // Stored (no compression)
        CTYPE_LZ_HUFFMAN = 1,      // LZ77 + Huffman (ACE 1.x)
        CTYPE_LZ_HUFFMAN_DELTA = 2, // LZ77 + Huffman + Delta (ACE 2.x)
    };

    // ACE head_type values
    enum HEADTYPE {
        HEADTYPE_ARCHIVE = 0,   // Archive header
        HEADTYPE_FILE = 1,      // File header
        HEADTYPE_RECOVERY = 2,  // Recovery record
        HEADTYPE_EOF = 0xFF,    // End-of-archive
    };

    // head_flags bits for archive header
    static const quint16 ARCHFLAG_ADVERT  = 0x0100;  // AV string present
    static const quint16 ARCHFLAG_COMMENT = 0x0200;  // Comment present
    static const quint16 ARCHFLAG_SOLID   = 0x0080;  // Solid archive

    // head_flags bits for file header
    static const quint16 FILEFLAG_ADDSIZE = 0x0001;  // Additional data (compsize) present
    static const quint16 FILEFLAG_COMMENT = 0x0002;  // Comment present
    static const quint16 FILEFLAG_ENCRYPT = 0x0004;  // Encrypted
    static const quint16 FILEFLAG_SOLID   = 0x0008;  // Part of solid archive

    // ACE magic at offset 7 of archive header block: "**ACE**"
    static const quint8 MAGIC[7];
    static const qint32 MAGIC_OFFSET = 7;  // Offset within archive block where magic starts

    explicit XACE(QIODevice *pDevice = nullptr);

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

private:
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
};

#endif  // XACE_H

