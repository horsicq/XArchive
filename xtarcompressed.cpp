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
#include "xtarcompressed.h"

#include "xdecompress.h"
#include "xtar_bzip2.h"
#include "xtar_compress.h"
#include "xtar_gz.h"
#include "xtar_lzip.h"
#include "xtar_lzma.h"
#include "xtar_lzop.h"
#include "xtar_xz.h"
#include "xtar_zstd.h"

#include <QBuffer>

XTARCOMPRESSED::XTARCOMPRESSED(QIODevice *pDevice) : XTAR(pDevice)
{
    m_pDecompressedData = nullptr;
    m_compressionType = COMPRESSION_UNKNOWN;
}

XTARCOMPRESSED::~XTARCOMPRESSED()
{
    if (m_pDecompressedData) {
        delete m_pDecompressedData;
        m_pDecompressedData = nullptr;
    }
}

bool XTARCOMPRESSED::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    return isValid(getDevice());
}

bool XTARCOMPRESSED::isValid(QIODevice *pDevice)
{
    if (!pDevice) {
        return false;
    }

    return (detectCompressionType(pDevice) != COMPRESSION_UNKNOWN);
}

XTARCOMPRESSED::COMPRESSION_TYPE XTARCOMPRESSED::detectCompressionType(QIODevice *pDevice)
{
    if (!pDevice) {
        return COMPRESSION_UNKNOWN;
    }

    qint64 nOffset = pDevice->pos();
    pDevice->seek(0);

    QByteArray baMagic = pDevice->read(6);
    bool bRead0 = (baMagic.size() >= 1);
    bool bRead1 = (baMagic.size() >= 2);
    bool bRead2 = (baMagic.size() >= 3);
    bool bRead3 = (baMagic.size() >= 4);
    bool bRead4 = (baMagic.size() >= 5);
    bool bRead5 = (baMagic.size() >= 6);

    quint8 nByte0 = bRead0 ? (quint8)(uchar)baMagic.at(0) : 0;
    quint8 nByte1 = bRead1 ? (quint8)(uchar)baMagic.at(1) : 0;
    quint8 nByte2 = bRead2 ? (quint8)(uchar)baMagic.at(2) : 0;
    quint8 nByte3 = bRead3 ? (quint8)(uchar)baMagic.at(3) : 0;
    quint8 nByte4 = bRead4 ? (quint8)(uchar)baMagic.at(4) : 0;
    quint8 nByte5 = bRead5 ? (quint8)(uchar)baMagic.at(5) : 0;

    COMPRESSION_TYPE result = COMPRESSION_UNKNOWN;

    if (bRead0 && bRead1) {
        if ((nByte0 == 0x1F) && (nByte1 == 0x8B)) {
            result = COMPRESSION_GZIP;
        } else if ((nByte0 == 0x42) && (nByte1 == 0x5A)) {
            result = COMPRESSION_BZIP2;
        } else if ((nByte0 == 0x1F) && (nByte1 == 0x9D)) {
            result = COMPRESSION_COMPRESS;
        } else if (bRead2 && (nByte0 == 0x89) && (nByte1 == 0x4C) && (nByte2 == 0x5A)) {
            result = COMPRESSION_LZOP;
        } else if ((nByte0 == 0x4C) && (nByte1 == 0x5A) && bRead2 && bRead3 && (nByte2 == 0x49) && (nByte3 == 0x26)) {
            result = COMPRESSION_LZIP;
        } else if (bRead3 && (nByte0 == 0x5D) && (nByte1 == 0x00) && (nByte2 == 0x00) && (nByte3 == 0x00)) {
            result = COMPRESSION_LZMA;
        } else if (bRead5 && (nByte0 == 0xFD) && (nByte1 == 0x37) && (nByte2 == 0x7A) && (nByte3 == 0x58) && (nByte4 == 0x5A) && (nByte5 == 0x00)) {
            result = COMPRESSION_XZ;
        } else if (bRead3 && (nByte0 == 0x28) && (nByte1 == 0xB5) && (nByte2 == 0x2F) && (nByte3 == 0xFD)) {
            result = COMPRESSION_ZSTD;
        }
    }

    pDevice->seek(nOffset);

    return result;
}

XArchive *XTARCOMPRESSED::getCompressionClassInstance(COMPRESSION_TYPE compressionType, QIODevice *pDevice)
{
    XArchive *pResult = nullptr;

    if (compressionType == COMPRESSION_GZIP) {
        pResult = new XTAR_GZ(pDevice);
    } else if (compressionType == COMPRESSION_BZIP2) {
        pResult = new XTAR_BZIP2(pDevice);
    } else if (compressionType == COMPRESSION_XZ) {
        pResult = new XTAR_XZ(pDevice);
    } else if (compressionType == COMPRESSION_LZMA) {
        pResult = new XTAR_LZMA(pDevice);
    } else if (compressionType == COMPRESSION_ZSTD) {
        pResult = new XTAR_ZSTD(pDevice);
    } else if (compressionType == COMPRESSION_COMPRESS) {
        pResult = new XTAR_COMPRESS(pDevice);
    } else if (compressionType == COMPRESSION_LZIP) {
        pResult = new XTAR_LZIP(pDevice);
    } else if (compressionType == COMPRESSION_LZOP) {
        pResult = new XTAR_LZOP(pDevice);
    }

    return pResult;
}

bool XTARCOMPRESSED::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (m_compressionType == COMPRESSION_UNKNOWN) {
        m_compressionType = detectCompressionType(getDevice());
    }

    if (m_compressionType == COMPRESSION_UNKNOWN) {
        return false;
    }

    m_pDecompressedData = decompressData(pPdStruct);

    if (!m_pDecompressedData) {
        return false;
    }

    pState->pContext = m_pDecompressedData;

    QIODevice *pOriginalDevice = getDevice();
    setDevice(m_pDecompressedData);

    bool bResult = XTAR::initUnpack(pState, mapProperties, pPdStruct);

    setDevice(pOriginalDevice);

    return bResult;
}

bool XTARCOMPRESSED::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    bool bResult = XTAR::finishUnpack(pState, pPdStruct);

    if (m_pDecompressedData) {
        delete m_pDecompressedData;
        m_pDecompressedData = nullptr;
        pState->pContext = nullptr;
    }

    return bResult;
}

QIODevice *XTARCOMPRESSED::decompressByMethod(HANDLE_METHOD handleMethod, qint64 nOffset, qint64 nSize, PDSTRUCT *pPdStruct)
{
    QIODevice *pDevice = getDevice();

    if (!pDevice) {
        return nullptr;
    }

    qint64 nInputSize = nSize;

    if (nInputSize == -1) {
        nInputSize = pDevice->size() - nOffset;
    }

    if ((nOffset < 0) || (nInputSize <= 0) || ((nOffset + nInputSize) > pDevice->size())) {
        return nullptr;
    }

    XDecompress decompress;
    QByteArray baData = decompress.decomressToByteArray(pDevice, nOffset, nInputSize, handleMethod, pPdStruct);

    if (baData.isEmpty()) {
        return nullptr;
    }

    return createMemoryBuffer(baData);
}

QIODevice *XTARCOMPRESSED::createMemoryBuffer(const QByteArray &baData)
{
    if (baData.isEmpty()) {
        return nullptr;
    }

    QBuffer *pBuffer = new QBuffer();
    pBuffer->setData(baData);

    if (!pBuffer->open(QIODevice::ReadOnly)) {
        delete pBuffer;
        return nullptr;
    }

    return pBuffer;
}
