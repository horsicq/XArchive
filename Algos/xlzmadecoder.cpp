/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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
#include "xlzmadecoder.h"
#include "algo_utils.h"
#include "xbranchdecoder.h"
#include "xalgo_local.h"
#include <QBuffer>
#include <QCryptographicHash>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <new>

namespace {
// All LZMA entry points share one explicit dictionary ceiling. This prevents
// crafted raw properties from requesting multi-gigabyte SDK allocations while
// keeping the same policy for raw LZMA, raw LZMA2, and XZ Blocks.
const qint64 LZMA_MAX_DICTIONARY_SIZE = 512LL * 1024 * 1024;
const qint32 LZMA_MIN_BUFFER_SIZE = 0x1000;
const qint32 LZMA_MAX_BUFFER_SIZE = 0x100000;
const qint64 XZ_MAX_PREFILTER_OUTPUT_SIZE = 512LL * 1024 * 1024;
const qint64 XZ_MAX_INDEX_SIZE = 64LL * 1024 * 1024;
const quint64 XZ_MAX_INDEX_RECORDS = 1000000;

struct XZIndexRecord {
    quint64 nUnpaddedSize;
    quint64 nUncompressedSize;
};

struct XZBlockDescriptor {
    qint64 nDataOffset;
    qint64 nDataSize;
    qint64 nUncompressedSize;
    quint8 nLZMA2PropsByte;
    QList<QPair<quint64, QByteArray>> listPrefilters;
    QByteArray baStoredCheck;
};

struct XZStreamDescriptor {
    qint64 nStartOffset;
    qint64 nEndOffset;
    quint8 nCheckType;
    QList<XZBlockDescriptor> listBlocks;
};

bool isLZMAAllocationAllowed(const XBinary::DATAPROCESS_STATE *pState, quint64 nSize)
{
    if (!pState || (nSize > (quint64)LZMA_MAX_DICTIONARY_SIZE)) {
        return false;
    }

    qint64 nOutputLimit = -1;
    return XBinary::getUnpackOutputLimit(pState->mapUnpackProperties, &nOutputLimit) && ((nOutputLimit < 0) || (nSize <= (quint64)nOutputLimit));
}

bool isLZMA2DictionaryAllowed(const XBinary::DATAPROCESS_STATE *pState, quint8 nProperty)
{
    if (nProperty > 40) return false;

    const quint64 nDictionarySize = (nProperty == 40) ? Q_UINT64_C(0xFFFFFFFF) : (((quint64)2 | (nProperty & 1)) << (nProperty / 2 + 11));
    return isLZMAAllocationAllowed(pState, nDictionarySize);
}

bool getLzmaAllocationSize(const CLzmaProps &properties, quint64 *pnSize)
{
    if (!pnSize || (properties.lc > 8) || (properties.lp > 4) || ((quint32)properties.lc + properties.lp > 12)) {
        return false;
    }

    quint64 nMask = (Q_UINT64_C(1) << 12) - 1;
    if (properties.dicSize >= (Q_UINT64_C(1) << 30)) {
        nMask = (Q_UINT64_C(1) << 22) - 1;
    } else if (properties.dicSize >= (Q_UINT64_C(1) << 22)) {
        nMask = (Q_UINT64_C(1) << 20) - 1;
    }
    const quint64 nDictionarySize = ((quint64)properties.dicSize + nMask) & ~nMask;
    const quint64 nNumberOfProbabilities = Q_UINT64_C(1984) + (Q_UINT64_C(0x300) << (properties.lc + properties.lp));
    const quint64 nProbabilitiesSize = nNumberOfProbabilities * sizeof(CLzmaProb);
    if (nDictionarySize > (std::numeric_limits<quint64>::max)() - nProbabilitiesSize) {
        return false;
    }
    *pnSize = nDictionarySize + nProbabilitiesSize;
    return true;
}

bool getLzmaBufferSize(XBinary::PDSTRUCT *pPdStruct, qint32 *pnBufferSize)
{
    if (!pnBufferSize) return false;

    const qint32 nRequestedBufferSize = XBinary::getBufferSize(pPdStruct);
    if (nRequestedBufferSize <= 0) return false;

    *pnBufferSize = qBound(LZMA_MIN_BUFFER_SIZE, nRequestedBufferSize, LZMA_MAX_BUFFER_SIZE);
    return true;
}

bool getLzmaDecoderMemorySize(const CLzmaProps &properties, qint32 nBufferSize, quint64 *pnSize)
{
    if (!pnSize || (nBufferSize < LZMA_MIN_BUFFER_SIZE) || (nBufferSize > LZMA_MAX_BUFFER_SIZE)) {
        return false;
    }

    quint64 nSdkAllocationSize = 0;
    if (!getLzmaAllocationSize(properties, &nSdkAllocationSize)) {
        return false;
    }

    const quint64 nIoBufferSize = static_cast<quint64>(nBufferSize) * 2;
    if (nSdkAllocationSize > (std::numeric_limits<quint64>::max)() - nIoBufferSize) {
        return false;
    }

    *pnSize = nSdkAllocationSize + nIoBufferSize;
    return true;
}

bool reserveLzmaAllocation(const XBinary::DATAPROCESS_STATE *pState, const CLzmaProps &properties, XBinary::UNPACK_MEMORY_RESERVATION *pReservation)
{
    quint64 nSize = 0;
    return pState && pReservation && getLzmaAllocationSize(properties, &nSize) && (nSize <= (quint64)(std::numeric_limits<qint64>::max)()) &&
           pReservation->acquire(pState->mapUnpackProperties, (qint64)nSize);
}

bool reserveLzmaDecoderMemory(const XBinary::DATAPROCESS_STATE *pState, const CLzmaProps &properties, qint32 nBufferSize,
                              XBinary::UNPACK_MEMORY_RESERVATION *pReservation)
{
    quint64 nSize = 0;
    return pState && pReservation && getLzmaDecoderMemorySize(properties, nBufferSize, &nSize) && (nSize <= (quint64)(std::numeric_limits<qint64>::max)()) &&
           pReservation->acquire(pState->mapUnpackProperties, (qint64)nSize);
}

bool getLzma2Properties(quint8 nProperty, CLzmaProps *pProperties)
{
    if (!pProperties || (nProperty > 40)) return false;
    *pProperties = CLzmaProps();
    pProperties->lc = 4;
    pProperties->dicSize = (nProperty == 40) ? 0xFFFFFFFFU : (quint32)(((quint64)2 | (nProperty & 1)) << (nProperty / 2 + 11));
    return true;
}

quint32 readLE32(const char *pData)
{
    return (quint32)(quint8)pData[0] | ((quint32)(quint8)pData[1] << 8) | ((quint32)(quint8)pData[2] << 16) | ((quint32)(quint8)pData[3] << 24);
}

quint32 xzCRC32(const char *pData, qint32 nSize)
{
    return XBinary::_getCRC32(pData, nSize, 0xFFFFFFFF, XBinary::_getCRC32Table_EDB88320()) ^ 0xFFFFFFFF;
}

std::array<quint64, 256> xzBuildCRC64Table()
{
    const quint64 XZ_CRC64_POLYNOMIAL = Q_UINT64_C(0xC96C5795D7870F42);
    std::array<quint64, 256> result = {};
    quint64 nTableIndex = 0;
    for (quint64 &nTableEntry : result) {
        quint64 nEntry = nTableIndex++;
        for (qint32 nBit = 0; nBit < 8; nBit++) {
            const quint64 nMask = (quint64)0 - (nEntry & 1);
            nEntry = (nEntry >> 1) ^ (XZ_CRC64_POLYNOMIAL & nMask);
        }
        nTableEntry = nEntry;
    }
    return result;
}

quint64 xzCRC64Update(quint64 nCRC, const char *pData, qint32 nSize)
{
    static const std::array<quint64, 256> XZ_CRC64_TABLE = xzBuildCRC64Table();

    for (qint32 i = 0; i < nSize; i++) {
        const quint8 nTableIndex = (quint8)(nCRC ^ (quint8)pData[i]);
        nCRC = XZ_CRC64_TABLE[(size_t)nTableIndex] ^ (nCRC >> 8);
    }

    return nCRC;
}

void appendLE32(QByteArray *pResult, quint32 nValue)
{
    pResult->append((char)nValue);
    pResult->append((char)(nValue >> 8));
    pResult->append((char)(nValue >> 16));
    pResult->append((char)(nValue >> 24));
}

void appendLE64(QByteArray *pResult, quint64 nValue)
{
    for (qint32 i = 0; i < 8; i++) {
        pResult->append((char)(nValue >> (i * 8)));
    }
}

class XZCheckState {
public:
    explicit XZCheckState(quint8 nCheckType)
        : m_nCheckType(nCheckType), m_nCRC32(0xFFFFFFFF), m_nCRC64(Q_UINT64_C(0xFFFFFFFFFFFFFFFF)), m_sha256(QCryptographicHash::Sha256)
    {
    }

    void update(const char *pData, qint32 nSize)
    {
        if (nSize <= 0) {
            return;
        }

        if (m_nCheckType == 1) {
            m_nCRC32 = XBinary::_getCRC32(pData, nSize, m_nCRC32, XBinary::_getCRC32Table_EDB88320());
        } else if (m_nCheckType == 4) {
            m_nCRC64 = xzCRC64Update(m_nCRC64, pData, nSize);
        } else if (m_nCheckType == 10) {
            m_sha256.addData(pData, nSize);
        }
    }

    QByteArray digest()
    {
        QByteArray result;

        if (m_nCheckType == 1) {
            appendLE32(&result, m_nCRC32 ^ 0xFFFFFFFF);
        } else if (m_nCheckType == 4) {
            appendLE64(&result, m_nCRC64 ^ Q_UINT64_C(0xFFFFFFFFFFFFFFFF));
        } else if (m_nCheckType == 10) {
            result = m_sha256.result();
        }

        return result;
    }

private:
    quint8 m_nCheckType;
    quint32 m_nCRC32;
    quint64 m_nCRC64;
    QCryptographicHash m_sha256;
};

class XZRangeInputDevice : public QIODevice {
public:
    XZRangeInputDevice(QIODevice *pSource, qint64 nOffset, qint64 nSize) : m_pSource(pSource), m_nOffset(nOffset), m_nSize(nSize)
    {
    }

    qint64 size() const override
    {
        return m_nSize;
    }

    bool seek(qint64 nPos) override
    {
        if (!m_pSource || (nPos < 0) || (nPos > m_nSize) || !m_pSource->seek(m_nOffset + nPos)) {
            return false;
        }

        return QIODevice::seek(nPos);
    }

protected:
    qint64 readData(char *pData, qint64 nMaxSize) override
    {
        if (!m_pSource || !pData || (nMaxSize < 0) || (pos() < 0) || (pos() > m_nSize)) {
            return -1;
        }

        const qint64 nReadSize = (std::min)(nMaxSize, m_nSize - pos());
        if (nReadSize == 0) {
            return 0;
        }
        if (!m_pSource->seek(m_nOffset + pos())) {
            return -1;
        }

        return m_pSource->read(pData, nReadSize);
    }

    qint64 writeData(const char *pData, qint64 nMaxSize) override
    {
        Q_UNUSED(pData)
        Q_UNUSED(nMaxSize)

        return -1;
    }

private:
    QIODevice *m_pSource;
    qint64 m_nOffset;
    qint64 m_nSize;
};

class XZCheckedOutputDevice : public QIODevice {
public:
    XZCheckedOutputDevice(XBinary::DATAPROCESS_STATE *pOutputState, quint8 nCheckType) : m_pOutputState(pOutputState), m_checkState(nCheckType)
    {
    }

    bool isSequential() const override
    {
        // The LZMA helper rewinds its destination before starting.  This
        // adapter only ever permits that initial seek through QIODevice's
        // default position handling; all actual output remains forward-only.
        return false;
    }

    QByteArray digest()
    {
        return m_checkState.digest();
    }

protected:
    qint64 readData(char *pData, qint64 nMaxSize) override
    {
        Q_UNUSED(pData)
        Q_UNUSED(nMaxSize)

        return -1;
    }

    qint64 writeData(const char *pData, qint64 nMaxSize) override
    {
        if (!m_pOutputState || !pData || (nMaxSize < 0)) {
            return -1;
        }

        qint64 nProcessed = 0;
        while (nProcessed < nMaxSize) {
            const qint32 nChunkSize = (qint32)(std::min)(nMaxSize - nProcessed, (qint64)(std::numeric_limits<qint32>::max)());
            m_checkState.update(pData + nProcessed, nChunkSize);
            if (XBinary::_writeDevice(pData + nProcessed, nChunkSize, m_pOutputState) != nChunkSize) {
                return -1;
            }
            nProcessed += nChunkSize;
        }

        return nMaxSize;
    }

private:
    XBinary::DATAPROCESS_STATE *m_pOutputState;
    XZCheckState m_checkState;
};

bool readExactAt(XBinary::DATAPROCESS_STATE *pState, qint64 nOffset, qint32 nSize, QByteArray *pResult)
{
    if (!pState || !pState->pDeviceInput || !pResult || (nOffset < 0) || (nSize < 0) || (nOffset > ((std::numeric_limits<qint64>::max)() - nSize))) {
        if (pState) pState->bReadError = true;
        return false;
    }
    if (!pState->pDeviceInput->seek(nOffset)) {
        pState->bReadError = true;
        return false;
    }

    pResult->resize(nSize);
    qint32 nReadTotal = 0;
    while (nReadTotal < nSize) {
        const qint64 nRead = pState->pDeviceInput->read(pResult->data() + nReadTotal, nSize - nReadTotal);
        if ((nRead <= 0) || (nRead > (nSize - nReadTotal))) {
            pState->bReadError = true;
            pResult->clear();
            return false;
        }
        nReadTotal += (qint32)nRead;
    }

    return true;
}

bool readExactState(char *pBuffer, qint32 nSize, XBinary::DATAPROCESS_STATE *pState)
{
    if (!pBuffer || !pState || (nSize < 0) || (pState->nCountInput < 0) || (pState->nInputLimit < -1) ||
        ((pState->nInputLimit != -1) && ((qint64)nSize > (pState->nInputLimit - pState->nCountInput)))) {
        if (pState) pState->bReadError = true;
        return false;
    }

    qint32 nTotalRead = 0;
    while (nTotalRead < nSize) {
        const qint32 nRead = XBinary::_readDevice(pBuffer + nTotalRead, nSize - nTotalRead, pState);
        if (nRead <= 0) {
            return false;
        }
        nTotalRead += nRead;
    }

    return true;
}
}  // namespace

XLZMADecoder::XLZMADecoder(QObject *parent) : QObject(parent)
{
}

bool XLZMADecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    if ((pDecompressState->nInputLimit != -1) && (pDecompressState->nInputLimit < 4)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);

    qint32 nPropSize = 0;
    char header1[4] = {};
    quint8 properties[32] = {};

    if (!readExactState(header1, sizeof(header1), pDecompressState)) {
        return false;
    }
    nPropSize = (quint8)header1[2];

    if (!nPropSize || nPropSize >= 30 || ((pDecompressState->nInputLimit != -1) && (nPropSize > (pDecompressState->nInputLimit - pDecompressState->nCountInput)))) {
        return false;
    }

    if (!readExactState((char *)properties, nPropSize, pDecompressState)) {
        return false;
    }

    CLzmaDec state = {};
    SRes ret = X_LzmaProps_Decode(&state.prop, (Byte *)properties, nPropSize);

    if ((ret != 0) || !isLZMAAllocationAllowed(pDecompressState,
                                               (quint64)state.prop.dicSize)) {  // S_OK
        return false;
    }

    qint32 nBufferSize = 0;
    XBinary::UNPACK_MEMORY_RESERVATION memoryReservation;
    if (!getLzmaBufferSize(pPdStruct, &nBufferSize) || !reserveLzmaDecoderMemory(pDecompressState, state.prop, nBufferSize, &memoryReservation)) {
        return false;
    }

    X_LzmaDec_Construct(&state);
    ret = X_LzmaDec_Allocate(&state, (Byte *)properties, nPropSize, Algo_utils::lzmaAlloc());

    if (ret != 0) {  // S_OK
        return false;
    }

    X_LzmaDec_Init(&state);
    bool bResult = Algo_utils::decompressLZMA(&state, pDecompressState, nBufferSize, pPdStruct);
    X_LzmaDec_Free(&state, Algo_utils::lzmaAlloc());

    return bResult;
}

bool XLZMADecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, const QByteArray &baProperty, XBinary::PDSTRUCT *pPdStruct)
{
    return decompressWithResult(pDecompressState, baProperty, pPdStruct) == DECOMPRESS_RESULT_SUCCESS;
}

XLZMADecoder::DECOMPRESS_RESULT XLZMADecoder::decompressWithResult(XBinary::DATAPROCESS_STATE *pDecompressState, const QByteArray &baProperty,
                                                                   XBinary::PDSTRUCT *pPdStruct)
{
    return decompressWithResult(pDecompressState, baProperty, pPdStruct, nullptr);
}

bool XLZMADecoder::getMemoryRequirement(const QByteArray &baProperty, qint64 *pnSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pnSize || (baProperty.size() <= 0) || (baProperty.size() >= 30)) {
        return false;
    }

    CLzmaProps properties = {};
    qint32 nBufferSize = 0;
    quint64 nSize = 0;
    if ((X_LzmaProps_Decode(&properties, (Byte *)baProperty.constData(), baProperty.size()) != 0) || !getLzmaBufferSize(pPdStruct, &nBufferSize) ||
        !getLzmaDecoderMemorySize(properties, nBufferSize, &nSize) || (nSize > (quint64)(std::numeric_limits<qint64>::max)())) {
        return false;
    }

    *pnSize = (qint64)nSize;
    return true;
}

XLZMADecoder::DECOMPRESS_RESULT XLZMADecoder::decompressWithResult(XBinary::DATAPROCESS_STATE *pDecompressState, const QByteArray &baProperty,
                                                                   XBinary::PDSTRUCT *pPdStruct, XBinary::UNPACK_MEMORY_RESERVATION *pReservedMemory)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        // qDebug("XLZMADecoder::decompress() FAILED: null pointer check");
        return DECOMPRESS_RESULT_INVALID_DATA;
    }

    if (baProperty.size() <= 0 || baProperty.size() >= 30) {
        // qDebug("XLZMADecoder::decompress() FAILED: invalid baProperty size: %d", baProperty.size());
        return DECOMPRESS_RESULT_INVALID_DATA;
    }

    Algo_utils::prepareState(pDecompressState);

    CLzmaDec state = {};
    SRes ret = X_LzmaProps_Decode(&state.prop, (Byte *)baProperty.constData(), baProperty.size());

    if (ret != 0) return DECOMPRESS_RESULT_INVALID_DATA;
    if (!isLZMAAllocationAllowed(pDecompressState, (quint64)state.prop.dicSize)) {
        return DECOMPRESS_RESULT_RESOURCE_LIMIT;
    }

    qint32 nBufferSize = 0;
    quint64 nRequiredSize = 0;
    if (!getLzmaBufferSize(pPdStruct, &nBufferSize) || !getLzmaDecoderMemorySize(state.prop, nBufferSize, &nRequiredSize) ||
        (nRequiredSize > (quint64)(std::numeric_limits<qint64>::max)())) {
        return DECOMPRESS_RESULT_RESOURCE_LIMIT;
    }

    XBinary::UNPACK_MEMORY_RESERVATION memoryReservation;
    if (pReservedMemory) {
        if (!pReservedMemory->isActive() || (nRequiredSize > static_cast<quint64>(pReservedMemory->size()))) {
            return DECOMPRESS_RESULT_RESOURCE_LIMIT;
        }
    } else if (!memoryReservation.acquire(pDecompressState->mapUnpackProperties, static_cast<qint64>(nRequiredSize))) {
        return DECOMPRESS_RESULT_RESOURCE_LIMIT;
    }

    X_LzmaDec_Construct(&state);
    ret = X_LzmaDec_Allocate(&state, (Byte *)baProperty.constData(), baProperty.size(), Algo_utils::lzmaAlloc());

    if (ret != 0) {
        return DECOMPRESS_RESULT_RESOURCE_LIMIT;
    }

    X_LzmaDec_Init(&state);
    bool bResult = Algo_utils::decompressLZMA(&state, pDecompressState, nBufferSize, pPdStruct);
    X_LzmaDec_Free(&state, Algo_utils::lzmaAlloc());

    return bResult ? DECOMPRESS_RESULT_SUCCESS : DECOMPRESS_RESULT_INVALID_DATA;
}

bool XLZMADecoder::decompressLZMA2(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    if ((pDecompressState->nInputLimit != -1) && (pDecompressState->nInputLimit < 1)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);

    // Read LZMA2 properties (1 byte)
    char propByte = 0;
    if (!readExactState(&propByte, 1, pDecompressState)) {
        return false;
    }
    if (!isLZMA2DictionaryAllowed(pDecompressState, (quint8)propByte)) {
        return false;
    }

    CLzmaProps allocationProperties = {};
    XBinary::UNPACK_MEMORY_RESERVATION memoryReservation;
    if (!getLzma2Properties((quint8)propByte, &allocationProperties) || !reserveLzmaAllocation(pDecompressState, allocationProperties, &memoryReservation)) {
        return false;
    }

    // LZMA2 state
    CLzma2Dec state = {};
    SRes ret = X_Lzma2Dec_Allocate(&state, (Byte)propByte, Algo_utils::lzmaAlloc());

    if (ret != 0) {  // S_OK
        return false;
    }

    X_Lzma2Dec_Init(&state);
    bool bResult = Algo_utils::decompressLZMA2(&state, pDecompressState, pPdStruct);
    X_Lzma2Dec_Free(&state, Algo_utils::lzmaAlloc());

    return bResult;
}

bool XLZMADecoder::decompressLZMA2(XBinary::DATAPROCESS_STATE *pDecompressState, const QByteArray &baProperty, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    if ((baProperty.size() != 1) || !isLZMA2DictionaryAllowed(pDecompressState, (quint8)baProperty.at(0))) {
        return false;
    }

    CLzmaProps allocationProperties = {};
    XBinary::UNPACK_MEMORY_RESERVATION memoryReservation;
    if (!getLzma2Properties((quint8)baProperty.at(0), &allocationProperties) || !reserveLzmaAllocation(pDecompressState, allocationProperties, &memoryReservation)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);

    // LZMA2 state
    CLzma2Dec state = {};
    SRes ret = X_Lzma2Dec_Allocate(&state, (Byte)baProperty[0], Algo_utils::lzmaAlloc());

    if (ret != 0) {  // S_OK
        return false;
    }

    X_Lzma2Dec_Init(&state);
    bool bResult = Algo_utils::decompressLZMA2(&state, pDecompressState, pPdStruct);
    X_Lzma2Dec_Free(&state, Algo_utils::lzmaAlloc());

    return bResult;
}

static bool xzGetCheckSize(quint8 nCheckType, qint32 *pnCheckSize)
{
    if (!pnCheckSize) return false;
    if (nCheckType == 0) {
        *pnCheckSize = 0;
    } else if (nCheckType == 1) {
        *pnCheckSize = 4;
    } else if (nCheckType == 4) {
        *pnCheckSize = 8;
    } else if (nCheckType == 10) {
        *pnCheckSize = 32;
    } else {
        return false;
    }
    return true;
}

static bool xzStripStreamPadding(XBinary::DATAPROCESS_STATE *pDecompressState, qint64 nOffset, qint64 nContainerEnd, XBinary::PDSTRUCT *pPdStruct, qint64 *pnEnd)
{
    if (!pnEnd || (*pnEnd < nOffset) || (*pnEnd > nContainerEnd)) return false;

    while ((*pnEnd - nOffset) >= 4) {
        qint64 nAvailable = *pnEnd - nOffset;
        qint32 nReadSize = (qint32)(std::min)(nAvailable - (nAvailable & 3), (qint64)0x10000);
        if (nReadSize < 4) break;

        QByteArray baTail;
        if (!readExactAt(pDecompressState, *pnEnd - nReadSize, nReadSize, &baTail)) return false;

        qint32 nPos = nReadSize;
        while (nPos >= 4) {
            const char *pGroup = baTail.constData() + nPos - 4;
            if (pGroup[0] || pGroup[1] || pGroup[2] || pGroup[3]) return true;
            nPos -= 4;
            *pnEnd -= 4;
        }

        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    }

    return true;
}

bool XLZMADecoder::decompressXZ(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    QIODevice *pDevice = pDecompressState->pDeviceInput;
    const qint64 nOffset = pDecompressState->nInputOffset;
    const qint64 nDeviceSize = pDevice->size();
    qint64 nTotalSize = pDecompressState->nInputLimit;
    const qint64 nMax = (std::numeric_limits<qint64>::max)();
    qint64 nConfiguredOutputLimit = -1;

    if ((nOffset < 0) || (nDeviceSize < 0) || (nOffset > nDeviceSize) || (pDecompressState->nInputLimit < -1) || (pDecompressState->nProcessedOffset < 0) ||
        (pDecompressState->nProcessedLimit < -1) || !XBinary::getUnpackOutputLimit(pDecompressState->mapUnpackProperties, &nConfiguredOutputLimit) ||
        ((pDecompressState->nProcessedLimit != -1) && (pDecompressState->nProcessedOffset > (nMax - pDecompressState->nProcessedLimit)))) {
        return false;
    }
    if (nTotalSize == -1) {
        nTotalSize = nDeviceSize - nOffset;
    }
    if ((nTotalSize < 32) || (nTotalSize > (nDeviceSize - nOffset)) || (nTotalSize & 3)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    static const quint8 XZ_MAGIC[6] = {0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00};
    const qint64 nContainerEnd = nOffset + nTotalSize;

    // Work backwards from each Stream Footer. This makes the Index authoritative
    // for all Block extents and naturally supports concatenated Streams and
    // Stream Padding without scanning compressed bytes for magic values.
    QList<XZStreamDescriptor> listStreams;
    qint64 nStreamEnd = nContainerEnd;
    if (!xzStripStreamPadding(pDecompressState, nOffset, nContainerEnd, pPdStruct, &nStreamEnd) || (nStreamEnd == nOffset)) return false;

    qint64 nTotalExpectedOutput = 0;
    while (nStreamEnd > nOffset) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) || ((nStreamEnd - nOffset) < 32) || (listStreams.count() >= 1000000)) return false;

        const qint64 nFooterOffset = nStreamEnd - 12;
        QByteArray baStreamFooter;
        if (!readExactAt(pDecompressState, nFooterOffset, 12, &baStreamFooter) || (baStreamFooter.at(10) != 'Y') || (baStreamFooter.at(11) != 'Z') ||
            ((quint8)baStreamFooter.at(8) != 0) || (((quint8)baStreamFooter.at(9) & 0xF0) != 0) ||
            (xzCRC32(baStreamFooter.constData() + 4, 6) != readLE32(baStreamFooter.constData()))) {
            return false;
        }

        const quint8 nCheckType = (quint8)baStreamFooter.at(9) & 0x0F;
        qint32 nCheckSize = 0;
        if (!xzGetCheckSize(nCheckType, &nCheckSize)) return false;

        const quint64 nIndexSize64 = ((quint64)readLE32(baStreamFooter.constData() + 4) + 1) * 4;
        if ((nIndexSize64 < 8) || (nIndexSize64 > (quint64)XZ_MAX_INDEX_SIZE) || (nIndexSize64 > (quint64)(nFooterOffset - nOffset - 12))) {
            return false;
        }
        const qint32 nIndexSize = (qint32)nIndexSize64;
        const qint64 nIndexOffset = nFooterOffset - nIndexSize;

        QByteArray baIndex;
        if (!readExactAt(pDecompressState, nIndexOffset, nIndexSize, &baIndex) ||
            (xzCRC32(baIndex.constData(), nIndexSize - 4) != readLE32(baIndex.constData() + nIndexSize - 4))) {
            return false;
        }

        const QByteArray baIndexFields = baIndex.left(nIndexSize - 4);
        qint32 nIndexPos = 0;
        quint64 nNumberOfRecords = 0;
        if (baIndexFields.isEmpty() || ((quint8)baIndexFields.at(nIndexPos++) != 0) || !Algo_utils::xzReadVarInt(baIndexFields, nIndexPos, nNumberOfRecords) ||
            (nNumberOfRecords > XZ_MAX_INDEX_RECORDS) || (nNumberOfRecords > (quint64)((baIndexFields.size() - nIndexPos) / 2))) {
            return false;
        }

        QList<XZIndexRecord> listIndexRecords;
        listIndexRecords.reserve((qint32)nNumberOfRecords);
        quint64 nPaddedBlocksSize64 = 0;
        for (quint64 i = 0; i < nNumberOfRecords; i++) {
            XZIndexRecord record = {};
            if (!Algo_utils::xzReadVarInt(baIndexFields, nIndexPos, record.nUnpaddedSize) ||
                !Algo_utils::xzReadVarInt(baIndexFields, nIndexPos, record.nUncompressedSize) || (record.nUnpaddedSize > (quint64)nMax - 3) ||
                (record.nUncompressedSize > (quint64)nMax)) {
                return false;
            }
            const quint64 nPaddedSize = (record.nUnpaddedSize + 3) & ~Q_UINT64_C(3);
            if (nPaddedBlocksSize64 > ((quint64)nMax - nPaddedSize)) return false;
            nPaddedBlocksSize64 += nPaddedSize;
            listIndexRecords.append(record);
        }

        const qint32 nIndexPaddingSize = baIndexFields.size() - nIndexPos;
        if ((nIndexPaddingSize < 0) || (nIndexPaddingSize > 3)) return false;
        for (; nIndexPos < baIndexFields.size(); nIndexPos++) {
            if (baIndexFields.at(nIndexPos) != 0) return false;
        }

        if ((nPaddedBlocksSize64 > (quint64)(nIndexOffset - nOffset - 12))) return false;
        const qint64 nStreamStart = nIndexOffset - (qint64)nPaddedBlocksSize64 - 12;
        if (nStreamStart < nOffset) return false;

        QByteArray baStreamHeader;
        if (!readExactAt(pDecompressState, nStreamStart, 12, &baStreamHeader) || (std::memcmp(baStreamHeader.constData(), XZ_MAGIC, sizeof(XZ_MAGIC)) != 0) ||
            ((quint8)baStreamHeader.at(6) != 0) || (((quint8)baStreamHeader.at(7) & 0xF0) != 0) ||
            (xzCRC32(baStreamHeader.constData() + 6, 2) != readLE32(baStreamHeader.constData() + 8)) ||
            (std::memcmp(baStreamFooter.constData() + 8, baStreamHeader.constData() + 6, 2) != 0)) {
            return false;
        }

        XZStreamDescriptor streamDescriptor = {};
        streamDescriptor.nStartOffset = nStreamStart;
        streamDescriptor.nEndOffset = nStreamEnd;
        streamDescriptor.nCheckType = nCheckType;

        qint64 nBlockOffset = nStreamStart + 12;
        for (qint32 nRecordIndex = 0; nRecordIndex < listIndexRecords.count(); nRecordIndex++) {
            const XZIndexRecord &indexRecord = listIndexRecords.at(nRecordIndex);
            QByteArray baHeaderSize;
            if (!readExactAt(pDecompressState, nBlockOffset, 1, &baHeaderSize)) return false;
            const quint8 nHeaderSizeByte = (quint8)baHeaderSize.at(0);
            if (nHeaderSizeByte == 0) return false;

            const qint32 nActualHeaderSize = ((qint32)nHeaderSizeByte + 1) * 4;
            if ((nActualHeaderSize < 8) || (nActualHeaderSize > 1024) || (indexRecord.nUnpaddedSize < ((quint64)nActualHeaderSize + (quint64)nCheckSize + 1))) {
                return false;
            }

            QByteArray baBlockHeader;
            if (!readExactAt(pDecompressState, nBlockOffset, nActualHeaderSize, &baBlockHeader) ||
                (xzCRC32(baBlockHeader.constData(), nActualHeaderSize - 4) != readLE32(baBlockHeader.constData() + nActualHeaderSize - 4))) {
                return false;
            }
            const QByteArray baBlockFields = baBlockHeader.left(nActualHeaderSize - 4);
            if (baBlockFields.size() < 2) return false;

            const quint8 nBlockFlags = (quint8)baBlockFields.at(1);
            if ((nBlockFlags & 0x3C) != 0) return false;
            const qint32 nNumFilters = (nBlockFlags & 0x03) + 1;
            const bool bHasCompressedSize = (nBlockFlags & 0x40) != 0;
            const bool bHasUncompressedSize = (nBlockFlags & 0x80) != 0;
            qint32 nBlockPos = 2;
            quint64 nDeclaredCompressedSize64 = 0;
            quint64 nDeclaredUncompressedSize64 = 0;
            if (bHasCompressedSize && !Algo_utils::xzReadVarInt(baBlockFields, nBlockPos, nDeclaredCompressedSize64)) return false;
            if (bHasUncompressedSize && !Algo_utils::xzReadVarInt(baBlockFields, nBlockPos, nDeclaredUncompressedSize64)) return false;

            XZBlockDescriptor blockDescriptor = {};
            for (qint32 nFilter = 0; nFilter < nNumFilters; nFilter++) {
                quint64 nFilterID = 0;
                quint64 nPropertySize64 = 0;
                if (!Algo_utils::xzReadVarInt(baBlockFields, nBlockPos, nFilterID) || !Algo_utils::xzReadVarInt(baBlockFields, nBlockPos, nPropertySize64) ||
                    (nPropertySize64 > 20) || (nPropertySize64 > (quint64)(baBlockFields.size() - nBlockPos))) {
                    return false;
                }

                const QByteArray baProperties = baBlockFields.mid(nBlockPos, (qint32)nPropertySize64);
                nBlockPos += (qint32)nPropertySize64;

                if (nFilter == (nNumFilters - 1)) {
                    if ((nFilterID != 0x21) || (baProperties.size() != 1) || ((quint8)baProperties.at(0) > 40)) return false;
                    blockDescriptor.nLZMA2PropsByte = (quint8)baProperties.at(0);
                } else {
                    if (nFilterID == 0x03) {
                        if (baProperties.size() != 1) return false;
                    } else if ((nFilterID >= 0x04) && (nFilterID <= 0x0A)) {
                        if ((baProperties.size() != 0) && (baProperties.size() != 4)) return false;
                        if (baProperties.size() == 4) {
                            const quint32 nStartOffset = readLE32(baProperties.constData());
                            if ((((nFilterID == 0x05) || (nFilterID == 0x07) || (nFilterID == 0x09) || (nFilterID == 0x0A)) && (nStartOffset & 3)) ||
                                ((nFilterID == 0x06) && (nStartOffset & 0x0F)) || ((nFilterID == 0x08) && (nStartOffset & 1))) {
                                return false;
                            }
                        }
                    } else {
                        return false;
                    }
                    blockDescriptor.listPrefilters.append(qMakePair(nFilterID, baProperties));
                }
            }

            for (; nBlockPos < baBlockFields.size(); nBlockPos++) {
                if (baBlockFields.at(nBlockPos) != 0) return false;
            }

            const quint64 nDictionarySize = (blockDescriptor.nLZMA2PropsByte == 40)
                                                ? Q_UINT64_C(0xFFFFFFFF)
                                                : (((quint64)2 | (blockDescriptor.nLZMA2PropsByte & 1)) << (blockDescriptor.nLZMA2PropsByte / 2 + 11));
            if (!isLZMAAllocationAllowed(pDecompressState, nDictionarySize)) {
                return false;
            }

            const quint64 nDataSize64 = indexRecord.nUnpaddedSize - (quint64)nActualHeaderSize - (quint64)nCheckSize;
            if ((nDataSize64 == 0) || (nDataSize64 > (quint64)nMax) || (bHasCompressedSize && (nDeclaredCompressedSize64 != nDataSize64)) ||
                (bHasUncompressedSize && (nDeclaredUncompressedSize64 != indexRecord.nUncompressedSize))) {
                return false;
            }

            const quint64 nPaddedBlockSize64 = (indexRecord.nUnpaddedSize + 3) & ~Q_UINT64_C(3);
            const qint32 nBlockPaddingSize = (qint32)(nPaddedBlockSize64 - indexRecord.nUnpaddedSize);
            blockDescriptor.nDataOffset = nBlockOffset + nActualHeaderSize;
            blockDescriptor.nDataSize = (qint64)nDataSize64;
            blockDescriptor.nUncompressedSize = (qint64)indexRecord.nUncompressedSize;
            if (!XBinary::isUnpackOutputSizeAllowed(pDecompressState->mapUnpackProperties, blockDescriptor.nUncompressedSize)) {
                return false;
            }
            const qint64 nBlockCheckOffset = blockDescriptor.nDataOffset + blockDescriptor.nDataSize + nBlockPaddingSize;

            QByteArray baBlockPadding;
            if ((nBlockPaddingSize > 0) && !readExactAt(pDecompressState, blockDescriptor.nDataOffset + blockDescriptor.nDataSize, nBlockPaddingSize, &baBlockPadding)) {
                return false;
            }
            for (qint32 i = 0; i < baBlockPadding.size(); i++) {
                if (baBlockPadding.at(i) != 0) return false;
            }
            if (!readExactAt(pDecompressState, nBlockCheckOffset, nCheckSize, &blockDescriptor.baStoredCheck) ||
                ((nBlockCheckOffset + nCheckSize) != (nBlockOffset + (qint64)nPaddedBlockSize64))) {
                return false;
            }

            if (nTotalExpectedOutput > (nMax - blockDescriptor.nUncompressedSize)) return false;
            nTotalExpectedOutput += blockDescriptor.nUncompressedSize;
            if ((nConfiguredOutputLimit >= 0) && (nTotalExpectedOutput > nConfiguredOutputLimit)) {
                return false;
            }
            streamDescriptor.listBlocks.append(blockDescriptor);
            nBlockOffset += (qint64)nPaddedBlockSize64;
        }

        if (nBlockOffset != nIndexOffset) return false;
        listStreams.prepend(streamDescriptor);

        nStreamEnd = nStreamStart;
        if (nStreamEnd == nOffset) break;
        const qint64 nBeforePadding = nStreamEnd;
        if (!xzStripStreamPadding(pDecompressState, nOffset, nContainerEnd, pPdStruct, &nStreamEnd)) return false;
        // Padding is valid only after a preceding Stream, never before the
        // first Stream in the container.
        if ((nStreamEnd == nOffset) && (nBeforePadding != nStreamEnd)) return false;
    }

    if ((nStreamEnd != nOffset) || listStreams.isEmpty()) return false;

    for (qint32 nStreamIndex = 0; nStreamIndex < listStreams.count(); nStreamIndex++) {
        const XZStreamDescriptor &streamDescriptor = listStreams.at(nStreamIndex);
        for (qint32 nBlockIndex = 0; nBlockIndex < streamDescriptor.listBlocks.count(); nBlockIndex++) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            const XZBlockDescriptor &blockDescriptor = streamDescriptor.listBlocks.at(nBlockIndex);
            const qint64 nOutputBeforeBlock = pDecompressState->nCountOutput;

            XZRangeInputDevice compressedDevice(pDevice, blockDescriptor.nDataOffset, blockDescriptor.nDataSize);
            if (!compressedDevice.open(QIODevice::ReadOnly)) {
                pDecompressState->bReadError = true;
                return false;
            }

            const QByteArray baLZMA2Property(1, (char)blockDescriptor.nLZMA2PropsByte);
            XBinary::DATAPROCESS_STATE lzma2State = {};
            lzma2State.pDeviceInput = &compressedDevice;
            lzma2State.nInputOffset = 0;
            lzma2State.nInputLimit = blockDescriptor.nDataSize;
            lzma2State.nProcessedOffset = 0;
            lzma2State.nProcessedLimit = -1;
            lzma2State.mapUnpackProperties = pDecompressState->mapUnpackProperties;
            lzma2State.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, blockDescriptor.nUncompressedSize);

            if (blockDescriptor.listPrefilters.isEmpty()) {
                XZCheckedOutputDevice checkedOutput(pDecompressState, streamDescriptor.nCheckType);
                if (!checkedOutput.open(QIODevice::WriteOnly)) {
                    pDecompressState->bWriteError = true;
                    compressedDevice.close();
                    return false;
                }
                lzma2State.pDeviceOutput = &checkedOutput;

                const bool bDecoded = XLZMADecoder::decompressLZMA2(&lzma2State, baLZMA2Property, pPdStruct);
                pDecompressState->bReadError = pDecompressState->bReadError || lzma2State.bReadError;
                pDecompressState->bWriteError = pDecompressState->bWriteError || lzma2State.bWriteError;
                const QByteArray baCalculatedCheck = checkedOutput.digest();
                checkedOutput.close();
                compressedDevice.close();

                if (!bDecoded || pDecompressState->bReadError || pDecompressState->bWriteError || (lzma2State.nCountInput != blockDescriptor.nDataSize) ||
                    (lzma2State.nCountOutput != blockDescriptor.nUncompressedSize) ||
                    (pDecompressState->nCountOutput != (nOutputBeforeBlock + blockDescriptor.nUncompressedSize)) ||
                    (baCalculatedCheck != blockDescriptor.baStoredCheck) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
                    return false;
                }
            } else {
                if ((blockDescriptor.nUncompressedSize > XZ_MAX_PREFILTER_OUTPUT_SIZE) || (blockDescriptor.nUncompressedSize > (std::numeric_limits<qint32>::max)())) {
                    compressedDevice.close();
                    return false;
                }

                // nUncompressedSize is pre-validated above against
                // XZ_MAX_PREFILTER_OUTPUT_SIZE, so the allocation size is bounded.
                XBinary::UNPACK_MEMORY_RESERVATION intermediateReservation;
                if (!intermediateReservation.acquire(pDecompressState->mapUnpackProperties, blockDescriptor.nUncompressedSize)) {
                    compressedDevice.close();
                    return false;
                }
                QByteArray baIntermediate;
                baIntermediate.resize((qint32)blockDescriptor.nUncompressedSize);

                QBuffer intermediateBuffer(&baIntermediate);
                if (!intermediateBuffer.open(QIODevice::WriteOnly)) {
                    compressedDevice.close();
                    return false;
                }
                lzma2State.pDeviceOutput = &intermediateBuffer;

                const bool bDecoded = XLZMADecoder::decompressLZMA2(&lzma2State, baLZMA2Property, pPdStruct);
                pDecompressState->bReadError = pDecompressState->bReadError || lzma2State.bReadError;
                pDecompressState->bWriteError = pDecompressState->bWriteError || lzma2State.bWriteError;
                intermediateBuffer.close();
                compressedDevice.close();

                if (!bDecoded || pDecompressState->bReadError || pDecompressState->bWriteError || (lzma2State.nCountInput != blockDescriptor.nDataSize) ||
                    (lzma2State.nCountOutput != blockDescriptor.nUncompressedSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
                    return false;
                }

                for (qint32 i = blockDescriptor.listPrefilters.count() - 1; i >= 0; i--) {
                    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
                    const quint64 nFilterID = blockDescriptor.listPrefilters.at(i).first;
                    const QByteArray &baProperties = blockDescriptor.listPrefilters.at(i).second;
                    if (nFilterID == 0x03) {
                        XBranchDecoder::applyDeltaDecode(baIntermediate, (qint32)(quint8)baProperties.at(0) + 1);
                    } else {
                        const quint32 nStartOffset = (baProperties.size() == 4) ? readLE32(baProperties.constData()) : 0;
                        if (nFilterID == 0x04) {
                            Algo_utils::applyBCJX86Decode(baIntermediate, nStartOffset);
                        } else if (nFilterID == 0x05) {
                            XBranchDecoder::applyBranchDecode(baIntermediate, XBranchDecoder::BTYPE_PPC, nStartOffset);
                        } else if (nFilterID == 0x06) {
                            XBranchDecoder::applyBranchDecode(baIntermediate, XBranchDecoder::BTYPE_IA64, nStartOffset);
                        } else if (nFilterID == 0x07) {
                            XBranchDecoder::applyBranchDecode(baIntermediate, XBranchDecoder::BTYPE_ARM, nStartOffset);
                        } else if (nFilterID == 0x08) {
                            XBranchDecoder::applyBranchDecode(baIntermediate, XBranchDecoder::BTYPE_ARMT, nStartOffset);
                        } else if (nFilterID == 0x09) {
                            XBranchDecoder::applyBranchDecode(baIntermediate, XBranchDecoder::BTYPE_SPARC, nStartOffset);
                        } else if (nFilterID == 0x0A) {
                            XBranchDecoder::applyBranchDecode(baIntermediate, XBranchDecoder::BTYPE_ARM64, nStartOffset);
                        } else {
                            return false;
                        }
                    }
                }

                XZCheckState checkState(streamDescriptor.nCheckType);
                checkState.update(baIntermediate.constData(), baIntermediate.size());
                if ((checkState.digest() != blockDescriptor.baStoredCheck) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

                const qint32 nWriteChunkSize = 64 * 1024;
                qint32 nWriteOffset = 0;
                while (nWriteOffset < baIntermediate.size()) {
                    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
                    const qint32 nChunkSize = (std::min)(nWriteChunkSize, static_cast<qint32>(baIntermediate.size() - nWriteOffset));
                    if (XBinary::_writeDevice(baIntermediate.constData() + nWriteOffset, nChunkSize, pDecompressState) != nChunkSize) return false;
                    nWriteOffset += nChunkSize;
                }
                if ((pDecompressState->nCountOutput != (nOutputBeforeBlock + blockDescriptor.nUncompressedSize)) || pDecompressState->bWriteError) {
                    return false;
                }
            }
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || pDecompressState->bReadError || pDecompressState->bWriteError ||
        (pDecompressState->nCountOutput != nTotalExpectedOutput)) {
        return false;
    }

    // Absolute validation reads leave the device near the last inspected
    // Block.  Publish a physical position consistent with the logical count so
    // callers can continue with the next member.
    if (!pDevice->seek(nContainerEnd) && (pDevice->pos() != nContainerEnd)) {
        pDecompressState->bReadError = true;
        return false;
    }
    pDecompressState->nCountInput = nTotalSize;
    return true;
}

/* ===== Begin embedded xlzma_local.c ===== */
/* Local renamed copies of the 7-Zip LZMA decoder C entry points. */

#include "xalgo_local.h"

#define LzmaDec_InitDicAndState X_LzmaDec_InitDicAndState
#define LzmaDec_Init X_LzmaDec_Init
#define LzmaDec_DecodeToDic X_LzmaDec_DecodeToDic
#define LzmaDec_DecodeToBuf X_LzmaDec_DecodeToBuf
#define LzmaDec_FreeProbs X_LzmaDec_FreeProbs
#define LzmaDec_Free X_LzmaDec_Free
#define LzmaProps_Decode X_LzmaProps_Decode
#define LzmaDec_AllocateProbs X_LzmaDec_AllocateProbs
#define LzmaDec_Allocate X_LzmaDec_Allocate
#define LzmaDecode X_LzmaDecode
/* LzmaDec.c -- LZMA Decoder
2023-04-07 : Igor Pavlov : Public domain */
#include <string.h>

// #define kNumTopBits 24
#define kTopValue ((UInt32)1 << 24)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)

#define RC_INIT_SIZE 5

#ifndef Z7_LZMA_DEC_OPT

#define kNumMoveBits 5
#define NORMALIZE                      \
    if (range < kTopValue) {           \
        range <<= 8;                   \
        code = (code << 8) | (*buf++); \
    }

#define IF_BIT_0(p)                                         \
    ttt = *(p);                                             \
    NORMALIZE;                                              \
    bound = (range >> kNumBitModelTotalBits) * (UInt32)ttt; \
    if (code < bound)
#define UPDATE_0(p) \
    range = bound;  \
    *(p) = (CLzmaProb)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits));
#define UPDATE_1(p) \
    range -= bound; \
    code -= bound;  \
    *(p) = (CLzmaProb)(ttt - (ttt >> kNumMoveBits));
#define GET_BIT2(p, i, A0, A1)       \
    IF_BIT_0(p)                      \
    {                                \
        UPDATE_0(p) i = (i + i);     \
        A0;                          \
    }                                \
    else                             \
    {                                \
        UPDATE_1(p) i = (i + i) + 1; \
        A1;                          \
    }

#define TREE_GET_BIT(probs, i)        \
    {                                 \
        GET_BIT2(probs + i, i, ;, ;); \
    }

#define REV_BIT(p, i, A0, A1) \
    IF_BIT_0(p + i)           \
    {                         \
        UPDATE_0(p + i) A0;   \
    }                         \
    else                      \
    {                         \
        UPDATE_1(p + i) A1;   \
    }
#define REV_BIT_VAR(p, i, m) REV_BIT(p, i, i += m; m += m, m += m; i += m;)
#define REV_BIT_CONST(p, i, m) REV_BIT(p, i, i += m;, i += m * 2;)
#define REV_BIT_LAST(p, i, m) REV_BIT(p, i, i -= m, ;)

#define TREE_DECODE(probs, limit, i) \
    {                                \
        i = 1;                       \
        do {                         \
            TREE_GET_BIT(probs, i);  \
        } while (i < limit);         \
        i -= limit;                  \
    }

/* #define Z7_LZMA_SIZE_OPT */

#ifdef Z7_LZMA_SIZE_OPT
#define TREE_6_DECODE(probs, i) TREE_DECODE(probs, (1 << 6), i)
#else
#define TREE_6_DECODE(probs, i) \
    {                           \
        i = 1;                  \
        TREE_GET_BIT(probs, i)  \
        TREE_GET_BIT(probs, i)  \
        TREE_GET_BIT(probs, i)  \
        TREE_GET_BIT(probs, i)  \
        TREE_GET_BIT(probs, i)  \
        TREE_GET_BIT(probs, i)  \
        i -= 0x40;              \
    }
#endif

#define NORMAL_LITER_DEC TREE_GET_BIT(prob, symbol)
#define MATCHED_LITER_DEC                   \
    matchByte += matchByte;                 \
    bit = offs;                             \
    offs &= matchByte;                      \
    probLit = prob + (offs + bit + symbol); \
    GET_BIT2(probLit, symbol, offs ^= bit;, ;)

#endif  // Z7_LZMA_DEC_OPT

#define NORMALIZE_CHECK                              \
    if (range < kTopValue) {                         \
        if (buf >= bufLimit) return DUMMY_INPUT_EOF; \
        range <<= 8;                                 \
        code = (code << 8) | (*buf++);               \
    }

#define IF_BIT_0_CHECK(p)                                                   \
    ttt = *(p);                                                             \
    NORMALIZE_CHECK bound = (range >> kNumBitModelTotalBits) * (UInt32)ttt; \
    if (code < bound)
#define UPDATE_0_CHECK range = bound;
#define UPDATE_1_CHECK \
    range -= bound;    \
    code -= bound;
#define GET_BIT2_CHECK(p, i, A0, A1)    \
    IF_BIT_0_CHECK(p)                   \
    {                                   \
        UPDATE_0_CHECK i = (i + i);     \
        A0;                             \
    }                                   \
    else                                \
    {                                   \
        UPDATE_1_CHECK i = (i + i) + 1; \
        A1;                             \
    }
#define GET_BIT_CHECK(p, i) GET_BIT2_CHECK(p, i, ;, ;)
#define TREE_DECODE_CHECK(probs, limit, i) \
    {                                      \
        i = 1;                             \
        do {                               \
            GET_BIT_CHECK(probs + i, i)    \
        } while (i < limit);               \
        i -= limit;                        \
    }

#define REV_BIT_CHECK(p, i, m) \
    IF_BIT_0_CHECK(p + i)      \
    {                          \
        UPDATE_0_CHECK i += m; \
        m += m;                \
    }                          \
    else                       \
    {                          \
        UPDATE_1_CHECK m += m; \
        i += m;                \
    }

#define kNumPosBitsMax 4
#define kNumPosStatesMax (1 << kNumPosBitsMax)

#define kLenNumLowBits 3
#define kLenNumLowSymbols (1 << kLenNumLowBits)
#define kLenNumHighBits 8
#define kLenNumHighSymbols (1 << kLenNumHighBits)

#define LenLow 0
#define LenHigh (LenLow + 2 * (kNumPosStatesMax << kLenNumLowBits))
#define kNumLenProbs (LenHigh + kLenNumHighSymbols)

#define LenChoice LenLow
#define LenChoice2 (LenLow + (1 << kLenNumLowBits))

#define kNumStates 12
#define kNumStates2 16
#define kNumLitStates 7

#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumFullDistances (1 << (kEndPosModelIndex >> 1))

#define kNumPosSlotBits 6
#define kNumLenToPosStates 4

#define kNumAlignBits 4
#define kAlignTableSize (1 << kNumAlignBits)

#define kMatchMinLen 2
#define kMatchSpecLenStart (kMatchMinLen + kLenNumLowSymbols * 2 + kLenNumHighSymbols)

#define kMatchSpecLen_Error_Data (1 << 9)
#define kMatchSpecLen_Error_Fail (kMatchSpecLen_Error_Data - 1)

/* External ASM code needs same CLzmaProb array layout. So don't change it. */

/* (probs_1664) is faster and better for code size at some platforms */
/*
#ifdef MY_CPU_X86_OR_AMD64
*/
#define kStartOffset 1664
#define GET_PROBS p->probs_1664
/*
#define GET_PROBS p->probs + kStartOffset
#else
#define kStartOffset 0
#define GET_PROBS p->probs
#endif
*/

#define SpecPos (-kStartOffset)
#define IsRep0Long (SpecPos + kNumFullDistances)
#define RepLenCoder (IsRep0Long + (kNumStates2 << kNumPosBitsMax))
#define LenCoder (RepLenCoder + kNumLenProbs)
#define IsMatch (LenCoder + kNumLenProbs)
#define Align (IsMatch + (kNumStates2 << kNumPosBitsMax))
#define IsRep (Align + kAlignTableSize)
#define IsRepG0 (IsRep + kNumStates)
#define IsRepG1 (IsRepG0 + kNumStates)
#define IsRepG2 (IsRepG1 + kNumStates)
#define PosSlot (IsRepG2 + kNumStates)
#define Literal (PosSlot + (kNumLenToPosStates << kNumPosSlotBits))
#define NUM_BASE_PROBS (Literal + kStartOffset)

#if Align != 0 && kStartOffset != 0
#error Stop_Compiling_Bad_LZMA_kAlign
#endif

#if NUM_BASE_PROBS != 1984
#error Stop_Compiling_Bad_LZMA_PROBS
#endif

#define LZMA_LIT_SIZE 0x300

#define LzmaProps_GetNumProbs(p) (NUM_BASE_PROBS + ((UInt32)LZMA_LIT_SIZE << ((p)->lc + (p)->lp)))

#define CALC_POS_STATE(processedPos, pbMask) (((processedPos) & (pbMask)) << 4)
#define COMBINED_PS_STATE (posState + state)
#define GET_LEN_STATE (posState)

#define LZMA_DIC_MIN (1 << 12)

/*
p->remainLen : shows status of LZMA decoder:
    < kMatchSpecLenStart  : the number of bytes to be copied with (p->rep0) offset
    = kMatchSpecLenStart  : the LZMA stream was finished with end mark
    = kMatchSpecLenStart + 1  : need init range coder
    = kMatchSpecLenStart + 2  : need init range coder and state
    = kMatchSpecLen_Error_Fail                : Internal Code Failure
    = kMatchSpecLen_Error_Data + [0 ... 273]  : LZMA Data Error
*/

/* ---------- LZMA_DECODE_REAL ---------- */
/*
LzmaDec_DecodeReal_3() can be implemented in external ASM file.
3 - is the code compatibility version of that function for check at link time.
*/

#define LZMA_DECODE_REAL LzmaDec_DecodeReal_3

/*
LZMA_DECODE_REAL()
In:
  RangeCoder is normalized
  if (p->dicPos == limit)
  {
    LzmaDec_TryDummy() was called before to exclude LITERAL and MATCH-REP cases.
    So first symbol can be only MATCH-NON-REP. And if that MATCH-NON-REP symbol
    is not END_OF_PAYALOAD_MARKER, then the function doesn't write any byte to dictionary,
    the function returns SZ_OK, and the caller can use (p->remainLen) and (p->reps[0]) later.
  }

Processing:
  The first LZMA symbol will be decoded in any case.
  All main checks for limits are at the end of main loop,
  It decodes additional LZMA-symbols while (p->buf < bufLimit && dicPos < limit),
  RangeCoder is still without last normalization when (p->buf < bufLimit) is being checked.
  But if (p->buf < bufLimit), the caller provided at least (LZMA_REQUIRED_INPUT_MAX + 1) bytes for
  next iteration  before limit (bufLimit + LZMA_REQUIRED_INPUT_MAX),
  that is enough for worst case LZMA symbol with one additional RangeCoder normalization for one bit.
  So that function never reads bufLimit [LZMA_REQUIRED_INPUT_MAX] byte.

Out:
  RangeCoder is normalized
  Result:
    SZ_OK - OK
      p->remainLen:
        < kMatchSpecLenStart : the number of bytes to be copied with (p->reps[0]) offset
        = kMatchSpecLenStart : the LZMA stream was finished with end mark

    SZ_ERROR_DATA - error, when the MATCH-Symbol refers out of dictionary
      p->remainLen : undefined
      p->reps[*]    : undefined
*/

#ifdef Z7_LZMA_DEC_OPT

int Z7_FASTCALL LZMA_DECODE_REAL(CLzmaDec *p, SizeT limit, const Byte *bufLimit);

#else

static int Z7_FASTCALL LZMA_DECODE_REAL(CLzmaDec *p, SizeT limit, const Byte *bufLimit)
{
    CLzmaProb *probs = GET_PROBS;
    unsigned state = (unsigned)p->state;
    UInt32 rep0 = p->reps[0], rep1 = p->reps[1], rep2 = p->reps[2], rep3 = p->reps[3];
    unsigned pbMask = ((unsigned)1 << (p->prop.pb)) - 1;
    unsigned lc = p->prop.lc;
    unsigned lpMask = ((unsigned)0x100 << p->prop.lp) - ((unsigned)0x100 >> lc);

    Byte *dic = p->dic;
    SizeT dicBufSize = p->dicBufSize;
    SizeT dicPos = p->dicPos;

    UInt32 processedPos = p->processedPos;
    UInt32 checkDicSize = p->checkDicSize;
    unsigned len = 0;

    const Byte *buf = p->buf;
    UInt32 range = p->range;
    UInt32 code = p->code;

    do {
        CLzmaProb *prob;
        UInt32 bound;
        unsigned ttt;
        unsigned posState = CALC_POS_STATE(processedPos, pbMask);

        prob = probs + IsMatch + COMBINED_PS_STATE;
        IF_BIT_0(prob)
        {
            unsigned symbol;
            UPDATE_0(prob)
            prob = probs + Literal;
            if (processedPos != 0 || checkDicSize != 0) prob += (UInt32)3 * ((((processedPos << 8) + dic[(dicPos == 0 ? dicBufSize : dicPos) - 1]) & lpMask) << lc);
            processedPos++;

            if (state < kNumLitStates) {
                state -= (state < 4) ? state : 3;
                symbol = 1;
#ifdef Z7_LZMA_SIZE_OPT
                do {
                    NORMAL_LITER_DEC
                } while (symbol < 0x100);
#else
                NORMAL_LITER_DEC
                NORMAL_LITER_DEC
                NORMAL_LITER_DEC
                NORMAL_LITER_DEC
                NORMAL_LITER_DEC
                NORMAL_LITER_DEC
                NORMAL_LITER_DEC
                NORMAL_LITER_DEC
#endif
            } else {
                unsigned matchByte = dic[dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0)];
                unsigned offs = 0x100;
                state -= (state < 10) ? 3 : 6;
                symbol = 1;
#ifdef Z7_LZMA_SIZE_OPT
                do {
                    unsigned bit;
                    CLzmaProb *probLit;
                    MATCHED_LITER_DEC
                } while (symbol < 0x100);
#else
                {
                    unsigned bit;
                    CLzmaProb *probLit;
                    MATCHED_LITER_DEC
                    MATCHED_LITER_DEC
                    MATCHED_LITER_DEC
                    MATCHED_LITER_DEC
                    MATCHED_LITER_DEC
                    MATCHED_LITER_DEC
                    MATCHED_LITER_DEC
                    MATCHED_LITER_DEC
                }
#endif
            }

            dic[dicPos++] = (Byte)symbol;
            continue;
        }

        {
            UPDATE_1(prob)
            prob = probs + IsRep + state;
            IF_BIT_0(prob)
            {
                UPDATE_0(prob)
                state += kNumStates;
                prob = probs + LenCoder;
            }
            else
            {
                UPDATE_1(prob)
                prob = probs + IsRepG0 + state;
                IF_BIT_0(prob)
                {
                    UPDATE_0(prob)
                    prob = probs + IsRep0Long + COMBINED_PS_STATE;
                    IF_BIT_0(prob)
                    {
                        UPDATE_0(prob)

                        // that case was checked before with kBadRepCode
                        // if (checkDicSize == 0 && processedPos == 0) { len = kMatchSpecLen_Error_Data + 1; break; }
                        // The caller doesn't allow (dicPos == limit) case here
                        // so we don't need the following check:
                        // if (dicPos == limit) { state = state < kNumLitStates ? 9 : 11; len = 1; break; }

                        dic[dicPos] = dic[dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0)];
                        dicPos++;
                        processedPos++;
                        state = state < kNumLitStates ? 9 : 11;
                        continue;
                    }
                    UPDATE_1(prob)
                }
                else
                {
                    UInt32 distance;
                    UPDATE_1(prob)
                    prob = probs + IsRepG1 + state;
                    IF_BIT_0(prob)
                    {
                        UPDATE_0(prob)
                        distance = rep1;
                    }
                    else
                    {
                        UPDATE_1(prob)
                        prob = probs + IsRepG2 + state;
                        IF_BIT_0(prob)
                        {
                            UPDATE_0(prob)
                            distance = rep2;
                        }
                        else
                        {
                            UPDATE_1(prob)
                            distance = rep3;
                            rep3 = rep2;
                        }
                        rep2 = rep1;
                    }
                    rep1 = rep0;
                    rep0 = distance;
                }
                state = state < kNumLitStates ? 8 : 11;
                prob = probs + RepLenCoder;
            }

#ifdef Z7_LZMA_SIZE_OPT
            {
                unsigned lim, offset;
                CLzmaProb *probLen = prob + LenChoice;
                IF_BIT_0(probLen)
                {
                    UPDATE_0(probLen)
                    probLen = prob + LenLow + GET_LEN_STATE;
                    offset = 0;
                    lim = (1 << kLenNumLowBits);
                }
                else
                {
                    UPDATE_1(probLen)
                    probLen = prob + LenChoice2;
                    IF_BIT_0(probLen)
                    {
                        UPDATE_0(probLen)
                        probLen = prob + LenLow + GET_LEN_STATE + (1 << kLenNumLowBits);
                        offset = kLenNumLowSymbols;
                        lim = (1 << kLenNumLowBits);
                    }
                    else
                    {
                        UPDATE_1(probLen)
                        probLen = prob + LenHigh;
                        offset = kLenNumLowSymbols * 2;
                        lim = (1 << kLenNumHighBits);
                    }
                }
                TREE_DECODE(probLen, lim, len)
                len += offset;
            }
#else
            {
                CLzmaProb *probLen = prob + LenChoice;
                IF_BIT_0(probLen)
                {
                    UPDATE_0(probLen)
                    probLen = prob + LenLow + GET_LEN_STATE;
                    len = 1;
                    TREE_GET_BIT(probLen, len)
                    TREE_GET_BIT(probLen, len)
                    TREE_GET_BIT(probLen, len)
                    len -= 8;
                }
                else
                {
                    UPDATE_1(probLen)
                    probLen = prob + LenChoice2;
                    IF_BIT_0(probLen)
                    {
                        UPDATE_0(probLen)
                        probLen = prob + LenLow + GET_LEN_STATE + (1 << kLenNumLowBits);
                        len = 1;
                        TREE_GET_BIT(probLen, len)
                        TREE_GET_BIT(probLen, len)
                        TREE_GET_BIT(probLen, len)
                    }
                    else
                    {
                        UPDATE_1(probLen)
                        probLen = prob + LenHigh;
                        TREE_DECODE(probLen, (1 << kLenNumHighBits), len)
                        len += kLenNumLowSymbols * 2;
                    }
                }
            }
#endif

            if (state >= kNumStates) {
                UInt32 distance;
                prob = probs + PosSlot + ((size_t)(len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) << kNumPosSlotBits);
                TREE_6_DECODE(prob, distance)
                if (distance >= kStartPosModelIndex) {
                    unsigned posSlot = (unsigned)distance;
                    unsigned numDirectBits = (unsigned)(((distance >> 1) - 1));
                    distance = (2 | (distance & 1));
                    if (posSlot < kEndPosModelIndex) {
                        distance <<= numDirectBits;
                        prob = probs + SpecPos;
                        {
                            UInt32 m = 1;
                            distance++;
                            do {
                                REV_BIT_VAR(prob, distance, m)
                            } while (--numDirectBits);
                            distance -= m;
                        }
                    } else {
                        numDirectBits -= kNumAlignBits;
                        do {
                            NORMALIZE
                            range >>= 1;

                            {
                                UInt32 t;
                                code -= range;
                                t = (0 - ((UInt32)code >> 31)); /* (UInt32)((Int32)code >> 31) */
                                distance = (distance << 1) + (t + 1);
                                code += range & t;
                            }
                            /*
                            distance <<= 1;
                            if (code >= range)
                            {
                              code -= range;
                              distance |= 1;
                            }
                            */
                        } while (--numDirectBits);
                        prob = probs + Align;
                        distance <<= kNumAlignBits;
                        {
                            unsigned i = 1;
                            REV_BIT_CONST(prob, i, 1)
                            REV_BIT_CONST(prob, i, 2)
                            REV_BIT_CONST(prob, i, 4)
                            REV_BIT_LAST(prob, i, 8)
                            distance |= i;
                        }
                        if (distance == (UInt32)0xFFFFFFFF) {
                            len = kMatchSpecLenStart;
                            state -= kNumStates;
                            break;
                        }
                    }
                }

                rep3 = rep2;
                rep2 = rep1;
                rep1 = rep0;
                rep0 = distance + 1;
                state = (state < kNumStates + kNumLitStates) ? kNumLitStates : kNumLitStates + 3;
                if (distance >= (checkDicSize == 0 ? processedPos : checkDicSize)) {
                    len += kMatchSpecLen_Error_Data + kMatchMinLen;
                    // len = kMatchSpecLen_Error_Data;
                    // len += kMatchMinLen;
                    break;
                }
            }

            len += kMatchMinLen;

            {
                SizeT rem;
                unsigned curLen;
                SizeT pos;

                if ((rem = limit - dicPos) == 0) {
                    /*
                    We stop decoding and return SZ_OK, and we can resume decoding later.
                    Any error conditions can be tested later in caller code.
                    For more strict mode we can stop decoding with error
                    // len += kMatchSpecLen_Error_Data;
                    */
                    break;
                }

                curLen = ((rem < len) ? (unsigned)rem : len);
                pos = dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0);

                processedPos += (UInt32)curLen;

                len -= curLen;
                if (curLen <= dicBufSize - pos) {
                    Byte *dest = dic + dicPos;
                    ptrdiff_t src = (ptrdiff_t)pos - (ptrdiff_t)dicPos;
                    const Byte *lim = dest + curLen;
                    dicPos += (SizeT)curLen;
                    do *(dest) = (Byte) * (dest + src);
                    while (++dest != lim);
                } else {
                    do {
                        dic[dicPos++] = dic[pos];
                        if (++pos == dicBufSize) pos = 0;
                    } while (--curLen != 0);
                }
            }
        }
    } while (dicPos < limit && buf < bufLimit);

    NORMALIZE

    p->buf = buf;
    p->range = range;
    p->code = code;
    p->remainLen = (UInt32)len;  // & (kMatchSpecLen_Error_Data - 1); // we can write real length for error matches too.
    p->dicPos = dicPos;
    p->processedPos = processedPos;
    p->reps[0] = rep0;
    p->reps[1] = rep1;
    p->reps[2] = rep2;
    p->reps[3] = rep3;
    p->state = (UInt32)state;
    if (len >= kMatchSpecLen_Error_Data) return SZ_ERROR_DATA;
    return SZ_OK;
}
#endif

static void Z7_FASTCALL LzmaDec_WriteRem(CLzmaDec *p, SizeT limit)
{
    unsigned len = (unsigned)p->remainLen;
    if (len == 0 /* || len >= kMatchSpecLenStart */) return;
    {
        SizeT dicPos = p->dicPos;
        Byte *dic;
        SizeT dicBufSize;
        SizeT rep0; /* we use SizeT to avoid the BUG of VC14 for AMD64 */
        {
            SizeT rem = limit - dicPos;
            if (rem < len) {
                len = (unsigned)(rem);
                if (len == 0) return;
            }
        }

        if (p->checkDicSize == 0 && p->prop.dicSize - p->processedPos <= len) p->checkDicSize = p->prop.dicSize;

        p->processedPos += (UInt32)len;
        p->remainLen -= (UInt32)len;
        dic = p->dic;
        rep0 = p->reps[0];
        dicBufSize = p->dicBufSize;
        do {
            dic[dicPos] = dic[dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0)];
            dicPos++;
        } while (--len);
        p->dicPos = dicPos;
    }
}

/*
At staring of new stream we have one of the following symbols:
  - Literal        - is allowed
  - Non-Rep-Match  - is allowed only if it's end marker symbol
  - Rep-Match      - is not allowed
We use early check of (RangeCoder:Code) over kBadRepCode to simplify main decoding code
*/

#define kRange0 0xFFFFFFFF
#define kBound0 ((kRange0 >> kNumBitModelTotalBits) << (kNumBitModelTotalBits - 1))
#define kBadRepCode (kBound0 + (((kRange0 - kBound0) >> kNumBitModelTotalBits) << (kNumBitModelTotalBits - 1)))
#if kBadRepCode != (0xC0000000 - 0x400)
#error Stop_Compiling_Bad_LZMA_Check
#endif

/*
LzmaDec_DecodeReal2():
  It calls LZMA_DECODE_REAL() and it adjusts limit according (p->checkDicSize).

We correct (p->checkDicSize) after LZMA_DECODE_REAL() and in LzmaDec_WriteRem(),
and we support the following state of (p->checkDicSize):
  if (total_processed < p->prop.dicSize) then
  {
    (total_processed == p->processedPos)
    (p->checkDicSize == 0)
  }
  else
    (p->checkDicSize == p->prop.dicSize)
*/

static int Z7_FASTCALL LzmaDec_DecodeReal2(CLzmaDec *p, SizeT limit, const Byte *bufLimit)
{
    if (p->checkDicSize == 0) {
        UInt32 rem = p->prop.dicSize - p->processedPos;
        if (limit - p->dicPos > rem) limit = p->dicPos + rem;
    }
    {
        int res = LZMA_DECODE_REAL(p, limit, bufLimit);
        if (p->checkDicSize == 0 && p->processedPos >= p->prop.dicSize) p->checkDicSize = p->prop.dicSize;
        return res;
    }
}

typedef enum {
    DUMMY_INPUT_EOF, /* need more input data */
    DUMMY_LIT,
    DUMMY_MATCH,
    DUMMY_REP
} ELzmaDummy;

#define IS_DUMMY_END_MARKER_POSSIBLE(dummyRes) ((dummyRes) == DUMMY_MATCH)

static ELzmaDummy LzmaDec_TryDummy(const CLzmaDec *p, const Byte *buf, const Byte **bufOut)
{
    UInt32 range = p->range;
    UInt32 code = p->code;
    const Byte *bufLimit = *bufOut;
    const CLzmaProb *probs = GET_PROBS;
    unsigned state = (unsigned)p->state;
    ELzmaDummy res;

    for (;;) {
        const CLzmaProb *prob;
        UInt32 bound;
        unsigned ttt;
        unsigned posState = CALC_POS_STATE(p->processedPos, ((unsigned)1 << p->prop.pb) - 1);

        prob = probs + IsMatch + COMBINED_PS_STATE;
        IF_BIT_0_CHECK(prob)
        {
            UPDATE_0_CHECK

            prob = probs + Literal;
            if (p->checkDicSize != 0 || p->processedPos != 0)
                prob += ((UInt32)LZMA_LIT_SIZE * ((((p->processedPos) & (((unsigned)1 << (p->prop.lp)) - 1)) << p->prop.lc) +
                                                  ((unsigned)p->dic[(p->dicPos == 0 ? p->dicBufSize : p->dicPos) - 1] >> (8 - p->prop.lc))));

            if (state < kNumLitStates) {
                unsigned symbol = 1;
                do {
                    GET_BIT_CHECK(prob + symbol, symbol)
                } while (symbol < 0x100);
            } else {
                unsigned matchByte = p->dic[p->dicPos - p->reps[0] + (p->dicPos < p->reps[0] ? p->dicBufSize : 0)];
                unsigned offs = 0x100;
                unsigned symbol = 1;
                do {
                    unsigned bit;
                    const CLzmaProb *probLit;
                    matchByte += matchByte;
                    bit = offs;
                    offs &= matchByte;
                    probLit = prob + (offs + bit + symbol);
                    GET_BIT2_CHECK(probLit, symbol, offs ^= bit;, ;)
                } while (symbol < 0x100);
            }
            res = DUMMY_LIT;
        }
        else
        {
            unsigned len;
            UPDATE_1_CHECK

            prob = probs + IsRep + state;
            IF_BIT_0_CHECK(prob)
            {
                UPDATE_0_CHECK
                state = 0;
                prob = probs + LenCoder;
                res = DUMMY_MATCH;
            }
            else
            {
                UPDATE_1_CHECK
                res = DUMMY_REP;
                prob = probs + IsRepG0 + state;
                IF_BIT_0_CHECK(prob)
                {
                    UPDATE_0_CHECK
                    prob = probs + IsRep0Long + COMBINED_PS_STATE;
                    IF_BIT_0_CHECK(prob)
                    {
                        UPDATE_0_CHECK
                        break;
                    }
                    else
                    {
                        UPDATE_1_CHECK
                    }
                }
                else
                {
                    UPDATE_1_CHECK
                    prob = probs + IsRepG1 + state;
                    IF_BIT_0_CHECK(prob)
                    {
                        UPDATE_0_CHECK
                    }
                    else
                    {
                        UPDATE_1_CHECK
                        prob = probs + IsRepG2 + state;
                        IF_BIT_0_CHECK(prob)
                        {
                            UPDATE_0_CHECK
                        }
                        else
                        {
                            UPDATE_1_CHECK
                        }
                    }
                }
                state = kNumStates;
                prob = probs + RepLenCoder;
            }
            {
                unsigned limit, offset;
                const CLzmaProb *probLen = prob + LenChoice;
                IF_BIT_0_CHECK(probLen)
                {
                    UPDATE_0_CHECK
                    probLen = prob + LenLow + GET_LEN_STATE;
                    offset = 0;
                    limit = 1 << kLenNumLowBits;
                }
                else
                {
                    UPDATE_1_CHECK
                    probLen = prob + LenChoice2;
                    IF_BIT_0_CHECK(probLen)
                    {
                        UPDATE_0_CHECK
                        probLen = prob + LenLow + GET_LEN_STATE + (1 << kLenNumLowBits);
                        offset = kLenNumLowSymbols;
                        limit = 1 << kLenNumLowBits;
                    }
                    else
                    {
                        UPDATE_1_CHECK
                        probLen = prob + LenHigh;
                        offset = kLenNumLowSymbols * 2;
                        limit = 1 << kLenNumHighBits;
                    }
                }
                TREE_DECODE_CHECK(probLen, limit, len)
                len += offset;
            }

            if (state < 4) {
                unsigned posSlot;
                prob = probs + PosSlot + ((size_t)(len < kNumLenToPosStates - 1 ? len : kNumLenToPosStates - 1) << kNumPosSlotBits);
                TREE_DECODE_CHECK(prob, 1 << kNumPosSlotBits, posSlot)
                if (posSlot >= kStartPosModelIndex) {
                    unsigned numDirectBits = ((posSlot >> 1) - 1);

                    if (posSlot < kEndPosModelIndex) {
                        prob = probs + SpecPos + ((size_t)(2 | (posSlot & 1)) << numDirectBits);
                    } else {
                        numDirectBits -= kNumAlignBits;
                        do {
                            NORMALIZE_CHECK
                            range >>= 1;
                            code -= range & (((code - range) >> 31) - 1);
                            /* if (code >= range) code -= range; */
                        } while (--numDirectBits);
                        prob = probs + Align;
                        numDirectBits = kNumAlignBits;
                    }
                    {
                        unsigned i = 1;
                        unsigned m = 1;
                        do {
                            REV_BIT_CHECK(prob, i, m)
                        } while (--numDirectBits);
                    }
                }
            }
        }
        break;
    }
    NORMALIZE_CHECK

    *bufOut = buf;
    return res;
}

void LzmaDec_InitDicAndState(CLzmaDec *p, BoolInt initDic, BoolInt initState);
void LzmaDec_InitDicAndState(CLzmaDec *p, BoolInt initDic, BoolInt initState)
{
    p->remainLen = kMatchSpecLenStart + 1;
    p->tempBufSize = 0;

    if (initDic) {
        p->processedPos = 0;
        p->checkDicSize = 0;
        p->remainLen = kMatchSpecLenStart + 2;
    }
    if (initState) p->remainLen = kMatchSpecLenStart + 2;
}

void LzmaDec_Init(CLzmaDec *p)
{
    p->dicPos = 0;
    LzmaDec_InitDicAndState(p, SZ_True, SZ_True);
}

/*
LZMA supports optional end_marker.
So the decoder can lookahead for one additional LZMA-Symbol to check end_marker.
That additional LZMA-Symbol can require up to LZMA_REQUIRED_INPUT_MAX bytes in input stream.
When the decoder reaches dicLimit, it looks (finishMode) parameter:
  if (finishMode == LZMA_FINISH_ANY), the decoder doesn't lookahead
  if (finishMode != LZMA_FINISH_ANY), the decoder lookahead, if end_marker is possible for current position

When the decoder lookahead, and the lookahead symbol is not end_marker, we have two ways:
  1) Strict mode (default) : the decoder returns SZ_ERROR_DATA.
  2) The relaxed mode (alternative mode) : we could return SZ_OK, and the caller
     must check (status) value. The caller can show the error,
     if the end of stream is expected, and the (status) is noit
     LZMA_STATUS_FINISHED_WITH_MARK or LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK.
*/

#define RETURN_NOT_FINISHED_FOR_FINISH  \
    *status = LZMA_STATUS_NOT_FINISHED; \
    return SZ_ERROR_DATA;  // for strict mode
                           // return SZ_OK; // for relaxed mode

SRes LzmaDec_DecodeToDic(CLzmaDec *p, SizeT dicLimit, const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status)
{
    SizeT inSize = *srcLen;
    (*srcLen) = 0;
    *status = LZMA_STATUS_NOT_SPECIFIED;

    if (p->remainLen > kMatchSpecLenStart) {
        if (p->remainLen > kMatchSpecLenStart + 2) return p->remainLen == kMatchSpecLen_Error_Fail ? SZ_ERROR_FAIL : SZ_ERROR_DATA;

        for (; inSize > 0 && p->tempBufSize < RC_INIT_SIZE; (*srcLen)++, inSize--) p->tempBuf[p->tempBufSize++] = *src++;
        if (p->tempBufSize != 0 && p->tempBuf[0] != 0) return SZ_ERROR_DATA;
        if (p->tempBufSize < RC_INIT_SIZE) {
            *status = LZMA_STATUS_NEEDS_MORE_INPUT;
            return SZ_OK;
        }
        p->code = ((UInt32)p->tempBuf[1] << 24) | ((UInt32)p->tempBuf[2] << 16) | ((UInt32)p->tempBuf[3] << 8) | ((UInt32)p->tempBuf[4]);

        if (p->checkDicSize == 0 && p->processedPos == 0 && p->code >= kBadRepCode) return SZ_ERROR_DATA;

        p->range = 0xFFFFFFFF;
        p->tempBufSize = 0;

        if (p->remainLen > kMatchSpecLenStart + 1) {
            SizeT numProbs = LzmaProps_GetNumProbs(&p->prop);
            SizeT i;
            CLzmaProb *probs = p->probs;
            for (i = 0; i < numProbs; i++) probs[i] = kBitModelTotal >> 1;
            p->reps[0] = p->reps[1] = p->reps[2] = p->reps[3] = 1;
            p->state = 0;
        }

        p->remainLen = 0;
    }

    for (;;) {
        if (p->remainLen == kMatchSpecLenStart) {
            if (p->code != 0) return SZ_ERROR_DATA;
            *status = LZMA_STATUS_FINISHED_WITH_MARK;
            return SZ_OK;
        }

        LzmaDec_WriteRem(p, dicLimit);

        {
            // (p->remainLen == 0 || p->dicPos == dicLimit)

            int checkEndMarkNow = 0;

            if (p->dicPos >= dicLimit) {
                if (p->remainLen == 0 && p->code == 0) {
                    *status = LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK;
                    return SZ_OK;
                }
                if (finishMode == LZMA_FINISH_ANY) {
                    *status = LZMA_STATUS_NOT_FINISHED;
                    return SZ_OK;
                }
                if (p->remainLen != 0) {
                    RETURN_NOT_FINISHED_FOR_FINISH
                }
                checkEndMarkNow = 1;
            }

            // (p->remainLen == 0)

            if (p->tempBufSize == 0) {
                const Byte *bufLimit;
                int dummyProcessed = -1;

                if (inSize < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow) {
                    const Byte *bufOut = src + inSize;

                    ELzmaDummy dummyRes = LzmaDec_TryDummy(p, src, &bufOut);

                    if (dummyRes == DUMMY_INPUT_EOF) {
                        size_t i;
                        if (inSize >= LZMA_REQUIRED_INPUT_MAX) break;
                        (*srcLen) += inSize;
                        p->tempBufSize = (unsigned)inSize;
                        for (i = 0; i < inSize; i++) p->tempBuf[i] = src[i];
                        *status = LZMA_STATUS_NEEDS_MORE_INPUT;
                        return SZ_OK;
                    }

                    dummyProcessed = (int)(bufOut - src);
                    if ((unsigned)dummyProcessed > LZMA_REQUIRED_INPUT_MAX) break;

                    if (checkEndMarkNow && !IS_DUMMY_END_MARKER_POSSIBLE(dummyRes)) {
                        unsigned i;
                        (*srcLen) += (unsigned)dummyProcessed;
                        p->tempBufSize = (unsigned)dummyProcessed;
                        for (i = 0; i < (unsigned)dummyProcessed; i++) p->tempBuf[i] = src[i];
                        // p->remainLen = kMatchSpecLen_Error_Data;
                        RETURN_NOT_FINISHED_FOR_FINISH
                    }

                    bufLimit = src;
                    // we will decode only one iteration
                } else bufLimit = src + inSize - LZMA_REQUIRED_INPUT_MAX;

                p->buf = src;

                {
                    int res = LzmaDec_DecodeReal2(p, dicLimit, bufLimit);

                    SizeT processed = (SizeT)(p->buf - src);

                    if (dummyProcessed < 0) {
                        if (processed > inSize) break;
                    } else if ((unsigned)dummyProcessed != processed) break;

                    src += processed;
                    inSize -= processed;
                    (*srcLen) += processed;

                    if (res != SZ_OK) {
                        p->remainLen = kMatchSpecLen_Error_Data;
                        return SZ_ERROR_DATA;
                    }
                }
                continue;
            }

            {
                // we have some data in (p->tempBuf)
                // in strict mode: tempBufSize is not enough for one Symbol decoding.
                // in relaxed mode: tempBufSize not larger than required for one Symbol decoding.

                unsigned rem = p->tempBufSize;
                unsigned ahead = 0;
                int dummyProcessed = -1;

                while (rem < LZMA_REQUIRED_INPUT_MAX && ahead < inSize) p->tempBuf[rem++] = src[ahead++];

                // ahead - the size of new data copied from (src) to (p->tempBuf)
                // rem   - the size of temp buffer including new data from (src)

                if (rem < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow) {
                    const Byte *bufOut = p->tempBuf + rem;

                    ELzmaDummy dummyRes = LzmaDec_TryDummy(p, p->tempBuf, &bufOut);

                    if (dummyRes == DUMMY_INPUT_EOF) {
                        if (rem >= LZMA_REQUIRED_INPUT_MAX) break;
                        p->tempBufSize = rem;
                        (*srcLen) += (SizeT)ahead;
                        *status = LZMA_STATUS_NEEDS_MORE_INPUT;
                        return SZ_OK;
                    }

                    dummyProcessed = (int)(bufOut - p->tempBuf);

                    if ((unsigned)dummyProcessed < p->tempBufSize) break;

                    if (checkEndMarkNow && !IS_DUMMY_END_MARKER_POSSIBLE(dummyRes)) {
                        (*srcLen) += (unsigned)dummyProcessed - p->tempBufSize;
                        p->tempBufSize = (unsigned)dummyProcessed;
                        // p->remainLen = kMatchSpecLen_Error_Data;
                        RETURN_NOT_FINISHED_FOR_FINISH
                    }
                }

                p->buf = p->tempBuf;

                {
                    // we decode one symbol from (p->tempBuf) here, so the (bufLimit) is equal to (p->buf)
                    int res = LzmaDec_DecodeReal2(p, dicLimit, p->buf);

                    SizeT processed = (SizeT)(p->buf - p->tempBuf);
                    rem = p->tempBufSize;

                    if (dummyProcessed < 0) {
                        if (processed > LZMA_REQUIRED_INPUT_MAX) break;
                        if (processed < rem) break;
                    } else if ((unsigned)dummyProcessed != processed) break;

                    processed -= rem;

                    src += processed;
                    inSize -= processed;
                    (*srcLen) += processed;
                    p->tempBufSize = 0;

                    if (res != SZ_OK) {
                        p->remainLen = kMatchSpecLen_Error_Data;
                        return SZ_ERROR_DATA;
                    }
                }
            }
        }
    }

    /*  Some unexpected error: internal error of code, memory corruption or hardware failure */
    p->remainLen = kMatchSpecLen_Error_Fail;
    return SZ_ERROR_FAIL;
}

SRes LzmaDec_DecodeToBuf(CLzmaDec *p, Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status)
{
    SizeT outSize = *destLen;
    SizeT inSize = *srcLen;
    *srcLen = *destLen = 0;
    for (;;) {
        SizeT inSizeCur = inSize, outSizeCur, dicPos;
        ELzmaFinishMode curFinishMode;
        SRes res;
        if (p->dicPos == p->dicBufSize) p->dicPos = 0;
        dicPos = p->dicPos;
        if (outSize > p->dicBufSize - dicPos) {
            outSizeCur = p->dicBufSize;
            curFinishMode = LZMA_FINISH_ANY;
        } else {
            outSizeCur = dicPos + outSize;
            curFinishMode = finishMode;
        }

        res = LzmaDec_DecodeToDic(p, outSizeCur, src, &inSizeCur, curFinishMode, status);
        src += inSizeCur;
        inSize -= inSizeCur;
        *srcLen += inSizeCur;
        outSizeCur = p->dicPos - dicPos;
        memcpy(dest, p->dic + dicPos, outSizeCur);
        dest += outSizeCur;
        outSize -= outSizeCur;
        *destLen += outSizeCur;
        if (res != 0) return res;
        if (outSizeCur == 0 || outSize == 0) return SZ_OK;
    }
}

void LzmaDec_FreeProbs(CLzmaDec *p, ISzAllocPtr alloc)
{
    ISzAlloc_Free(alloc, p->probs);
    p->probs = NULL;
}

static void LzmaDec_FreeDict(CLzmaDec *p, ISzAllocPtr alloc)
{
    ISzAlloc_Free(alloc, p->dic);
    p->dic = NULL;
}

void LzmaDec_Free(CLzmaDec *p, ISzAllocPtr alloc)
{
    LzmaDec_FreeProbs(p, alloc);
    LzmaDec_FreeDict(p, alloc);
}

SRes LzmaProps_Decode(CLzmaProps *p, const Byte *data, unsigned size)
{
    UInt32 dicSize;
    Byte d;

    if (size < LZMA_PROPS_SIZE) return SZ_ERROR_UNSUPPORTED;
    else dicSize = data[1] | ((UInt32)data[2] << 8) | ((UInt32)data[3] << 16) | ((UInt32)data[4] << 24);

    if (dicSize < LZMA_DIC_MIN) dicSize = LZMA_DIC_MIN;
    p->dicSize = dicSize;

    d = data[0];
    if (d >= (9 * 5 * 5)) return SZ_ERROR_UNSUPPORTED;

    p->lc = (Byte)(d % 9);
    d /= 9;
    p->pb = (Byte)(d / 5);
    p->lp = (Byte)(d % 5);

    return SZ_OK;
}

static SRes LzmaDec_AllocateProbs2(CLzmaDec *p, const CLzmaProps *propNew, ISzAllocPtr alloc)
{
    UInt32 numProbs = LzmaProps_GetNumProbs(propNew);
    if (!p->probs || numProbs != p->numProbs) {
        LzmaDec_FreeProbs(p, alloc);
        p->probs = (CLzmaProb *)ISzAlloc_Alloc(alloc, numProbs * sizeof(CLzmaProb));
        if (!p->probs) return SZ_ERROR_MEM;
        p->probs_1664 = p->probs + 1664;
        p->numProbs = numProbs;
    }
    return SZ_OK;
}

SRes LzmaDec_AllocateProbs(CLzmaDec *p, const Byte *props, unsigned propsSize, ISzAllocPtr alloc)
{
    CLzmaProps propNew;
    RINOK(LzmaProps_Decode(&propNew, props, propsSize))
    RINOK(LzmaDec_AllocateProbs2(p, &propNew, alloc))
    p->prop = propNew;
    return SZ_OK;
}

SRes LzmaDec_Allocate(CLzmaDec *p, const Byte *props, unsigned propsSize, ISzAllocPtr alloc)
{
    CLzmaProps propNew;
    SizeT dicBufSize;
    RINOK(LzmaProps_Decode(&propNew, props, propsSize))
    RINOK(LzmaDec_AllocateProbs2(p, &propNew, alloc))

    {
        UInt32 dictSize = propNew.dicSize;
        SizeT mask = ((UInt32)1 << 12) - 1;
        if (dictSize >= ((UInt32)1 << 30)) mask = ((UInt32)1 << 22) - 1;
        else if (dictSize >= ((UInt32)1 << 22)) mask = ((UInt32)1 << 20) - 1;
        dicBufSize = ((SizeT)dictSize + mask) & ~mask;
        if (dicBufSize < dictSize) dicBufSize = dictSize;
    }

    if (!p->dic || dicBufSize != p->dicBufSize) {
        LzmaDec_FreeDict(p, alloc);
        p->dic = (Byte *)ISzAlloc_Alloc(alloc, dicBufSize);
        if (!p->dic) {
            LzmaDec_FreeProbs(p, alloc);
            return SZ_ERROR_MEM;
        }
    }
    p->dicBufSize = dicBufSize;
    p->prop = propNew;
    return SZ_OK;
}

SRes LzmaDecode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen, const Byte *propData, unsigned propSize, ELzmaFinishMode finishMode, ELzmaStatus *status,
                ISzAllocPtr alloc)
{
    CLzmaDec p;
    SRes res;
    SizeT outSize = *destLen, inSize = *srcLen;
    *destLen = *srcLen = 0;
    *status = LZMA_STATUS_NOT_SPECIFIED;
    if (inSize < RC_INIT_SIZE) return SZ_ERROR_INPUT_EOF;
    LzmaDec_CONSTRUCT(&p) RINOK(LzmaDec_AllocateProbs(&p, propData, propSize, alloc)) p.dic = dest;
    p.dicBufSize = outSize;
    LzmaDec_Init(&p);
    *srcLen = inSize;
    res = LzmaDec_DecodeToDic(&p, outSize, src, srcLen, finishMode, status);
    *destLen = p.dicPos;
    if (res == SZ_OK && *status == LZMA_STATUS_NEEDS_MORE_INPUT) res = SZ_ERROR_INPUT_EOF;
    LzmaDec_FreeProbs(&p, alloc);
    return res;
}
#undef LzmaDecode
#undef LzmaDec_Allocate
#undef LzmaDec_AllocateProbs
#undef LzmaProps_Decode
#undef LzmaDec_Free
#undef LzmaDec_FreeProbs
#undef LzmaDec_DecodeToBuf
#undef LzmaDec_DecodeToDic
#undef LzmaDec_Init
#undef LzmaDec_InitDicAndState

#define LzmaDec_InitDicAndState X_LzmaDec_InitDicAndState
#define LzmaDec_Init X_LzmaDec_Init
#define LzmaDec_DecodeToDic X_LzmaDec_DecodeToDic
#define LzmaDec_AllocateProbs X_LzmaDec_AllocateProbs
#define LzmaDec_Allocate X_LzmaDec_Allocate
#define LzmaDec_FreeProbs X_LzmaDec_FreeProbs
#define Lzma2Dec_AllocateProbs X_Lzma2Dec_AllocateProbs
#define Lzma2Dec_Allocate X_Lzma2Dec_Allocate
#define Lzma2Dec_Init X_Lzma2Dec_Init
#define Lzma2Dec_DecodeToDic X_Lzma2Dec_DecodeToDic
#define Lzma2Dec_Parse X_Lzma2Dec_Parse
#define Lzma2Dec_DecodeToBuf X_Lzma2Dec_DecodeToBuf
#define Lzma2Decode X_Lzma2Decode
/* Lzma2Dec.c -- LZMA2 Decoder
2024-03-01 : Igor Pavlov : Public domain */

/* #define SHOW_DEBUG_INFO */
#ifdef SHOW_DEBUG_INFO
#include <stdio.h>
#endif

#include <string.h>
/*
00000000  -  End of data
00000001 U U  -  Uncompressed, reset dic, need reset state and set new prop
00000010 U U  -  Uncompressed, no reset
100uuuuu U U P P  -  LZMA, no reset
101uuuuu U U P P  -  LZMA, reset state
110uuuuu U U P P S  -  LZMA, reset state + set new prop
111uuuuu U U P P S  -  LZMA, reset state + set new prop, reset dic

  u, U - Unpack Size
  P - Pack Size
  S - Props
*/

#define LZMA2_CONTROL_COPY_RESET_DIC 1

#define LZMA2_IS_UNCOMPRESSED_STATE(p) (((p)->control & (1 << 7)) == 0)

#define LZMA2_LCLP_MAX 4
#define LZMA2_DIC_SIZE_FROM_PROP(p) (((UInt32)2 | ((p) & 1)) << ((p) / 2 + 11))

#ifdef SHOW_DEBUG_INFO
#define PRF(x) x
#else
#define PRF(x)
#endif

typedef enum {
    LZMA2_STATE_CONTROL,
    LZMA2_STATE_UNPACK0,
    LZMA2_STATE_UNPACK1,
    LZMA2_STATE_PACK0,
    LZMA2_STATE_PACK1,
    LZMA2_STATE_PROP,
    LZMA2_STATE_DATA,
    LZMA2_STATE_DATA_CONT,
    LZMA2_STATE_FINISHED,
    LZMA2_STATE_ERROR
} ELzma2State;

static SRes Lzma2Dec_GetOldProps(Byte prop, Byte *props)
{
    UInt32 dicSize;
    if (prop > 40) return SZ_ERROR_UNSUPPORTED;
    dicSize = (prop == 40) ? 0xFFFFFFFF : LZMA2_DIC_SIZE_FROM_PROP(prop);
    props[0] = (Byte)LZMA2_LCLP_MAX;
    props[1] = (Byte)(dicSize);
    props[2] = (Byte)(dicSize >> 8);
    props[3] = (Byte)(dicSize >> 16);
    props[4] = (Byte)(dicSize >> 24);
    return SZ_OK;
}

SRes Lzma2Dec_AllocateProbs(CLzma2Dec *p, Byte prop, ISzAllocPtr alloc)
{
    Byte props[LZMA_PROPS_SIZE];
    RINOK(Lzma2Dec_GetOldProps(prop, props))
    return LzmaDec_AllocateProbs(&p->decoder, props, LZMA_PROPS_SIZE, alloc);
}

SRes Lzma2Dec_Allocate(CLzma2Dec *p, Byte prop, ISzAllocPtr alloc)
{
    Byte props[LZMA_PROPS_SIZE];
    RINOK(Lzma2Dec_GetOldProps(prop, props))
    return LzmaDec_Allocate(&p->decoder, props, LZMA_PROPS_SIZE, alloc);
}

void Lzma2Dec_Init(CLzma2Dec *p)
{
    p->state = LZMA2_STATE_CONTROL;
    p->needInitLevel = 0xE0;
    p->isExtraMode = SZ_False;
    p->unpackSize = 0;

    // p->decoder.dicPos = 0; // we can use it instead of full init
    LzmaDec_Init(&p->decoder);
}

// ELzma2State
static unsigned Lzma2Dec_UpdateState(CLzma2Dec *p, Byte b)
{
    switch (p->state) {
        case LZMA2_STATE_CONTROL:
            p->isExtraMode = SZ_False;
            p->control = b;
            PRF(printf("\n %8X", (unsigned)p->decoder.dicPos));
            PRF(printf(" %02X", (unsigned)b));
            if (b == 0) return LZMA2_STATE_FINISHED;
            if (LZMA2_IS_UNCOMPRESSED_STATE(p)) {
                if (b == LZMA2_CONTROL_COPY_RESET_DIC) p->needInitLevel = 0xC0;
                else if (b > 2 || p->needInitLevel == 0xE0) return LZMA2_STATE_ERROR;
            } else {
                if (b < p->needInitLevel) return LZMA2_STATE_ERROR;
                p->needInitLevel = 0;
                p->unpackSize = (UInt32)(b & 0x1F) << 16;
            }
            return LZMA2_STATE_UNPACK0;

        case LZMA2_STATE_UNPACK0: p->unpackSize |= (UInt32)b << 8; return LZMA2_STATE_UNPACK1;

        case LZMA2_STATE_UNPACK1:
            p->unpackSize |= (UInt32)b;
            p->unpackSize++;
            PRF(printf(" %7u", (unsigned)p->unpackSize));
            return LZMA2_IS_UNCOMPRESSED_STATE(p) ? LZMA2_STATE_DATA : LZMA2_STATE_PACK0;

        case LZMA2_STATE_PACK0: p->packSize = (UInt32)b << 8; return LZMA2_STATE_PACK1;

        case LZMA2_STATE_PACK1:
            p->packSize |= (UInt32)b;
            p->packSize++;
            // if (p->packSize < 5) return LZMA2_STATE_ERROR;
            PRF(printf(" %5u", (unsigned)p->packSize));
            return (p->control & 0x40) ? LZMA2_STATE_PROP : LZMA2_STATE_DATA;

        case LZMA2_STATE_PROP: {
            unsigned lc, lp;
            if (b >= (9 * 5 * 5)) return LZMA2_STATE_ERROR;
            lc = b % 9;
            b /= 9;
            p->decoder.prop.pb = (Byte)(b / 5);
            lp = b % 5;
            if (lc + lp > LZMA2_LCLP_MAX) return LZMA2_STATE_ERROR;
            p->decoder.prop.lc = (Byte)lc;
            p->decoder.prop.lp = (Byte)lp;
            return LZMA2_STATE_DATA;
        }

        default: return LZMA2_STATE_ERROR;
    }
}

static void LzmaDec_UpdateWithUncompressed(CLzmaDec *p, const Byte *src, SizeT size)
{
    memcpy(p->dic + p->dicPos, src, size);
    p->dicPos += size;
    if (p->checkDicSize == 0 && p->prop.dicSize - p->processedPos <= size) p->checkDicSize = p->prop.dicSize;
    p->processedPos += (UInt32)size;
}

void LzmaDec_InitDicAndState(CLzmaDec *p, BoolInt initDic, BoolInt initState);

SRes Lzma2Dec_DecodeToDic(CLzma2Dec *p, SizeT dicLimit, const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status)
{
    SizeT inSize = *srcLen;
    *srcLen = 0;
    *status = LZMA_STATUS_NOT_SPECIFIED;

    while (p->state != LZMA2_STATE_ERROR) {
        SizeT dicPos;

        if (p->state == LZMA2_STATE_FINISHED) {
            *status = LZMA_STATUS_FINISHED_WITH_MARK;
            return SZ_OK;
        }

        dicPos = p->decoder.dicPos;

        if (dicPos == dicLimit && finishMode == LZMA_FINISH_ANY) {
            *status = LZMA_STATUS_NOT_FINISHED;
            return SZ_OK;
        }

        if (p->state != LZMA2_STATE_DATA && p->state != LZMA2_STATE_DATA_CONT) {
            if (*srcLen == inSize) {
                *status = LZMA_STATUS_NEEDS_MORE_INPUT;
                return SZ_OK;
            }
            (*srcLen)++;
            p->state = Lzma2Dec_UpdateState(p, *src++);
            if (dicPos == dicLimit && p->state != LZMA2_STATE_FINISHED) break;
            continue;
        }

        {
            SizeT inCur = inSize - *srcLen;
            SizeT outCur = dicLimit - dicPos;
            ELzmaFinishMode curFinishMode = LZMA_FINISH_ANY;

            if (outCur >= p->unpackSize) {
                outCur = (SizeT)p->unpackSize;
                curFinishMode = LZMA_FINISH_END;
            }

            if (LZMA2_IS_UNCOMPRESSED_STATE(p)) {
                if (inCur == 0) {
                    *status = LZMA_STATUS_NEEDS_MORE_INPUT;
                    return SZ_OK;
                }

                if (p->state == LZMA2_STATE_DATA) {
                    BoolInt initDic = (p->control == LZMA2_CONTROL_COPY_RESET_DIC);
                    LzmaDec_InitDicAndState(&p->decoder, initDic, SZ_False);
                }

                if (inCur > outCur) inCur = outCur;
                if (inCur == 0) break;

                LzmaDec_UpdateWithUncompressed(&p->decoder, src, inCur);

                src += inCur;
                *srcLen += inCur;
                p->unpackSize -= (UInt32)inCur;
                p->state = (p->unpackSize == 0) ? LZMA2_STATE_CONTROL : LZMA2_STATE_DATA_CONT;
            } else {
                SRes res;

                if (p->state == LZMA2_STATE_DATA) {
                    BoolInt initDic = (p->control >= 0xE0);
                    BoolInt initState = (p->control >= 0xA0);
                    LzmaDec_InitDicAndState(&p->decoder, initDic, initState);
                    p->state = LZMA2_STATE_DATA_CONT;
                }

                if (inCur > p->packSize) inCur = (SizeT)p->packSize;

                res = LzmaDec_DecodeToDic(&p->decoder, dicPos + outCur, src, &inCur, curFinishMode, status);

                src += inCur;
                *srcLen += inCur;
                p->packSize -= (UInt32)inCur;
                outCur = p->decoder.dicPos - dicPos;
                p->unpackSize -= (UInt32)outCur;

                if (res != 0) break;

                if (*status == LZMA_STATUS_NEEDS_MORE_INPUT) {
                    if (p->packSize == 0) break;
                    return SZ_OK;
                }

                if (inCur == 0 && outCur == 0) {
                    if (*status != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK || p->unpackSize != 0 || p->packSize != 0) break;
                    p->state = LZMA2_STATE_CONTROL;
                }

                *status = LZMA_STATUS_NOT_SPECIFIED;
            }
        }
    }

    *status = LZMA_STATUS_NOT_SPECIFIED;
    p->state = LZMA2_STATE_ERROR;
    return SZ_ERROR_DATA;
}

ELzma2ParseStatus Lzma2Dec_Parse(CLzma2Dec *p, SizeT outSize, const Byte *src, SizeT *srcLen, int checkFinishBlock)
{
    SizeT inSize = *srcLen;
    *srcLen = 0;

    while (p->state != LZMA2_STATE_ERROR) {
        if (p->state == LZMA2_STATE_FINISHED) return (ELzma2ParseStatus)LZMA_STATUS_FINISHED_WITH_MARK;

        if (outSize == 0 && !checkFinishBlock) return (ELzma2ParseStatus)LZMA_STATUS_NOT_FINISHED;

        if (p->state != LZMA2_STATE_DATA && p->state != LZMA2_STATE_DATA_CONT) {
            if (*srcLen == inSize) return (ELzma2ParseStatus)LZMA_STATUS_NEEDS_MORE_INPUT;
            (*srcLen)++;

            p->state = Lzma2Dec_UpdateState(p, *src++);

            if (p->state == LZMA2_STATE_UNPACK0) {
                // if (p->decoder.dicPos != 0)
                if (p->control == LZMA2_CONTROL_COPY_RESET_DIC || p->control >= 0xE0) return LZMA2_PARSE_STATUS_NEW_BLOCK;
                // if (outSize == 0) return LZMA_STATUS_NOT_FINISHED;
            }

            // The following code can be commented.
            // It's not big problem, if we read additional input bytes.
            // It will be stopped later in LZMA2_STATE_DATA / LZMA2_STATE_DATA_CONT state.

            if (outSize == 0 && p->state != LZMA2_STATE_FINISHED) {
                // checkFinishBlock is true. So we expect that block must be finished,
                // We can return LZMA_STATUS_NOT_SPECIFIED or LZMA_STATUS_NOT_FINISHED here
                // break;
                return (ELzma2ParseStatus)LZMA_STATUS_NOT_FINISHED;
            }

            if (p->state == LZMA2_STATE_DATA) return LZMA2_PARSE_STATUS_NEW_CHUNK;

            continue;
        }

        if (outSize == 0) return (ELzma2ParseStatus)LZMA_STATUS_NOT_FINISHED;

        {
            SizeT inCur = inSize - *srcLen;

            if (LZMA2_IS_UNCOMPRESSED_STATE(p)) {
                if (inCur == 0) return (ELzma2ParseStatus)LZMA_STATUS_NEEDS_MORE_INPUT;
                if (inCur > p->unpackSize) inCur = p->unpackSize;
                if (inCur > outSize) inCur = outSize;
                p->decoder.dicPos += inCur;
                src += inCur;
                *srcLen += inCur;
                outSize -= inCur;
                p->unpackSize -= (UInt32)inCur;
                p->state = (p->unpackSize == 0) ? LZMA2_STATE_CONTROL : LZMA2_STATE_DATA_CONT;
            } else {
                p->isExtraMode = SZ_True;

                if (inCur == 0) {
                    if (p->packSize != 0) return (ELzma2ParseStatus)LZMA_STATUS_NEEDS_MORE_INPUT;
                } else if (p->state == LZMA2_STATE_DATA) {
                    p->state = LZMA2_STATE_DATA_CONT;
                    if (*src != 0) {
                        // first byte of lzma chunk must be Zero
                        *srcLen += 1;
                        p->packSize--;
                        break;
                    }
                }

                if (inCur > p->packSize) inCur = (SizeT)p->packSize;

                src += inCur;
                *srcLen += inCur;
                p->packSize -= (UInt32)inCur;

                if (p->packSize == 0) {
                    SizeT rem = outSize;
                    if (rem > p->unpackSize) rem = p->unpackSize;
                    p->decoder.dicPos += rem;
                    p->unpackSize -= (UInt32)rem;
                    outSize -= rem;
                    if (p->unpackSize == 0) p->state = LZMA2_STATE_CONTROL;
                }
            }
        }
    }

    p->state = LZMA2_STATE_ERROR;
    return (ELzma2ParseStatus)LZMA_STATUS_NOT_SPECIFIED;
}

SRes Lzma2Dec_DecodeToBuf(CLzma2Dec *p, Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status)
{
    SizeT outSize = *destLen, inSize = *srcLen;
    *srcLen = *destLen = 0;

    for (;;) {
        SizeT inCur = inSize, outCur, dicPos;
        ELzmaFinishMode curFinishMode;
        SRes res;

        if (p->decoder.dicPos == p->decoder.dicBufSize) p->decoder.dicPos = 0;
        dicPos = p->decoder.dicPos;
        curFinishMode = LZMA_FINISH_ANY;
        outCur = p->decoder.dicBufSize - dicPos;

        if (outCur >= outSize) {
            outCur = outSize;
            curFinishMode = finishMode;
        }

        res = Lzma2Dec_DecodeToDic(p, dicPos + outCur, src, &inCur, curFinishMode, status);

        src += inCur;
        inSize -= inCur;
        *srcLen += inCur;
        outCur = p->decoder.dicPos - dicPos;
        memcpy(dest, p->decoder.dic + dicPos, outCur);
        dest += outCur;
        outSize -= outCur;
        *destLen += outCur;
        if (res != 0) return res;
        if (outCur == 0 || outSize == 0) return SZ_OK;
    }
}

SRes Lzma2Decode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen, Byte prop, ELzmaFinishMode finishMode, ELzmaStatus *status, ISzAllocPtr alloc)
{
    CLzma2Dec p;
    SRes res;
    SizeT outSize = *destLen, inSize = *srcLen;
    *destLen = *srcLen = 0;
    *status = LZMA_STATUS_NOT_SPECIFIED;
    Lzma2Dec_CONSTRUCT(&p) RINOK(Lzma2Dec_AllocateProbs(&p, prop, alloc)) p.decoder.dic = dest;
    p.decoder.dicBufSize = outSize;
    Lzma2Dec_Init(&p);
    *srcLen = inSize;
    res = Lzma2Dec_DecodeToDic(&p, outSize, src, srcLen, finishMode, status);
    *destLen = p.decoder.dicPos;
    if (res == SZ_OK && *status == LZMA_STATUS_NEEDS_MORE_INPUT) res = SZ_ERROR_INPUT_EOF;
    Lzma2Dec_FreeProbs(&p, alloc);
    return res;
}

#undef PRF
#undef Lzma2Decode
#undef Lzma2Dec_DecodeToBuf
#undef Lzma2Dec_Parse
#undef Lzma2Dec_DecodeToDic
#undef Lzma2Dec_Init
#undef Lzma2Dec_Allocate
#undef Lzma2Dec_AllocateProbs
#undef LzmaDec_Allocate
#undef LzmaDec_AllocateProbs
#undef LzmaDec_FreeProbs
#undef LzmaDec_DecodeToDic
#undef LzmaDec_Init
#undef LzmaDec_InitDicAndState
/* ===== End embedded xlzma_local.c ===== */
