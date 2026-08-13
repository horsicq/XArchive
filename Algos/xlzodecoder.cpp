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
#include "xlzodecoder.h"
#include "algo_utils.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string.h>

XLZODecoder::XLZODecoder(QObject *parent) : QObject(parent)
{
}

namespace {
const quint32 LZOP_F_ADLER32_D = 0x00000001U;
const quint32 LZOP_F_ADLER32_C = 0x00000002U;
const quint32 LZOP_F_H_EXTRA_FIELD = 0x00000040U;
const quint32 LZOP_F_CRC32_D = 0x00000100U;
const quint32 LZOP_F_CRC32_C = 0x00000200U;
const quint32 LZOP_F_MULTIPART = 0x00000400U;
const quint32 LZOP_F_H_FILTER = 0x00000800U;
const quint32 LZOP_F_H_CRC32 = 0x00001000U;
const quint32 LZOP_F_MASK = 0x00003FFFU;
const quint32 LZOP_F_OS_MASK = 0xFF000000U;
const quint32 LZOP_F_CS_MASK = 0x00F00000U;
const quint32 LZOP_MAX_BLOCK_SIZE = 64U * 1024U * 1024U;
const quint16 LZOP_MIN_VERSION = 0x0900U;
const quint16 LZOP_SUPPORTED_VERSION = 0x1040U;

quint32 lzoAdler32Update(quint32 nAdler, const char *pData, qint32 nSize)
{
    const quint32 nMod = 65521U;
    quint32 nA = nAdler & 0xFFFFU;
    quint32 nB = nAdler >> 16;

    while (nSize > 0) {
        const qint32 nChunk = (std::min)(nSize, (qint32)5552);
        nSize -= nChunk;
        for (qint32 i = 0; i < nChunk; i++) {
            nA += (quint8)pData[i];
            nB += nA;
        }
        nA %= nMod;
        nB %= nMod;
        pData += nChunk;
    }

    return (nB << 16) | nA;
}

struct LZOP_CHECKSUMS {
    quint32 nAdler32 = 1U;
    quint32 nCRC32 = 0xFFFFFFFFU;

    void update(const char *pData, qint32 nSize)
    {
        if (!pData || (nSize <= 0)) return;
        nAdler32 = lzoAdler32Update(nAdler32, pData, nSize);
        nCRC32 = XBinary::_getCRC32(pData, nSize, nCRC32, XBinary::_getCRC32Table_EDB88320());
    }

    quint32 crc32() const { return nCRC32 ^ 0xFFFFFFFFU; }
};

bool lzoReadExact(XBinary::DATAPROCESS_STATE *pState, char *pData, qint32 nSize,
                  XBinary::PDSTRUCT *pPdStruct, LZOP_CHECKSUMS *pChecksums = nullptr)
{
    if (!pState || (nSize < 0) || ((nSize > 0) && !pData)) {
        if (pState) pState->bReadError = true;
        return false;
    }

    qint32 nDone = 0;
    while ((nDone < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nRead = XBinary::_readDevice(pData + nDone, nSize - nDone, pState);
        if (nRead <= 0) {
            if (XBinary::isPdStructNotCanceled(pPdStruct)) pState->bReadError = true;
            return false;
        }
        if (pChecksums) pChecksums->update(pData + nDone, nRead);
        nDone += nRead;
    }

    return (nDone == nSize) && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool lzoReadBE16(XBinary::DATAPROCESS_STATE *pState, quint16 *pValue, XBinary::PDSTRUCT *pPdStruct,
                 LZOP_CHECKSUMS *pChecksums = nullptr)
{
    quint8 aData[2] = {};
    if (!pValue || !lzoReadExact(pState, (char *)aData, sizeof(aData), pPdStruct, pChecksums)) return false;
    *pValue = ((quint16)aData[0] << 8) | (quint16)aData[1];
    return true;
}

bool lzoReadBE32(XBinary::DATAPROCESS_STATE *pState, quint32 *pValue, XBinary::PDSTRUCT *pPdStruct,
                 LZOP_CHECKSUMS *pChecksums = nullptr)
{
    quint8 aData[4] = {};
    if (!pValue || !lzoReadExact(pState, (char *)aData, sizeof(aData), pPdStruct, pChecksums)) return false;
    *pValue = ((quint32)aData[0] << 24) | ((quint32)aData[1] << 16) | ((quint32)aData[2] << 8) | (quint32)aData[3];
    return true;
}

bool lzoWriteState(XBinary::DATAPROCESS_STATE *pState, const char *pData, qint64 nSize,
                   XBinary::PDSTRUCT *pPdStruct)
{
    if (!pState || (nSize < 0) || ((nSize > 0) && !pData)) {
        if (pState) pState->bWriteError = true;
        return false;
    }

    qint64 nDone = 0;
    while ((nDone < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nChunk = (qint32)(std::min)(nSize - nDone, (qint64)0x10000);
        if (XBinary::_writeDevice(pData + nDone, nChunk, pState) != nChunk) return false;
        nDone += nChunk;
    }

    return (nDone == nSize) && !pState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool lzoConsumeExtraField(XBinary::DATAPROCESS_STATE *pState, bool bCRC32,
                          XBinary::PDSTRUCT *pPdStruct)
{
    LZOP_CHECKSUMS checksums;
    quint32 nLength = 0;
    if (!lzoReadBE32(pState, &nLength, pPdStruct, &checksums)) return false;

    QByteArray baBuffer(0x10000, 0);
    quint64 nRemaining = nLength;
    while ((nRemaining > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nChunk = (qint32)(std::min)(nRemaining, (quint64)baBuffer.size());
        if (!lzoReadExact(pState, baBuffer.data(), nChunk, pPdStruct, &checksums)) return false;
        nRemaining -= (quint32)nChunk;
    }

    quint32 nExpected = 0;
    return (nRemaining == 0) && lzoReadBE32(pState, &nExpected, pPdStruct) &&
           (nExpected == (bCRC32 ? checksums.crc32() : checksums.nAdler32));
}

bool lzoIsExactInputEnd(XBinary::DATAPROCESS_STATE *pState)
{
    if (!pState) return false;
    if (pState->nInputLimit != -1) return pState->nCountInput == pState->nInputLimit;

    char cExtra = 0;
    const qint32 nRead = XBinary::_readDevice(&cExtra, 1, pState);
    return (nRead == 0) && !pState->bReadError;
}
}  // namespace

// LZO1X decompressor - compatible with lzop/minilzo output
// Based on the public LZO1X decompression algorithm
#define LZO_M2_MAX_OFFSET 0x0800

static quint64 lzoBytesAvailable(const quint8 *pCurrent, const quint8 *pEnd)
{
    return (quint64)(pEnd - pCurrent);
}

bool XLZODecoder::decompressBlock(const quint8 *pInput, qint64 nInputSize, quint8 *pOutput, qint64 nOutputSize, qint64 *pnBytesWritten)
{
    if (pnBytesWritten) *pnBytesWritten = 0;
    const quint64 nPointerLimit = (quint64)(std::numeric_limits<std::ptrdiff_t>::max)();
    if (!pInput || !pOutput || nInputSize <= 0 || nOutputSize <= 0 ||
        ((quint64)nInputSize > nPointerLimit) || ((quint64)nOutputSize > nPointerLimit)) {
        return false;
    }

    const quint8 *ip = pInput;
    const quint8 *ip_end = pInput + nInputSize;
    quint8 *op = pOutput;
    quint8 *op_end = pOutput + nOutputSize;

    quint64 t = 0;

    if (*ip > 17) {
        t = *ip++ - 17;
        if (t < 4) {
            goto match_next;
        }
        if ((t > lzoBytesAvailable(op, op_end)) || (t > lzoBytesAvailable(ip, ip_end))) return false;
        memcpy(op, ip, (size_t)t);
        op += (qint64)t;
        ip += (qint64)t;
        goto first_literal_run;
    }

    for (;;) {
        if (ip >= ip_end) return false;
        t = *ip++;

        if (t >= 16) {
            goto match;
        }

        // Literal run (state 0)
        if (t == 0) {
            while (ip < ip_end && *ip == 0) {
                if (t > ((std::numeric_limits<quint64>::max)() - 255U)) return false;
                t += 255;
                ip++;
            }
            if (ip >= ip_end) return false;
            if (t > ((std::numeric_limits<quint64>::max)() - 15U - *ip)) return false;
            t += 15 + *ip++;
        }
        // Copy (t + 3) literal bytes
        {
            if (t > ((std::numeric_limits<quint64>::max)() - 3U)) return false;
            const quint64 nLitLen = t + 3;
            if ((nLitLen > lzoBytesAvailable(op, op_end)) || (nLitLen > lzoBytesAvailable(ip, ip_end))) return false;
            memcpy(op, ip, (size_t)nLitLen);
            op += (qint64)nLitLen;
            ip += (qint64)nLitLen;
        }

    first_literal_run:
        if (ip >= ip_end) return false;
        t = *ip++;

        if (t >= 16) {
            goto match;
        }

        // M1 match: distance = 1 + 0x0800 + (t>>2) + (*ip << 2), length = 3
        {
            if (ip >= ip_end) return false;
            quint32 nDist = 1 + LZO_M2_MAX_OFFSET + (quint32)(t >> 2) + ((quint32)*ip++ << 2);
            if ((qint64)(op - pOutput) < (qint64)nDist || lzoBytesAvailable(op, op_end) < 3) return false;
            const quint8 *pRef = op - nDist;
            *op++ = *pRef++;
            *op++ = *pRef++;
            *op++ = *pRef;
        }
        goto match_done;

        for (;;) {
        match:
            if (t >= 64) {
                // M2 match: distance = 1 + ((t>>2)&7) + (*ip<<3), length = (t>>5)+1 + 2
                if (ip >= ip_end) return false;
                quint32 nDist = 1 + (quint32)((t >> 2) & 7) + ((quint32)*ip++ << 3);
                quint64 nLen = (t >> 5) - 1 + 2;  // -1 because we copy 2 initial + t loop
                if ((qint64)(op - pOutput) < (qint64)nDist || nLen > lzoBytesAvailable(op, op_end)) return false;
                const quint8 *pRef = op - nDist;
                *op++ = *pRef++;
                *op++ = *pRef++;
                for (quint64 i = 0; i < nLen - 2; i++) {
                    *op++ = *pRef++;
                }
            } else if (t >= 32) {
                // M3 match: variable length, distance from next 2 bytes
                quint64 nLen = t & 31;
                if (nLen == 0) {
                    while (ip < ip_end && *ip == 0) {
                        if (nLen > ((std::numeric_limits<quint64>::max)() - 255U)) return false;
                        nLen += 255;
                        ip++;
                    }
                    if (ip >= ip_end) return false;
                    if (nLen > ((std::numeric_limits<quint64>::max)() - 31U - *ip)) return false;
                    nLen += 31 + *ip++;
                }
                if (nLen > ((std::numeric_limits<quint64>::max)() - 2U)) return false;
                nLen += 2;
                if (lzoBytesAvailable(ip, ip_end) < 2) return false;
                quint32 nDist = 1 + ((ip[0] >> 2) + ((quint32)ip[1] << 6));
                ip += 2;
                if ((qint64)(op - pOutput) < (qint64)nDist || nLen > lzoBytesAvailable(op, op_end)) return false;
                const quint8 *pRef = op - nDist;
                for (quint64 i = 0; i < nLen; i++) {
                    *op++ = *pRef++;
                }
            } else if (t >= 16) {
                // M4 match: variable length, large distance
                quint32 nDistHi = (quint32)(t & 8) << 11;  // 0 or 0x4000
                quint64 nLen = t & 7;
                if (nLen == 0) {
                    while (ip < ip_end && *ip == 0) {
                        if (nLen > ((std::numeric_limits<quint64>::max)() - 255U)) return false;
                        nLen += 255;
                        ip++;
                    }
                    if (ip >= ip_end) return false;
                    if (nLen > ((std::numeric_limits<quint64>::max)() - 7U - *ip)) return false;
                    nLen += 7 + *ip++;
                }
                if (nLen > ((std::numeric_limits<quint64>::max)() - 2U)) return false;
                nLen += 2;
                if (lzoBytesAvailable(ip, ip_end) < 2) return false;
                quint32 nDistLo = (ip[0] >> 2) + ((quint32)ip[1] << 6);
                ip += 2;
                quint32 nDist = nDistHi + nDistLo;
                if (nDist == 0) {
                    goto eof_found;
                }
                nDist += 0x4000;
                if ((qint64)(op - pOutput) < (qint64)nDist || nLen > lzoBytesAvailable(op, op_end)) return false;
                const quint8 *pRef = op - nDist;
                for (quint64 i = 0; i < nLen; i++) {
                    *op++ = *pRef++;
                }
            } else {
                // t < 16: shouldn't reach here in match context
                return false;
            }

        match_done:
            if ((ip - pInput) < 2) return false;
            t = ip[-2] & 3;

            if (t == 0) {
                break;
            }

        match_next:
            if ((t < 1) || (t > 3) || (t > lzoBytesAvailable(op, op_end)) || ((t + 1) > lzoBytesAvailable(ip, ip_end))) return false;
            *op++ = *ip++;
            if (t > 1) {
                *op++ = *ip++;
                if (t > 2) {
                    *op++ = *ip++;
                }
            }
            t = *ip++;
        }
    }

eof_found:
    if (ip != ip_end) return false;
    if (pnBytesWritten) {
        *pnBytesWritten = op - pOutput;
    }

    return true;
}

#undef LZO_M2_MAX_OFFSET

// LZOP container format decompressor
// Format: magic(9) + version info + flags + mode + mtime + ... + blocks
// Each block: uncompressed_size(4) + compressed_size(4) + [checksums] + data
bool XLZODecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    const qint64 nMax = (std::numeric_limits<qint64>::max)();
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput ||
        (pDecompressState->nInputOffset < 0) || (pDecompressState->nInputLimit < -1) ||
        ((pDecompressState->nInputLimit != -1) &&
         (pDecompressState->nInputOffset > (nMax - pDecompressState->nInputLimit))) ||
        (pDecompressState->nProcessedOffset < 0) || (pDecompressState->nProcessedLimit < -1) ||
        ((pDecompressState->nProcessedLimit != -1) &&
         (pDecompressState->nProcessedOffset > (nMax - pDecompressState->nProcessedLimit))) ||
        (XBinary::getBufferSize(pPdStruct) <= 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError) return false;

    // LZOP magic: 89 4C 5A 4F 00 0D 0A 1A 0A (9 bytes)
    const quint8 nExpectedMagic[] = {0x89, 0x4C, 0x5A, 0x4F, 0x00, 0x0D, 0x0A, 0x1A, 0x0A};
    quint8 aMagic[sizeof(nExpectedMagic)] = {};
    if (!lzoReadExact(pDecompressState, (char *)aMagic, sizeof(aMagic), pPdStruct) ||
        (memcmp(aMagic, nExpectedMagic, sizeof(aMagic)) != 0)) {
        return false;
    }

    // Read header fields (big-endian)
    LZOP_CHECKSUMS headerChecksums;
    quint16 nVersion = 0;
    quint16 nLibVersion = 0;
    quint16 nExtractVersion = 0;
    if (!lzoReadBE16(pDecompressState, &nVersion, pPdStruct, &headerChecksums) ||
        !lzoReadBE16(pDecompressState, &nLibVersion, pPdStruct, &headerChecksums) ||
        (nVersion < LZOP_MIN_VERSION) || (nVersion > LZOP_SUPPORTED_VERSION)) {
        return false;
    }

    if (nVersion >= 0x0940) {
        if (!lzoReadBE16(pDecompressState, &nExtractVersion, pPdStruct, &headerChecksums) ||
            (nExtractVersion < LZOP_MIN_VERSION) || (nExtractVersion > LZOP_SUPPORTED_VERSION)) {
            return false;
        }
    }

    quint8 nMethod = 0;
    quint8 nLevel = 0;
    if (!lzoReadExact(pDecompressState, (char *)&nMethod, 1, pPdStruct, &headerChecksums) ||
        ((nMethod != 1) && (nMethod != 2) && (nMethod != 3))) {
        return false;
    }
    if ((nVersion >= 0x0940) &&
        !lzoReadExact(pDecompressState, (char *)&nLevel, 1, pPdStruct, &headerChecksums)) return false;

    quint32 nFlags = 0;
    if (!lzoReadBE32(pDecompressState, &nFlags, pPdStruct, &headerChecksums) ||
        (nFlags & ~(LZOP_F_MASK | LZOP_F_OS_MASK | LZOP_F_CS_MASK)) ||
        (nFlags & (LZOP_F_H_FILTER | LZOP_F_MULTIPART))) {
        // Filtered and multipart LZOP streams require transformations/framing
        // this decoder does not implement; accepting them would fabricate data.
        return false;
    }

    quint32 nMode = 0;
    quint32 nMTimeLow = 0;
    quint32 nMTimeHigh = 0;
    if (!lzoReadBE32(pDecompressState, &nMode, pPdStruct, &headerChecksums) ||
        !lzoReadBE32(pDecompressState, &nMTimeLow, pPdStruct, &headerChecksums)) return false;
    if (nVersion >= 0x0940) {
        if (!lzoReadBE32(pDecompressState, &nMTimeHigh, pPdStruct, &headerChecksums)) return false;
    }

    // Original file name
    quint8 nNameLen = 0;
    if (!lzoReadExact(pDecompressState, (char *)&nNameLen, 1, pPdStruct, &headerChecksums)) return false;
    if (nNameLen > 0) {
        char aName[255] = {};
        if (!lzoReadExact(pDecompressState, aName, nNameLen, pPdStruct, &headerChecksums)) return false;
    }

    // Header checksum
    quint32 nExpectedHeaderChecksum = 0;
    if (!lzoReadBE32(pDecompressState, &nExpectedHeaderChecksum, pPdStruct) ||
        (nExpectedHeaderChecksum != ((nFlags & LZOP_F_H_CRC32) ? headerChecksums.crc32() : headerChecksums.nAdler32))) {
        return false;
    }
    if ((nFlags & LZOP_F_H_EXTRA_FIELD) &&
        !lzoConsumeExtraField(pDecompressState, (nFlags & LZOP_F_H_CRC32) != 0, pPdStruct)) return false;

    Q_UNUSED(nLibVersion)
    Q_UNUSED(nLevel)
    Q_UNUSED(nMode)
    Q_UNUSED(nMTimeLow)
    Q_UNUSED(nMTimeHigh)

    const bool bHasAdler32Uncompressed = (nFlags & LZOP_F_ADLER32_D) != 0;
    const bool bHasCrc32Uncompressed = (nFlags & LZOP_F_CRC32_D) != 0;
    const bool bHasAdler32Compressed = (nFlags & LZOP_F_ADLER32_C) != 0;
    const bool bHasCrc32Compressed = (nFlags & LZOP_F_CRC32_C) != 0;
    const bool bHasExpectedOutput = pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE);
    const qint64 nExpectedOutput = bHasExpectedOutput
                                      ? pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong()
                                      : -1;
    if (bHasExpectedOutput && (nExpectedOutput < 0)) return false;

    qint64 nTotalOutput = 0;

    // Process blocks
    for (;;) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        quint32 nUncompressedBlockSize = 0;
        if (!lzoReadBE32(pDecompressState, &nUncompressedBlockSize, pPdStruct)) return false;

        if (nUncompressedBlockSize == 0) {
            break;
        }

        if ((nUncompressedBlockSize > LZOP_MAX_BLOCK_SIZE) ||
            (nTotalOutput > (nMax - nUncompressedBlockSize)) ||
            (bHasExpectedOutput && ((nTotalOutput > nExpectedOutput) ||
                                    ((qint64)nUncompressedBlockSize > (nExpectedOutput - nTotalOutput))))) return false;

        quint32 nCompressedBlockSize = 0;
        if (!lzoReadBE32(pDecompressState, &nCompressedBlockSize, pPdStruct) ||
            (nCompressedBlockSize == 0) || (nCompressedBlockSize > nUncompressedBlockSize)) return false;

        quint32 nExpectedAdler32Uncompressed = 0;
        quint32 nExpectedCrc32Uncompressed = 0;
        quint32 nExpectedAdler32Compressed = 0;
        quint32 nExpectedCrc32Compressed = 0;
        if (bHasAdler32Uncompressed &&
            !lzoReadBE32(pDecompressState, &nExpectedAdler32Uncompressed, pPdStruct)) return false;
        if (bHasCrc32Uncompressed &&
            !lzoReadBE32(pDecompressState, &nExpectedCrc32Uncompressed, pPdStruct)) return false;
        if (nCompressedBlockSize < nUncompressedBlockSize) {
            if (bHasAdler32Compressed &&
                !lzoReadBE32(pDecompressState, &nExpectedAdler32Compressed, pPdStruct)) return false;
            if (bHasCrc32Compressed &&
                !lzoReadBE32(pDecompressState, &nExpectedCrc32Compressed, pPdStruct)) return false;
        }

        QByteArray baCompressed((qint32)nCompressedBlockSize, 0);
        if (!lzoReadExact(pDecompressState, baCompressed.data(), baCompressed.size(), pPdStruct)) return false;

        LZOP_CHECKSUMS compressedChecksums;
        compressedChecksums.update(baCompressed.constData(), baCompressed.size());
        if ((nCompressedBlockSize < nUncompressedBlockSize) &&
            ((bHasAdler32Compressed && (compressedChecksums.nAdler32 != nExpectedAdler32Compressed)) ||
             (bHasCrc32Compressed && (compressedChecksums.crc32() != nExpectedCrc32Compressed)))) return false;

        QByteArray baUncompressed;

        if (nCompressedBlockSize == nUncompressedBlockSize) {
            baUncompressed = baCompressed;
        } else {
            baUncompressed.resize((qint32)nUncompressedBlockSize);
            qint64 nBytesWritten = 0;

            if (!decompressBlock((const quint8 *)baCompressed.constData(), nCompressedBlockSize,
                                 (quint8 *)baUncompressed.data(), nUncompressedBlockSize, &nBytesWritten) ||
                (nBytesWritten != (qint64)nUncompressedBlockSize)) return false;
        }

        LZOP_CHECKSUMS uncompressedChecksums;
        uncompressedChecksums.update(baUncompressed.constData(), baUncompressed.size());
        if ((bHasAdler32Uncompressed && (uncompressedChecksums.nAdler32 != nExpectedAdler32Uncompressed)) ||
            (bHasCrc32Uncompressed && (uncompressedChecksums.crc32() != nExpectedCrc32Uncompressed)) ||
            !lzoWriteState(pDecompressState, baUncompressed.constData(), baUncompressed.size(), pPdStruct)) return false;

        nTotalOutput += nUncompressedBlockSize;
    }

    return lzoIsExactInputEnd(pDecompressState) && !pDecompressState->bReadError && !pDecompressState->bWriteError &&
           XBinary::isPdStructNotCanceled(pPdStruct) && (pDecompressState->nCountOutput == nTotalOutput) &&
           (!bHasExpectedOutput || (nTotalOutput == nExpectedOutput));
}
