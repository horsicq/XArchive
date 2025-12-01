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
#include "xiso9660.h"

XBinary::XCONVERT _TABLE_XISO9660_STRUCTID[] = {{XISO9660::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                {XISO9660::STRUCTID_PVDESC, "PVDESC", QString("Primary Volume Descriptor")},
                                                {XISO9660::STRUCTID_DIR_RECORD, "DIR_RECORD", QString("Directory Record")}};

XISO9660::XISO9660(QIODevice *pDevice) : XArchive(pDevice)
{
}

XISO9660::~XISO9660()
{
}

bool XISO9660::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() >= 0x8000) {  // At least PVD offset + size
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

        // ISO 9660 Primary Volume Descriptor is typically at offset 0x8000 (32KB)
        // Check for "CD001" signature
        if (compareSignature(&memoryMap, "4344303031", 0x8001, pPdStruct)) {  // "CD001"
            bResult = true;
        }
    }

    return bResult;
}

bool XISO9660::isValid(QIODevice *pDevice)
{
    XISO9660 xiso(pDevice);
    return xiso.isValid();
}

XISO9660::ISO9660_PVDESC XISO9660::_readPrimaryVolumeDescriptor(qint64 nOffset)
{
    ISO9660_PVDESC desc = {};
    read_array(nOffset, (char *)&desc, sizeof(ISO9660_PVDESC));
    return desc;
}

qint32 XISO9660::_getLogicalBlockSize()
{
    // Logical block size is at offset 128 (0x80) in the PVD (little-endian 16-bit value stored as 32-bit)
    quint16 nBlockSize = read_uint16(0x8000 + 128);
    return (qint32)nBlockSize;
}

qint64 XISO9660::_getPrimaryVolumeDescriptorOffset()
{
    // Primary Volume Descriptor is always at sector 16 (offset 0x8000)
    return 0x8000;
}

bool XISO9660::_isValidDescriptor(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    char szStandard[6] = {0};
    read_array(nOffset + 1, szStandard, 5);

    return QString::fromLatin1(szStandard, 5) == "CD001";
}

QString XISO9660::getFileFormatExt()
{
    return "iso";
}

QString XISO9660::getFileFormatExtsString()
{
    return "ISO 9660 (*.iso)";
}

QString XISO9660::getMIMEString()
{
    return "application/x-iso9660-image";
}

XBinary::FT XISO9660::getFileType()
{
    return FT_ISO9660;
}

QList<XBinary::MAPMODE> XISO9660::getMapModesList()
{
    QList<MAPMODE> listResult;
    listResult.append(MAPMODE_REGIONS);
    return listResult;
}

XBinary::_MEMORY_MAP XISO9660::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result = _getMemoryMap(FILEPART_HEADER | FILEPART_REGION | FILEPART_OVERLAY, pPdStruct);

    return result;
}

QString XISO9660::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XISO9660_STRUCTID, sizeof(_TABLE_XISO9660_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XISO9660::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(dataHeadersOptions)
    Q_UNUSED(pPdStruct)

    QList<DATA_HEADER> listResult;
    // TODO: Implement data header extraction
    return listResult;
}

QList<XBinary::FPART> XISO9660::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;
    qint64 nTotalSize = getSize();

    if (nFileParts & FILEPART_HEADER) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = _getPrimaryVolumeDescriptorOffset();
        record.nFileSize = sizeof(ISO9660_PVDESC);
        record.nVirtualAddress = -1;
        record.sName = tr("Primary Volume Descriptor");

        listResult.append(record);
    }

    if (nFileParts & FILEPART_REGION) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = _getPrimaryVolumeDescriptorOffset() + sizeof(ISO9660_PVDESC);
        record.nFileSize = nTotalSize - (_getPrimaryVolumeDescriptorOffset() + sizeof(ISO9660_PVDESC));
        record.nVirtualAddress = -1;
        record.sName = tr("Data");

        listResult.append(record);
    }

    return listResult;
}

QString XISO9660::_cleanFileName(const QString &sFileName)
{
    QString sResult = sFileName;
    
    // Remove version number (e.g., ";1" at the end)
    qint32 nSemicolon = sResult.indexOf(';');
    if (nSemicolon != -1) {
        sResult = sResult.left(nSemicolon);
    }
    
    return sResult;
}

QList<XBinary::ARCHIVERECORD> XISO9660::_parseDirectory(qint64 nOffset, qint64 nSize, qint32 nBlockSize, PDSTRUCT *pPdStruct)
{
    QList<ARCHIVERECORD> listResult;
    
    qint64 nCurrentOffset = nOffset;
    qint64 nEndOffset = nOffset + nSize;
    
    while ((nCurrentOffset < nEndOffset) && isPdStructNotCanceled(pPdStruct)) {
        qint64 nBlockStart = nCurrentOffset;
        qint64 nBlockEnd = qMin(nBlockStart + nBlockSize, nEndOffset);
        
        while (nCurrentOffset < nBlockEnd) {
            quint8 nRecordLength = read_uint8(nCurrentOffset);
            
            if (nRecordLength == 0) {
                // Padding to next block
                nCurrentOffset = nBlockEnd;
                break;
            }
            
            if (nRecordLength < 34 || nCurrentOffset + nRecordLength > nBlockEnd) {
                // Invalid record or extends beyond block
                nCurrentOffset = nBlockEnd;
                break;
            }
            
            // Read directory record
            quint8 nExtAttrLength = read_uint8(nCurrentOffset + 1);
            quint32 nExtentLocation = read_uint32(nCurrentOffset + 2);  // Little-endian
            quint32 nDataLength = read_uint32(nCurrentOffset + 10);     // Little-endian
            quint8 nFileFlags = read_uint8(nCurrentOffset + 25);
            quint8 nFileNameLength = read_uint8(nCurrentOffset + 32);
            
            // Read file name
            QString sFileName;
            if (nFileNameLength > 0 && nCurrentOffset + 33 + nFileNameLength <= nBlockEnd) {
                QByteArray baFileName = read_array(nCurrentOffset + 33, nFileNameLength);
                sFileName = QString::fromLatin1(baFileName);
            }
            
            // Skip "." and ".." entries
            if (nFileNameLength == 1 && (sFileName == "\x00" || sFileName == "\x01")) {
                nCurrentOffset += nRecordLength;
                continue;
            }
            
            // Create archive record
            ARCHIVERECORD record = {};
            record.nStreamOffset = (qint64)nExtentLocation * nBlockSize + nExtAttrLength * nBlockSize;
            record.nStreamSize = nDataLength;
            
            // Set properties
            record.mapProperties[FPART_PROP_ORIGINALNAME] = _cleanFileName(sFileName);
            record.mapProperties[FPART_PROP_UNCOMPRESSEDSIZE] = (qint64)nDataLength;
            record.mapProperties[FPART_PROP_COMPRESSEDSIZE] = (qint64)nDataLength;
            record.mapProperties[FPART_PROP_COMPRESSMETHOD] = COMPRESS_METHOD_STORE;
            record.mapProperties[FPART_PROP_ISFOLDER] = (bool)(nFileFlags & 0x02);
            
            // Store directory info for later processing
            if (nFileFlags & 0x02) {
                record.mapProperties[FPART_PROP_STREAMOFFSET] = (qint64)nExtentLocation * nBlockSize;
                record.mapProperties[FPART_PROP_STREAMSIZE] = (qint64)nDataLength;
            }
            
            listResult.append(record);
            
            nCurrentOffset += nRecordLength;
        }
    }
    
    return listResult;
}

bool XISO9660::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)
    Q_UNUSED(pPdStruct)
    
    if (!pState) return false;
    
    // Initialize state
    pState->nCurrentOffset = 0;
    pState->nTotalSize = getSize();
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    
    // Create unpack context
    ISO9660_UNPACK_CONTEXT *pContext = new ISO9660_UNPACK_CONTEXT;
    pContext->nLogicalBlockSize = _getLogicalBlockSize();
    
    if (pContext->nLogicalBlockSize < 512 || pContext->nLogicalBlockSize > 8192) {
        delete pContext;
        return false;
    }
    
    // Read root directory from PVD
    // The root directory record is embedded in the PVD at offset 156
    // It's 34 bytes minimum, contains:
    // +0: record length (1 byte)
    // +1: extended attr length (1 byte)  
    // +2: extent location LE (4 bytes)
    // +6: extent location BE (4 bytes)
    // +10: data length LE (4 bytes)
    // +14: data length BE (4 bytes)
    
    qint64 nRootRecordOffset = 0x8000 + 156;
    quint8 nRootRecordLength = read_uint8(nRootRecordOffset);
    
    if (nRootRecordLength < 34) {
        delete pContext;
        return false;
    }
    
    quint8 nExtAttrLength = read_uint8(nRootRecordOffset + 1);
    quint32 nRootExtentLocation = read_uint32(nRootRecordOffset + 2);
    quint32 nRootDataLength = read_uint32(nRootRecordOffset + 10);
    
    qint64 nRootOffset = (qint64)nRootExtentLocation * pContext->nLogicalBlockSize;
    qint64 nRootSize = (qint64)nRootDataLength;
    
    // Parse root directory
    pContext->listCurrentDirRecords = _parseDirectory(nRootOffset, nRootSize, pContext->nLogicalBlockSize, pPdStruct);
    pContext->nCurrentRecordIndex = 0;
    pContext->setProcessedBlocks.insert(nRootExtentLocation);
    
    // Count total records (will be updated as we discover subdirectories)
    pState->nNumberOfRecords = pContext->listCurrentDirRecords.count();
    
    pState->pContext = pContext;
    
    return true;
}

XBinary::ARCHIVERECORD XISO9660::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    
    ARCHIVERECORD record = {};
    
    if (!pState || !pState->pContext) return record;
    
    ISO9660_UNPACK_CONTEXT *pContext = (ISO9660_UNPACK_CONTEXT *)pState->pContext;
    
    if (pContext->nCurrentRecordIndex >= 0 && pContext->nCurrentRecordIndex < pContext->listCurrentDirRecords.count()) {
        record = pContext->listCurrentDirRecords[pContext->nCurrentRecordIndex];
    }
    
    return record;
}

bool XISO9660::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    
    if (!pState || !pState->pContext || !pDevice) return false;
    
    ISO9660_UNPACK_CONTEXT *pContext = (ISO9660_UNPACK_CONTEXT *)pState->pContext;
    
    if (pContext->nCurrentRecordIndex < 0 || pContext->nCurrentRecordIndex >= pContext->listCurrentDirRecords.count()) {
        return false;
    }
    
    ARCHIVERECORD record = pContext->listCurrentDirRecords[pContext->nCurrentRecordIndex];
    
    // Don't extract folders
    if (record.mapProperties.value(FPART_PROP_ISFOLDER).toBool()) {
        return true;
    }
    
    // Read and write data
    qint64 nOffset = record.nStreamOffset;
    qint64 nSize = record.nStreamSize;
    qint64 nBufferSize = 65536;
    
    while (nSize > 0 && isPdStructNotCanceled(pPdStruct)) {
        qint64 nCurrentSize = qMin(nSize, nBufferSize);
        QByteArray baData = read_array(nOffset, nCurrentSize);
        
        if (baData.size() != nCurrentSize) {
            return false;
        }
        
        if (pDevice->write(baData) != nCurrentSize) {
            return false;
        }
        
        nOffset += nCurrentSize;
        nSize -= nCurrentSize;
    }
    
    return true;
}

bool XISO9660::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext) return false;
    
    ISO9660_UNPACK_CONTEXT *pContext = (ISO9660_UNPACK_CONTEXT *)pState->pContext;
    
    // Check if current record is a directory - add to processing stack
    if (pContext->nCurrentRecordIndex >= 0 && pContext->nCurrentRecordIndex < pContext->listCurrentDirRecords.count()) {
        ARCHIVERECORD record = pContext->listCurrentDirRecords[pContext->nCurrentRecordIndex];
        
        if (record.mapProperties.value(FPART_PROP_ISFOLDER).toBool()) {
            qint64 nDirOffset = record.mapProperties.value(FPART_PROP_STREAMOFFSET).toLongLong();
            qint64 nDirSize = record.mapProperties.value(FPART_PROP_STREAMSIZE).toLongLong();
            qint64 nDirBlock = nDirOffset / pContext->nLogicalBlockSize;
            
            // Only add if not already processed (avoid loops)
            if (!pContext->setProcessedBlocks.contains(nDirBlock)) {
                pContext->listDirStack.append(QPair<qint64, qint64>(nDirOffset, nDirSize));
                pContext->setProcessedBlocks.insert(nDirBlock);
            }
        }
    }
    
    pContext->nCurrentRecordIndex++;
    pState->nCurrentIndex++;
    
    // If we've processed all records in current directory, load next directory from stack
    if (pContext->nCurrentRecordIndex >= pContext->listCurrentDirRecords.count()) {
        if (!pContext->listDirStack.isEmpty()) {
            QPair<qint64, qint64> dirInfo = pContext->listDirStack.takeFirst();
            pContext->listCurrentDirRecords = _parseDirectory(dirInfo.first, dirInfo.second, pContext->nLogicalBlockSize, pPdStruct);
            pContext->nCurrentRecordIndex = 0;
            
            // Update total count
            pState->nNumberOfRecords += pContext->listCurrentDirRecords.count();
        }
    }
    
    return true;
}

bool XISO9660::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    
    if (!pState) return false;
    
    if (pState->pContext) {
        ISO9660_UNPACK_CONTEXT *pContext = (ISO9660_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
    }
    
    return true;
}
