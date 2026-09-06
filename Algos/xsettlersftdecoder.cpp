/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xsettlersftdecoder.h"

#include <QtEndian>

namespace {
// Member header shared by both image kinds: type, width, height and two words
// that must be zero for the flat variant, then the pixel payload.
const qint32 SETTLERS_IMAGE_HEADER_SIZE = 10;
const qint32 SETTLERS_MIN_IMAGE_SIZE = 11;
// The row buffer of the original is a fixed 4 KiB stack block, so a wider
// sprite than this could never have been written by the game's tools.
const qint32 SETTLERS_MAX_WIDTH = 0x400;
// Sprite type 2 paints a solid coverage mask instead of palette indices.
const qint32 SETTLERS_TYPE_MASK = 2;
const qint32 SETTLERS_BMP_FILE_HEADER_SIZE = 14;
const qint32 SETTLERS_BMP_INFO_HEADER_SIZE = 40;
const qint32 SETTLERS_BMP_PALETTE_SIZE = 1024;

qint32 settlersS16(const quint8 *pData, qint32 nOffset)
{
    return static_cast<qint32>(
        static_cast<qint16>(qFromLittleEndian<quint16>(pData + nOffset)));
}

qint32 settlersU16(const quint8 *pData, qint32 nOffset)
{
    return static_cast<qint32>(qFromLittleEndian<quint16>(pData + nOffset));
}

void settlersAppendLE16(QByteArray *pba, quint16 nValue)
{
    pba->append(static_cast<char>(nValue & 0xff));
    pba->append(static_cast<char>((nValue >> 8) & 0xff));
}

void settlersAppendLE32(QByteArray *pba, quint32 nValue)
{
    settlersAppendLE16(pba, static_cast<quint16>(nValue & 0xffff));
    settlersAppendLE16(pba, static_cast<quint16>((nValue >> 16) & 0xffff));
}

// Row-oriented RLE: each row is a sequence of [skip][run] pairs closed by a
// run of 0.  A skip advances over transparent pixels, a run either paints
// `run` palette indices (type != 2) or `run` opaque white pixels (type 2).
// pbaOut == nullptr performs the bounded trial decode used for classification;
// the checks are identical either way, which is what makes the decision and
// the conversion agree by construction.
bool settlersDecodeRLE(const quint8 *pPalette, qint32 nType,
                       const quint8 *pData, qint32 nSize, qint32 nWidth,
                       qint32 nHeight, QByteArray *pbaOut,
                       XBinary::PDSTRUCT *pPdStruct)
{
    if ((nWidth <= 0) || (nWidth > SETTLERS_MAX_WIDTH) || (nHeight <= 0)) {
        // A negative or zero width can never satisfy the per-row bounds below,
        // so rejecting it here matches the original's behaviour exactly while
        // keeping the row buffer well defined.
        return false;
    }

    QByteArray baRow;
    qint32 nPosition = 0;
    qint32 nLeft = nSize;

    for (qint32 nRow = 0; nRow < nHeight; nRow++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (pbaOut) {
            baRow.fill('\0', nWidth * 4);
            if (baRow.size() != nWidth * 4) return false;
        }
        qint32 x = 0;
        for (;;) {
            if (nLeft < 2) return false;
            x += pData[nPosition];
            if (x > nWidth) return false;
            const qint32 nRun = pData[nPosition + 1];
            nPosition += 2;
            nLeft -= 2;
            if (nRun == 0) break;
            if (x + nRun > nWidth) return false;
            if (nType == SETTLERS_TYPE_MASK) {
                if (pbaOut) {
                    for (qint32 i = 0; i < nRun; i++) {
                        baRow[(x + i) * 4 + 0] = static_cast<char>(0xff);
                        baRow[(x + i) * 4 + 1] = static_cast<char>(0xff);
                        baRow[(x + i) * 4 + 2] = static_cast<char>(0xff);
                        baRow[(x + i) * 4 + 3] = static_cast<char>(0xff);
                    }
                }
                x += nRun;
            } else {
                if (nLeft < nRun) return false;
                for (qint32 i = 0; i < nRun; i++) {
                    const quint32 nIndex = pData[nPosition];
                    if (pbaOut && pPalette) {
                        // The archive palette is RGB triplets; a BMP pixel is
                        // stored blue first.
                        baRow[x * 4 + 0] =
                            static_cast<char>(pPalette[nIndex * 3 + 2]);
                        baRow[x * 4 + 1] =
                            static_cast<char>(pPalette[nIndex * 3 + 1]);
                        baRow[x * 4 + 2] =
                            static_cast<char>(pPalette[nIndex * 3 + 0]);
                        baRow[x * 4 + 3] = static_cast<char>(0xff);
                    }
                    nPosition++;
                    nLeft--;
                    x++;
                }
            }
        }
        if (x != nWidth) return false;
        if (pbaOut) pbaOut->append(baRow);
    }

    // Every byte of the member has to be consumed: this is the check that
    // makes the trial decode a reliable classifier instead of a guess.
    return (nLeft == 0);
}
}  // namespace

XSettlersFTDecoder::XSettlersFTDecoder(QObject *parent) : QObject(parent)
{
}

XSettlersFTDecoder::KIND XSettlersFTDecoder::classify(const QByteArray &baEntry,
                                                      qint64 *pnOutputSize)
{
    const qint32 nSize = baEntry.size();
    const quint8 *pData = reinterpret_cast<const quint8 *>(baEntry.constData());
    qint64 nOutputSize = nSize;
    KIND kind = KIND_BIN;

    if ((nSize >= SETTLERS_MIN_IMAGE_SIZE) && (settlersS16(pData, 2) != 0) &&
        (settlersS16(pData, 4) != 0) &&
        settlersDecodeRLE(nullptr, settlersS16(pData, 0),
                          pData + SETTLERS_IMAGE_HEADER_SIZE,
                          nSize - SETTLERS_IMAGE_HEADER_SIZE,
                          settlersS16(pData, 2), settlersS16(pData, 4), nullptr,
                          nullptr)) {
        kind = KIND_MASK;
        nOutputSize = SETTLERS_BMP_FILE_HEADER_SIZE +
                      SETTLERS_BMP_INFO_HEADER_SIZE +
                      static_cast<qint64>(settlersU16(pData, 2)) * 4 *
                          settlersU16(pData, 4);
    } else if ((nSize >= SETTLERS_MIN_IMAGE_SIZE) &&
               (settlersS16(pData, 0) == 1) && (settlersS16(pData, 2) != 0) &&
               (settlersS16(pData, 4) != 0) && (settlersS16(pData, 6) == 0) &&
               (settlersS16(pData, 8) == 0) &&
               (static_cast<qint64>(settlersU16(pData, 2)) *
                    settlersU16(pData, 4) +
                SETTLERS_IMAGE_HEADER_SIZE == nSize)) {
        kind = KIND_BITMAP;
        nOutputSize = SETTLERS_BMP_FILE_HEADER_SIZE +
                      SETTLERS_BMP_INFO_HEADER_SIZE +
                      SETTLERS_BMP_PALETTE_SIZE +
                      (nSize - SETTLERS_IMAGE_HEADER_SIZE);
    } else if ((nSize >= 5) && (baEntry.left(4) == QByteArray("FORM", 4))) {
        kind = KIND_XMI;
    } else if (nSize == PALETTE_SIZE) {
        kind = KIND_PALETTE;
    }

    if (pnOutputSize) *pnOutputSize = nOutputSize;
    return kind;
}

bool XSettlersFTDecoder::decode(const QByteArray &baPacked,
                                const QByteArray &baPalette,
                                qint64 nUncompressedSize,
                                QByteArray *pbaUnpacked,
                                XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaUnpacked || (baPalette.size() != PALETTE_SIZE)) return false;

    qint64 nExpectedSize = 0;
    const KIND kind = classify(baPacked, &nExpectedSize);
    if ((kind != KIND_BITMAP) && (kind != KIND_MASK)) return false;
    if (nExpectedSize != nUncompressedSize) return false;

    const quint8 *pData = reinterpret_cast<const quint8 *>(baPacked.constData());
    const quint8 *pPalette =
        reinterpret_cast<const quint8 *>(baPalette.constData());
    const qint32 nWidth = settlersU16(pData, 2);
    const qint32 nHeight = settlersU16(pData, 4);

    QByteArray baResult;
    baResult.reserve(static_cast<qint32>(nUncompressedSize));

    if (kind == KIND_MASK) {
        const quint32 nImageSize =
            static_cast<quint32>(static_cast<qint64>(nWidth) * 4 * nHeight);
        baResult.append("BM", 2);
        settlersAppendLE32(&baResult,
                           nImageSize + SETTLERS_BMP_FILE_HEADER_SIZE +
                               SETTLERS_BMP_INFO_HEADER_SIZE);
        settlersAppendLE16(&baResult, 0);
        settlersAppendLE16(&baResult, 0);
        settlersAppendLE32(&baResult, SETTLERS_BMP_FILE_HEADER_SIZE +
                                          SETTLERS_BMP_INFO_HEADER_SIZE);
        settlersAppendLE32(&baResult, SETTLERS_BMP_INFO_HEADER_SIZE);
        settlersAppendLE32(&baResult, static_cast<quint32>(nWidth));
        // Negative height: the rows are stored top-down.
        settlersAppendLE32(&baResult,
                           static_cast<quint32>(-static_cast<qint32>(nHeight)));
        settlersAppendLE16(&baResult, 1);
        settlersAppendLE16(&baResult, 32);
        settlersAppendLE32(&baResult, 0);
        settlersAppendLE32(&baResult, nImageSize);
        settlersAppendLE32(&baResult, 0);
        settlersAppendLE32(&baResult, 0);
        settlersAppendLE32(&baResult, 0);
        settlersAppendLE32(&baResult, 0);

        if (!settlersDecodeRLE(pPalette, settlersS16(pData, 0),
                               pData + SETTLERS_IMAGE_HEADER_SIZE,
                               baPacked.size() - SETTLERS_IMAGE_HEADER_SIZE,
                               settlersS16(pData, 2), settlersS16(pData, 4),
                               &baResult, pPdStruct)) {
            return false;
        }
    } else {
        // The original derives biSizeImage from a 4-byte aligned stride but
        // still copies the unaligned rows verbatim; both are reproduced so the
        // output matches byte for byte.
        const quint32 nStride = static_cast<quint32>((nWidth + 3) & ~3);
        const quint32 nImageSize =
            static_cast<quint32>(static_cast<qint64>(nStride) * nHeight);
        const quint32 nOffBits = SETTLERS_BMP_FILE_HEADER_SIZE +
                                 SETTLERS_BMP_INFO_HEADER_SIZE +
                                 SETTLERS_BMP_PALETTE_SIZE;
        baResult.append("BM", 2);
        settlersAppendLE32(&baResult, nImageSize + nOffBits);
        settlersAppendLE16(&baResult, 0);
        settlersAppendLE16(&baResult, 0);
        settlersAppendLE32(&baResult, nOffBits);
        settlersAppendLE32(&baResult, SETTLERS_BMP_INFO_HEADER_SIZE);
        settlersAppendLE32(&baResult, static_cast<quint32>(nWidth));
        settlersAppendLE32(&baResult,
                           static_cast<quint32>(-static_cast<qint32>(nHeight)));
        settlersAppendLE16(&baResult, 1);
        settlersAppendLE16(&baResult, 8);
        settlersAppendLE32(&baResult, 0);
        settlersAppendLE32(&baResult, nImageSize);
        settlersAppendLE32(&baResult, 0);
        settlersAppendLE32(&baResult, 0);
        settlersAppendLE32(&baResult, 0x100);
        settlersAppendLE32(&baResult, 0x100);

        QByteArray baTable(SETTLERS_BMP_PALETTE_SIZE, '\0');
        for (qint32 i = 0; i < 256; i++) {
            baTable[i * 4 + 0] = static_cast<char>(pPalette[i * 3 + 2]);
            baTable[i * 4 + 1] = static_cast<char>(pPalette[i * 3 + 1]);
            baTable[i * 4 + 2] = static_cast<char>(pPalette[i * 3 + 0]);
            baTable[i * 4 + 3] = '\0';
        }
        baResult.append(baTable);
        baResult.append(baPacked.mid(SETTLERS_IMAGE_HEADER_SIZE));
    }

    if (static_cast<qint64>(baResult.size()) != nUncompressedSize) return false;
    *pbaUnpacked = baResult;
    return true;
}
