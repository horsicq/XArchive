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
#include "xpea.h"

namespace {
bool isPeaNonStreamControl(quint8 nValue)
{
    return (nValue <= 3) || ((nValue >= 0x10) && (nValue <= 0x19));
}

bool isPeaStreamControl(quint8 nValue)
{
    return isPeaNonStreamControl(nValue) || ((nValue >= 0x30) && (nValue <= 0x33)) ||
           ((nValue >= 0x41) && (nValue <= 0x4C));
}
}  // namespace

XPEA::XPEA(QIODevice *pDevice)
    : XExternalArchive(pDevice, BACKEND_PEA)
{
}

quint8 XPEA::getFormatVersion() const
{
    XPEA *pThis = const_cast<XPEA *>(this);
    return (pThis->getSize() >= 2) ? pThis->read_uint8(1) : 0;
}

quint8 XPEA::getFormatRevision() const
{
    XPEA *pThis = const_cast<XPEA *>(this);
    return (pThis->getSize() >= 3) ? pThis->read_uint8(2) : 0;
}

qint64 XPEA::getFirstStreamTriggerOffset() const
{
    XPEA *pThis = const_cast<XPEA *>(this);
    if (pThis->getSize() < PEA_ARCHIVE_HEADER_SIZE + PEA_STREAM_FIXED_SIZE) return -1;

    const bool bBigEndian = (pThis->read_uint8(8) & 0x80U) != 0;
    const quint16 nNameSize = pThis->read_uint16(PEA_ARCHIVE_HEADER_SIZE, bBigEndian);
    return (nNameSize == 0) ? (PEA_ARCHIVE_HEADER_SIZE + 2) : -1;
}

bool XPEA::isValid(PDSTRUCT *pPdStruct)
{
    const qint64 nFileSize = getSize();
    if (!isPdStructNotCanceled(pPdStruct) ||
        (nFileSize < PEA_ARCHIVE_HEADER_SIZE + PEA_STREAM_FIXED_SIZE)) {
        return false;
    }

    const QByteArray baArchiveHeader = read_array(0, PEA_ARCHIVE_HEADER_SIZE);
    if ((baArchiveHeader.size() != PEA_ARCHIVE_HEADER_SIZE) ||
        ((quint8)baArchiveHeader.at(0) != 0xEA) ||
        ((quint8)baArchiveHeader.at(1) != 1) ||
        ((quint8)baArchiveHeader.at(2) > 6) ||
        !isPeaNonStreamControl((quint8)baArchiveHeader.at(3))) {
        return false;
    }

    const qint64 nTriggerOffset = getFirstStreamTriggerOffset();
    if ((nTriggerOffset < PEA_ARCHIVE_HEADER_SIZE + 2) ||
        (nTriggerOffset > nFileSize - 8)) {
        return false;
    }

    const QByteArray baTrigger = read_array(nTriggerOffset, 4);
    const quint8 nCompression = read_uint8(nTriggerOffset + 4);
    const quint8 nStreamControl = read_uint8(nTriggerOffset + 6);
    const quint8 nObjectControl = read_uint8(nTriggerOffset + 7);

    return isPdStructNotCanceled(pPdStruct) &&
           (baTrigger == QByteArray("POD\0", 4)) && (nCompression <= 3) &&
           isPeaStreamControl(nStreamControl) && isPeaNonStreamControl(nObjectControl);
}

bool XPEA::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPEA pea(pDevice);
    return pea.isValid(pPdStruct);
}

XBinary::MODE XPEA::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XPEA::getEndian()
{
    return (read_uint8(8) & 0x80U) ? ENDIAN_BIG : ENDIAN_LITTLE;
}

QString XPEA::getFileFormatExt()
{
    return "pea";
}

QString XPEA::getFileFormatExtsString()
{
    return "PEA (*.pea)";
}

XBinary::FT XPEA::getFileType()
{
    return FT_PEA;
}

qint64 XPEA::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

QString XPEA::getMIMEString()
{
    return "application/x-pea";
}

XBinary::OSNAME XPEA::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QString XPEA::getVersion()
{
    return QString("%1.%2").arg(getFormatVersion()).arg(getFormatRevision());
}

QList<QString> XPEA::getSearchSignatures()
{
    return {"EA01................0000'POD'00"};
}

XBinary *XPEA::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPEA(pDevice);
}
