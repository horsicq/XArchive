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
#include "xtar_compress.h"

XTAR_COMPRESS::XTAR_COMPRESS(QIODevice *pDevice) : XTARCOMPRESSED(pDevice)
{
    m_compressionType = COMPRESSION_COMPRESS;
}

XTAR_COMPRESS::~XTAR_COMPRESS()
{
}

bool XTAR_COMPRESS::isValid(PDSTRUCT *pPdStruct)
{
    return isValid(getDevice());
}

bool XTAR_COMPRESS::isValid(QIODevice *pDevice)
{
    if (!pDevice) {
        return false;
    }

    bool bResult = false;
    qint64 nOffset = pDevice->pos();
    pDevice->seek(0);

    QByteArray baMagic = pDevice->read(2);

    if (baMagic.size() == 2) {
        quint8 nByte0 = (quint8)(uchar)baMagic.at(0);
        quint8 nByte1 = (quint8)(uchar)baMagic.at(1);

        // Compress magic: 0x1F 0x9D
        bResult = (nByte0 == 0x1F) && (nByte1 == 0x9D);
    }

    pDevice->seek(nOffset);

    return bResult;
}

XBinary::FT XTAR_COMPRESS::getFileType()
{
    return FT_TAR_Z;
}

QString XTAR_COMPRESS::getFileFormatExt()
{
    return "tar.Z";
}

QString XTAR_COMPRESS::getFileFormatExtsString()
{
    return "*.tar.Z;*.tZ;*.taZ";
}

QString XTAR_COMPRESS::getMIMEString()
{
    return "application/x-compress";
}

QIODevice *XTAR_COMPRESS::decompressData(PDSTRUCT *pPdStruct)
{
    return decompressByMethod(HANDLE_METHOD_COMPRESS, 0, -1, pPdStruct);
}
