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

XDecompress::XDecompress(QObject *parent) : QObject(parent)
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

    return multiDecompress(&state, pPdStruct);
}

bool XDecompress::decompressArchiveRecord(const XBinary::ARCHIVERECORD &archiveRecord, QIODevice *pDeviceInput, QIODevice *pDeviceOutput, const QMap<XBinary::UNPACK_PROP, QVariant> &mapUnpackProperties, XBinary::PDSTRUCT *pPdStruct)
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

bool XDecompress::multiDecompress(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    bool bIsSolid = pState->mapProperties.value(XBinary::FPART_PROP_ISSOLID, false).toBool();

    qint32 nNumberOfMethods = 1;

    if (pState->mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD2)) {
        nNumberOfMethods = 2;
    }

    if ((nNumberOfMethods == 1) && (!bIsSolid)) {
        bResult = decompress(pState, pPdStruct);
    } else {
        QIODevice *pDeviceInput = nullptr;
        QIODevice *pDeviceOutput = nullptr;
        qint64 nIntermediateSize = 0;  // Bytes written to the intermediate buffer

        qint64 nStreamSize = pState->mapProperties.value(XBinary::FPART_PROP_STREAMSIZE, 0).toLongLong();

        for (qint32 i = nNumberOfMethods - 1; i >= 0; i--) {
            XBinary::DATAPROCESS_STATE state = *pState;

            XBinary::XBinary::FPART_PROP fpHandleMethod = XBinary::FPART_PROP_HANDLEMETHOD;
            XBinary::XBinary::FPART_PROP fpCompressProperties = XBinary::FPART_PROP_COMPRESSPROPERTIES;
            XBinary::XBinary::FPART_PROP fpCompressedSize = XBinary::FPART_PROP_COMPRESSEDSIZE;
            XBinary::XBinary::FPART_PROP fpUncompressedSize = XBinary::FPART_PROP_UNCOMPRESSEDSIZE;

            if (i == 1) {
                if (pState->mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD2)) {
                    fpHandleMethod = XBinary::FPART_PROP_HANDLEMETHOD2;
                }

                if (pState->mapProperties.contains(XBinary::FPART_PROP_COMPRESSPROPERTIES2)) {
                    fpCompressProperties = XBinary::FPART_PROP_COMPRESSPROPERTIES2;
                }

                if (pState->mapProperties.contains(XBinary::FPART_PROP_COMPRESSEDSIZE2)) {
                    fpCompressedSize = XBinary::FPART_PROP_COMPRESSEDSIZE2;
                }

                if (pState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE2)) {
                    fpUncompressedSize = XBinary::FPART_PROP_UNCOMPRESSEDSIZE2;
                }
            }

            state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, pState->mapProperties.value(fpHandleMethod));
            state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSPROPERTIES, pState->mapProperties.value(fpCompressProperties));
            state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, pState->mapProperties.value(fpCompressedSize));
            state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pState->mapProperties.value(fpUncompressedSize));

            state.pDeviceInput = nullptr;
            state.pDeviceOutput = nullptr;

            if (i == nNumberOfMethods - 1) {
                state.pDeviceInput = pState->pDeviceInput;
            } else if ((i == 0) && (!bIsSolid)) {
                state.pDeviceOutput = pState->pDeviceOutput;
            }

            if (state.pDeviceInput == nullptr) {
                state.pDeviceInput = pDeviceInput;
                // Intermediate buffer always starts at offset 0 (not the archive stream offset)
                state.nInputOffset = 0;
                state.nInputLimit = nIntermediateSize;
            }

            if (state.pDeviceOutput == nullptr) {
                pDeviceOutput = XBinary::createFileBuffer(nStreamSize, pPdStruct);
                state.pDeviceOutput = pDeviceOutput;
            }

            bResult = decompress(&state, pPdStruct);
            nIntermediateSize = state.nCountOutput;  // Track how many bytes were written

            if (pDeviceInput) {
                XBinary::freeFileBuffer(&pDeviceInput);
            }

            if (pDeviceOutput) {
                pDeviceInput = pDeviceOutput;
                pDeviceOutput = nullptr;
            }

            if (!bResult) {
                break;
            }
        }

        if (pDeviceInput) {
            XBinary::freeFileBuffer(&pDeviceInput);
        }

        if (pDeviceOutput) {
            XBinary::freeFileBuffer(&pDeviceOutput);
        }
    }

    return bResult;
}

bool XDecompress::decompress(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (pState->pDeviceInput) {
        pState->pDeviceInput->seek(pState->nInputOffset);
    }

    if (pState->pDeviceOutput) {
        pState->pDeviceOutput->seek(0);
    }

    XBinary::HANDLE_METHOD compressMethod = (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_STORE).toUInt();
    QByteArray baProperty = pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES).toByteArray();
    qint64 nUncompressedSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();
    qint64 nWindowSize = pState->mapProperties.value(XBinary::FPART_PROP_WINDOWSIZE, 0).toLongLong();

    // state.compressMethod = (XBinary::HANDLE_METHOD)fpart.mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD1, XBinary::HANDLE_METHOD_UNKNOWN).toUInt();
    // state.nUncompressedSize = fpart.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();

    if (compressMethod == XBinary::HANDLE_METHOD_STORE) {
        bResult = XStoreDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_BZIP2) {
        bResult = XBZIP2Decoder::decompress(pState, pPdStruct);
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
        // x86 BCJ inverse filter (7-Zip compatible)
        // Only transforms E8/E9 instructions where MSB of 4-byte operand is 0x00 or 0xFF
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            QByteArray baData = pState->pDeviceInput->read(pState->nInputLimit);
            pState->nCountInput = baData.size();

            qint32 nSize = baData.size();
            unsigned char *pData = reinterpret_cast<unsigned char *>(baData.data());
            qint32 nPos = 0;

            while (nPos < nSize - 4) {
                unsigned char b = pData[nPos];
                if (b == 0xE8 || b == 0xE9) {
                    unsigned char bMSB = pData[nPos + 4];
                    if (bMSB == 0x00 || bMSB == 0xFF) {
                        quint32 nSrc = (quint32)pData[nPos + 1] | ((quint32)pData[nPos + 2] << 8) | ((quint32)pData[nPos + 3] << 16) | ((quint32)bMSB << 24);
                        quint32 nDest = nSrc - (quint32)(nPos + 5);
                        pData[nPos + 1] = (unsigned char)(nDest);
                        pData[nPos + 2] = (unsigned char)(nDest >> 8);
                        pData[nPos + 3] = (unsigned char)(nDest >> 16);
                        pData[nPos + 4] = (unsigned char)(nDest >> 24);
                        nPos += 5;
                    } else {
                        nPos++;
                    }
                } else {
                    nPos++;
                }
            }

            qint64 nWritten = pState->pDeviceOutput->write(baData);
            pState->nCountOutput = nWritten;
            bResult = (nWritten == (qint64)baData.size());
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
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZH5) {
        bResult = XLZHDecoder::decompress(pState, 5, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZH6) {
        bResult = XLZHDecoder::decompress(pState, 6, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZH7) {
        bResult = XLZHDecoder::decompress(pState, 7, pPdStruct);
    } else if ((compressMethod == XBinary::HANDLE_METHOD_RAR_15) || (compressMethod == XBinary::HANDLE_METHOD_RAR_20) ||
               (compressMethod == XBinary::HANDLE_METHOD_RAR_29) || (compressMethod == XBinary::HANDLE_METHOD_RAR_50) ||
               (compressMethod == XBinary::HANDLE_METHOD_RAR_70)) {
        bool bIsSolid = false;
        rar_Unpack rarUnpack;
        rarUnpack.setDevices(pState->pDeviceInput, pState->pDeviceOutput);
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

            bResult = true;
        }
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
    } else {
#ifdef QT_DEBUG
        qDebug() << "Unknown compression method" << XBinary::handleMethodToString(compressMethod);
#endif
        emit errorMessage(QString("%1: %2").arg(tr("Unknown compression method"), XBinary::handleMethodToString(compressMethod)));
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

            multiDecompress(&state, pPdStruct);

            buffer.close();
        }
    }

    return baResult;
}

qint64 XDecompress::getCompressedDataSize(QIODevice *pDevice, qint64 nOffset, qint64 nSize, XBinary::HANDLE_METHOD compressMethod, XBinary::PDSTRUCT *pPdStruct)
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
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

        multiDecompress(&state, pPdStruct);

        nResult = state.nCountInput;
    }

    return nResult;
}
