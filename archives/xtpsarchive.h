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
#ifndef XTPSARCHIVE_H
#define XTPSARCHIVE_H

#include "xarchive.h"

// TPS - a stream archive: every 0x16-byte directory entry is IMMEDIATELY
// followed by that member's payload, there is no table and no heap.
//
//   +0x00  "TPS", 0x1a
//   then, repeatedly:
//     +0x00  u8   entry tag, always 0x02 (the same 0x02 the detector checks at
//                 file offset 4)
//     +0x01  u8   name length, 1..12
//     +0x02  12   name, NUL padded
//     +0x0e  i32  the RAW stored length, WHICH MAY BE NEGATIVE - take its
//                 absolute value.  n = abs(raw) - 4 coded bytes are on disk;
//                 the 4 subtracted bytes are the size dword at +0x12, which the
//                 block checksum covers as if it were the head of the payload.
//     +0x12  u32  uncompressed length, stored XOR 0x80808080
//     then the framed payload - see XTPSDecoder for the XOR 0x80, the 0x4000
//     blocks, the additive check byte per block and the 0x01 end marker.
//
// The archive ends when exactly one byte is left and that byte is 0x04.
//
// The reference implementation's own "skip to the next entry" arithmetic is
// n + ((n + 0x3fff) >> 14) + 1, which under-counts the check bytes whenever
// (n + 4) crosses a 0x4000 boundary that n alone does not - a latent bug in it.
// This reader derives the stride from the block rule its own reader uses,
// ceil((n + 4) / 0x4000), which is what the data actually follows.
class XTPSArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;  // the framed on-disk length
        qint64 nUncompressedSize;
        QString sFileName;
    };

    explicit XTPSArchive(QIODevice *pDevice = nullptr);
    ~XTPSArchive() override;

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
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XTPSARCHIVE_H
