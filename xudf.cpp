/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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

static XBinary::XCONVERT _TABLE_XUDF_STRUCTID[] = {{XUDF::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                            {XUDF::STRUCTID_TAG, "TAG", QString("Tag")},
                                            {XUDF::STRUCTID_ANCHOR_VOLUME_DESCRIPTOR, "ANCHOR_VOLUME_DESCRIPTOR", QString("Anchor Volume Descriptor")},
                                            {XUDF::STRUCTID_PRIMARY_VOLUME_DESCRIPTOR, "PRIMARY_VOLUME_DESCRIPTOR", QString("Primary Volume Descriptor")},
                                            {XUDF::STRUCTID_FILE_ENTRY, "FILE_ENTRY", QString("File Entry")}};

XUDF::XUDF(QIODevice *pDevice) : XArchive(pDevice)
{
    if (isValid()) {
        m_sVolumeIdentifier = getVolumeIdentifier();
        m_sVolumeSetIdentifier = getVolumeSetIdentifier();
    }
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

bool XUDF::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
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

QString XUDF::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XUDF_STRUCTID, sizeof(_TABLE_XUDF_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XUDF::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XUDF_STRUCTID, sizeof(_TABLE_XUDF_STRUCTID) / sizeof(XBinary::XCONVERT));
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
            dataHeader.listRecords.append(
                getDataRecord(16, 8, "Main Volume Descriptor Sequence Extent", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
            dataHeader.listRecords.append(
                getDataRecord(24, 8, "Reserve Volume Descriptor Sequence Extent", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

            listResult.append(dataHeader);
        }
    }

    return listResult;
}

QList<XBinary::FPART> XUDF::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    QList<FPART> listResult;
    qint64 nTotalSize = getSize();
    qint64 nFormatSize = getFileFormatSize(pPdStruct);

    if (nFileParts & FILEPART_HEADER) {
        qint64 nAnchorOffset = _getAnchorVolumeDescriptorOffset();

        if (nAnchorOffset != -1) {
            FPART record = {};
            record.filePart = FILEPART_HEADER;
            record.nFileOffset = nAnchorOffset;
            record.nFileSize = 512;
            record.nVirtualAddress = -1;
            record.sName = tr("Anchor Volume Descriptor");

            listResult.append(record);
        }
    }

    if (nFileParts & FILEPART_STREAM) {
        qint64 nAnchorOffset = _getAnchorVolumeDescriptorOffset();
        qint64 nStreamOffset = 0;
        qint64 nStreamSize = nTotalSize;

        if (nAnchorOffset != -1) {
            nStreamSize = nAnchorOffset;  // Data before anchor
        }

        if (nStreamSize > 0) {
            FPART record = {};
            record.filePart = FILEPART_STREAM;
            record.nFileOffset = nStreamOffset;
            record.nFileSize = nStreamSize;
            record.nVirtualAddress = -1;
            record.sName = tr("Data");

            listResult.append(record);
        }
    }

    if (nFileParts & FILEPART_OVERLAY) {
        if (nFormatSize > 0 && nFormatSize < nTotalSize) {
            FPART record = {};
            record.filePart = FILEPART_OVERLAY;
            record.nFileOffset = nFormatSize;
            record.nFileSize = nTotalSize - nFormatSize;
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
                decompressState.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, XArchive::HANDLE_METHOD_STORE);
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
    QList<ARCHIVERECORD> listResult;

    qint64 nAnchorOffset = _getAnchorVolumeDescriptorOffset();
    if (nAnchorOffset == -1) {
        return listResult;
    }

    UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER anchor = _readAnchorVolumeDescriptor(nAnchorOffset);
    qint64 nVDSOffset = (qint64)anchor.mainVolumeDescriptorSequenceExtent.nLocation * nBlockSize;
    qint64 nVDSLength = (qint64)anchor.mainVolumeDescriptorSequenceExtent.nLength;

    if (nVDSOffset <= 0 || nVDSLength <= 0 || nVDSOffset >= getSize()) {
        return listResult;
    }

    // Scan Volume Descriptor Sequence for Logical Volume Descriptor (tag id 6)
    qint64 nFSDLocation = -1;
    qint64 nCurrentVDSOffset = nVDSOffset;
    qint64 nVDSEnd = nVDSOffset + nVDSLength;

    while (nCurrentVDSOffset + (qint64)sizeof(UDF_TAG) <= nVDSEnd && isPdStructNotCanceled(pPdStruct)) {
        UDF_TAG tag = _readTag(nCurrentVDSOffset);

        if (tag.nTagIdentifier == TAG_TERMINATING_DESCRIPTOR) {
            break;
        }

        if (tag.nTagIdentifier == TAG_LOGICAL_VOLUME_DESCRIPTOR) {
            // Logical Volume Descriptor: File Set Descriptor location is at offset
            // sizeof(UDF_TAG) + 440 bytes (integrity sequence extent) within the
            // LogicalVolumeContentsUse field.
            // Layout: tag(16) + VolumeDescriptorSequenceNumber(4) + DescriptorCharacterSet(64)
            // + LogicalVolumeIdentifier(128, dstring) + LogicalBlockSize(4)
            // + DomainIdentifier(32) + LogicalVolumeContentsUse(16) <-- FSD extent here
            // + MapTableLength(4) + NumberOfPartitionMaps(4)
            // LogicalVolumeContentsUse starts at offset 16+4+64+128+4+32 = 248
            quint32 nFSDLoc = read_uint32(nCurrentVDSOffset + 248);
            // quint32 nFSDLen = read_uint32(nCurrentVDSOffset + 252);  // extent length
            if (nFSDLoc > 0) {
                nFSDLocation = (qint64)nFSDLoc * nBlockSize;
            }
        }

        nCurrentVDSOffset += nBlockSize;
    }

    if (nFSDLocation <= 0 || nFSDLocation >= getSize()) {
        return listResult;
    }

    // Read File Set Descriptor (tag id 256)
    UDF_TAG fsdTag = _readTag(nFSDLocation);
    if (fsdTag.nTagIdentifier != TAG_FILE_SET_DESCRIPTOR) {
        return listResult;
    }

    // File Set Descriptor layout:
    // tag(16) + RecordingDateAndTime(12) + InterchangeLevel(2) + MaxInterchangeLevel(2)
    // + CharacterSetList(4) + MaxCharacterSetList(4) + FileSetNumber(4) + FileSetDescriptorNumber(4)
    // + LogicalVolumeIdentifierCharacterSet(64) + LogicalVolumeIdentifier(128)
    // + FileSetCharacterSet(64) + FileSetIdentifier(32) + ReferencingTimeStamp(12)
    // + (ICB exts: 16 bytes each) ...
    // Root Directory ICB is at offset 16+12+2+2+4+4+4+4+64+128+64+32+12 = 352
    // It is a long_ad: ExtentLength(4) + ExtentLocation: LogicalBlockNumber(4) + PartitionReferenceNumber(2) + ImplementationUse(6) = 16 bytes total
    quint32 nRootICBLocation = read_uint32(nFSDLocation + 352 + 4);  // LogicalBlockNumber of root ICB
    quint16 nRootPartRef    = read_uint16(nFSDLocation + 352 + 4 + 4);  // PartitionReferenceNumber
    Q_UNUSED(nRootPartRef)

    if (nRootICBLocation == 0) {
        return listResult;
    }

    qint64 nRootFileEntryOffset = (qint64)nRootICBLocation * nBlockSize;
    if (nRootFileEntryOffset >= getSize()) {
        return listResult;
    }

    // BFS traversal of directory tree
    struct DirEntry {
        qint64 nFileEntryOffset;
        QString sPath;
    };

    QList<DirEntry> listQueue;
    QSet<qint64> setVisited;

    DirEntry rootEntry;
    rootEntry.nFileEntryOffset = nRootFileEntryOffset;
    rootEntry.sPath = QString();
    listQueue.append(rootEntry);
    setVisited.insert(nRootFileEntryOffset);

    while (!listQueue.isEmpty() && isPdStructNotCanceled(pPdStruct)) {
        DirEntry dirInfo = listQueue.takeFirst();

        UDF_TAG feTag = _readTag(dirInfo.nFileEntryOffset);
        if (feTag.nTagIdentifier != TAG_FILE_ENTRY && feTag.nTagIdentifier != TAG_EXTENDED_FILE_ENTRY) {
            continue;
        }

        // File Entry layout:
        // tag(16) + ICBTag(20) + UID(4) + GID(4) + Permissions(4) + FileLinkCount(2)
        // + RecordFormat(1) + RecordDisplayAttributes(1) + RecordLength(4)
        // + InformationLength(8) + LogicalBlocksRecorded(8)
        // + AccessTime(12) + ModificationTime(12) + AttributeTime(12) + Checkpoint(4)
        // + ExtendedAttributeICB(16) + ImplementationIdentifier(32) + UniqueID(8)
        // + LengthOfExtendedAttributes(4) + LengthOfAllocationDescriptors(4)
        // = 16+20+4+4+4+2+1+1+4+8+8+12+12+12+4+16+32+8+4+4 = 176
        quint32 nLenExtAttrs = read_uint32(dirInfo.nFileEntryOffset + 168);
        quint32 nLenAllocDescs = read_uint32(dirInfo.nFileEntryOffset + 172);
        // ICBTag file type is at offset 16+12 = 28 (within ICBTag at offset 16)
        quint8 nICBFileType = read_uint8(dirInfo.nFileEntryOffset + 28);
        // ICBTag flags (allocation type) at offset 16+18 = 34
        quint16 nICBFlags = read_uint16(dirInfo.nFileEntryOffset + 34);
        quint8 nAllocType = (quint8)(nICBFlags & 0x07);

        bool bIsDirectory = (nICBFileType == 4);

        qint64 nAllocDescsOffset = dirInfo.nFileEntryOffset + 176 + (qint64)nLenExtAttrs;

        if (!bIsDirectory) {
            // Regular file - read allocation descriptors to get data location
            quint64 nInfoLength = read_uint64(dirInfo.nFileEntryOffset + 56);

            ARCHIVERECORD record = {};
            record.mapProperties[FPART_PROP_ORIGINALNAME] = dirInfo.sPath;
            record.mapProperties[FPART_PROP_UNCOMPRESSEDSIZE] = (qint64)nInfoLength;
            record.mapProperties[FPART_PROP_COMPRESSEDSIZE] = (qint64)nInfoLength;
            record.mapProperties[FPART_PROP_HANDLEMETHOD] = HANDLE_METHOD_STORE;
            record.mapProperties[FPART_PROP_ISFOLDER] = false;

            if (nAllocType == 0 && nLenAllocDescs >= 8) {
                // Short allocation descriptor: ExtentLength(4) + ExtentPosition(4)
                quint32 nExtLength = read_uint32(nAllocDescsOffset) & 0x3FFFFFFF;
                quint32 nExtPos    = read_uint32(nAllocDescsOffset + 4);
                record.nStreamOffset = (qint64)nExtPos * nBlockSize;
                record.nStreamSize   = (qint64)nExtLength;
            } else if (nAllocType == 1 && nLenAllocDescs >= 16) {
                // Long allocation descriptor: ExtentLength(4) + ExtentLocation: LogicalBlockNum(4) + PartRef(2) + ImplUse(6)
                quint32 nExtLength = read_uint32(nAllocDescsOffset) & 0x3FFFFFFF;
                quint32 nExtPos    = read_uint32(nAllocDescsOffset + 4);
                record.nStreamOffset = (qint64)nExtPos * nBlockSize;
                record.nStreamSize   = (qint64)nExtLength;
            } else if (nAllocType == 3 && nLenAllocDescs > 0) {
                // Data stored directly in allocation descriptors (inline)
                record.nStreamOffset = nAllocDescsOffset;
                record.nStreamSize   = (qint64)nLenAllocDescs;
            }

            listResult.append(record);
        } else {
            // Directory - add folder record and enqueue children
            if (!dirInfo.sPath.isEmpty()) {
                ARCHIVERECORD record = {};
                record.mapProperties[FPART_PROP_ORIGINALNAME] = dirInfo.sPath;
                record.mapProperties[FPART_PROP_UNCOMPRESSEDSIZE] = (qint64)0;
                record.mapProperties[FPART_PROP_COMPRESSEDSIZE] = (qint64)0;
                record.mapProperties[FPART_PROP_HANDLEMETHOD] = HANDLE_METHOD_STORE;
                record.mapProperties[FPART_PROP_ISFOLDER] = true;
                listResult.append(record);
            }

            // Read directory data to find File Identifier Descriptors
            qint64 nDirDataOffset = -1;
            qint64 nDirDataSize = 0;

            quint64 nInfoLength = read_uint64(dirInfo.nFileEntryOffset + 56);

            if (nAllocType == 0 && nLenAllocDescs >= 8) {
                quint32 nExtPos = read_uint32(nAllocDescsOffset + 4);
                nDirDataOffset = (qint64)nExtPos * nBlockSize;
                nDirDataSize   = (qint64)nInfoLength;
            } else if (nAllocType == 1 && nLenAllocDescs >= 16) {
                quint32 nExtPos = read_uint32(nAllocDescsOffset + 4);
                nDirDataOffset = (qint64)nExtPos * nBlockSize;
                nDirDataSize   = (qint64)nInfoLength;
            }

            if (nDirDataOffset <= 0 || nDirDataSize <= 0 || nDirDataOffset >= getSize()) {
                continue;
            }

            // Parse File Identifier Descriptors (tag id 257)
            qint64 nFIDOffset = nDirDataOffset;
            qint64 nFIDEnd = nDirDataOffset + nDirDataSize;

            while (nFIDOffset < nFIDEnd && isPdStructNotCanceled(pPdStruct)) {
                if (nFIDOffset + (qint64)sizeof(UDF_TAG) > getSize()) {
                    break;
                }

                UDF_TAG fidTag = _readTag(nFIDOffset);

                if (fidTag.nTagIdentifier != TAG_FILE_IDENTIFIER_DESCRIPTOR) {
                    break;
                }

                // File Identifier Descriptor layout:
                // tag(16) + FileVersionNumber(2) + FileCharacteristics(1) + LengthOfFileIdentifier(1)
                // + ICB(16) + LengthOfImplementationUse(2) [+ ImplementationUse(var)] [+ FileIdentifier(var)] [+ padding]
                quint8 nFileCharacteristics = read_uint8(nFIDOffset + 18);
                quint8 nLenFileId = read_uint8(nFIDOffset + 19);
                quint16 nLenImplUse = read_uint16(nFIDOffset + 36);

                // ICB (long_ad) of child: ExtentLength(4) + ExtentLocation: LogicalBlockNum(4) + PartRef(2) + ImplUse(6)
                quint32 nChildICBLocation = read_uint32(nFIDOffset + 20 + 4);

                bool bIsParent = (nFileCharacteristics & 0x08) != 0;  // Parent directory
                bool bChildIsDir = (nFileCharacteristics & 0x02) != 0;
                Q_UNUSED(bChildIsDir)

                // Compute name
                QString sChildName;
                if (!bIsParent && nLenFileId > 0) {
                    qint64 nNameOffset = nFIDOffset + 38 + (qint64)nLenImplUse;
                    if (nNameOffset + nLenFileId <= getSize()) {
                        QByteArray baName = read_array(nNameOffset, nLenFileId);
                        // OSTA CS0 encoded: if first byte is 8, rest is ASCII; if 16, UTF-16BE
                        if (baName.size() > 0) {
                            quint8 nEncType = (quint8)baName.at(0);
                            if (nEncType == 16 && baName.size() >= 3) {
                                sChildName = QString::fromUtf16(reinterpret_cast<const char16_t *>(baName.constData() + 1), (baName.size() - 1) / 2);
                            } else if (nEncType == 8 && baName.size() >= 2) {
                                sChildName = QString::fromLatin1(baName.constData() + 1, baName.size() - 1);
                            }
                        }
                    }
                }

                // Total FID size (must be 4-byte aligned)
                qint32 nFIDSize = 38 + (qint32)nLenImplUse + (qint32)nLenFileId;
                qint32 nFIDPadded = (nFIDSize + 3) & ~3;

                if (nFIDPadded <= 0) {
                    break;
                }

                if (!bIsParent && !sChildName.isEmpty() && nChildICBLocation > 0) {
                    QString sChildPath;
                    if (dirInfo.sPath.isEmpty()) {
                        sChildPath = sChildName;
                    } else {
                        sChildPath = dirInfo.sPath + "/" + sChildName;
                    }

                    qint64 nChildFileEntryOffset = (qint64)nChildICBLocation * nBlockSize;

                    if (nChildFileEntryOffset > 0 && nChildFileEntryOffset < getSize() && !setVisited.contains(nChildFileEntryOffset)) {
                        setVisited.insert(nChildFileEntryOffset);

                        DirEntry childEntry;
                        childEntry.nFileEntryOffset = nChildFileEntryOffset;
                        childEntry.sPath = sChildPath;
                        listQueue.append(childEntry);
                    }
                }

                nFIDOffset += nFIDPadded;
            }
        }
    }

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

QList<QString> XUDF::getSearchSignatures()
{
    QList<QString> listResult;

    return listResult;
}

XBinary *XUDF::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XUDF(pDevice);
}

