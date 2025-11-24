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
#include "xcpio.h"

XBinary::XCONVERT _TABLE_XCPIO_STRUCTID[] = {{XCPIO::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                             {XCPIO::STRUCTID_NEWC_HEADER, "NEWC_HEADER", QString("CPIO newc header")},
                                             {XCPIO::STRUCTID_CRC_HEADER, "CRC_HEADER", QString("CPIO CRC header")},
                                             {XCPIO::STRUCTID_ODC_HEADER, "ODC_HEADER", QString("CPIO odc header")}};

XCPIO::XCPIO(QIODevice *pDevice) : XArchive(pDevice)
{
}

XCPIO::~XCPIO()
{
}

bool XCPIO::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() >= 110) {  // Minimum header size
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

        if (compareSignature(&memoryMap, "303730373031", 0, pPdStruct) ||  // "070701"
            compareSignature(&memoryMap, "303730373032", 0, pPdStruct) ||  // "070702"
            compareSignature(&memoryMap, "303730373037", 0, pPdStruct)) {  // "070707"
            bResult = true;
        }
    }

    return bResult;
}

bool XCPIO::isValid(QIODevice *pDevice)
{
    XCPIO xcpio(pDevice);
    return xcpio.isValid();
}

XCPIO::CPIO_FORMAT XCPIO::_detectFormat(qint64 nOffset)
{
    CPIO_FORMAT result = CPIO_FORMAT_UNKNOWN;

    char szMagic[7] = {0};
    read_array(nOffset, szMagic, 6);

    if (getSize() - nOffset < 6) {
        return result;
    }

    QString sMagic = QString::fromLatin1(szMagic, 6);

    if (sMagic == "070701") {
        result = CPIO_FORMAT_NEWC;
    } else if (sMagic == "070702") {
        result = CPIO_FORMAT_CRC;
    } else if (sMagic == "070707") {
        result = CPIO_FORMAT_ODC;
    }

    return result;
}

qint64 XCPIO::_readHexValue(const char *pValue, qint32 nSize)
{
    if (!pValue || nSize <= 0) {
        return 0;
    }

    QString sValue = QString::fromLatin1(pValue, nSize).trimmed();
    bool bOk = false;
    qint64 nResult = sValue.toLongLong(&bOk, 16);

    return bOk ? nResult : 0;
}

XCPIO::CPIO_NEWC_HEADER XCPIO::_readNewcHeader(qint64 nOffset)
{
    CPIO_NEWC_HEADER header = {};
    read_array(nOffset, (char *)&header, sizeof(CPIO_NEWC_HEADER));
    return header;
}

XCPIO::CPIO_ODC_HEADER XCPIO::_readOdcHeader(qint64 nOffset)
{
    CPIO_ODC_HEADER header = {};
    read_array(nOffset, (char *)&header, sizeof(CPIO_ODC_HEADER));
    return header;
}

bool XCPIO::_isTrailerRecord(const QString &sFileName)
{
    return sFileName == "TRAILER!!!";
}

QString XCPIO::getFileFormatExt()
{
    return "cpio";
}

QString XCPIO::getFileFormatExtsString()
{
    return "CPIO (*.cpio)";
}

QString XCPIO::getMIMEString()
{
    return "application/x-cpio";
}

XBinary::FT XCPIO::getFileType()
{
    return FT_CPIO;
}

QList<XBinary::MAPMODE> XCPIO::getMapModesList()
{
    QList<MAPMODE> listResult;
    listResult.append(MAPMODE_REGIONS);
    return listResult;
}

XBinary::_MEMORY_MAP XCPIO::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result = _getMemoryMap(FILEPART_HEADER | FILEPART_REGION | FILEPART_OVERLAY, pPdStruct);

    return result;
}

QString XCPIO::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XCPIO_STRUCTID, sizeof(_TABLE_XCPIO_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XCPIO::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

        CPIO_FORMAT format = _detectFormat(0);
        if (format == CPIO_FORMAT_NEWC || format == CPIO_FORMAT_CRC) {
            _dataHeadersOptions.nID = (format == CPIO_FORMAT_NEWC) ? STRUCTID_NEWC_HEADER : STRUCTID_CRC_HEADER;
        } else if (format == CPIO_FORMAT_ODC) {
            _dataHeadersOptions.nID = STRUCTID_ODC_HEADER;
        }

        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;
        _dataHeadersOptions.nCount = getNumberOfRecords(pPdStruct);

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            CPIO_FORMAT format = _detectFormat(nStartOffset);

            if (format == CPIO_FORMAT_NEWC || format == CPIO_FORMAT_CRC) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCPIO::structIDToString(dataHeadersOptions.nID));

                dataHeader.nSize = sizeof(CPIO_NEWC_HEADER);

                dataHeader.listRecords.append(getDataRecord(0, 6, "Magic", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(6, 8, "Inode", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(14, 8, "Mode", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(22, 8, "UID", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(30, 8, "GID", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(38, 8, "Nlink", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(46, 8, "MTime", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(54, 8, "Filesize", VT_CHAR_ARRAY, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(62, 8, "DevMajor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(70, 8, "DevMinor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(78, 8, "RDevMajor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(86, 8, "RDevMinor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(94, 8, "Namesize", VT_CHAR_ARRAY, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(102, 8, "Check", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);
            } else if (format == CPIO_FORMAT_ODC) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCPIO::structIDToString(dataHeadersOptions.nID));

                dataHeader.nSize = sizeof(CPIO_ODC_HEADER);

                dataHeader.listRecords.append(getDataRecord(0, 6, "Magic", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(6, 6, "Inode", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(12, 6, "Mode", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(18, 6, "UID", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(24, 6, "GID", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(30, 6, "Nlink", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(36, 11, "MTime", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(47, 11, "Filesize", VT_CHAR_ARRAY, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(58, 6, "DevMajor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(64, 6, "DevMinor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(70, 6, "RDevMajor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(76, 6, "RDevMinor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(82, 6, "Namesize", VT_CHAR_ARRAY, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(88, 11, "Check", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XCPIO::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    QList<FPART> listResult;
    QList<RECORD> listRecords = getRecords(nLimit, pPdStruct);

    qint64 nTotalSize = getSize();
    qint64 nCurrentOffset = 0;

    for (qint32 i = 0; i < listRecords.count(); i++) {
        if (!isPdStructNotCanceled(pPdStruct)) {
            break;
        }

        nCurrentOffset = listRecords.at(i).nHeaderOffset;

        if (nFileParts & FILEPART_REGION) {
            FPART record = {};
            record.filePart = FILEPART_REGION;
            record.nFileOffset = listRecords.at(i).nHeaderOffset;
            record.nFileSize = listRecords.at(i).nHeaderSize + listRecords.at(i).nDataSize;
            record.nVirtualAddress = -1;
            record.sName = listRecords.at(i).spInfo.sRecordName;

            listResult.append(record);
        }
    }

    if (nFileParts & FILEPART_OVERLAY) {
        if (nCurrentOffset < nTotalSize) {
            FPART record = {};
            record.filePart = FILEPART_OVERLAY;
            record.nFileOffset = nCurrentOffset;
            record.nFileSize = nTotalSize - nCurrentOffset;
            record.nVirtualAddress = -1;
            record.sName = tr("Overlay");

            listResult.append(record);
        }
    }

    return listResult;
}

bool XCPIO::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    CPIO_UNPACK_CONTEXT *pContext = new CPIO_UNPACK_CONTEXT;
    pContext->format = _detectFormat(0);
    pContext->nHeaderSize = (pContext->format == CPIO_FORMAT_NEWC || pContext->format == CPIO_FORMAT_CRC) ? sizeof(CPIO_NEWC_HEADER) : sizeof(CPIO_ODC_HEADER);

    pState->pContext = pContext;
    pState->nCurrentOffset = 0;

    return true;
}

XArchive::ARCHIVERECORD XCPIO::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    ARCHIVERECORD result = {};

    if (!pState || !pState->pContext) {
        return result;
    }

    CPIO_UNPACK_CONTEXT *pContext = (CPIO_UNPACK_CONTEXT *)pState->pContext;
    QList<RECORD> listRecords = getRecords(-1, pPdStruct);

    for (const RECORD &record : listRecords) {
        if (record.nHeaderOffset == pState->nCurrentOffset) {
            result.nStreamOffset = record.nDataOffset;
            result.nStreamSize = record.nDataSize;
            result.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, record.spInfo.sRecordName);
            result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_STORE);
            break;
        }
    }

    return result;
}

bool XCPIO::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState || !pState->pContext || !pDevice) {
        return false;
    }

    QList<RECORD> listRecords = getRecords(-1, pPdStruct);

    for (const RECORD &record : listRecords) {
        if (record.nHeaderOffset == pState->nCurrentOffset) {
            return copyDeviceMemory(getDevice(), record.nDataOffset, pDevice, 0, record.nDataSize);
        }
    }

    return false;
}

bool XCPIO::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState || !pState->pContext) {
        return false;
    }

    QList<RECORD> listRecords = getRecords(-1, pPdStruct);

    for (qint32 i = 0; i < listRecords.count(); i++) {
        if (listRecords.at(i).nHeaderOffset == pState->nCurrentOffset) {
            if (i + 1 < listRecords.count()) {
                pState->nCurrentOffset = listRecords.at(i + 1).nHeaderOffset;
                return true;
            }
            break;
        }
    }

    return false;
}

bool XCPIO::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if (pState->pContext) {
        CPIO_UNPACK_CONTEXT *pContext = (CPIO_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
    }

    pState->nCurrentOffset = 0;

    return true;
}
