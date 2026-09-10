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
#include "xuleaddecoder.h"

#include "xsharedlzwdecoder.h"

#include <QtEndian>

namespace {
const char ULEAD_MAGIC[] = "U_LEAD CORP.";
const qint32 ULEAD_MAGIC_SIZE = 12;
const qint32 ULEAD_BLOCK_SIZE = 0x4000;
const qint32 ULEAD_HEADER1_SIZE = 0x1c;
const qint32 ULEAD_HEADER2_SIZE = 0x2c;
const qint32 ULEAD_NAME_OFFSET = 0x18;
const qint32 ULEAD_NAME_SIZE = 0x10;

quint16 uleadRead16(const QByteArray &baFile, qint64 nOffset)
{
    return qFromLittleEndian<quint16>((const uchar *)baFile.constData() + nOffset);
}

quint32 uleadRead32(const QByteArray &baFile, qint64 nOffset)
{
    return qFromLittleEndian<quint32>((const uchar *)baFile.constData() + nOffset);
}
}  // namespace

bool XULEADDecoder::parseHeader(const QByteArray &baFile, qint64 nFileSize, HEADER *pHeader)
{
    if (!pHeader || (baFile.size() < ULEAD_HEADER2_SIZE) || (nFileSize < ULEAD_HEADER2_SIZE)) return false;
    if (memcmp(baFile.constData(), ULEAD_MAGIC, ULEAD_MAGIC_SIZE) != 0) return false;

    // Fixed words that must hold before either layout is accepted: two zero
    // dwords straddling a pair of 1s.  Every member of the family carries them.
    if (uleadRead32(baFile, 12) != 0) return false;
    if (uleadRead16(baFile, 16) != 1) return false;
    if (uleadRead16(baFile, 18) != 1) return false;
    if (uleadRead32(baFile, 20) != 0) return false;

    const qint32 nCount1 = (qint32)uleadRead16(baFile, 24);
    const qint32 nLast1 = (qint32)uleadRead16(baFile, 26);
    const qint32 nCount2 = (qint32)uleadRead16(baFile, 0x28);
    const qint32 nLast2 = (qint32)uleadRead16(baFile, 0x2a);

    const bool bLayout1 = (nCount1 != 0) && (nLast1 <= ULEAD_BLOCK_SIZE) && (nFileSize > ((qint64)nCount1 * 2 + ULEAD_HEADER1_SIZE));
    const bool bLayout2 = (nCount2 != 0) && (nLast2 <= ULEAD_BLOCK_SIZE) && (nFileSize > ((qint64)nCount2 * 2 + ULEAD_HEADER2_SIZE));

    qint32 nLayout = 0;
    if (bLayout1 && !bLayout2) {
        nLayout = 1;
    } else if (bLayout2 && !bLayout1) {
        nLayout = 2;
    } else if (bLayout1 && bLayout2) {
        // Both fit, so the tie goes on the block count: the shorter header
        // wins while its count is still plausible for one.
        nLayout = (nCount1 < 0x2000) ? 1 : 2;
    }
    if (nLayout == 0) return false;

    HEADER header = {};
    header.nLayout = nLayout;
    if (nLayout == 1) {
        header.nTableOffset = ULEAD_HEADER1_SIZE;
        header.nBlockCount = nCount1;
        header.nLastBlockSize = nLast1;
    } else {
        header.nTableOffset = ULEAD_HEADER2_SIZE;
        header.nBlockCount = nCount2;
        header.nLastBlockSize = nLast2;
        QByteArray baName = baFile.mid(ULEAD_NAME_OFFSET, ULEAD_NAME_SIZE);
        const qint32 nZero = baName.indexOf('\0');
        if (nZero >= 0) baName.truncate(nZero);
        header.sFileName = QString::fromLatin1(baName);
    }
    header.nDataOffset = header.nTableOffset + ((qint64)header.nBlockCount * 2);
    if (header.nDataOffset > nFileSize) return false;
    header.nUncompressedSize = ((qint64)header.nBlockCount - 1) * ULEAD_BLOCK_SIZE + header.nLastBlockSize;
    if (header.nUncompressedSize <= 0) return false;

    *pHeader = header;

    return true;
}

bool XULEADDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize <= 0) || (nUncompressedSize > 0x7fffffff)) return false;

    HEADER header = {};
    if (!parseHeader(baPacked, baPacked.size(), &header)) return false;
    if (header.nUncompressedSize != nUncompressedSize) return false;
    if ((header.nTableOffset + (qint64)header.nBlockCount * 2) > baPacked.size()) return false;

    XSharedLZWDecoder::OPTIONS options;
    options.nMaxBits = 12;
    options.bHasClearCode = true;
    options.bHasEndCode = true;
    options.bMsbFirst = true;
    options.bUnRle90 = false;
    options.bBlockPadding = false;
    options.nWidthStepBias = 1;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);
    qint64 nPosition = header.nDataOffset;

    for (qint32 i = 0; i < header.nBlockCount; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const qint32 nPackedSize = (qint32)uleadRead16(baPacked, header.nTableOffset + ((qint64)i * 2));
        const qint32 nPlainSize = (i == (header.nBlockCount - 1)) ? header.nLastBlockSize : ULEAD_BLOCK_SIZE;
        if ((nPackedSize <= 0) || ((nPosition + nPackedSize) > baPacked.size())) {
            // The block table sums to the file size in an intact member, so a
            // payload that runs off the end means the input is short, not that
            // the layout was misread.  Say which block, because "cannot unpack"
            // on its own reads as a missing capability.
            XBinary::setPdStructErrorString(pPdStruct, QString("ULEAD: block %1 of %2 needs %3 bytes at offset %4, past the end of %5")
                                                           .arg(i + 1)
                                                           .arg(header.nBlockCount)
                                                           .arg(nPackedSize)
                                                           .arg(nPosition)
                                                           .arg(baPacked.size()));
            return false;
        }

        if (nPackedSize == nPlainSize) {
            // a block that did not compress is written verbatim; equal sizes
            // are the only marker for it
            baOut.append(baPacked.constData() + nPosition, nPlainSize);
        } else {
            QByteArray baBlock;
            // The LZW decoder assigns its partial output even when it returns
            // false, so baBlock says how far into the block the stream stayed
            // coherent - which is the one number that tells a damaged member
            // apart from an unsupported one.  On
            // ULEAD/97_kybgskriwbjqckcr_ALBUM.HL_ it stops inside block 5 of
            // 11, at the byte where a 0x9400-long run of that file has been
            // overwritten with a copy of an earlier run.
            const bool bBlockDecoded = XSharedLZWDecoder::decode(baPacked.mid((qint32)nPosition, nPackedSize), options, nPlainSize, &baBlock, pPdStruct);
            if (!bBlockDecoded || (baBlock.size() != nPlainSize)) {
                if (XBinary::isPdStructNotCanceled(pPdStruct)) {
                    XBinary::setPdStructErrorString(pPdStruct, QString("ULEAD: LZW block %1 of %2 decoded %3 of %4 bytes - the compressed stream is damaged")
                                                                   .arg(i + 1)
                                                                   .arg(header.nBlockCount)
                                                                   .arg(baBlock.size())
                                                                   .arg(nPlainSize));
                }
                return false;
            }
            baOut.append(baBlock);
        }
        nPosition += nPackedSize;
    }

    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
