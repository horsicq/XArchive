/* Copyright (c) 2023-2025 hors<horsicq@gmail.com>
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

static void *SzAlloc(ISzAllocPtr, size_t size)
{
    return malloc(size);
}

static void SzFree(ISzAllocPtr, void *address)
{
    free(address);
}

static ISzAlloc g_Alloc = {SzAlloc, SzFree};

qint32 XDecompress::_readDevice(char *pBuffer, qint32 nBufferSize, STATE *pState)
{
    qint32 nRead = pState->pDeviceInput->read(pBuffer, nBufferSize);

    pState->nCountInput += nRead;

    if (nRead != nBufferSize) {
        pState->bReadError = true;
    }

    return nRead;
}

qint32 XDecompress::_writeDevice(char *pBuffer, qint32 nBufferSize, STATE *pState)
{
    qint64 nRealSize = 0;
    qint64 nSkip = 0;

    if (pState->nDecompressedOffset == 0 && (pState->nDecompressedLimit == -1)) {
        nRealSize = nBufferSize;
        nSkip = 0;
    } else if (pState->nDecompressedOffset > 0) {
        nSkip = pState->nDecompressedOffset; // TODO fix
        nRealSize = nBufferSize - nSkip;

        if (nRealSize < 0) {
            nRealSize = 0;
        }

        if (pState->nDecompressedLimit != -1) {
            if ((pState->nDecompressedOffset + nRealSize) > pState->nDecompressedLimit) {
                nRealSize = pState->nDecompressedLimit - pState->nDecompressedOffset;
            }
        }
    } else {
        nRealSize = nBufferSize;
    }

    if ((nRealSize > 0) && (pState->pDeviceOutput)) {
        qint64 nWritten = pState->pDeviceOutput->write(pBuffer + nSkip, nRealSize);

        if (nWritten != nRealSize) {
            pState->bWriteError = true;
        }
    }

    pState->nCountOutput += nBufferSize;

    return nBufferSize;
}

XDecompress::XDecompress(QObject *parent)
    : XThreadObject(parent)
{
    g_mode = MODE_UNKNOWN;
    g_fileFormat = XBinary::FT_UNKNOWN;
    g_pDevice = nullptr;
    g_pPdStruct = nullptr;
}

bool XDecompress::decompressFPART(const XBinary::FPART &fpart, QIODevice *pDeviceInput, QIODevice *pDeviceOutput, qint64 nDecompressedOffset, qint64 nDecompressedLimit, XBinary::PDSTRUCT *pPdStruct)
{
    STATE state = {};
    state.compressMethod = (XBinary::COMPRESS_METHOD)fpart.mapProperties.value(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_UNKNOWN).toUInt();
    state.pDeviceInput = pDeviceInput;
    state.pDeviceOutput = pDeviceOutput;
    state.nInputOffset = fpart.nFileOffset;
    state.nInputLimit = fpart.nFileSize;
    state.nDecompressedOffset = nDecompressedOffset;
    state.nDecompressedLimit = nDecompressedLimit;

    return decompress(&state, pPdStruct);
}

bool XDecompress::checkCRC(const XBinary::FPART &fpart, QIODevice *pDevice, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    XBinary::CRC_TYPE crcType = (XBinary::CRC_TYPE)fpart.mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();

    if (crcType != XBinary::CRC_TYPE_UNKNOWN) {
        QVariant varCRC;
        QVariant varCRC_calc;

        XBinary binary(pDevice);

        pDevice->reset();

        if (crcType == XBinary::CRC_TYPE_ZIP) {
            varCRC = fpart.mapProperties.value(XBinary::FPART_PROP_CRC_VALUE, 0).toUInt();
            varCRC_calc = binary._getCRC32(0, -1, 0xFFFFFFFF, XBinary::_getCRC32Table_EDB88320(), pPdStruct);
        }

        pDevice->reset();

        if (varCRC == varCRC_calc) {
            bResult = true;
        } else {
            emit warningMessage(QString("%1: %2").arg(tr("Invalid CRC"), fpart.sOriginalName));
        }
    } else {
        emit errorMessage(QString("%1: %2").arg(tr("Unknown CRC type"), fpart.sOriginalName));
    }

    return bResult;
}

bool XDecompress::decompress(STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pState->nInputOffset > 0) {
        pState->pDeviceInput->seek(pState->nInputOffset);
    }
    pState->pDeviceOutput->seek(0);

    const qint32 N_BUFFER_SIZE = 0x4000;

    char bufferIn[N_BUFFER_SIZE];
    char bufferOut[N_BUFFER_SIZE];

    if (pState->compressMethod == XBinary::COMPRESS_METHOD_STORE) {
        for (qint64 nOffset = 0; (nOffset < pState->nInputLimit) && XBinary::isPdStructNotCanceled(pPdStruct);) {
            qint32 nBufferSize = qMin((qint32)(pState->nInputLimit - nOffset), N_BUFFER_SIZE);

            qint32 nRead = _readDevice(bufferIn, nBufferSize, pState);

            if (nRead > 0) {
                _writeDevice(bufferIn, nRead, pState);
            } else {
                break;
            }

            if (pState->bReadError || pState->bWriteError) {
                break;
            }

            nOffset += nRead;
        }
    } else if (pState->compressMethod == XBinary::COMPRESS_METHOD_BZIP2) {
        bz_stream strm = {};
        qint32 ret = BZ_MEM_ERROR;

        qint32 rc = BZ2_bzDecompressInit(&strm, 0, 0);

        if (rc == BZ_OK) {
            do {
                qint32 nBufferSize = qMin((qint32)(pState->nInputLimit - pState->nCountInput), N_BUFFER_SIZE);

                strm.avail_in = _readDevice(bufferIn, nBufferSize, pState);

                if (strm.avail_in == 0) {
                    ret = BZ_MEM_ERROR;
                    break;
                }

                strm.next_in = bufferIn;

                do {
                    strm.total_in_hi32 = 0;
                    strm.total_in_lo32 = 0;
                    strm.total_out_hi32 = 0;
                    strm.total_out_lo32 = 0;
                    strm.avail_out = N_BUFFER_SIZE;
                    strm.next_out = bufferOut;
                    ret = BZ2_bzDecompress(&strm);

                    if ((ret != BZ_STREAM_END) && (ret != BZ_OK)) {
                        break;
                    }

                    qint32 nTemp = N_BUFFER_SIZE - strm.avail_out;

                    if (nTemp > 0) {
                        if (!_writeDevice((char *)bufferOut, nTemp, pState)) {
                            ret = BZ_MEM_ERROR;
                            break;
                        }
                    }
                } while (strm.avail_out == 0);

                if (ret != BZ_OK) {
                    break;
                }
            } while (ret != BZ_STREAM_END);

            BZ2_bzDecompressEnd(&strm);
        }
    } else if (pState->compressMethod == XBinary::COMPRESS_METHOD_LZMA) {
        if (pState->nInputLimit >= 4) {
            qint32 nPropSize = 0;
            char header1[4] = {};
            quint8 properties[32] = {};

            _readDevice(header1, sizeof(header1), pState);
            // if (header1[0] != 0x5D || header1[1] != 0x00 || header1[2] != 0x00 || header1[3] != 0x00) {
            //     emit errorMessage(tr("Invalid LZMA header"));
            //     return false;
            // }
            nPropSize = header1[2];  // TODO Check

            if (nPropSize && (nPropSize < 30)) {
                _readDevice((char *)properties, nPropSize, pState);

                CLzmaDec state = {};

                SRes ret = LzmaProps_Decode(&state.prop, (Byte *)properties, nPropSize);

                if (ret == 0)  // S_OK
                {
                    LzmaDec_Construct(&state);
                    ret = LzmaDec_Allocate(&state, (Byte *)properties, nPropSize, &g_Alloc);

                    if (ret == 0)  // S_OK
                    {
                        LzmaDec_Init(&state);
                        bool bRun = true;

                        while (bRun) {
                            qint32 nBufferSize = qMin((qint32)(pState->nInputLimit - pState->nCountInput), N_BUFFER_SIZE);
                            qint32 nSize = _readDevice(bufferIn, nBufferSize, pState);

                            if (nSize) {
                                qint64 nPos = 0;

                                while (true) {
                                    ELzmaStatus status;
                                    SizeT inProcessed = nSize - nPos;
                                    SizeT outProcessed = N_BUFFER_SIZE;

                                    ret = LzmaDec_DecodeToBuf(&state, (Byte *)bufferOut, &outProcessed, (Byte *)(bufferIn + nPos), &inProcessed, LZMA_FINISH_ANY, &status);

                                    // TODO Check ret

                                    nPos += inProcessed;

                                    if (!_writeDevice((char *)bufferOut, (qint32)outProcessed, pState)) {
                                        // result = COMPRESS_RESULT_WRITEERROR;
                                        bRun = false;
                                        break;
                                    }

                                    if (status != LZMA_STATUS_NOT_FINISHED) {
                                        if (status == LZMA_STATUS_FINISHED_WITH_MARK) {
                                            // result = COMPRESS_RESULT_OK;
                                            bRun = false;
                                        }

                                        break;
                                    }
                                }
                            } else {
                                // result = COMPRESS_RESULT_READERROR;
                                bRun = false;
                            }
                        }
                    }

                    LzmaDec_Free(&state, &g_Alloc);
                }
            }
        }
    } else {
        emit errorMessage(tr("Unknown compression method"));
    }

    return bResult;
}

bool XDecompress::unpackDeviceToFolder(XBinary::FT fileFormat, QIODevice *pDevice, QString sFolderName, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (fileFormat == XBinary::FT_UNKNOWN) {
        fileFormat = XBinary::getPrefFileType(pDevice, true);
    }

    QList<XBinary::FPART> listParts = XFormats::getFileParts(fileFormat, pDevice, XBinary::FILEPART_STREAM, -1, false, -1, pPdStruct);

    qint32 nNumberOfParts = listParts.count();

    if (nNumberOfParts > 0) {
        if (XBinary::createDirectory(sFolderName)) {
            bResult = true;
            qint32 nGlobalIndex = XBinary::getFreeIndex(pPdStruct);
            XBinary::setPdStructInit(pPdStruct, nGlobalIndex, nNumberOfParts);

            for (qint32 i = 0; (i < nNumberOfParts) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
                XBinary::setPdStructStatus(pPdStruct, nGlobalIndex, listParts.at(i).sOriginalName);

                QString sResultFileName = sFolderName + QDir::separator() + listParts.at(i).sOriginalName;

                QFileInfo fi(sResultFileName);
                if (XBinary::createDirectory(fi.absolutePath())) {
                    QFile file;
                    file.setFileName(sResultFileName);

                    if (file.open(QIODevice::ReadWrite)) {
                        if (!decompressFPART(listParts.at(i), pDevice, &file, 0, -1, pPdStruct)) {
                            if (!checkCRC(listParts.at(i), &file, pPdStruct)) {
                                emit warningMessage(QString("%1: %2").arg(tr("Invalid CRC"), listParts.at(i).sOriginalName));
                            }
                        } else {
                            emit errorMessage(QString("%1: %2").arg(tr("Cannot decompress"), listParts.at(i).sOriginalName));
                            bResult = false;
                        }

                        file.close();
                    } else {
                        emit errorMessage(QString("%1: %2").arg(tr("Cannot create"), sResultFileName));
                        bResult = false;
                    }
                } else {
                    emit errorMessage(QString("%1: %2").arg(tr("Cannot create"), fi.absolutePath()));
                    bResult = false;
                }

                XBinary::setPdStructCurrentIncrement(pPdStruct, nGlobalIndex);
            }

            XBinary::setPdStructFinished(pPdStruct, nGlobalIndex);
        } else {
            emit errorMessage(QString("%1: %2").arg(tr("Cannot create"), sFolderName));
            bResult = false;
        }
    }

    return bResult;
}

void XDecompress::setData(MODE mode, XBinary::FT fileFormat, QIODevice *pDevice, QString sFolderName, XBinary::PDSTRUCT *pPdStruct)
{
    g_mode = mode;
    g_fileFormat = fileFormat;
    g_pDevice = pDevice;
    g_sFolderName = sFolderName;
    g_pPdStruct = pPdStruct;
}

void XDecompress::process()
{
    if (g_mode == MODE_UNPACKDEVICETOFOLDER) {
        if (g_pDevice) {
            if (!unpackDeviceToFolder(g_fileFormat, g_pDevice, g_sFolderName, g_pPdStruct)) {
                emit errorMessage(tr("Cannot unpack"));
            }
        }
    }
}
