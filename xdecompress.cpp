/* Copyright (c) 2023-2025 hors<horsicq@gmail.com>
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
#include "xdecompress.h"

qint32 XDecompress::_readDevice(char *pBuffer, qint32 nBufferSize, STATE *pState)
{
    qint32 nRead = pState->pDeviceInput->read(pBuffer, nBufferSize);

    pState->nCountInput = nRead;

    if (nRead != nBufferSize) {
        pState->bReadError = true;
    }

    return 0;
}

qint32 XDecompress::_writeDevice(char *pBuffer, qint32 nBufferSize, STATE *pState)
{
    qint64 nRealSize = 0;
    qint64 nSkip = 0;

    if (pState->nDecompressedOffset == 0 && (pState->nDecompressedLimit == -1)) {
        nRealSize = nBufferSize;
        nSkip = 0;
    }

    if ((nRealSize > 0) && (pState->pDeviceOutput)) {
        qint64 nWritten = pState->pDeviceOutput->write(pBuffer + nSkip, nRealSize);

        pState->nCountOutput += nWritten;

        if (nWritten != nRealSize) {
            pState->bWriteError = true;
        }
    } else {
        pState->nCountOutput += nBufferSize;
    }

    return nBufferSize;
}

XDecompress::XDecompress(QObject *parent)
    : XThreadObject(parent)
{
    g_mode = MODE_UNKNOWN;
    g_fileFormat = XBinary::FT_UNKNOWN;
    g_pDevice = nullptr;
    g_pPdStruct = nullptr;
}

bool XDecompress::decompressFPART(const XBinary::FPART &fpart, QIODevice *pDeviceInput, QIODevice *pDeviceOutput, qint64 nDecompressedOffset, qint64 nDecompressedLimit, XBinary::PDSTRUCT *pPdStruct)
{
    STATE state = {};
    state.compressMethod = (XBinary::COMPRESS_METHOD)fpart.mapProperties.value(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_UNKNOWN).toUInt();
    state.pDeviceInput = pDeviceInput;
    state.pDeviceOutput = pDeviceOutput;
    state.nInputOffset = fpart.nFileOffset;
    state.nInputLimit = fpart.nFileSize;
    state.nDecompressedOffset = nDecompressedOffset;
    state.nDecompressedLimit = nDecompressedLimit;

    return decompress(&state, pPdStruct);
}

bool XDecompress::decompress(STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pState->nInputOffset > 0) {
        pState->pDeviceInput->seek(pState->nInputOffset);
    }
    pState->pDeviceOutput->seek(0);

    const qint32 N_BUFFER_SIZE = 0x4000;

    char buffer[N_BUFFER_SIZE];

    if (pState->compressMethod == XBinary::COMPRESS_METHOD_STORE) {
        for (qint64 nOffset = 0; nOffset < pState->nInputLimit;) {
            qint32 nBufferSize = qMin((qint32)(pState->nInputLimit - nOffset), N_BUFFER_SIZE);

            qint32 nRead = _readDevice(buffer, nBufferSize, pState);

            if (nRead > 0) {
                _writeDevice(buffer, nRead, pState);
            } else {
                break;
            }

            if (pState->bReadError || pState->bWriteError) {
                break;
            }

            nOffset += nRead;
        }
    } else {
        emit errorMessage(tr("Unknown compression method"));
    }

    return bResult;
}

bool XDecompress::unpackDeviceToFolder(XBinary::FT fileFormat, QIODevice *pDevice, QString sFolderName, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (fileFormat == XBinary::FT_UNKNOWN) {
        fileFormat = XBinary::getPrefFileType(pDevice, true);
    }

    QList<XBinary::FPART> listParts = XFormats::getFileParts(fileFormat, pDevice, XBinary::FILEPART_STREAM, -1, false, -1, pPdStruct);

    qint32 nNumberOfParts = listParts.count();

    if (nNumberOfParts > 0) {
        if (XBinary::createDirectory(sFolderName)) {
            bResult = true;
            qint32 nGlobalIndex = XBinary::getFreeIndex(pPdStruct);
            XBinary::setPdStructInit(pPdStruct, nGlobalIndex, nNumberOfParts);

            for (qint32 i = 0; (i < nNumberOfParts) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
                XBinary::setPdStructStatus(pPdStruct, nGlobalIndex, listParts.at(i).sOriginalName);

                QString sResultFileName = sFolderName + QDir::separator() + listParts.at(i).sOriginalName;

                QFileInfo fi(sResultFileName);
                if (XBinary::createDirectory(fi.absolutePath())) {
                    QFile file;
                    file.setFileName(sResultFileName);

                    if (file.open(QIODevice::ReadWrite)) {
                        if (!decompressFPART(listParts.at(i), pDevice, &file, 0, -1, pPdStruct)) {
                            emit errorMessage(QString("%1: %2").arg(tr("Cannot decompress"), listParts.at(i).sOriginalName));
                            bResult = false;
                        }

                        file.close();
                    } else {
                        emit errorMessage(QString("%1: %2").arg(tr("Cannot create"), sResultFileName));
                        bResult = false;
                    }
                } else {
                    emit errorMessage(QString("%1: %2").arg(tr("Cannot create"), fi.absolutePath()));
                    bResult = false;
                }

                XBinary::setPdStructCurrentIncrement(pPdStruct, nGlobalIndex);
            }

            XBinary::setPdStructFinished(pPdStruct, nGlobalIndex);
        } else {
            emit errorMessage(QString("%1: %2").arg(tr("Cannot create"), sFolderName));
            bResult = false;
        }
    }

    return bResult;
}

void XDecompress::setData(MODE mode, XBinary::FT fileFormat, QIODevice *pDevice, QString sFolderName, XBinary::PDSTRUCT *pPdStruct)
{
    g_mode = mode;
    g_fileFormat = fileFormat;
    g_pDevice = pDevice;
    g_sFolderName = sFolderName;
    g_pPdStruct = pPdStruct;
}

void XDecompress::process()
{
    if (g_mode == MODE_UNPACKDEVICETOFOLDER) {
        if (g_pDevice) {
            if (!unpackDeviceToFolder(g_fileFormat, g_pDevice, g_sFolderName, g_pPdStruct)) {
                emit errorMessage(tr("Cannot unpack"));
            }
        }
    }
}
