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
#ifndef XLZPIS2_H
#define XLZPIS2_H

#include "xarchive.h"

// "LZPIS2" compressed file, as produced by the DOS/Windows installer that packs
// each payload individually and replaces the LAST character of the file's
// extension with '$' (l731bam.dl$, R120B.EX$, startup.wa$, MONO.C$ ...).
//
// The container holds exactly one object and stores no name, no timestamp and
// no checksum: the 6-byte ASCII magic is followed by a chain of chunks, each
// one { u16 LE uncompressed size (1..0x1000), u16 LE compressed size } plus
// that many compressed bytes, tiling the file exactly to EOF.  Every chunk is
// an independent LZHUF stream (see Algos/xlzpis2decoder.h for the parameters).
//
// isValid() is deliberately strict because a six-byte ASCII magic on its own is
// weak: the whole chunk chain must tile the container to the last byte, every
// chunk header must be in range, and the first chunk must actually decode to
// its declared length without reading past its own compressed bytes.
class XLzpis2 : public XArchive {
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
        STRUCTID_LZPIS2_HEADER,
        STRUCTID_LZPIS2_CHUNK_HEADER
    };

#pragma pack(push)
#pragma pack(1)
    struct LZPIS2_HEADER {
        char signature[6];
    };

    struct LZPIS2_CHUNK_HEADER {
        quint16 uncompressed_size;
        quint16 compressed_size;
    };
#pragma pack(pop)

    explicit XLzpis2(QIODevice *pDevice = nullptr);

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

    virtual QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    // Walks the chunk chain.  Returns false unless the chain tiles the whole
    // container and (when bTrialDecode) the first chunk really decodes.
    bool _scanChain(qint64 *pnUncompressedSize, qint32 *pnChunkCount, bool bTrialDecode, PDSTRUCT *pPdStruct);

    struct LZPIS2_UNPACK_CONTEXT {
        qint64 nTotalSize;
        qint64 nUncompressedSize;
        qint32 nChunkCount;
        QString sFileName;
    };

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XLZPIS2_H
