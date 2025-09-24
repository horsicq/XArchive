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

XDecompress::XDecompress(QObject *parent) : XThreadObject(parent)
{
}

bool XDecompress::decompressFPART(const XBinary::FPART &fpart, QIODevice *pDeviceInput, QIODevice *pDeviceOutput, qint64 nDecompressedOffset, qint64 nDecompressedLimit,
                                  XBinary::PDSTRUCT *pPdStruct)
{
    XBinary::DECOMPRESS_STATE state = {};
    state.mapProperties = fpart.mapProperties;
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
    bool bResult = true;

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

bool XDecompress::decompress(XBinary::DECOMPRESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    const qint32 N_BUFFER_SIZE = 0x4000;

    char bufferIn[N_BUFFER_SIZE];
    char bufferOut[N_BUFFER_SIZE];

    XBinary::COMPRESS_METHOD compressMethod =
        (XBinary::COMPRESS_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_STORE).toUInt();

    // state.compressMethod = (XBinary::COMPRESS_METHOD)fpart.mapProperties.value(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_UNKNOWN).toUInt();
    // state.nUncompressedSize = fpart.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();

    if (compressMethod == XBinary::COMPRESS_METHOD_STORE) {
        if (pState->nInputOffset > 0) {
            pState->pDeviceInput->seek(pState->nInputOffset);
        }

        if (pState->pDeviceOutput) {
            pState->pDeviceOutput->seek(0);
        }

        for (qint64 nOffset = 0; (nOffset < pState->nInputLimit) && XBinary::isPdStructNotCanceled(pPdStruct);) {
            qint32 nBufferSize = qMin((qint32)(pState->nInputLimit - nOffset), N_BUFFER_SIZE);

            qint32 nRead = XBinary::_readDevice(bufferIn, nBufferSize, pState);

            if (nRead > 0) {
                XBinary::_writeDevice(bufferIn, nRead, pState);
            } else {
                break;
            }

            if (pState->bReadError || pState->bWriteError) {
                break;
            }

            nOffset += nRead;
        }
    } else if (compressMethod == XBinary::COMPRESS_METHOD_BZIP2) {
        if (pState->nInputOffset > 0) {
            pState->pDeviceInput->seek(pState->nInputOffset);
        }

        if (pState->pDeviceOutput) {
            pState->pDeviceOutput->seek(0);
        }

        bz_stream strm = {};
        qint32 ret = BZ_MEM_ERROR;

        qint32 rc = BZ2_bzDecompressInit(&strm, 0, 0);

        if (rc == BZ_OK) {
            do {
                qint32 nBufferSize = qMin((qint32)(pState->nInputLimit - pState->nCountInput), N_BUFFER_SIZE);

                strm.avail_in = XBinary::_readDevice(bufferIn, nBufferSize, pState);

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
                        if (!XBinary::_writeDevice((char *)bufferOut, nTemp, pState)) {
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
    } else if (compressMethod == XBinary::COMPRESS_METHOD_LZMA) {
        bResult = XLZMADecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_DEFLATE) {
        bResult = XDeflateDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_DEFLATE64) {
        bResult = XDeflateDecoder::decompress64(pState, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_IT214_8) {
        bResult = XIT214Decoder::decompress(pState, 8, false, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_IT214_16) {
        bResult = XIT214Decoder::decompress(pState, 16, false, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_IT215_8) {
        bResult = XIT214Decoder::decompress(pState, 8, true, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_IT215_16) {
        bResult = XIT214Decoder::decompress(pState, 16, true, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_IMPLODED_4KDICT_2TREES) {
        bResult = XImplodeDecoder::decompress(pState, false, false, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_IMPLODED_4KDICT_3TREES) {
        bResult = XImplodeDecoder::decompress(pState, false, true, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_IMPLODED_8KDICT_2TREES) {
        bResult = XImplodeDecoder::decompress(pState, true, false, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_IMPLODED_8KDICT_3TREES) {
        bResult = XImplodeDecoder::decompress(pState, true, true, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_SHRINK) {
        bResult = XShrinkDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_REDUCE_1) {
        bResult = XReduceDecoder::decompress(pState, 1, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_REDUCE_2) {
        bResult = XReduceDecoder::decompress(pState, 2, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_REDUCE_3) {
        bResult = XReduceDecoder::decompress(pState, 3, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_REDUCE_4) {
        bResult = XReduceDecoder::decompress(pState, 4, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_ZLIB) {
        bResult = XDeflateDecoder::decompress_zlib(pState, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_LZW_PDF) {
        bResult = XLZWDecoder::decompress_pdf(pState, pPdStruct);
        // bResult = XStoreDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_ASCII85) {
        bResult = XASCII85Decoder::decompress_pdf(pState, pPdStruct);
    } else {
#ifdef QT_DEBUG
        qDebug() << "Unknown compression method" << XBinary::compressMethodToString(compressMethod);
#endif
        emit errorMessage(QString("%1: %2").arg(tr("Unknown compression method"), XBinary::compressMethodToString(compressMethod)));
    }

    return bResult;
}

QByteArray XDecompress::decomressToByteArray(QIODevice *pDevice, qint64 nOffset, qint64 nSize, XBinary::COMPRESS_METHOD compressMethod, XBinary::PDSTRUCT *pPdStruct)
{
    QByteArray baResult;

    if (pDevice) {
        QBuffer buffer(&baResult);

        if (buffer.open(QIODevice::ReadWrite)) {
            XBinary::DECOMPRESS_STATE state = {};
            state.pDeviceInput = pDevice;
            state.pDeviceOutput = &buffer;
            state.nInputOffset = nOffset;
            state.nInputLimit = nSize;
            state.nDecompressedOffset = 0;
            state.nDecompressedLimit = -1;
            state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, compressMethod);

            decompress(&state, pPdStruct);

            buffer.close();
        }
    }

    return baResult;
}

qint64 XDecompress::getCompressedDataSize(QIODevice *pDevice, qint64 nOffset, qint64 nSize, XBinary::COMPRESS_METHOD compressMethod, XBinary::PDSTRUCT *pPdStruct)
{
    if (nSize == -1) {
        nSize = pDevice->size() - nOffset;
    }

    qint64 nResult = 0;

    if (pDevice) {
        XBinary::DECOMPRESS_STATE state = {};
        state.pDeviceInput = pDevice;
        state.pDeviceOutput = nullptr;
        state.nInputOffset = nOffset;
        state.nInputLimit = nSize;
        state.nDecompressedOffset = 0;
        state.nDecompressedLimit = -1;
        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, compressMethod);

        decompress(&state, pPdStruct);

        nResult = state.nCountInput;
    }

    return nResult;
}

void XDecompress::process()
{
}
