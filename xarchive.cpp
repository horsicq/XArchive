/* Copyright (c) 2017-2025 hors<horsicq@gmail.com>
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
#include "xarchive.h"
#include "xdecompress.h"

#if defined(_MSC_VER)
#if _MSC_VER > 1800                                   // TODO Check !!!
#pragma comment(lib, "legacy_stdio_definitions.lib")  // bzip2.lib(compress.obj) __imp__fprintf

FILE _iob[] = {*stdin, *stdout, *stderr};  // bzip2.lib(compress.obj) _iob_func

extern "C" FILE *__cdecl __iob_func(void)
{
    return _iob;
}
#endif
#endif

static void *SzAlloc(ISzAllocPtr, size_t size)
{
    return malloc(size);
}

static void SzFree(ISzAllocPtr, void *address)
{
    free(address);
}

static ISzAlloc g_Alloc = {SzAlloc, SzFree};

XArchive::XArchive(QIODevice *pDevice) : XBinary(pDevice)
{
}

XArchive::COMPRESS_RESULT XArchive::_decompress(DECOMPRESSSTRUCT *pDecompressStruct, PDSTRUCT *pPdStruct)
{
    if (pDecompressStruct->nDecompressedLimit == 0) {
        pDecompressStruct->nDecompressedLimit = -1;
    }

    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    COMPRESS_RESULT result = COMPRESS_RESULT_UNKNOWN;

    if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_STORE) {
        XBinary::DECOMPRESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_STORE);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pDecompressStruct->spInfo.nUncompressedSize);
        decompressState.pDeviceInput = pDecompressStruct->pSourceDevice;
        decompressState.pDeviceOutput = pDecompressStruct->pDestDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = pDecompressStruct->nInSize != 0 ? pDecompressStruct->nInSize : pDecompressStruct->pSourceDevice->size();
        decompressState.nDecompressedOffset = pDecompressStruct->nDecompressedOffset;
        decompressState.nDecompressedLimit = pDecompressStruct->nDecompressedLimit;

        if (XStoreDecoder::decompress(&decompressState, pPdStruct)) {
            pDecompressStruct->nInSize = decompressState.nCountInput;
            pDecompressStruct->nOutSize = decompressState.nCountOutput;
            pDecompressStruct->bLimit = (pDecompressStruct->nDecompressedLimit > 0) && (decompressState.nCountOutput >= pDecompressStruct->nDecompressedLimit);
            result = COMPRESS_RESULT_OK;
        } else {
            result = decompressState.bReadError ? COMPRESS_RESULT_READERROR : COMPRESS_RESULT_DATAERROR;
        }
    } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_PPMD) {
        // TODO Check
#ifdef PPMD_SUPPORT
        quint8 nOrder = 0;
        quint32 nMemSize = 0;

        pSourceDevice->read((char *)(&nOrder), 1);
        pSourceDevice->read((char *)(&nMemSize), 4);

        bool bSuccess = true;

        if ((nOrder < PPMD7_MIN_ORDER) || (nOrder > PPMD7_MAX_ORDER) || (nMemSize < PPMD7_MIN_MEM_SIZE) || (nMemSize > PPMD7_MAX_MEM_SIZE)) {
            bSuccess = false;
        }

        bSuccess = true;

        if (bSuccess) {
            CPpmd7 ppmd;
            Ppmd7_Construct(&ppmd);

            if (Ppmd7_Alloc(&ppmd, nMemSize, &g_Alloc)) {
                Ppmd7_Init(&ppmd, nOrder);
            }
        }
#endif
    } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_DEFLATE) {
        XBinary::DECOMPRESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_DEFLATE);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pDecompressStruct->spInfo.nUncompressedSize);
        decompressState.pDeviceInput = pDecompressStruct->pSourceDevice;
        decompressState.pDeviceOutput = pDecompressStruct->pDestDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = pDecompressStruct->nInSize != 0 ? pDecompressStruct->nInSize : pDecompressStruct->pSourceDevice->size();
        decompressState.nDecompressedOffset = pDecompressStruct->nDecompressedOffset;
        decompressState.nDecompressedLimit = pDecompressStruct->nDecompressedLimit;

        if (XDeflateDecoder::decompress(&decompressState, pPdStruct)) {
            pDecompressStruct->nInSize = decompressState.nCountInput;
            pDecompressStruct->nOutSize = decompressState.nCountOutput;
            pDecompressStruct->bLimit = (pDecompressStruct->nDecompressedLimit > 0) && (decompressState.nCountOutput >= pDecompressStruct->nDecompressedLimit);
            result = COMPRESS_RESULT_OK;
        } else {
            if (decompressState.bReadError) {
                result = COMPRESS_RESULT_READERROR;
            } else if (decompressState.bWriteError) {
                result = COMPRESS_RESULT_WRITEERROR;
            } else {
                result = COMPRESS_RESULT_DATAERROR;
            }
        }
    } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_BZIP2) {
        XBinary::DECOMPRESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_BZIP2);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pDecompressStruct->spInfo.nUncompressedSize);
        decompressState.pDeviceInput = pDecompressStruct->pSourceDevice;
        decompressState.pDeviceOutput = pDecompressStruct->pDestDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = pDecompressStruct->nInSize != 0 ? pDecompressStruct->nInSize : pDecompressStruct->pSourceDevice->size();
        decompressState.nDecompressedOffset = pDecompressStruct->nDecompressedOffset;
        decompressState.nDecompressedLimit = pDecompressStruct->nDecompressedLimit;

        if (XBZIP2Decoder::decompress(&decompressState, pPdStruct)) {
            pDecompressStruct->nInSize = decompressState.nCountInput;
            pDecompressStruct->nOutSize = decompressState.nCountOutput;
            pDecompressStruct->bLimit = (pDecompressStruct->nDecompressedLimit > 0) && (decompressState.nCountOutput >= pDecompressStruct->nDecompressedLimit);
            result = COMPRESS_RESULT_OK;
        } else {
            if (decompressState.bReadError) {
                result = COMPRESS_RESULT_READERROR;
            } else if (decompressState.bWriteError) {
                result = COMPRESS_RESULT_WRITEERROR;
            } else {
                result = COMPRESS_RESULT_DATAERROR;
            }
        }
    } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_LZMA) {
        XBinary::DECOMPRESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_LZMA);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pDecompressStruct->spInfo.nUncompressedSize);
        decompressState.pDeviceInput = pDecompressStruct->pSourceDevice;
        decompressState.pDeviceOutput = pDecompressStruct->pDestDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = pDecompressStruct->nInSize != 0 ? pDecompressStruct->nInSize : pDecompressStruct->pSourceDevice->size();
        decompressState.nDecompressedOffset = pDecompressStruct->nDecompressedOffset;
        decompressState.nDecompressedLimit = pDecompressStruct->nDecompressedLimit;

        if (XLZMADecoder::decompress(&decompressState, pPdStruct)) {
            pDecompressStruct->nInSize = decompressState.nCountInput;
            pDecompressStruct->nOutSize = decompressState.nCountOutput;
            pDecompressStruct->bLimit = (pDecompressStruct->nDecompressedLimit > 0) && (decompressState.nCountOutput >= pDecompressStruct->nDecompressedLimit);
            result = COMPRESS_RESULT_OK;
        } else {
            if (decompressState.bReadError) {
                result = COMPRESS_RESULT_READERROR;
            } else if (decompressState.bWriteError) {
                result = COMPRESS_RESULT_WRITEERROR;
            } else {
                result = COMPRESS_RESULT_DATAERROR;
            }
        }
    } else if ((pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_LZH5) || (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_LZH6) ||
               (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_LZH7)) {
        qint32 nMethod = 5;

        if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_LZH5) {
            nMethod = 5;
        } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_LZH6) {
            nMethod = 6;
        } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_LZH7) {
            nMethod = 7;
        }

        XBinary::DECOMPRESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, pDecompressStruct->spInfo.compressMethod);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pDecompressStruct->spInfo.nUncompressedSize);
        decompressState.pDeviceInput = pDecompressStruct->pSourceDevice;
        decompressState.pDeviceOutput = pDecompressStruct->pDestDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = pDecompressStruct->nInSize != 0 ? pDecompressStruct->nInSize : pDecompressStruct->pSourceDevice->size();
        decompressState.nDecompressedOffset = pDecompressStruct->nDecompressedOffset;
        decompressState.nDecompressedLimit = pDecompressStruct->nDecompressedLimit;

        if (XLZHDecoder::decompress(&decompressState, nMethod, pPdStruct)) {
            pDecompressStruct->nInSize = decompressState.nCountInput;
            pDecompressStruct->nOutSize = decompressState.nCountOutput;
            pDecompressStruct->bLimit = (pDecompressStruct->nDecompressedLimit > 0) && (decompressState.nCountOutput >= pDecompressStruct->nDecompressedLimit);
            result = COMPRESS_RESULT_OK;
        } else {
            if (decompressState.bReadError) {
                result = COMPRESS_RESULT_READERROR;
            } else if (decompressState.bWriteError) {
                result = COMPRESS_RESULT_WRITEERROR;
            } else {
                result = COMPRESS_RESULT_DATAERROR;
            }
        }
    } else if ((pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_15) || (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_20) ||
               (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_29) || (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_50) ||
               (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_70)) {
        bool bIsSolid = false;
        rar_Unpack rarUnpack;
        rarUnpack.setDevices(pDecompressStruct->pSourceDevice, pDecompressStruct->pDestDevice);
        qint32 nInit = rarUnpack.Init(pDecompressStruct->spInfo.nWindowSize, bIsSolid);

        if (nInit > 0) {
            rarUnpack.SetDestSize(pDecompressStruct->spInfo.nUncompressedSize);

            if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_15) {
                rarUnpack.Unpack15(bIsSolid, pPdStruct);
            } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_20) {
                rarUnpack.Unpack20(bIsSolid, pPdStruct);
            } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_29) {
                rarUnpack.Unpack29(bIsSolid, pPdStruct);
            } else if ((pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_50) || (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_RAR_70)) {
                rarUnpack.Unpack5(bIsSolid, pPdStruct);
            }
        }
    } else if (pDecompressStruct->spInfo.compressMethod == COMPRESS_METHOD_LZSS_SZDD) {
        XBinary::DECOMPRESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_LZSS_SZDD);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pDecompressStruct->spInfo.nUncompressedSize);
        decompressState.pDeviceInput = pDecompressStruct->pSourceDevice;
        decompressState.pDeviceOutput = pDecompressStruct->pDestDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = pDecompressStruct->nInSize != 0 ? pDecompressStruct->nInSize : pDecompressStruct->pSourceDevice->size();
        decompressState.nDecompressedOffset = pDecompressStruct->nDecompressedOffset;
        decompressState.nDecompressedLimit = pDecompressStruct->nDecompressedLimit;

        if (XLZSSDecoder::decompress(&decompressState, pPdStruct)) {
            pDecompressStruct->nInSize = decompressState.nCountInput;
            pDecompressStruct->nOutSize = decompressState.nCountOutput;
            pDecompressStruct->bLimit = (pDecompressStruct->nDecompressedLimit > 0) && (decompressState.nCountOutput >= pDecompressStruct->nDecompressedLimit);
            result = COMPRESS_RESULT_OK;
        } else {
            if (decompressState.bReadError) {
                result = COMPRESS_RESULT_READERROR;
            } else if (decompressState.bWriteError) {
                result = COMPRESS_RESULT_WRITEERROR;
            } else {
                result = COMPRESS_RESULT_DATAERROR;
            }
        }
    }

    return result;
}

bool XArchive::_decompressRecord(const RECORD *pRecord, QIODevice *pSourceDevice, QIODevice *pDestDevice, PDSTRUCT *pPdStruct, qint64 nDecompressedOffset = 0,
                                 qint64 nDecompressedLimit = -1)
{
    bool bResult = false;

    SubDevice sd(pSourceDevice, pRecord->nDataOffset, pRecord->nDataSize);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DECOMPRESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, pRecord->spInfo.compressMethod);
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pRecord->spInfo.nUncompressedSize);
        state.mapProperties.insert(XBinary::FPART_PROP_WINDOWSIZE, pRecord->spInfo.nWindowSize);
        state.pDeviceInput = &sd;
        state.pDeviceOutput = pDestDevice;
        state.nInputOffset = 0;
        state.nInputLimit = pRecord->nDataSize;
        state.nDecompressedOffset = nDecompressedOffset;
        state.nDecompressedLimit = nDecompressedLimit;

        XDecompress decompressor;
        bResult = decompressor.decompress(&state, pPdStruct);

        sd.close();
    }

    return bResult;
}

XArchive::COMPRESS_RESULT XArchive::_compress(XArchive::COMPRESS_METHOD compressMethod, QIODevice *pSourceDevice, QIODevice *pDestDevice, PDSTRUCT *pPdStruct)
{
    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    COMPRESS_RESULT result = COMPRESS_RESULT_UNKNOWN;

    if (compressMethod == COMPRESS_METHOD_STORE) {
        const qint32 CHUNK = COMPRESS_BUFFERSIZE;
        char buffer[CHUNK];
        qint64 nSize = pSourceDevice->size();

        result = COMPRESS_RESULT_OK;

        while ((nSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            qint64 nTemp = qMin((qint64)CHUNK, nSize);

            if (pSourceDevice->read(buffer, nTemp) != nTemp) {
                result = COMPRESS_RESULT_READERROR;
                break;
            }

            if (pDestDevice->write(buffer, nTemp) != nTemp) {
                result = COMPRESS_RESULT_WRITEERROR;
                break;
            }

            nSize -= nTemp;
        }
    } else if (compressMethod == COMPRESS_METHOD_DEFLATE) {
        result = _compress_deflate(pSourceDevice, pDestDevice, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);  // -MAX_WBITS for raw data
    }

    return result;
}

XArchive::COMPRESS_RESULT XArchive::_compress_deflate(QIODevice *pSourceDevice, QIODevice *pDestDevice, qint32 nLevel, qint32 nMethod, qint32 nWindowsBits,
                                                      qint32 nMemLevel, qint32 nStrategy, PDSTRUCT *pPdStruct)
{
    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    COMPRESS_RESULT result = COMPRESS_RESULT_UNKNOWN;

    const qint32 CHUNK = COMPRESS_BUFFERSIZE;

    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    z_stream strm;

    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.opaque = nullptr;
    strm.avail_in = 0;
    strm.next_in = nullptr;

    qint32 ret = Z_OK;

    if (deflateInit2(&strm, nLevel, nMethod, nWindowsBits, nMemLevel, nStrategy) == Z_OK) {
        do {
            strm.avail_in = pSourceDevice->read((char *)in, CHUNK);

            qint32 nFlush = Z_NO_FLUSH;

            if (strm.avail_in != CHUNK) {
                nFlush = Z_FINISH;
            }

            if (strm.avail_in == 0) {
                if (!pSourceDevice->atEnd()) {
                    ret = Z_ERRNO;
                    break;
                }
            }

            strm.next_in = in;

            do {
                strm.avail_out = CHUNK;
                strm.next_out = out;
                ret = deflate(&strm, nFlush);

                if ((ret == Z_DATA_ERROR) || (ret == Z_MEM_ERROR) || (ret == Z_NEED_DICT)) {
                    break;
                }

                qint32 nTemp = CHUNK - strm.avail_out;

                if (pDestDevice->write((char *)out, nTemp) != nTemp) {
                    ret = Z_ERRNO;
                    break;
                }
            } while ((strm.avail_out == 0) && XBinary::isPdStructNotCanceled(pPdStruct));

            if ((ret == Z_DATA_ERROR) || (ret == Z_MEM_ERROR) || (ret == Z_NEED_DICT) || (ret == Z_ERRNO)) {
                break;
            }
        } while (ret != Z_STREAM_END);

        deflateEnd(&strm);

        if ((ret == Z_OK) || (ret == Z_STREAM_END)) {
            result = COMPRESS_RESULT_OK;
        } else if (ret == Z_BUF_ERROR) {
            result = COMPRESS_RESULT_BUFFERERROR;
        } else if (ret == Z_MEM_ERROR) {
            result = COMPRESS_RESULT_MEMORYERROR;
        } else if (ret == Z_DATA_ERROR) {
            result = COMPRESS_RESULT_DATAERROR;
        } else {
            result = COMPRESS_RESULT_UNKNOWN;
        }
    }

    return result;
}

QByteArray XArchive::decompress(const XArchive::RECORD *pRecord, PDSTRUCT *pPdStruct, qint64 nDecompressedOffset, qint64 nDecompressedLimit)
{
    QByteArray result;

    QBuffer buffer;
    buffer.setBuffer(&result);

    if (buffer.open(QIODevice::WriteOnly)) {
        _decompressRecord(pRecord, getDevice(), &buffer, pPdStruct, nDecompressedOffset, nDecompressedLimit);
        buffer.close();
    }

    return result;
}

QByteArray XArchive::decompress(QList<XArchive::RECORD> *pListArchive, const QString &sRecordFileName, PDSTRUCT *pPdStruct)
{
    QByteArray baResult;

    XArchive::RECORD record = XArchive::getArchiveRecord(sRecordFileName, pListArchive, pPdStruct);

    if (!record.spInfo.sRecordName.isEmpty()) {
        if (record.spInfo.nUncompressedSize) {
            baResult = decompress(&record, pPdStruct);
        }
    }

    return baResult;
}

QByteArray XArchive::decompress(const QString &sRecordFileName, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listArchive = getRecords(-1, pPdStruct);

    return decompress(&listArchive, sRecordFileName, pPdStruct);
}

bool XArchive::decompressToFile(const XArchive::RECORD *pRecord, const QString &sResultFileName, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    QFileInfo fi(sResultFileName);

    bResult = XBinary::createDirectory(fi.absolutePath());

    if (pRecord->nDataSize) {
        QFile file;
        file.setFileName(sResultFileName);

        if (file.open(QIODevice::ReadWrite)) {
            bResult = _decompressRecord(pRecord, getDevice(), &file, pPdStruct, 0, -1);
            file.close();
        }
    }

    return bResult;
}

bool XArchive::decompressToDevice(const RECORD *pRecord, QIODevice *pDestDevice, PDSTRUCT *pPdStruct)
{
    return _decompressRecord(pRecord, getDevice(), pDestDevice, pPdStruct, 0, -1);
}

bool XArchive::decompressToFile(QList<XArchive::RECORD> *pListArchive, const QString &sRecordFileName, const QString &sResultFileName, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    XArchive::RECORD record = getArchiveRecord(sRecordFileName, pListArchive);

    if (record.spInfo.sRecordName != "")  // TODO bIsValid
    {
        bResult = decompressToFile(&record, sResultFileName, pPdStruct);
    }

    return bResult;
}

bool XArchive::decompressToPath(QList<XArchive::RECORD> *pListArchive, const QString &sRecordFileName, const QString &sResultPathName, PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    QFileInfo fi(sResultPathName);

    XBinary::createDirectory(fi.absolutePath());

    qint32 nNumberOfArchives = pListArchive->count();

    for (qint32 i = 0; (i < nNumberOfArchives) && isPdStructNotCanceled(pPdStruct); i++) {
        XArchive::RECORD record = pListArchive->at(i);

        bool bNamePresent = XBinary::isRegExpPresent(QString("^%1").arg(sRecordFileName), record.spInfo.sRecordName);

        if (bNamePresent || (sRecordFileName == "/") || (sRecordFileName == "")) {
            QString sFileName = record.spInfo.sRecordName;

            if (bNamePresent) {
                sFileName = sFileName.mid(sRecordFileName.size(), -1);
            }

            QString sResultFileName = sResultPathName + QDir::separator() + sFileName;

            QFileInfo fi(sResultFileName);
            XBinary::createDirectory(fi.absolutePath());

            if (!decompressToFile(&record, sResultFileName, pPdStruct)) {
                bResult = false;
            }
        }
    }
    // TODO emits

    return bResult;
}

bool XArchive::decompressToFile(const QString &sArchiveFileName, const QString &sRecordFileName, const QString &sResultFileName, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    QFile file;
    file.setFileName(sArchiveFileName);

    if (file.open(QIODevice::ReadOnly)) {
        setDevice(&file);

        if (isValid()) {
            QList<RECORD> listRecords = getRecords(-1, pPdStruct);

            bResult = decompressToFile(&listRecords, sRecordFileName, sResultFileName, pPdStruct);
        }

        file.close();
    }

    return bResult;
}

bool XArchive::decompressToPath(const QString &sArchiveFileName, const QString &sRecordPathName, const QString &sResultPathName, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    QFile file;
    file.setFileName(sArchiveFileName);

    if (file.open(QIODevice::ReadOnly)) {
        setDevice(&file);

        if (isValid()) {
            QList<RECORD> listRecords = getRecords(-1, pPdStruct);

            bResult = decompressToPath(&listRecords, sRecordPathName, sResultPathName, pPdStruct);
        }

        file.close();
    }

    return bResult;
}

bool XArchive::dumpToFile(const XArchive::RECORD *pRecord, const QString &sFileName, PDSTRUCT *pPdStruct)
{
    return XBinary::dumpToFile(sFileName, pRecord->nDataOffset, pRecord->nDataSize, pPdStruct);
}

XArchive::RECORD XArchive::getArchiveRecord(const QString &sRecordFileName, QList<XArchive::RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    RECORD result = {};

    qint32 nNumberOfArchives = pListRecords->count();

    for (qint32 i = 0; (i < nNumberOfArchives) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        if (pListRecords->at(i).spInfo.sRecordName == sRecordFileName) {
            result = pListRecords->at(i);
            break;
        }
    }

    return result;
}

XArchive::RECORD XArchive::getArchiveRecordByUUID(const QString &sUUID, QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    RECORD result = {};

    qint32 nNumberOfArchives = pListRecords->count();

    for (qint32 i = 0; (i < nNumberOfArchives) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        if (pListRecords->at(i).sUUID == sUUID) {
            result = pListRecords->at(i);
            break;
        }
    }

    return result;
}

bool XArchive::isArchiveRecordPresent(const QString &sRecordFileName, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listRecords = getRecords(-1, pPdStruct);

    return isArchiveRecordPresent(sRecordFileName, &listRecords, pPdStruct);
}

bool XArchive::isArchiveRecordPresent(const QString &sRecordFileName, QList<XArchive::RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    return (!getArchiveRecord(sRecordFileName, pListRecords, pPdStruct).spInfo.sRecordName.isEmpty());
}

bool XArchive::isArchiveRecordPresentExp(const QString &sRecordFileName, QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    qint32 nNumberOfArchives = pListRecords->count();

    for (qint32 i = 0; (i < nNumberOfArchives) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        if (isRegExpPresent(sRecordFileName, pListRecords->at(i).spInfo.sRecordName)) {
            bResult = true;
            break;
        }
    }

    return bResult;
}

quint32 XArchive::getCompressBufferSize()
{
    return COMPRESS_BUFFERSIZE;
}

quint32 XArchive::getDecompressBufferSize()
{
    return DECOMPRESS_BUFFERSIZE;
}

void XArchive::showRecords(QList<XArchive::RECORD> *pListArchive)
{
    qint32 nNumberOfRecords = pListArchive->count();

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
#ifdef QT_DEBUG
        qDebug("%s", pListArchive->at(i).spInfo.sRecordName.toUtf8().data());
#endif
    }
}

XBinary::MODE XArchive::getMode()
{
    return MODE_DATA;
}

qint32 XArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XArchive::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_ARCHIVE: sResult = tr("Archive"); break;
        case TYPE_DOSEXTENDER: sResult = QString("DOS %1").arg(tr("extender")); break;
    }

    return sResult;
}

bool XArchive::isArchive()
{
    return true;
}

bool XArchive::_writeToDevice(char *pBuffer, qint32 nBufferSize, DECOMPRESSSTRUCT *pDecompressStruct)
{
    bool bResult = true;

    if (pDecompressStruct->pDestDevice) {
        char *_pOffset = pBuffer;
        qint32 _nSize = nBufferSize;
        qint64 nDecompressedSize = pDecompressStruct->nDecompressedLimit;

        if (nDecompressedSize == -1) {
            nDecompressedSize = pDecompressStruct->nOutSize + nBufferSize;
        }

        if ((pDecompressStruct->nDecompressedOffset) < (pDecompressStruct->nOutSize + nBufferSize)) {
            if ((pDecompressStruct->nDecompressedOffset < (pDecompressStruct->nOutSize + nBufferSize)) &&
                (pDecompressStruct->nDecompressedOffset > pDecompressStruct->nOutSize)) {
                _pOffset += (pDecompressStruct->nDecompressedOffset - pDecompressStruct->nOutSize);
                _nSize -= (pDecompressStruct->nDecompressedOffset - pDecompressStruct->nOutSize);
            }

            if ((pDecompressStruct->nDecompressedOffset + nDecompressedSize) < (pDecompressStruct->nOutSize + nBufferSize)) {
                _nSize -= ((pDecompressStruct->nOutSize + nBufferSize) - (pDecompressStruct->nDecompressedOffset + nDecompressedSize));
            }

            if (_nSize > 0) {
                qint64 nBytesWrote = pDecompressStruct->pDestDevice->write(_pOffset, _nSize);

                pDecompressStruct->nDecompressedWrote += nBytesWrote;

                if (nBytesWrote != _nSize) {
                    bResult = false;
                }
            }
        }
    }

    return bResult;
}

// XBinary::_MEMORY_MAP XArchive::getMemoryMap()
//{
//     _MEMORY_MAP result={};

//    qint64 nTotalSize=getSize();

//    result.nBaseAddress=_getBaseAddress();
//    result.nRawSize=nTotalSize;
//    result.nImageSize=nTotalSize;
//    result.fileType=FT_ARCHIVE;
//    result.mode=getMode();
//    result.sArch=getArch();
//    result.bIsBigEndian=isBigEndian();
//    result.sType=getTypeAsString();

//    qint32 nIndex=0;

//    QList<XArchive::RECORD> listRecords=getRecords();

//    qint32 nNumberOfRecords=listRecords.count();

//    for(qint32 i=0;i<nNumberOfRecords;i++)
//    {
//        _MEMORY_RECORD record={};
//        record.nAddress=-1;
//        record.segment=ADDRESS_SEGMENT_FLAT;
//        record.nOffset=listRecords.at(i).nDataOffset;
//        record.nSize=listRecords.at(i).nDataSize;
//        record.nIndex=nIndex++;
//        record.type=MMT_FILESEGMENT;
//        record.sName=listRecords.at(i).sFileName;

//        result.listRecords.append(record);
//    }

//    return result;
//}
