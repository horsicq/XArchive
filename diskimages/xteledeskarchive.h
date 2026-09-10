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
#ifndef XTELEDESKARCHIVE_H
#define XTELEDESKARCHIVE_H

#include "xarchive.h"

// Sydex TeleDisk (.TD0) - a floppy image, published as ONE member.
//
// The reference extractor does not walk the FAT filesystem inside: it rebuilds
// the raw sector image and writes it out as "<archive basename>.ima", so this
// reader has exactly one record and it covers the WHOLE FILE (the
// xuleadarchive.cpp pattern), because the payload transform starts at offset 12
// and every track shares it.
//
//   +0   "TD" or "td"          signature; the case is not cosmetic, see below
//   +2   u8   sequence
//   +3   u8   check signature
//   +4   u8   version          10..21
//   +5   u8   data rate        low 7 bits must be <= 2
//   +6   u8   drive type       <= 6
//   +7   u8   stepping         bit 7 set means a comment block follows
//   +8   u8   DOS allocation flag
//   +9   u8   sides            1 or 2
//   +10  u16  CRC16 of the first ten bytes
//
// THE PAYLOAD TRANSFORM IS CHOSEN BY SIGNATURE **AND** VERSION - see
// Algos/xteledeskdecoder.h for the three of them.  The one that shapes this
// class is "td" version 20..21: Okumura LZHUF as ONE CONTINUOUS STREAM ACROSS
// THE WHOLE ARCHIVE.  A member therefore cannot be addressed by a file extent
// and the decoder has to be pull-based, which is why the record hands over the
// whole file rather than a payload slice.
//
// CRC16 everywhere is poly 0xA097, MSB first, init 0, no reflection and no
// final xor.
//
// THREE THINGS THAT LOOK LIKE BUGS AND ARE THE FORMAT:
//   * nSectors == 0xFF ends the archive and is tested BEFORE the track header
//     CRC.  Bytes after the terminator are ignored - 21 of the 23 reference
//     samples have some;
//   * sectors are written IN FILE ORDER, each padded to max(size, 512).  No
//     sorting, no seeking to a computed offset, no filling of missing sectors;
//   * on any error the reference KEEPS the partial image and reports the
//     failure alongside it.  A reader that discards partial output will not
//     match it, so the record's uncompressed size is whatever the rebuild
//     actually produced, complete or not.
class XTeleDiskArchive : public XArchive {
    Q_OBJECT

public:
    explicit XTeleDiskArchive(QIODevice *pDevice = nullptr);
    ~XTeleDiskArchive() override;

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
        quint8 nVersion;
        bool bCompressed;
        bool bAdvancedCodec;
        bool bComplete;
        bool bSizeKnown;
        QString sFileName;
    };

    // bMeasure runs the whole rebuild to learn the image size; detection does
    // not need it and must not pay for it.
    bool parseContext(CONTEXT *pContext, bool bMeasure, PDSTRUCT *pPdStruct);
    static QString methodToString(const CONTEXT &context);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XTELEDESKARCHIVE_H
