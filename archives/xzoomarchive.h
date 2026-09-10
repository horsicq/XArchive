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
#ifndef XZOOMARCHIVE_H
#define XZOOMARCHIVE_H

#include "xarchive.h"

// Zoom - an Amiga floppy imager (magic "ZOM5").  The archive holds a whole
// floppy and the product is ONE .adf image of
// (lastCylinder - firstCylinder + 1) * 0x2C00 bytes, where 0x2C00 is a cylinder
// of 2 tracks * 11 sectors * 512 bytes.  Everything multi-byte is BIG ENDIAN;
// it is an Amiga format.
//
// File header, 0x4C bytes:
//   0x00   4  magic "ZOM5"
//   0x04   1  first cylinder
//   0x05   1  last cylinder
//   0x06   1  format version, must be 5
//   0x1C   4  length N of a trailing note block, 0 = none; when it is set the
//             header is followed by N + 4 bytes (note plus its checksum) that
//             are skipped
//   0x24   1  non zero = password protected, which the reference reader
//             refuses outright
//   0x48   4  checksum over bytes 0..0x47
//
// Then chunk records back to back, each a 42-byte header plus its payload:
//   0x00   5  five cylinder numbers, one per slot; 0xFF = slot unused
//   0x05   1  padding
//   0x06  20  five u32 sector bitmasks; bit k (k = 0..21, LSB first) set means
//             "sector k of that cylinder is stored"
//   0x1A   2  u16 packed payload length
//   0x1C   2  u16 length after the LZHUF stage, 0 = there is no RLE stage
//   0x1E   2  u16 final decoded length
//   0x20   2  u16 flag, non zero = the payload is LZHUF compressed
//   0x22   4  u32 checksum of the payload
//   0x26   4  u32 checksum of bytes 0..0x25 of this record
//
// The three traps of the chunk layer - the two codec stages that run in the
// OPPOSITE order from the packer's, the LZHUF dialect that is not the textbook
// one, and the unstored sectors and cylinders that are still part of the image -
// all live in Algos/xzoomdecoder.h, which spells them out and lists what the
// reference corpus leaves unverified.
//
// The whole file is therefore the record's stream (the xuleadarchive.cpp
// pattern) and XZoomDecoder::decodeImage() walks it: the chunk index is spread
// across the records themselves and no member extent exists.
class XZoomArchive : public XArchive {
    Q_OBJECT

public:
    explicit XZoomArchive(QIODevice *pDevice = nullptr);
    ~XZoomArchive() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    qint32 getType() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QString getVersion() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nChunksOffset;
        qint64 nUncompressedSize;
        quint8 nFirstCylinder;
        quint8 nLastCylinder;
        bool bProtected;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XZOOMARCHIVE_H
