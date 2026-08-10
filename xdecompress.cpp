/* Copyright (c) 2023-2026 hors<horsicq@gmail.com>
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
#include "xdecompress.h"
#include "subdevice.h"
#include "xpng.h"
#include "Algos/algo_utils.h"
#include <algorithm>
#include <limits>

namespace {
class DecBoundedReadDevice : public QIODevice {
public:
    DecBoundedReadDevice(QIODevice *pSource, qint64 nLimit)
        : m_pSource(pSource), m_nLimit(nLimit), m_nConsumed(0), m_bError(false)
    {
    }

    bool isSequential() const override { return true; }
    qint64 consumed() const { return m_nConsumed; }
    bool hasError() const { return m_bError; }

protected:
    qint64 readData(char *pData, qint64 nMaximumSize) override
    {
        if (!m_pSource || (nMaximumSize < 0) || ((nMaximumSize > 0) && !pData) || (m_nConsumed < 0) || (m_nConsumed > m_nLimit)) {
            m_bError = true;
            return -1;
        }

        const qint64 nRemaining = m_nLimit - m_nConsumed;
        if ((nMaximumSize == 0) || (nRemaining == 0)) {
            return 0;
        }

        const qint64 nRequest = (std::min)(nMaximumSize, nRemaining);
        const qint64 nResult = m_pSource->read(pData, nRequest);
        if ((nResult < 0) || (nResult > nRequest)) {
            m_bError = true;
            return -1;
        }
        if (nResult == 0) {
            m_bError = true;
            return 0;
        }

        m_nConsumed += nResult;
        return nResult;
    }

    qint64 writeData(const char *, qint64) override { return -1; }

private:
    QIODevice *m_pSource;
    qint64 m_nLimit;
    qint64 m_nConsumed;
    bool m_bError;
};

class DecWindowWriteDevice : public QIODevice {
public:
    explicit DecWindowWriteDevice(XBinary::DATAPROCESS_STATE *pState) : m_pState(pState), m_bError(false) {}

    bool isSequential() const override { return true; }
    bool hasError() const { return m_bError; }

protected:
    qint64 readData(char *, qint64) override { return -1; }

    qint64 writeData(const char *pData, qint64 nSize) override
    {
        if (!m_pState || (nSize < 0) || ((nSize > 0) && !pData)) {
            m_bError = true;
            return -1;
        }

        qint64 nDone = 0;
        while (nDone < nSize) {
            const qint32 nChunk = (qint32)(std::min)(nSize - nDone, (qint64)(std::numeric_limits<qint32>::max)());
            if (XBinary::_writeDevice(pData + nDone, nChunk, m_pState) != nChunk) {
                m_bError = true;
                return -1;
            }
            nDone += nChunk;
        }

        return nSize;
    }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    bool m_bError;
};

class DecDiscardWriteDevice : public QIODevice {
public:
    bool isSequential() const override { return true; }

protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *pData, qint64 nSize) override
    {
        return ((nSize >= 0) && ((nSize == 0) || pData)) ? nSize : -1;
    }
};

static bool decPrepareBoundedInput(QIODevice *pDevice, qint64 nOffset, qint64 nLimit, qint64 *pnEffectiveLimit)
{
    const qint64 nMax = (std::numeric_limits<qint64>::max)();
    if (!pDevice || !pnEffectiveLimit || (nOffset < 0) || (nLimit < -1) || ((nLimit != -1) && (nOffset > (nMax - nLimit)))) {
        return false;
    }

    const qint64 nDeviceSize = pDevice->size();
    if (nLimit == -1) {
        if ((nDeviceSize < 0) || (nOffset > nDeviceSize)) {
            return false;
        }
        nLimit = nDeviceSize - nOffset;
    } else if (!pDevice->isSequential() && (nDeviceSize >= 0) && ((nOffset > nDeviceSize) || (nLimit > (nDeviceSize - nOffset)))) {
        return false;
    }

    if (!pDevice->seek(nOffset) && (pDevice->pos() != nOffset)) {
        return false;
    }

    *pnEffectiveLimit = nLimit;
    return true;
}
}  // namespace

XDecompress::XDecompress(QObject *parent) : QObject(parent)
{
    m_pCurrentSolidDevice = nullptr;
    m_pRarUnpacker = nullptr;
    m_nRarSolidIndex = 0;
}

// A decompressed size is usable as a QByteArray length only if it is non-negative
// and fits in the qint32 that QByteArray::resize takes.
static bool decIsValidBufferSize(qint64 nSize)
{
    return (nSize >= 0) && (nSize <= (std::numeric_limits<qint32>::max)());
}

static bool decReadInputToByteArray(XBinary::DATAPROCESS_STATE *pState, QByteArray *pData)
{
    if (!pState || !pState->pDeviceInput || !pData) return false;

    qint64 nSize = pState->nInputLimit;
    if (nSize == -1) {
        const qint64 nDeviceSize = pState->pDeviceInput->size();
        if ((nDeviceSize < 0) || (pState->nInputOffset < 0) || (pState->nInputOffset > nDeviceSize)) {
            pState->bReadError = true;
            return false;
        }
        nSize = nDeviceSize - pState->nInputOffset;
    }
    if (!decIsValidBufferSize(nSize)) {
        pState->bReadError = true;
        return false;
    }

    pData->resize((qint32)nSize);
    qint64 nReadTotal = 0;
    while (nReadTotal < nSize) {
        const qint64 nRead = pState->pDeviceInput->read(pData->data() + nReadTotal, nSize - nReadTotal);
        if ((nRead <= 0) || (nRead > (nSize - nReadTotal))) {
            pState->bReadError = true;
            pData->clear();
            return false;
        }
        nReadTotal += nRead;
    }

    pState->nCountInput = nReadTotal;
    return true;
}

// Decoders normally seek to offset zero before writing, but seeking alone does
// not remove stale bytes when the new result is empty.  Clear random-access
// devices explicitly; a sequential device is usable only if no bytes have
// already been written to it.
static bool decClearOutputDevice(QIODevice *pDevice)
{
    if (!pDevice) {
        return false;
    }

    if (pDevice->isSequential()) {
        return pDevice->pos() == 0;
    }

    if (!pDevice->seek(0)) {
        return false;
    }

    return (pDevice->size() == 0) || XBinary::resize(pDevice, 0);
}

static bool decWriteAll(QIODevice *pDevice, const char *pData, qint64 nSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDevice || (nSize < 0) || ((nSize > 0) && !pData)) {
        return false;
    }

    qint64 nWritten = 0;
    while ((nWritten < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint64 nResult = pDevice->write(pData + nWritten, nSize - nWritten);
        if ((nResult <= 0) || (nResult > (nSize - nWritten))) {
            return false;
        }
        nWritten += nResult;
    }

    return (nWritten == nSize) && XBinary::isPdStructNotCanceled(pPdStruct);
}

// Copy a complete logical result through XBinary's processed-output window.
// The source is always consumed in full so nCountOutput continues to describe
// the complete decoded stream, while only the requested slice reaches the
// caller's device.
static bool decEmitDevice(QIODevice *pSource, qint64 nOffset, qint64 nSize, XBinary::DATAPROCESS_STATE *pState,
                          XBinary::PDSTRUCT *pPdStruct)
{
    if (!pSource || !pState || (nOffset < 0) || (nSize < 0) || !pState->pDeviceOutput ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (pState) pState->bWriteError = true;
        return false;
    }

    pState->bReadError = false;
    pState->bWriteError = false;
    pState->nCountOutput = 0;

    if (!decClearOutputDevice(pState->pDeviceOutput)) {
        pState->bWriteError = true;
        return false;
    }
    if (!pSource->seek(nOffset)) {
        pState->bReadError = true;
        return false;
    }

    char buffer[0x10000];
    qint64 nReadTotal = 0;
    while ((nReadTotal < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nRequest = (qint32)(std::min)(nSize - nReadTotal, (qint64)sizeof(buffer));
        const qint64 nRead = pSource->read(buffer, nRequest);
        if ((nRead <= 0) || (nRead > nRequest)) {
            pState->bReadError = true;
            break;
        }
        if (XBinary::_writeDevice(buffer, (qint32)nRead, pState) != (qint32)nRead) {
            break;
        }
        nReadTotal += nRead;
    }

    return (nReadTotal == nSize) && (pState->nCountOutput == nSize) && !pState->bReadError && !pState->bWriteError &&
           XBinary::isPdStructNotCanceled(pPdStruct);
}

class DecInputStateGuard {
public:
    explicit DecInputStateGuard(XBinary::DATAPROCESS_STATE *pState)
        : m_pState(pState), m_pDevice(pState ? pState->pDeviceInput : nullptr), m_nOffset(pState ? pState->nInputOffset : 0),
          m_nLimit(pState ? pState->nInputLimit : 0)
    {
    }

    ~DecInputStateGuard()
    {
        if (m_pState) {
            m_pState->pDeviceInput = m_pDevice;
            m_pState->nInputOffset = m_nOffset;
            m_pState->nInputLimit = m_nLimit;
        }
    }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    QIODevice *m_pDevice;
    qint64 m_nOffset;
    qint64 m_nLimit;
};

// Each MSZIP CFDATA block is a fresh raw-DEFLATE stream, but blocks after the
// first inherit the previous 32 KiB as their dictionary.  The bundled inflater
// has no inflateSetDictionary entry point, so feed that history through a
// non-final stored block and strip it from the decoded result.
static bool decInflateMSZIPBlock(const QByteArray &baPayload, const QByteArray &baHistory, qint32 nExpectedSize, QByteArray *pbaResult,
                                 XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult || (baPayload.size() < 2) || (baPayload.at(0) != 'C') || (baPayload.at(1) != 'K') || (nExpectedSize < 0) ||
        (nExpectedSize > 32768)) {
        return false;
    }

    qint32 nDictionarySize = qMin(32768, baHistory.size());
    QByteArray baInput;

    if (nDictionarySize > 0) {
        baInput.resize(5 + nDictionarySize);
        quint16 nLength = (quint16)nDictionarySize;
        quint16 nInverseLength = (quint16)~nLength;
        baInput[0] = 0;  // BFINAL=0, BTYPE=stored, then byte alignment.
        baInput[1] = (char)(nLength & 0xFF);
        baInput[2] = (char)((nLength >> 8) & 0xFF);
        baInput[3] = (char)(nInverseLength & 0xFF);
        baInput[4] = (char)((nInverseLength >> 8) & 0xFF);
        memcpy(baInput.data() + 5, baHistory.constData() + baHistory.size() - nDictionarySize, nDictionarySize);
    }

    baInput.append(baPayload.constData() + 2, baPayload.size() - 2);

    QBuffer inputBuffer(&baInput);
    QByteArray baDecoded;
    QBuffer outputBuffer(&baDecoded);
    if (!inputBuffer.open(QIODevice::ReadOnly) || !outputBuffer.open(QIODevice::WriteOnly)) {
        return false;
    }

    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = &inputBuffer;
    state.pDeviceOutput = &outputBuffer;
    state.nInputOffset = 0;
    state.nInputLimit = baInput.size();
    state.nProcessedOffset = 0;
    state.nProcessedLimit = (qint64)nDictionarySize + nExpectedSize;

    bool bResult = XDeflateDecoder::decompress(&state, pPdStruct);
    inputBuffer.close();
    outputBuffer.close();

    qint64 nExpectedDecodedSize = (qint64)nDictionarySize + nExpectedSize;
    if (!bResult || state.bReadError || state.bWriteError || (state.nCountInput != baInput.size()) || (state.nCountOutput != nExpectedDecodedSize) ||
        (baDecoded.size() != nExpectedDecodedSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    *pbaResult = baDecoded.mid(nDictionarySize);
    return pbaResult->size() == nExpectedSize;
}

XDecompress::~XDecompress()
{
    clearSolidCache();
}

bool XDecompress::decompressFPART(const XBinary::FPART &fPart, QIODevice *pDeviceInput, QIODevice *pDeviceOutput, XBinary::PDSTRUCT *pPdStruct)
{
    XBinary::DATAPROCESS_STATE state = {};
    state.mapProperties = fPart.mapProperties;
    state.pDeviceInput = pDeviceInput;
    state.pDeviceOutput = pDeviceOutput;
    state.nInputOffset = fPart.nFileOffset;
    state.nInputLimit = fPart.nFileSize;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;

    return multiDecompress(&state, pPdStruct);
}

bool XDecompress::decompressArchiveRecord(const XBinary::ARCHIVERECORD &archiveRecord, QIODevice *pDeviceInput, QIODevice *pDeviceOutput,
                                          const QMap<XBinary::UNPACK_PROP, QVariant> &mapUnpackProperties, XBinary::PDSTRUCT *pPdStruct)
{
    XBinary::DATAPROCESS_STATE state = {};
    state.mapProperties = archiveRecord.mapProperties;
    state.mapUnpackProperties = mapUnpackProperties;
    state.pDeviceInput = pDeviceInput;
    state.pDeviceOutput = pDeviceOutput;
    state.nInputOffset = archiveRecord.nStreamOffset;
    state.nInputLimit = archiveRecord.nStreamSize;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;

    return multiDecompress(&state, pPdStruct);
}

void XDecompress::clearSolidCache()
{
    QList<QString> listKeys = m_mapSolidCache.keys();
    for (qint32 i = 0; i < listKeys.count(); i++) {
        QIODevice *pDevice = m_mapSolidCache.value(listKeys.at(i));
        XBinary::freeFileBuffer(&pDevice);
    }
    m_mapSolidCache.clear();

    delete m_pRarUnpacker;
    m_pRarUnpacker = nullptr;
    m_nRarSolidIndex = 0;
    m_pCurrentSolidDevice = nullptr;
}

bool XDecompress::decompressRarSolid(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (!pState || !pState->pDeviceInput || !pState->pDeviceOutput) {
        return false;
    }

    qint64 nSolidFolderIndex = pState->mapProperties.value(XBinary::FPART_PROP_SOLIDFOLDERINDEX, (qint64)-1).toLongLong();
    // Names are not record identities: a valid archive may contain duplicate
    // names.  Include the solid folder and exact packed-stream region so a
    // duplicate name cannot return another record's cached bytes or skip the
    // decoder step required to advance solid state.
    QString sCacheKey = QString("rar_%1_%2_%3_%4")
                            .arg(QString::number((qulonglong)(quintptr)pState->pDeviceInput, 16))
                            .arg(nSolidFolderIndex)
                            .arg(pState->nInputOffset)
                            .arg(pState->nInputLimit);

    // If the requested file is not yet cached, decompress it using a persistent rar_Unpack
    // instance that maintains decoder dictionary state across sequential solid files.
    if (!m_mapSolidCache.contains(sCacheKey)) {
        bool bCacheCreated = false;
        bool bRarDecodeAttempted = false;
        qint64 nConsumedInput = 0;
        XBinary::HANDLE_METHOD compressMethod =
            (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_STORE).toUInt();
        qint64 nWindowSize = pState->mapProperties.value(XBinary::FPART_PROP_WINDOWSIZE, 0).toLongLong();
        qint64 nUncompressedSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();

        // For encrypted RAR5: decrypt first, then use the inner compression method
        QIODevice *pDecryptedDevice = nullptr;
        QIODevice *pInputDevice = pState->pDeviceInput;
        qint64 nInputOffset = pState->nInputOffset;
        qint64 nInputLimit = pState->nInputLimit;
        bool bInputReady = true;

        XBinary::HANDLE_METHOD outerMethod =
            (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD2, XBinary::HANDLE_METHOD_UNKNOWN).toUInt();
        if (outerMethod == XBinary::HANDLE_METHOD_RAR5_AES) {
            bInputReady = false;
            // Decrypt the encrypted data into a temporary buffer
            QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
            qint64 nEncryptedSize = pState->nInputLimit;

            // Align to AES block size
            if (nEncryptedSize > 0 && (nEncryptedSize % AES_BLOCK_SIZE) == 0) {
                pDecryptedDevice = XBinary::createFileBuffer(nEncryptedSize, pPdStruct);
                if (pDecryptedDevice) {
                    // Seek to the encrypted data offset before reading
                    pState->pDeviceInput->seek(pState->nInputOffset);

                    XBinary::DATAPROCESS_STATE decryptState = *pState;
                    decryptState.pDeviceOutput = pDecryptedDevice;
                    decryptState.nCountInput = 0;
                    decryptState.nCountOutput = 0;
                    decryptState.nProcessedOffset = 0;
                    decryptState.nProcessedLimit = -1;

                    if (XAESDecoder::decryptRar5(&decryptState, sPassword, pPdStruct)) {
                        pInputDevice = pDecryptedDevice;
                        nInputOffset = 0;
                        nInputLimit = decryptState.nCountOutput;
                        bInputReady = true;
                    } else {
                        XBinary::freeFileBuffer(&pDecryptedDevice);
                        pDecryptedDevice = nullptr;
                    }
                }
            }
        }

        // For solid archives: first file is not solid (bIsSolid=false), subsequent files are solid (bIsSolid=true)
        bool bIsSolid = (m_nRarSolidIndex > 0);

        if (bInputReady && (nUncompressedSize >= 0)) {
            QIODevice *pBuffer = XBinary::createFileBuffer(nUncompressedSize, pPdStruct);

            if (pBuffer) {
                bool bDecompressOk = false;

                if (compressMethod == XBinary::HANDLE_METHOD_STORE) {
                    // STORE: copy data directly, decoder state is unaffected
                    qint64 nStoreSize = qMin(qMax((qint64)0, nInputLimit), nUncompressedSize);
                    bDecompressOk = (nStoreSize == nUncompressedSize) &&
                                    XBinary::copyDeviceMemory(pInputDevice, nInputOffset, pBuffer, 0, nStoreSize, pPdStruct) &&
                                    XBinary::isPdStructNotCanceled(pPdStruct);
                    if (bDecompressOk) nConsumedInput = nStoreSize;
                } else if ((compressMethod == XBinary::HANDLE_METHOD_RAR_15) || (compressMethod == XBinary::HANDLE_METHOD_RAR_20) ||
                           (compressMethod == XBinary::HANDLE_METHOD_RAR_29) || (compressMethod == XBinary::HANDLE_METHOD_RAR_50) ||
                           (compressMethod == XBinary::HANDLE_METHOD_RAR_70)) {
                    if (!m_pRarUnpacker) {
                        m_pRarUnpacker = new rar_Unpack();
                    }

                    qint64 nEffectiveInputLimit = 0;
                    if (m_pRarUnpacker && (nWindowSize >= 0) &&
                        decPrepareBoundedInput(pInputDevice, nInputOffset, nInputLimit, &nEffectiveInputLimit) && decClearOutputDevice(pBuffer)) {
                        DecBoundedReadDevice inputDevice(pInputDevice, nEffectiveInputLimit);
                        XBinary::DATAPROCESS_STATE cacheOutputState = {};
                        cacheOutputState.pDeviceOutput = pBuffer;
                        cacheOutputState.nProcessedOffset = 0;
                        cacheOutputState.nProcessedLimit = -1;
                        DecWindowWriteDevice outputDevice(&cacheOutputState);

                        if (inputDevice.open(QIODevice::ReadOnly) && outputDevice.open(QIODevice::WriteOnly)) {
                            bRarDecodeAttempted = true;
                            m_pRarUnpacker->setDevices(&inputDevice, &outputDevice);
                            qint32 nInit = m_pRarUnpacker->Init(nWindowSize, bIsSolid);

                            if (nInit > 0) {
                                m_pRarUnpacker->SetDestSize(nUncompressedSize);

                                if (compressMethod == XBinary::HANDLE_METHOD_RAR_15) {
                                    m_pRarUnpacker->Unpack15(bIsSolid, pPdStruct);
                                } else if (compressMethod == XBinary::HANDLE_METHOD_RAR_20) {
                                    m_pRarUnpacker->Unpack20(bIsSolid, pPdStruct);
                                } else if (compressMethod == XBinary::HANDLE_METHOD_RAR_29) {
                                    m_pRarUnpacker->Unpack29(bIsSolid, pPdStruct);
                                } else if ((compressMethod == XBinary::HANDLE_METHOD_RAR_50) || (compressMethod == XBinary::HANDLE_METHOD_RAR_70)) {
                                    m_pRarUnpacker->Unpack5(bIsSolid, pPdStruct);
                                }

                                bDecompressOk = m_pRarUnpacker->IsFileExtracted() && XBinary::isPdStructNotCanceled(pPdStruct) &&
                                                !inputDevice.hasError() && !outputDevice.hasError() && !cacheOutputState.bWriteError &&
                                                (cacheOutputState.nCountOutput == nUncompressedSize);
                                if (bDecompressOk) nConsumedInput = inputDevice.consumed();
                            }

                            outputDevice.close();
                            inputDevice.close();
                        } else {
                            outputDevice.close();
                            inputDevice.close();
                        }
                    }
                }

                if (bDecompressOk) {
                    pBuffer->setProperty("RAR_INPUT_CONSUMED", nConsumedInput);
                    m_mapSolidCache.insert(sCacheKey, pBuffer);
                    bCacheCreated = true;
                } else {
                    XBinary::freeFileBuffer(&pBuffer);
                }
            }
        }

        // Clean up decrypted device if we created one
        XBinary::freeFileBuffer(&pDecryptedDevice);

        if (!bCacheCreated) {
            if (bRarDecodeAttempted) {
                // A failed solid decode can leave the persistent dictionary in
                // an indeterminate state.  Drop it and its dependent cache.
                clearSolidCache();
            }
            return false;
        }

        m_nRarSolidIndex++;
    }

    // Retrieve the requested file from cache
    if (m_mapSolidCache.contains(sCacheKey)) {
        QIODevice *pCachedDevice = m_mapSolidCache.value(sCacheKey);
        qint64 nDecompressedSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0).toLongLong();

        pState->bReadError = false;
        pState->bWriteError = false;
        pState->nCountInput = 0;
        pState->nCountOutput = 0;

        const qint64 nMax = (std::numeric_limits<qint64>::max)();
        if (!pCachedDevice || (nDecompressedSize < 0) || (pCachedDevice->size() != nDecompressedSize) || (pState->nProcessedOffset < 0) ||
            (pState->nProcessedLimit < -1) ||
            ((pState->nProcessedLimit != -1) && (pState->nProcessedOffset > (nMax - pState->nProcessedLimit)))) {
            return false;
        }

        // Validate the full cached file before applying an output window; a CRC
        // over only the requested slice would reject a valid record.
        XBinary::CRC_TYPE crcType =
            (XBinary::CRC_TYPE)pState->mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();
        if (XBinary::isUnpackCRCEnabled(pState->mapUnpackProperties, crcType)) {
            QVariant varCRC = pState->mapProperties.value(XBinary::FPART_PROP_RESULTCRC, 0);
            if (!checkCRC(crcType, varCRC, pCachedDevice, pPdStruct)) {
                return false;
            }
        }

        if (!decClearOutputDevice(pState->pDeviceOutput) || !pCachedDevice->seek(0)) {
            pState->bWriteError = true;
            return false;
        }

        char buffer[0x10000];
        qint64 nReadTotal = 0;
        while ((nReadTotal < nDecompressedSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            const qint32 nRequest = (qint32)(std::min)(nDecompressedSize - nReadTotal, (qint64)sizeof(buffer));
            const qint64 nRead = pCachedDevice->read(buffer, nRequest);
            if ((nRead <= 0) || (nRead > nRequest)) {
                pState->bReadError = true;
                break;
            }
            if (XBinary::_writeDevice(buffer, (qint32)nRead, pState) != (qint32)nRead) {
                break;
            }
            nReadTotal += nRead;
        }

        bResult = (nReadTotal == nDecompressedSize) && (pState->nCountOutput == nDecompressedSize) &&
                  XBinary::isPdStructNotCanceled(pPdStruct) && !pState->bReadError && !pState->bWriteError;
        if (bResult) {
            pState->nCountInput = pCachedDevice->property("RAR_INPUT_CONSUMED").toLongLong();
        }
    }

    return bResult;
}

bool XDecompress::checkCRC(XBinary::CRC_TYPE crcType, QVariant value, QIODevice *pDevice, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (crcType != XBinary::CRC_TYPE_UNKNOWN) {
        if (!pDevice || !pDevice->isReadable()) {
            XBinary::setPdStructErrorString(pPdStruct, tr("CRC check requires a readable output device"));
            emit warningMessage(tr("CRC check requires a readable output device"));
            return false;
        }

        if (!pDevice->seek(0)) {
            XBinary::setPdStructErrorString(pPdStruct, tr("Cannot seek output for CRC check"));
            emit warningMessage(tr("Cannot seek output for CRC check"));
            return false;
        }

        bResult = XBinary::checkCRC(pDevice, crcType, value, pPdStruct);
        pDevice->seek(0);

        if (!bResult) {
            XBinary::setPdStructErrorString(pPdStruct, tr("Invalid CRC"));
            emit warningMessage(QString("%1").arg(tr("Invalid CRC")));
        }
    }

    return bResult;
}

bool XDecompress::multiDecompress(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (!pState) {
        return false;
    }

    pState->bReadError = false;
    pState->bWriteError = false;
    pState->nCountInput = 0;
    pState->nCountOutput = 0;

    // Extraction has exact-replacement semantics.  Clearing up front also
    // guarantees that cancellation, CRC failure, or an unsupported method
    // cannot leave bytes from an earlier use of the destination behind.
    if (pState->pDeviceOutput && !decClearOutputDevice(pState->pDeviceOutput)) {
        pState->bWriteError = true;
        return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QString sArchiveMD5 = pState->mapProperties.value(XBinary::FPART_PROP_FILEMD5).toString();
    if (!sArchiveMD5.isEmpty() && sArchiveMD5 != m_sCurrentArchiveMD5) {
        clearSolidCache();
        m_sCurrentArchiveMD5 = sArchiveMD5;
    }

    bool bIsSolid = pState->mapProperties.value(XBinary::FPART_PROP_ISSOLID, false).toBool();

    if (bIsSolid && (m_pCurrentSolidDevice != pState->pDeviceInput)) {
        clearSolidCache();
        m_pCurrentSolidDevice = pState->pDeviceInput;
    }

    qint32 nNumberOfMethods = 1;

    XBinary::HANDLE_METHOD topMethod = (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_STORE).toUInt();
    // BCJ2 handles its own 4 sub-streams internally in decompress() — never treat it as multi-method
    if (topMethod != XBinary::HANDLE_METHOD_BCJ2) {
        if (pState->mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD3)) {
            nNumberOfMethods = 3;
        } else if (pState->mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD2)) {
            nNumberOfMethods = 2;
        }
    }

    if ((nNumberOfMethods == 1) && (!bIsSolid)) {
        const XBinary::CRC_TYPE crcType =
            (XBinary::CRC_TYPE)pState->mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();
        const bool bCheckCRC = (crcType != XBinary::CRC_TYPE_UNKNOWN) &&
                               pState->mapProperties.contains(XBinary::FPART_PROP_RESULTCRC) &&
                               XBinary::isUnpackCRCEnabled(pState->mapUnpackProperties, crcType);
        const bool bWindowed = (pState->nProcessedOffset != 0) || (pState->nProcessedLimit != -1);

        if (bCheckCRC && bWindowed) {
            // A record CRC covers the complete decoded record, never a caller's
            // output slice.  Decode and authenticate the full record first,
            // then apply the requested processed-output window.
            const qint64 nExpectedSize =
                qMax<qint64>(0, pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0).toLongLong());
            QIODevice *pFullDevice = XBinary::createFileBuffer(nExpectedSize, pPdStruct);
            if (pFullDevice) {
                XBinary::DATAPROCESS_STATE fullState = *pState;
                fullState.pDeviceOutput = pFullDevice;
                fullState.nProcessedOffset = 0;
                fullState.nProcessedLimit = -1;
                fullState.bReadError = false;
                fullState.bWriteError = false;
                fullState.nCountInput = 0;
                fullState.nCountOutput = 0;

                bResult = decompress(&fullState, pPdStruct);
                pState->nCountInput = fullState.nCountInput;
                pState->bReadError = fullState.bReadError;
                pState->bWriteError = fullState.bWriteError;

                const qint64 nFullSize = fullState.nCountOutput;
                bResult = bResult && (nFullSize >= 0) && (pFullDevice->size() == nFullSize);
                if (bResult) {
                    const QVariant varCRC = pState->mapProperties.value(XBinary::FPART_PROP_RESULTCRC, 0);
                    bResult = checkCRC(crcType, varCRC, pFullDevice, pPdStruct);
                }
                if (bResult) {
                    bResult = decEmitDevice(pFullDevice, 0, nFullSize, pState, pPdStruct);
                }
                XBinary::freeFileBuffer(&pFullDevice);
            }
        } else {
            // Full-output or unchecked single-method extraction can stream
            // directly to the caller.
            bResult = decompress(pState, pPdStruct);
            if (bResult && bCheckCRC && pState->pDeviceOutput) {
                const QVariant varCRC = pState->mapProperties.value(XBinary::FPART_PROP_RESULTCRC, 0);
                bResult = checkCRC(crcType, varCRC, pState->pDeviceOutput, pPdStruct);
            }
        }
    } else if (bIsSolid) {
        // Check if this is a RAR solid archive — RAR solid requires sequential decompression
        // with persistent decoder state, unlike 7z solid which uses a single compressed block.
        bool bIsRarSolid = (topMethod == XBinary::HANDLE_METHOD_RAR_15) || (topMethod == XBinary::HANDLE_METHOD_RAR_20) || (topMethod == XBinary::HANDLE_METHOD_RAR_29) ||
                           (topMethod == XBinary::HANDLE_METHOD_RAR_50) || (topMethod == XBinary::HANDLE_METHOD_RAR_70);

        // STORE files inside a RAR solid archive must also use decompressRarSolid() to keep
        // the solid index counter in sync. RAR records have SOLIDFOLDERINDEX but no
        // SUBSTREAMOFFSET (unlike 7z/CAB), which distinguishes them.
        if (!bIsRarSolid && (topMethod == XBinary::HANDLE_METHOD_STORE) && pState->mapProperties.contains(XBinary::FPART_PROP_SOLIDFOLDERINDEX) &&
            !pState->mapProperties.contains(XBinary::FPART_PROP_SUBSTREAMOFFSET)) {
            bIsRarSolid = true;
        }

        if (bIsRarSolid) {
            // RAR solid: use XRar streaming API to decompress all files with proper decoder state,
            // cache each file's output, and return the requested file from cache.
            bResult = decompressRarSolid(pState, pPdStruct);
        } else {
            // Non-RAR solid (e.g., 7z): decompress the entire folder block once, cache it,
            // then extract this file's sub-stream.
            // Prefer the explicit solid-folder ID (set by archive parsers such as XSevenZip
            // via FPART_PROP_SOLIDFOLDERINDEX); fall back to offset_size when absent.
            QString sCacheKey;
            qint64 nSolidFolderIndex = pState->mapProperties.value(XBinary::FPART_PROP_SOLIDFOLDERINDEX, (qint64)-1).toLongLong();
            const QString sDeviceKey = QString::number((qulonglong)(quintptr)pState->pDeviceInput, 16);
            if (nSolidFolderIndex >= 0) {
                sCacheKey = QString("%1_f%2_%3_%4").arg(sDeviceKey).arg(nSolidFolderIndex).arg(pState->nInputOffset).arg(pState->nInputLimit);
            } else {
                sCacheKey = QString("%1_%2_%3").arg(sDeviceKey).arg(pState->nInputOffset).arg(pState->nInputLimit);
            }

            if (!m_mapSolidCache.contains(sCacheKey)) {
                qint64 nStreamUnpackedSize = pState->mapProperties.value(XBinary::FPART_PROP_STREAMUNPACKEDSIZE, (qint64)0).toLongLong();

                // Build a block-level state: same source, ISSOLID=false, full block uncompressed size.
                // The recursive call goes to single-method or multi-method non-solid branch.
                XBinary::DATAPROCESS_STATE blockState = *pState;
                blockState.mapProperties.insert(XBinary::FPART_PROP_ISSOLID, false);
                blockState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nStreamUnpackedSize);
                blockState.nProcessedOffset = 0;
                blockState.nProcessedLimit = -1;
                blockState.bReadError = false;
                blockState.bWriteError = false;
                blockState.nCountInput = 0;
                blockState.nCountOutput = 0;
                // A solid folder CRC covers the complete decompressed block. If
                // it is unavailable, remove the per-file CRC before decoding the block.
                if (blockState.mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDCRC)) {
                    blockState.mapProperties.insert(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
                    blockState.mapProperties.insert(XBinary::FPART_PROP_RESULTCRC, blockState.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDCRC));
                } else {
                    blockState.mapProperties.remove(XBinary::FPART_PROP_CRC_TYPE);
                    blockState.mapProperties.remove(XBinary::FPART_PROP_RESULTCRC);
                }

                QIODevice *pSolidDevice =
                    (nStreamUnpackedSize >= 0) ? XBinary::createFileBuffer(nStreamUnpackedSize, pPdStruct) : nullptr;
                blockState.pDeviceOutput = pSolidDevice;

                bool bBlockResult = pSolidDevice && (nStreamUnpackedSize >= 0) && multiDecompress(&blockState, pPdStruct);
                if (pSolidDevice && bBlockResult && (blockState.nCountOutput == nStreamUnpackedSize) &&
                    (pSolidDevice->size() == nStreamUnpackedSize)) {
                    pSolidDevice->setProperty("SOLID_INPUT_CONSUMED", blockState.nCountInput);
                    m_mapSolidCache.insert(sCacheKey, pSolidDevice);
                } else {
                    XBinary::freeFileBuffer(&pSolidDevice);
                }
            }

            if (m_mapSolidCache.contains(sCacheKey)) {
                const qint64 nSubstreamOffset = pState->mapProperties.value(XBinary::FPART_PROP_SUBSTREAMOFFSET, (qint64)0).toLongLong();
                const qint64 nDecompressedSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0).toLongLong();
                QIODevice *pSolidDevice = m_mapSolidCache.value(sCacheKey);
                if (pState->pDeviceOutput && pSolidDevice && (nSubstreamOffset >= 0) && (nDecompressedSize >= 0) &&
                    (nSubstreamOffset <= pSolidDevice->size()) && (nDecompressedSize <= (pSolidDevice->size() - nSubstreamOffset))) {
                    bResult = true;

                    // Authenticate the complete logical file before applying a
                    // partial output window.
                    const XBinary::CRC_TYPE crcType =
                        (XBinary::CRC_TYPE)pState->mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();
                    if (XBinary::isUnpackCRCEnabled(pState->mapUnpackProperties, crcType)) {
                        SubDevice crcDevice(pSolidDevice, nSubstreamOffset, nDecompressedSize);
                        if (!crcDevice.open(QIODevice::ReadOnly)) {
                            bResult = false;
                        } else {
                            const QVariant varCRC = pState->mapProperties.value(XBinary::FPART_PROP_RESULTCRC, 0);
                            bResult = checkCRC(crcType, varCRC, &crcDevice, pPdStruct);
                            crcDevice.close();
                        }
                    }

                    if (bResult) {
                        pState->nCountInput = pSolidDevice->property("SOLID_INPUT_CONSUMED").toLongLong();
                        bResult = decEmitDevice(pSolidDevice, nSubstreamOffset, nDecompressedSize, pState, pPdStruct);
                    }
                }
            }
        }
    } else {
        // Multi-method, non-solid: every layer is decoded in full into its own
        // temporary device.  Processed-output windows belong only to the final
        // logical record; inheriting them in an intermediate layer discards
        // bytes required by the next filter.
        QIODevice *pIntermediateDevice = nullptr;
        qint64 nIntermediateSize = 0;
        qint64 nSourceCount = 0;

        for (qint32 i = nNumberOfMethods - 1; i >= 0; i--) {
            XBinary::DATAPROCESS_STATE state = *pState;

            XBinary::FPART_PROP fpHandleMethod = XBinary::FPART_PROP_HANDLEMETHOD;
            XBinary::FPART_PROP fpCompressProperties = XBinary::FPART_PROP_COMPRESSPROPERTIES;
            XBinary::FPART_PROP fpCompressedSize = XBinary::FPART_PROP_COMPRESSEDSIZE;
            XBinary::FPART_PROP fpUncompressedSize = XBinary::FPART_PROP_UNCOMPRESSEDSIZE;

            if (i == 2) {
                if (pState->mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD3)) fpHandleMethod = XBinary::FPART_PROP_HANDLEMETHOD3;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_COMPRESSPROPERTIES3)) fpCompressProperties = XBinary::FPART_PROP_COMPRESSPROPERTIES3;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_COMPRESSEDSIZE3)) fpCompressedSize = XBinary::FPART_PROP_COMPRESSEDSIZE3;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE3)) fpUncompressedSize = XBinary::FPART_PROP_UNCOMPRESSEDSIZE3;
            } else if (i == 1) {
                if (pState->mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD2)) fpHandleMethod = XBinary::FPART_PROP_HANDLEMETHOD2;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_COMPRESSPROPERTIES2)) fpCompressProperties = XBinary::FPART_PROP_COMPRESSPROPERTIES2;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_COMPRESSEDSIZE2)) fpCompressedSize = XBinary::FPART_PROP_COMPRESSEDSIZE2;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE2)) fpUncompressedSize = XBinary::FPART_PROP_UNCOMPRESSEDSIZE2;
            }

            state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, pState->mapProperties.value(fpHandleMethod));
            state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSPROPERTIES, pState->mapProperties.value(fpCompressProperties));
            state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, pState->mapProperties.value(fpCompressedSize));
            state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pState->mapProperties.value(fpUncompressedSize));
            state.nProcessedOffset = 0;
            state.nProcessedLimit = -1;
            state.bReadError = false;
            state.bWriteError = false;
            state.nCountInput = 0;
            state.nCountOutput = 0;

            if (i == nNumberOfMethods - 1) {
                state.pDeviceInput = pState->pDeviceInput;
            } else {
                state.pDeviceInput = pIntermediateDevice;
                state.nInputOffset = 0;
                state.nInputLimit = nIntermediateSize;
            }

            const qint64 nExpectedSize =
                qMax<qint64>(0, state.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0).toLongLong());
            QIODevice *pStageOutput = XBinary::createFileBuffer(nExpectedSize, pPdStruct);
            state.pDeviceOutput = pStageOutput;

            bResult = pStageOutput && decompress(&state, pPdStruct);
            nIntermediateSize = state.nCountOutput;
            if (i == nNumberOfMethods - 1) nSourceCount = state.nCountInput;
            pState->bReadError = pState->bReadError || state.bReadError;
            pState->bWriteError = pState->bWriteError || state.bWriteError;

            if (pIntermediateDevice) XBinary::freeFileBuffer(&pIntermediateDevice);

            if (!bResult || (nIntermediateSize < 0) || !pStageOutput || (pStageOutput->size() != nIntermediateSize)) {
                XBinary::freeFileBuffer(&pStageOutput);
                bResult = false;
                break;
            }

            pIntermediateDevice = pStageOutput;
        }

        if (bResult && pIntermediateDevice) {
            const XBinary::CRC_TYPE crcType =
                (XBinary::CRC_TYPE)pState->mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();
            if (XBinary::isUnpackCRCEnabled(pState->mapUnpackProperties, crcType)) {
                const QVariant varCRC = pState->mapProperties.value(XBinary::FPART_PROP_RESULTCRC, 0);
                bResult = checkCRC(crcType, varCRC, pIntermediateDevice, pPdStruct);
            }
            pState->nCountInput = nSourceCount;
            if (bResult) bResult = decEmitDevice(pIntermediateDevice, 0, nIntermediateSize, pState, pPdStruct);
        }

        if (pIntermediateDevice) XBinary::freeFileBuffer(&pIntermediateDevice);
    }

    if (!bResult && pState->pDeviceOutput && !pState->pDeviceOutput->isSequential()) {
        decClearOutputDevice(pState->pDeviceOutput);
    }

    return bResult;
}

bool XDecompress::decompress(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pState) {
        return false;
    }

    DecInputStateGuard inputStateGuard(pState);
    bool bResult = false;
    pState->bReadError = false;
    pState->bWriteError = false;
    pState->nCountInput = 0;
    pState->nCountOutput = 0;

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    if (pState->pDeviceInput) {
        if (!pState->pDeviceInput->seek(pState->nInputOffset) && (pState->pDeviceInput->pos() != pState->nInputOffset)) {
            pState->bReadError = true;
            return false;
        }
    }

    if (pState->pDeviceOutput && !decClearOutputDevice(pState->pDeviceOutput)) {
        pState->bWriteError = true;
        return false;
    }

    XBinary::HANDLE_METHOD compressMethod = (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_STORE).toUInt();
    QByteArray baProperty = pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES).toByteArray();
    bool bUncompressedSizeDefined = pState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE);
    qint64 nUncompressedSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();
    qint64 nWindowSize = pState->mapProperties.value(XBinary::FPART_PROP_WINDOWSIZE, 0).toLongLong();

    // ARJ GARBLE pre-decryption: if PASSWORD_MODIFIER is present, XOR the compressed stream
    // with (modifier + password[i % len]) mod 256 before decompressing.
    QByteArray baArjGarbleDecrypted;
    QBuffer arjGarbleBuf;
    if (pState->mapProperties.contains(XBinary::FPART_PROP_PASSWORD_MODIFIER)) {
        quint8 nModifier = (quint8)pState->mapProperties.value(XBinary::FPART_PROP_PASSWORD_MODIFIER).toUInt();
        QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
        if (!sPassword.isEmpty() && pState->pDeviceInput) {
            if ((pState->nInputLimit <= 0) || (pState->nInputLimit > (std::numeric_limits<qint32>::max)()) || !pState->pDeviceInput->seek(pState->nInputOffset)) {
                return false;
            }
            baArjGarbleDecrypted.resize((qint32)pState->nInputLimit);
            qint32 nRead = pState->pDeviceInput->read(baArjGarbleDecrypted.data(), (qint32)pState->nInputLimit);
            if (nRead == (qint32)pState->nInputLimit) {
                const qint32 nPwdLen = sPassword.length();
                for (qint32 i = 0; i < nRead; i++) {
                    quint8 k = (quint8)(nModifier + (quint8)sPassword[i % nPwdLen].toLatin1());
                    baArjGarbleDecrypted.data()[i] = (char)((quint8)baArjGarbleDecrypted[i] ^ k);
                }
                arjGarbleBuf.setBuffer(&baArjGarbleDecrypted);
                if (!arjGarbleBuf.open(QIODevice::ReadOnly)) {
                    return false;
                }
                pState->pDeviceInput = &arjGarbleBuf;
                pState->nInputOffset = 0;
            }
        }
    }

    if (compressMethod == XBinary::HANDLE_METHOD_STORE) {
        // For STORE after AES decryption, the input may include AES padding bytes.
        // Cap input to the actual uncompressed size (including zero) to avoid
        // copying padding or stale data into an empty result.
        if (bUncompressedSizeDefined && (nUncompressedSize >= 0) && (nUncompressedSize < pState->nInputLimit)) {
            pState->nInputLimit = nUncompressedSize;
        }
        bResult = XStoreDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_BZIP2) {
        bResult = XBZIP2Decoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_BROTLI) {
        bResult = XBrotliDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZMA) {
        if (!baProperty.isEmpty()) {
            bResult = XLZMADecoder::decompress(pState, baProperty, pPdStruct);
        } else {
            bResult = XLZMADecoder::decompress(pState, pPdStruct);
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZMA2) {
        if (!baProperty.isEmpty()) {
            bResult = XLZMADecoder::decompressLZMA2(pState, baProperty, pPdStruct);
        } else {
            bResult = XLZMADecoder::decompressLZMA2(pState, pPdStruct);
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_BCJ) {
        // x86 BCJ inverse filter — delegate to the single byte-exact reference port.
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            qint64 nFilterSize = pState->nInputLimit;
            if (nFilterSize == -1) nFilterSize = pState->pDeviceInput->size() - pState->nInputOffset;
            if (!decIsValidBufferSize(nFilterSize)) {
                pState->bReadError = true;
                return false;
            }
            QByteArray baData = pState->pDeviceInput->read(nFilterSize);
            pState->nCountInput = baData.size();
            if (baData.size() != nFilterSize) {
                pState->bReadError = true;
                return false;
            }

            // Optional 4-byte LE start-offset property (ip); absent/0 for standard 7z.
            quint32 nIp = 0;
            if (baProperty.size() >= 4) {
                nIp = (quint32)(quint8)baProperty.at(0) | ((quint32)(quint8)baProperty.at(1) << 8) | ((quint32)(quint8)baProperty.at(2) << 16) |
                      ((quint32)(quint8)baProperty.at(3) << 24);
            }

            Algo_utils::applyBCJX86Decode(baData, nIp);

            bResult = XBinary::_writeDevice(baData.constData(), baData.size(), pState) == baData.size();
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_ARM64_BCJ) {
        bResult = XBranchDecoder::decompressBranch(pState, XBranchDecoder::BTYPE_ARM64, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ARM_BCJ) {
        bResult = XBranchDecoder::decompressBranch(pState, XBranchDecoder::BTYPE_ARM, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ARMT_BCJ) {
        bResult = XBranchDecoder::decompressBranch(pState, XBranchDecoder::BTYPE_ARMT, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_PPC_BCJ) {
        bResult = XBranchDecoder::decompressBranch(pState, XBranchDecoder::BTYPE_PPC, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_SPARC_BCJ) {
        bResult = XBranchDecoder::decompressBranch(pState, XBranchDecoder::BTYPE_SPARC, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IA64_BCJ) {
        bResult = XBranchDecoder::decompressBranch(pState, XBranchDecoder::BTYPE_IA64, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_DELTA) {
        // Property byte holds distance - 1 (7z and XZ delta filter convention)
        qint32 nDistance = baProperty.isEmpty() ? 1 : ((qint32)(quint8)baProperty.at(0) + 1);
        bResult = XBranchDecoder::decompressDelta(pState, nDistance, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_KWAJ_XOR) {
        // KWAJ compression method 1: every byte XOR 0xFF
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            qint64 nFilterSize = pState->nInputLimit;
            if (nFilterSize == -1) nFilterSize = pState->pDeviceInput->size() - pState->nInputOffset;
            if (!decIsValidBufferSize(nFilterSize)) {
                pState->bReadError = true;
                return false;
            }
            QByteArray baData = pState->pDeviceInput->read(nFilterSize);
            pState->nCountInput = baData.size();
            if (baData.size() != nFilterSize) {
                pState->bReadError = true;
                return false;
            }

            for (qint32 i = 0; i < baData.size(); i++) {
                baData[i] = (char)((quint8)baData.at(i) ^ 0xFF);
            }

            bResult = XBinary::_writeDevice(baData.constData(), baData.size(), pState) == baData.size();
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_XZ) {
        bResult = XLZMADecoder::decompressXZ(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_PPMD7) {
        bResult = XPPMdDecoder::decompressPPMD7(pState, baProperty, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_PPMD8) {
        bResult = XPPMdDecoder::decompressPPMD8(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_DEFLATE) {
        bResult = XDeflateDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_DEFLATE64) {
        bResult = XDeflateDecoder::decompress64(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IT214_8) {
        bResult = XIT214Decoder::decompress(pState, 8, false, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IT214_16) {
        bResult = XIT214Decoder::decompress(pState, 16, false, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IT215_8) {
        bResult = XIT214Decoder::decompress(pState, 8, true, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IT215_16) {
        bResult = XIT214Decoder::decompress(pState, 16, true, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IMPLODED_4KDICT_2TREES) {
        bResult = XImplodeDecoder::decompress(pState, false, false, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IMPLODED_4KDICT_3TREES) {
        bResult = XImplodeDecoder::decompress(pState, false, true, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IMPLODED_8KDICT_2TREES) {
        bResult = XImplodeDecoder::decompress(pState, true, false, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IMPLODED_8KDICT_3TREES) {
        bResult = XImplodeDecoder::decompress(pState, true, true, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_SHRINK) {
        bResult = XShrinkDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_REDUCE_1) {
        bResult = XReduceDecoder::decompress(pState, 1, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_REDUCE_2) {
        bResult = XReduceDecoder::decompress(pState, 2, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_REDUCE_3) {
        bResult = XReduceDecoder::decompress(pState, 3, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_REDUCE_4) {
        bResult = XReduceDecoder::decompress(pState, 4, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ZLIB) {
        bResult = XDeflateDecoder::decompress_zlib(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZW_PDF) {
        bResult = XLZWDecoder::decompress_pdf(pState, pPdStruct);
        // bResult = XStoreDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ASCII85) {
        bResult = XASCII85Decoder::decompress_pdf(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ASCIIHEX) {
        bResult = XASCIIHexDecoder::decompress_pdf(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_RUNLENGTH) {
        bResult = XRunLengthDecoder::decompress_pdf(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZH1) {
        bResult = XLZHDecoder::decompress(pState, 1, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZH5) {
        bResult = XLZHDecoder::decompress(pState, 5, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZH6) {
        bResult = XLZHDecoder::decompress(pState, 6, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZH7) {
        bResult = XLZHDecoder::decompress(pState, 7, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ARJ) {
        bResult = XArjDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ARJ_FASTEST) {
        bResult = XArjDecoder::decompressFastest(pState, pPdStruct);
    } else if ((compressMethod == XBinary::HANDLE_METHOD_RAR_15) || (compressMethod == XBinary::HANDLE_METHOD_RAR_20) ||
               (compressMethod == XBinary::HANDLE_METHOD_RAR_29) || (compressMethod == XBinary::HANDLE_METHOD_RAR_50) ||
               (compressMethod == XBinary::HANDLE_METHOD_RAR_70)) {
        pState->bReadError = false;
        pState->bWriteError = false;
        pState->nCountInput = 0;
        pState->nCountOutput = 0;

        const qint64 nMax = (std::numeric_limits<qint64>::max)();
        if (!pState->pDeviceInput || !pState->pDeviceOutput || !bUncompressedSizeDefined || (nUncompressedSize < 0) || (nWindowSize < 0) ||
            (pState->nProcessedOffset < 0) || (pState->nProcessedLimit < -1) ||
            ((pState->nProcessedLimit != -1) && (pState->nProcessedOffset > (nMax - pState->nProcessedLimit)))) {
            pState->bWriteError = true;
            return false;
        }

        qint64 nEffectiveInputLimit = 0;
        if (!decPrepareBoundedInput(pState->pDeviceInput, pState->nInputOffset, pState->nInputLimit, &nEffectiveInputLimit)) {
            pState->bReadError = true;
            return false;
        }
        if (!decClearOutputDevice(pState->pDeviceOutput)) {
            pState->bWriteError = true;
            return false;
        }

        DecBoundedReadDevice inputDevice(pState->pDeviceInput, nEffectiveInputLimit);
        DecWindowWriteDevice outputDevice(pState);
        if (!inputDevice.open(QIODevice::ReadOnly)) {
            pState->bReadError = true;
            return false;
        }
        if (!outputDevice.open(QIODevice::WriteOnly)) {
            inputDevice.close();
            pState->bWriteError = true;
            return false;
        }

        bool bIsSolid = false;
        rar_Unpack rarUnpack;
        rarUnpack.setDevices(&inputDevice, &outputDevice);
        qint32 nInit = rarUnpack.Init(nWindowSize, bIsSolid);

        if (nInit > 0) {
            rarUnpack.SetDestSize(nUncompressedSize);

            if (compressMethod == XBinary::HANDLE_METHOD_RAR_15) {
                rarUnpack.Unpack15(bIsSolid, pPdStruct);
            } else if (compressMethod == XBinary::HANDLE_METHOD_RAR_20) {
                rarUnpack.Unpack20(bIsSolid, pPdStruct);
            } else if (compressMethod == XBinary::HANDLE_METHOD_RAR_29) {
                rarUnpack.Unpack29(bIsSolid, pPdStruct);
            } else if ((compressMethod == XBinary::HANDLE_METHOD_RAR_50) || (compressMethod == XBinary::HANDLE_METHOD_RAR_70)) {
                rarUnpack.Unpack5(bIsSolid, pPdStruct);
            }

            bResult = rarUnpack.IsFileExtracted() && XBinary::isPdStructNotCanceled(pPdStruct) && !inputDevice.hasError() &&
                      !outputDevice.hasError() && !pState->bReadError && !pState->bWriteError && (pState->nCountOutput == nUncompressedSize);
        } else {
            bResult = false;
        }

        pState->nCountInput = inputDevice.consumed();
        if (inputDevice.hasError()) pState->bReadError = true;
        if (outputDevice.hasError()) pState->bWriteError = true;
        outputDevice.close();
        inputDevice.close();
    } else if ((compressMethod == XBinary::HANDLE_METHOD_ZIP_AES) || (compressMethod == XBinary::HANDLE_METHOD_ZIP_AES128) ||
               (compressMethod == XBinary::HANDLE_METHOD_ZIP_AES192 || (compressMethod == XBinary::HANDLE_METHOD_ZIP_AES256))) {
        QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();

        if (compressMethod == XBinary::HANDLE_METHOD_ZIP_AES) {
            compressMethod = XBinary::HANDLE_METHOD_ZIP_AES256;
        }

        bResult = XAESDecoder::decrypt(pState, sPassword, compressMethod, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ZIPCRYPTO) {
        QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
        bResult = XZipCryptoDecoder::decrypt(pState, sPassword, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_7Z_AES) {
        QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
        bResult = XAESDecoder::decrypt(pState, baProperty, sPassword, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_RAR5_AES) {
        QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
        bResult = XAESDecoder::decryptRar5(pState, sPassword, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_BCJ2) {
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            XBinary::HANDLE_METHOD cmMain =
                (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD4, (quint32)XBinary::HANDLE_METHOD_LZMA).toUInt();
            QByteArray baPropMain = pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES).toByteArray();
            qint64 nMainUnpack = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE4, (qint64)0).toLongLong();

            XBinary::HANDLE_METHOD cmCall =
                (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD2, (quint32)XBinary::HANDLE_METHOD_LZMA).toUInt();
            QByteArray baPropCall = pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES2).toByteArray();
            qint64 nCallUnpack = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE2, (qint64)0).toLongLong();
            qint64 nCallOffset = pState->mapProperties.value(XBinary::FPART_PROP_STREAMOFFSET2, (qint64)0).toLongLong();
            qint64 nCallSize = pState->mapProperties.value(XBinary::FPART_PROP_STREAMSIZE2, (qint64)0).toLongLong();

            XBinary::HANDLE_METHOD cmJmp =
                (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD3, (quint32)XBinary::HANDLE_METHOD_LZMA).toUInt();
            QByteArray baPropJmp = pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES3).toByteArray();
            qint64 nJmpUnpack = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE3, (qint64)0).toLongLong();
            qint64 nJmpOffset = pState->mapProperties.value(XBinary::FPART_PROP_STREAMOFFSET3, (qint64)0).toLongLong();
            qint64 nJmpSize = pState->mapProperties.value(XBinary::FPART_PROP_STREAMSIZE3, (qint64)0).toLongLong();

            qint64 nRangeOffset = pState->mapProperties.value(XBinary::FPART_PROP_STREAMOFFSET4, (qint64)0).toLongLong();
            qint64 nRangeSize = pState->mapProperties.value(XBinary::FPART_PROP_STREAMSIZE4, (qint64)0).toLongLong();

            qint64 nOutputSize = nUncompressedSize;  // BCJ2 total output set by multiDecompress

            // BCJ2 AES-encrypted layout: each sub-stream separately AES-encrypted before compression
            QString sBCJ2Password = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
            QByteArray aBCJ2AESProps[4];
            qint64 aBCJ2AESUnpack[4] = {0, 0, 0, 0};
            aBCJ2AESProps[0] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_PROPS_0).toByteArray();
            aBCJ2AESUnpack[0] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_UNPACK_0, (qint64)0).toLongLong();
            aBCJ2AESProps[1] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_PROPS_1).toByteArray();
            aBCJ2AESUnpack[1] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_UNPACK_1, (qint64)0).toLongLong();
            aBCJ2AESProps[2] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_PROPS_2).toByteArray();
            aBCJ2AESUnpack[2] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_UNPACK_2, (qint64)0).toLongLong();
            aBCJ2AESProps[3] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_PROPS_3).toByteArray();
            aBCJ2AESUnpack[3] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_UNPACK_3, (qint64)0).toLongLong();
            bool bBCJ2HasAES = !aBCJ2AESProps[0].isEmpty();

            // Pre-decrypt AES-encrypted BCJ2 sub-streams into temp buffers
            QByteArray aBCJ2Decrypted[4];
            bool bAESDecryptOk = true;
            if (bBCJ2HasAES) {
                qint64 aEncOffsets[3] = {pState->nInputOffset, nCallOffset, nJmpOffset};
                qint64 aEncSizes[3] = {pState->nInputLimit, nCallSize, nJmpSize};
                for (qint32 ni = 0; ni < 3 && bAESDecryptOk; ni++) {
                    if (aBCJ2AESProps[ni].isEmpty()) continue;
                    QBuffer decBuf(&aBCJ2Decrypted[ni]);
                    if (!decBuf.open(QIODevice::WriteOnly)) {
                        bAESDecryptOk = false;
                        break;
                    }
                    XBinary::DATAPROCESS_STATE aesState = {};
                    aesState.pDeviceInput = pState->pDeviceInput;
                    aesState.pDeviceOutput = &decBuf;
                    aesState.nInputOffset = aEncOffsets[ni];
                    aesState.nInputLimit = aEncSizes[ni];
                    aesState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, aBCJ2AESUnpack[ni]);
                    pState->pDeviceInput->seek(aEncOffsets[ni]);
                    bAESDecryptOk = XAESDecoder::decrypt(&aesState, aBCJ2AESProps[ni], sBCJ2Password, pPdStruct);
                    decBuf.close();
                }
                // Range stream
                if (bAESDecryptOk && !aBCJ2AESProps[3].isEmpty()) {
                    QBuffer decBuf(&aBCJ2Decrypted[3]);
                    if (decBuf.open(QIODevice::WriteOnly)) {
                        XBinary::DATAPROCESS_STATE aesState = {};
                        aesState.pDeviceInput = pState->pDeviceInput;
                        aesState.pDeviceOutput = &decBuf;
                        aesState.nInputOffset = nRangeOffset;
                        aesState.nInputLimit = nRangeSize;
                        aesState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, aBCJ2AESUnpack[3]);
                        pState->pDeviceInput->seek(nRangeOffset);
                        bAESDecryptOk = XAESDecoder::decrypt(&aesState, aBCJ2AESProps[3], sBCJ2Password, pPdStruct);
                        decBuf.close();
                    } else {
                        bAESDecryptOk = false;
                    }
                }
            }

            // nCallUnpack / nJmpUnpack may be 0 when the data contains no CALL/JMP instructions
            // (e.g. pure image or text files). Only require nMainUnpack > 0 and nOutputSize > 0.
            if (nMainUnpack > 0 && nOutputSize > 0 && bAESDecryptOk && decIsValidBufferSize(nMainUnpack) && decIsValidBufferSize(nCallUnpack) &&
                decIsValidBufferSize(nJmpUnpack)) {
                QByteArray baMain, baCall, baJmp;
                baMain.resize((qint32)nMainUnpack);
                baCall.resize((qint32)nCallUnpack);
                baJmp.resize((qint32)nJmpUnpack);

                struct _BCJ2Task {
                    qint64 nOffset;
                    qint64 nSize;
                    qint64 nOutputSize;
                    XBinary::HANDLE_METHOD cm;
                    QByteArray *pOutput;
                    const QByteArray *pProperty;
                };

                _BCJ2Task tasks[3];
                tasks[0] = {pState->nInputOffset, pState->nInputLimit, nMainUnpack, cmMain, &baMain, &baPropMain};
                tasks[1] = {nCallOffset, nCallSize, nCallUnpack, cmCall, &baCall, &baPropCall};
                tasks[2] = {nJmpOffset, nJmpSize, nJmpUnpack, cmJmp, &baJmp, &baPropJmp};

                bool bLZMAOk = true;
                for (qint32 nTask = 0; nTask < 3 && bLZMAOk && XBinary::isPdStructNotCanceled(pPdStruct); nTask++) {
                    QBuffer outBuf(tasks[nTask].pOutput);
                    if (!outBuf.open(QIODevice::WriteOnly)) {
                        bLZMAOk = false;
                        break;
                    }
                    XBinary::DATAPROCESS_STATE dpState = {};
                    dpState.pDeviceOutput = &outBuf;
                    dpState.nProcessedOffset = 0;
                    dpState.nProcessedLimit = tasks[nTask].nOutputSize;
                    dpState.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, (quint32)tasks[nTask].cm);
                    dpState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSPROPERTIES, *tasks[nTask].pProperty);
                    dpState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, tasks[nTask].nOutputSize);
                    if (bBCJ2HasAES && !aBCJ2Decrypted[nTask].isEmpty()) {
                        // Use AES-decrypted buffer as LZMA input
                        QBuffer lzmaBuf(&aBCJ2Decrypted[nTask]);
                        if (lzmaBuf.open(QIODevice::ReadOnly)) {
                            dpState.pDeviceInput = &lzmaBuf;
                            dpState.nInputOffset = 0;
                            dpState.nInputLimit = aBCJ2Decrypted[nTask].size();
                            bLZMAOk = decompress(&dpState, pPdStruct);
                            lzmaBuf.close();
                        } else {
                            bLZMAOk = false;
                        }
                    } else {
                        dpState.pDeviceInput = pState->pDeviceInput;
                        dpState.nInputOffset = tasks[nTask].nOffset;
                        dpState.nInputLimit = tasks[nTask].nSize;
                        bLZMAOk = decompress(&dpState, pPdStruct);
                    }
                    outBuf.close();
                }

                if (bLZMAOk && XBinary::isPdStructNotCanceled(pPdStruct)) {
                    // Range coder stream: raw or AES-decrypted
                    QByteArray baRange;
                    if (bBCJ2HasAES && !aBCJ2Decrypted[3].isEmpty()) {
                        baRange = aBCJ2Decrypted[3];
                    } else {
                        pState->pDeviceInput->seek(nRangeOffset);
                        baRange = pState->pDeviceInput->read(nRangeSize);
                    }
                    if (baRange.size() > 0) {
                        QBuffer mainBuf(&baMain);
                        QBuffer callBuf(&baCall);
                        QBuffer jmpBuf(&baJmp);
                        QBuffer rangeBuf(&baRange);
                        if (mainBuf.open(QIODevice::ReadOnly) && callBuf.open(QIODevice::ReadOnly) && jmpBuf.open(QIODevice::ReadOnly)) {
                            if (rangeBuf.open(QIODevice::ReadOnly)) {
                                QIODevice *pBCJ2Output = XBinary::createFileBuffer(nOutputSize, pPdStruct);
                                if (pBCJ2Output && decClearOutputDevice(pBCJ2Output)) {
                                    bResult = XBCJ2Decoder::decompress(&mainBuf, &callBuf, &jmpBuf, &rangeBuf, pBCJ2Output, nOutputSize, pPdStruct) &&
                                              (pBCJ2Output->size() == nOutputSize);
                                    if (bResult) {
                                        const qint64 nMax = (std::numeric_limits<qint64>::max)();
                                        bResult = (pState->nInputLimit >= 0) && (nCallSize >= 0) && (nJmpSize >= 0) && (nRangeSize >= 0) &&
                                                  (pState->nInputLimit <= nMax - nCallSize) &&
                                                  (pState->nInputLimit + nCallSize <= nMax - nJmpSize) &&
                                                  (pState->nInputLimit + nCallSize + nJmpSize <= nMax - nRangeSize);
                                        if (bResult) {
                                            pState->nCountInput = pState->nInputLimit + nCallSize + nJmpSize + nRangeSize;
                                            bResult = decEmitDevice(pBCJ2Output, 0, nOutputSize, pState, pPdStruct);
                                        }
                                    }
                                }
                                XBinary::freeFileBuffer(&pBCJ2Output);
                            }
                        }
                    }
                }
            } else {
                bResult = false;
            }
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_PDF_CCITTIMAGE) {
        // CCITT Fax image: wrap raw data in a TIFF container
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            QByteArray baData;
            if (!decReadInputToByteArray(pState, &baData)) return false;

            qint32 nWidth = pState->mapProperties.value(XBinary::FPART_PROP_WIDTH).toInt();
            qint32 nHeight = pState->mapProperties.value(XBinary::FPART_PROP_HEIGHT).toInt();
            qint32 nCcittK = pState->mapProperties.value(XBinary::FPART_PROP_CCITTK, -1).toInt();

            // TIFF compression type from CCITT /K parameter
            quint16 nTiffCompression = 4;  // Group 4 (default for /K < 0)
            if (nCcittK == 0) {
                nTiffCompression = 3;  // Group 3 1D
            } else if (nCcittK > 0) {
                nTiffCompression = 3;  // Group 3 mixed
            }

            const qint32 nTagCount = 9;
            const qint32 nIfdSize = 2 + nTagCount * 12 + 4;
            const qint32 nStripOffset = 8 + nIfdSize;
            const qint32 nStripSize = baData.size();

            QByteArray baTiff;
            QBuffer tiffBuffer(&baTiff);
            if (!tiffBuffer.open(QIODevice::WriteOnly)) {
                return false;
            }
            QDataStream ds(&tiffBuffer);
            ds.setByteOrder(QDataStream::LittleEndian);

            // TIFF header: "II" (little-endian), magic 42, IFD offset
            ds.writeRawData("II", 2);
            ds << (quint16)42;
            ds << (quint32)8;

            // IFD
            ds << (quint16)nTagCount;
            // Tag 256 (0x0100): ImageWidth
            ds << (quint16)0x0100 << (quint16)3 << (quint32)1 << (quint16)nWidth << (quint16)0;
            // Tag 257 (0x0101): ImageLength
            ds << (quint16)0x0101 << (quint16)3 << (quint32)1 << (quint16)nHeight << (quint16)0;
            // Tag 258 (0x0102): BitsPerSample
            ds << (quint16)0x0102 << (quint16)3 << (quint32)1 << (quint16)1 << (quint16)0;
            // Tag 259 (0x0103): Compression
            ds << (quint16)0x0103 << (quint16)3 << (quint32)1 << (quint16)nTiffCompression << (quint16)0;
            // Tag 262 (0x0106): PhotometricInterpretation (0=WhiteIsZero)
            ds << (quint16)0x0106 << (quint16)3 << (quint32)1 << (quint16)0 << (quint16)0;
            // Tag 273 (0x0111): StripOffsets
            ds << (quint16)0x0111 << (quint16)4 << (quint32)1 << (quint32)nStripOffset;
            // Tag 278 (0x0116): RowsPerStrip
            ds << (quint16)0x0116 << (quint16)3 << (quint32)1 << (quint16)nHeight << (quint16)0;
            // Tag 279 (0x0117): StripByteCounts
            ds << (quint16)0x0117 << (quint16)4 << (quint32)1 << (quint32)nStripSize;
            // Tag 296 (0x0128): ResolutionUnit (1=No absolute unit)
            ds << (quint16)0x0128 << (quint16)3 << (quint32)1 << (quint16)1 << (quint16)0;

            // Next IFD offset (0 = no more IFDs)
            ds << (quint32)0;

            // Strip data
            ds.writeRawData(baData.constData(), nStripSize);

            tiffBuffer.close();

            bResult = XBinary::_writeDevice(baTiff.constData(), baTiff.size(), pState) == baTiff.size();
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_PDF_PALETTE) {
        // Palette data: build RIFF PAL container from decompressed RGB data
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            QByteArray baData;
            if (!decReadInputToByteArray(pState, &baData)) return false;

            qint32 nRgbSize = baData.size();
            qint32 nColorCount = nRgbSize / 3;

            if ((nColorCount > 0) && (nColorCount <= 256)) {
                qint32 nDataChunkPayload = 4 + nColorCount * 4;
                qint32 nFileSize = 4 + 8 + nDataChunkPayload;

                QByteArray baPal;
                QBuffer palBuffer(&baPal);
                if (!palBuffer.open(QIODevice::WriteOnly)) {
                    return false;
                }
                QDataStream ds(&palBuffer);
                ds.setByteOrder(QDataStream::LittleEndian);

                // RIFF header
                ds.writeRawData("RIFF", 4);
                ds << (quint32)nFileSize;
                ds.writeRawData("PAL ", 4);

                // data chunk
                ds.writeRawData("data", 4);
                ds << (quint32)nDataChunkPayload;

                // PAL version and color count
                ds << (quint16)0x0300;
                ds << (quint16)nColorCount;

                // RGBX entries
                const quint8 *pRgb = (const quint8 *)baData.constData();

                for (qint32 i = 0; i < nColorCount; ++i) {
                    ds << pRgb[i * 3];      // R
                    ds << pRgb[i * 3 + 1];  // G
                    ds << pRgb[i * 3 + 2];  // B
                    ds << (quint8)0;        // Flags
                }

                palBuffer.close();

                bResult = XBinary::_writeDevice(baPal.constData(), baPal.size(), pState) == baPal.size();
            } else {
                // Fallback: write raw data
                bResult = XBinary::_writeDevice(baData.constData(), baData.size(), pState) == baData.size();
            }
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_PDF_IMAGEDATA) {
        // Raw pixel data image: convert decompressed data to PNG
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            QByteArray baData;
            if (!decReadInputToByteArray(pState, &baData)) return false;

            qint32 nWidth = pState->mapProperties.value(XBinary::FPART_PROP_WIDTH).toInt();
            qint32 nHeight = pState->mapProperties.value(XBinary::FPART_PROP_HEIGHT).toInt();
            qint32 nBitsPerComponent = pState->mapProperties.value(XBinary::FPART_PROP_BITSPERCOMPONENT).toInt();
            QString sColorSpace = pState->mapProperties.value(XBinary::FPART_PROP_COLORSPACE).toString();

            bool bConverted = false;

            if ((nWidth > 0) && (nHeight > 0) && (nBitsPerComponent > 0)) {
                XPNG::COLOR_TYPE pngColorType = XPNG::COLOR_TYPE_RGB;
                qint32 nBytesPerPixel = 0;
                bool bValidFormat = false;

                if ((sColorSpace == QLatin1String("/DeviceRGB")) || sColorSpace.isEmpty()) {
                    if (nBitsPerComponent == 8) {
                        pngColorType = XPNG::COLOR_TYPE_RGB;
                        nBytesPerPixel = 3;
                        bValidFormat = true;
                    }
                } else if (sColorSpace == QLatin1String("/DeviceGray")) {
                    if (nBitsPerComponent == 8) {
                        pngColorType = XPNG::COLOR_TYPE_GRAYSCALE;
                        nBytesPerPixel = 1;
                        bValidFormat = true;
                    } else if (nBitsPerComponent == 1) {
                        pngColorType = XPNG::COLOR_TYPE_GRAYSCALE;
                        nBytesPerPixel = 0;  // 1 bit per pixel
                        bValidFormat = true;
                    }
                } else if (sColorSpace == QLatin1String("/DeviceCMYK")) {
                    if (nBitsPerComponent == 8) {
                        nBytesPerPixel = 4;  // CMYK uses 4 bytes per pixel
                    }
                } else if (sColorSpace == QLatin1String("/Indexed")) {
                    if (nBitsPerComponent == 8) {
                        nBytesPerPixel = 1;
                    }
                }

                // Indexed colorspace with palette
                if ((sColorSpace == QLatin1String("/Indexed")) && (nBitsPerComponent == 8) && (nBytesPerPixel == 1)) {
                    QByteArray baPalette = pState->mapProperties.value(XBinary::FPART_PROP_PALETTE).toByteArray();
                    QString sBaseColorSpace = pState->mapProperties.value(XBinary::FPART_PROP_BASECOLORSPACE).toString();
                    qint64 nExpectedSize = (qint64)nWidth * nHeight;

                    if ((baData.size() >= nExpectedSize) && !baPalette.isEmpty()) {
                        qint32 nColorsPerEntry = 3;  // Default: RGB

                        if (sBaseColorSpace == QLatin1String("/DeviceGray")) {
                            nColorsPerEntry = 1;
                        } else if (sBaseColorSpace == QLatin1String("/DeviceCMYK")) {
                            nColorsPerEntry = 4;
                        }

                        // Convert palette to RGB format (3 bytes per entry) for PNG PLTE chunk
                        qint32 nPalColorCount = baPalette.size() / nColorsPerEntry;
                        QByteArray baRgbPalette;
                        baRgbPalette.reserve(nPalColorCount * 3);
                        const quint8 *pPal = (const quint8 *)baPalette.constData();

                        for (qint32 i = 0; i < nPalColorCount; i++) {
                            quint8 nR = 0;
                            quint8 nG = 0;
                            quint8 nB = 0;

                            if (nColorsPerEntry == 3) {
                                nR = pPal[i * 3];
                                nG = pPal[i * 3 + 1];
                                nB = pPal[i * 3 + 2];
                            } else if (nColorsPerEntry == 1) {
                                nR = pPal[i];
                                nG = pPal[i];
                                nB = pPal[i];
                            } else if (nColorsPerEntry == 4) {
                                quint8 nC = pPal[i * 4];
                                quint8 nM = pPal[i * 4 + 1];
                                quint8 nY = pPal[i * 4 + 2];
                                quint8 nK = pPal[i * 4 + 3];
                                nR = (quint8)(255 - qMin(255, (qint32)nC + nK));
                                nG = (quint8)(255 - qMin(255, (qint32)nM + nK));
                                nB = (quint8)(255 - qMin(255, (qint32)nY + nK));
                            }

                            baRgbPalette.append((char)nR);
                            baRgbPalette.append((char)nG);
                            baRgbPalette.append((char)nB);
                        }

                        QBuffer pngBuffer;
                        if (pngBuffer.open(QIODevice::ReadWrite)) {
                            bConverted = XPNG::createPNGIndexed(&pngBuffer, nWidth, nHeight, baData.left(nExpectedSize), baRgbPalette);
                            pngBuffer.close();
                        }

                        if (bConverted) {
                            bResult = XBinary::_writeDevice(pngBuffer.data().constData(), pngBuffer.data().size(), pState) == pngBuffer.data().size();
                        }
                    }
                }

                if (!bConverted && bValidFormat && (nBytesPerPixel > 0)) {
                    qint64 nExpectedSize = (qint64)nWidth * nHeight * nBytesPerPixel;

                    if (baData.size() >= nExpectedSize) {
                        QBuffer pngBuffer;
                        if (pngBuffer.open(QIODevice::ReadWrite)) {
                            bConverted = XPNG::createPNG(&pngBuffer, nWidth, nHeight, baData.left(nExpectedSize), pngColorType, nBitsPerComponent);
                            pngBuffer.close();
                        }

                        if (bConverted) {
                            bResult = XBinary::_writeDevice(pngBuffer.data().constData(), pngBuffer.data().size(), pState) == pngBuffer.data().size();
                        }
                    }
                }

                if (!bConverted && (sColorSpace == QLatin1String("/DeviceCMYK")) && (nBitsPerComponent == 8)) {
                    // Manual CMYK to RGB conversion
                    qint64 nExpectedSize = (qint64)nWidth * nHeight * 4;

                    if (baData.size() >= nExpectedSize) {
                        QByteArray baRgbData;
                        baRgbData.resize(nWidth * nHeight * 3);

                        const quint8 *pSrc = (const quint8 *)baData.constData();
                        quint8 *pDst = (quint8 *)baRgbData.data();

                        for (qint32 y = 0; y < nHeight; y++) {
                            for (qint32 x = 0; x < nWidth; x++) {
                                qint32 nSrcIdx = (y * nWidth + x) * 4;
                                qint32 nDstIdx = (y * nWidth + x) * 3;
                                quint8 nC = pSrc[nSrcIdx];
                                quint8 nM = pSrc[nSrcIdx + 1];
                                quint8 nY = pSrc[nSrcIdx + 2];
                                quint8 nK = pSrc[nSrcIdx + 3];

                                pDst[nDstIdx] = (quint8)(255 - qMin(255, (qint32)nC + nK));
                                pDst[nDstIdx + 1] = (quint8)(255 - qMin(255, (qint32)nM + nK));
                                pDst[nDstIdx + 2] = (quint8)(255 - qMin(255, (qint32)nY + nK));
                            }
                        }

                        QBuffer pngBuffer;
                        if (pngBuffer.open(QIODevice::ReadWrite)) {
                            bConverted = XPNG::createPNG(&pngBuffer, nWidth, nHeight, baRgbData, XPNG::COLOR_TYPE_RGB);
                            pngBuffer.close();
                        }

                        if (bConverted) {
                            bResult = XBinary::_writeDevice(pngBuffer.data().constData(), pngBuffer.data().size(), pState) == pngBuffer.data().size();
                        }
                    }
                }

                if (!bConverted && (nBitsPerComponent == 1)) {
                    // 1-bit monochrome
                    qint32 nBytesPerRow = (nWidth + 7) / 8;
                    qint64 nExpectedSize = (qint64)nBytesPerRow * nHeight;

                    if (baData.size() >= nExpectedSize) {
                        QBuffer pngBuffer;
                        if (pngBuffer.open(QIODevice::ReadWrite)) {
                            bConverted = XPNG::createPNG(&pngBuffer, nWidth, nHeight, baData.left(nExpectedSize), XPNG::COLOR_TYPE_GRAYSCALE, 1);
                            pngBuffer.close();
                        }

                        if (bConverted) {
                            bResult = XBinary::_writeDevice(pngBuffer.data().constData(), pngBuffer.data().size(), pState) == pngBuffer.data().size();
                        }
                    }
                }
            }

            if (!bConverted) {
                // Fallback: write raw decompressed data
                bResult = XBinary::_writeDevice(baData.constData(), baData.size(), pState) == baData.size();
            }
        }
    } else if ((compressMethod == XBinary::HANDLE_METHOD_STORE_CAB) || (compressMethod == XBinary::HANDLE_METHOD_MSZIP_CAB)) {
        // CAB archive: data is stored in CFDATA blocks with 8-byte headers
        // CFDATA: checksum(4) + cbData(2) + cbUncomp(2) + [reserved] + payload(cbData)
        qint64 nSubstreamOffset = pState->mapProperties.value(XBinary::FPART_PROP_SUBSTREAMOFFSET, 0).toLongLong();
        qint64 nDataReservedSize = pState->mapProperties.value(XBinary::FPART_PROP_OPTHEADER_SIZE, 0).toLongLong();
        qint64 nStreamSize = pState->nInputLimit;
        const qint64 nCFDataHeaderSize = 8;  // sizeof(CFDATA): checksum(4) + cbData(2) + cbUncomp(2)
        bool bTargetRangeValid = (nSubstreamOffset >= 0) && (nUncompressedSize >= 0) &&
                                 (nUncompressedSize <= (std::numeric_limits<qint64>::max)() - nSubstreamOffset);
        qint64 nMinimumFolderSize = bTargetRangeValid ? nSubstreamOffset + nUncompressedSize : -1;
        qint64 nDeclaredFolderSize = pState->mapProperties.value(XBinary::FPART_PROP_STREAMUNPACKEDSIZE, nMinimumFolderSize).toLongLong();

        QByteArray baFolderData;
        qint64 nOffset = 0;
        qint64 nDecodedFolderSize = 0;
        bResult = bTargetRangeValid && (nDataReservedSize >= 0) && (nStreamSize >= 0) && decIsValidBufferSize(nDeclaredFolderSize) &&
                  (nDeclaredFolderSize >= nMinimumFolderSize);
        if (bResult) {
            baFolderData.reserve((qint32)nDeclaredFolderSize);
        }

        while (bResult && (nOffset < nStreamSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            if ((nCFDataHeaderSize + nDataReservedSize > nStreamSize - nOffset) ||
                !pState->pDeviceInput->seek(pState->nInputOffset + nOffset)) {
                bResult = false;
                break;
            }

            // Read CFDATA header (little-endian)
            char header[8];
            if (pState->pDeviceInput->read(header, 8) != 8) {
                bResult = false;
                break;
            }

            quint16 nCbData = (quint8)header[4] | ((quint16)(quint8)header[5] << 8);
            quint16 nCbUncomp = (quint8)header[6] | ((quint16)(quint8)header[7] << 8);

            qint64 nPayloadOffset = nOffset + nCFDataHeaderSize + nDataReservedSize;

            if ((nCbData == 0) || (nCbUncomp > 32768) || ((qint64)nCbData > nStreamSize - nPayloadOffset) ||
                ((qint64)nCbUncomp > nDeclaredFolderSize - nDecodedFolderSize)) {
                bResult = false;
                break;
            }

            if (!pState->pDeviceInput->seek(pState->nInputOffset + nPayloadOffset)) {
                bResult = false;
                break;
            }
            QByteArray baPayload = pState->pDeviceInput->read(nCbData);

            if (baPayload.size() != nCbData) {
                bResult = false;
                break;
            }

            if (compressMethod == XBinary::HANDLE_METHOD_STORE_CAB) {
                if (nCbData != nCbUncomp) {
                    bResult = false;
                    break;
                }
                baFolderData.append(baPayload);
            } else {
                QByteArray baUncompressedBlock;
                if (!decInflateMSZIPBlock(baPayload, baFolderData, nCbUncomp, &baUncompressedBlock, pPdStruct)) {
                    bResult = false;
                    break;
                }

                baFolderData.append(baUncompressedBlock);
            }

            nDecodedFolderSize += nCbUncomp;
            nOffset = nPayloadOffset + nCbData;
        }

        bResult = bResult && XBinary::isPdStructNotCanceled(pPdStruct) && (nOffset == nStreamSize) &&
                  (nDecodedFolderSize == nDeclaredFolderSize) && (baFolderData.size() == nDeclaredFolderSize);

        if (bResult) {
            const char *pOutput = (nUncompressedSize > 0) ? (baFolderData.constData() + nSubstreamOffset) : nullptr;
            bResult = decWriteAll(pState->pDeviceOutput, pOutput, nUncompressedSize, pPdStruct);
            pState->nCountOutput = bResult ? nUncompressedSize : 0;
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZX_CAB) {
        // CAB LZX: gather every CFDATA payload into one folder stream, then LZX-decode the whole folder.
        // The window bits are carried in FPART_PROP_WINDOWSIZE (extracted from CFFOLDER.typeCompress).
        qint64 nSubstreamOffset = pState->mapProperties.value(XBinary::FPART_PROP_SUBSTREAMOFFSET, 0).toLongLong();
        qint64 nDataReservedSize = pState->mapProperties.value(XBinary::FPART_PROP_OPTHEADER_SIZE, 0).toLongLong();
        qint32 nWindowBits = (qint32)pState->mapProperties.value(XBinary::FPART_PROP_WINDOWSIZE, 0).toInt();
        qint64 nStreamSize = pState->nInputLimit;
        const qint64 nCFDataHeaderSize = 8;

        if ((nWindowBits < 15) || (nWindowBits > 21)) {
            nWindowBits = 21;  // Reasonable default; most CAB LZX folders use the largest window
        }

        QByteArray baCompressedFolder;
        bool bFolderSizeValid = (nSubstreamOffset >= 0) && (nUncompressedSize >= 0) &&
                                (nUncompressedSize <= (std::numeric_limits<qint64>::max)() - nSubstreamOffset);
        qint64 nMinimumFolderSize = bFolderSizeValid ? (nSubstreamOffset + nUncompressedSize) : -1;
        qint64 nFolderUncompressed = pState->mapProperties.value(XBinary::FPART_PROP_STREAMUNPACKEDSIZE, nMinimumFolderSize).toLongLong();
        qint64 nOffset = 0;
        qint64 nDeclaredBlockOutput = 0;
        bResult = bFolderSizeValid && (nDataReservedSize >= 0) && (nStreamSize >= 0) && decIsValidBufferSize(nFolderUncompressed) &&
                  (nFolderUncompressed >= nMinimumFolderSize);

        while (bResult && (nOffset < nStreamSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            if ((nCFDataHeaderSize + nDataReservedSize > nStreamSize - nOffset) ||
                !pState->pDeviceInput->seek(pState->nInputOffset + nOffset)) {
                bResult = false;
                break;
            }

            char header[8];
            if (pState->pDeviceInput->read(header, 8) != 8) {
                bResult = false;
                break;
            }

            quint16 nCbData = (quint8)header[4] | ((quint16)(quint8)header[5] << 8);
            quint16 nCbUncomp = (quint8)header[6] | ((quint16)(quint8)header[7] << 8);

            qint64 nPayloadOffset = nOffset + nCFDataHeaderSize + nDataReservedSize;

            if ((nCbData == 0) || (nCbUncomp > 32768) || ((qint64)nCbData > nStreamSize - nPayloadOffset) ||
                ((qint64)nCbUncomp > nFolderUncompressed - nDeclaredBlockOutput)) {
                bResult = false;
                break;
            }

            if (!pState->pDeviceInput->seek(pState->nInputOffset + nPayloadOffset)) {
                bResult = false;
                break;
            }
            QByteArray baPayload = pState->pDeviceInput->read(nCbData);

            if (baPayload.size() != nCbData) {
                bResult = false;
                break;
            }

            baCompressedFolder.append(baPayload);
            nDeclaredBlockOutput += nCbUncomp;
            nOffset = nPayloadOffset + nCbData;
        }

        bResult = bResult && XBinary::isPdStructNotCanceled(pPdStruct) && (nOffset == nStreamSize) &&
                  (nDeclaredBlockOutput == nFolderUncompressed);

        if (bResult && (nFolderUncompressed == 0)) {
            bResult = (nUncompressedSize == 0) && (nSubstreamOffset == 0) &&
                      decWriteAll(pState->pDeviceOutput, nullptr, 0, pPdStruct);
            pState->nCountOutput = 0;
        } else if (bResult) {
            QByteArray baFolderData;
            bResult = XLZXDecoder::decompressCABFolder(baCompressedFolder, &baFolderData, nFolderUncompressed, nWindowBits, pPdStruct);

            if (bResult && (baFolderData.size() == nFolderUncompressed)) {
                const char *pOutput = (nUncompressedSize > 0) ? (baFolderData.constData() + nSubstreamOffset) : nullptr;
                bResult = decWriteAll(pState->pDeviceOutput, pOutput, nUncompressedSize, pPdStruct);
                pState->nCountOutput = bResult ? nUncompressedSize : 0;
            } else {
                bResult = false;
            }
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_ZSTD) {
        bResult = XZstdDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZOP) {
        bResult = XLZODecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_COMPRESS) {
        bResult = XCompressDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZIP) {
        // LZIP uses LZMA internally with fixed properties: lc=3, lp=0, pb=2
        // Read dict size code from byte 5 of the LZIP header
        qint64 nSavedInputOffset = pState->nInputOffset;
        pState->pDeviceInput->seek(nSavedInputOffset + 5);
        char cDictSizeCode = 0;
        pState->pDeviceInput->read(&cDictSizeCode, 1);
        quint8 nDictSizeCode = (quint8)cDictSizeCode;
        quint8 nExponent = nDictSizeCode & 0x1F;
        quint32 nDictSize = (nExponent >= 12 && nExponent <= 29) ? (1U << nExponent) : 4096;

        // Build 5-byte LZMA properties: prop_byte + dict_size(4 LE)
        QByteArray baProperty(5, 0);
        baProperty[0] = (char)0x5D;  // lc=3, lp=0, pb=2
        baProperty[1] = (char)(nDictSize & 0xFF);
        baProperty[2] = (char)((nDictSize >> 8) & 0xFF);
        baProperty[3] = (char)((nDictSize >> 16) & 0xFF);
        baProperty[4] = (char)((nDictSize >> 24) & 0xFF);

        pState->nInputOffset = nSavedInputOffset + 6;
        bResult = XLZMADecoder::decompress(pState, baProperty, pPdStruct);
        pState->nInputOffset = nSavedInputOffset;
    } else {
#ifdef QT_DEBUG
        qDebug() << "Unknown compression method" << XBinary::handleMethodToString(compressMethod);
#endif
        emit errorMessage(QString("%1: %2").arg(tr("Unknown compression method")).arg(XBinary::handleMethodToString(compressMethod)));
        bResult = false;
    }

    bResult = bResult && !pState->bReadError && !pState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
    if (!bResult && pState->pDeviceOutput && !pState->pDeviceOutput->isSequential()) {
        decClearOutputDevice(pState->pDeviceOutput);
    }

    return bResult;
}

QByteArray XDecompress::decomressToByteArray(QIODevice *pDevice, qint64 nOffset, qint64 nSize, XBinary::HANDLE_METHOD compressMethod, XBinary::PDSTRUCT *pPdStruct)
{
    QByteArray baResult;

    if (pDevice) {
        QBuffer buffer(&baResult);

        if (buffer.open(QIODevice::ReadWrite)) {
            XBinary::DATAPROCESS_STATE state = {};
            state.pDeviceInput = pDevice;
            state.pDeviceOutput = &buffer;
            state.nInputOffset = nOffset;
            state.nInputLimit = nSize;
            state.nProcessedOffset = 0;
            state.nProcessedLimit = -1;
            state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

            if (!multiDecompress(&state, pPdStruct)) {
                baResult.clear();
            }

            buffer.close();
        }
    }

    return baResult;
}

qint64 XDecompress::getCompressedDataSize(QIODevice *pDevice, qint64 nOffset, qint64 nSize, XBinary::HANDLE_METHOD compressMethod, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDevice || (nOffset < 0) || (nSize < -1)) {
        return 0;
    }

    if (nSize == -1) {
        const qint64 nDeviceSize = pDevice->size();
        if ((nDeviceSize < 0) || (nOffset > nDeviceSize)) return 0;
        nSize = nDeviceSize - nOffset;
    }

    qint64 nResult = 0;
    DecDiscardWriteDevice outputDevice;
    if (outputDevice.open(QIODevice::WriteOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.pDeviceInput = pDevice;
        state.pDeviceOutput = &outputDevice;
        state.nInputOffset = nOffset;
        state.nInputLimit = nSize;
        state.nProcessedOffset = 0;
        state.nProcessedLimit = -1;
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

        if (multiDecompress(&state, pPdStruct)) {
            nResult = state.nCountInput;
        }
        outputDevice.close();
    }

    return nResult;
}
