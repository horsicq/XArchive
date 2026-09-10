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
#ifndef XZCMPARCHIVE_H
#define XZCMPARCHIVE_H

#include "xarchive.h"

// Zcmp - the Solaris "compressed file" wrapper: ONE payload split into fixed
// size blocks, each an independent zlib stream, behind a seek index so any
// block can be reached without inflating the ones before it.
//
// Header, 40 bytes, ALL BIG ENDIAN - this is the first thing to get wrong,
// since every other single-member container in this tree is little endian:
//
//   0x00   4  0x00000000
//   0x04   4  magic "Zcmp"
//   0x08   8  0x0000000000000001   version / format id
//   0x10   8  0x0000000000000001   version / format id
//   0x18   8  uncompressed size of the payload
//   0x20   8  block size in bytes, greater than zero
//
// TRAP - A SKIPPED SEEK INDEX SITS BETWEEN THE HEADER AND THE DATA.  Straight
// after the header come (blockCount + 1) big-endian 8-byte file offsets, where
// blockCount is ceil(uncompressedSize / blockSize).  It is easy to miss because
// the reference reader never READS it - it only skips that many bytes - and
// because the trailing entry is zero in every sample, so an implementation that
// tried to use the table would look plausible and then seek to offset 0.  The
// first byte of real data is at 40 + (blockCount + 1) * 8; starting to inflate
// at 40 finds an index entry, not a 78 DA.
//
// The data is blockCount zlib streams BACK TO BACK, each inflating to exactly
// the block size except the last, which yields the remainder.  There are no
// lengths in front of them: a block ends where zlib says it ends, and the next
// one starts at the very next byte, which is why Algos/xzcmpdecoder.h drives
// inflate itself instead of handing one buffer to a generic zlib method.
// XZcmpDecoder::decode() is the whole of HANDLE_METHOD_ZCMP_BLOCKS and takes
// the record's published stream - the bytes after the header and the skipped
// index.
//
// Nothing about the member is stored - no name, no timestamp, no CRC - so the
// member is named after the ARCHIVE's own file, which is what the reference
// extractor does.
class XZcmpArchive : public XArchive {
    Q_OBJECT

public:
    explicit XZcmpArchive(QIODevice *pDevice = nullptr);
    ~XZcmpArchive() override;

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
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        qint64 nBlockSize;
        qint64 nBlockCount;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XZCMPARCHIVE_H
