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
#include "xtar_zstd.h"

XTAR_ZSTD::XTAR_ZSTD(QIODevice *pDevice) : XTARCOMPRESSED(pDevice)
{
    m_compressionType = COMPRESSION_ZSTD;
}

XTAR_ZSTD::~XTAR_ZSTD()
{
}

bool XTAR_ZSTD::isValid(PDSTRUCT *pPdStruct)
{
    return isValid(getDevice());
}

bool XTAR_ZSTD::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
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

        // Zstd magic: 0x28 0xB5 0x2F 0xFD
        bResult = (nByte0 == 0x28) && (nByte1 == 0xB5) && (nByte2 == 0x2F) && (nByte3 == 0xFD);
    }

    pDevice->seek(nOffset);

    return bResult;
}

XBinary::FT XTAR_ZSTD::getFileType()
{
    return FT_TAR_ZSTD;
}

QString XTAR_ZSTD::getFileFormatExt()
{
    return "tar.zst";
}

QString XTAR_ZSTD::getFileFormatExtsString()
{
    return "*.tar.zst;*.tzst";
}

QString XTAR_ZSTD::getMIMEString()
{
    return "application/zstd";
}

QIODevice *XTAR_ZSTD::decompressData(PDSTRUCT *pPdStruct)
{
    return decompressByMethod(HANDLE_METHOD_ZSTD, 0, -1, pPdStruct);
}

QList<QString> XTAR_ZSTD::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("28B52FFD");

    return listResult;
}

XBinary *XTAR_ZSTD::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XTAR_ZSTD(pDevice);
}
