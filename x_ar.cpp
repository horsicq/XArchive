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

X_Ar::X_Ar(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool X_Ar::isValid(PDSTRUCT *pPdStruct)
{
    // TODO more checks
    bool bResult = false;

    _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

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
    quint64 nResult = 0;

    qint64 nOffset = 0;
    qint64 nSize = getSize();

    nOffset += 8;
    nSize -= 8;

    while ((nSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        char fileSize[16];

        read_array(nOffset + offsetof(FRECORD, fileSize), fileSize, 10);

        QString sSize = QString(fileSize);

        sSize.resize(10);

        qint32 nRecordSize = sSize.trimmed().toULongLong();

        if (nRecordSize == 0) {
            break;
        }

        nOffset += sizeof(FRECORD);
        nOffset += S_ALIGN_UP(nRecordSize, 2);

        nSize -= sizeof(FRECORD);
        nSize -= S_ALIGN_UP(nRecordSize, 2);

        nResult++;
    }

    return nResult;
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

XBinary::_MEMORY_MAP X_Ar::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    // TODO HEADER

    return XBinary::getMemoryMap(mapMode, pPdStruct);
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

    if (!(nFileParts & (FILEPART_REGION | FILEPART_DATA))) {
        return listResult;
    }

    QString sList;
    qint32 nIndex = 0;

    while ((nSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        FRECORD frecord = readFRECORD(nOffset);

        QString sSize = QString(frecord.fileSize);
        sSize.resize(sizeof(frecord.fileSize));
        qint64 nRecordSize = sSize.trimmed().toLongLong();
        if (nRecordSize <= 0) break;

        QString sName = QString(frecord.fileId);
        sName.resize(sizeof(frecord.fileId));
        sName = sName.trimmed();

        qint64 dataOffset = nOffset + (qint64)sizeof(FRECORD);
        qint64 dataSize = nRecordSize;

        if (sName == "//") {
            // GNU table of long filenames; still a data region
        } else if (sName.section('/', 0, 0) == "#1") {  // BSD style
            qint32 nFileNameLength = sName.section('/', 1, 1).toInt();
            dataOffset += nFileNameLength;
            dataSize -= nFileNameLength;
        } else if (sName.size() >= 2) {
            if ((sName.at(0) == QChar('/')) && (sName.at(sName.size() - 1) != QChar('/'))) {
                // Name comes from the string table; keep raw for part name
            } else if ((sName.size() > 2) && (sName.at(sName.size() - 1) == QChar('/'))) {
                sName.remove(sName.size() - 1, 1);
            }
        }

        if (dataSize < 0) dataSize = 0;

        if (nFileParts & FILEPART_REGION) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = dataOffset;
            part.nFileSize = dataSize;
            part.nVirtualAddress = -1;
            part.sName = sName.isEmpty() ? tr("Record %1").arg(nIndex) : sName;
            listResult.append(part);
        }

        // advance to next, 2-byte aligned
        qint64 nAligned = S_ALIGN_UP(nRecordSize, 2);
        nOffset += (qint64)sizeof(FRECORD) + nAligned;
        nSize -= (qint64)sizeof(FRECORD) + nAligned;
        nIndex++;

        if ((nLimit != -1) && (nIndex >= nLimit)) break;
    }

    return listResult;
}
