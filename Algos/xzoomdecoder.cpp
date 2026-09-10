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
#include "xzoomdecoder.h"

#include "xlzhufdecoder.h"

#include <QtEndian>

namespace {
const qint64 N_ZOOM_HEADER_SIZE = XZoomDecoder::HEADER_SIZE;
const qint64 N_ZOOM_CHUNK_HEADER_SIZE = XZoomDecoder::CHUNK_HEADER_SIZE;
const qint64 N_ZOOM_CYLINDER_SIZE = XZoomDecoder::CYLINDER_SIZE;
const qint64 N_ZOOM_SECTOR_SIZE = XZoomDecoder::SECTOR_SIZE;
const qint32 N_ZOOM_SECTORS_PER_CYLINDER = XZoomDecoder::SECTORS_PER_CYLINDER;
const qint32 N_ZOOM_SLOTS = XZoomDecoder::SLOTS;
const quint8 N_ZOOM_SLOT_UNUSED = (quint8)XZoomDecoder::SLOT_UNUSED;
const qint64 N_ZOOM_MAX_OUTPUT = 0x7fffffff;

qint32 zoomPopCount(quint32 nMask)
{
    qint32 nResult = 0;
    for (qint32 i = 0; i < N_ZOOM_SECTORS_PER_CYLINDER; ++i) {
        if (nMask & (1U << i)) ++nResult;
    }

    return nResult;
}

bool zoomAppendZeroes(QByteArray *pbaOut, qint64 nCount, qint64 nMaxOutput)
{
    if ((nCount < 0) || (nCount > (nMaxOutput - pbaOut->size()))) return false;
    if (nCount == 0) return true;
    pbaOut->append(QByteArray((qint32)nCount, (char)0));

    return true;
}

bool zoomParseHeader(const QByteArray &baFile, quint8 *pnFirst, quint8 *pnLast, bool *pbProtected, qint64 *pnChunksOffset)
{
    if (baFile.size() < N_ZOOM_HEADER_SIZE) return false;
    if (baFile.left(4) != QByteArray("ZOM5", 4)) return false;
    if ((quint8)baFile.at(6) != 5) return false;

    const quint8 nFirst = (quint8)baFile.at(4);
    const quint8 nLast = (quint8)baFile.at(5);
    if (nLast < nFirst) return false;

    const qint64 nNoteSize = (qint64)qFromBigEndian<quint32>((const uchar *)baFile.constData() + 0x1c);
    if (nNoteSize < 0) return false;

    qint64 nChunksOffset = N_ZOOM_HEADER_SIZE;
    if (nNoteSize != 0) {
        // The note block carries its own four-byte checksum after the text.
        if (nNoteSize > (baFile.size() - N_ZOOM_HEADER_SIZE - 4)) return false;
        nChunksOffset += nNoteSize + 4;
    }

    *pnFirst = nFirst;
    *pnLast = nLast;
    *pbProtected = ((quint8)baFile.at(0x24) != 0);
    *pnChunksOffset = nChunksOffset;

    return true;
}

}  // namespace

bool XZoomDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    const qint64 nSize = baPacked.size();
    if (nSize < 4) return false;

    const quint8 *pData = (const quint8 *)baPacked.constData();

    // BE24 length written by the packer.  It has to agree with the record's
    // final length exactly - a mismatch means the LZHUF stage handed over the
    // wrong bytes, not that this chunk is merely unusual.
    const qint64 nDeclared = ((qint64)pData[0] << 16) | ((qint64)pData[1] << 8) | (qint64)pData[2];
    if (nDeclared != nUncompressedSize) return false;

    const quint8 nEscape = pData[3];

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);

    qint64 nPosition = 4;
    qint64 nGuard = 0;

    while (nPosition < nSize) {
        ++nGuard;
        if ((nGuard & 0xFFF) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }

        const quint8 nByte = pData[nPosition];

        if (nByte != nEscape) {
            if (baOut.size() >= nUncompressedSize) return false;
            baOut.append((char)nByte);
            ++nPosition;
            continue;
        }

        if ((nPosition + 1) >= nSize) return false;

        const qint32 nCount = pData[nPosition + 1];

        if (nCount == 0) {
            // An escaped escape: one literal copy of the escape byte itself.
            if (baOut.size() >= nUncompressedSize) return false;
            baOut.append((char)nEscape);
            nPosition += 2;
        } else {
            if ((nPosition + 2) >= nSize) return false;
            const char nValue = (char)pData[nPosition + 2];
            // A run of n is n + 1 bytes out.
            if ((qint64)baOut.size() + (qint64)nCount + 1 > nUncompressedSize) return false;
            for (qint32 i = 0; i <= nCount; ++i) baOut.append(nValue);
            nPosition += 3;
        }
    }

    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}

bool XZoomDecoder::decodeImage(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize <= 0) || (nUncompressedSize > N_ZOOM_MAX_OUTPUT)) return false;

    quint8 nFirstCylinder = 0;
    quint8 nLastCylinder = 0;
    bool bProtected = false;
    qint64 nOffset = 0;
    if (!zoomParseHeader(baPacked, &nFirstCylinder, &nLastCylinder, &bProtected, &nOffset)) return false;
    if (bProtected) return false;

    const qint64 nCylinders = (qint64)nLastCylinder - (qint64)nFirstCylinder + 1;
    const qint64 nMaxOutput = nCylinders * N_ZOOM_CYLINDER_SIZE;
    if (nMaxOutput != nUncompressedSize) return false;

    const XLZHUFDecoder::OPTIONS options = XLZHUFDecoder::getZoomOptions();
    const uchar *pData = (const uchar *)baPacked.constData();
    const qint64 nSize = baPacked.size();

    QByteArray baOut;
    baOut.reserve((qint32)nMaxOutput);
    // The reference walks cylinders from zero regardless of firstCylinder.
    qint64 nCurrentCylinder = 0;

    while ((nOffset + N_ZOOM_CHUNK_HEADER_SIZE) <= nSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const uchar *pRecord = pData + nOffset;
        quint8 arrCylinders[N_ZOOM_SLOTS];
        quint32 arrMasks[N_ZOOM_SLOTS];
        for (qint32 i = 0; i < N_ZOOM_SLOTS; ++i) {
            arrCylinders[i] = pRecord[i];
            arrMasks[i] = qFromBigEndian<quint32>(pRecord + 6 + (i * 4));
        }
        const qint64 nPackedSize = (qint64)qFromBigEndian<quint16>(pRecord + 0x1a);
        const qint64 nMiddleSize = (qint64)qFromBigEndian<quint16>(pRecord + 0x1c);
        qint64 nFinalSize = (qint64)qFromBigEndian<quint16>(pRecord + 0x1e);
        const quint16 nFlag = qFromBigEndian<quint16>(pRecord + 0x20);
        nOffset += N_ZOOM_CHUNK_HEADER_SIZE;

        if (nPackedSize > (nSize - nOffset)) return false;
        const QByteArray baBlob = baPacked.mid((qint32)nOffset, (qint32)nPackedSize);
        if (baBlob.size() != nPackedSize) return false;
        nOffset += nPackedSize;

        qint64 nTotal = 0;
        for (qint32 i = 0; i < N_ZOOM_SLOTS; ++i) {
            if (arrCylinders[i] == N_ZOOM_SLOT_UNUSED) continue;
            nTotal += N_ZOOM_SECTOR_SIZE * (qint64)zoomPopCount(arrMasks[i]);
        }
        // A record with no RLE stage does not state its final length; the
        // stored sectors are the length.
        if (nMiddleSize == 0) nFinalSize = nTotal;

        QByteArray baBuffer = baBlob;
        if (nFlag != 0) {
            QByteArray baStage;
            if (!XLZHUFDecoder::decode(baBuffer, options, (nMiddleSize != 0) ? nMiddleSize : nFinalSize, &baStage, pPdStruct)) return false;
            baBuffer = baStage;
        }
        if (nMiddleSize != 0) {
            QByteArray baStage;
            if (!decode(baBuffer, nFinalSize, &baStage, pPdStruct)) return false;
            baBuffer = baStage;
        }

        if (nTotal < nFinalSize) nFinalSize = nTotal;
        if (baBuffer.size() < nFinalSize) return false;

        qint64 nSource = 0;
        qint64 nAvailable = nFinalSize;

        for (qint32 i = 0; i < N_ZOOM_SLOTS; ++i) {
            if (arrCylinders[i] == N_ZOOM_SLOT_UNUSED) continue;
            while (nCurrentCylinder < (qint64)arrCylinders[i]) {
                if (!zoomAppendZeroes(&baOut, N_ZOOM_CYLINDER_SIZE, nMaxOutput)) return false;
                ++nCurrentCylinder;
            }

            quint32 nMask = arrMasks[i];
            for (qint32 k = 0; k < N_ZOOM_SECTORS_PER_CYLINDER; ++k) {
                if (nMask & 1) {
                    if (nAvailable < N_ZOOM_SECTOR_SIZE) return false;
                    if (N_ZOOM_SECTOR_SIZE > (nMaxOutput - baOut.size())) return false;
                    baOut.append(baBuffer.constData() + nSource, (qint32)N_ZOOM_SECTOR_SIZE);
                    nSource += N_ZOOM_SECTOR_SIZE;
                    nAvailable -= N_ZOOM_SECTOR_SIZE;
                } else {
                    if (!zoomAppendZeroes(&baOut, N_ZOOM_SECTOR_SIZE, nMaxOutput)) return false;
                }
                nMask >>= 1;
            }
            ++nCurrentCylinder;
        }

        // Every stored sector of the record has to be consumed exactly.
        if (nAvailable != 0) return false;
    }

    while (nCurrentCylinder < nCylinders) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (!zoomAppendZeroes(&baOut, N_ZOOM_CYLINDER_SIZE, nMaxOutput)) return false;
        ++nCurrentCylinder;
    }

    if (baOut.size() != nUncompressedSize) return false;
    *pbaResult = baOut;

    return true;
}
