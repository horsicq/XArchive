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
#include "xtar.h"

XTAR::XCONVERT _TABLE_XTAR_STRUCTID[] = {{XTAR::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                         {XTAR::STRUCTID_POSIX_HEADER, "posix_header", QString("posix_header")}};

XTAR::XTAR(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XTAR::isValid(PDSTRUCT *pPdStruct)
{
    _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

    return _isValid(&memoryMap, 0, pPdStruct);
}

bool XTAR::_isValid(_MEMORY_MAP *pMemoryMap, qint64 nOffset, PDSTRUCT *pPdStruct)
{
    // TODO more checks
    bool bResult = false;

    if ((getSize() - nOffset) >= 0x200)  // TODO const
    {
        if (compareSignature(pMemoryMap, "00'ustar'", nOffset + 0x100, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

bool XTAR::isValid(QIODevice *pDevice)
{
    XTAR xtar(pDevice);

    return xtar.isValid();
}

quint64 XTAR::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    quint64 nResult = 0;

    _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

    qint64 nOffset = 0;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        XTAR::posix_header header = read_posix_header(nOffset);

        if (!compareMemory(header.magic, "ustar", 5)) {
            break;
        }

        nResult++;

        nOffset += (0x200);
        nOffset += align_up(_getSize(header), 0x200);
    }

    return nResult;
}

QList<XArchive::RECORD> XTAR::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listResult;

    qint64 nOffset = 0;

    qint32 nIndex = 0;

    while (isPdStructNotCanceled(pPdStruct)) {
        XTAR::posix_header header = read_posix_header(nOffset);

        if (!compareMemory(header.magic, "ustar", 5)) {
            break;
        }

        RECORD record = {};
        record.spInfo.compressMethod = COMPRESS_METHOD_STORE;
        record.nDataOffset = nOffset + 0x200;
        record.nHeaderOffset = nOffset;
        record.nHeaderSize = 0x200;
        record.spInfo.nUncompressedSize = _getSize(header);
        record.spInfo.sRecordName = header.name;
        record.nDataSize = record.spInfo.nUncompressedSize;

        listResult.append(record);

        nIndex++;

        if ((nLimit != -1) && (nIndex > nLimit)) {
            break;
        }

        nOffset += (0x200);
        nOffset += align_up(record.nDataSize, 0x200);
    }

    return listResult;
}

QString XTAR::getFileFormatExt()
{
    return "tar";
}

QString XTAR::getFileFormatExtsString()
{
    return "TAR (*.tar)";
}

QString XTAR::getMIMEString()
{
    return "application/x-tar";
}

QString XTAR::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XTAR_STRUCTID, sizeof(_TABLE_XTAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

qint32 XTAR::readTableRow(qint32 nRow, LT locType, XADDR nLocation, const DATA_RECORDS_OPTIONS &dataRecordsOptions, QList<DATA_RECORD_ROW> *pListDataRecords,
                          void *pUserData, PDSTRUCT *pPdStruct)
{
    qint32 nResult = 0;

    if (dataRecordsOptions.dataHeaderFirst.dsID.nID == STRUCTID_POSIX_HEADER) {
        nResult = XBinary::readTableRow(nRow, locType, nLocation, dataRecordsOptions, pListDataRecords, pUserData, pPdStruct);

        qint64 nStartOffset = locationToOffset(dataRecordsOptions.pMemoryMap, locType, nLocation);

        XTAR::posix_header header = read_posix_header(nStartOffset);

        nResult = 0x200 + align_up(_getSize(header), 0x200);
    } else {
        nResult = XBinary::readTableRow(nRow, locType, nLocation, dataRecordsOptions, pListDataRecords, pUserData, pPdStruct);
    }

    return nResult;
}

qint32 XTAR::_getNumberOf_posix_headers(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    qint32 nResult = 0;

    while (isPdStructNotCanceled(pPdStruct)) {
        XTAR::posix_header header = read_posix_header(nOffset);

        if (!compareMemory(header.magic, "ustar", 5)) {
            break;
        }

        nResult++;

        nOffset += (0x200);
        nOffset += align_up(_getSize(header), 0x200);
    }

    return nResult;
}

QList<XBinary::DATA_HEADER> XTAR::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<XBinary::DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
        _dataHeadersOptions.nCount = _getNumberOf_posix_headers(0, pPdStruct);
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;
        _dataHeadersOptions.nID = STRUCTID_POSIX_HEADER;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_POSIX_HEADER) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XTAR::structIDToString(dataHeadersOptions.nID));

                dataHeader.listRecords.append(getDataRecord(0, 100, "Name", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(100, 8, "Mode", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(108, 8, "UID", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(116, 8, "GID", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(124, 12, "Size", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(136, 12, "MTime", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(148, 8, "Checksum", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(156, 1, "Typeflag", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(157, 100, "Linkname", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(257, 6, "Magic", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(263, 2, "Version", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(265, 32, "Uname", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(297, 32, "Gname", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(329, 8, "Devmajor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(337, 8, "Devminor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(345, 155, "Prefix", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.nSize = 500;  // TODO const

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XTAR::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XBinary::FPART> listResult;

    qint64 nOffset = 0;
    qint32 nCount = 0;

    while (isPdStructNotCanceled(pPdStruct)) {
        XTAR::posix_header header = read_posix_header(nOffset);

        if (!compareMemory(header.magic, "ustar", 5)) {
            break;
        }

        if (nFileParts & FILEPART_HEADER) {
            XBinary::FPART record = {};
            record.filePart = FILEPART_HEADER;
            record.nFileOffset = nOffset;
            record.nFileSize = 0x200;  // TODO const
            record.nVirtualAddress = -1;
            record.sName = tr("Header");
            listResult.append(record);
        }

        if (nFileParts & FILEPART_STREAM) {
            qint64 nRawSize = _getSize(header);
            qint64 nAlignedSize = align_up(nRawSize, 0x200);
            
            XBinary::FPART record = {};
            record.filePart = FILEPART_STREAM;
            record.nFileOffset = nOffset + 0x200;
            record.nFileSize = nAlignedSize;  // Padded size in archive
            record.nVirtualAddress = -1;
            record.sName = header.name;
            record.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_STORE);
            record.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, nAlignedSize);
            record.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nRawSize);  // Actual file size
            record.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, QString(header.name));
            // TODO Checksum
            listResult.append(record);
        }

        nOffset += (0x200);
        nOffset += align_up(_getSize(header), 0x200);
        
        nCount++;
    }

    if (nFileParts & FILEPART_DATA) {
        XBinary::FPART record = {};
        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = nOffset;  // Total size of the file
        record.nVirtualAddress = -1;
        record.sName = tr("Data");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_OVERLAY) && (nOffset < getSize())) {
        XBinary::FPART record = {};
        record.filePart = FILEPART_OVERLAY;
        record.nFileOffset = nOffset;
        record.nFileSize = getSize() - nOffset;
        record.nVirtualAddress = -1;
        record.sName = tr("Overlay");
        listResult.append(record);
    }

    return listResult;
}

XBinary::_MEMORY_MAP XTAR::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_DATA;  // Default mode
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

QList<XBinary::MAPMODE> XTAR::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::FT XTAR::getFileType()
{
    return FT_TAR;
}

XTAR::posix_header XTAR::read_posix_header(qint64 nOffset)
{
    posix_header record = {};

    read_array(nOffset, (char *)&record, sizeof(record));

    return record;
}

qint64 XTAR::_getSize(const posix_header &header)
{
    // Parse size field - trim whitespace as some tar implementations pad with leading zeros and trailing spaces
    // Standard format: null-terminated octal string
    // Some implementations: "00000000400 " (11 digits + space)
    QString sSizeField = QString(QByteArray(header.size, 12)).trimmed();
    return sSizeField.toULongLong(0, 8);
}

bool XTAR::packFolderToDevice(const QString &sFolderName, QIODevice *pDevice, void *pOptions, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (!XBinary::isDirectoryExists(sFolderName)) {
        return false;
    }

    if (!pDevice || !pDevice->isWritable()) {
        return false;
    }

    QList<QString> listFiles;
    XBinary::findFiles(sFolderName, &listFiles, true, 0, pPdStruct);

    qint32 nNumberOfFiles = listFiles.count();

    for (qint32 i = 0; (i < nNumberOfFiles) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        QString sFilePath = listFiles.at(i);

        QFileInfo fileInfo(sFilePath);

        if (fileInfo.isDir()) {
            continue;
        }

        QString sRelativePath = sFilePath.mid(sFolderName.length());

        if (sRelativePath.startsWith("/") || sRelativePath.startsWith("\\")) {
            sRelativePath = sRelativePath.mid(1);
        }

        sRelativePath.replace("\\", "/");

        qint64 nFileSize = fileInfo.size();
        quint32 nMode = 0;
        qint64 nMTime = fileInfo.lastModified().toSecsSinceEpoch();

#ifdef Q_OS_WIN
        nMode = 00644;  // Octal: owner read/write, group/others read
#else
        QFile::Permissions permissions = fileInfo.permissions();

        if (permissions & QFile::ReadOwner) nMode |= 0400;
        if (permissions & QFile::WriteOwner) nMode |= 0200;
        if (permissions & QFile::ExeOwner) nMode |= 0100;
        if (permissions & QFile::ReadGroup) nMode |= 0040;
        if (permissions & QFile::WriteGroup) nMode |= 0020;
        if (permissions & QFile::ExeGroup) nMode |= 0010;
        if (permissions & QFile::ReadOther) nMode |= 0004;
        if (permissions & QFile::WriteOther) nMode |= 0002;
        if (permissions & QFile::ExeOther) nMode |= 0001;
#endif

        posix_header header = createHeader(sRelativePath, "", nFileSize, nMode, nMTime);
        
        // Write header (500 bytes)
        if (pDevice->write((char *)&header, sizeof(posix_header)) != sizeof(posix_header)) {
            return false;
        }
        
        // Pad header to 512 bytes
        qint64 nHeaderPadding = 512 - sizeof(posix_header);
        if (nHeaderPadding > 0) {
            QByteArray baHeaderPadding(nHeaderPadding, 0);
            if (pDevice->write(baHeaderPadding) != nHeaderPadding) {
                return false;
            }
        }

        QFile file(sFilePath);

        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }

        qint64 nBytesWritten = 0;

        while (nBytesWritten < nFileSize && XBinary::isPdStructNotCanceled(pPdStruct)) {
            QByteArray baBuffer = file.read(qMin((qint64)0x10000, nFileSize - nBytesWritten));

            if (baBuffer.isEmpty()) {
                file.close();
                return false;
            }

            if (pDevice->write(baBuffer) != baBuffer.size()) {
                file.close();
                return false;
            }

            nBytesWritten += baBuffer.size();
        }

        file.close();

        qint64 nPadding = (512 - (nFileSize % 512)) % 512;

        if (nPadding > 0) {
            QByteArray baPadding(nPadding, 0);

            if (pDevice->write(baPadding) != nPadding) {
                return false;
            }
        }
    }

    if (XBinary::isPdStructNotCanceled(pPdStruct)) {
        QByteArray baTerminator(1024, 0);

        if (pDevice->write(baTerminator) != 1024) {
            return false;
        }

        bResult = true;
    }

    return bResult;
}

XTAR::posix_header XTAR::createHeader(const QString &sFileName, const QString &sBasePath, qint64 nFileSize, quint32 nMode, qint64 nMTime)
{
    posix_header header;

    // Zero out the entire header
    memset(&header, 0, sizeof(posix_header));

    // Build full name
    QString sFullName = sBasePath;

    if (!sFullName.isEmpty() && !sFullName.endsWith("/")) {
        sFullName += "/";
    }

    sFullName += sFileName;

    // Copy name (max 100 bytes)
    QByteArray baName = sFullName.toUtf8();
    qint32 nNameLen = qMin(baName.size(), 100);
    
    if (nNameLen > 0) {
        memcpy(header.name, baName.constData(), nNameLen);
    }

    // Write fields in octal format
    writeOctal(header.mode, sizeof(header.mode), nMode);
    writeOctal(header.uid, sizeof(header.uid), 0);
    writeOctal(header.gid, sizeof(header.gid), 0);
    writeOctal(header.size, sizeof(header.size), nFileSize);
    writeOctal(header.mtime, sizeof(header.mtime), nMTime);

    // Checksum field filled with spaces initially
    memset(header.chksum, ' ', 8);

    // Type flag (regular file)
    header.typeflag[0] = '0';

    // Magic and version (ustar format)
    memcpy(header.magic, "ustar", 5);
    header.magic[5] = 0;
    memcpy(header.version, "00", 2);

    // Calculate and write checksum
    quint32 nChecksum = calculateChecksum(header);

    writeOctal(header.chksum, sizeof(header.chksum) - 1, nChecksum);
    header.chksum[6] = 0;
    header.chksum[7] = ' ';

    return header;
}

quint32 XTAR::calculateChecksum(const posix_header &header)
{
    quint32 nChecksum = 0;
    const unsigned char *pData = (const unsigned char *)&header;

    for (qint32 i = 0; i < (qint32)sizeof(posix_header); i++) {
        nChecksum += pData[i];
    }

    return nChecksum;
}

void XTAR::writeOctal(char *pDest, qint32 nSize, qint64 nValue)
{
    memset(pDest, 0, nSize);

    QString sOctal = QString::number(nValue, 8);
    QByteArray baOctal = sOctal.toUtf8();

    qint32 nLength = qMin(baOctal.size(), nSize - 1);

    if (nLength > 0) {
        memcpy(pDest, baOctal.constData(), nLength);
    }
}

bool XTAR::initUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 0;
        pState->pContext = nullptr;

        // Count total number of records
        qint64 nOffset = 0;
        qint64 nTotalSize = pState->nTotalSize;

        while ((nOffset < nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            posix_header header = read_posix_header(nOffset);
            
            // Check for end of archive (empty header)
            if (header.name[0] == 0) {
                break;
            }

            qint64 nFileSize = _getSize(header);
            qint64 nRecordSize = 512 + ((nFileSize + 511) / 512) * 512;

            pState->nNumberOfRecords++;
            nOffset += nRecordSize;
        }

        bResult = (pState->nNumberOfRecords > 0);
    }

    return bResult;
}

XBinary::ARCHIVERECORD XTAR::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        posix_header header = read_posix_header(pState->nCurrentOffset);
        qint64 nFileSize = _getSize(header);

        result.nStreamOffset = pState->nCurrentOffset + 512;
        result.nStreamSize = nFileSize;
        result.nDecompressedOffset = 0;
        result.nDecompressedSize = nFileSize;

        // Extract file name
        QString sFileName = QString::fromUtf8(header.name, qMin((qint32)sizeof(header.name), (qint32)100));
        qint32 nNullPos = sFileName.indexOf(QChar('\0'));
        if (nNullPos != -1) {
            sFileName = sFileName.left(nNullPos);
        }

        result.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, sFileName);
        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_STORE);
    }

    return result;
}

bool XTAR::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pDevice && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        posix_header header = read_posix_header(pState->nCurrentOffset);
        qint64 nFileSize = _getSize(header);
        qint64 nDataOffset = pState->nCurrentOffset + 512;

        // Copy data directly (no compression in TAR)
        bResult = copyDeviceMemory(getDevice(), nDataOffset, pDevice, 0, nFileSize);
    }

    return bResult;
}

bool XTAR::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        posix_header header = read_posix_header(pState->nCurrentOffset);
        qint64 nFileSize = _getSize(header);
        qint64 nRecordSize = 512 + ((nFileSize + 511) / 512) * 512;

        pState->nCurrentOffset += nRecordSize;
        pState->nCurrentIndex++;

        bResult = (pState->nCurrentIndex < pState->nNumberOfRecords);
    }

    return bResult;
}
