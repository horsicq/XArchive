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
#include "xtar_lzma.h"

XTAR_LZMA::XTAR_LZMA(QIODevice *pDevice) : XTARCOMPRESSED(pDevice)
{
    m_compressionType = COMPRESSION_LZMA;
}

XTAR_LZMA::~XTAR_LZMA()
{
}

bool XTAR_LZMA::isValid(PDSTRUCT *pPdStruct)
{
    return isValid(getDevice());
}

bool XTAR_LZMA::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
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

        // LZMA magic: 0x5D 0x00 0x00 0x00
        bResult = (nByte0 == 0x5D) && (nByte1 == 0x00) && (nByte2 == 0x00) && (nByte3 == 0x00);
    }

    pDevice->seek(nOffset);

    return bResult;
}

XBinary::FT XTAR_LZMA::getFileType()
{
    return FT_TAR_LZMA;
}

QString XTAR_LZMA::getFileFormatExt()
{
    return "tar.lzma";
}

QString XTAR_LZMA::getFileFormatExtsString()
{
    return "*.tar.lzma;*.tlz";
}

QString XTAR_LZMA::getMIMEString()
{
    return "application/x-lzma";
}

QIODevice *XTAR_LZMA::decompressData(PDSTRUCT *pPdStruct)
{
    return decompressByMethod(HANDLE_METHOD_LZMA, 0, -1, pPdStruct);
}

QList<QString> XTAR_LZMA::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("5D000000");

    return listResult;
}

XBinary *XTAR_LZMA::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XTAR_LZMA(pDevice);
}
