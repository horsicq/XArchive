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
#include "xtar_xz.h"

XTAR_XZ::XTAR_XZ(QIODevice *pDevice) : XTARCOMPRESSED(pDevice)
{
    m_compressionType = COMPRESSION_XZ;
}

XTAR_XZ::~XTAR_XZ()
{
}

bool XTAR_XZ::isValid(PDSTRUCT *pPdStruct)
{
    return isValid(getDevice());
}

bool XTAR_XZ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pDevice) {
        return false;
    }

    bool bResult = false;
    qint64 nOffset = pDevice->pos();
    pDevice->seek(0);

    QByteArray baMagic = pDevice->read(6);

    if (baMagic.size() == 6) {
        quint8 nByte0 = (quint8)(uchar)baMagic.at(0);
        quint8 nByte1 = (quint8)(uchar)baMagic.at(1);
        quint8 nByte2 = (quint8)(uchar)baMagic.at(2);
        quint8 nByte3 = (quint8)(uchar)baMagic.at(3);
        quint8 nByte4 = (quint8)(uchar)baMagic.at(4);
        quint8 nByte5 = (quint8)(uchar)baMagic.at(5);

        // XZ magic: 0xFD 0x37 0x7A 0x58 0x5A 0x00 (".7zXZ\0")
        bResult = (nByte0 == 0xFD) && (nByte1 == 0x37) && (nByte2 == 0x7A) && (nByte3 == 0x58) && (nByte4 == 0x5A) && (nByte5 == 0x00);
    }

    pDevice->seek(nOffset);

    return bResult;
}

XBinary::FT XTAR_XZ::getFileType()
{
    return FT_TAR_XZ;
}

QString XTAR_XZ::getFileFormatExt()
{
    return "tar.xz";
}

QString XTAR_XZ::getFileFormatExtsString()
{
    return "*.tar.xz;*.txz";
}

QString XTAR_XZ::getMIMEString()
{
    return "application/x-xz";
}

QIODevice *XTAR_XZ::decompressData(PDSTRUCT *pPdStruct)
{
    return decompressByMethod(HANDLE_METHOD_XZ, 0, -1, pPdStruct);
}

QList<QString> XTAR_XZ::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("FD377A585A00");

    return listResult;
}

XBinary *XTAR_XZ::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XTAR_XZ(pDevice);
}

