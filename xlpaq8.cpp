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
#include "xlpaq8.h"

XLPAQ8::XLPAQ8(QIODevice *pDevice) : XExternalArchive(pDevice, BACKEND_LPAQ8)
{
}

quint32 XLPAQ8::getUncompressedSize() const
{
    XLPAQ8 *pThis = const_cast<XLPAQ8 *>(this);
    return (pThis->getSize() >= 8) ? pThis->read_uint32(4, true) : 0;
}

qint32 XLPAQ8::getMemoryLevel() const
{
    XLPAQ8 *pThis = const_cast<XLPAQ8 *>(this);
    if (pThis->getSize() < 4) return -1;

    const quint8 nOption = pThis->read_uint8(3);
    return ((nOption >= '0') && (nOption <= '9')) ? (nOption - '0') : -1;
}

qint32 XLPAQ8::getDataMode() const
{
    XLPAQ8 *pThis = const_cast<XLPAQ8 *>(this);
    return (pThis->getSize() >= LPAQ8_HEADER_SIZE) ? pThis->read_uint8(8) : -1;
}

bool XLPAQ8::isValid(PDSTRUCT *pPdStruct)
{
    if (!isPdStructNotCanceled(pPdStruct) || (getSize() <= LPAQ8_HEADER_SIZE)) return false;

    const QByteArray baHeader = read_array(0, LPAQ8_HEADER_SIZE);
    if ((baHeader.size() != LPAQ8_HEADER_SIZE) || (baHeader.at(0) != 'p') || (baHeader.at(1) != 'Q') || ((quint8)baHeader.at(2) != 8)) {
        return false;
    }

    const quint8 nOption = (quint8)baHeader.at(3);
    const quint32 nUncompressedSize = getUncompressedSize();
    const quint8 nDataMode = (quint8)baHeader.at(8);

    return isPdStructNotCanceled(pPdStruct) && (nOption >= '0') && (nOption <= '9') && (nUncompressedSize <= 0x7FFFFFFFU) && (nDataMode <= 2);
}

bool XLPAQ8::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLPAQ8 lpaq8(pDevice);
    return lpaq8.isValid(pPdStruct);
}

XBinary::MODE XLPAQ8::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XLPAQ8::getEndian()
{
    return ENDIAN_BIG;
}

QString XLPAQ8::getFileFormatExt()
{
    return "lpaq8";
}

QString XLPAQ8::getFileFormatExtsString()
{
    return "LPAQ8 (*.lpaq8 *.lpq)";
}

XBinary::FT XLPAQ8::getFileType()
{
    return FT_LPAQ8;
}

qint64 XLPAQ8::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

QString XLPAQ8::getMIMEString()
{
    return "application/x-lpaq8";
}

XBinary::OSNAME XLPAQ8::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QString XLPAQ8::getVersion()
{
    return "8";
}

QList<QString> XLPAQ8::getSearchSignatures()
{
    return {"'pQ'08"};
}

XBinary *XLPAQ8::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XLPAQ8(pDevice);
}
