/* Copyright (c) 2024 hors<horsicq@gmail.com>
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
#include "xcompresseddevice.h"

XCompressedDevice::XCompressedDevice(QObject *parent) : XIODevice(parent)
{
    g_pSubDevice = nullptr;
    g_pOrigDevice = nullptr;
    g_fileType = XBinary::FT_UNKNOWN;
    g_bIsValid = false;
    g_nLayerSize = 0;
    g_nLayerOffset = 0;
    g_compressMethod = XArchive::COMPRESS_METHOD_UNKNOWN;
}

XCompressedDevice::~XCompressedDevice()
{
    if (g_pSubDevice) {
        g_pSubDevice->close();
        delete g_pSubDevice;
    }
}

bool XCompressedDevice::setData(QIODevice *pDevice, XBinary::FT fileType)
{
    bool bResult = false;

    g_pOrigDevice = pDevice;
    g_fileType = fileType;

    if (fileType == XBinary::FT_GZIP) {
        XGzip xgzip(g_pOrigDevice);

        if (xgzip.isValid()) {
            XBinary::PDSTRUCT pdStruct = XBinary::createPdStruct();
            QList<XArchive::RECORD> listRecords = xgzip.getRecords(1, &pdStruct);

            if (listRecords.count()) {
                XArchive::RECORD record = listRecords.at(0);

                if (g_pSubDevice) {
                    g_pSubDevice->close();
                    delete g_pSubDevice;
                }

                g_pSubDevice = new SubDevice(g_pOrigDevice, record.nDataOffset, record.nCompressedSize);

                if (g_pSubDevice->open(QIODevice::ReadOnly)) {
                    setSize(record.nUncompressedSize);
                    setLayerSize(record.nUncompressedSize);
                    setLayerOffset(record.nDataOffset);
                    setLayerCompressMethod(XArchive::COMPRESS_METHOD_DEFLATE);
                    g_bIsValid = true;
                }
            }
        }
    }

    bResult = g_bIsValid;

    return bResult;
}

bool XCompressedDevice::open(OpenMode mode)
{
    bool bResult = false;

    if ((g_bIsValid) && (mode == QIODevice::ReadOnly)) {
        bResult = XIODevice::open(mode);
    }

    return bResult;
}

void XCompressedDevice::setLayerSize(qint64 nLayerSize)
{
    g_nLayerSize = nLayerSize;
}

qint64 XCompressedDevice::getLayerSize()
{
    return g_nLayerSize;
}

void XCompressedDevice::setLayerOffset(qint64 nLayerOffset)
{
    g_nLayerOffset = nLayerOffset;
}

qint64 XCompressedDevice::getLayerOffset()
{
    return g_nLayerOffset;
}

void XCompressedDevice::setLayerCompressMethod(XArchive::COMPRESS_METHOD compressMethod)
{
    g_compressMethod = compressMethod;
}

XArchive::COMPRESS_METHOD XCompressedDevice::getLayerCompressMethod()
{
    return g_compressMethod;
}

QIODevice *XCompressedDevice::getOrigDevice()
{
    return g_pOrigDevice;
}

qint64 XCompressedDevice::readData(char *pData, qint64 nMaxSize)
{
    qint64 nResult = 0;
#ifdef QT_DEBUG
    qDebug("XCompressedDevice::readData: seekpos %d", (qint32)pos());
    qDebug("XCompressedDevice::readData: size %d", (qint32)nMaxSize);
#endif
    QBuffer buffer;

    if (buffer.open(QIODevice::ReadWrite)) {
        g_pSubDevice->seek(0);

        XArchive::DECOMPRESSSTRUCT decompressStruct = {};
        decompressStruct.compressMethod = g_compressMethod;
        decompressStruct.pSourceDevice = g_pSubDevice;
        decompressStruct.pDestDevice = &buffer;
        decompressStruct.nDecompressedOffset = pos();
        decompressStruct.nDecompressedSize = nMaxSize;

        XArchive::COMPRESS_RESULT compressResult = XArchive::_decompress(&decompressStruct);

        if ((compressResult == XArchive::COMPRESS_RESULT_OK) && (buffer.size() == decompressStruct.nDecompressedWrote)) {
            XBinary::_copyMemory(pData, (char *)(buffer.data().data()), decompressStruct.nDecompressedWrote);
            nResult = decompressStruct.nDecompressedWrote;
        }

        buffer.close();
    }

    return nResult;
}

qint64 XCompressedDevice::writeData(const char *pData, qint64 nMaxSize)
{
#ifdef QT_DEBUG
    qDebug("XCompressedDevice::writeData: seekpos %ll", pos());
#endif
    return 0;
}
