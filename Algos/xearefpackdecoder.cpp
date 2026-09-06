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
#include "xearefpackdecoder.h"

#include <limits>

namespace {
const quint8 REFPACK_MAGIC_LOW = 0xfbU;   // second signature byte, constant
const quint8 REFPACK_FLAG_LARGE = 0x01U;  // size fields are four bytes wide
const quint8 REFPACK_FLAG_PACKED = 0x80U;  // packed size precedes unpacked size
// Bits 0x02..0x40 are unassigned; requiring them to be clear is what keeps a
// stray 0xFB byte in arbitrary data from being read as a header.
const quint8 REFPACK_FLAG_BASE = 0x10U;
const quint8 REFPACK_FLAG_MASK = 0x7eU;

// One decoded command.  nCommandSize counts only the command bytes, not the
// literals that follow them.
struct REFPACK_COMMAND {
    qint32 nCommandSize;
    qint32 nLiterals;
    qint32 nCopy;
    qint64 nDistance;
    bool bTerminator;
};

// Decodes the command at pData[nPosition].  Returns false when the command
// runs past the end of the buffer - the caller treats that as a broken stream
// rather than as a normal end, because RefPack always ends on an explicit
// 0xFC..0xFF terminator.
bool refpackReadCommand(const quint8 *pData, qint64 nSize, qint64 nPosition, REFPACK_COMMAND *pCommand)
{
    if (!pData || !pCommand || (nPosition < 0) || (nPosition >= nSize)) return false;

    const quint8 nByte0 = pData[nPosition];
    REFPACK_COMMAND command = {};

    if (nByte0 < 0x80U) {
        if ((nSize - nPosition) < 2) return false;
        const quint8 nByte1 = pData[nPosition + 1];
        command.nCommandSize = 2;
        command.nLiterals = qint32(nByte0 & 0x03U);
        command.nCopy = qint32((nByte0 & 0x1cU) >> 2) + 3;
        command.nDistance = qint64((quint32(nByte0 & 0x60U) << 3) + quint32(nByte1)) + 1;
    } else if (nByte0 < 0xc0U) {
        if ((nSize - nPosition) < 3) return false;
        const quint8 nByte1 = pData[nPosition + 1];
        const quint8 nByte2 = pData[nPosition + 2];
        command.nCommandSize = 3;
        command.nLiterals = qint32((nByte1 >> 6) & 0x03U);
        command.nCopy = qint32(nByte0 & 0x3fU) + 4;
        command.nDistance = qint64((quint32(nByte1 & 0x3fU) << 8) + quint32(nByte2)) + 1;
    } else if (nByte0 < 0xe0U) {
        if ((nSize - nPosition) < 4) return false;
        const quint8 nByte1 = pData[nPosition + 1];
        const quint8 nByte2 = pData[nPosition + 2];
        const quint8 nByte3 = pData[nPosition + 3];
        command.nCommandSize = 4;
        command.nLiterals = qint32(nByte0 & 0x03U);
        command.nCopy = qint32((quint32(nByte0 & 0x0cU) << 6) + quint32(nByte3)) + 5;
        command.nDistance = qint64((quint32(nByte0 & 0x10U) << 12) + (quint32(nByte1) << 8) + quint32(nByte2)) + 1;
    } else if (nByte0 < 0xfcU) {
        command.nCommandSize = 1;
        // The literal-run count is always a multiple of four; the encoder can
        // therefore never express a tail of 1..3 bytes with this command,
        // which is why the terminator carries its own short literal count.
        command.nLiterals = qint32((quint32(nByte0 & 0x1fU) << 2)) + 4;
        command.nCopy = 0;
        command.nDistance = 0;
    } else {
        command.nCommandSize = 1;
        command.nLiterals = qint32(nByte0 & 0x03U);
        command.nCopy = 0;
        command.nDistance = 0;
        command.bTerminator = true;
    }

    *pCommand = command;

    return true;
}
}  // namespace

bool XEARefPackDecoder::readHeader(const char *pData, qint64 nDataSize, qint64 nTotalSize, HEADER *pHeader)
{
    if (!pData || !pHeader || (nDataSize < 2)) return false;

    const quint8 *pBytes = reinterpret_cast<const quint8 *>(pData);
    if (pBytes[1] != REFPACK_MAGIC_LOW) return false;
    if ((pBytes[0] & REFPACK_FLAG_MASK) != REFPACK_FLAG_BASE) return false;

    HEADER header = {};
    header.bLargeSizes = (pBytes[0] & REFPACK_FLAG_LARGE) != 0;
    header.bHasPackedSize = (pBytes[0] & REFPACK_FLAG_PACKED) != 0;
    header.nPackedSize = -1;

    const qint32 nFieldSize = header.bLargeSizes ? 4 : 3;
    qint64 nPosition = 2;

    if (header.bHasPackedSize) {
        if ((nDataSize - nPosition) < nFieldSize) return false;
        qint64 nValue = 0;
        for (qint32 i = 0; i < nFieldSize; i++) {
            nValue = (nValue << 8) | qint64(pBytes[nPosition]);
            nPosition++;
        }
        header.nPackedSize = nValue;
    }

    if ((nDataSize - nPosition) < nFieldSize) return false;
    qint64 nUnpacked = 0;
    for (qint32 i = 0; i < nFieldSize; i++) {
        nUnpacked = (nUnpacked << 8) | qint64(pBytes[nPosition]);
        nPosition++;
    }
    header.nUnpackedSize = nUnpacked;
    header.nHeaderSize = nPosition;

    // A zero-length payload cannot be encoded (the shortest stream is still a
    // terminator command) and an absurd one is a false positive.
    if (header.nUnpackedSize <= 0) return false;
    if (header.nUnpackedSize > REFPACK_MAX_OUTPUT_SIZE) return false;
    if (header.bHasPackedSize) {
        if (header.nPackedSize <= header.nHeaderSize) return false;
        if ((nTotalSize >= 0) && (header.nPackedSize > nTotalSize)) return false;
    }
    if ((nTotalSize >= 0) && (nTotalSize <= header.nHeaderSize)) return false;

    *pHeader = header;

    return true;
}

bool XEARefPackDecoder::probeStream(const QByteArray &baStream, const HEADER &header, bool bComplete, qint64 nProduceLimit, qint64 *pnProduced, qint64 *pnConsumed)
{
    if (pnProduced) *pnProduced = 0;
    if (pnConsumed) *pnConsumed = 0;

    const qint64 nSize = baStream.size();
    if ((header.nHeaderSize <= 0) || (nSize <= header.nHeaderSize)) return false;

    const quint8 *pData = reinterpret_cast<const quint8 *>(baStream.constData());
    qint64 nPosition = header.nHeaderSize;
    qint64 nProduced = 0;
    bool bTerminated = false;
    qint64 nCommands = 0;

    while (nPosition < nSize) {
        REFPACK_COMMAND command = {};
        if (!refpackReadCommand(pData, nSize, nPosition, &command)) {
            // A command truncated by the sample window is not an error when
            // the sample is only a prefix of the container.
            if (!bComplete) break;
            return false;
        }
        nPosition += command.nCommandSize;
        if ((nSize - nPosition) < command.nLiterals) {
            if (!bComplete) break;
            return false;
        }
        nPosition += command.nLiterals;
        nProduced += command.nLiterals;

        if (command.nCopy > 0) {
            // No pre-filled window: a back-reference that reaches in front of
            // the output is impossible in a real stream.
            if ((command.nDistance <= 0) || (command.nDistance > nProduced)) return false;
            nProduced += command.nCopy;
        }

        nCommands++;

        if (command.bTerminator) {
            bTerminated = true;
            break;
        }
        if ((nProduceLimit > 0) && (nProduced > nProduceLimit)) break;
        if (nProduced > REFPACK_MAX_OUTPUT_SIZE) return false;
    }

    if (pnProduced) *pnProduced = nProduced;
    if (pnConsumed) *pnConsumed = nPosition;

    if (nCommands <= 0) return false;

    if (bComplete) {
        // The whole container was inspected: the grammar has to close on the
        // explicit terminator and account for exactly the declared output.
        if (!bTerminated) return false;
        if (nProduced != header.nUnpackedSize) return false;
    } else if (nProduced <= 0) {
        return false;
    }

    return true;
}

bool XEARefPackDecoder::decode(const QByteArray &packed, qint64 nExpectedSize, QByteArray *pOutput, qint64 *pConsumedSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput) return false;

    const qint64 nSize = packed.size();
    if ((nSize < 5) || (nSize > REFPACK_MAX_INPUT_SIZE)) return false;

    HEADER header = {};
    if (!readHeader(packed.constData(), nSize, nSize, &header)) return false;

    qint64 nTarget = header.nUnpackedSize;
    if (nExpectedSize >= 0) {
        // The container is authoritative; a caller that disagrees is looking
        // at the wrong record, so refuse instead of truncating silently.
        if (nExpectedSize != nTarget) return false;
    }
    if ((nTarget <= 0) || (nTarget > REFPACK_MAX_OUTPUT_SIZE) || (nTarget > qint64((std::numeric_limits<qint32>::max)()))) {
        return false;
    }

    QByteArray output;
    output.reserve(qint32(nTarget));

    const quint8 *pData = reinterpret_cast<const quint8 *>(packed.constData());
    qint64 nPosition = header.nHeaderSize;
    bool bTerminated = false;
    qint32 nCancelCounter = 0;

    while (nPosition < nSize) {
        if (((++nCancelCounter) & 0x3ff) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }

        REFPACK_COMMAND command = {};
        if (!refpackReadCommand(pData, nSize, nPosition, &command)) return false;
        nPosition += command.nCommandSize;

        if ((nSize - nPosition) < command.nLiterals) return false;
        if ((qint64(output.size()) + command.nLiterals + command.nCopy) > nTarget) return false;

        if (command.nLiterals > 0) {
            output.append(reinterpret_cast<const char *>(pData + nPosition), command.nLiterals);
            nPosition += command.nLiterals;
        }

        if (command.nCopy > 0) {
            if ((command.nDistance <= 0) || (command.nDistance > qint64(output.size()))) return false;
            // Byte-by-byte: overlapping copies (distance < length) are how the
            // encoder expresses runs, so a block move would be wrong here.
            for (qint32 i = 0; i < command.nCopy; i++) {
                output.append(output.at(output.size() - qint32(command.nDistance)));
            }
        }

        if (command.bTerminator) {
            bTerminated = true;
            break;
        }
    }

    if (!bTerminated) return false;
    if (qint64(output.size()) != nTarget) return false;

    *pOutput = output;
    if (pConsumedSize) *pConsumedSize = nPosition;

    return true;
}
