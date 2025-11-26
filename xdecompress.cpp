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

    return decompress(&state, pPdStruct);
}

bool XDecompress::decompressArchiveRecord(const XBinary::ARCHIVERECORD &archiveRecord, QIODevice *pDeviceInput, QIODevice *pDeviceOutput, XBinary::PDSTRUCT *pPdStruct)
{
    XBinary::DATAPROCESS_STATE state = {};
    state.mapProperties = archiveRecord.mapProperties;
    state.pDeviceInput = pDeviceInput;
    state.pDeviceOutput = pDeviceOutput;
    state.nInputOffset = archiveRecord.nStreamOffset;
    state.nInputLimit = archiveRecord.nStreamSize;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;

    return decompress(&state, pPdStruct);
}

bool XDecompress::checkCRC(const QMap<XBinary::FPART_PROP, QVariant> &mapProperties, QIODevice *pDevice, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    XBinary::CRC_TYPE crcType = (XBinary::CRC_TYPE)mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();

    if (crcType != XBinary::CRC_TYPE_UNKNOWN) {
        QVariant varCRC;
        QVariant varCRC_calc;

        XBinary binary(pDevice);

        pDevice->reset();

        if (crcType == XBinary::CRC_TYPE_EDB88320) {
            varCRC = mapProperties.value(XBinary::FPART_PROP_CRC_VALUE, 0).toUInt();
            varCRC_calc = binary._getCRC32(0, -1, 0xFFFFFFFF, XBinary::_getCRC32Table_EDB88320(), pPdStruct);
        }

        pDevice->reset();

        if (varCRC == varCRC_calc) {
            bResult = true;
        } else {
            emit warningMessage(QString("%1").arg(tr("Invalid CRC")));
        }
    } else {
        emit errorMessage(QString("%1").arg(tr("Unknown CRC type")));
    }

    return bResult;
}

bool XDecompress::decompress(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    const qint32 N_BUFFER_SIZE = 0x4000;

    char bufferIn[N_BUFFER_SIZE];
    char bufferOut[N_BUFFER_SIZE];

    if (pState->pDeviceInput) {
        pState->pDeviceInput->seek(pState->nInputOffset);
    }

    if (pState->pDeviceOutput) {
        pState->pDeviceOutput->seek(0);
    }

    XBinary::COMPRESS_METHOD compressMethod =
        (XBinary::COMPRESS_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_STORE).toUInt();
    qint64 nUncompressedSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();
    qint64 nWindowSize = pState->mapProperties.value(XBinary::FPART_PROP_WINDOWSIZE, 0).toLongLong();

    // state.compressMethod = (XBinary::COMPRESS_METHOD)fpart.mapProperties.value(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_UNKNOWN).toUInt();
    // state.nUncompressedSize = fpart.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();

    if (compressMethod == XBinary::COMPRESS_METHOD_STORE) {
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
        bResult = XBZIP2Decoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_LZMA) {
        bResult = XLZMADecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_PPMD) {
        QByteArray baProperty = pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES).toByteArray();
        bResult = XPPMdDecoder::decompressPPMdH(pState, baProperty, pPdStruct);
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
    } else if (compressMethod == XBinary::COMPRESS_METHOD_LZH5) {
        bResult = XLZHDecoder::decompress(pState, 5, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_LZH6) {
        bResult = XLZHDecoder::decompress(pState, 6, pPdStruct);
    } else if (compressMethod == XBinary::COMPRESS_METHOD_LZH7) {
        bResult = XLZHDecoder::decompress(pState, 7, pPdStruct);
    } else if ((compressMethod == XBinary::COMPRESS_METHOD_RAR_15) || (compressMethod == XBinary::COMPRESS_METHOD_RAR_20) ||
               (compressMethod == XBinary::COMPRESS_METHOD_RAR_29) || (compressMethod == XBinary::COMPRESS_METHOD_RAR_50) ||
               (compressMethod == XBinary::COMPRESS_METHOD_RAR_70)) {
        bool bIsSolid = false;
        rar_Unpack rarUnpack;
        rarUnpack.setDevices(pState->pDeviceInput, pState->pDeviceOutput);
        qint32 nInit = rarUnpack.Init(nWindowSize, bIsSolid);

        if (nInit > 0) {
            rarUnpack.SetDestSize(nUncompressedSize);

            if (compressMethod == XBinary::COMPRESS_METHOD_RAR_15) {
                rarUnpack.Unpack15(bIsSolid, pPdStruct);
            } else if (compressMethod == XBinary::COMPRESS_METHOD_RAR_20) {
                rarUnpack.Unpack20(bIsSolid, pPdStruct);
            } else if (compressMethod == XBinary::COMPRESS_METHOD_RAR_29) {
                rarUnpack.Unpack29(bIsSolid, pPdStruct);
            } else if ((compressMethod == XBinary::COMPRESS_METHOD_RAR_50) || (compressMethod == XBinary::COMPRESS_METHOD_RAR_70)) {
                rarUnpack.Unpack5(bIsSolid, pPdStruct);
            }

            bResult = true;
        }
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
            XBinary::DATAPROCESS_STATE state = {};
            state.pDeviceInput = pDevice;
            state.pDeviceOutput = &buffer;
            state.nInputOffset = nOffset;
            state.nInputLimit = nSize;
            state.nProcessedOffset = 0;
            state.nProcessedLimit = -1;
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
        XBinary::DATAPROCESS_STATE state = {};
        state.pDeviceInput = pDevice;
        state.pDeviceOutput = nullptr;
        state.nInputOffset = nOffset;
        state.nInputLimit = nSize;
        state.nProcessedOffset = 0;
        state.nProcessedLimit = -1;
        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, compressMethod);

        decompress(&state, pPdStruct);

        nResult = state.nCountInput;
    }

    return nResult;
}

void XDecompress::process()
{
}
