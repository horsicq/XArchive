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
#include "xtopspeeddecoder.h"

#include <QtEndian>

namespace {
const qint32 TS_HEADER_SIZE = 6;
const qint32 TS_MAX_UNCOMPRESSED = 0x3800;
const qint32 TS_MAX_COMPRESSED = 0x4600;
const qint32 TS_DICT_SIZE = 0x1000;
const qint32 TS_DICT_FIRST = 0x100;
const qint32 TS_DICT_LAST = 0xfff;

// Codes are packed two per three bytes with both high nibbles in the leading
// byte, so the reader holds that nibble byte across a pair of codes.
class CodeReader {
public:
    CodeReader(const quint8 *pData, qint32 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nHigh(0), m_nPair(0)
    {
    }

    // -1 on exhaustion
    qint32 read()
    {
        if (m_nPair == 0) {
            if (m_nPosition >= m_nSize) return -1;
            m_nHigh = m_pData[m_nPosition];
            ++m_nPosition;
            m_nPair = 2;
        }
        if (m_nPosition >= m_nSize) return -1;
        const qint32 nCode = (qint32)(((quint32)(m_nHigh & 0xf0) << 4) | m_pData[m_nPosition]);
        ++m_nPosition;
        m_nHigh = (quint8)(m_nHigh << 4);
        --m_nPair;
        return nCode;
    }

private:
    const quint8 *m_pData;
    qint32 m_nSize;
    qint32 m_nPosition;
    quint8 m_nHigh;
    qint32 m_nPair;
};

bool decodeBlock(const quint8 *pData, qint32 nPackedSize, qint32 nPlainSize, QByteArray *pbaOut)
{
    QVector<quint16> listPrefix(TS_DICT_SIZE + 1, 0);
    QVector<quint8> listSuffix(TS_DICT_SIZE + 1, 0);
    QVector<quint8> listStack(TS_DICT_SIZE + 1, 0);

    CodeReader reader(pData, nPackedSize);
    qint32 nCode = reader.read();
    if (nCode < 0) return false;

    qint32 nProduced = 0;
    const qint32 nStart = pbaOut->size();

    while (true) {
        // a restart, and the very first code of the block, always emit a
        // literal and re-seed the dictionary
        qint32 nNext = TS_DICT_FIRST;
        listPrefix[TS_DICT_FIRST] = (quint16)nCode;
        pbaOut->append((char)(quint8)(nCode & 0xff));
        ++nProduced;
        qint32 nPreviousFirst = nCode & 0xff;

        bool bRestart = false;
        while (nProduced < nPlainSize) {
            nCode = reader.read();
            if (nCode < 0) return false;

            if (nNext == TS_DICT_LAST) {
                bRestart = true;
                break;
            }
            listPrefix[nNext + 1] = (quint16)nCode;

            qint32 nFirst = 0;
            if (nCode < TS_DICT_FIRST) {
                pbaOut->append((char)(quint8)nCode);
                ++nProduced;
                listSuffix[nNext] = (quint8)nCode;
                nFirst = nCode;
            } else {
                if (nCode > nNext) return false;
                qint32 nStackTop = 0;
                qint32 nWalk = nCode;
                if (nCode == nNext) {
                    listStack[nStackTop] = (quint8)nPreviousFirst;
                    ++nStackTop;
                    nWalk = (qint32)listPrefix[nNext];
                }
                while (nWalk > 0xff) {
                    if ((nStackTop >= listStack.size()) || (nWalk > TS_DICT_LAST)) return false;
                    listStack[nStackTop] = listSuffix[nWalk];
                    ++nStackTop;
                    nWalk = (qint32)listPrefix[nWalk];
                }
                nFirst = nWalk;
                pbaOut->append((char)(quint8)nWalk);
                ++nProduced;
                listSuffix[nNext] = (quint8)nWalk;
                while (nStackTop > 0) {
                    --nStackTop;
                    pbaOut->append((char)listStack[nStackTop]);
                    ++nProduced;
                }
            }
            ++nNext;
            nPreviousFirst = nFirst;
        }

        if (!bRestart) break;
    }

    return (pbaOut->size() - nStart) == nPlainSize;
}
}  // namespace

bool XTopSpeedDecoder::measure(const QByteArray &baFile, qint64 nFileSize, qint64 *pnUncompressedSize)
{
    if (!pnUncompressedSize || (nFileSize < TS_HEADER_SIZE) || (baFile.size() != nFileSize)) return false;

    const quint8 *pData = (const quint8 *)baFile.constData();
    qint64 nPosition = 0;
    qint64 nTotal = 0;
    qint32 nBlocks = 0;

    while (nPosition < nFileSize) {
        if ((nFileSize - nPosition) < TS_HEADER_SIZE) return false;
        const quint32 nChecksum = qFromLittleEndian<quint16>(pData + nPosition);
        const qint32 nPlainSize = (qint32)qFromLittleEndian<quint16>(pData + nPosition + 2);
        const qint32 nPackedSize = (qint32)qFromLittleEndian<quint16>(pData + nPosition + 4);
        if ((nPlainSize == 0) || (nPackedSize == 0)) return false;
        if (nPlainSize > TS_MAX_UNCOMPRESSED) return false;
        // the compressed cap applies ONLY to a real compressed block; a stored
        // block's "compressed" field is hypothetical and legitimately larger
        if ((nPackedSize < nPlainSize) && (nPackedSize > TS_MAX_COMPRESSED)) return false;

        const qint32 nOnDisk = qMin(nPlainSize, nPackedSize);
        if ((nFileSize - nPosition - TS_HEADER_SIZE) < nOnDisk) return false;

        quint32 nSum = 0;
        const quint8 *pPayload = pData + nPosition + TS_HEADER_SIZE;
        for (qint32 i = 0; i < nOnDisk; ++i) nSum += pPayload[i];
        if ((nSum & 0xffff) != nChecksum) return false;

        nTotal += nPlainSize;
        nPosition += TS_HEADER_SIZE + nOnDisk;
        ++nBlocks;
        if (nBlocks > 100000) return false;
    }

    if ((nPosition != nFileSize) || (nBlocks == 0) || (nTotal <= 0)) return false;

    *pnUncompressedSize = nTotal;

    return true;
}

bool XTopSpeedDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize <= 0) || (nUncompressedSize > 0x7fffffff)) return false;

    qint64 nMeasured = 0;
    if (!measure(baPacked, baPacked.size(), &nMeasured)) return false;
    if (nMeasured != nUncompressedSize) return false;

    const quint8 *pData = (const quint8 *)baPacked.constData();
    const qint64 nFileSize = baPacked.size();
    qint64 nPosition = 0;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);

    while (nPosition < nFileSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const qint32 nPlainSize = (qint32)qFromLittleEndian<quint16>(pData + nPosition + 2);
        const qint32 nPackedSize = (qint32)qFromLittleEndian<quint16>(pData + nPosition + 4);
        const qint32 nOnDisk = qMin(nPlainSize, nPackedSize);
        const quint8 *pPayload = pData + nPosition + TS_HEADER_SIZE;

        if (nPackedSize >= nPlainSize) {
            baOut.append((const char *)pPayload, nPlainSize);
        } else {
            if (!decodeBlock(pPayload, nPackedSize, nPlainSize, &baOut)) return false;
        }
        nPosition += TS_HEADER_SIZE + nOnDisk;
    }

    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
