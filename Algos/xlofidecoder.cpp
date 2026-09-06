/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xlofidecoder.h"

#include "algo_utils.h"
#include "xalgo_local.h"

#include <QtEndian>

#include <cstring>
#include <limits>

namespace {
const qint64 LOFI_NAME_SIZE = 36;
const qint64 LOFI_INDEX_OFFSET = 0x30;
const qint64 LOFI_MIN_HEADER = LOFI_INDEX_OFFSET + 8;
// One byte of framing, then the 5-byte LZMA props and the 8-byte size.
const qint64 LOFI_SEGMENT_PREFIX = 1;
const qint64 LOFI_ALONE_HEADER = 13;
// `lofiadm` caps the segment size well below this; the bound only keeps a
// corrupt header from asking for an absurd allocation.
const qint64 LOFI_MAX_SEGMENT_SIZE = 64 * 1024 * 1024;
// Index entries are 8 bytes each, so this is a 512 MiB index - far past any
// image `lofiadm` can produce, and it keeps nIndexEntries * 8 in range.
const qint64 LOFI_MAX_INDEX_ENTRIES = 64 * 1024 * 1024;
}  // namespace

bool XLOFIDecoder::parseGeometry(const QByteArray &baHeader, qint64 nInputSize,
                                 GEOMETRY *pGeometry, QList<qint64> *pListIndex)
{
    if (!pGeometry) return false;
    if (baHeader.size() < LOFI_MIN_HEADER) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    // The name field is the algorithm followed by zero padding.  Only "lzma" is
    // supported here; see the header comment for why "gzip" is refused.
    if (memcmp(pHeader, "lzma", 4) != 0) return false;
    for (qint64 i = 4; i < LOFI_NAME_SIZE; i++) {
        if (pHeader[i] != 0) return false;
    }

    GEOMETRY geometry = {};
    geometry.nSegmentSize = static_cast<qint64>(qFromBigEndian<quint32>(pHeader + 0x24));
    geometry.nIndexEntries = static_cast<qint64>(qFromBigEndian<quint32>(pHeader + 0x28));
    geometry.nLastSegmentSize = static_cast<qint64>(qFromBigEndian<quint32>(pHeader + 0x2c));

    // Exactly the reference detector's rules: a positive segment size, more than
    // one index entry, and a final segment that is positive and no larger than a
    // full one.
    if ((geometry.nSegmentSize <= 0) || (geometry.nSegmentSize > LOFI_MAX_SEGMENT_SIZE)) return false;
    if ((geometry.nIndexEntries <= 1) || (geometry.nIndexEntries > LOFI_MAX_INDEX_ENTRIES)) return false;
    if ((geometry.nLastSegmentSize <= 0) || (geometry.nLastSegmentSize > geometry.nSegmentSize)) return false;

    geometry.nDataOffset = LOFI_INDEX_OFFSET + geometry.nIndexEntries * 8;
    if ((nInputSize >= 0) && (geometry.nDataOffset > nInputSize)) return false;
    if (baHeader.size() < geometry.nDataOffset) {
        // Caller handed us a prefix that stops inside the index.
        return false;
    }

    const qint64 nSegments = geometry.nIndexEntries - 1;
    // (segments - 1) full segments plus the short final one.
    if (nSegments > ((std::numeric_limits<qint64>::max)() / geometry.nSegmentSize)) return false;
    geometry.nImageSize = (nSegments - 1) * geometry.nSegmentSize + geometry.nLastSegmentSize;

    QList<qint64> listIndex;
    listIndex.reserve(qint32(qMin<qint64>(geometry.nIndexEntries, 1 << 20)));
    qint64 nPrevious = -1;
    for (qint64 i = 0; i < geometry.nIndexEntries; i++) {
        const quint64 nRaw = qFromBigEndian<quint64>(pHeader + LOFI_INDEX_OFFSET + i * 8);
        if (nRaw > quint64((std::numeric_limits<qint64>::max)())) return false;
        const qint64 nValue = static_cast<qint64>(nRaw);
        // Entry 0 is 0 (the reference detector checks exactly this), and the
        // rest strictly ascend: every segment must carry at least its own
        // framing bytes.
        if (i == 0) {
            if (nValue != 0) return false;
        } else if (nValue - nPrevious < LOFI_SEGMENT_PREFIX + LOFI_ALONE_HEADER) {
            return false;
        }
        nPrevious = nValue;
        listIndex.append(nValue);
    }
    geometry.nDataSize = nPrevious;
    if ((nInputSize >= 0) && (geometry.nDataOffset + geometry.nDataSize > nInputSize)) return false;

    *pGeometry = geometry;
    if (pListIndex) *pListIndex = listIndex;
    return true;
}

bool XLOFIDecoder::decode(const QByteArray &packed, qint64 nUncompressedSize,
                          QByteArray *pOutput, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput) return false;
    if ((nUncompressedSize <= 0) || (nUncompressedSize > qint64((std::numeric_limits<qint32>::max)()))) return false;

    GEOMETRY geometry = {};
    QList<qint64> listIndex;
    if (!parseGeometry(packed, packed.size(), &geometry, &listIndex)) return false;
    if (geometry.nImageSize != nUncompressedSize) return false;

    QByteArray result(qint32(nUncompressedSize), 0);
    const uchar *pBase = reinterpret_cast<const uchar *>(packed.constData()) + geometry.nDataOffset;

    CLzmaDec state = {};
    X_LzmaDec_Construct(&state);
    bool bAllocated = false;
    quint8 currentProps[5] = {0, 0, 0, 0, 0};
    bool bResult = true;
    qint64 nProduced = 0;
    const qint64 nSegments = geometry.nIndexEntries - 1;

    for (qint64 i = 0; bResult && (i < nSegments); i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            bResult = false;
            break;
        }
        const qint64 nStart = listIndex.at(qint32(i));
        const qint64 nEnd = listIndex.at(qint32(i + 1));
        const qint64 nSegmentBytes = nEnd - nStart;
        if (nSegmentBytes <= LOFI_SEGMENT_PREFIX + LOFI_ALONE_HEADER) {
            bResult = false;
            break;
        }
        const uchar *pSegment = pBase + nStart;
        // Every segment carries this framing byte in every known image; it is
        // not part of the LZMA "alone" header that follows it.
        if (pSegment[0] != 0x01) {
            bResult = false;
            break;
        }
        const uchar *pProps = pSegment + LOFI_SEGMENT_PREFIX;
        const quint64 nDeclared = qFromLittleEndian<quint64>(pProps + 5);
        const qint64 nWant = (i + 1 == nSegments) ? geometry.nLastSegmentSize : geometry.nSegmentSize;
        if (nDeclared != quint64(nWant)) {
            bResult = false;
            break;
        }
        if (nProduced + nWant > nUncompressedSize) {
            bResult = false;
            break;
        }

        if (!bAllocated || (memcmp(currentProps, pProps, 5) != 0)) {
            if (bAllocated) {
                X_LzmaDec_Free(&state, Algo_utils::lzmaAlloc());
                bAllocated = false;
                X_LzmaDec_Construct(&state);
            }
            if (X_LzmaDec_Allocate(&state, reinterpret_cast<const Byte *>(pProps), 5, Algo_utils::lzmaAlloc()) != 0) {
                bResult = false;
                break;
            }
            bAllocated = true;
            memcpy(currentProps, pProps, 5);
        }
        X_LzmaDec_Init(&state);

        const uchar *pInput = pSegment + LOFI_SEGMENT_PREFIX + LOFI_ALONE_HEADER;
        qint64 nInputLeft = nSegmentBytes - LOFI_SEGMENT_PREFIX - LOFI_ALONE_HEADER;
        qint64 nOutputLeft = nWant;
        while (nOutputLeft > 0) {
            SizeT nDestLength = SizeT(nOutputLeft);
            SizeT nSourceLength = SizeT(nInputLeft);
            ELzmaStatus status = LZMA_STATUS_NOT_SPECIFIED;
            const SRes ret = X_LzmaDec_DecodeToBuf(&state, reinterpret_cast<Byte *>(result.data()) + nProduced, &nDestLength,
                                                  reinterpret_cast<const Byte *>(pInput), &nSourceLength, LZMA_FINISH_ANY, &status);
            if (ret != 0) {
                bResult = false;
                break;
            }
            // No forward progress means the stream ended before the segment's
            // declared length was produced.
            if ((nDestLength == 0) && (nSourceLength == 0)) {
                bResult = false;
                break;
            }
            nProduced += qint64(nDestLength);
            nOutputLeft -= qint64(nDestLength);
            pInput += nSourceLength;
            nInputLeft -= qint64(nSourceLength);
        }
    }

    if (bAllocated) X_LzmaDec_Free(&state, Algo_utils::lzmaAlloc());
    if (!bResult || (nProduced != nUncompressedSize)) return false;

    *pOutput = result;
    return true;
}
