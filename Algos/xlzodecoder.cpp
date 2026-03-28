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
#include <string.h>

XLZODecoder::XLZODecoder(QObject *parent) : QObject(parent)
{
}

// LZO1X decompressor - compatible with lzop/minilzo output
// Based on the public LZO1X decompression algorithm
#define LZO_M2_MAX_OFFSET 0x0800

bool XLZODecoder::decompressBlock(const quint8 *pInput, qint64 nInputSize, quint8 *pOutput, qint64 nOutputSize, qint64 *pnBytesWritten)
{
    if (!pInput || !pOutput || nInputSize <= 0 || nOutputSize <= 0) {
        return false;
    }

    const quint8 *ip = pInput;
    const quint8 *ip_end = pInput + nInputSize;
    quint8 *op = pOutput;
    quint8 *op_end = pOutput + nOutputSize;

    quint32 t = 0;

    if (*ip > 17) {
        t = *ip++ - 17;
        if (t < 4) {
            goto match_next;
        }
        if (op + t > op_end || ip + t > ip_end) return false;
        memcpy(op, ip, t);
        op += t;
        ip += t;
        goto first_literal_run;
    }

    for (;;) {
        t = *ip++;

        if (t >= 16) {
            goto match;
        }

        // Literal run (state 0)
        if (t == 0) {
            while (ip < ip_end && *ip == 0) {
                t += 255;
                ip++;
            }
            if (ip >= ip_end) return false;
            t += 15 + *ip++;
        }
        // Copy (t + 3) literal bytes
        {
            quint32 nLitLen = t + 3;
            if (op + nLitLen > op_end || ip + nLitLen > ip_end) return false;
            memcpy(op, ip, nLitLen);
            op += nLitLen;
            ip += nLitLen;
        }

    first_literal_run:
        t = *ip++;

        if (t >= 16) {
            goto match;
        }

        // M1 match: distance = 1 + 0x0800 + (t>>2) + (*ip << 2), length = 3
        {
            quint32 nDist = 1 + LZO_M2_MAX_OFFSET + (t >> 2) + ((quint32)*ip++ << 2);
            if ((qint64)(op - pOutput) < (qint64)nDist || op + 3 > op_end) return false;
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
                quint32 nDist = 1 + ((t >> 2) & 7) + ((quint32)*ip++ << 3);
                quint32 nLen = (t >> 5) - 1 + 2;  // -1 because we copy 2 initial + t loop
                if ((qint64)(op - pOutput) < (qint64)nDist || op + nLen > op_end) return false;
                const quint8 *pRef = op - nDist;
                *op++ = *pRef++;
                *op++ = *pRef++;
                for (quint32 i = 0; i < nLen - 2; i++) {
                    *op++ = *pRef++;
                }
            } else if (t >= 32) {
                // M3 match: variable length, distance from next 2 bytes
                quint32 nLen = t & 31;
                if (nLen == 0) {
                    while (ip < ip_end && *ip == 0) {
                        nLen += 255;
                        ip++;
                    }
                    if (ip >= ip_end) return false;
                    nLen += 31 + *ip++;
                }
                nLen += 2;
                if (ip + 2 > ip_end) return false;
                quint32 nDist = 1 + ((ip[0] >> 2) + ((quint32)ip[1] << 6));
                ip += 2;
                if ((qint64)(op - pOutput) < (qint64)nDist || op + nLen > op_end) return false;
                const quint8 *pRef = op - nDist;
                for (quint32 i = 0; i < nLen; i++) {
                    *op++ = *pRef++;
                }
            } else if (t >= 16) {
                // M4 match: variable length, large distance
                quint32 nDistHi = (t & 8) << 11;  // 0 or 0x4000
                quint32 nLen = t & 7;
                if (nLen == 0) {
                    while (ip < ip_end && *ip == 0) {
                        nLen += 255;
                        ip++;
                    }
                    if (ip >= ip_end) return false;
                    nLen += 7 + *ip++;
                }
                nLen += 2;
                if (ip + 2 > ip_end) return false;
                quint32 nDistLo = (ip[0] >> 2) + ((quint32)ip[1] << 6);
                ip += 2;
                quint32 nDist = nDistHi + nDistLo;
                if (nDist == 0) {
                    // End of stream marker (m_pos == op)
                    break;
                }
                nDist += 0x4000;
                if ((qint64)(op - pOutput) < (qint64)nDist || op + nLen > op_end) return false;
                const quint8 *pRef = op - nDist;
                for (quint32 i = 0; i < nLen; i++) {
                    *op++ = *pRef++;
                }
            } else {
                // t < 16: shouldn't reach here in match context
                return false;
            }

        match_done:
            t = ip[-2] & 3;

            if (t == 0) {
                break;
            }

        match_next:
            if (op + t > op_end || ip + t > ip_end) return false;
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
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    QIODevice *pInput = pDecompressState->pDeviceInput;
    QIODevice *pOutput = pDecompressState->pDeviceOutput;

    pInput->seek(pDecompressState->nInputOffset);

    // LZOP magic: 89 4C 5A 4F 00 0D 0A 1A 0A (9 bytes)
    QByteArray baMagic = pInput->read(9);
    if (baMagic.size() != 9) return false;

    const quint8 nExpectedMagic[] = {0x89, 0x4C, 0x5A, 0x4F, 0x00, 0x0D, 0x0A, 0x1A, 0x0A};
    if (memcmp(baMagic.constData(), nExpectedMagic, 9) != 0) return false;

    // Read header fields (big-endian)
    auto readBE16 = [&pInput]() -> quint16 {
        quint8 buf[2];
        if (pInput->read((char *)buf, 2) != 2) return 0;
        return ((quint16)buf[0] << 8) | buf[1];
    };

    auto readBE32 = [&pInput]() -> quint32 {
        quint8 buf[4];
        if (pInput->read((char *)buf, 4) != 4) return 0;
        return ((quint32)buf[0] << 24) | ((quint32)buf[1] << 16) | ((quint32)buf[2] << 8) | buf[3];
    };

    quint16 nVersion = readBE16();     // lzop version
    quint16 nLibVersion = readBE16();  // lzo lib version
    quint16 nExtractVersion = 0;

    if (nVersion >= 0x0940) {
        nExtractVersion = readBE16();  // version needed to extract
    }

    quint8 nMethod = 0;
    pInput->read((char *)&nMethod, 1);  // compression method
    quint8 nLevel = 0;

    if (nVersion >= 0x0940) {
        pInput->read((char *)&nLevel, 1);  // compression level
    }

    quint32 nFlags = readBE32();  // flags

    // Filter (for version >= 0x0940 with filter flag 0x00000800)
    if (nVersion >= 0x0940 && (nFlags & 0x00000800)) {
        readBE32();  // filter
    }

    quint32 nMode = readBE32();  // file mode
    readBE32();                  // mtime_low
    if (nVersion >= 0x0940) {
        readBE32();  // mtime_high
    }

    // Original file name
    quint8 nNameLen = 0;
    pInput->read((char *)&nNameLen, 1);
    if (nNameLen > 0) {
        pInput->read(nNameLen);  // skip filename
    }

    // Header checksum
    readBE32();

    Q_UNUSED(nVersion)
    Q_UNUSED(nLibVersion)
    Q_UNUSED(nExtractVersion)
    Q_UNUSED(nMethod)
    Q_UNUSED(nLevel)
    Q_UNUSED(nMode)

    bool bHasAdler32Uncompressed = (nFlags & 0x00000001) != 0;
    bool bHasCrc32Uncompressed = (nFlags & 0x00000100) != 0;
    bool bHasAdler32Compressed = (nFlags & 0x00000002) != 0;
    bool bHasCrc32Compressed = (nFlags & 0x00000200) != 0;

    qint64 nTotalOutput = 0;
    qint64 nTotalInput = 0;

    // Process blocks
    for (;;) {
        if (pPdStruct && !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        quint32 nUncompressedBlockSize = readBE32();

        if (nUncompressedBlockSize == 0) {
            // End of stream
            break;
        }

        if (nUncompressedBlockSize > 64 * 1024 * 1024) {
            return false;  // Sanity check
        }

        quint32 nCompressedBlockSize = readBE32();

        if (nCompressedBlockSize > nUncompressedBlockSize) {
            return false;
        }

        // Skip checksums
        if (bHasAdler32Uncompressed) readBE32();
        if (bHasCrc32Uncompressed) readBE32();
        if (nCompressedBlockSize < nUncompressedBlockSize) {
            if (bHasAdler32Compressed) readBE32();
            if (bHasCrc32Compressed) readBE32();
        }

        QByteArray baCompressed = pInput->read(nCompressedBlockSize);
        if (baCompressed.size() != (qint32)nCompressedBlockSize) return false;

        if (nCompressedBlockSize == nUncompressedBlockSize) {
            // Stored block (not compressed)
            qint64 nWritten = pOutput->write(baCompressed);
            if (nWritten != (qint64)nUncompressedBlockSize) return false;
            nTotalOutput += nWritten;
        } else {
            QByteArray baUncompressed(nUncompressedBlockSize, 0);
            qint64 nBytesWritten = 0;

            bool bOk =
                decompressBlock((const quint8 *)baCompressed.constData(), nCompressedBlockSize, (quint8 *)baUncompressed.data(), nUncompressedBlockSize, &nBytesWritten);

            if (!bOk || nBytesWritten != (qint64)nUncompressedBlockSize) {
                return false;
            }

            qint64 nWritten = pOutput->write(baUncompressed.constData(), nBytesWritten);
            if (nWritten != nBytesWritten) return false;
            nTotalOutput += nWritten;
        }

        nTotalInput += nCompressedBlockSize;
    }

    pDecompressState->nCountInput = pInput->pos() - pDecompressState->nInputOffset;
    pDecompressState->nCountOutput = nTotalOutput;

    return true;
}
