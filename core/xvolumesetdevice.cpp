/* Copyright (c) 2026 hors<horsicq@gmail.com>
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
#include "xvolumesetdevice.h"

#include <QFile>

#include "xbinary.h"

XVolumeSetDevice::XVolumeSetDevice(QObject *pParent) : QIODevice(pParent), m_nSize(0), m_nPosition(0)
{
}

XVolumeSetDevice::~XVolumeSetDevice()
{
}

bool XVolumeSetDevice::appendSegment(QIODevice *pDevice, qint64 nOffset, qint64 nLength)
{
    if (!pDevice || isOpen() || (nOffset < 0) || (nLength < 0)) return false;
    if (!pDevice->isOpen() || !pDevice->isReadable() || pDevice->isSequential()) return false;

    const qint64 nDeviceSize = pDevice->size();
    if ((nDeviceSize < 0) || (nOffset > nDeviceSize) || (nLength > (nDeviceSize - nOffset))) return false;
    if (nLength > (LLONG_MAX - m_nSize)) return false;

    SEGMENT segment;
    segment.pDevice = pDevice;
    segment.nOffset = nOffset;
    segment.nLength = nLength;
    segment.nStart = m_nSize;
    m_listSegments.append(segment);
    m_nSize += nLength;

    return true;
}

bool XVolumeSetDevice::appendFile(const QString &sFileName, qint64 nOffset, qint64 nLength)
{
    if (isOpen() || sFileName.isEmpty()) return false;

    QFile *pFile = new QFile(sFileName, this);
    if (!pFile->open(QIODevice::ReadOnly)) {
        delete pFile;
        return false;
    }

    const qint64 nFileSize = pFile->size();
    if (nLength < 0) {
        nLength = (nOffset <= nFileSize) ? (nFileSize - nOffset) : -1;
    }

    if ((nLength < 0) || !appendSegment(pFile, nOffset, nLength)) {
        delete pFile;
        return false;
    }

    return true;
}

qint32 XVolumeSetDevice::getNumberOfSegments() const
{
    return m_listSegments.size();
}

qint64 XVolumeSetDevice::getSegmentStart(qint32 nIndex) const
{
    if ((nIndex < 0) || (nIndex >= m_listSegments.size())) return -1;

    return m_listSegments.at(nIndex).nStart;
}

qint64 XVolumeSetDevice::getSegmentLength(qint32 nIndex) const
{
    if ((nIndex < 0) || (nIndex >= m_listSegments.size())) return -1;

    return m_listSegments.at(nIndex).nLength;
}

QIODevice *XVolumeSetDevice::getSegmentDevice(qint32 nIndex) const
{
    if ((nIndex < 0) || (nIndex >= m_listSegments.size())) return nullptr;

    return m_listSegments.at(nIndex).pDevice.data();
}

qint64 XVolumeSetDevice::toLogicalOffset(qint32 nIndex, qint64 nOffsetInSegment) const
{
    if ((nIndex < 0) || (nIndex >= m_listSegments.size()) || (nOffsetInSegment < 0)) return -1;

    const SEGMENT &segment = m_listSegments.at(nIndex);
    if (nOffsetInSegment > segment.nLength) return -1;

    return segment.nStart + nOffsetInSegment;
}

bool XVolumeSetDevice::aliases(QIODevice *pDevice) const
{
    const qint32 nNumberOfSegments = m_listSegments.size();

    for (qint32 i = 0; i < nNumberOfSegments; i++) {
        QIODevice *pSegmentDevice = m_listSegments.at(i).pDevice.data();
        if (!pSegmentDevice) return true;
        if (XBinary::devicesAlias(pSegmentDevice, pDevice)) return true;
    }

    return false;
}

bool XVolumeSetDevice::open(OpenMode mode)
{
    if (isOpen() || m_listSegments.isEmpty()) return false;
    if (mode.testFlag(QIODevice::WriteOnly) || mode.testFlag(QIODevice::Append) || mode.testFlag(QIODevice::Truncate) || mode.testFlag(QIODevice::Text)) {
        return false;
    }

    m_nPosition = 0;

    // Unbuffered: every read goes straight to readData(), so the position
    // this object keeps is the position QIODevice reports.
    return QIODevice::open(mode | QIODevice::Unbuffered);
}

qint64 XVolumeSetDevice::size() const
{
    return m_nSize;
}

bool XVolumeSetDevice::isSequential() const
{
    return false;
}

bool XVolumeSetDevice::seek(qint64 nPos)
{
    if ((nPos < 0) || (nPos > m_nSize) || !QIODevice::seek(nPos)) return false;

    m_nPosition = nPos;

    return true;
}

bool XVolumeSetDevice::atEnd() const
{
    return m_nPosition >= m_nSize;
}

qint32 XVolumeSetDevice::segmentIndexForPosition(qint64 nPosition) const
{
    const qint32 nNumberOfSegments = m_listSegments.size();
    qint32 nResult = -1;

    for (qint32 i = 0; i < nNumberOfSegments; i++) {
        const SEGMENT &segment = m_listSegments.at(i);
        if ((nPosition >= segment.nStart) && (nPosition < (segment.nStart + segment.nLength))) {
            nResult = i;
            break;
        }
    }

    return nResult;
}

qint64 XVolumeSetDevice::readData(char *pData, qint64 nMaxSize)
{
    if (!pData || (nMaxSize < 0) || (m_nPosition < 0) || (m_nPosition > m_nSize)) return -1;

    qint64 nDone = 0;

    while ((nDone < nMaxSize) && (m_nPosition < m_nSize)) {
        const qint32 nIndex = segmentIndexForPosition(m_nPosition);
        if (nIndex < 0) return -1;

        const SEGMENT &segment = m_listSegments.at(nIndex);
        QIODevice *pDevice = segment.pDevice.data();
        if (!pDevice || !pDevice->isOpen() || !pDevice->isReadable()) return -1;

        const qint64 nSegmentPosition = m_nPosition - segment.nStart;
        const qint64 nRequest = qMin(nMaxSize - nDone, segment.nLength - nSegmentPosition);
        if (nRequest <= 0) return -1;

        // A volume that shrank below its declared range is refused, never
        // read short: the set must stay the bytes it was validated against.
        const qint64 nCurrentSize = pDevice->size();
        if (nCurrentSize < (segment.nOffset + segment.nLength)) return -1;

        if (!pDevice->seek(segment.nOffset + nSegmentPosition)) return -1;

        const qint64 nRead = pDevice->read(pData + nDone, nRequest);
        if ((nRead <= 0) || (nRead > nRequest)) return -1;

        nDone += nRead;
        m_nPosition += nRead;
    }

    return nDone;
}

qint64 XVolumeSetDevice::writeData(const char *pData, qint64 nMaxSize)
{
    Q_UNUSED(pData)
    Q_UNUSED(nMaxSize)

    return -1;
}
