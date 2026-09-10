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
#include "xvmspcsidecoder.h"

#include <QtEndian>

#include <cstring>
#include <limits>

namespace {
const char *PCSI_BANNER = "OpenVMS DCX PCSI Compressed File";
const qint32 PCSI_BANNER_SIZE = 32;
const qint64 PCSI_HEADER_SIZE = 0x50;
const qint64 PCSI_TABLE_OFFSET = 0x200;
const qint64 PCSI_TABLE_HEADER_SIZE = 0x14;
const qint64 PCSI_BLOCK_HEADER_SIZE = 0x0c;
const qint64 PCSI_SLOT_SIZE = 0x440;
const qint64 PCSI_SLOT_NODE = 0x40;
const qint64 PCSI_SLOT_MAP = 0x240;
const qint32 PCSI_MAX_NODE = 0x200;
const quint32 PCSI_DCX_MAGIC = 0x5bf5a3a7;
const qint64 PCSI_RECORD_HEADER_SIZE = 2;
const qint32 PCSI_MAX_RECORDS = 0x1000000;
// A sanity ceiling on the walk, not a format limit; real kits reach tens of
// megabytes and the exact allocation always comes from measure().
const qint64 PCSI_MAX_OUTPUT = 0x20000000;

// The blob's own end is NOT where the records start; it is rounded up first.
qint64 pcsiRoundUp(qint64 nValue)
{
    return (nValue + (PCSI_TABLE_OFFSET - 1)) & ~(PCSI_TABLE_OFFSET - 1);
}

bool pcsiParseHeader(const quint8 *pData, qint64 nSize, quint32 *pnTableSize, qint32 *pnRecordCount)
{
    if ((nSize < PCSI_HEADER_SIZE) || (nSize < (PCSI_TABLE_OFFSET + PCSI_TABLE_HEADER_SIZE))) return false;
    for (qint32 i = 0; i < PCSI_BANNER_SIZE; ++i) {
        if (pData[i] != (quint8)PCSI_BANNER[i]) return false;
    }

    const quint32 nTableSize = qFromLittleEndian<quint32>(pData + 0x38);
    const qint32 nRecordCount = (qint32)qFromLittleEndian<quint32>(pData + 0x40);
    // A count of zero produces no output at all, which the reference reports as
    // a failure rather than as an empty file.
    if ((nRecordCount < 1) || (nRecordCount > PCSI_MAX_RECORDS)) return false;
    if (nTableSize < (quint32)PCSI_TABLE_HEADER_SIZE) return false;

    *pnTableSize = nTableSize;
    *pnRecordCount = nRecordCount;

    return true;
}

// Expands the context blocks into fixed slots and reports where the records
// begin.  pbaContexts comes back holding nContextCount * 0x440 bytes.
bool pcsiParseTables(const quint8 *pData, qint64 nSize, quint32 nTableSize, QByteArray *pbaContexts, qint32 *pnContextCount, qint64 *pnRecordsOffset,
                     XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaContexts || !pnContextCount || !pnRecordsOffset) return false;

    qint64 nPosition = PCSI_TABLE_OFFSET;
    if ((nSize - nPosition) < PCSI_TABLE_HEADER_SIZE) return false;

    const quint32 nSize0 = qFromLittleEndian<quint32>(pData + nPosition);
    const quint32 nZero1 = qFromLittleEndian<quint32>(pData + nPosition + 0x04);
    const quint32 nMagic = qFromLittleEndian<quint32>(pData + nPosition + 0x08);
    const quint32 nZero2 = qFromLittleEndian<quint32>(pData + nPosition + 0x0c);
    const qint32 nContextCount = (qint32)qFromLittleEndian<quint16>(pData + nPosition + 0x10);
    const quint32 nHeaderLength = qFromLittleEndian<quint16>(pData + nPosition + 0x12);
    if ((nSize0 != nTableSize) || nZero1 || (nMagic != PCSI_DCX_MAGIC) || nZero2 || (nHeaderLength != PCSI_TABLE_HEADER_SIZE)) return false;
    // Context 0 is the walk's entry point, so an empty table is unusable.
    if (nContextCount < 1) return false;
    // Every block costs at least its own 0x0c-byte header on disk, so the blob
    // size bounds the slot array and a crafted count cannot amplify it.
    if ((qint64)nContextCount > ((qint64)nTableSize - PCSI_TABLE_HEADER_SIZE) / PCSI_BLOCK_HEADER_SIZE) return false;

    const qint64 nSlotBytes = (qint64)nContextCount * PCSI_SLOT_SIZE;
    if (nSlotBytes > (qint64)(std::numeric_limits<qint32>::max)()) return false;

    QByteArray baContexts((qint32)nSlotBytes, (char)0);
    if (baContexts.size() != (qint32)nSlotBytes) return false;
    quint8 *pSlots = (quint8 *)baContexts.data();

    nPosition += PCSI_TABLE_HEADER_SIZE;

    for (qint32 i = 0; i < nContextCount; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if ((nSize - nPosition) < PCSI_BLOCK_HEADER_SIZE) return false;

        const qint64 nBlockLength = (qint64)qFromLittleEndian<quint16>(pData + nPosition);
        const qint32 nFirst = pData[nPosition + 0x02];
        const qint32 nLast = pData[nPosition + 0x03];
        const quint8 nZero3 = pData[nPosition + 0x04];
        const quint8 nZero4 = pData[nPosition + 0x05];
        const qint64 nBlockHeader = (qint64)qFromLittleEndian<quint16>(pData + nPosition + 0x06);
        const qint64 nNodeOffset = (qint64)qFromLittleEndian<quint16>(pData + nPosition + 0x08);
        const qint64 nMapOffset = (qint64)qFromLittleEndian<quint16>(pData + nPosition + 0x0a);

        if ((nFirst > nLast) || nZero3 || nZero4 || (nBlockHeader != PCSI_BLOCK_HEADER_SIZE)) return false;
        if ((nNodeOffset <= PCSI_BLOCK_HEADER_SIZE) || ((nNodeOffset - PCSI_BLOCK_HEADER_SIZE) > PCSI_SLOT_NODE)) return false;
        if ((nBlockLength < nNodeOffset) || (nBlockLength > (nSize - nPosition))) return false;

        quint8 *pSlot = pSlots + (qint64)i * PCSI_SLOT_SIZE;
        const qint64 nBitmapSize = nNodeOffset - PCSI_BLOCK_HEADER_SIZE;
        memcpy(pSlot, pData + nPosition + PCSI_BLOCK_HEADER_SIZE, (size_t)nBitmapSize);

        const qint64 nSymbolCount = (qint64)nLast - nFirst + 1;

        if (nMapOffset == 0) {
            const qint64 nNodeSize = nBlockLength - nNodeOffset;
            if ((nNodeSize <= 0) || (nNodeSize > (PCSI_SLOT_MAP - PCSI_SLOT_NODE))) return false;
            memcpy(pSlot + PCSI_SLOT_NODE, pData + nPosition + nNodeOffset, (size_t)nNodeSize);
        } else {
            const qint64 nNodeSize = nMapOffset - nNodeOffset;
            if ((nNodeSize <= 0) || (nNodeSize > (PCSI_SLOT_MAP - PCSI_SLOT_NODE))) return false;
            if ((nBlockLength - nMapOffset) != (nSymbolCount * 2)) return false;
            memcpy(pSlot + PCSI_SLOT_NODE, pData + nPosition + nNodeOffset, (size_t)nNodeSize);
            // 0x240 + 2 * (last + 1) can never pass the end of the slot, but the
            // arithmetic is the one place a malformed first/last pair would
            // reach out of it, so it is checked rather than reasoned about.
            const qint64 nMapStart = PCSI_SLOT_MAP + (qint64)nFirst * 2;
            if ((nMapStart < PCSI_SLOT_MAP) || ((nMapStart + nSymbolCount * 2) > PCSI_SLOT_SIZE)) return false;
            memcpy(pSlot + nMapStart, pData + nPosition + nMapOffset, (size_t)(nSymbolCount * 2));
        }

        nPosition += nBlockLength;
    }

    // Every symbol of every context must name a context that exists.  Doing it
    // here, once, is what keeps the inner decode loop free of a range test.
    for (qint32 i = 0; i < nContextCount; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const quint8 *pSlot = pSlots + (qint64)i * PCSI_SLOT_SIZE;
        for (qint32 j = 0; j < 256; ++j) {
            if ((qint32)qFromLittleEndian<quint16>(pSlot + PCSI_SLOT_MAP + j * 2) >= nContextCount) return false;
        }
    }

    *pbaContexts = baContexts;
    *pnContextCount = nContextCount;
    *pnRecordsOffset = pcsiRoundUp(nPosition);

    return true;
}

// One pass over every record.  pOutput may be null, in which case the walk only
// counts - that is how the container's output size is learned without an
// allocation.
bool pcsiRunRecords(const quint8 *pData, qint64 nSize, qint64 nPosition, const quint8 *pSlots, qint32 nRecordCount, quint8 *pOutput,
                    qint64 nOutputCapacity, qint64 *pnProduced, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pnProduced) return false;

    qint64 nProduced = 0;

    for (qint32 nRecord = 0; nRecord < nRecordCount; ++nRecord) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        // TRAP: the 2-byte record header the reference reads and discards.
        if ((nSize - nPosition) < PCSI_RECORD_HEADER_SIZE) return false;
        nPosition += PCSI_RECORD_HEADER_SIZE;

        const quint8 *pContext = pSlots;
        qint32 nNode = 0;
        quint32 nAccumulator = 0;
        qint32 nBitsLeft = 0;

        for (;;) {
            if (nBitsLeft == 0) {
                if (nPosition >= nSize) return false;
                nAccumulator = pData[nPosition];
                ++nPosition;
                nBitsLeft = 8;
            }
            const quint32 nBit = nAccumulator & 1;
            nAccumulator >>= 1;
            --nBitsLeft;
            if (nBit) ++nNode;
            if (nNode >= PCSI_MAX_NODE) return false;

            if (pContext[nNode >> 3] & (1 << (nNode & 7))) {
                const qint32 nSymbol = pContext[PCSI_SLOT_NODE + nNode];
                if (nProduced >= nOutputCapacity) return false;
                if (pOutput) pOutput[nProduced] = (quint8)nSymbol;
                ++nProduced;
                const qint32 nNext = (qint32)qFromLittleEndian<quint16>(pContext + PCSI_SLOT_MAP + nSymbol * 2);
                pContext = pSlots + (qint64)nNext * PCSI_SLOT_SIZE;
                nNode = 0;
            } else {
                const qint32 nChild = pContext[PCSI_SLOT_NODE + nNode];
                if (nChild == 0) break;
                nNode = nChild * 2;
            }
        }
    }

    *pnProduced = nProduced;

    return true;
}

bool pcsiPrepare(const QByteArray &baFile, QByteArray *pbaContexts, qint32 *pnContextCount, qint32 *pnRecordCount, qint64 *pnRecordsOffset,
                 XBinary::PDSTRUCT *pPdStruct)
{
    const qint64 nSize = baFile.size();
    if (nSize < (PCSI_TABLE_OFFSET + PCSI_TABLE_HEADER_SIZE)) return false;

    const quint8 *pData = (const quint8 *)baFile.constData();
    quint32 nTableSize = 0;
    if (!pcsiParseHeader(pData, nSize, &nTableSize, pnRecordCount)) return false;
    if (!pcsiParseTables(pData, nSize, nTableSize, pbaContexts, pnContextCount, pnRecordsOffset, pPdStruct)) return false;
    if ((*pnRecordsOffset < 0) || (*pnRecordsOffset > nSize)) return false;

    return true;
}
}  // namespace

bool XVMSPCSIDecoder::isBannerValid(const QByteArray &baHeader)
{
    if (baHeader.size() < PCSI_BANNER_SIZE) return false;
    return memcmp(baHeader.constData(), PCSI_BANNER, (size_t)PCSI_BANNER_SIZE) == 0;
}

bool XVMSPCSIDecoder::measure(const QByteArray &baFile, qint64 *pnUncompressedSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pnUncompressedSize) return false;
    *pnUncompressedSize = 0;

    QByteArray baContexts;
    qint32 nContextCount = 0;
    qint32 nRecordCount = 0;
    qint64 nRecordsOffset = 0;
    if (!pcsiPrepare(baFile, &baContexts, &nContextCount, &nRecordCount, &nRecordsOffset, pPdStruct)) return false;

    qint64 nProduced = 0;
    if (!pcsiRunRecords((const quint8 *)baFile.constData(), baFile.size(), nRecordsOffset, (const quint8 *)baContexts.constData(), nRecordCount, nullptr,
                        PCSI_MAX_OUTPUT, &nProduced, pPdStruct)) {
        return false;
    }
    if (nProduced <= 0) return false;

    *pnUncompressedSize = nProduced;

    return true;
}

bool XVMSPCSIDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize <= 0) || (nUncompressedSize > (qint64)(std::numeric_limits<qint32>::max)())) return false;

    QByteArray baContexts;
    qint32 nContextCount = 0;
    qint32 nRecordCount = 0;
    qint64 nRecordsOffset = 0;
    if (!pcsiPrepare(baPacked, &baContexts, &nContextCount, &nRecordCount, &nRecordsOffset, pPdStruct)) return false;

    QByteArray baOut((qint32)nUncompressedSize, (char)0);
    if (baOut.size() != (qint32)nUncompressedSize) return false;

    qint64 nProduced = 0;
    if (!pcsiRunRecords((const quint8 *)baPacked.constData(), baPacked.size(), nRecordsOffset, (const quint8 *)baContexts.constData(), nRecordCount,
                        (quint8 *)baOut.data(), nUncompressedSize, &nProduced, pPdStruct)) {
        return false;
    }
    if (nProduced != nUncompressedSize) return false;

    *pbaResult = baOut;

    return true;
}
