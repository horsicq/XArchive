/* Copyright (c) 2025 hors<horsicq@gmail.com>
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
#include "xudf.h"
#include "Algos/xstoredecoder.h"

XBinary::XCONVERT _TABLE_XUDF_STRUCTID[] = {{XUDF::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                            {XUDF::STRUCTID_TAG, "TAG", QString("Tag")},
                                            {XUDF::STRUCTID_ANCHOR_VOLUME_DESCRIPTOR, "ANCHOR_VOLUME_DESCRIPTOR", QString("Anchor Volume Descriptor")},
                                            {XUDF::STRUCTID_PRIMARY_VOLUME_DESCRIPTOR, "PRIMARY_VOLUME_DESCRIPTOR", QString("Primary Volume Descriptor")},
                                            {XUDF::STRUCTID_FILE_ENTRY, "FILE_ENTRY", QString("File Entry")}};

XUDF::XUDF(QIODevice *pDevice) : XArchive(pDevice)
{
    m_sVolumeIdentifier = getVolumeIdentifier();
    m_sVolumeSetIdentifier = getVolumeSetIdentifier();
}

XUDF::~XUDF()
{
}

bool XUDF::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() >= 0x8000) {
        _MEMORY_MAP memoryMap = XBinary::getSimpleMemoryMap();

        // UDF Anchor Volume Descriptor Pointer is typically at sector 256 (offset 0x20000)
        // Check for tag identifier 2 (AVDP) with NSR descriptor
        qint64 nAnchorOffset = 256 * 2048;  // Sector 256 with 2048 byte blocks

        if (nAnchorOffset < getSize()) {
            quint16 nTagIdentifier = read_uint16(nAnchorOffset);
            if (nTagIdentifier == TAG_ANCHOR_VOLUME_DESCRIPTOR_POINTER) {
                bResult = true;
            }
        }

        // Also check at end of volume (last sector)
        if (!bResult) {
            qint64 nEndOffset = (getSize() / 2048) * 2048 - 2048;
            if (nEndOffset > 0) {
                quint16 nTagIdentifier = read_uint16(nEndOffset);
                if (nTagIdentifier == TAG_ANCHOR_VOLUME_DESCRIPTOR_POINTER) {
                    bResult = true;
                }
            }
        }
    }

    Q_UNUSED(pPdStruct)

    return bResult;
}

bool XUDF::isValid(QIODevice *pDevice)
{
    XUDF xudf(pDevice);
    return xudf.isValid();
}

QString XUDF::getFileFormatExt()
{
    return "udf";
}

QString XUDF::getFileFormatExtsString()
{
    return "UDF files (*.udf *.iso *.img)";
}

qint64 XUDF::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

QString XUDF::getMIMEString()
{
    return "application/x-udf";
}

XBinary::FT XUDF::getFileType()
{
    return FT_UDF;
}

QList<XBinary::MAPMODE> XUDF::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XUDF::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XUDF::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XUDF_STRUCTID, sizeof(_TABLE_XUDF_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XUDF::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

        _dataHeadersOptions.nID = STRUCTID_ANCHOR_VOLUME_DESCRIPTOR;
        _dataHeadersOptions.nLocation = _getAnchorVolumeDescriptorOffset();
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else if (dataHeadersOptions.nID == STRUCTID_ANCHOR_VOLUME_DESCRIPTOR) {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XUDF::structIDToString(dataHeadersOptions.nID));
            dataHeader.nSize = 512;

            dataHeader.listRecords.append(getDataRecord(0, 16, "Tag", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
            dataHeader.listRecords.append(getDataRecord(16, 8, "Main Volume Descriptor Sequence Extent", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
            dataHeader.listRecords.append(getDataRecord(24, 8, "Reserve Volume Descriptor Sequence Extent", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

            listResult.append(dataHeader);
        }
    }

    return listResult;
}

QList<XBinary::FPART> XUDF::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    QList<FPART> listResult;

    if (nFileParts & FILEPART_HEADER) {
        qint64 nAnchorOffset = _getAnchorVolumeDescriptorOffset();
        
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = nAnchorOffset;
        record.nFileSize = 512;
        record.nVirtualAddress = -1;
        record.sName = tr("Anchor Volume Descriptor");

        listResult.append(record);
    }

    if (nFileParts & FILEPART_OVERLAY) {
        qint64 nDataEnd = getSize();
        qint64 nFormatSize = getFileFormatSize(pPdStruct);
        
        if (nFormatSize < nDataEnd) {
            FPART record = {};
            record.filePart = FILEPART_OVERLAY;
            record.nFileOffset = nFormatSize;
            record.nFileSize = nDataEnd - nFormatSize;
            record.nVirtualAddress = -1;
            record.sName = tr("Overlay");

            listResult.append(record);
        }
    }

    return listResult;
}

bool XUDF::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)

    bool bResult = false;

    if (pState) {
        UDF_UNPACK_CONTEXT *pContext = new UDF_UNPACK_CONTEXT;
        pContext->nBlockSize = _getBlockSize();
        pContext->listRecords = _parseFileSystem(pContext->nBlockSize, pPdStruct);
        pContext->nCurrentRecordIndex = 0;

        pState->pContext = pContext;
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = pContext->listRecords.count();

        bResult = true;
    }

    return bResult;
}

XBinary::ARCHIVERECORD XUDF::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    ARCHIVERECORD result = {};

    if (pState && pState->pContext) {
        UDF_UNPACK_CONTEXT *pContext = (UDF_UNPACK_CONTEXT *)pState->pContext;

        if (pContext->nCurrentRecordIndex < pContext->listRecords.count()) {
            result = pContext->listRecords.at(pContext->nCurrentRecordIndex);
        }
    }

    return result;
}

bool XUDF::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pState && pState->pContext && pDevice) {
        UDF_UNPACK_CONTEXT *pContext = (UDF_UNPACK_CONTEXT *)pState->pContext;

        if (pContext->nCurrentRecordIndex < pContext->listRecords.count()) {
            ARCHIVERECORD ar = pContext->listRecords.at(pContext->nCurrentRecordIndex);

            if (!ar.mapProperties.value(FPART_PROP_ISFOLDER).toBool()) {
                XBinary::DATAPROCESS_STATE decompressState = {};
                decompressState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, XArchive::COMPRESS_METHOD_STORE);
                decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, ar.nStreamSize);
                decompressState.pDeviceInput = getDevice();
                decompressState.pDeviceOutput = pDevice;
                decompressState.nInputOffset = ar.nStreamOffset;
                decompressState.nInputLimit = ar.nStreamSize;
                decompressState.nProcessedOffset = 0;
                decompressState.nProcessedLimit = -1;

                bResult = XStoreDecoder::decompress(&decompressState, pPdStruct);
            } else {
                bResult = true;
            }
        }
    }

    return bResult;
}

bool XUDF::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pState->pContext) {
        UDF_UNPACK_CONTEXT *pContext = (UDF_UNPACK_CONTEXT *)pState->pContext;

        pContext->nCurrentRecordIndex++;
        pState->nCurrentIndex++;

        bResult = (pContext->nCurrentRecordIndex < pContext->listRecords.count());
    }

    return bResult;
}

bool XUDF::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pState->pContext) {
        UDF_UNPACK_CONTEXT *pContext = (UDF_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;

        bResult = true;
    }

    return bResult;
}

XUDF::UDF_TAG XUDF::_readTag(qint64 nOffset)
{
    UDF_TAG tag = {};
    read_array(nOffset, (char *)&tag, sizeof(UDF_TAG));
    return tag;
}

XUDF::UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER XUDF::_readAnchorVolumeDescriptor(qint64 nOffset)
{
    UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER desc = {};
    read_array(nOffset, (char *)&desc, sizeof(UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER));
    return desc;
}

XUDF::UDF_PRIMARY_VOLUME_DESCRIPTOR XUDF::_readPrimaryVolumeDescriptor(qint64 nOffset)
{
    UDF_PRIMARY_VOLUME_DESCRIPTOR desc = {};
    read_array(nOffset, (char *)&desc, sizeof(UDF_PRIMARY_VOLUME_DESCRIPTOR));
    return desc;
}

QString XUDF::getVolumeIdentifier()
{
    QString sResult;

    qint64 nAnchorOffset = _getAnchorVolumeDescriptorOffset();
    if (nAnchorOffset != -1) {
        UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER anchor = _readAnchorVolumeDescriptor(nAnchorOffset);
        qint64 nVDSOffset = (qint64)anchor.mainVolumeDescriptorSequenceExtent.nLocation * 2048;

        if (nVDSOffset > 0 && nVDSOffset < getSize()) {
            UDF_TAG tag = _readTag(nVDSOffset);
            if (tag.nTagIdentifier == TAG_PRIMARY_VOLUME_DESCRIPTOR) {
                UDF_PRIMARY_VOLUME_DESCRIPTOR pvd = _readPrimaryVolumeDescriptor(nVDSOffset);
                sResult = QString::fromLatin1(pvd.szVolumeIdentifier, 32).trimmed();
            }
        }
    }

    return sResult;
}

QString XUDF::getVolumeSetIdentifier()
{
    QString sResult;

    qint64 nAnchorOffset = _getAnchorVolumeDescriptorOffset();
    if (nAnchorOffset != -1) {
        UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER anchor = _readAnchorVolumeDescriptor(nAnchorOffset);
        qint64 nVDSOffset = (qint64)anchor.mainVolumeDescriptorSequenceExtent.nLocation * 2048;

        if (nVDSOffset > 0 && nVDSOffset < getSize()) {
            UDF_TAG tag = _readTag(nVDSOffset);
            if (tag.nTagIdentifier == TAG_PRIMARY_VOLUME_DESCRIPTOR) {
                UDF_PRIMARY_VOLUME_DESCRIPTOR pvd = _readPrimaryVolumeDescriptor(nVDSOffset);
                sResult = QString::fromLatin1(pvd.szVolumeSetIdentifier, 128).trimmed();
            }
        }
    }

    return sResult;
}

qint32 XUDF::_getBlockSize()
{
    // UDF typically uses 2048 byte blocks
    return 2048;
}

qint64 XUDF::_getAnchorVolumeDescriptorOffset()
{
    // Anchor Volume Descriptor Pointer is at sector 256
    qint64 nOffset = 256 * 2048;

    if (nOffset < getSize()) {
        quint16 nTagIdentifier = read_uint16(nOffset);
        if (nTagIdentifier == TAG_ANCHOR_VOLUME_DESCRIPTOR_POINTER) {
            return nOffset;
        }
    }

    // Try at end of volume
    qint64 nEndOffset = (getSize() / 2048) * 2048 - 2048;
    if (nEndOffset > 0) {
        quint16 nTagIdentifier = read_uint16(nEndOffset);
        if (nTagIdentifier == TAG_ANCHOR_VOLUME_DESCRIPTOR_POINTER) {
            return nEndOffset;
        }
    }

    return -1;
}

bool XUDF::_isValidTag(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (nOffset + sizeof(UDF_TAG) > getSize()) {
        return false;
    }

    UDF_TAG tag = _readTag(nOffset);
    return (tag.nTagIdentifier > 0 && tag.nTagIdentifier < 300);
}

QList<XBinary::ARCHIVERECORD> XUDF::_parseFileSystem(qint32 nBlockSize, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nBlockSize)
    Q_UNUSED(pPdStruct)

    QList<ARCHIVERECORD> listResult;

    // Placeholder implementation - full UDF parsing is complex
    // This would need to parse the File Set Descriptor and File Entry structures
    // to enumerate all files in the file system

    return listResult;
}

QString XUDF::_cleanFileName(const QString &sFileName)
{
    QString sResult = sFileName;

    // Remove null terminators and trim whitespace
    qint32 nIndex = sResult.indexOf(QChar('\0'));
    if (nIndex != -1) {
        sResult = sResult.left(nIndex);
    }

    sResult = sResult.trimmed();

    return sResult;
}
