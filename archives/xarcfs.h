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
#ifndef XARCFS_H
#define XARCFS_H

#include "xarchive.h"

// ArcFS - the Acorn RISC OS archive (also what Spark writes).  It is an ARC
// derivative: the codecs are ARC's, but the layout is a flat pre-order
// directory block rather than a chain of member headers.
//
//   header, 96 bytes
//     +0   "Archive" and a NUL
//     +8   u32  directory size in bytes, always a multiple of 36
//     +12  u32  where the member data starts
//
//   directory entry, 36 bytes, entries run from +96
//     +0   u8   status: 0 ends the current directory level, 1 marks a deleted
//               entry, anything else IS the compression method
//     +1   11   name, NUL padded
//     +12  u32  uncompressed size
//     +16  u32  load address
//     +20  u32  exec address
//     +24  u32  attributes; BYTE 1 OF THIS FIELD is the LZW code width
//     +28  u32  compressed size
//     +32  u32  data offset, relative to +12 of the header; bit 31 marks a
//               directory, whose contents are simply the entries that follow
//
// Methods: 0x82 stored, 0x83 RLE90 only, 0x88 "crunched" (LZW then RLE90) and
// 0xFF "compressed" (plain LZW).  Both LZW variants are LSB-first with a clear
// code and unix-compress block padding - see XSharedLZWDecoder, whose two subtle
// rules this format depends on.
//
// One deliberate difference from the reference implementation: it hard-codes a
// 12-bit width for method 0x88 and writes an EMPTY file for every member that
// declares 13 or 16, which eight of the fifty-one reference archives do.  The
// width from the entry is honoured here instead, so those archives extract.
class XArcFS : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nLoadAddress;
        quint32 nExecAddress;
        quint8 nMethod;
        quint8 nMaxBits;
        bool bIsFolder;
        QString sFileName;
    };

    explicit XArcFS(QIODevice *pDevice = nullptr);
    ~XArcFS() override;

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
        qint64 nArchiveSize;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(quint8 nMethod);
    static HANDLE_METHOD methodToHandleMethod(quint8 nMethod);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XARCFS_H
