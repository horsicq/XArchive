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
#include "xtar_lzip.h"

XTAR_LZIP::XTAR_LZIP(QIODevice *pDevice) : XTARCOMPRESSED(pDevice)
{
    m_compressionType = COMPRESSION_LZIP;
}

XTAR_LZIP::~XTAR_LZIP()
{
}

bool XTAR_LZIP::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    return isValid(getDevice());
}

bool XTAR_LZIP::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pDevice) {
        return false;
    }

    bool bResult = false;
    qint64 nOffset = pDevice->pos();
    pDevice->seek(0);

    QByteArray baMagic = pDevice->read(4);

    if (baMagic.size() == 4) {
        quint8 nByte0 = (quint8)(uchar)baMagic.at(0);
        quint8 nByte1 = (quint8)(uchar)baMagic.at(1);
        quint8 nByte2 = (quint8)(uchar)baMagic.at(2);
        quint8 nByte3 = (quint8)(uchar)baMagic.at(3);

        // Lzip magic: 4C 5A 49 50 ("LZIP")
        bResult = (nByte0 == 0x4C) && (nByte1 == 0x5A) && (nByte2 == 0x49) && (nByte3 == 0x50);
    }

    pDevice->seek(nOffset);

    return bResult;
}

XBinary::FT XTAR_LZIP::getFileType()
{
    return FT_TAR_LZIP;
}

QString XTAR_LZIP::getFileFormatExt()
{
    return "tar.lz";
}

QString XTAR_LZIP::getFileFormatExtsString()
{
    return "*.tar.lz";
}

QString XTAR_LZIP::getMIMEString()
{
    return "application/x-lzip";
}

QIODevice *XTAR_LZIP::decompressData(PDSTRUCT *pPdStruct)
{
    return decompressByMethod(HANDLE_METHOD_LZIP, 0, -1, pPdStruct);
}

QList<QString> XTAR_LZIP::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("'LZIP'");

    return listResult;
}

XBinary *XTAR_LZIP::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XTAR_LZIP(pDevice);
}
