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
#ifndef XTRDOSARCHIVE_H
#define XTRDOSARCHIVE_H

#include "xarchive.h"

// TR-DOS disk image (.TRD) - the raw sector dump of a ZX Spectrum Beta Disk
// floppy.  Track 0 sectors 0..7, i.e. file bytes 0x000..0x7ff, are the
// catalogue: 128 records of 16 bytes.
//
//   record: +0x00 char[8] name  (byte 0 == 0 ends the catalogue, == 1 deleted)
//           +0x08 char    the single-character TR-DOS extension
//           +0x09 u16     start address
//           +0x0b u16     length in bytes
//           +0x0d u8      sector count
//           +0x0e u8      first sector, 0..15
//           +0x0f u8      first track
//
//   member data lives at  sector * 0x100 + track * 0x1000  and is
//   sectorCount * 0x100 bytes of raw, uncompressed sectors.
//
// A MEMBER IS NOT WRITTEN AS RAW SECTORS.  The reference extractor emits a
// HOBETA file: a 17-byte header - the record's first 13 bytes, a zero, the
// sector count, and a little-endian u16 checksum of
// (sum of those 15 bytes) * 0x101 + 0x69 - followed by the sectors.  That is
// the same prefix-then-copy shape as the SCL container, so it reuses
// HANDLE_METHOD_SCL_SECTORS with the prefix in FPART_PROP_COMPRESSPROPERTIES
// rather than introducing a codec.
//
// DETECTION IS THE HARD PART, and this class deliberately does MORE than the
// reference walker.  A .TRD has NO MAGIC: the reference detector accepts any
// file of 0x800..0xa0000 bytes whose first catalogue record is not free, which
// in a shared detector chain would claim a large share of every other family's
// samples.  So the TR-DOS DISK DESCRIPTOR in sector 8 is required as well:
//
//   +0x8e3 u8  disk type, 0x16..0x19 (80/40 tracks, double/single sided)
//   +0x8e7 u8  0x10, the TR-DOS identifier
//
// Both hold in all 21 reference images and both are part of the on-disk
// format, not a heuristic.  If a legitimately damaged image ever needs to be
// read, THAT is the check to relax - not the catalogue walk.
class XTRDOSArchive : public XArchive {
    Q_OBJECT

public:
    struct MEMBER {
        qint64 nHeaderOffset;
        qint64 nDataOffset;
        qint64 nDataSize;
        QByteArray baPrefix;
        QString sFileName;
    };

    explicit XTRDOSArchive(QIODevice *pDevice = nullptr);
    ~XTRDOSArchive() override;

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
        quint8 nDiskType;
        QList<MEMBER> listMembers;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XTRDOSARCHIVE_H
