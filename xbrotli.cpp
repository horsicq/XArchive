/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xbrotli.h"
#include "Algos/xbrotlidecoder.h"
#include "xdecompress.h"

XBinary::XCONVERT _TABLE_XBrotli_STRUCTID[] = {{XBrotli::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                               {XBrotli::STRUCTID_BROTLI_STREAM, "BROTLI_STREAM", QString("Brotli stream")}};

XBrotli::XBrotli(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBrotli::~XBrotli()
{
}

bool XBrotli::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    // Brotli has no magic bytes. Validate by attempting a trial decompression.
    qint64 nFileSize = getSize();

    if (nFileSize >= 1) {
        SubDevice sd(getDevice(), 0, nFileSize);

        if (sd.open(QIODevice::ReadOnly)) {
            QBuffer buffer;
            if (buffer.open(QIODevice::WriteOnly)) {
                XBinary::DATAPROCESS_STATE decompressState = {};
                decompressState.pDeviceInput = &sd;
                decompressState.pDeviceOutput = &buffer;
                decompressState.nInputOffset = 0;
                decompressState.nInputLimit = nFileSize;

                bResult = XBrotliDecoder::decompress(&decompressState, pPdStruct);

                buffer.close();
            }

            sd.close();
        }
    }

    return bResult;
}

XBinary::MODE XBrotli::getMode()
{
    return MODE_DATA;
}

qint32 XBrotli::getType()
{
    return TYPE_BR;
}

XBinary::ENDIAN XBrotli::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XBrotli::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_BR: sResult = QString("BR"); break;
    }

    return sResult;
}

QString XBrotli::getFileFormatExt()
{
    return "br";
}

XBinary::FT XBrotli::getFileType()
{
    return FT_BROTLI;
}

QString XBrotli::getFileFormatExtsString()
{
    return "br";
}

qint64 XBrotli::getFileFormatSize(XBinary::PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QString XBrotli::getMIMEString()
{
    return "application/x-brotli";
}

XBinary::OSNAME XBrotli::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QList<XBinary::MAPMODE> XBrotli::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XBrotli::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_DATA;
    }

    if (mapMode == MAPMODE_REGIONS) {
        result = _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    } else if (mapMode == MAPMODE_STREAMS) {
        result = _getMemoryMap(FILEPART_STREAM, pPdStruct);
    } else if (mapMode == MAPMODE_DATA) {
        result = _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
    }

    return result;
}

QString XBrotli::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XBrotli_STRUCTID, sizeof(_TABLE_XBrotli_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XBrotli::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XBrotli_STRUCTID, sizeof(_TABLE_XBrotli_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XBrotli::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XBrotli_STRUCTID, sizeof(_TABLE_XBrotli_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XBrotli::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<XBinary::DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
        _dataHeadersOptions.nID = STRUCTID_BROTLI_STREAM;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;

        if (isPdStructNotCanceled(pPdStruct)) {
            listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
        }
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_BROTLI_STREAM) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XBrotli::structIDToString(dataHeadersOptions.nID));
                dataHeader.nSize = getSize();

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XBrotli::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)
    QList<FPART> listResult;

    const qint64 nFileSize = getSize();
    if (nFileSize <= 0) return listResult;

    qint64 nMaxOffset = 0;

    {
        SubDevice sd(getDevice(), 0, nFileSize);

        if (sd.open(QIODevice::ReadOnly)) {
            QBuffer buffer;
            if (buffer.open(QIODevice::WriteOnly)) {
                XBinary::DATAPROCESS_STATE decompressState = {};
                decompressState.pDeviceInput = &sd;
                decompressState.pDeviceOutput = &buffer;
                decompressState.nInputOffset = 0;
                decompressState.nInputLimit = nFileSize;

                if (XBrotliDecoder::decompress(&decompressState, pPdStruct)) {
                    nMaxOffset = decompressState.nCountInput;

                    if (nFileParts & FILEPART_STREAM) {
                        FPART region = {};
                        region.filePart = FILEPART_STREAM;
                        region.nFileOffset = 0;
                        region.nFileSize = nMaxOffset;
                        region.nVirtualAddress = -1;
                        region.sName = tr("Stream");
                        region.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_BROTLI);
                        region.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, decompressState.nCountOutput);
                        region.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, decompressState.nCountInput);

                        listResult.append(region);
                    }
                }

                buffer.close();
            }

            sd.close();
        }
    }

    // Data: entire file
    if (nFileParts & FILEPART_DATA) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = 0;
        data.nFileSize = nMaxOffset;
        data.nVirtualAddress = -1;
        data.sName = tr("Data");
        listResult.append(data);
    }

    // Overlay: trailing bytes
    if (nFileParts & FILEPART_OVERLAY) {
        if (nMaxOffset < nFileSize) {
            FPART ov = {};
            ov.filePart = FILEPART_OVERLAY;
            ov.nFileOffset = nMaxOffset;
            ov.nFileSize = nFileSize - nMaxOffset;
            ov.nVirtualAddress = -1;
            ov.sName = tr("Overlay");
            listResult.append(ov);
        }
    }

    return listResult;
}

bool XBrotli::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)

    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        if (!isValid(pPdStruct)) {
            return false;
        }

        BROTLI_UNPACK_CONTEXT *pContext = new BROTLI_UNPACK_CONTEXT;
        pContext->sFileName = XBinary::getDeviceFileBaseName(getDevice());

        qint64 nFileSize = getSize();
        SubDevice sd(getDevice(), 0, nFileSize);

        if (sd.open(QIODevice::ReadOnly)) {
            QBuffer buffer;
            if (buffer.open(QIODevice::WriteOnly)) {
                XBinary::DATAPROCESS_STATE decompressState = {};
                decompressState.pDeviceInput = &sd;
                decompressState.pDeviceOutput = &buffer;
                decompressState.nInputOffset = 0;
                decompressState.nInputLimit = nFileSize;

                if (XBrotliDecoder::decompress(&decompressState, pPdStruct)) {
                    pContext->nCompressedSize = decompressState.nCountInput;
                    pContext->nUncompressedSize = decompressState.nCountOutput;
                } else {
                    pContext->nCompressedSize = nFileSize;
                    pContext->nUncompressedSize = 0;
                }

                buffer.close();
            }

            sd.close();
        }

        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 1;
        pState->pContext = pContext;

        bResult = true;
    }

    return bResult;
}

XBinary::ARCHIVERECORD XBrotli::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext) {
        return result;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return result;
    }

    BROTLI_UNPACK_CONTEXT *pContext = (BROTLI_UNPACK_CONTEXT *)pState->pContext;

    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nCompressedSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_BROTLI);

    return result;
}

bool XBrotli::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (!pState || !pState->pContext) {
        return false;
    }

    pState->nCurrentIndex++;

    bResult = false;

    return bResult;
}

bool XBrotli::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if (pState->pContext) {
        BROTLI_UNPACK_CONTEXT *pContext = (BROTLI_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
    }

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;

    return true;
}

QList<XBinary::FPART_PROP> XBrotli::getAvailableFPARTProperties()
{
    QList<XBinary::FPART_PROP> listResult;

    listResult.append(FPART_PROP_ORIGINALNAME);
    listResult.append(FPART_PROP_COMPRESSEDSIZE);
    listResult.append(FPART_PROP_UNCOMPRESSEDSIZE);
    listResult.append(FPART_PROP_HANDLEMETHOD);
    listResult.append(FPART_PROP_STREAMOFFSET);
    listResult.append(FPART_PROP_STREAMSIZE);

    return listResult;
}

QList<QString> XBrotli::getSearchSignatures()
{
    QList<QString> listResult;

    return listResult;
}

XBinary *XBrotli::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XBrotli(pDevice);
}
