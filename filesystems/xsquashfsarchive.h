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
#ifndef XSQUASHFSARCHIVE_H
#define XSQUASHFSARCHIVE_H

#include "xarchive.h"

#include "Algos/xsquashfsdecoder.h"

// SquashFS reader for versions 1, 2, 3 AND 4.
//
// SQUASHFS IS NOT ONE LAYOUT: a v4-only reader covers only four of the seven
// reference images, the rest being v3.0 and v2.1, and the three superblock
// shapes are 96, 119 (PACKED, with mkfs_time UNALIGNED at 0x27 and the 64-bit
// tables only from 0x3F) and 63 bytes.  XSquashFSDecoder carries the rest of
// the layout facts, including why v1..v3 have to sniff the compressor instead
// of reading it - one v3 image in the corpus stores LZMA data blocks under the
// ordinary 'hsqs' magic, and assuming zlib yields 1,334 bytes where the real
// content is 13.5 MB.
//
// Detection is magic plus a major version of 1..4, which is all the reference
// checks: there is no length field and no checksum anywhere in the format, so
// isValid() stays cheap and the inode walk only happens on the full parse.
//
// A member's data is a block list plus an optional fragment, so it is not one
// contiguous range.  The record publishes the smallest span that holds every
// piece and the chunk list travels with it as FPART_PROP_COMPRESSPROPERTIES.
class XSquashFSArchive : public XArchive {
    Q_OBJECT

public:
    explicit XSquashFSArchive(QIODevice *pDevice = nullptr);
    ~XSquashFSArchive() override;

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
        XSquashFSDecoder::SUPERBLOCK superBlock;
        QList<XSquashFSDecoder::MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, bool bFull, PDSTRUCT *pPdStruct);
    static QString compressorToString(qint32 nCompressor);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XSQUASHFSARCHIVE_H
