/* Copyright (c) 2017-2025 hors<horsicq@gmail.com>
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
#include "xtgz.h"

XTGZ::XTGZ(QIODevice *pDevice)
{
    m_pDevice = nullptr;
    m_pXtar = new XTAR;
    m_pCompressedDevice = new XCompressedDevice;

    if (pDevice) {
        setDevice(pDevice);
    }
}

XTGZ::~XTGZ()
{
    _closeResources();
}

void XTGZ::_closeResources()
{
    if (m_pCompressedDevice && m_pCompressedDevice->isOpen()) {
        m_pCompressedDevice->close();
    }

    if (m_pXtar) {
        delete m_pXtar;
        m_pXtar = nullptr;
    }

    if (m_pCompressedDevice) {
        delete m_pCompressedDevice;
        m_pCompressedDevice = nullptr;
    }
}

bool XTGZ::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if ((m_pDevice) && (m_pCompressedDevice) && (m_pXtar)) {
        // Check if this is a valid gzip file first
        XGzip gzip(m_pDevice);

        if (gzip.isValid(pPdStruct)) {
            // Get the header to determine where the compressed stream starts
            XGzip::GZIP_HEADER gzipHeader = gzip._read_GZIP_HEADER(0);
            qint64 nHeaderOffset = sizeof(XGzip::GZIP_HEADER);

            // Account for optional filename if present
            if (gzipHeader.nFileFlags & 8) {
                QString sFileName = gzip.read_ansiString(nHeaderOffset);
                nHeaderOffset += sFileName.size() + 1;
            }

            // Create FPART for just the compressed stream (not the gzip header/footer)
            qint64 nFileSize = gzip.getSize();
            XBinary::FPART fPart = {};
            fPart.filePart = XBinary::FILEPART_DATA;
            fPart.nFileOffset = nHeaderOffset;
            fPart.nFileSize = nFileSize - nHeaderOffset - 8;  // Subtract 8-byte footer (CRC + ISIZE)
            fPart.mapProperties[XBinary::FPART_PROP_COMPRESSMETHOD] = XBinary::COMPRESS_METHOD_DEFLATE;

            // Try to set up the compressed device
            if (m_pCompressedDevice->setData(m_pDevice, fPart, pPdStruct)) {
                if (m_pCompressedDevice->open(QIODevice::ReadOnly)) {
                    // Set TAR device to the decompressed gzip data
                    m_pXtar->setDevice(m_pCompressedDevice);

                    // Check if the decompressed data is a valid TAR archive
                    bResult = m_pXtar->isValid(pPdStruct);

                    // Seek back to beginning for further operations
                    if (bResult) {
                        m_pCompressedDevice->seek(0);
                    }
                }
            }
        }
    }

    return bResult;
}

bool XTGZ::isValid(QIODevice *pDevice)
{
    bool bResult = false;

    if (pDevice) {
        bool bNeedToClose = false;

        // Open device if not already open
        if (!pDevice->isOpen()) {
            if (pDevice->open(QIODevice::ReadOnly)) {
                bNeedToClose = true;
            } else {
                return false;
            }
        }

        XTGZ xtgz(pDevice);
        bResult = xtgz.isValid();

        // Close device if we opened it
        if (bNeedToClose) {
            pDevice->close();
        }
    }

    return bResult;
}

void XTGZ::setDevice(QIODevice *pDevice)
{
    m_pDevice = pDevice;
    XBinary::setDevice(m_pDevice);

    if ((m_pDevice) && (m_pCompressedDevice) && (m_pXtar)) {
        // Close compressed device if already open
        if (m_pCompressedDevice->isOpen()) {
            m_pCompressedDevice->close();
        }

        // Check if this is a valid gzip file
        XGzip gzip(m_pDevice);

        if (gzip.isValid()) {
            // Get the header to determine where the compressed stream starts
            XGzip::GZIP_HEADER gzipHeader = gzip._read_GZIP_HEADER(0);
            qint64 nHeaderOffset = sizeof(XGzip::GZIP_HEADER);

            // Account for optional filename if present
            if (gzipHeader.nFileFlags & 8) {
                QString sFileName = gzip.read_ansiString(nHeaderOffset);
                nHeaderOffset += sFileName.size() + 1;
            }

            // Create FPART for just the compressed stream (not the gzip header/footer)
            qint64 nFileSize = gzip.getSize();
            XBinary::FPART fPart = {};
            fPart.filePart = XBinary::FILEPART_DATA;
            fPart.nFileOffset = nHeaderOffset;
            fPart.nFileSize = nFileSize - nHeaderOffset - 8;  // Subtract 8-byte footer (CRC + ISIZE)
            fPart.mapProperties[XBinary::FPART_PROP_COMPRESSMETHOD] = XBinary::COMPRESS_METHOD_DEFLATE;

            // Set up the compressed device for decompression
            if (m_pCompressedDevice->setData(m_pDevice, fPart, nullptr)) {
                if (m_pCompressedDevice->open(QIODevice::ReadOnly)) {
                    // Set TAR device to the decompressed gzip data
                    m_pXtar->setDevice(m_pCompressedDevice);
                }
            }
        }
    }
}

bool XTGZ::initUnpack(UNPACK_STATE *pUnpackState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if ((m_pXtar) && (pUnpackState)) {
        bResult = m_pXtar->initUnpack(pUnpackState, mapProperties, pPdStruct);
    }

    return bResult;
}

bool XTGZ::unpackCurrent(UNPACK_STATE *pUnpackState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if ((m_pXtar) && (pUnpackState) && (pDevice)) {
        bResult = m_pXtar->unpackCurrent(pUnpackState, pDevice, pPdStruct);
    }

    return bResult;
}

bool XTGZ::moveToNext(UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if ((m_pXtar) && (pUnpackState)) {
        bResult = m_pXtar->moveToNext(pUnpackState, pPdStruct);
    }

    return bResult;
}

bool XTGZ::finishUnpack(UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if ((m_pXtar) && (pUnpackState)) {
        bResult = m_pXtar->finishUnpack(pUnpackState, pPdStruct);
    }

    return bResult;
}

quint64 XTGZ::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    quint64 nResult = 0;

    if (m_pXtar) {
        nResult = m_pXtar->getNumberOfRecords(pPdStruct);
    }

    return nResult;
}

QList<XArchive::RECORD> XTGZ::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> result;

    if (m_pXtar) {
        result = m_pXtar->getRecords(nLimit, pPdStruct);
    }

    return result;
}

QString XTGZ::getFileFormatExt()
{
    return "tar.gz";
}

QList<XBinary::MAPMODE> XTGZ::getMapModesList()
{
    QList<XBinary::MAPMODE> listResult;

    if (m_pXtar) {
        listResult = m_pXtar->getMapModesList();
    }

    return listResult;
}

XBinary::FT XTGZ::getFileType()
{
    return FT_TARGZ;
}

qint64 XTGZ::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    qint64 nResult = 0;

    if (m_pDevice) {
        XGzip gzip(m_pDevice);
        nResult = gzip.getFileFormatSize(pPdStruct);
    }

    return nResult;
}

QString XTGZ::getMIMEString()
{
    return "application/gzip";
}

XCompressedDevice *XTGZ::getCompressedDevice()
{
    return m_pCompressedDevice;
}
