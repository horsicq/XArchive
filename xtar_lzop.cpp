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
#include "xtar_lzop.h"

XTAR_LZOP::XTAR_LZOP(QIODevice *pDevice) : XTARCOMPRESSED(pDevice)
{
    m_compressionType = COMPRESSION_LZOP;
}

XTAR_LZOP::~XTAR_LZOP()
{
}

bool XTAR_LZOP::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    return isValid(getDevice());
}

bool XTAR_LZOP::isValid(QIODevice *pDevice)
{
    if (!pDevice) {
        return false;
    }

    bool bResult = false;
    qint64 nOffset = pDevice->pos();
    pDevice->seek(0);

    QByteArray baMagic = pDevice->read(3);

    if (baMagic.size() == 3) {
        quint8 nByte0 = (quint8)(uchar)baMagic.at(0);
        quint8 nByte1 = (quint8)(uchar)baMagic.at(1);
        quint8 nByte2 = (quint8)(uchar)baMagic.at(2);

        // LZOP magic prefix: 89 4C 5A
        bResult = (nByte0 == 0x89) && (nByte1 == 0x4C) && (nByte2 == 0x5A);
    }

    pDevice->seek(nOffset);

    return bResult;
}

XBinary::FT XTAR_LZOP::getFileType()
{
    return FT_TAR_LZOP;
}

QString XTAR_LZOP::getFileFormatExt()
{
    return "tar.lzo";
}

QString XTAR_LZOP::getFileFormatExtsString()
{
    return "*.tar.lzo";
}

QString XTAR_LZOP::getMIMEString()
{
    return "application/x-lzop";
}

QIODevice *XTAR_LZOP::decompressData(PDSTRUCT *pPdStruct)
{
    return decompressByMethod(HANDLE_METHOD_LZOP, 0, -1, pPdStruct);
}

QList<QString> XTAR_LZOP::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("894C5A");

    return listResult;
}

XBinary *XTAR_LZOP::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XTAR_LZOP(pDevice);
}
