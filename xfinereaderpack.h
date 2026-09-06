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
#ifndef XFINEREADERPACK_H
#define XFINEREADERPACK_H

#include "xarchive.h"

// BIT Software / ABBYY FineReader install-media compressed file.
//
// 17-byte header: "FINEAR" + DD 88 DD, then LE u32 CRC-16/ARC of the plaintext
// (zero-extended to 32 bits) and LE u32 uncompressed size.  Everything after
// byte 17 is one Yoshizaki/Okumura LZHUF stream - a 4 KiB LZSS window whose
// literal/length alphabet (256 literals + lengths 3..60, N_CHAR = 314) is coded
// with an adaptive Huffman tree and whose match positions use the canonical
// fixed 64-symbol table.  That is bit-for-bit the LHA -lh1- codec, so the
// member is handed to the existing HANDLE_METHOD_LZH1; no new codec is added.
class XFineReaderPack : public XArchive {
    Q_OBJECT
public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_FINEAR_HEADER
    };

#pragma pack(push)
#pragma pack(1)
    struct FINEAR_HEADER {
        char signature[6];  // "FINEAR"
        quint8 magic[3];    // DD 88 DD
        quint32 crc;        // CRC-16/ARC of the plaintext
        quint32 unpacked_size;
    };
#pragma pack(pop)

    explicit XFineReaderPack(QIODevice *pDevice = nullptr);

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    FT getFileType() override;
    MODE getMode() override;
    QString getMIMEString() override;
    qint32 getType() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    bool isSigned() override;
    OSNAME getOsName() override;
    QString getOsVersion() override;
    QString getVersion() override;
    bool isEncrypted() override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;

    QString structIDToString(quint32 nID) override;
    QString structIDToFtString(quint32 nID) override;
    quint32 ftStringToStructID(const QString &sFtString) override;
    QList<XFHEADER> getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct) override;
    QList<XFRECORD> getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    FINEAR_HEADER _read_FINEAR_HEADER(qint64 nOffset);

    virtual QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    bool _readAndCheckHeader(quint32 *pnUnpackedSize, quint32 *pnCrc, PDSTRUCT *pPdStruct);

    struct FINEAR_UNPACK_CONTEXT {
        qint64 nTotalSize;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nCrc;
        QString sFileName;
    };

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XFINEREADERPACK_H
