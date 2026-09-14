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
#ifndef XLZWDARCHIVE_H
#define XLZWDARCHIVE_H

#include "xarchive.h"

// HP NewWave "<FC>LZW" compressed file - the family the ARC8 corpus and the
// reference tool both label LZWD.  One member behind an eleven-byte header and
// nothing else:
//
//   +0   i32  uncompressed size (never negative)
//   +4   u32  0x575A4CFC, the bytes FC 'L' 'Z' 'W'
//   +8   u16  format version, always 1
//   +10  u8   flags, always 0
//   +11       the LZW stream (XNewWaveLZWDecoder), always opening 0x80 - a
//             nine-bit MSB-first clear code
//
// Nothing else is stored: no name, no timestamp, no checksum, no member count.
// The container is named after the file it holds - the payloads are HP NewWave
// parts such as HPOMF.INS, VNWD.386 and NWOS.HLP - so the member takes the
// archive's own file name verbatim, which is also what the reference publishes.
// There is exactly one member, so that name can never collide with a sibling.
class XLZWDArchive : public XArchive {
    Q_OBJECT

public:
    explicit XLZWDArchive(QIODevice *pDevice = nullptr);
    ~XLZWDArchive() override;

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
        quint16 nVersion;
        QString sFileName;
    };

    bool parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XLZWDARCHIVE_H
