/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Native GameCube RVZ reader.
 */
#include "xrvzarchive.h"

#include "Algos/xzstddecoder.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QtEndian>

#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace {
const qint64 RVZ_FILE_HEADER_SIZE = 0x48;
const qint64 RVZ_HEADER_MIN_SIZE = 0xd5;
const qint64 RVZ_HEADER_MAX_SIZE = 0x1000;
const qint64 RVZ_DISC_HEADER_SIZE = 0x80;
const qint64 RVZ_RAW_ENTRY_SIZE = 0x18;
const qint64 RVZ_GROUP_ENTRY_SIZE = 0x0c;
const qint64 RVZ_SECTOR_SIZE = 0x8000;
const qint64 RVZ_GAMECUBE_MAX_ISO_SIZE = Q_INT64_C(0x57058000);
const qint64 RVZ_MAX_CHUNK_SIZE = Q_INT64_C(2) * 1024 * 1024;
const qint64 RVZ_MAX_PACKED_GROUP_SIZE = Q_INT64_C(8) * 1024 * 1024;
const qint64 RVZ_MAX_COMPRESSED_GROUP_SIZE =
    RVZ_MAX_PACKED_GROUP_SIZE + Q_INT64_C(1024) * 1024;
const qint64 RVZ_MAX_TABLE_COMPRESSED_SIZE = Q_INT64_C(16) * 1024 * 1024;
const quint32 RVZ_VERSION_1 = 0x01000000U;
const quint32 RVZ_COMPRESSION_NONE = 0;
const quint32 RVZ_COMPRESSION_ZSTD = 5;
const quint32 RVZ_GROUP_COMPRESSED = 0x80000000U;
const quint32 RVZ_GROUP_SIZE_MASK = 0x7fffffffU;
const qint32 RVZ_MAX_RAW_ENTRIES = 8192;
const qint32 RVZ_MAX_GROUP_ENTRIES = 65536;

class RvzDevicePositionGuard {
public:
    explicit RvzDevicePositionGuard(QIODevice *pDevice)
        : m_pDevice(pDevice), m_nPosition(-1), m_bRestored(false)
    {
        if (!m_pDevice || m_pDevice->isSequential()) return;
        m_nPosition = m_pDevice->pos();
    }

    ~RvzDevicePositionGuard()
    {
        restore();
    }

    bool isValid() const
    {
        return m_pDevice && (m_nPosition >= 0);
    }

    bool restore()
    {
        if (m_bRestored) return true;
        m_bRestored = true;
        return m_pDevice && (m_nPosition >= 0) &&
               m_pDevice->seek(m_nPosition);
    }

private:
    QIODevice *m_pDevice;
    qint64 m_nPosition;
    bool m_bRestored;
};

bool isPowerOfTwo(quint32 nValue)
{
    return nValue && ((nValue & (nValue - 1U)) == 0);
}

bool isGameCubeMagic(const QByteArray &baHeader)
{
    static const uchar GAMECUBE_MAGIC[] = {0xc2, 0x33, 0x9f, 0x3d};
    return (baHeader.size() >= RVZ_DISC_HEADER_SIZE) &&
           (memcmp(baHeader.constData() + 0x1c, GAMECUBE_MAGIC,
                   sizeof(GAMECUBE_MAGIC)) == 0);
}

void advanceJunkState(quint32 *pBuffer)
{
    for (qint32 i = 0; i < 32; ++i) {
        pBuffer[i] ^= pBuffer[i + 521 - 32];
    }
    for (qint32 i = 32; i < 521; ++i) {
        pBuffer[i] ^= pBuffer[i - 32];
    }
}

quint8 nextJunkByte(quint32 *pBuffer, qint32 *pnWord, qint32 *pnByte)
{
    const quint32 nValue = pBuffer[*pnWord];
    quint8 nResult = 0;
    switch (*pnByte) {
        case 0: nResult = quint8(nValue >> 24U); break;
        case 1: nResult = quint8(nValue >> 18U); break;
        case 2: nResult = quint8(nValue >> 8U); break;
        default: nResult = quint8(nValue); break;
    }
    ++(*pnByte);
    if (*pnByte == 4) {
        *pnByte = 0;
        ++(*pnWord);
        if (*pnWord == 521) {
            advanceJunkState(pBuffer);
            *pnWord = 0;
        }
    }
    return nResult;
}

bool fillPackedJunk(const uchar *pSeed, qint64 nDataOffset,
                    char *pOutput, qint64 nOutputSize)
{
    if (!pSeed || !pOutput || (nDataOffset < 0) || (nOutputSize < 0)) {
        return false;
    }

    quint32 buffer[521] = {};
    for (qint32 i = 0; i < 17; ++i) {
        buffer[i] = qFromBigEndian<quint32>(pSeed + i * 4);
    }
    for (qint32 i = 17; i < 521; ++i) {
        buffer[i] = (buffer[i - 17] << 23U) ^
                    (buffer[i - 16] >> 9U) ^ buffer[i - 1];
    }

    for (qint32 i = 0; i < 4; ++i) advanceJunkState(buffer);

    qint32 nWord = 0;
    qint32 nByte = 0;

    const qint64 nSkip = nDataOffset % RVZ_SECTOR_SIZE;
    for (qint64 i = 0; i < nSkip; ++i) {
        Q_UNUSED(nextJunkByte(buffer, &nWord, &nByte))
    }
    for (qint64 i = 0; i < nOutputSize; ++i) {
        pOutput[i] = static_cast<char>(nextJunkByte(buffer, &nWord, &nByte));
    }
    return true;
}
}  // namespace

XRVZArchive::XRVZArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XRVZArchive::~XRVZArchive()
{
}

bool XRVZArchive::rangeWithin(qint64 nTotalSize, qint64 nOffset,
                               qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

bool XRVZArchive::rangesOverlap(qint64 nOffset1, qint64 nSize1,
                                 qint64 nOffset2, qint64 nSize2)
{
    if ((nSize1 <= 0) || (nSize2 <= 0)) return false;
    return (nOffset1 < nOffset2 + nSize2) &&
           (nOffset2 < nOffset1 + nSize1);
}

bool XRVZArchive::addChecked(qint64 nLeft, qint64 nRight,
                              qint64 *pnResult)
{
    if (!pnResult || (nLeft < 0) || (nRight < 0) ||
        (nLeft > (std::numeric_limits<qint64>::max)() - nRight)) {
        return false;
    }
    *pnResult = nLeft + nRight;
    return true;
}

bool XRVZArchive::readBE64(const uchar *pData, qint64 *pnResult)
{
    if (!pData || !pnResult) return false;
    const quint64 nValue = qFromBigEndian<quint64>(pData);
    if (nValue > quint64((std::numeric_limits<qint64>::max)())) return false;
    *pnResult = qint64(nValue);
    return true;
}

bool XRVZArchive::decompressZstd(const QByteArray &baCompressed,
                                  qint64 nExpectedSize, QByteArray *pResult,
                                  PDSTRUCT *pPdStruct)
{
    if (pResult) pResult->clear();
    if (!pResult || baCompressed.isEmpty() || (nExpectedSize < 1) ||
        (nExpectedSize > RVZ_MAX_PACKED_GROUP_SIZE) ||
        (nExpectedSize > (std::numeric_limits<qint32>::max)()) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QByteArray baInput = baCompressed;
    QByteArray baOutput;
    QBuffer input(&baInput);
    QBuffer output(&baOutput);
    if (!input.open(QIODevice::ReadOnly) ||
        !output.open(QIODevice::ReadWrite)) {
        return false;
    }

    DATAPROCESS_STATE state = {};
    state.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                               nExpectedSize);
    // XZstdDecoder uses the output-limit property to bound the zstd history
    // window.  The exact expected-size property still rejects extra bytes.
    state.mapUnpackProperties.insert(UNPACK_PROP_MAX_OUTPUT_SIZE,
                                     qMax<qint64>(1024, nExpectedSize));
    state.pDeviceInput = &input;
    state.pDeviceOutput = &output;
    state.nInputOffset = 0;
    state.nInputLimit = baInput.size();
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;

    const bool bResult = XZstdDecoder::decompress(&state, pPdStruct) &&
                         (state.nCountInput == baInput.size()) &&
                         (state.nCountOutput == nExpectedSize) &&
                         (baOutput.size() == nExpectedSize) &&
                         isPdStructNotCanceled(pPdStruct);
    input.close();
    output.close();
    if (!bResult) return false;
    *pResult = baOutput;
    return true;
}

bool XRVZArchive::decodeTable(const QByteArray &baCompressed,
                               qint64 nExpectedSize,
                               quint32 nCompression, QByteArray *pResult,
                               PDSTRUCT *pPdStruct)
{
    if (pResult) pResult->clear();
    if (!pResult || (nExpectedSize < 1) ||
        (nExpectedSize > RVZ_MAX_PACKED_GROUP_SIZE) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    if (nCompression == RVZ_COMPRESSION_NONE) {
        if (baCompressed.size() != nExpectedSize) return false;
        *pResult = baCompressed;
        return true;
    }
    if (nCompression != RVZ_COMPRESSION_ZSTD) return false;
    return decompressZstd(baCompressed, nExpectedSize, pResult, pPdStruct);
}

bool XRVZArchive::decodeRvzPacked(const QByteArray &baPacked,
                                   qint64 nExpectedSize,
                                   qint64 nDataOffset, QByteArray *pResult,
                                   PDSTRUCT *pPdStruct)
{
    if (pResult) pResult->clear();
    if (!pResult || (nExpectedSize < 1) ||
        (nExpectedSize > RVZ_MAX_CHUNK_SIZE) || (nDataOffset < 0) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QByteArray baResult(qint32(nExpectedSize), 0);
    if (baResult.size() != nExpectedSize) return false;
    const uchar *pData =
        reinterpret_cast<const uchar *>(baPacked.constData());
    qint64 nInputOffset = 0;
    qint64 nOutputOffset = 0;

    while ((nInputOffset < baPacked.size()) &&
           isPdStructNotCanceled(pPdStruct)) {
        if (baPacked.size() - nInputOffset < 4) return false;
        const quint32 nSizeWord =
            qFromBigEndian<quint32>(pData + nInputOffset);
        nInputOffset += 4;
        const qint64 nSize = qint64(nSizeWord & RVZ_GROUP_SIZE_MASK);
        if ((nSize == 0) || (nSize > nExpectedSize - nOutputOffset)) {
            return false;
        }

        if (nSizeWord & RVZ_GROUP_COMPRESSED) {
            if (baPacked.size() - nInputOffset < 68) return false;
            qint64 nAbsoluteOffset = 0;
            if (!addChecked(nDataOffset, nOutputOffset, &nAbsoluteOffset) ||
                !fillPackedJunk(pData + nInputOffset, nAbsoluteOffset,
                                baResult.data() + nOutputOffset, nSize)) {
                return false;
            }
            nInputOffset += 68;
        } else {
            if (nSize > baPacked.size() - nInputOffset) return false;
            memcpy(baResult.data() + nOutputOffset,
                   baPacked.constData() + nInputOffset, size_t(nSize));
            nInputOffset += nSize;
        }
        nOutputOffset += nSize;
    }

    if ((nInputOffset != baPacked.size()) ||
        (nOutputOffset != nExpectedSize) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    *pResult = baResult;
    return true;
}

bool XRVZArchive::parseContext(QIODevice *pDevice, CONTEXT *pContext,
                                PDSTRUCT *pPdStruct)
{
    if (pContext) *pContext = CONTEXT();
    QIODevice *guardedDevice = pDevice;
    RvzDevicePositionGuard positionGuard(guardedDevice);
    if (!guardedDevice || !pContext || !positionGuard.isValid() ||
        !guardedDevice->isOpen() || !guardedDevice->isReadable() ||
        guardedDevice->isSequential() ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint64 nSourceSize = guardedDevice->size();
    if (!guardedDevice || (nSourceSize < RVZ_FILE_HEADER_SIZE)) return false;
    const QByteArray baFileHeader = XBinary::read_array_process(
        guardedDevice, 0, RVZ_FILE_HEADER_SIZE, pPdStruct);
    if (!guardedDevice || (baFileHeader.size() != RVZ_FILE_HEADER_SIZE) ||
        (memcmp(baFileHeader.constData(), "RVZ\x01", 4) != 0)) {
        return false;
    }
    const uchar *pFileHeader = reinterpret_cast<const uchar *>(
        baFileHeader.constData());
    const quint32 nVersion = qFromBigEndian<quint32>(pFileHeader + 4);
    const quint32 nCompatibleVersion =
        qFromBigEndian<quint32>(pFileHeader + 8);
    const quint32 nHeaderSize = qFromBigEndian<quint32>(pFileHeader + 12);
    qint64 nIsoSize = 0;
    qint64 nDeclaredFileSize = 0;
    if ((nVersion != RVZ_VERSION_1) ||
        (nCompatibleVersion > RVZ_VERSION_1) ||
        (nHeaderSize < RVZ_HEADER_MIN_SIZE) ||
        (nHeaderSize > RVZ_HEADER_MAX_SIZE) ||
        !readBE64(pFileHeader + 0x24, &nIsoSize) ||
        !readBE64(pFileHeader + 0x2c, &nDeclaredFileSize) ||
        (nDeclaredFileSize != nSourceSize) || (nIsoSize < RVZ_DISC_HEADER_SIZE) ||
        (nIsoSize > RVZ_GAMECUBE_MAX_ISO_SIZE) ||
        (QCryptographicHash::hash(baFileHeader.left(0x34),
                                  QCryptographicHash::Sha1) !=
         baFileHeader.mid(0x34, 20))) {
        return false;
    }

    qint64 nHeaderEnd = 0;
    if (!addChecked(RVZ_FILE_HEADER_SIZE, nHeaderSize, &nHeaderEnd) ||
        !rangeWithin(nSourceSize, RVZ_FILE_HEADER_SIZE, nHeaderSize)) {
        return false;
    }
    const QByteArray baHeader = XBinary::read_array_process(
        guardedDevice, RVZ_FILE_HEADER_SIZE, nHeaderSize, pPdStruct);
    if (!guardedDevice || (baHeader.size() != nHeaderSize) ||
        (QCryptographicHash::hash(baHeader, QCryptographicHash::Sha1) !=
         baFileHeader.mid(0x10, 20))) {
        return false;
    }

    const uchar *pHeader = reinterpret_cast<const uchar *>(
        baHeader.constData());
    const quint32 nDiscType = qFromBigEndian<quint32>(pHeader);
    const quint32 nCompression = qFromBigEndian<quint32>(pHeader + 4);
    const quint32 nChunkSize = qFromBigEndian<quint32>(pHeader + 12);
    const quint32 nPartitions = qFromBigEndian<quint32>(pHeader + 0x90);
    const quint32 nRawCount = qFromBigEndian<quint32>(pHeader + 0xb4);
    const quint32 nGroupCount = qFromBigEndian<quint32>(pHeader + 0xc4);
    const quint32 nRawTableSize = qFromBigEndian<quint32>(pHeader + 0xc0);
    const quint32 nGroupTableSize = qFromBigEndian<quint32>(pHeader + 0xd0);
    const quint8 nCompressorDataSize = pHeader[0xd4];
    qint64 nRawTableOffset = 0;
    qint64 nGroupTableOffset = 0;
    if ((nDiscType != 1) || (nPartitions != 0) ||
        ((nCompression != RVZ_COMPRESSION_NONE) &&
         (nCompression != RVZ_COMPRESSION_ZSTD)) ||
        (nCompressorDataSize != 0) || !isGameCubeMagic(baHeader.mid(0x10,
                                                                     RVZ_DISC_HEADER_SIZE)) ||
        (nChunkSize < RVZ_SECTOR_SIZE) ||
        (nChunkSize > RVZ_MAX_CHUNK_SIZE) ||
        ((nChunkSize < 0x200000U && !isPowerOfTwo(nChunkSize)) ||
         (nChunkSize >= 0x200000U && (nChunkSize % 0x200000U))) ||
        !readBE64(pHeader + 0xb8, &nRawTableOffset) ||
        !readBE64(pHeader + 0xc8, &nGroupTableOffset) ||
        (nRawCount == 0) || (nRawCount > RVZ_MAX_RAW_ENTRIES) ||
        (nGroupCount == 0) || (nGroupCount > RVZ_MAX_GROUP_ENTRIES) ||
        (nRawTableSize == 0) ||
        (nRawTableSize > RVZ_MAX_TABLE_COMPRESSED_SIZE) ||
        (nGroupTableSize == 0) ||
        (nGroupTableSize > RVZ_MAX_TABLE_COMPRESSED_SIZE) ||
        !rangeWithin(nSourceSize, nRawTableOffset, nRawTableSize) ||
        !rangeWithin(nSourceSize, nGroupTableOffset, nGroupTableSize) ||
        (nRawTableOffset < nHeaderEnd) ||
        (nGroupTableOffset < nHeaderEnd) ||
        rangesOverlap(nRawTableOffset, nRawTableSize, nGroupTableOffset,
                      nGroupTableSize)) {
        return false;
    }

    const qint64 nRawTableDecodedSize = qint64(nRawCount) *
                                         RVZ_RAW_ENTRY_SIZE;
    const qint64 nGroupTableDecodedSize = qint64(nGroupCount) *
                                           RVZ_GROUP_ENTRY_SIZE;
    if ((nRawTableDecodedSize > RVZ_MAX_PACKED_GROUP_SIZE) ||
        (nGroupTableDecodedSize > RVZ_MAX_PACKED_GROUP_SIZE)) {
        return false;
    }
    const QByteArray baRawTableCompressed = XBinary::read_array_process(
        guardedDevice, nRawTableOffset, nRawTableSize, pPdStruct);
    const QByteArray baGroupTableCompressed = XBinary::read_array_process(
        guardedDevice, nGroupTableOffset, nGroupTableSize, pPdStruct);
    QByteArray baRawTable;
    QByteArray baGroupTable;
    if (!guardedDevice ||
        (baRawTableCompressed.size() != nRawTableSize) ||
        (baGroupTableCompressed.size() != nGroupTableSize) ||
        !decodeTable(baRawTableCompressed, nRawTableDecodedSize,
                     nCompression, &baRawTable, pPdStruct) ||
        !decodeTable(baGroupTableCompressed, nGroupTableDecodedSize,
                     nCompression, &baGroupTable, pPdStruct) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QVector<RAW_DATA_ENTRY> listRawData;
    listRawData.resize(qint32(nRawCount));
    const uchar *pRawTable = reinterpret_cast<const uchar *>(
        baRawTable.constData());
    qint64 nExpectedOutputOffset = RVZ_DISC_HEADER_SIZE;
    quint64 nExpectedGroupIndex = 0;
    for (quint32 i = 0; i < nRawCount; ++i) {
        const uchar *pEntry = pRawTable + qint64(i) * RVZ_RAW_ENTRY_SIZE;
        RAW_DATA_ENTRY entry = {};
        if (!readBE64(pEntry, &entry.nOffset) ||
            !readBE64(pEntry + 8, &entry.nSize) ||
            (entry.nSize < 1) ||
            !addChecked(entry.nOffset, entry.nSize, &entry.nOutputSize) ||
            (entry.nOutputSize > nIsoSize) ||
            (entry.nOffset > nExpectedOutputOffset) ||
            ((i > 0) && (entry.nOffset != nExpectedOutputOffset)) ||
            (entry.nOutputSize <= nExpectedOutputOffset)) {
            return false;
        }
        const qint64 nEndOffset = entry.nOutputSize;
        entry.nAlignedOffset = entry.nOffset -
                               (entry.nOffset % RVZ_SECTOR_SIZE);
        entry.nLogicalSize = nEndOffset - entry.nAlignedOffset;
        entry.nOutputSkip = nExpectedOutputOffset - entry.nAlignedOffset;
        entry.nOutputSize = nEndOffset - nExpectedOutputOffset;
        entry.nGroupIndex = qFromBigEndian<quint32>(pEntry + 16);
        entry.nGroupCount = qFromBigEndian<quint32>(pEntry + 20);
        const qint64 nRequiredGroups =
            (entry.nLogicalSize + nChunkSize - 1) / nChunkSize;
        if ((entry.nLogicalSize < 1) || (entry.nOutputSkip < 0) ||
            (entry.nOutputSkip >= entry.nLogicalSize) ||
            (entry.nOutputSize < 1) ||
            (nRequiredGroups < 1) ||
            (quint64(entry.nGroupIndex) != nExpectedGroupIndex) ||
            (entry.nGroupCount != nRequiredGroups) ||
            (nExpectedGroupIndex > nGroupCount) ||
            (quint64(entry.nGroupCount) >
             quint64(nGroupCount) - nExpectedGroupIndex)) {
            return false;
        }
        nExpectedGroupIndex += entry.nGroupCount;
        nExpectedOutputOffset = nEndOffset;
        listRawData[qint32(i)] = entry;
    }
    if ((nExpectedOutputOffset != nIsoSize) ||
        (nExpectedGroupIndex != nGroupCount)) {
        return false;
    }

    QVector<GROUP_ENTRY> listGroups;
    listGroups.resize(qint32(nGroupCount));
    const uchar *pGroupTable = reinterpret_cast<const uchar *>(
        baGroupTable.constData());
    for (quint32 i = 0; i < nGroupCount; ++i) {
        const uchar *pEntry = pGroupTable + qint64(i) * RVZ_GROUP_ENTRY_SIZE;
        const quint32 nDataOffset4 = qFromBigEndian<quint32>(pEntry);
        const quint32 nDataSizeWord = qFromBigEndian<quint32>(pEntry + 4);
        GROUP_ENTRY entry = {};
        entry.nDataOffset = qint64(nDataOffset4) * 4;
        entry.nDataSize = nDataSizeWord & RVZ_GROUP_SIZE_MASK;
        entry.nPackedSize = qFromBigEndian<quint32>(pEntry + 8);
        entry.bCompressed = (nDataSizeWord & RVZ_GROUP_COMPRESSED) != 0;
        listGroups[qint32(i)] = entry;
    }

    bool bHasPacking = false;
    for (const RAW_DATA_ENTRY &rawData : listRawData) {
        for (quint32 i = 0; i < rawData.nGroupCount; ++i) {
            const quint32 nGroupIndex = rawData.nGroupIndex + i;
            GROUP_ENTRY &group = listGroups[qint32(nGroupIndex)];
            const qint64 nGroupOffset = qint64(i) * nChunkSize;
            const qint64 nExpectedGroupSize = qMin<qint64>(
                nChunkSize, rawData.nLogicalSize - nGroupOffset);
            if (nExpectedGroupSize < 1) return false;

            if (group.nDataSize == 0) {
                if (group.nPackedSize != 0) return false;
                continue;
            }
            const qint64 nIntermediateSize = group.nPackedSize
                ? qint64(group.nPackedSize) : nExpectedGroupSize;
            if ((nIntermediateSize < 1) ||
                (nIntermediateSize > RVZ_MAX_PACKED_GROUP_SIZE) ||
                (group.bCompressed &&
                 (nCompression != RVZ_COMPRESSION_ZSTD)) ||
                (!group.bCompressed &&
                 (group.nDataSize != nIntermediateSize)) ||
                (group.bCompressed &&
                 (group.nDataSize > RVZ_MAX_COMPRESSED_GROUP_SIZE)) ||
                !rangeWithin(nSourceSize, group.nDataOffset,
                             group.nDataSize) ||
                rangesOverlap(group.nDataOffset, group.nDataSize, 0,
                              nHeaderEnd) ||
                rangesOverlap(group.nDataOffset, group.nDataSize,
                              nRawTableOffset, nRawTableSize) ||
                rangesOverlap(group.nDataOffset, group.nDataSize,
                              nGroupTableOffset, nGroupTableSize)) {
                return false;
            }
            bHasPacking = bHasPacking || (group.nPackedSize != 0);
        }
    }

    pContext->baDiscHeader = baHeader.mid(0x10, RVZ_DISC_HEADER_SIZE);
    pContext->listRawData = listRawData;
    pContext->listGroups = listGroups;
    pContext->nSourceSize = nSourceSize;
    pContext->nIsoSize = nIsoSize;
    pContext->nChunkSize = nChunkSize;
    pContext->nCompression = nCompression;
    pContext->sReportedMethod = (nCompression == RVZ_COMPRESSION_ZSTD)
        ? QStringLiteral("RVZ / Zstandard")
        : QStringLiteral("RVZ / Store");
    if (bHasPacking) {
        pContext->sReportedMethod += QStringLiteral(" + RVZ packing");
    }
    return guardedDevice && positionGuard.restore() &&
           isPdStructNotCanceled(pPdStruct);
}

bool XRVZArchive::decodeGroup(QIODevice *pDevice, const CONTEXT &context,
                               const GROUP_ENTRY &group,
                               qint64 nExpectedSize, qint64 nDataOffset,
                               QByteArray *pResult, PDSTRUCT *pPdStruct)
{
    if (pResult) pResult->clear();
    QIODevice *guardedDevice = pDevice;
    if (!guardedDevice || !pResult || (nExpectedSize < 1) ||
        (nExpectedSize > RVZ_MAX_CHUNK_SIZE) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    if (group.nDataSize == 0) {
        QByteArray baZeroes(qint32(nExpectedSize), 0);
        if (baZeroes.size() != nExpectedSize) return false;
        *pResult = baZeroes;
        return true;
    }

    const QByteArray baStored = XBinary::read_array_process(
        guardedDevice, group.nDataOffset, group.nDataSize, pPdStruct);
    if (!guardedDevice || (baStored.size() != group.nDataSize)) return false;

    const qint64 nIntermediateSize = group.nPackedSize
        ? qint64(group.nPackedSize) : nExpectedSize;
    QByteArray baIntermediate;
    if (group.bCompressed) {
        if ((context.nCompression != RVZ_COMPRESSION_ZSTD) ||
            !decompressZstd(baStored, nIntermediateSize, &baIntermediate,
                            pPdStruct)) {
            return false;
        }
    } else {
        if (baStored.size() != nIntermediateSize) return false;
        baIntermediate = baStored;
    }

    if (group.nPackedSize != 0) {
        return decodeRvzPacked(baIntermediate, nExpectedSize, nDataOffset,
                               pResult, pPdStruct);
    }
    if (baIntermediate.size() != nExpectedSize) return false;
    *pResult = baIntermediate;
    return true;
}

bool XRVZArchive::writeStage(
    QIODevice *pStage, const char *pData, qint64 nSize,
    const QSharedPointer<OUTPUT_BUDGET> &spBudget, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedStage = pStage;
    if (!guardedStage || (nSize < 0) || ((nSize > 0) && !pData)) {
        return false;
    }
    qint64 nWritten = 0;
    while ((nWritten < nSize) && isPdStructNotCanceled(pPdStruct)) {
        const qint64 nChunk = qMin<qint64>(0x10000, nSize - nWritten);
        if (spBudget && !spBudget->debit(nChunk) &&
            spBudget->isEnforcing()) {
            setPdStructErrorString(
                pPdStruct, QStringLiteral("Unpacked output exceeds the configured limit"));
            return false;
        }
        qint64 nChunkWritten = 0;
        while ((nChunkWritten < nChunk) && isPdStructNotCanceled(pPdStruct)) {
            const qint64 nResult = guardedStage->write(
                pData + nWritten + nChunkWritten, nChunk - nChunkWritten);
            if (!guardedStage || (nResult <= 0) ||
                (nResult > nChunk - nChunkWritten)) {
                return false;
            }
            nChunkWritten += nResult;
        }
        if (nChunkWritten != nChunk) return false;
        nWritten += nChunk;
    }
    return (nWritten == nSize) && isPdStructNotCanceled(pPdStruct);
}

bool XRVZArchive::writeZeroes(
    QIODevice *pStage, qint64 nSize,
    const QSharedPointer<OUTPUT_BUDGET> &spBudget, PDSTRUCT *pPdStruct)
{
    if (nSize < 0) return false;
    const QByteArray baZeroes(0x10000, 0);
    qint64 nWritten = 0;
    while ((nWritten < nSize) && isPdStructNotCanceled(pPdStruct)) {
        const qint64 nChunk = qMin<qint64>(baZeroes.size(), nSize - nWritten);
        if (!writeStage(pStage, baZeroes.constData(), nChunk, spBudget,
                        pPdStruct)) {
            return false;
        }
        nWritten += nChunk;
    }
    return (nWritten == nSize) && isPdStructNotCanceled(pPdStruct);
}

bool XRVZArchive::isValid(PDSTRUCT *pPdStruct)
{
    UNPACK_STATE state = {};
    const bool bResult = initUnpack(&state, QMap<UNPACK_PROP, QVariant>(),
                                    pPdStruct);
    const bool bFinished = finishUnpack(&state, nullptr);
    return bResult && bFinished;
}

bool XRVZArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRVZArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary::FT XRVZArchive::getFileType()
{
    return FT_RVZ;
}

XBinary::MODE XRVZArchive::getMode()
{
    return MODE_DATA;
}

qint32 XRVZArchive::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XRVZArchive::getEndian()
{
    return ENDIAN_BIG;
}

QString XRVZArchive::getArch()
{
    return QStringLiteral("PowerPC");
}

QString XRVZArchive::getFileFormatExt()
{
    return QStringLiteral("rvz");
}

QString XRVZArchive::getFileFormatExtsString()
{
    return QStringLiteral("Dolphin RVZ image (*.rvz)");
}

QString XRVZArchive::getMIMEString()
{
    return QStringLiteral("application/x-dolphin-rvz");
}

qint64 XRVZArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(getDevice(), &context, pPdStruct)
        ? context.nSourceSize : 0;
}

XBinary::OSNAME XRVZArchive::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QString XRVZArchive::getVersion()
{
    QIODevice *guardedDevice = getDevice();
    if (!guardedDevice) return QString();
    const QByteArray baHeader = XBinary::read_array_process(
        guardedDevice, 0, 8, nullptr);
    if ((baHeader.size() == 8) &&
        (memcmp(baHeader.constData(), "RVZ\x01", 4) == 0) &&
        (qFromBigEndian<quint32>(
             reinterpret_cast<const uchar *>(baHeader.constData()) + 4) ==
         RVZ_VERSION_1)) {
        return QStringLiteral("1.0");
    }
    return QString();
}

QList<QString> XRVZArchive::getSearchSignatures()
{
    return {QStringLiteral("'RVZ'01")};
}

XBinary *XRVZArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                      XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XRVZArchive(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant>
XRVZArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XRVZArchive::initUnpack(
    UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedSource ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) goto failed;
    if (!parseContext(guardedSource, pContext, pPdStruct) || !guardedSource ||
        !isUnpackOutputSizeAllowed(mapProperties, pContext->nIsoSize)) {
        goto failed;
    }

    {
        QString sBase = fixFileName(getDeviceFileBaseName(guardedSource));
        if (sBase.isEmpty()) sBase = QStringLiteral("gamecube-disc");
        pContext->sFileName = sBase + QStringLiteral(".iso");
        pState->mapUnpackProperties = mapProperties;
        pState->mapArchiveProperties.insert(
            FPART_PROP_INFO,
            QStringLiteral("GameCube RVZ v1: %1, %2 KiB chunks")
                .arg(pContext->sReportedMethod)
                .arg(pContext->nChunkSize / 1024));
        pState->nCurrentOffset = 0;
        pState->nTotalSize = pContext->nSourceSize;
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 1;
        pState->pContext = pContext;
    }
    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        goto failed;
    }
    return true;

failed:
    releaseUnpackSource(pState);
    delete pContext;
    *pState = UNPACK_STATE();
    return false;
}

XBinary::ARCHIVERECORD XRVZArchive::infoCurrent(UNPACK_STATE *pState,
                                                 PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        (pState->nCurrentIndex != 0) ||
        (pState->nNumberOfRecords != 1) ||
        !isUnpackSourceCurrent(pState, pPdStruct)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pContext->nSourceSize != pState->nTotalSize) ||
        (pContext->nIsoSize < RVZ_DISC_HEADER_SIZE) ||
        pContext->sFileName.isEmpty()) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD record = {};
    record.nStreamOffset = 0;
    record.nStreamSize = pContext->nSourceSize;
    record.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pContext->nSourceSize);
    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                pContext->nIsoSize);
    record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                pContext->sReportedMethod);
    record.mapProperties.insert(FPART_PROP_FILEMODE, (quint32)0644);
    record.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (!markArchiveStreamRecord(&record, 0)) return ARCHIVERECORD();
    return record;
}

bool XRVZArchive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                                PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QIODevice *guardedOutput = pDevice;
    QIODevice *guardedSource = getDevice();
    if (!operationGuard.isAcquired() || !pState ||
        !guardedOutput || !guardedSource || (pState->nCurrentIndex != 0) ||
        (pState->nNumberOfRecords != 1) ||
        !isUnpackOutputSupported(guardedOutput) ||
        devicesAlias(guardedSource, guardedOutput) ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pContext->nSourceSize != pState->nTotalSize) ||
        (pContext->nIsoSize < RVZ_DISC_HEADER_SIZE) ||
        !isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                   pContext->nIsoSize)) {
        return false;
    }
    if (pState->spOutputBudget &&
        !pState->spOutputBudget->beginEntry(0, pContext->sFileName) &&
        pState->spOutputBudget->isEnforcing()) {
        setPdStructErrorString(
            pPdStruct, QStringLiteral("Unpacked output exceeds the configured limit"));
        return false;
    }

    std::unique_ptr<QIODevice> pStage(
        createFileBuffer(pContext->nIsoSize, pPdStruct));
    if (!pStage || !pStage->seek(0) ||
        !writeStage(pStage.get(), pContext->baDiscHeader.constData(),
                    pContext->baDiscHeader.size(), pState->spOutputBudget,
                    pPdStruct)) {
        return false;
    }

    for (const RAW_DATA_ENTRY &rawData : pContext->listRawData) {
        qint64 nSkip = rawData.nOutputSkip;
        qint64 nRemaining = rawData.nOutputSize;
        for (quint32 i = 0; i < rawData.nGroupCount; ++i) {
            if (!guardedOutput || !guardedSource ||
                !isPdStructNotCanceled(pPdStruct)) {
                return false;
            }
            const qint64 nGroupOffset = qint64(i) * pContext->nChunkSize;
            const qint64 nExpectedGroupSize = qMin<qint64>(
                pContext->nChunkSize, rawData.nLogicalSize - nGroupOffset);
            qint64 nDiscGroupOffset = 0;
            if ((nExpectedGroupSize < 1) ||
                !addChecked(rawData.nAlignedOffset, nGroupOffset,
                            &nDiscGroupOffset) ||
                !rangeWithin(pContext->nIsoSize, nDiscGroupOffset,
                             nExpectedGroupSize)) {
                return false;
            }
            const GROUP_ENTRY &group = pContext->listGroups.at(
                qint32(rawData.nGroupIndex + i));
            QByteArray baGroup;
            if (!decodeGroup(guardedSource, *pContext, group,
                             nExpectedGroupSize, nDiscGroupOffset, &baGroup,
                             pPdStruct) || !guardedOutput || !guardedSource ||
                (baGroup.size() != nExpectedGroupSize)) {
                return false;
            }
            const qint64 nSkipped = qMin(nSkip, nExpectedGroupSize);
            nSkip -= nSkipped;
            const qint64 nAvailable = nExpectedGroupSize - nSkipped;
            const qint64 nToWrite = qMin(nRemaining, nAvailable);
            if ((nToWrite > 0) &&
                !writeStage(pStage.get(), baGroup.constData() + nSkipped,
                            nToWrite, pState->spOutputBudget, pPdStruct)) {
                return false;
            }
            nRemaining -= nToWrite;
        }
        if ((nSkip != 0) || (nRemaining != 0)) return false;
    }

    if (!guardedOutput || !guardedSource ||
        (pStage->pos() != pContext->nIsoSize) ||
        (pStage->size() != pContext->nIsoSize) ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const bool bResult = publishUnpackOutput(pStage.get(), guardedOutput,
                                             pState, pPdStruct);
    if (bResult) pState->nCurrentOffset = pContext->nSourceSize;
    return bResult;
}

bool XRVZArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        (pState->nCurrentIndex != 0) ||
        (pState->nNumberOfRecords != 1) ||
        !isUnpackSourceCurrent(pState, pPdStruct)) {
        return false;
    }
    pState->nCurrentIndex = 1;
    pState->nCurrentOffset = pState->nTotalSize;
    return false;
}

bool XRVZArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}

QList<XBinary::FPART_PROP> XRVZArchive::getAvailableFPARTProperties()
{
    QList<FPART_PROP> listResult = XArchive::getAvailableFPARTProperties();
    const QList<FPART_PROP> listRequired = {
        FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE,
        FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_REPORTEDMETHOD,
        FPART_PROP_FILEMODE, FPART_PROP_ISFOLDER};
    for (FPART_PROP prop : listRequired) {
        if (!listResult.contains(prop)) listResult.append(prop);
    }
    return listResult;
}
