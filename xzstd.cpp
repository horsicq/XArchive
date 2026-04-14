/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
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
#include "xzstd.h"
#include "Algos/xzstddecoder.h"

XBinary::XCONVERT _TABLE_XZstd_STRUCTID[] = {{XZstd::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                             {XZstd::STRUCTID_ZSTD_HEADER, "ZSTD_HEADER", QString("Zstandard header")}};

XZstd::XZstd(QIODevice *pDevice) : XArchive(pDevice)
{
}

XZstd::~XZstd()
{
}

bool XZstd::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (getSize() >= 4) {
        // Zstd magic number: 0xFD2FB528 (little-endian) = bytes 28 B5 2F FD
        quint32 nMagic = read_uint32(0, false);
        bResult = (nMagic == 0xFD2FB528);
    }

    return bResult;
}

bool XZstd::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZstd xzstd(pDevice);

    return xzstd.isValid();
}

XBinary::MODE XZstd::getMode()
{
    return MODE_DATA;
}

qint32 XZstd::getType()
{
    return TYPE_ZST;
}

XBinary::ENDIAN XZstd::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XZstd::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_ZST: sResult = QString("ZST"); break;
    }

    return sResult;
}

QString XZstd::getFileFormatExt()
{
    return "zst";
}

XBinary::FT XZstd::getFileType()
{
    return FT_ZSTD;
}

QString XZstd::getFileFormatExtsString()
{
    return "zst";
}

qint64 XZstd::getFileFormatSize(XBinary::PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QString XZstd::getMIMEString()
{
    return "application/zstd";
}

XBinary::OSNAME XZstd::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QList<XBinary::MAPMODE> XZstd::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XZstd::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XZstd::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XZstd_STRUCTID, sizeof(_TABLE_XZstd_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XZstd::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XZstd_STRUCTID, sizeof(_TABLE_XZstd_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XZstd::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XZstd_STRUCTID, sizeof(_TABLE_XZstd_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XZstd::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<XBinary::DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
        _dataHeadersOptions.nID = STRUCTID_ZSTD_HEADER;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;

        if (isPdStructNotCanceled(pPdStruct)) {
            listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
        }
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_ZSTD_HEADER) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XZstd::structIDToString(dataHeadersOptions.nID));
                dataHeader.nSize = sizeof(ZSTD_HEADER);

                dataHeader.listRecords.append(getDataRecord(offsetof(ZSTD_HEADER, nMagic), 4, "nMagic", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XZstd::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)
    QList<FPART> listResult;

    const qint64 nFileSize = getSize();
    if (nFileSize <= 0) return listResult;

    // Header: fixed 4 bytes (magic number)
    if (nFileParts & FILEPART_HEADER) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = qMin<qint64>(4, nFileSize);
        header.nVirtualAddress = -1;
        header.sName = tr("Header");
        listResult.append(header);
    }

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

                if (XZstdDecoder::decompress(&decompressState, pPdStruct)) {
                    nMaxOffset = decompressState.nCountInput;

                    if (nFileParts & FILEPART_STREAM) {
                        FPART region = {};
                        region.filePart = FILEPART_STREAM;
                        region.nFileOffset = 0;
                        region.nFileSize = nMaxOffset;
                        region.nVirtualAddress = -1;
                        region.sName = tr("Stream");
                        region.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZSTD);
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

    // Overlay: any trailing bytes
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

XZstd::ZSTD_HEADER XZstd::_read_ZSTD_HEADER(qint64 nOffset)
{
    ZSTD_HEADER result = {};

    result.nMagic = read_uint32(nOffset, false);

    return result;
}

bool XZstd::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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

        ZSTD_UNPACK_CONTEXT *pContext = new ZSTD_UNPACK_CONTEXT;
        pContext->nHeaderSize = 4;  // Magic number size
        pContext->sFileName = XBinary::getDeviceFileBaseName(getDevice());

        // Decompress to get sizes
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
                decompressState.nProcessedLimit = -1;

                if (XZstdDecoder::decompress(&decompressState, pPdStruct)) {
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
        pState->nNumberOfRecords = 1;  // Single compressed stream
        pState->pContext = pContext;

        bResult = true;
    }

    return bResult;
}

XBinary::ARCHIVERECORD XZstd::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext) {
        return result;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return result;
    }

    ZSTD_UNPACK_CONTEXT *pContext = (ZSTD_UNPACK_CONTEXT *)pState->pContext;

    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nCompressedSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZSTD);

    return result;
}

bool XZstd::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (!pState || !pState->pContext || !pDevice) {
        return false;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }

    qint64 nFileSize = getSize();
    SubDevice sd(getDevice(), 0, nFileSize);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE decompressState = {};
        decompressState.pDeviceInput = &sd;
        decompressState.pDeviceOutput = pDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = nFileSize;
        decompressState.nProcessedLimit = -1;

        bResult = XZstdDecoder::decompress(&decompressState, pPdStruct);

        sd.close();
    }

    return bResult;
}

bool XZstd::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (!pState || !pState->pContext) {
        return false;
    }

    pState->nCurrentIndex++;

    // Zstd has only one record
    bResult = false;

    return bResult;
}

bool XZstd::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if (pState->pContext) {
        ZSTD_UNPACK_CONTEXT *pContext = (ZSTD_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
    }

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;

    return true;
}

QList<XBinary::FPART_PROP> XZstd::getAvailableFPARTProperties()
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

QList<QString> XZstd::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("28B52FFD");

    return listResult;
}

XBinary *XZstd::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XZstd(pDevice);
}

