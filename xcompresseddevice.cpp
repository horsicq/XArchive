/* Copyright (c) 2024-2025 hors<horsicq@gmail.com>
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

XCompressedDevice::XCompressedDevice(QObject *pParent) : XIODevice(pParent)
{
    g_pOrigDevice = nullptr;
    g_pSubDevice = nullptr;
    g_pCurrentDevice = nullptr;
    g_pBufferDevice = nullptr;
    g_pTempFile = nullptr;
    g_bIsValid = false;
    g_nBufferSize = 2 * 1024 * 1024; // 2 MB buffer size
    g_pBuffer = nullptr;
}

XCompressedDevice::~XCompressedDevice()
{
    if (g_pSubDevice) {
        g_pSubDevice->close();
        delete g_pSubDevice;
    }

    if (g_pBufferDevice) {
        g_pBufferDevice->close();
        delete g_pBufferDevice;
    }

    if (g_pTempFile) {
        g_pTempFile->close();
        delete g_pTempFile;
    }

    if (g_pBuffer) {
        delete g_pBuffer;
        g_pBuffer = nullptr;
    }
}

bool XCompressedDevice::setData(QIODevice *pDevice, const XBinary::FPART &fPart, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    g_pOrigDevice = pDevice;

    if (fPart.mapProperties.value(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_STORE) != XBinary::COMPRESS_METHOD_STORE) {
        qint64 nUncompressedSize = fPart.mapProperties.value(XBinary::FPART_PROP_COMPRESSEDSIZE, 0).toLongLong();

        if (nUncompressedSize <= g_nBufferSize) {
            g_pBufferDevice = new QBuffer;
            g_pBuffer = new char[nUncompressedSize];

            if (g_pBuffer) {
                g_pBufferDevice->setData(g_pBuffer, nUncompressedSize);

                if (g_pBufferDevice->open(QIODevice::ReadWrite)) {
                    g_pBufferDevice->setProperty("Memory", true);
                    g_pCurrentDevice = g_pBufferDevice;
                    bResult = XDecompress().decompressFPART(fPart, pDevice, g_pBufferDevice, 0, -1, pPdStruct);
                }
            }
        } else {
            g_pTempFile = new QTemporaryFile;

            if (g_pTempFile->open()) {
                g_pCurrentDevice = g_pTempFile;
                bResult = XDecompress().decompressFPART(fPart, pDevice, g_pTempFile, 0, -1, pPdStruct);
            }
        }
    } else {
        if ((fPart.nFileOffset == 0) && (pDevice->size() == fPart.nFileSize)) {
            g_pCurrentDevice = g_pOrigDevice;
            bResult = true;
        } else {
            g_pSubDevice = new SubDevice(g_pOrigDevice, fPart.nFileOffset, fPart.nFileSize);
            if (g_pSubDevice->open(QIODevice::ReadOnly)) {
                g_pCurrentDevice = g_pSubDevice;
                bResult = true;
            }
        }
    }

    g_bIsValid = bResult;

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

QIODevice *XCompressedDevice::getOrigDevice()
{
    return g_pOrigDevice;
}

qint64 XCompressedDevice::size() const
{
    qint64 nResult = 0;

    if (g_pCurrentDevice) {
        nResult = g_pCurrentDevice->size();
    }

    return nResult;
}

bool XCompressedDevice::seek(qint64 nPos)
{
    bool bResult = false;

    if (g_pCurrentDevice) {
        bResult = g_pCurrentDevice->seek(nPos) && XIODevice::seek(nPos);
    }

    return bResult;
}

qint64 XCompressedDevice::pos() const
{
    qint64 nResult = 0;

    if (g_pCurrentDevice) {
        nResult = g_pCurrentDevice->pos();
    }

    return nResult;
}

qint64 XCompressedDevice::readData(char *pData, qint64 nMaxSize)
{
    qint64 nResult = 0;

    if (g_pCurrentDevice->seek(pos())) {
        nResult = g_pCurrentDevice->read(pData, nMaxSize);
    }

    return nResult;
}

qint64 XCompressedDevice::writeData(const char *pData, qint64 nMaxSize)
{
    Q_UNUSED(pData)
    Q_UNUSED(nMaxSize)
#ifdef QT_DEBUG
    qDebug("XCompressedDevice::writeData: seekpos %lld", pos());
#endif
    return 0;
}
