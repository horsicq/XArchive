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

#include <limits>
#include <memory>
#include <new>

XBinary::XCONVERT _TABLE_XCPIO_STRUCTID[] = {{XCPIO::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                             {XCPIO::STRUCTID_NEWC_HEADER, "NEWC_HEADER", QString("CPIO newc header")},
                                             {XCPIO::STRUCTID_CRC_HEADER, "CRC_HEADER", QString("CPIO CRC header")},
                                             {XCPIO::STRUCTID_ODC_HEADER, "ODC_HEADER", QString("CPIO odc header")},
                                             {XCPIO::STRUCTID_AFIO_HEADER, "AFIO_HEADER", QString("CPIO afio large-ASCII header")},
                                             {XCPIO::STRUCTID_BINARY_HEADER, "BINARY_HEADER", QString("CPIO binary header")}};

static const quint32 CPIO_MODE_IFMT = 0170000;
static const quint32 CPIO_MODE_IFDIR = 0040000;
static const qint32 CPIO_MAX_RECORDS = 0x100000;

XCPIO::XCPIO(QIODevice *pDevice) : XArchive(pDevice)
{
}

XCPIO::~XCPIO()
{
}

bool XCPIO::isValid(PDSTRUCT *pPdStruct)
{
    return _scanArchive(-1, nullptr, nullptr, pPdStruct);
}

bool XCPIO::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCPIO xcpio(pDevice);
    return xcpio.isValid(pPdStruct);
}

XCPIO::CPIO_FORMAT XCPIO::_detectFormat(qint64 nOffset)
{
    QPointer<XCPIO> guardedThis(this);
    CPIO_FORMAT result = CPIO_FORMAT_UNKNOWN;

    const qint64 nTotalSize = getSize();
    if (!guardedThis) return result;
    if ((nOffset < 0) || (nOffset > nTotalSize)) {
        return result;
    }
    const qint64 nAvailableSize = nTotalSize - nOffset;

    if (nAvailableSize < 2) {
        return result;
    }

    if (nAvailableSize >= 6) {
        char szMagic[7] = {0};
        if (read_array_process(nOffset, szMagic, 6, nullptr) != 6) {
            return result;
        }
        if (!guardedThis) return result;

        QString sMagic = QString::fromLatin1(szMagic, 6);

        if (sMagic == "070701") {
            return CPIO_FORMAT_NEWC;
        } else if (sMagic == "070702") {
            return CPIO_FORMAT_CRC;
        } else if (sMagic == "070707") {
            return CPIO_FORMAT_ODC;
        } else if (sMagic == "070727") {
            return CPIO_FORMAT_AFIO;
        }
    }

    const quint16 nMagicLE = read_uint16(nOffset);
    if (!guardedThis) return result;
    if (nMagicLE == 0x71C7) {
        result = CPIO_FORMAT_BINARY_LE;
    } else {
        const quint16 nMagicBE = read_uint16(nOffset, true);
        if (!guardedThis) return result;
        if (nMagicBE == 0x71C7) result = CPIO_FORMAT_BINARY_BE;
    }

    return result;
}

qint64 XCPIO::_readHexValue(const char *pValue, qint32 nSize)
{
    if (!pValue || nSize <= 0) {
        return -1;
    }

    qint64 nResult = 0;
    for (qint32 i = 0; i < nSize; i++) {
        const quint8 nCharacter = (quint8)pValue[i];
        qint32 nDigit = -1;
        if ((nCharacter >= '0') && (nCharacter <= '9')) {
            nDigit = nCharacter - '0';
        } else if ((nCharacter >= 'a') && (nCharacter <= 'f')) {
            nDigit = nCharacter - 'a' + 10;
        } else if ((nCharacter >= 'A') && (nCharacter <= 'F')) {
            nDigit = nCharacter - 'A' + 10;
        }
        if ((nDigit < 0) || (nResult > (((std::numeric_limits<qint64>::max)() - nDigit) / 16))) {
            return -1;
        }
        nResult = nResult * 16 + nDigit;
    }

    return nResult;
}

qint64 XCPIO::_readOctValue(const char *pValue, qint32 nSize)
{
    if (!pValue || nSize <= 0) {
        return -1;
    }

    qint64 nResult = 0;
    for (qint32 i = 0; i < nSize; i++) {
        const quint8 nCharacter = (quint8)pValue[i];
        if ((nCharacter < '0') || (nCharacter > '7')) {
            return -1;
        }
        const qint32 nDigit = nCharacter - '0';
        if (nResult > (((std::numeric_limits<qint64>::max)() - nDigit) / 8)) {
            return -1;
        }
        nResult = nResult * 8 + nDigit;
    }

    return nResult;
}

quint16 XCPIO::_readBinaryUInt16(qint64 nOffset, bool bIsBigEndian)
{
    return read_uint16(nOffset, bIsBigEndian);
}

quint32 XCPIO::_readBinaryUInt32(qint64 nOffset, bool bIsBigEndian)
{
    QPointer<XCPIO> guardedThis(this);
    quint32 nHigh = _readBinaryUInt16(nOffset, bIsBigEndian);
    if (!guardedThis) return 0;
    quint32 nLow = _readBinaryUInt16(nOffset + 2, bIsBigEndian);
    if (!guardedThis) return 0;

    return (nHigh << 16) | nLow;
}

XCPIO::CPIO_NEWC_HEADER XCPIO::_readNewcHeader(qint64 nOffset)
{
    CPIO_NEWC_HEADER header = {};
    read_array_process(nOffset, (char *)&header, sizeof(CPIO_NEWC_HEADER), nullptr);
    return header;
}

XCPIO::CPIO_ODC_HEADER XCPIO::_readOdcHeader(qint64 nOffset)
{
    CPIO_ODC_HEADER header = {};
    read_array_process(nOffset, (char *)&header, sizeof(CPIO_ODC_HEADER), nullptr);
    return header;
}

XCPIO::CPIO_AFIO_HEADER XCPIO::_readAfioHeader(qint64 nOffset)
{
    CPIO_AFIO_HEADER header = {};
    read_array_process(nOffset, (char *)&header, sizeof(CPIO_AFIO_HEADER), nullptr);
    return header;
}

bool XCPIO::_parseRecord(qint64 nOffset, CPIO_RECORD_INFO *pInfo, PDSTRUCT *pPdStruct)
{
    QPointer<XCPIO> guardedThis(this);
    if ((!pInfo) || (nOffset < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    *pInfo = {};
    pInfo->format = _detectFormat(nOffset);
    if (!guardedThis) return false;
    pInfo->nHeaderOffset = nOffset;

    if (pInfo->format == CPIO_FORMAT_UNKNOWN) {
        return false;
    }

    const qint64 nTotalSize = getSize();
    if (!guardedThis) return false;
    qint64 nNameSize = 0;
    qint64 nDataSize = 0;
    qint64 nExpectedCheck = -1;

    if ((pInfo->format == CPIO_FORMAT_NEWC) || (pInfo->format == CPIO_FORMAT_CRC)) {
        if ((nOffset > nTotalSize) || ((qint64)sizeof(CPIO_NEWC_HEADER) > (nTotalSize - nOffset))) {
            return false;
        }

        CPIO_NEWC_HEADER header = _readNewcHeader(nOffset);
        if (!guardedThis) return false;

        const char *pFields[] = {header.ino,      header.mode,     header.uid,       header.gid,       header.nlink,    header.mtime, header.filesize,
                                 header.devmajor, header.devminor, header.rdevmajor, header.rdevminor, header.namesize, header.check};
        for (qint32 i = 0; i < (qint32)(sizeof(pFields) / sizeof(pFields[0])); i++) {
            if (_readHexValue(pFields[i], 8) < 0) {
                return false;
            }
        }

        pInfo->nHeaderSize = sizeof(CPIO_NEWC_HEADER);
        nNameSize = _readHexValue(header.namesize, 8);
        nDataSize = _readHexValue(header.filesize, 8);
        pInfo->nMode = (quint32)_readHexValue(header.mode, 8);
        pInfo->nUID = (quint32)_readHexValue(header.uid, 8);
        pInfo->nGID = (quint32)_readHexValue(header.gid, 8);
        pInfo->nNLink = (quint32)_readHexValue(header.nlink, 8);
        pInfo->nMTime = (quint64)_readHexValue(header.mtime, 8);
        nExpectedCheck = _readHexValue(header.check, 8);
    } else if (pInfo->format == CPIO_FORMAT_ODC) {
        if ((nOffset > nTotalSize) || ((qint64)sizeof(CPIO_ODC_HEADER) > (nTotalSize - nOffset))) {
            return false;
        }

        CPIO_ODC_HEADER header = _readOdcHeader(nOffset);
        if (!guardedThis) return false;

        if ((_readOctValue(header.dev, 6) < 0) || (_readOctValue(header.ino, 6) < 0) || (_readOctValue(header.mode, 6) < 0) || (_readOctValue(header.uid, 6) < 0) ||
            (_readOctValue(header.gid, 6) < 0) || (_readOctValue(header.nlink, 6) < 0) || (_readOctValue(header.rdev, 6) < 0) || (_readOctValue(header.mtime, 11) < 0) ||
            (_readOctValue(header.namesize, 6) < 0) || (_readOctValue(header.filesize, 11) < 0)) {
            return false;
        }

        pInfo->nHeaderSize = sizeof(CPIO_ODC_HEADER);
        nNameSize = _readOctValue(header.namesize, 6);
        nDataSize = _readOctValue(header.filesize, 11);
        pInfo->nMode = (quint32)_readOctValue(header.mode, 6);
        pInfo->nUID = (quint32)_readOctValue(header.uid, 6);
        pInfo->nGID = (quint32)_readOctValue(header.gid, 6);
        pInfo->nNLink = (quint32)_readOctValue(header.nlink, 6);
        pInfo->nRDev = (quint32)_readOctValue(header.rdev, 6);
        pInfo->nMTime = (quint64)_readOctValue(header.mtime, 11);
    } else if (pInfo->format == CPIO_FORMAT_AFIO) {
        if ((nOffset > nTotalSize) || ((qint64)sizeof(CPIO_AFIO_HEADER) > (nTotalSize - nOffset))) {
            return false;
        }

        CPIO_AFIO_HEADER header = _readAfioHeader(nOffset);
        if (!guardedThis) return false;

        if ((header.inoMarker != 'm') || (header.mtimeMarker != 'n') || (header.xsizeMarker != 's') || (header.filesizeMarker != ':') ||
            (_readHexValue(header.dev, 8) < 0) || (_readHexValue(header.ino, 16) < 0) || (_readOctValue(header.mode, 6) < 0) || (_readHexValue(header.uid, 8) < 0) ||
            (_readHexValue(header.gid, 8) < 0) || (_readHexValue(header.nlink, 8) < 0) || (_readHexValue(header.rdev, 8) < 0) || (_readHexValue(header.mtime, 16) < 0) ||
            (_readHexValue(header.namesize, 4) < 0) || (_readHexValue(header.flag, 4) < 0) || (_readHexValue(header.xsize, 4) < 0) ||
            (_readHexValue(header.filesize, 16) < 0)) {
            return false;
        }

        pInfo->nHeaderSize = sizeof(CPIO_AFIO_HEADER);
        nNameSize = _readHexValue(header.namesize, 4);
        nDataSize = _readHexValue(header.filesize, 16);
        pInfo->nMode = (quint32)_readOctValue(header.mode, 6);
        pInfo->nUID = (quint32)_readHexValue(header.uid, 8);
        pInfo->nGID = (quint32)_readHexValue(header.gid, 8);
        pInfo->nNLink = (quint32)_readHexValue(header.nlink, 8);
        pInfo->nRDev = (quint32)_readHexValue(header.rdev, 8);
        pInfo->nMTime = (quint64)_readHexValue(header.mtime, 16);
    } else {
        bool bIsBigEndian = (pInfo->format == CPIO_FORMAT_BINARY_BE);

        if ((nOffset > nTotalSize) || ((qint64)sizeof(CPIO_BINARY_HEADER) > (nTotalSize - nOffset))) {
            return false;
        }

        pInfo->nHeaderSize = sizeof(CPIO_BINARY_HEADER);
        nNameSize = _readBinaryUInt16(nOffset + offsetof(CPIO_BINARY_HEADER, namesize), bIsBigEndian);
        if (!guardedThis) return false;
        nDataSize = _readBinaryUInt32(nOffset + offsetof(CPIO_BINARY_HEADER, filesizeHigh), bIsBigEndian);
        if (!guardedThis) return false;
        pInfo->nMode = _readBinaryUInt16(nOffset + offsetof(CPIO_BINARY_HEADER, mode), bIsBigEndian);
        if (!guardedThis) return false;
        pInfo->nUID = _readBinaryUInt16(nOffset + offsetof(CPIO_BINARY_HEADER, uid), bIsBigEndian);
        if (!guardedThis) return false;
        pInfo->nGID = _readBinaryUInt16(nOffset + offsetof(CPIO_BINARY_HEADER, gid), bIsBigEndian);
        if (!guardedThis) return false;
        pInfo->nNLink = _readBinaryUInt16(nOffset + offsetof(CPIO_BINARY_HEADER, nlink), bIsBigEndian);
        if (!guardedThis) return false;
        pInfo->nRDev = _readBinaryUInt16(nOffset + offsetof(CPIO_BINARY_HEADER, rdev), bIsBigEndian);
        if (!guardedThis) return false;
        pInfo->nMTime = _readBinaryUInt32(nOffset + offsetof(CPIO_BINARY_HEADER, mtimeHigh), bIsBigEndian);
        if (!guardedThis) return false;
    }

    if ((nNameSize <= 0) || (nNameSize > 0x10000) || (nDataSize < 0) || ((pInfo->format == CPIO_FORMAT_CRC) && (nExpectedCheck < 0))) {
        return false;
    }

    if ((nOffset > nTotalSize) || (pInfo->nHeaderSize > (nTotalSize - nOffset))) {
        return false;
    }
    const qint64 nNameOffset = nOffset + pInfo->nHeaderSize;
    if (nNameSize > (nTotalSize - nNameOffset)) {
        return false;
    }
    const qint64 nNameEnd = nNameOffset + nNameSize;

    QByteArray baName = read_array_process(nNameOffset, nNameSize, pPdStruct);
    if (!guardedThis) return false;

    if ((baName.size() != nNameSize) || baName.isEmpty() || (baName.back() != '\0')) {
        return false;
    }

    baName.chop(1);
    if (baName.contains('\0')) {
        return false;
    }

    pInfo->sFileName = QString::fromLatin1(baName.constData(), baName.size());

    qint64 nDataOffset = nNameEnd;

    if ((pInfo->format == CPIO_FORMAT_NEWC) || (pInfo->format == CPIO_FORMAT_CRC)) {
        if (nDataOffset > ((std::numeric_limits<qint64>::max)() - 3)) {
            return false;
        }
        nDataOffset = (nDataOffset + 3) & ~((qint64)3);
    } else if ((pInfo->format == CPIO_FORMAT_BINARY_LE) || (pInfo->format == CPIO_FORMAT_BINARY_BE)) {
        if (nDataOffset == (std::numeric_limits<qint64>::max)()) {
            return false;
        }
        nDataOffset = (nDataOffset + 1) & ~((qint64)1);
    }

    if ((nDataOffset < 0) || (nDataOffset > nTotalSize) || (nDataSize > (nTotalSize - nDataOffset))) {
        return false;
    }

    qint64 nNextOffset = nDataOffset + nDataSize;

    if ((pInfo->format == CPIO_FORMAT_NEWC) || (pInfo->format == CPIO_FORMAT_CRC)) {
        if (nNextOffset > ((std::numeric_limits<qint64>::max)() - 3)) {
            return false;
        }
        nNextOffset = (nNextOffset + 3) & ~((qint64)3);
    } else if ((pInfo->format == CPIO_FORMAT_BINARY_LE) || (pInfo->format == CPIO_FORMAT_BINARY_BE)) {
        if (nNextOffset == (std::numeric_limits<qint64>::max)()) {
            return false;
        }
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

    if (pInfo->format == CPIO_FORMAT_CRC) {
        quint32 nCalculatedCheck = 0;
        qint64 nCurrentOffset = nDataOffset;
        qint64 nRemaining = nDataSize;
        const qint32 nBufferCapacity = 0x4000;
        std::unique_ptr<char[]> pBuffer(new (std::nothrow) char[nBufferCapacity]);
        if (!pBuffer) {
            return false;
        }

        while ((nRemaining > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            const qint32 nChunkSize = (qint32)qMin<qint64>(nBufferCapacity, nRemaining);
            const qint64 nRead = read_array_process(nCurrentOffset, pBuffer.get(), nChunkSize, pPdStruct);
            if (!guardedThis || (nRead != nChunkSize)) {
                return false;
            }
            for (qint32 i = 0; i < nChunkSize; i++) {
                nCalculatedCheck += (quint8)pBuffer[i];
            }
            nCurrentOffset += nChunkSize;
            nRemaining -= nChunkSize;
        }

        if (!XBinary::isPdStructNotCanceled(pPdStruct) || (nRemaining != 0) || (nCalculatedCheck != (quint32)nExpectedCheck)) {
            return false;
        }
    }

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XCPIO::_scanArchive(qint32 nLimit, QList<RECORD> *pListRecords, qint64 *pArchiveEnd, PDSTRUCT *pPdStruct)
{
    QPointer<XCPIO> guardedThis(this);
    if (pListRecords) {
        pListRecords->clear();
    }
    if (pArchiveEnd) {
        *pArchiveEnd = 0;
    }
    if ((nLimit < -1) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    qint64 nOffset = 0;
    const qint64 nTotalSize = getSize();
    if (!guardedThis) return false;
    qint32 nRecordCount = 0;
    bool bSawTrailer = false;

    while ((nOffset < nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        CPIO_RECORD_INFO info = {};
        const bool bParsed = _parseRecord(nOffset, &info, pPdStruct);
        if (!guardedThis) return false;
        if (!bParsed) {
            break;
        }

        if (_isTrailerRecord(info.sFileName)) {
            if (info.nDataSize != 0) {
                break;
            }
            bSawTrailer = true;
            if (pArchiveEnd) {
                *pArchiveEnd = info.nNextOffset;
            }
            break;
        }

        if (nRecordCount >= CPIO_MAX_RECORDS) {
            break;
        }
        nRecordCount++;

        if (pListRecords && ((nLimit == -1) || (pListRecords->count() < nLimit))) {
            RECORD record = {};
            record.spInfo.sRecordName = info.sFileName;
            record.spInfo.nUncompressedSize = info.nDataSize;
            record.spInfo.compressMethod = HANDLE_METHOD_STORE;
            record.nHeaderOffset = info.nHeaderOffset;
            record.nHeaderSize = info.nHeaderSize;
            record.nDataOffset = info.nDataOffset;
            record.nDataSize = info.nDataSize;
            pListRecords->append(record);
        }

        nOffset = info.nNextOffset;
    }

    const bool bResult = bSawTrailer && XBinary::isPdStructNotCanceled(pPdStruct);
    if (!bResult && pListRecords) {
        pListRecords->clear();
    }
    if (!bResult && pArchiveEnd) {
        *pArchiveEnd = 0;
    }
    return bResult;
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
    listResult.append("'070727'");
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

    if ((nLimit < -1) || (nLimit == 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    _scanArchive(nLimit, &listResult, nullptr, pPdStruct);

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

// QList<XBinary::DATA_HEADER> XCPIO::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

//         CPIO_FORMAT format = _detectFormat(0);
//         if (format == CPIO_FORMAT_NEWC || format == CPIO_FORMAT_CRC) {
//             _dataHeadersOptions.nID = (format == CPIO_FORMAT_NEWC) ? STRUCTID_NEWC_HEADER : STRUCTID_CRC_HEADER;
//         } else if (format == CPIO_FORMAT_ODC) {
//             _dataHeadersOptions.nID = STRUCTID_ODC_HEADER;
//         } else if ((format == CPIO_FORMAT_BINARY_LE) || (format == CPIO_FORMAT_BINARY_BE)) {
//             _dataHeadersOptions.nID = STRUCTID_BINARY_HEADER;
//         }

//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;
//         _dataHeadersOptions.nCount = getNumberOfRecords(pPdStruct);

//         if (_dataHeadersOptions.nID != STRUCTID_UNKNOWN) {
//             listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//         }
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             CPIO_FORMAT format = _detectFormat(nStartOffset);

//             if (format == CPIO_FORMAT_NEWC || format == CPIO_FORMAT_CRC) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCPIO::structIDToString(dataHeadersOptions.nID));

//                 dataHeader.nSize = sizeof(CPIO_NEWC_HEADER);

//                 dataHeader.listRecords.append(getDataRecord(0, 6, "Magic", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(6, 8, "Inode", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(14, 8, "Mode", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(22, 8, "UID", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(30, 8, "GID", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(38, 8, "Nlink", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(46, 8, "MTime", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(54, 8, "Filesize", VT_CHAR_ARRAY, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(62, 8, "DevMajor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(70, 8, "DevMinor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(78, 8, "RDevMajor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(86, 8, "RDevMinor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(94, 8, "Namesize", VT_CHAR_ARRAY, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(102, 8, "Check", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);
//             } else if (format == CPIO_FORMAT_ODC) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCPIO::structIDToString(dataHeadersOptions.nID));

//                 dataHeader.nSize = sizeof(CPIO_ODC_HEADER);

//                 dataHeader.listRecords.append(getDataRecord(0, 6, "Magic", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(6, 6, "Device", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(12, 6, "Inode", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(18, 6, "Mode", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(24, 6, "UID", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(30, 6, "GID", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(36, 6, "Nlink", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(42, 6, "RDev", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(48, 11, "MTime", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(59, 6, "Namesize", VT_CHAR_ARRAY, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(65, 11, "Filesize", VT_CHAR_ARRAY, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);
//             } else if ((format == CPIO_FORMAT_BINARY_LE) || (format == CPIO_FORMAT_BINARY_BE)) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCPIO::structIDToString(dataHeadersOptions.nID));

//                 dataHeader.nSize = sizeof(CPIO_BINARY_HEADER);

//                 dataHeader.listRecords.append(getDataRecord(0, 2, "Magic", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(2, 2, "Device", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(4, 2, "Inode", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(6, 2, "Mode", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(8, 2, "UID", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(10, 2, "GID", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(12, 2, "Nlink", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(14, 2, "RDev", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(16, 2, "MTimeHigh", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(18, 2, "MTimeLow", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(20, 2, "Namesize", VT_UINT16, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(22, 2, "FilesizeHigh", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(24, 2, "FilesizeLow", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);
//             }
//         }
//     }

//     return listResult;
// }

QList<XBinary::XFHEADER> XCPIO::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XBinary::XFHEADER> listResult;
    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        CPIO_FORMAT format = _detectFormat(0);

        if ((format == CPIO_FORMAT_NEWC) || (format == CPIO_FORMAT_CRC)) {
            nStructID = (format == CPIO_FORMAT_NEWC) ? STRUCTID_NEWC_HEADER : STRUCTID_CRC_HEADER;
        } else if (format == CPIO_FORMAT_ODC) {
            nStructID = STRUCTID_ODC_HEADER;
        } else if (format == CPIO_FORMAT_AFIO) {
            nStructID = STRUCTID_AFIO_HEADER;
        } else if ((format == CPIO_FORMAT_BINARY_LE) || (format == CPIO_FORMAT_BINARY_BE)) {
            nStructID = STRUCTID_BINARY_HEADER;
        }

        if (nStructID != STRUCTID_UNKNOWN) {
            XFSTRUCT _xfStruct = xfStruct;
            _xfStruct.nStructID = nStructID;
            _xfStruct.xLoc = offsetToLoc(0);
            listResult.append(getXFHeaders(_xfStruct, pPdStruct));
        }
    } else if ((nStructID == STRUCTID_NEWC_HEADER) || (nStructID == STRUCTID_CRC_HEADER) || (nStructID == STRUCTID_ODC_HEADER) || (nStructID == STRUCTID_AFIO_HEADER) ||
               (nStructID == STRUCTID_BINARY_HEADER)) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);
        qint64 nHeaderSize = 0;

        if ((nStructID == STRUCTID_NEWC_HEADER) || (nStructID == STRUCTID_CRC_HEADER)) {
            nHeaderSize = sizeof(CPIO_NEWC_HEADER);
        } else if (nStructID == STRUCTID_ODC_HEADER) {
            nHeaderSize = sizeof(CPIO_ODC_HEADER);
        } else if (nStructID == STRUCTID_AFIO_HEADER) {
            nHeaderSize = sizeof(CPIO_AFIO_HEADER);
        } else if (nStructID == STRUCTID_BINARY_HEADER) {
            nHeaderSize = sizeof(CPIO_BINARY_HEADER);
        }

        if ((nHeaderOffset != -1) && (nHeaderSize > 0) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, nHeaderSize)) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(nStructID);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = nHeaderSize;
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, nStructID, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(nStructID), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XCPIO::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if ((nStructID == STRUCTID_NEWC_HEADER) || (nStructID == STRUCTID_CRC_HEADER)) {
        listResult.append({"Magic", (qint32)offsetof(CPIO_NEWC_HEADER, magic), (qint32)sizeof(((CPIO_NEWC_HEADER *)0)->magic), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"Inode", (qint32)offsetof(CPIO_NEWC_HEADER, ino), (qint32)sizeof(((CPIO_NEWC_HEADER *)0)->ino), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"Mode", (qint32)offsetof(CPIO_NEWC_HEADER, mode), (qint32)sizeof(((CPIO_NEWC_HEADER *)0)->mode), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"UID", (qint32)offsetof(CPIO_NEWC_HEADER, uid), (qint32)sizeof(((CPIO_NEWC_HEADER *)0)->uid), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"GID", (qint32)offsetof(CPIO_NEWC_HEADER, gid), (qint32)sizeof(((CPIO_NEWC_HEADER *)0)->gid), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"Nlink", (qint32)offsetof(CPIO_NEWC_HEADER, nlink), (qint32)sizeof(((CPIO_NEWC_HEADER *)0)->nlink), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"MTime", (qint32)offsetof(CPIO_NEWC_HEADER, mtime), (qint32)sizeof(((CPIO_NEWC_HEADER *)0)->mtime), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append(
            {"Filesize", (qint32)offsetof(CPIO_NEWC_HEADER, filesize), (qint32)sizeof(((CPIO_NEWC_HEADER *)0)->filesize), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append(
            {"DevMajor", (qint32)offsetof(CPIO_NEWC_HEADER, devmajor), (qint32)sizeof(((CPIO_NEWC_HEADER *)0)->devmajor), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append(
            {"DevMinor", (qint32)offsetof(CPIO_NEWC_HEADER, devminor), (qint32)sizeof(((CPIO_NEWC_HEADER *)0)->devminor), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append(
            {"RDevMajor", (qint32)offsetof(CPIO_NEWC_HEADER, rdevmajor), (qint32)sizeof(((CPIO_NEWC_HEADER *)0)->rdevmajor), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append(
            {"RDevMinor", (qint32)offsetof(CPIO_NEWC_HEADER, rdevminor), (qint32)sizeof(((CPIO_NEWC_HEADER *)0)->rdevminor), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append(
            {"Namesize", (qint32)offsetof(CPIO_NEWC_HEADER, namesize), (qint32)sizeof(((CPIO_NEWC_HEADER *)0)->namesize), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"Check", (qint32)offsetof(CPIO_NEWC_HEADER, check), (qint32)sizeof(((CPIO_NEWC_HEADER *)0)->check), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
    } else if (nStructID == STRUCTID_ODC_HEADER) {
        listResult.append({"Magic", (qint32)offsetof(CPIO_ODC_HEADER, magic), (qint32)sizeof(((CPIO_ODC_HEADER *)0)->magic), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"Device", (qint32)offsetof(CPIO_ODC_HEADER, dev), (qint32)sizeof(((CPIO_ODC_HEADER *)0)->dev), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"Inode", (qint32)offsetof(CPIO_ODC_HEADER, ino), (qint32)sizeof(((CPIO_ODC_HEADER *)0)->ino), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"Mode", (qint32)offsetof(CPIO_ODC_HEADER, mode), (qint32)sizeof(((CPIO_ODC_HEADER *)0)->mode), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"UID", (qint32)offsetof(CPIO_ODC_HEADER, uid), (qint32)sizeof(((CPIO_ODC_HEADER *)0)->uid), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"GID", (qint32)offsetof(CPIO_ODC_HEADER, gid), (qint32)sizeof(((CPIO_ODC_HEADER *)0)->gid), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"Nlink", (qint32)offsetof(CPIO_ODC_HEADER, nlink), (qint32)sizeof(((CPIO_ODC_HEADER *)0)->nlink), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"RDev", (qint32)offsetof(CPIO_ODC_HEADER, rdev), (qint32)sizeof(((CPIO_ODC_HEADER *)0)->rdev), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"MTime", (qint32)offsetof(CPIO_ODC_HEADER, mtime), (qint32)sizeof(((CPIO_ODC_HEADER *)0)->mtime), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"Namesize", (qint32)offsetof(CPIO_ODC_HEADER, namesize), (qint32)sizeof(((CPIO_ODC_HEADER *)0)->namesize), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"Filesize", (qint32)offsetof(CPIO_ODC_HEADER, filesize), (qint32)sizeof(((CPIO_ODC_HEADER *)0)->filesize), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
    } else if (nStructID == STRUCTID_AFIO_HEADER) {
        listResult.append({"Magic", (qint32)offsetof(CPIO_AFIO_HEADER, magic), (qint32)sizeof(((CPIO_AFIO_HEADER *)0)->magic), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"Device", (qint32)offsetof(CPIO_AFIO_HEADER, dev), (qint32)sizeof(((CPIO_AFIO_HEADER *)0)->dev), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"Inode", (qint32)offsetof(CPIO_AFIO_HEADER, ino), (qint32)sizeof(((CPIO_AFIO_HEADER *)0)->ino), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"Mode", (qint32)offsetof(CPIO_AFIO_HEADER, mode), (qint32)sizeof(((CPIO_AFIO_HEADER *)0)->mode), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"UID", (qint32)offsetof(CPIO_AFIO_HEADER, uid), (qint32)sizeof(((CPIO_AFIO_HEADER *)0)->uid), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"GID", (qint32)offsetof(CPIO_AFIO_HEADER, gid), (qint32)sizeof(((CPIO_AFIO_HEADER *)0)->gid), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"Nlink", (qint32)offsetof(CPIO_AFIO_HEADER, nlink), (qint32)sizeof(((CPIO_AFIO_HEADER *)0)->nlink), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"RDev", (qint32)offsetof(CPIO_AFIO_HEADER, rdev), (qint32)sizeof(((CPIO_AFIO_HEADER *)0)->rdev), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"MTime", (qint32)offsetof(CPIO_AFIO_HEADER, mtime), (qint32)sizeof(((CPIO_AFIO_HEADER *)0)->mtime), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append(
            {"Namesize", (qint32)offsetof(CPIO_AFIO_HEADER, namesize), (qint32)sizeof(((CPIO_AFIO_HEADER *)0)->namesize), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append(
            {"Filesize", (qint32)offsetof(CPIO_AFIO_HEADER, filesize), (qint32)sizeof(((CPIO_AFIO_HEADER *)0)->filesize), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
    } else if (nStructID == STRUCTID_BINARY_HEADER) {
        listResult.append({"Magic", (qint32)offsetof(CPIO_BINARY_HEADER, magic), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"Device", (qint32)offsetof(CPIO_BINARY_HEADER, dev), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"Inode", (qint32)offsetof(CPIO_BINARY_HEADER, ino), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"Mode", (qint32)offsetof(CPIO_BINARY_HEADER, mode), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"UID", (qint32)offsetof(CPIO_BINARY_HEADER, uid), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"GID", (qint32)offsetof(CPIO_BINARY_HEADER, gid), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"Nlink", (qint32)offsetof(CPIO_BINARY_HEADER, nlink), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"RDev", (qint32)offsetof(CPIO_BINARY_HEADER, rdev), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"MTimeHigh", (qint32)offsetof(CPIO_BINARY_HEADER, mtimeHigh), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"MTimeLow", (qint32)offsetof(CPIO_BINARY_HEADER, mtimeLow), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"Namesize", (qint32)offsetof(CPIO_BINARY_HEADER, namesize), 2, XFRECORD_FLAG_COUNT, VT_UINT16});
        listResult.append({"FilesizeHigh", (qint32)offsetof(CPIO_BINARY_HEADER, filesizeHigh), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"FilesizeLow", (qint32)offsetof(CPIO_BINARY_HEADER, filesizeLow), 2, XFRECORD_FLAG_NONE, VT_UINT16});
    }

    return listResult;
}

static bool cpioCanAppendPart(XBinary::PDSTRUCT *pPdStruct, qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return XBinary::isPdStructNotCanceled(pPdStruct) && ((nLimit == -1) || (listResult.count() < nLimit));
}

QList<XBinary::FPART> XCPIO::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    const qint64 nTotalSize = getSize();
    qint64 nOffset = 0;
    qint64 nArchiveEnd = 0;
    qint32 nRecordCount = 0;
    bool bSawTrailer = false;
    bool bParseError = false;

    while ((nOffset < nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        CPIO_RECORD_INFO info = {};

        if (!_parseRecord(nOffset, &info, pPdStruct)) {
            bParseError = true;
            break;
        }

        nArchiveEnd = info.nNextOffset;

        if (!_isTrailerRecord(info.sFileName)) {
            if (nRecordCount >= CPIO_MAX_RECORDS) {
                bParseError = true;
                break;
            }
            nRecordCount++;

            if ((nFileParts & FILEPART_HEADER) && cpioCanAppendPart(pPdStruct, nLimit, listResult)) {
                FPART header = {};
                header.filePart = FILEPART_HEADER;
                header.nFileOffset = info.nHeaderOffset;
                header.nFileSize = info.nHeaderSize;
                header.nVirtualAddress = XADDR_MAX;
                header.sName = info.sFileName.isEmpty() ? tr("Header") : QString("%1 (%2)").arg(info.sFileName).arg(tr("Header"));
                listResult.append(header);
            }

            if ((nFileParts & FILEPART_REGION) && cpioCanAppendPart(pPdStruct, nLimit, listResult)) {
                FPART region = {};
                region.filePart = FILEPART_REGION;
                region.nFileOffset = info.nDataOffset;
                region.nFileSize = info.nDataSize;
                region.nVirtualAddress = XADDR_MAX;
                region.sName = info.sFileName;
                listResult.append(region);
            }

        } else {
            if (info.nDataSize != 0) {
                bParseError = true;
                break;
            }
            if ((nFileParts & FILEPART_HEADER) && cpioCanAppendPart(pPdStruct, nLimit, listResult)) {
                FPART trailer = {};
                trailer.filePart = FILEPART_HEADER;
                trailer.nFileOffset = info.nHeaderOffset;
                trailer.nFileSize = info.nHeaderSize;
                trailer.nVirtualAddress = XADDR_MAX;
                trailer.sName = tr("Trailer");
                listResult.append(trailer);
            }
            bSawTrailer = true;
            break;
        }

        nOffset = info.nNextOffset;
    }

    if ((nFileParts & FILEPART_OVERLAY) && cpioCanAppendPart(pPdStruct, nLimit, listResult) && bSawTrailer && (nArchiveEnd < nTotalSize)) {
        FPART record = {};
        record.filePart = FILEPART_OVERLAY;
        record.nFileOffset = nArchiveEnd;
        record.nFileSize = nTotalSize - nArchiveEnd;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Overlay");

        listResult.append(record);
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || bParseError || !bSawTrailer) {
        listResult.clear();
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XCPIO::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XCPIO::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XCPIO> guardedThis(this);
    if (m_bUnpackOperationInProgress) {
        return false;
    }
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    CPIO_UNPACK_CONTEXT *pOldContext = static_cast<CPIO_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !isPdStructNotCanceled(pPdStruct)) return false;
    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;

    pState->mapUnpackProperties = mapProperties;

    const CPIO_FORMAT format = _detectFormat(0);
    if (!guardedThis) return false;
    qint64 nHeaderSize = 0;
    if ((format == CPIO_FORMAT_NEWC) || (format == CPIO_FORMAT_CRC)) {
        nHeaderSize = sizeof(CPIO_NEWC_HEADER);
    } else if (format == CPIO_FORMAT_ODC) {
        nHeaderSize = sizeof(CPIO_ODC_HEADER);
    } else if (format == CPIO_FORMAT_AFIO) {
        nHeaderSize = sizeof(CPIO_AFIO_HEADER);
    } else if ((format == CPIO_FORMAT_BINARY_LE) || (format == CPIO_FORMAT_BINARY_BE)) {
        nHeaderSize = sizeof(CPIO_BINARY_HEADER);
    } else {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    QList<RECORD> listRecords;
    const bool bScanned = _scanArchive(-1, &listRecords, nullptr, pPdStruct);
    if (!guardedThis) return false;
    if (!bScanned) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    if (!isPdStructNotCanceled(pPdStruct)) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    const qint64 nTotalSize = getSize();
    if (!guardedThis) return false;
    CPIO_UNPACK_CONTEXT *pContext = new (std::nothrow) CPIO_UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    pContext->format = format;
    pContext->nHeaderSize = nHeaderSize;
    pContext->listRecords = listRecords;
    pContext->nCurrentRecord = 0;

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listRecords.count();
    pState->nCurrentOffset = (pState->nNumberOfRecords > 0) ? pContext->listRecords.at(0).nHeaderOffset : 0;
    pState->nTotalSize = nTotalSize;

    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XArchive::ARCHIVERECORD XCPIO::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XCPIO> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();

    ARCHIVERECORD result = {};

    if (!pState || !pState->pContext) {
        return result;
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return result;
    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nTotalSize != nCurrentSize) || (pState->nNumberOfRecords < 0)) return result;

    CPIO_UNPACK_CONTEXT *pContext = (CPIO_UNPACK_CONTEXT *)pState->pContext;

    if ((pState->nCurrentIndex >= 0) && (pState->nCurrentIndex < pContext->listRecords.count())) {
        const RECORD &record = pContext->listRecords.at(pState->nCurrentIndex);
        CPIO_RECORD_INFO info = {};

        const bool bParsed = _parseRecord(record.nHeaderOffset, &info, pPdStruct);
        if (!guardedThis) return ARCHIVERECORD();
        if (!bParsed || (info.nHeaderOffset != record.nHeaderOffset) || (info.nHeaderSize != record.nHeaderSize) || (info.nDataOffset != record.nDataOffset) ||
            (info.nDataSize != record.nDataSize) || (info.sFileName != record.spInfo.sRecordName)) {
            return ARCHIVERECORD();
        }

        result.nStreamOffset = record.nDataOffset;
        result.nStreamSize = record.nDataSize;
        result.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, record.spInfo.sRecordName);
        result.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, record.spInfo.nUncompressedSize);
        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, record.nDataSize);
        result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_STORE);
        result.mapProperties.insert(XBinary::FPART_PROP_HEADER_OFFSET, record.nHeaderOffset);
        result.mapProperties.insert(XBinary::FPART_PROP_HEADER_SIZE, record.nHeaderSize);

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

    return result;
}

bool XCPIO::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XCPIO> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState || !pState->pContext) {
        return false;
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return false;
    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords) || (pState->nTotalSize != nCurrentSize)) return false;

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
    QPointer<XCPIO> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    CPIO_UNPACK_CONTEXT *pContext = static_cast<CPIO_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    delete pContext;
    Q_UNUSED(guardedThis)
    return true;
}

bool XCPIO::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XCPIO> guardedThis(this);
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo = static_cast<XArchive::INTERNAL_INFO *>(guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(guardedThis->m_internalInfo) = *pInfo;
    }

    return guardedThis && bResult;
}

void *XCPIO::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XCPIO> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XCPIO::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
