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
#include "algo_utils.h"
#include "xalgo_local.h"

#include <QCryptographicHash>
#include <algorithm>
#include <cstdlib>

namespace {
const qint32 N_ALGO_UTILS_BUFFER_SIZE = 65536;
ISzAlloc g_lzmaAlloc = {Algo_utils::szAlloc, Algo_utils::szFree};
ISzAlloc g_ppmdAlloc = {Algo_utils::szAlloc, Algo_utils::szFree};
}  // namespace

int Algo_utils::ascii85ReadByte(XBinary::DATAPROCESS_STATE *pState)
{
    char c = 0;
    qint64 nRead = pState->pDeviceInput->read(&c, 1);
    if (nRead != 1) {
        pState->bReadError = true;
        return -1;
    }

    pState->nCountInput++;

    return (unsigned char)c;
}

void Algo_utils::ascii85WriteBytes(XBinary::DATAPROCESS_STATE *pState, const unsigned char *pBuffer, int nSize)
{
    if (nSize > 0) {
        XBinary::_writeDevice((char *)pBuffer, nSize, pState);
    }
}

void *Algo_utils::szAlloc(ISzAllocPtr pAlloc, size_t nSize)
{
    Q_UNUSED(pAlloc)

    return malloc(nSize);
}

void Algo_utils::szFree(ISzAllocPtr pAlloc, void *pAddress)
{
    Q_UNUSED(pAlloc)

    free(pAddress);
}

ISzAlloc *Algo_utils::lzmaAlloc()
{
    return &g_lzmaAlloc;
}

ISzAlloc *Algo_utils::ppmdAlloc()
{
    return &g_ppmdAlloc;
}

bool Algo_utils::decompressLZMA(CLzmaDec *pState, XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    qint32 _nBufferSize = XBinary::getBufferSize(pPdStruct);
    qint64 nExpectedOutput = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)-1).toLongLong();
    qint64 nTotalOutput = 0;

    char *bufferIn = new char[_nBufferSize];
    char *bufferOut = new char[_nBufferSize];

    ELzmaStatus lastStatus = LZMA_STATUS_NOT_FINISHED;
    qint32 nLoopCount = 0;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint32 nBufferSize = _nBufferSize;
        if (pDecompressState->nInputLimit != -1) {
            qint64 nRemainingInput = pDecompressState->nInputLimit - pDecompressState->nCountInput;
            if (nRemainingInput <= 0) {
                break;
            }
            nBufferSize = (qint32)(std::min)(nRemainingInput, (qint64)_nBufferSize);
        }
        qint32 nSize = XBinary::_readDevice(bufferIn, nBufferSize, pDecompressState);
        if (nSize < 0) {
            delete[] bufferIn;
            delete[] bufferOut;
            return false;
        }

        nLoopCount++;
        Q_UNUSED(nLoopCount)

        qint64 nPos = 0;
        bool bContinueReading = true;

        while (bContinueReading && nPos < nSize && XBinary::isPdStructNotCanceled(pPdStruct)) {
            ELzmaStatus status;
            SizeT inProcessed = nSize - nPos;
            SizeT outProcessed = _nBufferSize;
            ELzmaFinishMode finishMode = LZMA_FINISH_ANY;

            if (nExpectedOutput >= 0) {
                qint64 nRemainingOutput = nExpectedOutput - nTotalOutput;

                if (nRemainingOutput <= 0) {
                    lastStatus = LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK;
                    bContinueReading = false;
                    break;
                }

                if (outProcessed > (SizeT)nRemainingOutput) {
                    outProcessed = (SizeT)nRemainingOutput;
                    finishMode = LZMA_FINISH_END;
                }
            }

            SRes ret = X_LzmaDec_DecodeToBuf(pState, (Byte *)bufferOut, &outProcessed, (Byte *)(bufferIn + nPos), &inProcessed, finishMode, &status);

            if (ret != 0) {
                delete[] bufferIn;
                delete[] bufferOut;
                return false;
            }

            nPos += inProcessed;

            if (outProcessed > 0) {
                if (!XBinary::_writeDevice((char *)bufferOut, (qint32)outProcessed, pDecompressState)) {
                    delete[] bufferIn;
                    delete[] bufferOut;
                    return false;
                }
                nTotalOutput += outProcessed;
            }

            lastStatus = status;

            if ((status == LZMA_STATUS_FINISHED_WITH_MARK) ||
                ((status == LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK) && (nExpectedOutput >= 0) && (nTotalOutput == nExpectedOutput))) {
                bContinueReading = false;
                break;
            }

            if ((inProcessed == 0) && (outProcessed == 0)) {
                break;
            }
        }

        if ((lastStatus == LZMA_STATUS_FINISHED_WITH_MARK) ||
            ((lastStatus == LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK) && (nExpectedOutput >= 0) && (nTotalOutput == nExpectedOutput))) {
            break;
        }

        if (nSize == 0) {
            break;
        }
    }

    delete[] bufferIn;
    delete[] bufferOut;

    if (nExpectedOutput >= 0) {
        return !pDecompressState->bReadError && !pDecompressState->bWriteError && (nTotalOutput == nExpectedOutput) &&
               ((lastStatus == LZMA_STATUS_FINISHED_WITH_MARK) || (lastStatus == LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK));
    }

    return !pDecompressState->bReadError && !pDecompressState->bWriteError;
}

bool Algo_utils::decompressLZMA2(CLzma2Dec *pState, XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    qint32 _nBufferSize = XBinary::getBufferSize(pPdStruct);

    char *bufferIn = new char[_nBufferSize];
    char *bufferOut = new char[_nBufferSize];

    ELzmaStatus lastStatus = LZMA_STATUS_NOT_FINISHED;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint32 nBufferSize = _nBufferSize;
        if (pDecompressState->nInputLimit != -1) {
            qint64 nRemainingInput = pDecompressState->nInputLimit - pDecompressState->nCountInput;
            if (nRemainingInput <= 0) {
                break;
            }
            nBufferSize = (qint32)(std::min)(nRemainingInput, (qint64)_nBufferSize);
        }
        qint32 nSize = XBinary::_readDevice(bufferIn, nBufferSize, pDecompressState);
        if (nSize < 0) {
            delete[] bufferIn;
            delete[] bufferOut;
            return false;
        }

        qint64 nPos = 0;
        bool bContinueReading = true;

        while (bContinueReading && nPos < nSize && XBinary::isPdStructNotCanceled(pPdStruct)) {
            ELzmaStatus status;
            SizeT inProcessed = nSize - nPos;
            SizeT outProcessed = _nBufferSize;

            SRes ret = X_Lzma2Dec_DecodeToBuf(pState, (Byte *)bufferOut, &outProcessed, (Byte *)(bufferIn + nPos), &inProcessed, LZMA_FINISH_ANY, &status);

            if (ret != 0) {
                delete[] bufferIn;
                delete[] bufferOut;
                return false;
            }

            nPos += inProcessed;

            if (outProcessed > 0) {
                if (!XBinary::_writeDevice((char *)bufferOut, (qint32)outProcessed, pDecompressState)) {
                    delete[] bufferIn;
                    delete[] bufferOut;
                    return false;
                }
            }

            lastStatus = status;

            if (status == LZMA_STATUS_FINISHED_WITH_MARK) {
                bContinueReading = false;
                break;
            }

            if (inProcessed == 0) {
                break;
            }
        }

        if (lastStatus == LZMA_STATUS_FINISHED_WITH_MARK) {
            break;
        }

        if (nSize == 0) {
            break;
        }
    }

    delete[] bufferIn;
    delete[] bufferOut;

    return !pDecompressState->bReadError && !pDecompressState->bWriteError;
}

bool Algo_utils::xzReadVarInt(const QByteArray &baData, qint32 &nPos, quint64 &nValue)
{
    nValue = 0;
    qint32 nShift = 0;

    while (nPos < baData.size()) {
        quint8 nByte = (quint8)baData.at(nPos++);
        nValue |= ((quint64)(nByte & 0x7F)) << nShift;
        if (!(nByte & 0x80)) {
            return true;
        }
        nShift += 7;
        if (nShift > 63) {
            return false;
        }
    }

    return false;
}

void Algo_utils::applyBCJX86Decode(QByteArray &baData)
{
    qint32 nSize = baData.size();

    if (nSize < 5) {
        return;
    }

    for (qint32 i = 0; i <= nSize - 5;) {
        quint8 nOpcode = (quint8)baData.at(i);
        if ((nOpcode != 0xE8) && (nOpcode != 0xE9)) {
            i++;
            continue;
        }

        quint8 nMSB = (quint8)baData.at(i + 4);
        if ((nMSB != 0x00) && (nMSB != 0xFF)) {
            i++;
            continue;
        }

        quint32 nStored =
            (quint32)(quint8)baData[i + 1] | ((quint32)(quint8)baData[i + 2] << 8) | ((quint32)(quint8)baData[i + 3] << 16) | ((quint32)(quint8)baData[i + 4] << 24);
        quint32 nRelAddr = nStored - (quint32)(i + 5);
        baData[i + 1] = (char)(nRelAddr & 0xFF);
        baData[i + 2] = (char)((nRelAddr >> 8) & 0xFF);
        baData[i + 3] = (char)((nRelAddr >> 16) & 0xFF);
        baData[i + 4] = (char)((nRelAddr >> 24) & 0xFF);
        i += 5;
    }
}

unsigned Algo_utils::deflate64ReadFunc(void *pInDesc, unsigned char **ppBuffer)
{
    XBinary::DATAPROCESS_STATE *pDecompressState = (XBinary::DATAPROCESS_STATE *)pInDesc;

    *ppBuffer = (unsigned char *)(pDecompressState->pInputBuffer);

    qint32 nRead = XBinary::_readDevice(pDecompressState);
    if (nRead < 0) {
        pDecompressState->bReadError = true;
        return 0;
    }

    return (unsigned)nRead;
}

int Algo_utils::deflate64WriteFunc(void *pOutDesc, unsigned char *pBuffer, unsigned nSize)
{
    XBinary::DATAPROCESS_STATE *pDecompressState = (XBinary::DATAPROCESS_STATE *)pOutDesc;

    XBinary::_writeDevice((char *)pBuffer, nSize, pDecompressState);

    return pDecompressState->bWriteError ? 1 : 0;
}

bool Algo_utils::compressDeflate(XBinary::DATAPROCESS_STATE *pCompressState, XBinary::PDSTRUCT *pPdStruct, int nCompressionLevel, int nWindowBits)
{
    bool bResult = false;

    if (pCompressState && pCompressState->pDeviceInput && pCompressState->pDeviceOutput) {
        pCompressState->bReadError = false;
        pCompressState->bWriteError = false;
        pCompressState->nCountInput = 0;
        pCompressState->nCountOutput = 0;

        pCompressState->pDeviceInput->seek(pCompressState->nInputOffset);
        pCompressState->pDeviceOutput->seek(0);

        z_stream stream;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;

        if (X_deflateInit2(&stream, nCompressionLevel, Z_DEFLATED, nWindowBits, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
            return false;
        }

        char inputBuffer[N_ALGO_UTILS_BUFFER_SIZE];
        char outputBuffer[N_ALGO_UTILS_BUFFER_SIZE];

        qint64 nTotalProcessed = 0;
        int flush = Z_NO_FLUSH;
        int ret = Z_OK;

        do {
            qint32 nToRead = N_ALGO_UTILS_BUFFER_SIZE;
            if (pCompressState->nInputLimit != -1) {
                nToRead = (qint32)(std::min)(pCompressState->nInputLimit - nTotalProcessed, (qint64)N_ALGO_UTILS_BUFFER_SIZE);
            }
            if (nToRead == 0) {
                flush = Z_FINISH;
                stream.avail_in = 0;
            } else {
                qint32 nRead = pCompressState->pDeviceInput->read(inputBuffer, nToRead);
                if (nRead < 0) {
                    pCompressState->bReadError = true;
                    X_deflateEnd(&stream);
                    return false;
                } else if (nRead == 0) {
                    if ((pCompressState->nInputLimit != -1) && (nTotalProcessed < pCompressState->nInputLimit)) {
                        pCompressState->bReadError = true;
                        X_deflateEnd(&stream);
                        return false;
                    }
                    flush = Z_FINISH;
                    stream.avail_in = 0;
                } else {
                    pCompressState->nCountInput += nRead;
                    nTotalProcessed += nRead;
                    stream.avail_in = nRead;
                    stream.next_in = (Bytef *)inputBuffer;
                }
            }

            do {
                stream.avail_out = N_ALGO_UTILS_BUFFER_SIZE;
                stream.next_out = (Bytef *)outputBuffer;

                ret = X_deflate(&stream, flush);
                if (ret == Z_STREAM_ERROR) {
                    X_deflateEnd(&stream);
                    return false;
                }

                qint32 nCompressed = N_ALGO_UTILS_BUFFER_SIZE - stream.avail_out;
                if (nCompressed > 0) {
                    qint64 nWritten = pCompressState->pDeviceOutput->write(outputBuffer, nCompressed);
                    if (nWritten != nCompressed) {
                        pCompressState->bWriteError = true;
                        X_deflateEnd(&stream);
                        return false;
                    }
                    pCompressState->nCountOutput += nCompressed;
                }
            } while ((stream.avail_out == 0) && (ret != Z_STREAM_END));

            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                X_deflateEnd(&stream);
                return false;
            }
        } while ((flush != Z_FINISH) || (ret != Z_STREAM_END));

        X_deflateEnd(&stream);
        bResult =
            !pCompressState->bReadError && !pCompressState->bWriteError && ((pCompressState->nInputLimit == -1) || (nTotalProcessed == pCompressState->nInputLimit));
    }

    return bResult;
}

bool Algo_utils::getUclMethodFromState(const XBinary::DATAPROCESS_STATE *pDecompressState, XUCLDecoder::METHOD *pMethod)
{
    QVariant vMethod = pDecompressState->mapProperties.value(XBinary::FPART_PROP_TYPE);

    if (!vMethod.isValid()) {
        vMethod = pDecompressState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES);
    }

    bool bIsValid = false;
    qint32 nMethod = vMethod.toInt(&bIsValid);

    if ((!bIsValid) || (nMethod < XUCLDecoder::METHOD_NRV2B_8) || (nMethod > XUCLDecoder::METHOD_NRV2E_LE32)) {
        return false;
    }

    *pMethod = (XUCLDecoder::METHOD)nMethod;

    return true;
}

bool Algo_utils::readInputData(XBinary::DATAPROCESS_STATE *pDecompressState, QByteArray *pbaInput, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pDecompressState && pDecompressState->pDeviceInput && pbaInput) {
        qint32 nChunkSize = XBinary::getBufferSize(pPdStruct);
        qint64 nRemaining = pDecompressState->nInputLimit;

        pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);

        pbaInput->clear();

        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            qint32 nReadSize = nChunkSize;

            if (nRemaining != -1) {
                if (nRemaining <= 0) {
                    bResult = true;
                    break;
                }

                nReadSize = (qint32)(std::min)(nRemaining, (qint64)nChunkSize);
            }

            QByteArray baChunk;
            baChunk.resize(nReadSize);

            qint64 nRead = pDecompressState->pDeviceInput->read(baChunk.data(), nReadSize);

            if (nRead < 0) {
                pDecompressState->bReadError = true;
                break;
            }

            baChunk.resize((qint32)nRead);

            if (baChunk.isEmpty()) {
                bResult = (nRemaining == -1) || (nRemaining == 0);

                if (!bResult) {
                    pDecompressState->bReadError = true;
                }

                break;
            }

            pbaInput->append(baChunk);
            pDecompressState->nCountInput += baChunk.size();

            if (nRemaining != -1) {
                nRemaining -= baChunk.size();

                if (nRemaining == 0) {
                    bResult = true;
                    break;
                }
            }
        }
    }

    return bResult;
}

QByteArray Algo_utils::hmacSha1(const QByteArray &baKey, const QByteArray &baMessage)
{
    const qint32 BLOCK_SIZE = 64;
    const quint8 IPAD = 0x36;
    const quint8 OPAD = 0x5c;

    QByteArray baKeyPadded;
    if (baKey.size() > BLOCK_SIZE) {
        baKeyPadded = QCryptographicHash::hash(baKey, QCryptographicHash::Sha1);
    } else {
        baKeyPadded = baKey;
    }
    while (baKeyPadded.size() < BLOCK_SIZE) {
        baKeyPadded.append((char)0);
    }

    QByteArray baInnerKey(BLOCK_SIZE, 0);
    QByteArray baOuterKey(BLOCK_SIZE, 0);
    for (qint32 i = 0; i < BLOCK_SIZE; i++) {
        baInnerKey[i] = baKeyPadded[i] ^ IPAD;
        baOuterKey[i] = baKeyPadded[i] ^ OPAD;
    }

    QCryptographicHash innerHash(QCryptographicHash::Sha1);
    innerHash.addData(baInnerKey);
    innerHash.addData(baMessage);
    QByteArray baInnerResult = innerHash.result();

    QCryptographicHash outerHash(QCryptographicHash::Sha1);
    outerHash.addData(baOuterKey);
    outerHash.addData(baInnerResult);

    return outerHash.result();
}

Byte Algo_utils::readFromQIODeviceStream(const IByteIn *pStream)
{
    auto *pStreamEx = Z7_CONTAINER_FROM_VTBL(pStream, QIODeviceByteInStream, vt);

    if (pStreamEx->bError || !pStreamEx->pDevice) {
        pStreamEx->bError = true;
        return 0;
    }

    char c = 0;
    qint64 nRead = pStreamEx->pDevice->read(&c, 1);

    if (nRead != 1) {
        pStreamEx->bError = true;
        return 0;
    }

    return (Byte)c;
}

size_t Algo_utils::readFromState(void *pState, void *pBuffer, size_t nSize)
{
    XBinary::DATAPROCESS_STATE *pDecompressState = (XBinary::DATAPROCESS_STATE *)pState;

    qint32 nRead = XBinary::_readDevice((char *)pBuffer, (qint32)nSize, pDecompressState);
    return nRead > 0 ? (size_t)nRead : 0;
}

size_t Algo_utils::writeToState(void *pState, const void *pBuffer, size_t nSize)
{
    XBinary::DATAPROCESS_STATE *pDecompressState = (XBinary::DATAPROCESS_STATE *)pState;

    qint32 nWritten = XBinary::_writeDevice((char *)pBuffer, (qint32)nSize, pDecompressState);
    return (!pDecompressState->bWriteError && (nWritten > 0)) ? (size_t)nWritten : 0;
}
