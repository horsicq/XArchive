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
#include "xtar_gz.h"
#include "xgzip.h"

XTAR_GZ::XTAR_GZ(QIODevice *pDevice) : XTARCOMPRESSED(pDevice)
{
    m_compressionType = COMPRESSION_GZIP;
}

XTAR_GZ::~XTAR_GZ()
{
}

bool XTAR_GZ::isValid(PDSTRUCT *pPdStruct)
{
    return isValid(getDevice());
}

bool XTAR_GZ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
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

        // Gzip magic: 0x1F 0x8B
        bResult = (nByte1 == 0x1F) && (nByte2 == 0x8B);
    }

    pDevice->seek(nOffset);

    return bResult;
}

XBinary::FT XTAR_GZ::getFileType()
{
    return FT_TAR_GZ;
}

QString XTAR_GZ::getFileFormatExt()
{
    return "tar.gz";
}

QString XTAR_GZ::getFileFormatExtsString()
{
    return "*.tar.gz;*.tgz;*.taz";
}

QString XTAR_GZ::getMIMEString()
{
    return "application/gzip";
}

bool XTAR_GZ::getOuterStreamInfo(qint64 &nOuterStreamOffset, qint64 &nOuterStreamSize, HANDLE_METHOD &handleMethod)
{
    XGzip xgzip(getDevice());
    if (!xgzip.isValid()) {
        return false;
    }
    qint64 nHeaderSize = xgzip.getHeaderSize();
    qint64 nTotalSize = getSize();
    if (nTotalSize <= (nHeaderSize + 8)) {
        return false;
    }
    nOuterStreamOffset = nHeaderSize;
    nOuterStreamSize = nTotalSize - nHeaderSize - 8;
    handleMethod = HANDLE_METHOD_DEFLATE;
    return true;
}

QIODevice *XTAR_GZ::decompressData(PDSTRUCT *pPdStruct)
{
    XGzip xgzip(getDevice());

    if (!xgzip.isValid(pPdStruct)) {
        return nullptr;
    }

    qint64 nHeaderSize = xgzip.getHeaderSize();
    qint64 nTotalSize = getSize();

    if (nTotalSize <= (nHeaderSize + 8)) {
        return nullptr;
    }

    qint64 nCompressedOffset = nHeaderSize;
    qint64 nCompressedSize = nTotalSize - nHeaderSize - 8;  // Footer: CRC32 + ISIZE

    return decompressByMethod(HANDLE_METHOD_DEFLATE, nCompressedOffset, nCompressedSize, pPdStruct);
}

