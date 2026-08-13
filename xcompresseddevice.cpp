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

#include <new>

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
    clearData();
}

void XCompressedDevice::clearData()
{
    if (isOpen()) {
        XIODevice::close();
    }

    if (m_pSubDevice) {
        QPointer<SubDevice> guardedSubDevice(m_pSubDevice);
        m_pSubDevice = nullptr;
        guardedSubDevice->close();
        if (guardedSubDevice) delete guardedSubDevice.data();
    }

    QIODevice *pBufferDevice = m_pBufferDevice.data();
    m_pBufferDevice = nullptr;
    XBinary::freeFileBuffer(&pBufferDevice);
    m_pOrigDevice = nullptr;
    m_pCurrentDevice = nullptr;
    m_bIsValid = false;
}

bool XCompressedDevice::setData(QIODevice *pDevice, const XBinary::FPART &fPart, XBinary::PDSTRUCT *pPdStruct)
{
    clearData();

    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice || guardedDevice->isSequential() || !guardedDevice ||
        !guardedDevice->isReadable() || !guardedDevice ||
        (fPart.nFileOffset < 0) ||
        (fPart.nFileSize < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint64 nDeviceSize = guardedDevice->size();
    if (!guardedDevice || (nDeviceSize < 0) ||
        (fPart.nFileOffset > nDeviceSize) ||
        (fPart.nFileSize > (nDeviceSize - fPart.nFileOffset))) {
        return false;
    }

    m_pOrigDevice = guardedDevice;
    const XBinary::HANDLE_METHOD handleMethod =
        (XBinary::HANDLE_METHOD)fPart.mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD,
                                                          XBinary::HANDLE_METHOD_STORE).toUInt();

    if (handleMethod != XBinary::HANDLE_METHOD_STORE) {
        const qint64 nUncompressedSize =
            fPart.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0).toLongLong();
        if (nUncompressedSize < 0) {
            clearData();
            return false;
        }

        m_pBufferDevice = XBinary::createFileBuffer(nUncompressedSize, pPdStruct);
        QPointer<QIODevice> guardedBufferDevice(m_pBufferDevice);
        if (guardedBufferDevice && guardedDevice &&
            XDecompress().decompressFPART(fPart, guardedDevice.data(),
                                          guardedBufferDevice.data(), pPdStruct) &&
            guardedDevice && guardedBufferDevice &&
            XBinary::isPdStructNotCanceled(pPdStruct) &&
            guardedBufferDevice->seek(0) && guardedBufferDevice) {
            m_pCurrentDevice = guardedBufferDevice.data();
            m_bIsValid = true;
        }
        if (!guardedBufferDevice) m_pBufferDevice = nullptr;
    } else {
        if (!guardedDevice) {
            clearData();
            return false;
        }
        m_pSubDevice = new (std::nothrow) SubDevice(
            guardedDevice.data(), fPart.nFileOffset, fPart.nFileSize);
        QPointer<SubDevice> guardedSubDevice(m_pSubDevice);
        if (guardedSubDevice && guardedDevice &&
            guardedSubDevice->open(QIODevice::ReadOnly) && guardedSubDevice && guardedDevice &&
            guardedSubDevice->seek(0) && guardedSubDevice && guardedDevice) {
            m_pCurrentDevice = guardedSubDevice.data();
            m_bIsValid = true;
        }
        if (!guardedSubDevice) m_pSubDevice = nullptr;
    }

    if (!m_bIsValid) {
        clearData();
    }

    return m_bIsValid;
}

bool XCompressedDevice::open(OpenMode mode)
{
    bool bResult = false;

    if (m_bIsValid && m_pCurrentDevice && (mode == QIODevice::ReadOnly) &&
        m_pCurrentDevice->seek(0) && m_pCurrentDevice &&
        XIODevice::open(mode)) {
        bResult = XIODevice::seek(0);
        if (!bResult) {
            XIODevice::close();
        }
    }

    return bResult;
}

QIODevice *XCompressedDevice::getOrigDevice()
{
    return m_pOrigDevice.data();
}

qint64 XCompressedDevice::size() const
{
    qint64 nResult = 0;

    if (m_pCurrentDevice) {
        nResult = m_pCurrentDevice->size();
        if (!m_pCurrentDevice) nResult = 0;
    }

    return nResult;
}

bool XCompressedDevice::seek(qint64 nPos)
{
    bool bResult = false;

    if (m_pCurrentDevice && (nPos >= 0)) {
        const qint64 nDeviceSize = m_pCurrentDevice->size();
        if (m_pCurrentDevice && (nDeviceSize >= 0) &&
            (nPos <= nDeviceSize)) {
            bResult = m_pCurrentDevice->seek(nPos) && m_pCurrentDevice &&
                      XIODevice::seek(nPos);
        }
    }

    return bResult;
}

qint64 XCompressedDevice::pos() const
{
    return XIODevice::pos();
}

qint64 XCompressedDevice::readData(char *pData, qint64 nMaxSize)
{
    if (!m_pCurrentDevice || (nMaxSize < 0) || ((nMaxSize > 0) && !pData)) {
        return -1;
    }

    qint64 nResult = -1;
    const qint64 nPosition = XIODevice::pos();
    if (m_pCurrentDevice->seek(nPosition) && m_pCurrentDevice) {
        nResult = m_pCurrentDevice->read(pData, nMaxSize);
        if (!m_pCurrentDevice) nResult = -1;
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
    return -1;
}
