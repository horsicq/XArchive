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
#ifndef XX64ARCHIVE_H
#define XX64ARCHIVE_H

#include "xarchive.h"

// X64 - a VICE C64 disk image.
//
//   +0x00 0x43 0x15 0x41 0x64   ('C', 0x15, 'A', 'd')
//   +0x04 u8 == 1               header version major
//   +0x05 u8                    header version minor
//   +0x06 u8                    device type
//   +0x07 u8                    track count (0x23 = 35)
//   ... a 64-byte header in total, then a plain D64 image.
//
// D64 geometry - sectors per track
//   tracks 1..17: 21    18..24: 19    25..30: 18    31..40: 17
// The byte offset of (track, sector) is the sectors on tracks 1..track-1 times
// 0x100, plus sector * 0x100.
//
// The directory chain starts at track 18 sector 1.  Each directory sector holds
// eight entries of 0x20 bytes, and bytes 0..1 of the sector are the track/sector
// link to the next directory sector (track zero ends the chain).
// Entry
//   +0x02 u8   file type (0 = deleted or empty; 0x80 | type when closed)
//   +0x03 u8   first track
//   +0x04 u8   first sector
//   +0x05 char[16] name, 0xA0 padded
//   +0x1e u16  size in blocks
// File data follows the sector chain: bytes 0..1 of every data sector are the
// link; while the link track is non-zero the sector carries 254 data bytes, and
// on the last sector (link track zero) the link's second byte is the index of
// the last used byte, so the payload is bytes 2..thatIndex inclusive.
//
// A member is therefore a CHAIN of 254-byte fragments scattered across the
// image, not one extent, so its records are index-paired archive-stream records
// and unpackCurrent() assembles them - the same shape XADFArchive uses for
// AmigaDOS block lists.
class XX64Archive : public XArchive {
    Q_OBJECT

public:
    struct EXTENT {
        qint64 nOffset;
        qint64 nSize;
    };

    struct MEMBER {
        qint64 nEntryOffset;
        qint64 nSize;
        qint32 nBlocks;
        quint8 nFileType;
        QString sFileName;
        QList<EXTENT> listExtents;
    };

    explicit XX64Archive(QIODevice *pDevice = nullptr);
    ~XX64Archive() override;

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
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct CONTEXT {
        qint64 nInputSize;
        qint64 nImageOffset;
        qint64 nImageSize;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    bool collectChain(CONTEXT *pContext, quint8 nTrack, quint8 nSector, MEMBER *pMember, PDSTRUCT *pPdStruct);
    static qint32 sectorsPerTrack(qint32 nTrack);
    static qint64 sectorOffset(qint32 nTrack, qint32 nSector);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XX64ARCHIVE_H
