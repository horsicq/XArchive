/* Copyright (c) 2022-2025 hors<horsicq@gmail.com>
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
#include "x_ar.h"

XBinary::XCONVERT _TABLE_XAr_STRUCTID[] = {
    {X_Ar::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {X_Ar::STRUCTID_FRECORD, "FRECORD", QString("FRECORD")},
    {X_Ar::STRUCTID_SIGNATURE, "Signature", QObject::tr("Signature")},
};

X_Ar::X_Ar(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool X_Ar::isValid(PDSTRUCT *pPdStruct)
{
    // TODO more checks
    bool bResult = false;

    _MEMORY_MAP memoryMap = XBinary::getSimpleMemoryMap();

    if (getSize() > (qint64)(8 + sizeof(RECORD)))  // TODO const
    {
        if (compareSignature(&memoryMap, "'!<arch>'0a", 0, pPdStruct)) {
            FRECORD frecord = readFRECORD(8);

            if ((frecord.endChar[0] == 0x60) && (frecord.endChar[1] == 0x0a)) {
                bResult = true;
            }
        }
    }

    return bResult;
}

bool X_Ar::isValid(QIODevice *pDevice)
{
    X_Ar x_ar(pDevice);

    return x_ar.isValid();
}

quint64 X_Ar::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    return _getNumberOfStreams(8, pPdStruct);
}

QList<XArchive::RECORD> X_Ar::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listRecords;

    qint64 nOffset = 0;
    qint64 nSize = getSize();

    nOffset += 8;
    nSize -= 8;

    QString sList;

    qint32 nIndex = 0;

    while ((nSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        RECORD record = {};
        record.nHeaderOffset = nOffset;

        FRECORD frecord = readFRECORD(nOffset);

        QString sSize = QString(frecord.fileSize);

        sSize.resize(sizeof(frecord.fileSize));

        qint32 nRecordSize = sSize.trimmed().toULongLong();

        if (nRecordSize == 0) {
            break;
        }

        record.spInfo.sRecordName = frecord.fileId;
        record.spInfo.sRecordName.resize(sizeof(frecord.fileId));
        record.spInfo.sRecordName = record.spInfo.sRecordName.trimmed();

        if (record.spInfo.sRecordName == "//")  // Linux/GNU
        {
            sList = read_ansiString(nOffset + sizeof(FRECORD), nRecordSize);
        }

        if (record.spInfo.sRecordName.section("/", 0, 0) == "#1")  // BSD style
        {
            qint32 nFileNameLength = record.spInfo.sRecordName.section("/", 1, 1).toInt();

            record.spInfo.sRecordName = read_ansiString(nOffset + sizeof(FRECORD), nFileNameLength);  // TODO Check UTF8

            record.nDataOffset = nOffset + sizeof(FRECORD) + nFileNameLength;
            record.nDataSize = nRecordSize - nFileNameLength;
            record.spInfo.nUncompressedSize = nRecordSize - nFileNameLength;
            record.nHeaderSize = record.nDataOffset - record.nHeaderOffset;
        } else {
            qint32 nFileNameSie = record.spInfo.sRecordName.size();

            if (nFileNameSie >= 2)  // Linux/GNU
            {
                if ((record.spInfo.sRecordName.at(0) == QChar('/')) && (record.spInfo.sRecordName.at(nFileNameSie - 1) != QChar('/'))) {
                    qint32 nIndex = record.spInfo.sRecordName.section("/", 1, 1).toULong();

                    if (nIndex < sList.size()) {
                        if (nIndex) {
                            record.spInfo.sRecordName = sList.right(nIndex).section("/", 0, 0);
                        } else {
                            record.spInfo.sRecordName = sList.section("/", 0, 0);
                        }
                    }
                } else if ((nFileNameSie > 2) && (record.spInfo.sRecordName.at(nFileNameSie - 1) == QChar('/'))) {
                    record.spInfo.sRecordName.remove(nFileNameSie - 1, 1);
                }
            }

            record.nDataOffset = nOffset + sizeof(FRECORD);
            record.nDataSize = nRecordSize;
            record.spInfo.nUncompressedSize = nRecordSize;
        }

        if (record.nDataSize < 0) {
            record.nDataSize = 0;
        }

        if (record.spInfo.nUncompressedSize < 0) {
            record.spInfo.nUncompressedSize = 0;
        }

        record.spInfo.compressMethod = COMPRESS_METHOD_STORE;

        listRecords.append(record);

        nOffset += sizeof(FRECORD);
        nOffset += S_ALIGN_UP(nRecordSize, 2);

        nSize -= sizeof(FRECORD);
        nSize -= S_ALIGN_UP(nRecordSize, 2);

        nIndex++;

        if ((nLimit != -1) && (nIndex >= nLimit)) {
            break;
        }
    }

    return listRecords;
}

QString X_Ar::getFileFormatExt()
{
    return "ar";
}

QList<XBinary::MAPMODE> X_Ar::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::FT X_Ar::getFileType()
{
    return XBinary::FT_AR;
}

qint32 X_Ar::getType()
{
    return TYPE_PACKAGE;
}

QString X_Ar::getMIMEString()
{
    return "application/x-archive";
}

QString X_Ar::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XAr_STRUCTID, sizeof(_TABLE_XAr_STRUCTID) / sizeof(XBinary::XCONVERT));
}

bool X_Ar::readTableInit(const DATA_RECORDS_OPTIONS &dataRecordsOptions, void **ppUserData, PDSTRUCT *pPdStruct)
{
    if (dataRecordsOptions.dataHeaderFirst.dsID.nID == STRUCTID_FRECORD) {
        qint64 nStartOffset = locationToOffset(dataRecordsOptions.pMemoryMap, dataRecordsOptions.dataHeaderFirst.locType, dataRecordsOptions.dataHeaderFirst.nLocation);

        if (nStartOffset != -1) {
            TABLE_LIST *pTableList = new TABLE_LIST;
            qint64 nOffset = nStartOffset;
            qint64 nTotalSize = getSize();

            while ((nOffset + (qint64)sizeof(FRECORD) <= nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                FRECORD frecord = readFRECORD(nOffset);

                // Validate member terminator
                if (!((frecord.endChar[0] == 0x60) && (frecord.endChar[1] == 0x0a))) {
                    break;
                }

                QString sSize = QString(frecord.fileSize);
                sSize.resize(sizeof(frecord.fileSize));
                qint64 nRecordSize = sSize.trimmed().toLongLong();
                if (nRecordSize < 0) {
                    break;
                }

                QString sName = QString(frecord.fileId);
                sName.resize(sizeof(frecord.fileId));
                sName = sName.trimmed();

                qint64 nDataSize = nRecordSize;

                // BSD style filename stored in data area immediately after header
                if (sName.section('/', 0, 0) == "#1") {
                    qint32 nFileNameLength = sName.section('/', 1, 1).toInt();
                    if (nFileNameLength > 0) {
                        if (nRecordSize >= nFileNameLength) {
                            nDataSize = nRecordSize - (qint64)nFileNameLength;
                        } else {
                            // Malformed entry; avoid underflow
                            nDataSize = 0;
                        }
                    }
                }

                if (nDataSize < 0) {
                    nDataSize = 0;
                }

                XBinary::OFFSETSIZE os = {};
                os.nOffset = nOffset;
                os.nSize = nDataSize;
                pTableList->listOffsetsSizes.append(os);

                qint64 nStep = (qint64)sizeof(FRECORD) + (qint64)S_ALIGN_UP(nRecordSize, 2);
                nOffset += nStep;
            }

            *ppUserData = (void *)pTableList;
        }
    }

    return true;
}

QList<XBinary::DATA_HEADER> X_Ar::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<XBinary::DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

        _dataHeadersOptions.locType = XBinary::LT_OFFSET;
        _dataHeadersOptions.nID = STRUCTID_SIGNATURE;
        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_SIGNATURE) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(dataHeadersOptions.nID));
                dataHeader.listRecords.append(getDataRecord(0, 8, "Magic", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.nSize = 8;
                listResult.append(dataHeader);

                DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
                _dataHeadersOptions.nLocation += 8;
                _dataHeadersOptions.dsID_parent = dataHeader.dsID;
                _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
                _dataHeadersOptions.nID = STRUCTID_FRECORD;
                _dataHeadersOptions.nCount = _getNumberOfStreams(8, pPdStruct);
                listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
            } else if (dataHeadersOptions.nID == STRUCTID_FRECORD) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(dataHeadersOptions.nID));

                dataHeader.listRecords.append(getDataRecord(offsetof(FRECORD, fileId), sizeof(((FRECORD *)0)->fileId), "FileId", VT_CHAR_ARRAY, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(FRECORD, fileMod), sizeof(((FRECORD *)0)->fileMod), "FileMod", VT_CHAR_ARRAY, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(FRECORD, ownerId), sizeof(((FRECORD *)0)->ownerId), "OwnerId", VT_CHAR_ARRAY, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(FRECORD, groupId), sizeof(((FRECORD *)0)->groupId), "GroupId", VT_CHAR_ARRAY, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(FRECORD, fileMode), sizeof(((FRECORD *)0)->fileMode), "FileMode", VT_CHAR_ARRAY, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(FRECORD, fileSize), sizeof(((FRECORD *)0)->fileSize), "FileSize", VT_CHAR_ARRAY, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(FRECORD, endChar), sizeof(((FRECORD *)0)->endChar), "EndChar", VT_BYTE_ARRAY, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.nSize = sizeof(FRECORD);

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

qint32 X_Ar::readTableRow(qint32 nRow, LT locType, XADDR nLocation, const DATA_RECORDS_OPTIONS &dataRecordsOptions, QList<DATA_RECORD_ROW> *pListDataRecords,
                          void *pUserData, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pUserData)
    qint32 nResult = 0;

    if (dataRecordsOptions.dataHeaderFirst.dsID.nID == STRUCTID_FRECORD) {
        TABLE_LIST *pTableList = (TABLE_LIST *)pUserData;

        if (pTableList) {
            if (nRow < pTableList->listOffsetsSizes.count()) {
                OFFSETSIZE os = pTableList->listOffsetsSizes.at(nRow);

                XBinary::readTableRow(nRow, LT_OFFSET, os.nOffset, dataRecordsOptions, pListDataRecords, pUserData, pPdStruct);
                nResult = os.nSize;
            }
        }
    } else {
        nResult = XBinary::readTableRow(nRow, locType, nLocation, dataRecordsOptions, pListDataRecords, pUserData, pPdStruct);
    }

    return nResult;
}

void X_Ar::readTableFinalize(const DATA_RECORDS_OPTIONS &dataRecordsOptions, void *pUserData, PDSTRUCT *pPdStruct)
{
    if (dataRecordsOptions.dataHeaderFirst.dsID.nID == STRUCTID_FRECORD) {
        TABLE_LIST *pTableList = (TABLE_LIST *)pUserData;

        if (pTableList) {
            delete pTableList;
        }
    }
}

quint64 X_Ar::_getNumberOfStreams(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    quint64 nResult = 0;

    qint64 nSize = getSize() - nOffset;

    while ((nSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        FRECORD frecord = readFRECORD(nOffset);

        // Validate header terminator for safety
        if (!((frecord.endChar[0] == 0x60) && (frecord.endChar[1] == 0x0a))) {
            break;
        }

        QString sSize = QString(frecord.fileSize);
        sSize.resize(sizeof(frecord.fileSize));
        qint64 nRecordSize = sSize.trimmed().toLongLong();

        if (nRecordSize <= 0) {
            break;
        }

        // BSD style name: "#1/<len>" has embedded filename immediately after header
        QString sName = QString(frecord.fileId);
        sName.resize(sizeof(frecord.fileId));
        sName = sName.trimmed();
        if (sName.section('/', 0, 0) == "#1") {
            qint32 nFileNameLength = sName.section('/', 1, 1).toInt();
            if (nRecordSize < nFileNameLength) {
                break;  // malformed; avoid underflow/loop
            }
        }

        qint64 nStep = (qint64)sizeof(FRECORD) + (qint64)S_ALIGN_UP(nRecordSize, 2);
        nOffset += nStep;
        nSize -= nStep;
        nResult++;
    }

    return nResult;
}

qint64 X_Ar::getNumberOfArchiveRecords(PDSTRUCT *pPdStruct)
{
    return _getNumberOfStreams(8, pPdStruct);
}

XBinary::_MEMORY_MAP X_Ar::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

X_Ar::FRECORD X_Ar::readFRECORD(qint64 nOffset)
{
    FRECORD record = {};

    read_array(nOffset + offsetof(FRECORD, fileId), record.fileId, sizeof(record.fileId));
    read_array(nOffset + offsetof(FRECORD, fileMod), record.fileMod, sizeof(record.fileMod));
    read_array(nOffset + offsetof(FRECORD, ownerId), record.ownerId, sizeof(record.ownerId));
    read_array(nOffset + offsetof(FRECORD, groupId), record.groupId, sizeof(record.groupId));
    read_array(nOffset + offsetof(FRECORD, fileMode), record.fileMode, sizeof(record.fileMode));
    read_array(nOffset + offsetof(FRECORD, fileSize), record.fileSize, sizeof(record.fileSize));
    read_array(nOffset + offsetof(FRECORD, endChar), record.endChar, sizeof(record.endChar));

    return record;
}

QList<XBinary::FPART> X_Ar::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    const qint64 fileSize = getSize();
    if (fileSize < 8) return listResult;

    qint64 nOffset = 0;
    qint64 nSize = fileSize;

    // Header magic
    if (nFileParts & FILEPART_HEADER) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = qMin<qint64>(8, fileSize);
        header.nVirtualAddress = -1;
        header.sName = tr("Header");
        listResult.append(header);
    }

    nOffset += 8;
    nSize -= 8;

    qint32 nIndex = 0;

    while ((nSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        FRECORD frecord = readFRECORD(nOffset);

        QString sSize = QString(frecord.fileSize);
        sSize.resize(sizeof(frecord.fileSize));
        qint64 nRecordSize = sSize.trimmed().toLongLong();
        if (nRecordSize <= 0) break;

        QString sOriginalName = QString(frecord.fileId);
        sOriginalName.resize(sizeof(frecord.fileId));
        sOriginalName = sOriginalName.trimmed();

        qint64 dataOffset = nOffset + (qint64)sizeof(FRECORD);
        qint64 dataSize = nRecordSize;
        qint32 nFileNameLength = 0;

        if (sOriginalName == "//") {
            // GNU table of long filenames; still a data region
        } else if (sOriginalName.section('/', 0, 0) == "#1") {  // BSD style
            nFileNameLength = sOriginalName.section('/', 1, 1).toInt();
            dataOffset += nFileNameLength;
            dataSize -= nFileNameLength;
        } else if (sOriginalName.size() >= 2) {
            if ((sOriginalName.at(0) == QChar('/')) && (sOriginalName.at(sOriginalName.size() - 1) != QChar('/'))) {
                // Name comes from the string table; keep raw for part name
            } else if ((sOriginalName.size() > 2) && (sOriginalName.at(sOriginalName.size() - 1) == QChar('/'))) {
                sOriginalName.remove(sOriginalName.size() - 1, 1);
            }
        }

        if (dataSize < 0) dataSize = 0;

        if (nFileParts & FILEPART_HEADER) {
            FPART h = {};
            h.filePart = FILEPART_HEADER;
            h.nFileOffset = nOffset;
            h.nFileSize = (qint64)sizeof(FRECORD) + (qint64)nFileNameLength;  // FRECORD + optional BSD filename
            h.nVirtualAddress = -1;
            h.sName = tr("Header");
            listResult.append(h);
        }

        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = dataOffset;
            part.nFileSize = dataSize;
            part.nVirtualAddress = -1;
            part.sName = tr("Record") + QString(" %1").arg(nIndex);
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME, sOriginalName);
            // Properties: ar stores raw bytes, no compression
            part.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, dataSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, dataSize);
            // Optional: modification time (ASCII epoch seconds in header)
            QString sMod = QString(frecord.fileMod);
            sMod.resize(sizeof(frecord.fileMod));
            sMod = sMod.trimmed();
            bool bOk = false;
            quint64 nModSecs = sMod.toULongLong(&bOk, 10);
            if (bOk) {
                QDateTime dt = QDateTime::fromSecsSinceEpoch((qint64)nModSecs, Qt::UTC);
                part.mapProperties.insert(FPART_PROP_DATETIME, dt);
            }
            listResult.append(part);
        }

        // advance to next, 2-byte aligned
        qint64 nAligned = S_ALIGN_UP(nRecordSize, 2);
        nOffset += (qint64)sizeof(FRECORD) + nAligned;
        nSize -= (qint64)sizeof(FRECORD) + nAligned;
        nIndex++;

        if ((nLimit != -1) && (nIndex >= nLimit)) break;
    }
    // Total parsed data area
    if (nFileParts & FILEPART_DATA) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = 8;
        data.nFileSize = nOffset - 8;
        data.nVirtualAddress = -1;
        data.sName = tr("Data");
        listResult.append(data);
    }

    // Trailing bytes after the last record are overlay
    if ((nFileParts & FILEPART_OVERLAY) && (nOffset < fileSize)) {
        FPART overlay = {};
        overlay.filePart = FILEPART_OVERLAY;
        overlay.nFileOffset = nOffset;
        overlay.nFileSize = fileSize - nOffset;
        overlay.nVirtualAddress = -1;
        overlay.sName = tr("Overlay");
        listResult.append(overlay);
    }

    return listResult;
}

bool X_Ar::packFolderToDevice(const QString &sFolderName, QIODevice *pDevice, void *pOptions, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pOptions)

    bool bResult = false;

    if (!XBinary::isDirectoryExists(sFolderName)) {
        return false;
    }

    if (!pDevice || !pDevice->isWritable()) {
        return false;
    }

    // Write AR header
    QByteArray baHeader = "!<arch>\n";
    if (pDevice->write(baHeader) != baHeader.size()) {
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

        // AR format traditionally stores only basenames (no directory paths)
        // This matches the behavior of system 'ar' command
        QFileInfo relativeFileInfo(sRelativePath);
        QString sBaseName = relativeFileInfo.fileName();

        qint64 nFileSize = fileInfo.size();
        quint32 nMode = 0;
        qint64 nMTime = fileInfo.lastModified().toSecsSinceEpoch();

#ifdef Q_OS_WIN
        nMode = 0644;  // owner read/write, group/others read
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

        // Check if we need BSD-style long filename format
        QByteArray baFileName = sBaseName.toUtf8();
        bool bUseBsdFormat = (baFileName.size() > 15);
        
        FRECORD header = createHeader(sBaseName, nFileSize, nMode, nMTime);

        // Write header
        if (pDevice->write((char *)&header, sizeof(FRECORD)) != sizeof(FRECORD)) {
            return false;
        }

        // If using BSD format, write the filename first
        if (bUseBsdFormat) {
            if (pDevice->write(baFileName) != baFileName.size()) {
                return false;
            }
        }

        // Write file content
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

        // Add padding if total size (filename + file content for BSD, or just file content for standard) is odd
        qint64 nTotalDataSize = nFileSize;
        if (bUseBsdFormat) {
            nTotalDataSize += baFileName.size();
        }
        
        if (nTotalDataSize % 2 != 0) {
            char cPadding = '\n';
            if (pDevice->write(&cPadding, 1) != 1) {
                return false;
            }
        }
    }

    if (XBinary::isPdStructNotCanceled(pPdStruct)) {
        bResult = true;
    }

    return bResult;
}

X_Ar::FRECORD X_Ar::createHeader(const QString &sFileName, qint64 nFileSize, quint32 nMode, qint64 nMTime)
{
    FRECORD header;

    // Zero out the entire header
    memset(&header, 0x20, sizeof(FRECORD));  // AR uses spaces for padding

    QByteArray baName = sFileName.toUtf8();
    
    // Check if filename fits in standard 16-byte field (minus 1 for trailing '/')
    // If not, we'll use BSD-style format with #1/<length>
    if (baName.size() <= 15) {
        // Standard format: filename with trailing '/'
        qint32 nNameLen = baName.size();
        if (nNameLen > 0) {
            memcpy(header.fileId, baName.constData(), nNameLen);
        }
        // Add trailing '/' as per AR format
        if (nNameLen < (qint32)sizeof(header.fileId)) {
            header.fileId[nNameLen] = '/';
        }
    } else {
        // BSD-style format: #1/<length>/
        // The actual filename will be stored at the beginning of the file data
        QString sBsdFormat = QString("#1/%1").arg(baName.size());
        QByteArray baBsdFormat = sBsdFormat.toLatin1();
        qint32 nBsdLen = qMin(baBsdFormat.size(), (qint32)sizeof(header.fileId));
        if (nBsdLen > 0) {
            memcpy(header.fileId, baBsdFormat.constData(), nBsdLen);
        }
        // Add trailing '/' if there's room
        if (nBsdLen < (qint32)sizeof(header.fileId)) {
            header.fileId[nBsdLen] = '/';
        }
        // Adjust file size to include the embedded filename
        nFileSize += baName.size();
    }

    // Write mtime as decimal string
    QString sMTime = QString::number(nMTime);
    QByteArray baMTime = sMTime.toLatin1();
    qint32 nMTimeLen = qMin(baMTime.size(), (qint32)sizeof(header.fileMod));
    if (nMTimeLen > 0) {
        memcpy(header.fileMod, baMTime.constData(), nMTimeLen);
    }

    // OwnerID and GroupID (default to 0)
    QByteArray baOwnerId = "0";
    memcpy(header.ownerId, baOwnerId.constData(), baOwnerId.size());

    QByteArray baGroupId = "0";
    memcpy(header.groupId, baGroupId.constData(), baGroupId.size());

    // File mode as octal string
    QString sMode = QString::number(nMode, 8);
    QByteArray baMode = sMode.toLatin1();
    qint32 nModeLen = qMin(baMode.size(), (qint32)sizeof(header.fileMode));
    if (nModeLen > 0) {
        memcpy(header.fileMode, baMode.constData(), nModeLen);
    }

    // File size as decimal string
    QString sSize = QString::number(nFileSize);
    QByteArray baSize = sSize.toLatin1();
    qint32 nSizeLen = qMin(baSize.size(), (qint32)sizeof(header.fileSize));
    if (nSizeLen > 0) {
        memcpy(header.fileSize, baSize.constData(), nSizeLen);
    }

    // End characters
    header.endChar[0] = 0x60;
    header.endChar[1] = 0x0a;

    return header;
}

bool X_Ar::initUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        pState->nCurrentOffset = 8;  // Start after the "!<arch>\n" signature
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 0;
        pState->pContext = nullptr;

        // Count total number of records
        qint64 nOffset = 8;
        qint64 nTotalSize = pState->nTotalSize;

        while ((nOffset + (qint64)sizeof(FRECORD) <= nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            FRECORD header = readFRECORD(nOffset);

            // Validate header terminator
            if (!((header.endChar[0] == 0x60) && (header.endChar[1] == 0x0a))) {
                break;
            }

            QString sSize = QString(header.fileSize);
            sSize.resize(sizeof(header.fileSize));
            qint64 nFileSize = sSize.trimmed().toLongLong();

            if (nFileSize < 0) {
                break;
            }

            qint64 nRecordSize = sizeof(FRECORD) + S_ALIGN_UP(nFileSize, 2);

            pState->nNumberOfRecords++;
            nOffset += nRecordSize;
        }

        bResult = (pState->nNumberOfRecords > 0);
    }

    return bResult;
}

XBinary::ARCHIVERECORD X_Ar::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        FRECORD header = readFRECORD(pState->nCurrentOffset);

        QString sSize = QString(header.fileSize);
        sSize.resize(sizeof(header.fileSize));
        qint64 nFileSize = sSize.trimmed().toLongLong();

        // Extract file name
        QString sFileName = QString::fromUtf8(header.fileId, (qint32)sizeof(header.fileId));
        sFileName = sFileName.trimmed();

        // Handle BSD-style long names
        if (sFileName.section("/", 0, 0) == "#1") {
            qint32 nFileNameLength = sFileName.section("/", 1, 1).toInt();
            if (nFileNameLength > 0 && nFileNameLength <= nFileSize) {
                sFileName = read_ansiString(pState->nCurrentOffset + sizeof(FRECORD), nFileNameLength);
                result.nStreamOffset = pState->nCurrentOffset + sizeof(FRECORD) + nFileNameLength;
                result.nStreamSize = nFileSize - nFileNameLength;
            } else {
                result.nStreamOffset = pState->nCurrentOffset + sizeof(FRECORD);
                result.nStreamSize = nFileSize;
            }
        } else {
            // Remove trailing '/' if present
            if (sFileName.endsWith('/')) {
                sFileName = sFileName.left(sFileName.length() - 1);
            }

            result.nStreamOffset = pState->nCurrentOffset + sizeof(FRECORD);
            result.nStreamSize = nFileSize;
        }

        result.nDecompressedOffset = 0;
        result.nDecompressedSize = result.nStreamSize;

        result.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, sFileName);
        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_STORE);

        // Parse file mode (octal)
        QString sMode = QString(header.fileMode);
        sMode.resize(sizeof(header.fileMode));
        sMode = sMode.trimmed();
        quint32 nMode = sMode.toUInt(nullptr, 8);
        result.mapProperties.insert(XBinary::FPART_PROP_FILEMODE, nMode);

        // Parse UID (decimal)
        QString sUid = QString(header.ownerId);
        sUid.resize(sizeof(header.ownerId));
        sUid = sUid.trimmed();
        quint32 nUid = sUid.toUInt();
        result.mapProperties.insert(XBinary::FPART_PROP_UID, nUid);

        // Parse GID (decimal)
        QString sGid = QString(header.groupId);
        sGid.resize(sizeof(header.groupId));
        sGid = sGid.trimmed();
        quint32 nGid = sGid.toUInt();
        result.mapProperties.insert(XBinary::FPART_PROP_GID, nGid);

        // Size
        result.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, result.nDecompressedSize);
        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, result.nStreamSize);

        // Parse mtime (decimal)
        QString sMTime = QString(header.fileMod);
        sMTime.resize(sizeof(header.fileMod));
        sMTime = sMTime.trimmed();
        qint64 nMTime = sMTime.toLongLong();
        QDateTime dateTime = QDateTime::fromSecsSinceEpoch(nMTime);
        result.mapProperties.insert(XBinary::FPART_PROP_DATETIME, dateTime);
    }

    return result;
}

bool X_Ar::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pDevice && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        FRECORD header = readFRECORD(pState->nCurrentOffset);

        QString sSize = QString(header.fileSize);
        sSize.resize(sizeof(header.fileSize));
        qint64 nFileSize = sSize.trimmed().toLongLong();

        qint64 nDataOffset = pState->nCurrentOffset + sizeof(FRECORD);
        qint64 nDataSize = nFileSize;

        // Handle BSD-style long names
        QString sFileName = QString::fromUtf8(header.fileId, (qint32)sizeof(header.fileId));
        sFileName = sFileName.trimmed();

        if (sFileName.section("/", 0, 0) == "#1") {
            qint32 nFileNameLength = sFileName.section("/", 1, 1).toInt();
            if (nFileNameLength > 0 && nFileNameLength <= nFileSize) {
                nDataOffset += nFileNameLength;
                nDataSize -= nFileNameLength;
            }
        }

        // Copy data directly (no compression in AR)
        bResult = copyDeviceMemory(getDevice(), nDataOffset, pDevice, 0, nDataSize);
    }

    return bResult;
}

bool X_Ar::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        FRECORD header = readFRECORD(pState->nCurrentOffset);

        QString sSize = QString(header.fileSize);
        sSize.resize(sizeof(header.fileSize));
        qint64 nFileSize = sSize.trimmed().toLongLong();

        qint64 nRecordSize = sizeof(FRECORD) + S_ALIGN_UP(nFileSize, 2);

        pState->nCurrentOffset += nRecordSize;
        pState->nCurrentIndex++;

        bResult = (pState->nCurrentIndex < pState->nNumberOfRecords);
    }

    return bResult;
}

