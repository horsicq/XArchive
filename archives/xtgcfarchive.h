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
#ifndef XTGCFARCHIVE_H
#define XTGCFARCHIVE_H

#include "xarchive.h"

// TGCF - the "Setup Specialist" SFX / multi-volume installer container.
//
// EVERYTHING IS BIG ENDIAN except the two method-ish fields at record offsets
// +0x0c and +0x0e, which the reference reads as LITTLE-endian u16 - so the odd
// byte of each pair has to be zero for the record to mean what it looks like.
//
// FILE HEADER (0x1c + namelen + [4] + 4)
//   +0x00 u32    'TGCF'
//   +0x04 u16BE  0x0024, the size of the header record proper
//   +0x06 u16BE  version 0x0130 / 0x0140 / 0x0160.  >= 0x0160 is "extended".
//   +0x08 u16BE  0x0130, the minimum reader version
//   +0x0a u16BE  0x0300
//   +0x0c u16BE  0x0002
//   +0x0e u32BE  time_t
//   +0x12 u32BE  time_t
//   +0x16 u16BE  volume (disk) number, 1 based
//   +0x18 u32BE  volume name length (the detector only reads the low u16 at
//                +0x1a and requires 1..0x400)
//   +0x1c        the volume name, not NUL terminated ("SETUP.3")
//   [+4          u32BE absolute offset of the record list - EXTENDED ONLY]
//   +4           u32BE header CRC (skipped)
//
// MEMBER RECORD (0x24 + name1 + name2 + 1 + [4] + 4)
//   +0x00 u32    'TGCF'
//   +0x04 u16BE  0x033c
//   +0x06 u16BE  version
//   +0x08 u16BE  0x0130
//   +0x0a u16BE  0x0300
//   +0x0c u16LE  split flag; 2 means this record is a VOLUME-SPANNING FRAGMENT
//                and only part of its stream lives in this volume.  The
//                reference skips such records entirely, so they are not listed.
//   +0x0e u16LE  method: 0 stored, 4 zlib
//   +0x10 u32BE  time_t mtime
//   +0x14 u32BE  compressed size
//   +0x18 u32BE  uncompressed size
//   +0x1c u32BE  CRC-32 of the uncompressed data
//   +0x20 u32BE  Win32 attributes
//   +0x24        name1 (NUL terminated 8.3 / short path), name2 (NUL terminated
//                long path), u8 flag
//   [+4          u32BE ABSOLUTE offset of this member's payload - EXTENDED ONLY]
//   +4           u32BE record CRC (skipped)
//   For version < 0x160 the payload follows the record immediately; for
//   >= 0x160 the record list is contiguous and the payload lives at the
//   absolute offset above.
//
// TRAILER: 'TGCF' u16BE(0) u32BE(the trailer's own offset).  The walk stops as
// soon as fewer than 11 bytes remain.
//
// METHOD 4 IS STOCK ZLIB (RFC 1950), NOT RAW DEFLATE: the reference calls
// inflateInit2(z, 15) and its inflate() parses the CMF/FLG pair and the
// trailing big-endian ADLER-32, so the payload carries the two-byte header and
// the four-byte checksum.  Feeding it to a raw-deflate path fails on the very
// first byte pair, which is why this reader publishes HANDLE_METHOD_ZLIB and
// not HANDLE_METHOD_DEFLATE.
//
// CRC: the reference seeds with 0xFFFFFFFF and complements the result when the
// record's "extended" flag is 0 - i.e. plain CRC-32 - and seeds with 0 without
// complementing when it is set (version >= 0x160).  Nothing here verifies it;
// the note is recorded because the two conventions differ by exactly the
// complement and are easy to confuse.
class XTGCFArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nCRC;
        quint32 nTime;
        quint32 nAttributes;
        quint16 nMethod;
        QString sFileName;
    };

    explicit XTGCFArchive(QIODevice *pDevice = nullptr);
    ~XTGCFArchive() override;

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
        qint64 nHeaderSize;
        quint16 nVersion;
        bool bExtended;
        QString sVolumeName;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static QString methodToString(quint16 nMethod);
    static HANDLE_METHOD methodToHandleMethod(quint16 nMethod, qint64 nCompressedSize, qint64 nUncompressedSize);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XTGCFARCHIVE_H
