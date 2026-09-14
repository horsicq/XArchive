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
#ifndef XSYDEXSFXARCHIVE_H
#define XSYDEXSFXARCHIVE_H

#include <QPointer>
#include <QSet>

#include "xarchive.h"

// Sydex's self-extracting DISK IMAGE (the 1995 "SFX Sydex" generation, an
// MZ + NE carrier).  It is a sibling of - and NOT the same thing as - XCopyQM,
// which decodes the TX help-screen overlay of the same vendor's tools.
//
// The payload lives in the DOS overlay, i.e. at (e_cp - 1) * 512 + e_cblp, and
// has two stages:
//
//   stage 1, the extractor's own text stage, 8-byte header:
//     +0  2  "WB"
//     +2  2  u16, always < 0x200
//     +4  2  u16, non-zero
//     +6  2  u16, non-zero - the DECODED length of the stage-1 stream
//     +8  ...  the stage-1 LZHUF stream; its first word is 0xE4D5 or 0xE5CC
//   The reference extractor decodes this block into a null sink purely to
//   learn where it ends, then rounds that length up to even.  The whole stage
//   is UI text this reader does not want, so it is skipped structurally
//   instead: the stage-2 header that follows is self-checking (see below) and
//   is located by scanning the overlay for it.
//
//   stage 2, the image header, 33 bytes:
//     +0x00  3  "SXD"
//     +0x03  2  u16 BYTES PER TRACK (the stage-2 block's decoded size)
//     +0x05  1  sectors per track
//     +0x06  1  heads
//     +0x07  1  cylinders on the medium
//     +0x08  1  cylinders actually STORED - heads * this is the block count
//     +0x0f  2  i16, non-zero means an encrypted image, which is refused
//     +0x13  2  i16 length of the description text that follows the header
//     +0x1f  2  u16 CRC-16/ARC (reflected 0xA001, init 0) of bytes 0..0x1e
//
// Then one block per track, in cylinder-major order:
//     +0  2  i16 CRC-16/ARC of the DECODED track
//     +2  2  i16 length: > 0 is that many bytes of LZHUF, < 0 is -length bytes
//            of a CopyQM-style RLE stream (i16 n: n > 0 copies n literal bytes,
//            n < 0 repeats the next single byte -n times)
//
// Both LZHUF stages are LHA "-lh1-" in the reference implementation's own
// parameterisation - distance variant 1, F = 60, THRESHOLD = 2, no end symbol,
// MAX_FREQ 0x8000 - with the ONE deviation that the ring buffer starts filled
// with 0x00 rather than 0x20.  That is exactly XLZHUFDecoder::getOptions(1, 1,
// 0, false, false, true), so this class adds no codec of its own.
//
// The whole image is one member, named after the carrier with a ".img" suffix,
// which is what the reference extractor emits as well.
//
// The base is XArchive, NOT XBinary.  XArchives::decompressToFolder() -
// which is what --extractarchive and --testarchive both go through -
// reaches an archive reader by dynamic_cast<XArchive *> and refuses
// anything that is not one, so an XBinary-derived reader that is
// registered as an archive lists perfectly and extracts nothing.
class XSydexSFXArchive : public XArchive {
    Q_OBJECT

public:
    struct FILE_ENTRY {
        QString sName;
        QByteArray baData;
        qint64 nStreamOffset = 0;
        qint64 nStreamSize = 0;
    };

    struct UNPACK_CONTEXT {
        QList<FILE_ENTRY> listEntries;
        QPointer<QIODevice> pSourceDevice;
        UNPACK_STATE *pOwnerState = nullptr;
        QByteArray baToken;
        quint64 nDeviceGeneration = 0;
        qint64 nSourceSize = 0;
        qint32 nCurrentIndex = 0;
        qint64 nCurrentOffset = 0;
    };

    explicit XSydexSFXArchive(QIODevice *pDevice = nullptr);
    ~XSydexSFXArchive() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);

    FT getFileType() override;
    MODE getMode() override;
    QString getArch() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

protected:
    bool isDeviceReplacementAllowed() const override;

private:
    // bDecode false stops after the two headers have been validated, which is
    // all isValid() needs and keeps detection off the LZHUF path.
    bool _parse(QList<FILE_ENTRY> *pEntries, qint64 *pnSourceSize, bool bDecode, PDSTRUCT *pPdStruct);
    bool _isContextCurrent(const UNPACK_STATE *pState, const UNPACK_CONTEXT *pContext);

    QSet<UNPACK_CONTEXT *> m_setContexts;
};

#endif  // XSYDEXSFXARCHIVE_H
