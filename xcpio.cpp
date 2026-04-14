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
#include "xcpio.h"

XBinary::XCONVERT _TABLE_XCPIO_STRUCTID[] = {{XCPIO::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                             {XCPIO::STRUCTID_NEWC_HEADER, "NEWC_HEADER", QString("CPIO newc header")},
                                             {XCPIO::STRUCTID_CRC_HEADER, "CRC_HEADER", QString("CPIO CRC header")},
                                             {XCPIO::STRUCTID_ODC_HEADER, "ODC_HEADER", QString("CPIO odc header")},
                                             {XCPIO::STRUCTID_BINARY_HEADER, "BINARY_HEADER", QString("CPIO binary header")}};

static const quint32 CPIO_MODE_IFMT = 0170000;
static const quint32 CPIO_MODE_IFDIR = 0040000;

XCPIO::XCPIO(QIODevice *pDevice) : XArchive(pDevice)
{
}

XCPIO::~XCPIO()
{
}

bool XCPIO::isValid(PDSTRUCT *pPdStruct)
{
    CPIO_RECORD_INFO info = {};

    return _parseRecord(0, &info);
}

bool XCPIO::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCPIO xcpio(pDevice);
    return xcpio.isValid(pPdStruct);
}

XCPIO::CPIO_FORMAT XCPIO::_detectFormat(qint64 nOffset)
{
    CPIO_FORMAT result = CPIO_FORMAT_UNKNOWN;

    qint64 nAvailableSize = getSize() - nOffset;

    if (nAvailableSize < 2) {
        return result;
    }

    if (nAvailableSize >= 6) {
        char szMagic[7] = {0};
        read_array(nOffset, szMagic, 6);

        QString sMagic = QString::fromLatin1(szMagic, 6);

        if (sMagic == "070701") {
            return CPIO_FORMAT_NEWC;
        } else if (sMagic == "070702") {
            return CPIO_FORMAT_CRC;
        } else if (sMagic == "070707") {
            return CPIO_FORMAT_ODC;
        }
    }

    if (read_uint16(nOffset) == 0x71C7) {
        result = CPIO_FORMAT_BINARY_LE;
    } else if (read_uint16(nOffset, true) == 0x71C7) {
        result = CPIO_FORMAT_BINARY_BE;
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

qint64 XCPIO::_readOctValue(const char *pValue, qint32 nSize)
{
    if (!pValue || nSize <= 0) {
        return 0;
    }

    QString sValue = QString::fromLatin1(pValue, nSize).trimmed();
    bool bOk = false;
    qint64 nResult = sValue.toLongLong(&bOk, 8);

    return bOk ? nResult : 0;
}

quint16 XCPIO::_readBinaryUInt16(qint64 nOffset, bool bIsBigEndian)
{
    return read_uint16(nOffset, bIsBigEndian);
}

quint32 XCPIO::_readBinaryUInt32(qint64 nOffset, bool bIsBigEndian)
{
    quint32 nHigh = _readBinaryUInt16(nOffset, bIsBigEndian);
    quint32 nLow = _readBinaryUInt16(nOffset + 2, bIsBigEndian);

    return (nHigh << 16) | nLow;
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

bool XCPIO::_parseRecord(qint64 nOffset, CPIO_RECORD_INFO *pInfo)
{
    if ((!pInfo) || (nOffset < 0)) {
        return false;
    }

    *pInfo = {};
    pInfo->format = _detectFormat(nOffset);
    pInfo->nHeaderOffset = nOffset;

    if (pInfo->format == CPIO_FORMAT_UNKNOWN) {
        return false;
    }

    const qint64 nTotalSize = getSize();
    qint64 nNameSize = 0;
    qint64 nDataSize = 0;

    if ((pInfo->format == CPIO_FORMAT_NEWC) || (pInfo->format == CPIO_FORMAT_CRC)) {
        if ((nOffset + (qint64)sizeof(CPIO_NEWC_HEADER)) > nTotalSize) {
            return false;
        }

        CPIO_NEWC_HEADER header = _readNewcHeader(nOffset);

        pInfo->nHeaderSize = sizeof(CPIO_NEWC_HEADER);
        nNameSize = _readHexValue(header.namesize, 8);
        nDataSize = _readHexValue(header.filesize, 8);
        pInfo->nMode = (quint32)_readHexValue(header.mode, 8);
        pInfo->nUID = (quint32)_readHexValue(header.uid, 8);
        pInfo->nGID = (quint32)_readHexValue(header.gid, 8);
        pInfo->nNLink = (quint32)_readHexValue(header.nlink, 8);
        pInfo->nMTime = (quint64)_readHexValue(header.mtime, 8);
    } else if (pInfo->format == CPIO_FORMAT_ODC) {
        if ((nOffset + (qint64)sizeof(CPIO_ODC_HEADER)) > nTotalSize) {
            return false;
        }

        CPIO_ODC_HEADER header = _readOdcHeader(nOffset);

        pInfo->nHeaderSize = sizeof(CPIO_ODC_HEADER);
        nNameSize = _readOctValue(header.namesize, 6);
        nDataSize = _readOctValue(header.filesize, 11);
        pInfo->nMode = (quint32)_readOctValue(header.mode, 6);
        pInfo->nUID = (quint32)_readOctValue(header.uid, 6);
        pInfo->nGID = (quint32)_readOctValue(header.gid, 6);
        pInfo->nNLink = (quint32)_readOctValue(header.nlink, 6);
        pInfo->nRDev = (quint32)_readOctValue(header.rdev, 6);
        pInfo->nMTime = (quint64)_readOctValue(header.mtime, 11);
    } else {
        bool bIsBigEndian = (pInfo->format == CPIO_FORMAT_BINARY_BE);

        if ((nOffset + (qint64)sizeof(CPIO_BINARY_HEADER)) > nTotalSize) {
            return false;
        }

        pInfo->nHeaderSize = sizeof(CPIO_BINARY_HEADER);
        nNameSize = _readBinaryUInt16(nOffset + offsetof(CPIO_BINARY_HEADER, namesize), bIsBigEndian);
        nDataSize = _readBinaryUInt32(nOffset + offsetof(CPIO_BINARY_HEADER, filesizeHigh), bIsBigEndian);
        pInfo->nMode = _readBinaryUInt16(nOffset + offsetof(CPIO_BINARY_HEADER, mode), bIsBigEndian);
        pInfo->nUID = _readBinaryUInt16(nOffset + offsetof(CPIO_BINARY_HEADER, uid), bIsBigEndian);
        pInfo->nGID = _readBinaryUInt16(nOffset + offsetof(CPIO_BINARY_HEADER, gid), bIsBigEndian);
        pInfo->nNLink = _readBinaryUInt16(nOffset + offsetof(CPIO_BINARY_HEADER, nlink), bIsBigEndian);
        pInfo->nRDev = _readBinaryUInt16(nOffset + offsetof(CPIO_BINARY_HEADER, rdev), bIsBigEndian);
        pInfo->nMTime = _readBinaryUInt32(nOffset + offsetof(CPIO_BINARY_HEADER, mtimeHigh), bIsBigEndian);
    }

    if ((nNameSize <= 0) || (nNameSize > 0x10000) || (nDataSize < 0)) {
        return false;
    }

    qint64 nNameOffset = nOffset + pInfo->nHeaderSize;
    qint64 nNameEnd = nNameOffset + nNameSize;

    if ((nNameOffset < 0) || (nNameEnd > nTotalSize)) {
        return false;
    }

    QByteArray baName = read_array(nNameOffset, nNameSize);

    if (baName.size() != nNameSize) {
        return false;
    }

    if (baName.endsWith('\0')) {
        baName.chop(1);
    }

    pInfo->sFileName = QString::fromLatin1(baName.constData(), baName.size());

    qint64 nDataOffset = nNameEnd;

    if ((pInfo->format == CPIO_FORMAT_NEWC) || (pInfo->format == CPIO_FORMAT_CRC)) {
        nDataOffset = (nDataOffset + 3) & ~((qint64)3);
    } else if ((pInfo->format == CPIO_FORMAT_BINARY_LE) || (pInfo->format == CPIO_FORMAT_BINARY_BE)) {
        nDataOffset = (nDataOffset + 1) & ~((qint64)1);
    }

    if ((nDataOffset < 0) || ((nDataOffset + nDataSize) > nTotalSize)) {
        return false;
    }

    qint64 nNextOffset = nDataOffset + nDataSize;

    if ((pInfo->format == CPIO_FORMAT_NEWC) || (pInfo->format == CPIO_FORMAT_CRC)) {
        nNextOffset = (nNextOffset + 3) & ~((qint64)3);
    } else if ((pInfo->format == CPIO_FORMAT_BINARY_LE) || (pInfo->format == CPIO_FORMAT_BINARY_BE)) {
        nNextOffset = (nNextOffset + 1) & ~((qint64)1);
    }

    if ((nNextOffset <= nOffset) || (nNextOffset > nTotalSize)) {
        return false;
    }

    pInfo->nDataOffset = nDataOffset;
    pInfo->nDataSize = nDataSize;
    pInfo->nHeaderSize = nDataOffset - nOffset;
    pInfo->nNextOffset = nNextOffset;
    pInfo->bIsFolder = ((pInfo->nMode & CPIO_MODE_IFMT) == CPIO_MODE_IFDIR) || pInfo->sFileName.endsWith(QLatin1Char('/'));

    return true;
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

XBinary::ENDIAN XCPIO::getEndian()
{
    CPIO_FORMAT format = _detectFormat(0);

    if (format == CPIO_FORMAT_BINARY_BE) {
        return ENDIAN_BIG;
    } else if (format == CPIO_FORMAT_BINARY_LE) {
        return ENDIAN_LITTLE;
    }

    return ENDIAN_UNKNOWN;
}

QList<QString> XCPIO::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("'070701'");
    listResult.append("'070702'");
    listResult.append("'070707'");
    listResult.append("C771");
    listResult.append("71C7");

    return listResult;
}

XBinary *XCPIO::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XCPIO(pDevice);
}

quint64 XCPIO::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    return getRecords(-1, pPdStruct).count();
}

QList<XArchive::RECORD> XCPIO::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<RECORD> listResult;

    qint64 nOffset = 0;
    qint64 nTotalSize = getSize();

    while ((nOffset < nTotalSize) && isPdStructNotCanceled(pPdStruct)) {
        CPIO_RECORD_INFO info = {};

        if (!_parseRecord(nOffset, &info)) {
            break;
        }

        bool bIsTrailer = _isTrailerRecord(info.sFileName);

        if (!bIsTrailer) {
            RECORD record = {};
            record.spInfo.sRecordName = info.sFileName;
            record.spInfo.nUncompressedSize = info.nDataSize;
            record.spInfo.compressMethod = HANDLE_METHOD_STORE;
            record.nHeaderOffset = info.nHeaderOffset;
            record.nHeaderSize = info.nHeaderSize;
            record.nDataOffset = info.nDataOffset;
            record.nDataSize = info.nDataSize;
            listResult.append(record);

            if ((nLimit != -1) && (listResult.count() >= nLimit)) {
                break;
            }
        }

        nOffset = info.nNextOffset;

        if (bIsTrailer) {
            break;
        }
    }

    return listResult;
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

QString XCPIO::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XCPIO_STRUCTID, sizeof(_TABLE_XCPIO_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XCPIO::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XCPIO_STRUCTID, sizeof(_TABLE_XCPIO_STRUCTID) / sizeof(XBinary::XCONVERT));
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
        } else if ((format == CPIO_FORMAT_BINARY_LE) || (format == CPIO_FORMAT_BINARY_BE)) {
            _dataHeadersOptions.nID = STRUCTID_BINARY_HEADER;
        }

        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;
        _dataHeadersOptions.nCount = getNumberOfRecords(pPdStruct);

        if (_dataHeadersOptions.nID != STRUCTID_UNKNOWN) {
            listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
        }
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
                dataHeader.listRecords.append(getDataRecord(6, 6, "Device", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(12, 6, "Inode", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(18, 6, "Mode", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(24, 6, "UID", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(30, 6, "GID", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(36, 6, "Nlink", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(42, 6, "RDev", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(48, 11, "MTime", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(59, 6, "Namesize", VT_CHAR_ARRAY, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(65, 11, "Filesize", VT_CHAR_ARRAY, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);
            } else if ((format == CPIO_FORMAT_BINARY_LE) || (format == CPIO_FORMAT_BINARY_BE)) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCPIO::structIDToString(dataHeadersOptions.nID));

                dataHeader.nSize = sizeof(CPIO_BINARY_HEADER);

                dataHeader.listRecords.append(getDataRecord(0, 2, "Magic", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(2, 2, "Device", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(4, 2, "Inode", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(6, 2, "Mode", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(8, 2, "UID", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(10, 2, "GID", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(12, 2, "Nlink", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(14, 2, "RDev", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(16, 2, "MTimeHigh", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(18, 2, "MTimeLow", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(20, 2, "Namesize", VT_UINT16, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(22, 2, "FilesizeHigh", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(24, 2, "FilesizeLow", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XCPIO::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    const qint64 nTotalSize = getSize();
    qint64 nOffset = 0;
    qint64 nArchiveEnd = 0;
    qint32 nRecordCount = 0;

    while ((nOffset < nTotalSize) && isPdStructNotCanceled(pPdStruct)) {
        CPIO_RECORD_INFO info = {};

        if (!_parseRecord(nOffset, &info)) {
            break;
        }

        nArchiveEnd = info.nNextOffset;

        if (!_isTrailerRecord(info.sFileName)) {
            if (nFileParts & FILEPART_HEADER) {
                FPART header = {};
                header.filePart = FILEPART_HEADER;
                header.nFileOffset = info.nHeaderOffset;
                header.nFileSize = info.nHeaderSize;
                header.nVirtualAddress = -1;
                header.sName = info.sFileName.isEmpty() ? tr("Header") : QString("%1 (%2)").arg(info.sFileName, tr("Header"));
                listResult.append(header);
            }

            if (nFileParts & FILEPART_REGION) {
                FPART region = {};
                region.filePart = FILEPART_REGION;
                region.nFileOffset = info.nDataOffset;
                region.nFileSize = info.nDataSize;
                region.nVirtualAddress = -1;
                region.sName = info.sFileName;
                listResult.append(region);
            }

            nRecordCount++;
            if ((nLimit != -1) && (nRecordCount >= nLimit)) {
                break;
            }
        } else {
            break;
        }

        nOffset = info.nNextOffset;
    }

    if ((nFileParts & FILEPART_OVERLAY) && (nArchiveEnd < nTotalSize)) {
        FPART record = {};
        record.filePart = FILEPART_OVERLAY;
        record.nFileOffset = nArchiveEnd;
        record.nFileSize = nTotalSize - nArchiveEnd;
        record.nVirtualAddress = -1;
        record.sName = tr("Overlay");

        listResult.append(record);
    }

    return listResult;
}

bool XCPIO::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)

    if (!pState) {
        return false;
    }

    CPIO_UNPACK_CONTEXT *pContext = new CPIO_UNPACK_CONTEXT;
    pContext->format = _detectFormat(0);
    if ((pContext->format == CPIO_FORMAT_NEWC) || (pContext->format == CPIO_FORMAT_CRC)) {
        pContext->nHeaderSize = sizeof(CPIO_NEWC_HEADER);
    } else if (pContext->format == CPIO_FORMAT_ODC) {
        pContext->nHeaderSize = sizeof(CPIO_ODC_HEADER);
    } else if ((pContext->format == CPIO_FORMAT_BINARY_LE) || (pContext->format == CPIO_FORMAT_BINARY_BE)) {
        pContext->nHeaderSize = sizeof(CPIO_BINARY_HEADER);
    } else {
        delete pContext;
        return false;
    }
    pContext->listRecords = getRecords(-1, pPdStruct);
    pContext->nCurrentRecord = 0;

    if (pContext->listRecords.isEmpty()) {
        delete pContext;
        return false;
    }

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listRecords.count();
    pState->nCurrentOffset = (pState->nNumberOfRecords > 0) ? pContext->listRecords.at(0).nHeaderOffset : 0;

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

    if ((pState->nCurrentIndex >= 0) && (pState->nCurrentIndex < pContext->listRecords.count())) {
        const RECORD &record = pContext->listRecords.at(pState->nCurrentIndex);
        CPIO_RECORD_INFO info = {};

        result.nStreamOffset = record.nDataOffset;
        result.nStreamSize = record.nDataSize;
        result.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, record.spInfo.sRecordName);
        result.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, record.spInfo.nUncompressedSize);
        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, record.nDataSize);
        result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_STORE);
        result.mapProperties.insert(XBinary::FPART_PROP_HEADER_OFFSET, record.nHeaderOffset);
        result.mapProperties.insert(XBinary::FPART_PROP_HEADER_SIZE, record.nHeaderSize);

        if (_parseRecord(record.nHeaderOffset, &info)) {
            result.mapProperties.insert(XBinary::FPART_PROP_FILEMODE, info.nMode);
            result.mapProperties.insert(XBinary::FPART_PROP_UID, info.nUID);
            result.mapProperties.insert(XBinary::FPART_PROP_GID, info.nGID);
            result.mapProperties.insert(XBinary::FPART_PROP_ISFOLDER, info.bIsFolder);

#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
            result.mapProperties.insert(XBinary::FPART_PROP_DATETIME, QDateTime::fromSecsSinceEpoch((qint64)info.nMTime));
#else
            result.mapProperties.insert(XBinary::FPART_PROP_DATETIME, QDateTime::fromMSecsSinceEpoch((qint64)info.nMTime * 1000));
#endif
        }
    }

    return result;
}

bool XCPIO::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState || !pState->pContext) {
        return false;
    }

    CPIO_UNPACK_CONTEXT *pContext = (CPIO_UNPACK_CONTEXT *)pState->pContext;

    pState->nCurrentIndex++;
    pContext->nCurrentRecord = pState->nCurrentIndex;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listRecords.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
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

