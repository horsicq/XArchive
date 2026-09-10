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
#ifndef XVMDKARCHIVE_H
#define XVMDKARCHIVE_H

#include "xarchive.h"

// VMware "KDMV" monolithic-sparse disk image, listed as the FAT files it
// contains.
//
// THE TEXT DESCRIPTOR IS DECORATIVE.  Only the binary SparseExtentHeader at
// offset 0 matters; createType/extent lists only become load bearing for split
// or flat extent sets, which no reference sample uses.
//
//   +0x00 u32 'KDMV'      +0x04 u32 version        +0x08 u32 flags
//   +0x0C u64 capacity in 512-byte sectors
//   +0x14 u64 grainSize in sectors
//   +0x1C u64 descriptorOffset  +0x24 u64 descriptorSize
//   +0x2C u32 numGTEsPerGT
//   +0x30 u64 rgdOffset   +0x38 u64 gdOffset       +0x40 u64 overHead
//   +0x4D u16 compressAlgorithm (0 none, 1 deflate - COWD/stream only)
//
// numGDEntries = ceil(capacity / (grainSize * numGTEsPerGT)).  The grain
// directory is that many u32 sector numbers, each pointing at a grain table of
// numGTEsPerGT u32 sector numbers.  A ZERO at either level means unallocated
// and reads as zeros - that is not an error, it is how a sparse image stores
// empty space.
//
// On top of the flat disk: an MBR at LBA 0 (types 0x05/0x0F/0x85 are extended
// containers followed recursively) and then a plain BPB/FAT walk - fewer than
// 4085 clusters is FAT12, fewer than 65525 is FAT16, otherwise FAT32, and
// FAT12 entries are nibble-packed.  Only regular files are emitted: 0xE5
// deleted entries, attr 0x0F long-name fragments, attr & 0x08 volume labels
// and "." / ".." are all skipped - but a ZERO-SIZE entry IS emitted, as an
// empty file.  Members are named "FAT.Partition.<n>/<path>".
//
// A member's bytes are a cluster chain, so they are not one contiguous range.
// The record therefore covers the smallest span that holds all of them and the
// run list travels with the member as FPART_PROP_COMPRESSPROPERTIES; see
// XVMDKDecoder.
class XVMDKArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;  // the span start, which is also the progress cursor
        qint64 nSpanOffset;
        qint64 nSpanSize;
        qint64 nUncompressedSize;
        QByteArray baDescriptor;
        QString sFileName;
    };

    explicit XVMDKArchive(QIODevice *pDevice = nullptr);
    ~XVMDKArchive() override;

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
        qint64 nVersion;
        qint64 nCapacity;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bFull, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

// The run list a VMDK member is assembled from.  It travels as
// FPART_PROP_COMPRESSPROPERTIES and its offsets are RELATIVE to the byte range
// the record publishes:
//
//   +0  u32 magic 'VMDR'
//   +4  u32 run count
//   +8  u64 member size
//   then one 16-byte run per entry:
//     +0  u64 relative offset, or 0xFFFFFFFFFFFFFFFF for a hole
//     +8  u64 length
//
// A hole is an unallocated grain: it reads as zeros and occupies no bytes in
// the file, which is why a fully sparse member can have an EMPTY byte range
// and still produce its full size.
class XVMDKDecoder {
public:
    static const qint64 MAX_UNCOMPRESSED_SIZE = 0x40000000;
    static const qint32 RUN_SIZE = 16;
    static const qint32 DESCRIPTOR_HEADER_SIZE = 16;

    static QByteArray startDescriptor();
    static bool appendRun(QByteArray *pbaDescriptor, qint64 nRelativeOffset, qint64 nSize);
    static void setSize(QByteArray *pbaDescriptor, qint64 nSize);

    static bool decode(const QByteArray &baPacked, const QByteArray &baProperties, qint64 nUncompressedSize, QByteArray *pbaResult,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XVMDKARCHIVE_H
