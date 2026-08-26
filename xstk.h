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
#ifndef XSTK_H
#define XSTK_H

#include "xarchive.h"

// Coktel Vision archive (Gob engine: Goblins, Woodruff, ...). Extensions .STK/.ITK/.LTK/.JTK.
//
// Two container generations share this reader:
//  * Classic (Gob1/2/3): headerless. uint16 file count, then fixed 22-byte directory entries
//    (13-byte DOS 8.3 name, uint32 size, uint32 offset, uint8 compression), then file data at the
//    recorded offsets.
//  * STK2.1 (later titles, e.g. Adibou 3): 32-byte header ("STK2.1" + 14-char timestamp + 8-byte
//    creator + uint32 directory offset), file data stored sequentially from offset 32, and a
//    directory at the tail: uint32 file count, uint32 metadata-table offset, a packed list of
//    NUL-terminated long file names, then fixed 61-byte metadata records (uint32 name pointer,
//    two 14-char timestamps, 8-byte creator, uint32 stored size, uint32 uncompressed size, ...).
//
// Both generations use the same Coktel LZSS for compressed members: a uint32 uncompressed size
// followed by the LZSS stream (see XCoktelLZDecoder / HANDLE_METHOD_COKTEL_LZ). A member is
// compressed when its stored size differs from its uncompressed size.
class XStk : public XArchive {
    Q_OBJECT

public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_HEADER
    };

    explicit XStk(QIODevice *pDevice = nullptr);

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    virtual FT getFileType() override;
    virtual MODE getMode() override;
    virtual QString getMIMEString() override;
    virtual qint32 getType() override;
    virtual ENDIAN getEndian() override;
    virtual QString getArch() override;
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    virtual OSNAME getOsName() override;
    virtual QString getVersion() override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;

    virtual QString structIDToString(quint32 nID) override;
    virtual QString structIDToFtString(quint32 nID) override;
    virtual quint32 ftStringToStructID(const QString &sFtString) override;
    virtual QList<XFHEADER> getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct) override;
    virtual QList<XFRECORD> getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    virtual XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    virtual QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    static const qint32 N_STK_ENTRY_SIZE = 22;    // classic: 13-byte name + uint32 size + uint32 offset + uint8 compression
    static const qint32 N_STK2_HEADER_SIZE = 32;  // STK2.1: "STK2.1" + timestamp(14) + creator(8) + uint32 dir offset
    static const qint32 N_STK2_RECORD_SIZE = 61;  // STK2.1: metadata record stride

    struct STK_RECORD {
        QString sFileName;
        qint64 nDataOffset;         // stream start (offset+4 for compressed, past the uint32 size prefix)
        qint64 nStreamSize;         // stored/compressed stream size (input to the decoder)
        qint64 nUncompressedSize;
        bool bCompressed;
    };

    struct STK_UNPACK_CONTEXT {
        QList<STK_RECORD> listRecords;
    };

    // True when the "STK2.1" signature is present (the later, 32-byte-header generation).
    bool _isStk2(PDSTRUCT *pPdStruct);
    bool _isValidStk2(PDSTRUCT *pPdStruct);
    bool _parseEntries(QList<STK_RECORD> *pListRecords, PDSTRUCT *pPdStruct);
    bool _parseEntriesStk2(QList<STK_RECORD> *pListRecords, PDSTRUCT *pPdStruct);

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XSTK_H
