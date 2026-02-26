/* Copyright (c) 2024-2026 hors<horsicq@gmail.com>
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
    m_pOrigDevice = nullptr;
    m_pSubDevice = nullptr;
    m_bIsValid = false;
    m_pCurrentDevice = nullptr;
    m_pBufferDevice = nullptr;
}

XCompressedDevice::~XCompressedDevice()
{
    if (m_pSubDevice) {
        m_pSubDevice->close();
        delete m_pSubDevice;
    }

    XBinary::freeFileBuffer(&m_pBufferDevice);
}

bool XCompressedDevice::setData(QIODevice *pDevice, const XBinary::FPART &fPart, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    m_pOrigDevice = pDevice;

    if (fPart.mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_STORE) != XBinary::HANDLE_METHOD_STORE) {
        qint64 nUncompressedSize = fPart.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();

        m_pBufferDevice = XBinary::createFileBuffer(nUncompressedSize, pPdStruct);

        bResult = XDecompress().decompressFPART(fPart, pDevice, m_pBufferDevice, pPdStruct);
        m_pCurrentDevice = m_pBufferDevice;
    } else {
        if ((fPart.nFileOffset == 0) && (pDevice->size() == fPart.nFileSize)) {
            m_pCurrentDevice = m_pOrigDevice;
            bResult = true;
        } else {
            m_pSubDevice = new SubDevice(m_pOrigDevice, fPart.nFileOffset, fPart.nFileSize);
            if (m_pSubDevice->open(QIODevice::ReadOnly)) {
                m_pCurrentDevice = m_pSubDevice;
                bResult = true;
            }
        }
    }

    m_bIsValid = bResult;

    return bResult;
}

bool XCompressedDevice::open(OpenMode mode)
{
    bool bResult = false;

    if ((m_bIsValid) && (mode == QIODevice::ReadOnly)) {
        bResult = XIODevice::open(mode);
    }

    return bResult;
}

QIODevice *XCompressedDevice::getOrigDevice()
{
    return m_pOrigDevice;
}

qint64 XCompressedDevice::size() const
{
    qint64 nResult = 0;

    if (m_pCurrentDevice) {
        nResult = m_pCurrentDevice->size();
    }

    return nResult;
}

bool XCompressedDevice::seek(qint64 nPos)
{
    bool bResult = false;

    if (m_pCurrentDevice) {
        bResult = m_pCurrentDevice->seek(nPos) && XIODevice::seek(nPos);
    }

    return bResult;
}

qint64 XCompressedDevice::pos() const
{
    qint64 nResult = 0;

    if (m_pCurrentDevice) {
        nResult = m_pCurrentDevice->pos();
    }

    return nResult;
}

qint64 XCompressedDevice::readData(char *pData, qint64 nMaxSize)
{
    qint64 nResult = 0;

    if (m_pCurrentDevice->seek(pos())) {
        nResult = m_pCurrentDevice->read(pData, nMaxSize);
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
