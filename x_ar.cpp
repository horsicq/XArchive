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

XBinary::_MEMORY_MAP X_Ar::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }
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
