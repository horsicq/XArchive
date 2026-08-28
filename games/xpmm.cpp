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
#include "xpmm.h"

#include <QtEndian>

#include <cstring>

namespace {
const qint32 PMM_SAMPLE_SLOT_COUNT = 8;
const qint64 PMM_PMA_OFFSET_FIELD = 0x10;
const qint64 PMM_SAMPLE_OFFSETS_FIELD = 0x14;
const qint64 PMM_RESERVED_FIELD = 0x34;
const qint64 PMM_MDH_OFFSET = 0x38;
const qint64 PMM_MDH_TRACK_OFFSET_FIELD = 0x3E;
const qint64 PMM_MDH_SAMPLE_COUNT_FIELD = 0x40;
const qint64 PMM_HEADER_PROBE_SIZE = PMM_MDH_SAMPLE_COUNT_FIELD + 1;
const qint64 SM8_HEADER_SIZE = 10;
}

XPMM::XPMM(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_PMM)
{
}

bool XPMM::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPMM archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XPMM::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPMM(pDevice);
}

bool XPMM::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                      PDSTRUCT *pPdStruct)
{
    QPointer<XPMM> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < PMM_HEADER_PROBE_SIZE) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const QByteArray baHeader = read_array_process(
        0, PMM_HEADER_PROBE_SIZE, pPdStruct);
    if (!guardedThis || (baHeader.size() != PMM_HEADER_PROBE_SIZE)) {
        return false;
    }
    if ((memcmp(baHeader.constData(), "MTCVTS PSM 2.00", 16) != 0) ||
        (memcmp(baHeader.constData() + PMM_MDH_OFFSET, "MDH\0", 4) != 0)) {
        return false;
    }

    const uchar *pHeader = reinterpret_cast<const uchar *>(
        baHeader.constData());
    if (readLE32(pHeader + PMM_RESERVED_FIELD) != 0) return false;

    const qint32 nSampleCount = pHeader[PMM_MDH_SAMPLE_COUNT_FIELD];
    if ((nSampleCount < 1) || (nSampleCount > PMM_SAMPLE_SLOT_COUNT) ||
        (nTotalSize < (PMM_HEADER_PROBE_SIZE + 4 +
                       ((qint64)nSampleCount * SM8_HEADER_SIZE)))) {
        return false;
    }

    const qint64 nPmaOffset = readLE32(pHeader + PMM_PMA_OFFSET_FIELD);
    const qint64 nTrackDataOffset = PMM_MDH_OFFSET +
        qFromLittleEndian<quint16>(pHeader + PMM_MDH_TRACK_OFFSET_FIELD);
    QList<qint64> listSampleOffsets;
    listSampleOffsets.reserve(nSampleCount);
    for (qint32 i = 0; i < PMM_SAMPLE_SLOT_COUNT; ++i) {
        const qint64 nOffset = readLE32(
            pHeader + PMM_SAMPLE_OFFSETS_FIELD + ((qint64)i * 4));
        if (i < nSampleCount) {
            listSampleOffsets.append(nOffset);
        } else if (nOffset != 0) {
            return false;
        }
    }

    if ((nPmaOffset < PMM_HEADER_PROBE_SIZE) ||
        (nTrackDataOffset < PMM_HEADER_PROBE_SIZE) ||
        (nTrackDataOffset >= nPmaOffset) ||
        !rangeWithin(nTotalSize, nPmaOffset, 4) ||
        (listSampleOffsets.constFirst() < (nPmaOffset + 4))) {
        return false;
    }
    for (qint32 i = 0; i < nSampleCount; ++i) {
        const qint64 nOffset = listSampleOffsets.at(i);
        if (!rangeWithin(nTotalSize, nOffset, SM8_HEADER_SIZE) ||
            ((i > 0) && (nOffset <= listSampleOffsets.at(i - 1)))) {
            return false;
        }
    }

    const QByteArray baPmaSignature = read_array_process(
        nPmaOffset, 4, pPdStruct);
    if (!guardedThis || (baPmaSignature.size() != 4) ||
        (memcmp(baPmaSignature.constData(), "PLX\0", 4) != 0)) {
        return false;
    }

    for (qint32 i = 0; i < nSampleCount; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const qint64 nOffset = listSampleOffsets.at(i);
        const qint64 nNextOffset = (i + 1 < nSampleCount)
            ? listSampleOffsets.at(i + 1) : nTotalSize;
        const qint64 nSectionSize = nNextOffset - nOffset;
        const QByteArray baSm8Header = read_array_process(
            nOffset, SM8_HEADER_SIZE, pPdStruct);
        if (!guardedThis || (baSm8Header.size() != SM8_HEADER_SIZE) ||
            (memcmp(baSm8Header.constData(), "SM8\0\0\1", 6) != 0)) {
            return false;
        }

        const uchar *pSm8 = reinterpret_cast<const uchar *>(
            baSm8Header.constData());
        const qint64 nPcmSize = qFromLittleEndian<quint16>(pSm8 + 6);
        if (nSectionSize != (SM8_HEADER_SIZE + nPcmSize)) return false;
    }

    if (pEntries) {
        ENTRY mdhEntry = {};
        mdhEntry.nHeaderOffset = 0;
        mdhEntry.nHeaderSize = PMM_MDH_OFFSET;
        mdhEntry.nDataOffset = PMM_MDH_OFFSET;
        mdhEntry.nDataSize = nPmaOffset - PMM_MDH_OFFSET;
        mdhEntry.sFileName = QStringLiteral("metadata.mdh");
        pEntries->append(mdhEntry);

        ENTRY pmaEntry = {};
        pmaEntry.nHeaderOffset = PMM_PMA_OFFSET_FIELD;
        pmaEntry.nHeaderSize = 4;
        pmaEntry.nDataOffset = nPmaOffset;
        pmaEntry.nDataSize = listSampleOffsets.constFirst() - nPmaOffset;
        pmaEntry.sFileName = QStringLiteral("instruments.pma");
        pEntries->append(pmaEntry);

        for (qint32 i = 0; i < nSampleCount; ++i) {
            const qint64 nOffset = listSampleOffsets.at(i);
            const qint64 nNextOffset = (i + 1 < nSampleCount)
                ? listSampleOffsets.at(i + 1) : nTotalSize;
            ENTRY sampleEntry = {};
            sampleEntry.nHeaderOffset =
                PMM_SAMPLE_OFFSETS_FIELD + ((qint64)i * 4);
            sampleEntry.nHeaderSize = 4;
            sampleEntry.nDataOffset = nOffset;
            sampleEntry.nDataSize = nNextOffset - nOffset;
            sampleEntry.sFileName = QStringLiteral("sample_%1.sm8")
                .arg(i + 1, 2, 10, QLatin1Char('0'));
            pEntries->append(sampleEntry);
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (pArchiveEnd) *pArchiveEnd = nTotalSize;
    return true;
}
