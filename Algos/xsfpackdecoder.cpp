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
#include "xsfpackdecoder.h"

#include <QtEndian>

namespace {
const qint32 SFPACK_LZW_TABLE = 0x1000;
const qint32 SFPACK_LZW_LAST = 0x0fff;
const qint32 SFPACK_MAX_BLOCK = 0x800;
const qint32 SFPACK_MAX_UNARY = 4096;
const qint64 SFPACK_MAX_CHUNK = 0x08000000;

// mode 1: refill one byte, hand out bits MSB first.  -1 on exhaustion.
class SfBitsByte {
public:
    SfBitsByte(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nCurrent(0), m_nCount(0)
    {
    }

    qint32 get(qint32 nWidth)
    {
        qint32 nValue = 0;
        for (qint32 i = 0; i < nWidth; ++i) {
            if (m_nCount == 0) {
                if (m_nPosition >= m_nSize) return -1;
                m_nCurrent = m_pData[m_nPosition];
                ++m_nPosition;
                m_nCount = 8;
            }
            nValue = (nValue << 1) | ((m_nCurrent >> 7) & 1);
            m_nCurrent = (quint8)((m_nCurrent << 1) & 0xff);
            --m_nCount;
        }

        return nValue;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint8 m_nCurrent;
    qint32 m_nCount;
};

// mode 4: refill a 32-bit LITTLE-ENDIAN word, hand out bits from bit31 down.
class SfBitsWord {
public:
    SfBitsWord(const quint8 *pData, qint64 nSize, qint64 nPosition) : m_pData(pData), m_nSize(nSize), m_nPosition(nPosition), m_nCurrent(0), m_nCount(0)
    {
    }

    // -1 means the stream is exhausted
    qint64 get(qint32 nWidth)
    {
        if (nWidth == 0) return 0;
        qint64 nValue = 0;
        for (qint32 i = 0; i < nWidth; ++i) {
            if (m_nCount == 0) {
                if ((m_nPosition < 0) || ((m_nPosition + 4) > m_nSize)) return -1;
                m_nCurrent = qFromLittleEndian<quint32>(m_pData + m_nPosition);
                m_nPosition += 4;
                m_nCount = 32;
            }
            nValue = (nValue << 1) | ((m_nCurrent >> 31) & 1);
            m_nCurrent = (quint32)(m_nCurrent << 1);
            --m_nCount;
        }

        return nValue;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint32 m_nCurrent;
    qint32 m_nCount;
};

// gamma / zigzag / uval on top of SfBitsWord.  Every reader returns -1 on a
// failure, which is why gamma() results are carried as qint64: a legitimate
// value can use all 32 bits.
class SfCoder {
public:
    explicit SfCoder(SfBitsWord *pBits) : m_pBits(pBits)
    {
    }

    qint64 gamma(qint32 nK)
    {
        qint32 nQ = 0;
        while (true) {
            const qint64 nBit = m_pBits->get(1);
            if (nBit < 0) return -1;
            if (nBit) break;
            ++nQ;
            // the reference has no bound here at all; this only stops a runaway
            // on a desynced stream and must stay loose, because a cold-start
            // residual with a small k routinely needs a q in the tens
            if (nQ > SFPACK_MAX_UNARY) return -1;
        }
        const qint64 nRest = m_pBits->get(nK);
        if (nRest < 0) return -1;
        const qint64 nValue = nRest | ((qint64)nQ << nK);
        if (nValue > 0xffffffffLL) return -1;

        return nValue;
    }

    // *pbOk distinguishes a legitimate negative result from a read failure
    qint32 sgamma(qint32 nK, bool *pbOk)
    {
        const qint64 nValue = gamma(nK + 1);
        if (nValue < 0) {
            *pbOk = false;
            return 0;
        }
        *pbOk = true;
        const quint32 nHalf = (quint32)(nValue >> 1);

        return (nValue & 1) ? (qint32)(~nHalf) : (qint32)nHalf;
    }

    qint64 uval()
    {
        const qint64 nK = gamma(2);
        if ((nK < 0) || (nK > 32)) return -1;

        return gamma((qint32)nK);
    }

private:
    SfBitsWord *m_pBits;
};

quint32 sfRead32(const QByteArray &baBuffer, qint64 nOffset)
{
    return qFromLittleEndian<quint32>((const uchar *)baBuffer.constData() + nOffset);
}

bool sfHasRange(const QByteArray &baBuffer, qint64 nOffset, qint64 nSize)
{
    return (nOffset >= 0) && (nSize >= 0) && (nOffset <= baBuffer.size()) && (nSize <= ((qint64)baBuffer.size() - nOffset));
}

}  // namespace

bool XSFPACKDecoder::decodeChunk(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > SFPACK_MAX_CHUNK)) return false;
    if (nUncompressedSize == 0) return true;

    SfBitsByte reader((const quint8 *)baPacked.constData(), baPacked.size());

    QByteArray baPrefix(SFPACK_LZW_TABLE * (qint32)sizeof(quint16), (char)0);
    QByteArray baSuffix(SFPACK_LZW_TABLE, (char)0);
    QByteArray baStack(SFPACK_LZW_TABLE, (char)0);
    quint16 *pPrefix = (quint16 *)baPrefix.data();
    quint8 *pSuffix = (quint8 *)baSuffix.data();
    quint8 *pStack = (quint8 *)baStack.data();

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);
    qint64 nLeft = nUncompressedSize;

    while (true) {  // one pass per full-table reset
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        memset(pPrefix, 0, SFPACK_LZW_TABLE * sizeof(quint16));
        memset(pSuffix, 0, SFPACK_LZW_TABLE);
        qint32 nWidth = 9;

        qint32 nCode = reader.get(9);
        if ((nCode < 0) || (nCode > 0xff)) return false;
        baOut.append((char)(quint8)nCode);
        --nLeft;
        qint32 nFirst = nCode;
        qint32 nPrevious = nCode;
        qint32 nFree = 0x100;

        bool bReset = false;
        while (!bReset) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            if (nLeft < 1) {
                if (nLeft != 0) return false;
                *pbaResult = baOut;
                return true;
            }
            if (nFree == SFPACK_LZW_LAST) {
                bReset = true;  // table full - start over from scratch
                break;
            }

            const qint32 nCurrent = reader.get(nWidth);
            if (nCurrent < 0) return false;

            qint32 nLast = 0;
            if (nCurrent < 0x100) {
                baOut.append((char)(quint8)nCurrent);
                --nLeft;
                nLast = nCurrent;
            } else {
                qint32 nStackSize = 0;
                qint32 c = nCurrent;
                if (c >= nFree) {
                    if (c > nFree) return false;
                    pStack[nStackSize++] = (quint8)nFirst;
                    c = nPrevious;
                }
                while (c > 0xff) {
                    if ((c >= SFPACK_LZW_TABLE) || (nStackSize >= SFPACK_LZW_TABLE)) return false;
                    pStack[nStackSize++] = pSuffix[c];
                    c = pPrefix[c];
                }
                if (nStackSize >= SFPACK_LZW_TABLE) return false;
                pStack[nStackSize++] = (quint8)c;
                nLast = c;
                nLeft -= nStackSize;
                for (qint32 i = nStackSize - 1; i >= 0; --i) baOut.append((char)pStack[i]);
            }

            pPrefix[nFree] = (quint16)nPrevious;
            pSuffix[nFree] = (quint8)nLast;
            // one code EARLY: the step happens when free+1 reaches the width
            const bool bGrow = ((nFree + 2) == (1 << nWidth));
            ++nFree;
            nFirst = nLast;
            nPrevious = nCurrent;
            if (bGrow) ++nWidth;
        }
    }
}

bool XSFPACKDecoder::decodeSample(const QByteArray &baFile, qint64 nOffset, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nOffset < 0) || (nOffset >= baFile.size())) return false;

    SfBitsWord bits((const quint8 *)baFile.constData(), baFile.size(), nOffset);
    SfCoder coder(&bits);

    const qint64 nBlockSize = coder.uval();
    if (nBlockSize < 0) return false;
    const qint64 nMaxOrder = coder.uval();
    if (nMaxOrder < 0) return false;
    if ((nBlockSize > SFPACK_MAX_BLOCK) || (nMaxOrder >= nBlockSize)) return false;
    const qint64 nDc = coder.uval();
    if (nDc < 0) return false;

    const qint32 nMax = (qint32)nBlockSize;
    const qint32 nOrderMax = (qint32)nMaxOrder;
    const qint32 nHistory = (nOrderMax >= 3) ? nOrderMax : 3;

    quint32 nAcc1 = 0;
    quint32 nAcc2 = (quint32)((quint32)nDc - 33000U);
    qint32 nIntMode = 0;
    qint32 nShift = 0;

    QVector<qint32> listBuffer(nHistory + nMax, 0);
    QVector<qint32> listCoefs((nOrderMax > 0) ? nOrderMax : 1, 0);
    qint32 *pBuffer = listBuffer.data();
    qint32 *pCoefs = listCoefs.data();

    qint32 nCount = nMax;
    QByteArray baOut;

    while (true) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const qint64 nCommand = coder.gamma(2);
        if (nCommand < 0) return false;
        if (nCommand == 9) {
            *pbaResult = baOut;
            return true;
        }

        if ((nCommand > 4) && (nCommand != 7)) {
            if (nCommand == 5) {
                const qint64 nValue = coder.gamma(0);
                if (nValue < 0) return false;
                nIntMode = (nValue > 0x7fffffff) ? 0x7fffffff : (qint32)nValue;
            } else if (nCommand == 6) {
                const qint64 nValue = coder.gamma(2);
                if ((nValue < 0) || (nValue > 31)) return false;
                nShift = (qint32)nValue;
            } else if (nCommand == 8) {
                const qint64 nValue = coder.uval();
                if ((nValue < 0) || (nValue > nMax)) return false;
                nCount = (qint32)nValue;
            } else {
                return false;
            }
            continue;
        }

        qint32 nK = 0;
        if (nCommand != 7) {
            const qint64 nValue = coder.gamma(3);
            if ((nValue < 0) || (nValue > 31)) return false;
            nK = (qint32)nValue;
        }

        bool bOk = true;
        if (nCommand == 0) {
            for (qint32 i = 0; i < nCount; ++i) {
                const qint32 nResidual = coder.sgamma(nK, &bOk);
                if (!bOk) return false;
                pBuffer[nHistory + i] = nResidual;
            }
        } else if (nCommand == 1) {
            for (qint32 i = 0; i < nCount; ++i) {
                const qint32 nResidual = coder.sgamma(nK, &bOk);
                if (!bOk) return false;
                pBuffer[nHistory + i] = (qint32)((quint32)nResidual + (quint32)pBuffer[nHistory + i - 1]);
            }
        } else if (nCommand == 2) {
            for (qint32 i = 0; i < nCount; ++i) {
                const qint32 nResidual = coder.sgamma(nK, &bOk);
                if (!bOk) return false;
                pBuffer[nHistory + i] =
                    (qint32)((quint32)nResidual + 2U * (quint32)pBuffer[nHistory + i - 1] - (quint32)pBuffer[nHistory + i - 2]);
            }
        } else if (nCommand == 3) {
            for (qint32 i = 0; i < nCount; ++i) {
                const qint32 nResidual = coder.sgamma(nK, &bOk);
                if (!bOk) return false;
                const quint32 nDelta = (quint32)pBuffer[nHistory + i - 1] - (quint32)pBuffer[nHistory + i - 2];
                pBuffer[nHistory + i] = (qint32)((quint32)nResidual + (quint32)pBuffer[nHistory + i - 3] + 3U * nDelta);
            }
        } else if (nCommand == 4) {
            const qint64 nOrderValue = coder.gamma(3);
            if ((nOrderValue < 0) || (nOrderValue > nMaxOrder)) return false;
            const qint32 nOrder = (qint32)nOrderValue;
            for (qint32 j = 0; j < nOrder; ++j) {
                const qint32 nCoef = coder.sgamma(5, &bOk);
                if (!bOk) return false;
                pCoefs[j] = nCoef;
            }
            for (qint32 i = 0; i < nCount; ++i) {
                quint32 nSum = 32;
                for (qint32 j = 0; j < nOrder; ++j) nSum += (quint32)pCoefs[j] * (quint32)pBuffer[nHistory + i - 1 - j];
                const qint32 nResidual = coder.sgamma(nK, &bOk);
                if (!bOk) return false;
                // arithmetic shift, so it floors
                pBuffer[nHistory + i] = (qint32)((quint32)nResidual + (quint32)((qint32)nSum >> 5));
            }
        } else if (nCommand == 7) {
            for (qint32 i = 0; i < nCount; ++i) pBuffer[nHistory + i] = 0;
        }

        // the predictor history is taken BEFORE the shift and the integration
        for (qint32 i = 0; i < nHistory; ++i) pBuffer[i] = pBuffer[i + nCount];
        if (nShift) {
            for (qint32 i = 0; i < nCount; ++i) pBuffer[nHistory + i] = (qint32)((quint32)pBuffer[nHistory + i] << nShift);
        }
        if (nIntMode == 0) {
            for (qint32 i = 0; i < nCount; ++i) {
                nAcc1 = (quint32)pBuffer[nHistory + i];
                nAcc2 = nAcc1 + nAcc2;
                pBuffer[nHistory + i] = (qint32)nAcc2;
            }
        } else if (nIntMode == 1) {
            for (qint32 i = 0; i < nCount; ++i) {
                nAcc1 = (quint32)pBuffer[nHistory + i] + nAcc1;
                nAcc2 = nAcc1 + nAcc2;
                pBuffer[nHistory + i] = (qint32)nAcc2;
            }
        }

        if ((baOut.size() + (qint64)nCount * 2) > MAX_UNCOMPRESSED_SIZE) return false;
        for (qint32 i = 0; i < nCount; ++i) {
            const quint16 nSample = (quint16)((quint32)pBuffer[nHistory + i] & 0xffff);
            baOut.append((char)(quint8)(nSample & 0xff));
            baOut.append((char)(quint8)((nSample >> 8) & 0xff));
        }
    }
}

bool XSFPACKDecoder::parseHeader(const QByteArray &baBuffer, qint64 nFileSize, HEADER *pHeader, qint64 *pnNeeded)
{
    if (pnNeeded) *pnNeeded = 0;
    if (!pHeader) return false;
    if ((nFileSize < 16) || (baBuffer.size() > nFileSize)) return false;
    if (!sfHasRange(baBuffer, 0, 16)) {
        if (pnNeeded) *pnNeeded = 16;
        return false;
    }
    if (baBuffer.left(4) != QByteArray("SFPK", 4)) return false;

    HEADER header = {};
    header.nFlags = qFromLittleEndian<quint16>((const uchar *)baBuffer.constData() + 6);
    if (header.nFlags & 4) return false;  // encrypted; the reference refuses it too
    header.nDeclaredSize = (qint32)sfRead32(baBuffer, 8);

    qint64 nPosition = 16;
    for (qint32 nChunk = 0; nChunk < 2; ++nChunk) {
        if (!sfHasRange(baBuffer, nPosition, 12)) {
            if (pnNeeded) *pnNeeded = nPosition + 12;
            return false;
        }
        const qint64 nA = (qint32)sfRead32(baBuffer, nPosition);
        const qint64 nUnpacked = (qint32)sfRead32(baBuffer, nPosition + 8);
        const char *pTag = (nChunk == 0) ? "INFO" : "pdta";
        if (baBuffer.mid((int)nPosition + 4, 4) != QByteArray(pTag, 4)) return false;
        if ((nA <= 8) || (nUnpacked <= 0) || (nUnpacked > SFPACK_MAX_CHUNK)) return false;
        const qint64 nNext = nPosition + 4 + nA;
        if ((nNext <= nPosition) || (nNext > nFileSize)) return false;
        if (nChunk == 0) {
            header.nInfoOffset = nPosition + 12;
            header.nInfoPackedSize = nA - 8;
            header.nInfoSize = nUnpacked;
        } else {
            header.nPdtaOffset = nPosition + 12;
            header.nPdtaPackedSize = nA - 8;
            header.nPdtaSize = nUnpacked;
        }
        nPosition = nNext;
    }

    if (!sfHasRange(baBuffer, nPosition, 4)) {
        if (pnNeeded) *pnNeeded = nPosition + 4;
        return false;
    }
    const qint64 nSkip = (qint32)sfRead32(baBuffer, nPosition);
    nPosition += 4;
    if (nSkip <= 0) return false;
    if (nSkip > (nFileSize - nPosition)) return false;
    nPosition += nSkip;

    if (!sfHasRange(baBuffer, nPosition, 4)) {
        if (pnNeeded) *pnNeeded = nPosition + 4;
        return false;
    }
    const qint64 nTableBytes = (qint32)sfRead32(baBuffer, nPosition);
    nPosition += 4;
    if ((nTableBytes <= 0) || (nTableBytes & 3)) return false;
    if (nTableBytes > (nFileSize - nPosition)) return false;
    if (!sfHasRange(baBuffer, nPosition, nTableBytes)) {
        if (pnNeeded) *pnNeeded = nPosition + nTableBytes;
        return false;
    }

    header.nTableOffset = nPosition;
    header.nSampleCount = (qint32)(nTableBytes >> 2);
    header.nStructureSize = nPosition + nTableBytes;

    for (qint32 i = 0; i < header.nSampleCount; ++i) {
        const qint64 nSampleOffset = (qint32)sfRead32(baBuffer, header.nTableOffset + (qint64)i * 4);
        if ((nSampleOffset < 1) || (nSampleOffset >= nFileSize)) return false;
    }

    *pHeader = header;

    return true;
}

namespace {
// walk the decompressed pdta looking for the shdr sub-chunk
qint64 sfFindShdr(const QByteArray &baPdta, qint32 nCount)
{
    qint64 nPosition = 0;
    while ((baPdta.size() - nPosition) >= 8) {
        const qint64 nSize = (qint32)qFromLittleEndian<quint32>((const uchar *)baPdta.constData() + nPosition + 4);
        if ((nSize < 0) || (nSize > ((qint64)baPdta.size() - nPosition - 8))) return -1;
        if (baPdta.mid((int)nPosition, 4) == QByteArray("shdr", 4)) {
            if (nSize != ((qint64)nCount + 1) * XSFPACKDecoder::SHDR_RECORD_SIZE) return -1;
            return nPosition + 8;
        }
        nPosition += 8 + nSize;
    }

    return -1;
}

bool sfSampleWords(const QByteArray &baPdta, qint64 nShdr, qint32 nCount, qint64 *pnWords)
{
    qint64 nWords = 0;
    const uchar *pData = (const uchar *)baPdta.constData();
    for (qint32 i = 0; i < nCount; ++i) {
        const qint64 nRecord = nShdr + (qint64)i * XSFPACKDecoder::SHDR_RECORD_SIZE;
        if ((nRecord < 0) || ((nRecord + XSFPACKDecoder::SHDR_RECORD_SIZE) > baPdta.size())) return false;
        const quint16 nType = qFromLittleEndian<quint16>(pData + nRecord + 0x2c);
        if (nType & 0x8000) continue;  // ROM sample: skipped entirely
        const qint64 nStart = (qint32)qFromLittleEndian<quint32>(pData + nRecord + 0x14);
        const qint64 nEnd = (qint32)qFromLittleEndian<quint32>(pData + nRecord + 0x18);
        nWords += (nEnd - nStart) + 23;
        if ((nWords < 0) || (nWords > (XSFPACKDecoder::MAX_UNCOMPRESSED_SIZE / 2))) return false;
    }
    *pnWords = nWords;

    return true;
}
}  // namespace

bool XSFPACKDecoder::measure(const QByteArray &baBuffer, const HEADER &header, qint64 *pnSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pnSize) return false;
    if (!sfHasRange(baBuffer, header.nPdtaOffset, header.nPdtaPackedSize)) return false;

    QByteArray baPdta;
    if (!decodeChunk(baBuffer.mid((int)header.nPdtaOffset, (int)header.nPdtaPackedSize), header.nPdtaSize, &baPdta, pPdStruct)) return false;
    if (baPdta.size() != header.nPdtaSize) return false;

    const qint64 nShdr = sfFindShdr(baPdta, header.nSampleCount);
    if (nShdr < 0) return false;
    qint64 nWords = 0;
    if (!sfSampleWords(baPdta, nShdr, header.nSampleCount, &nWords)) return false;

    const qint64 nSize = 56 + header.nInfoSize + 2 * nWords + header.nPdtaSize;
    if ((nSize < 0) || (nSize > MAX_UNCOMPRESSED_SIZE)) return false;
    *pnSize = nSize;

    return true;
}

bool XSFPACKDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();

    HEADER header = {};
    if (!parseHeader(baPacked, baPacked.size(), &header, nullptr)) return false;
    if (!sfHasRange(baPacked, header.nInfoOffset, header.nInfoPackedSize)) return false;
    if (!sfHasRange(baPacked, header.nPdtaOffset, header.nPdtaPackedSize)) return false;

    QByteArray baInfo;
    if (!decodeChunk(baPacked.mid((int)header.nInfoOffset, (int)header.nInfoPackedSize), header.nInfoSize, &baInfo, pPdStruct)) return false;
    if (baInfo.size() != header.nInfoSize) return false;

    QByteArray baPdta;
    if (!decodeChunk(baPacked.mid((int)header.nPdtaOffset, (int)header.nPdtaPackedSize), header.nPdtaSize, &baPdta, pPdStruct)) return false;
    if (baPdta.size() != header.nPdtaSize) return false;

    const qint64 nShdr = sfFindShdr(baPdta, header.nSampleCount);
    if (nShdr < 0) return false;
    qint64 nWords = 0;
    if (!sfSampleWords(baPdta, nShdr, header.nSampleCount, &nWords)) return false;

    QByteArray baSamples;
    uchar *pPdta = (uchar *)baPdta.data();
    for (qint32 i = 0; i < header.nSampleCount; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nRecord = nShdr + (qint64)i * SHDR_RECORD_SIZE;
        const quint16 nType = qFromLittleEndian<quint16>(pPdta + nRecord + 0x2c);
        if (nType & 0x8000) continue;

        const qint64 nSampleOffset = (qint32)sfRead32(baPacked, header.nTableOffset + (qint64)i * 4);
        QByteArray baSample;
        if (!decodeSample(baPacked, nSampleOffset, &baSample, pPdStruct)) return false;
        if ((baSamples.size() + (qint64)baSample.size() + SHDR_RECORD_SIZE) > MAX_UNCOMPRESSED_SIZE) return false;

        const qint64 nBase = baSamples.size();
        baSamples.append(baSample);
        baSamples.append(QByteArray(SHDR_RECORD_SIZE, (char)0));  // the 46 zero bytes SFPack appends

        const qint64 nOldStart = (qint32)qFromLittleEndian<quint32>(pPdta + nRecord + 0x14);
        const qint64 nDelta = (nBase / 2) - nOldStart;
        const qint32 arrFields[4] = {0x14, 0x18, 0x1c, 0x20};
        for (qint32 j = 0; j < 4; ++j) {
            const quint32 nValue = qFromLittleEndian<quint32>(pPdta + nRecord + arrFields[j]);
            qToLittleEndian<quint32>((quint32)((qint32)nValue + (qint32)nDelta), pPdta + nRecord + arrFields[j]);
        }
    }
    if (baSamples.size() != (nWords * 2)) return false;

    const qint64 nTotal = 56 + baInfo.size() + baSamples.size() + baPdta.size();
    if ((nTotal > MAX_UNCOMPRESSED_SIZE) || (nTotal > 0x7fffffffLL)) return false;

    QByteArray baOut;
    baOut.reserve((qint32)nTotal);
    char arrHead[4];

    baOut.append("RIFF", 4);
    qToLittleEndian<quint32>((quint32)(baInfo.size() + baSamples.size() + baPdta.size() + 0x30), (uchar *)arrHead);
    baOut.append(arrHead, 4);
    baOut.append("sfbk", 4);

    baOut.append("LIST", 4);
    qToLittleEndian<quint32>((quint32)(baInfo.size() + 4), (uchar *)arrHead);
    baOut.append(arrHead, 4);
    baOut.append("INFO", 4);
    baOut.append(baInfo);

    baOut.append("LIST", 4);
    qToLittleEndian<quint32>((quint32)(baSamples.size() + 0x0c), (uchar *)arrHead);
    baOut.append(arrHead, 4);
    baOut.append("sdta", 4);
    baOut.append("smpl", 4);
    qToLittleEndian<quint32>((quint32)baSamples.size(), (uchar *)arrHead);
    baOut.append(arrHead, 4);
    baOut.append(baSamples);

    baOut.append("LIST", 4);
    qToLittleEndian<quint32>((quint32)(baPdta.size() + 4), (uchar *)arrHead);
    baOut.append(arrHead, 4);
    baOut.append("pdta", 4);
    baOut.append(baPdta);

    if ((nUncompressedSize >= 0) && (baOut.size() != nUncompressedSize)) return false;
    *pbaResult = baOut;

    return true;
}
