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
#include "xtar_bzip2.h"

XTAR_BZIP2::XTAR_BZIP2(QIODevice *pDevice) : XTARCOMPRESSED(pDevice)
{
    m_compressionType = COMPRESSION_BZIP2;
}

XTAR_BZIP2::~XTAR_BZIP2()
{
}

bool XTAR_BZIP2::isValid(PDSTRUCT *pPdStruct)
{
    return isValid(getDevice());
}

bool XTAR_BZIP2::isValid(QIODevice *pDevice)
{
    if (!pDevice) {
        return false;
    }

    bool bResult = false;
    qint64 nOffset = pDevice->pos();
    pDevice->seek(0);

    QByteArray baMagic = pDevice->read(2);
    if (baMagic.size() == 2) {
        quint8 nByte1 = (quint8)(uchar)baMagic.at(0);
        quint8 nByte2 = (quint8)(uchar)baMagic.at(1);

        // Bzip2 magic: 0x42 0x5A ("BZ")
        bResult = (nByte1 == 0x42) && (nByte2 == 0x5A);
    }

    pDevice->seek(nOffset);

    return bResult;
}

XBinary::FT XTAR_BZIP2::getFileType()
{
    return FT_TAR_BZIP2;
}

QString XTAR_BZIP2::getFileFormatExt()
{
    return "tar.bz2";
}

QString XTAR_BZIP2::getFileFormatExtsString()
{
    return "*.tar.bz2;*.tbz;*.tbz2;*.tb2;*.tz2";
}

QString XTAR_BZIP2::getMIMEString()
{
    return "application/x-bzip2";
}

QIODevice *XTAR_BZIP2::decompressData(PDSTRUCT *pPdStruct)
{
    return decompressByMethod(HANDLE_METHOD_BZIP2, 0, -1, pPdStruct);
}

QList<QString> XTAR_BZIP2::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("'BZh'");

    return listResult;
}

XBinary *XTAR_BZIP2::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XTAR_BZIP2(pDevice);
}
