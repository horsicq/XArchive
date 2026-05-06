/* Copyright (c) 2026 hors<horsicq@gmail.com>
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
#include "xarj.h"
#include "xdecompress.h"

XBinary::XCONVERT _TABLE_XARJ_STRUCTID[] = {
    {XARJ::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XARJ::STRUCTID_HEADER, "HEADER", QString("Header")},
    {XARJ::STRUCTID_RECORD, "RECORD", QString("Record")},
};

namespace {
const qint64 ARJ_MARKER_SIZE = 2;
const qint64 ARJ_BASIC_HEADER_SIZE_FIELD_SIZE = 2;
const qint64 ARJ_ENTRY_PREFIX_SIZE = ARJ_MARKER_SIZE + ARJ_BASIC_HEADER_SIZE_FIELD_SIZE;
const qint64 ARJ_HEADER_CRC_SIZE = 4;
const qint64 ARJ_EXT_HEADER_SIZE_FIELD_SIZE = 2;
const qint64 ARJ_MIN_VALID_SIZE = ARJ_ENTRY_PREFIX_SIZE + XARJ::FIXED_HEADER_SIZE;
const quint16 ARJ_MAX_BASIC_HEADER_SIZE = 2600;
const quint8 ARJ_MARKER_BYTE0 = 0x60;
const quint8 ARJ_MARKER_BYTE1 = 0xEA;
const quint8 ARJ_FLAG_GARBLE = 0x01;

const qint64 ARJ_BASIC_FIRST_HEADER_SIZE = 0;
const qint64 ARJ_BASIC_ARCHIVER_VERSION = 1;
const qint64 ARJ_BASIC_MIN_VERSION = 2;
const qint64 ARJ_BASIC_HOST_OS = 3;
const qint64 ARJ_BASIC_FLAGS = 4;
const qint64 ARJ_BASIC_METHOD = 5;
const qint64 ARJ_BASIC_FILE_TYPE = 6;
const qint64 ARJ_BASIC_PASSWORD_MODIFIER = 7;
const qint64 ARJ_BASIC_DOS_DATE_TIME = 8;
const qint64 ARJ_BASIC_COMPRESSED_SIZE = 12;
const qint64 ARJ_BASIC_ORIGINAL_SIZE = 16;
const qint64 ARJ_BASIC_CRC32 = 20;
const qint64 ARJ_BASIC_ENTRY_NAME_POS = 24;
const qint64 ARJ_BASIC_FILE_ACCESS_MODE = 26;
const qint64 ARJ_BASIC_FIRST_CHAPTER = 28;
const qint64 ARJ_BASIC_LAST_CHAPTER = 29;

struct ARJ_RECORD_FIELD {
    const char *pszDataName;
    const char *pszXFName;
    qint32 nOffset;
    qint32 nSize;
    XBinary::VT valueType;
    quint32 nDataRecordFlags;
    quint64 nXFRecordFlags;
};

const ARJ_RECORD_FIELD g_arjRecordFields[] = {
    {"Marker", "Marker", 0, 2, XBinary::VT_UINT16, XBinary::DRF_UNKNOWN, XBinary::XFRECORD_FLAG_NONE},
    {"Basic Header Size", "Basic Header Size", 2, 2, XBinary::VT_UINT16, XBinary::DRF_SIZE, XBinary::XFRECORD_FLAG_SIZE},
    {"First Hdr Size", "First Header Size", 4, 1, XBinary::VT_UINT8, XBinary::DRF_SIZE, XBinary::XFRECORD_FLAG_SIZE},
    {"Archiver Version", "Archiver Version", 5, 1, XBinary::VT_UINT8, XBinary::DRF_UNKNOWN, XBinary::XFRECORD_FLAG_VERSION},
    {"Min Version", "Min Version", 6, 1, XBinary::VT_UINT8, XBinary::DRF_UNKNOWN, XBinary::XFRECORD_FLAG_VERSION},
    {"Host OS", "Host OS", 7, 1, XBinary::VT_UINT8, XBinary::DRF_UNKNOWN, XBinary::XFRECORD_FLAG_NONE},
    {"ARJ Flags", "ARJ Flags", 8, 1, XBinary::VT_UINT8, XBinary::DRF_UNKNOWN, XBinary::XFRECORD_FLAG_NONE},
    {"Method", "Method", 9, 1, XBinary::VT_UINT8, XBinary::DRF_UNKNOWN, XBinary::XFRECORD_FLAG_NONE},
    {"File Type", "File Type", 10, 1, XBinary::VT_UINT8, XBinary::DRF_UNKNOWN, XBinary::XFRECORD_FLAG_NONE},
    {"Reserved", "Reserved", 11, 1, XBinary::VT_UINT8, XBinary::DRF_UNKNOWN, XBinary::XFRECORD_FLAG_NONE},
    {"Date/Time", "Date/Time", 12, 4, XBinary::VT_UINT32, XBinary::DRF_UNKNOWN, XBinary::XFRECORD_FLAG_NONE},
    {"Compressed Size", "Compressed Size", 16, 4, XBinary::VT_UINT32, XBinary::DRF_SIZE, XBinary::XFRECORD_FLAG_SIZE},
    {"Original Size", "Original Size", 20, 4, XBinary::VT_UINT32, XBinary::DRF_SIZE, XBinary::XFRECORD_FLAG_SIZE},
    {"CRC32", "CRC32", 24, 4, XBinary::VT_UINT32, XBinary::DRF_UNKNOWN, XBinary::XFRECORD_FLAG_NONE},
    {"Entry Name Pos", "Entry Name Pos", 28, 2, XBinary::VT_UINT16, XBinary::DRF_UNKNOWN, XBinary::XFRECORD_FLAG_OFFSET},
    {"File Access Mode", "File Access Mode", 30, 2, XBinary::VT_UINT16, XBinary::DRF_UNKNOWN, XBinary::XFRECORD_FLAG_NONE},
    {"First Chapter", "First Chapter", 32, 1, XBinary::VT_UINT8, XBinary::DRF_UNKNOWN, XBinary::XFRECORD_FLAG_NONE},
    {"Last Chapter", "Last Chapter", 33, 1, XBinary::VT_UINT8, XBinary::DRF_UNKNOWN, XBinary::XFRECORD_FLAG_NONE},
};

struct ARJ_ENTRY_INFO {
    qint64 nOffset;
    quint16 nBasicHeaderSize;
    qint64 nHeaderSize;
    quint8 nFirstHeaderSize;
    quint8 nFlags;
    quint8 nMethod;
    quint8 nPasswordModifier;
    quint32 nDosDateTime;
    quint32 nCompressedSize;
    quint32 nOriginalSize;
    quint32 nCRC32;
    QString sFileName;
    bool bEndOfArchive;
};

qint32 xarjStructIdCount()
{
    return sizeof(_TABLE_XARJ_STRUCTID) / sizeof(_TABLE_XARJ_STRUCTID[0]);
}

qint32 arjRecordFieldCount()
{
    return sizeof(g_arjRecordFields) / sizeof(g_arjRecordFields[0]);
}

qint64 basicHeaderOffset(qint64 nEntryOffset)
{
    return nEntryOffset + ARJ_ENTRY_PREFIX_SIZE;
}

qint64 basicFieldOffset(qint64 nEntryOffset, qint64 nRelativeOffset)
{
    return basicHeaderOffset(nEntryOffset) + nRelativeOffset;
}

bool hasArjMarker(XARJ *pArj, qint64 nOffset)
{
    return pArj && ((nOffset + ARJ_ENTRY_PREFIX_SIZE) <= pArj->getSize()) && (pArj->read_uint8(nOffset) == ARJ_MARKER_BYTE0) &&
           (pArj->read_uint8(nOffset + 1) == ARJ_MARKER_BYTE1);
}

quint16 readBasicHeaderSize(XARJ *pArj, qint64 nOffset)
{
    return pArj->read_uint16(nOffset + ARJ_MARKER_SIZE, false);
}

bool isEndOfArchiveHeader(quint16 nBasicHeaderSize)
{
    return nBasicHeaderSize == 0;
}

qint64 readEntryHeaderSize(XARJ *pArj, qint64 nOffset)
{
    if (!hasArjMarker(pArj, nOffset)) {
        return -1;
    }

    quint16 nBasicHeaderSize = readBasicHeaderSize(pArj, nOffset);

    if (isEndOfArchiveHeader(nBasicHeaderSize)) {
        return ARJ_ENTRY_PREFIX_SIZE;
    }

    if ((nOffset + ARJ_ENTRY_PREFIX_SIZE + nBasicHeaderSize + ARJ_HEADER_CRC_SIZE) > pArj->getSize()) {
        return -1;
    }

    qint64 nPos = nOffset + ARJ_ENTRY_PREFIX_SIZE + nBasicHeaderSize + ARJ_HEADER_CRC_SIZE;

    while (true) {
        if ((nPos + ARJ_EXT_HEADER_SIZE_FIELD_SIZE) > pArj->getSize()) {
            break;
        }

        quint16 nExtSize = pArj->read_uint16(nPos, false);
        nPos += ARJ_EXT_HEADER_SIZE_FIELD_SIZE;

        if (nExtSize == 0) {
            break;
        }

        nPos += nExtSize + ARJ_HEADER_CRC_SIZE;

        if (nPos > pArj->getSize()) {
            break;
        }
    }

    return nPos - nOffset;
}

QString readEntryFileName(XARJ *pArj, qint64 nOffset)
{
    quint8 nFirstHeaderSize = pArj->read_uint8(basicFieldOffset(nOffset, ARJ_BASIC_FIRST_HEADER_SIZE));
    qint64 nNameOffset = basicHeaderOffset(nOffset) + nFirstHeaderSize;
    quint16 nBasicHeaderSize = readBasicHeaderSize(pArj, nOffset);
    qint64 nMaxNameLen = (basicHeaderOffset(nOffset) + nBasicHeaderSize) - nNameOffset;

    if (nMaxNameLen <= 0) {
        return QString();
    }

    return pArj->read_ansiString(nNameOffset, (qint32)nMaxNameLen);
}

bool readEntryInfo(XARJ *pArj, qint64 nOffset, ARJ_ENTRY_INFO *pInfo)
{
    if (!pInfo) {
        return false;
    }

    ARJ_ENTRY_INFO info = {};

    if (!hasArjMarker(pArj, nOffset)) {
        return false;
    }

    info.nOffset = nOffset;
    info.nBasicHeaderSize = readBasicHeaderSize(pArj, nOffset);
    info.bEndOfArchive = isEndOfArchiveHeader(info.nBasicHeaderSize);
    info.nHeaderSize = readEntryHeaderSize(pArj, nOffset);

    if (info.nHeaderSize <= 0) {
        return false;
    }

    if (info.bEndOfArchive) {
        *pInfo = info;
        return true;
    }

    if (info.nBasicHeaderSize < XARJ::FIXED_HEADER_SIZE) {
        return false;
    }

    info.nFirstHeaderSize = pArj->read_uint8(basicFieldOffset(nOffset, ARJ_BASIC_FIRST_HEADER_SIZE));
    info.nFlags = pArj->read_uint8(basicFieldOffset(nOffset, ARJ_BASIC_FLAGS));
    info.nMethod = pArj->read_uint8(basicFieldOffset(nOffset, ARJ_BASIC_METHOD));
    info.nPasswordModifier = pArj->read_uint8(basicFieldOffset(nOffset, ARJ_BASIC_PASSWORD_MODIFIER));
    info.nDosDateTime = pArj->read_uint32(basicFieldOffset(nOffset, ARJ_BASIC_DOS_DATE_TIME), false);
    info.nCompressedSize = pArj->read_uint32(basicFieldOffset(nOffset, ARJ_BASIC_COMPRESSED_SIZE), false);
    info.nOriginalSize = pArj->read_uint32(basicFieldOffset(nOffset, ARJ_BASIC_ORIGINAL_SIZE), false);
    info.nCRC32 = pArj->read_uint32(basicFieldOffset(nOffset, ARJ_BASIC_CRC32), false);
    info.sFileName = readEntryFileName(pArj, nOffset);

    *pInfo = info;

    return true;
}

qint64 entryStreamOffset(const ARJ_ENTRY_INFO &info)
{
    return info.nOffset + info.nHeaderSize;
}

qint64 entryEndOffset(const ARJ_ENTRY_INFO &info)
{
    return entryStreamOffset(info) + info.nCompressedSize;
}

qint64 firstFileRecordOffset(XARJ *pArj)
{
    ARJ_ENTRY_INFO info = {};

    if (readEntryInfo(pArj, 0, &info) && !info.bEndOfArchive) {
        return info.nHeaderSize;
    }

    return 0;
}

qint32 countFileRecords(XARJ *pArj, qint64 nStartOffset, XBinary::PDSTRUCT *pPdStruct, qint64 *pEndOffset)
{
    qint32 nResult = 0;
    qint64 nCurrentOffset = nStartOffset;
    qint64 nLastEndOffset = nStartOffset;
    qint64 nFileSize = pArj->getSize();

    while ((nCurrentOffset < nFileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        ARJ_ENTRY_INFO info = {};

        if (!readEntryInfo(pArj, nCurrentOffset, &info) || info.bEndOfArchive) {
            break;
        }

        nResult++;
        nLastEndOffset = entryEndOffset(info);
        nCurrentOffset = nLastEndOffset;
    }

    if (pEndOffset) {
        *pEndOffset = nLastEndOffset;
    }

    return nResult;
}

XBinary::HANDLE_METHOD handleMethodForArjMethod(quint8 nMethod)
{
    XBinary::HANDLE_METHOD result = XBinary::HANDLE_METHOD_UNKNOWN;

    if ((nMethod == XARJ::CMETHOD_STORED) || (nMethod == XARJ::CMETHOD_NO_COMPRESSION_1) || (nMethod == XARJ::CMETHOD_NO_COMPRESSION_2)) {
        result = XBinary::HANDLE_METHOD_STORE;
    } else if ((nMethod == XARJ::CMETHOD_COMPRESSED_MOST) || (nMethod == XARJ::CMETHOD_COMPRESSED) || (nMethod == XARJ::CMETHOD_COMPRESSED_FASTER)) {
        result = XBinary::HANDLE_METHOD_ARJ;
    } else if (nMethod == XARJ::CMETHOD_COMPRESSED_FASTEST) {
        result = XBinary::HANDLE_METHOD_ARJ_FASTEST;
    }

    return result;
}

QDateTime dosDateTimeToDateTime(quint32 nDosDateTime)
{
    qint32 nYear = ((nDosDateTime >> 25) & 0x7F) + 1980;
    qint32 nMonth = (nDosDateTime >> 21) & 0x0F;
    qint32 nDay = (nDosDateTime >> 16) & 0x1F;
    qint32 nHour = (nDosDateTime >> 11) & 0x1F;
    qint32 nMinute = (nDosDateTime >> 5) & 0x3F;
    qint32 nSecond = (nDosDateTime & 0x1F) * 2;

    return QDateTime(QDate(nYear, nMonth, nDay), QTime(nHour, nMinute, nSecond));
}

void appendArjDataRecords(XARJ *pArj, QList<XBinary::DATA_RECORD> *pListRecords, XBinary::ENDIAN endian)
{
    qint32 nNumberOfRecords = arjRecordFieldCount();

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
        const ARJ_RECORD_FIELD &field = g_arjRecordFields[i];
        pListRecords->append(pArj->getDataRecord(field.nOffset, field.nSize, field.pszDataName, field.valueType, field.nDataRecordFlags, endian));
    }
}

XBinary::FPART createFilePart(XBinary::FILEPART filePart, qint64 nOffset, qint64 nSize, const QString &sName)
{
    XBinary::FPART result = {};
    result.filePart = filePart;
    result.nFileOffset = nOffset;
    result.nFileSize = nSize;
    result.nVirtualAddress = -1;
    result.sName = sName;

    return result;
}
}  // namespace

XARJ::XARJ(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XARJ::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (getSize() >= ARJ_MIN_VALID_SIZE) {
        quint16 nBasicHeaderSize = readBasicHeaderSize(this, 0);

        if (hasArjMarker(this, 0) && (nBasicHeaderSize >= FIXED_HEADER_SIZE) && (nBasicHeaderSize <= ARJ_MAX_BASIC_HEADER_SIZE)) {
            quint8 nFirstHeaderSize = read_uint8(basicFieldOffset(0, ARJ_BASIC_FIRST_HEADER_SIZE));

            if (nFirstHeaderSize >= FIXED_HEADER_SIZE) {
                bResult = true;
            }
        }
    }

    return bResult;
}

bool XARJ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XARJ xarj(pDevice);

    return xarj.isValid(pPdStruct);
}

qint64 XARJ::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QList<XBinary::MAPMODE> XARJ::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XARJ::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

XBinary::FT XARJ::getFileType()
{
    return FT_ARJ;
}

QString XARJ::getFileFormatExt()
{
    return "arj";
}

QString XARJ::getFileFormatExtsString()
{
    return "ARJ (*.arj)";
}

QString XARJ::getMIMEString()
{
    return "application/x-arj";
}

QString XARJ::getVersion()
{
    qint64 nVersionOffset = basicFieldOffset(0, ARJ_BASIC_ARCHIVER_VERSION);

    if (getSize() > nVersionOffset) {
        quint8 nVersion = read_uint8(nVersionOffset);

        return QString::number(nVersion);
    }

    return QString();
}

QString XARJ::getArch()
{
    return QString();
}

XBinary::ENDIAN XARJ::getEndian()
{
    return ENDIAN_LITTLE;
}

XBinary::MODE XARJ::getMode()
{
    return MODE_DATA;
}

bool XARJ::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
        pState->mapUnpackProperties = mapProperties;

        qint64 nOffset = firstFileRecordOffset(this);
        pState->nCurrentOffset = nOffset;
        pState->nNumberOfRecords = countFileRecords(this, nOffset, pPdStruct, nullptr);

        bResult = (pState->nNumberOfRecords > 0);
    }

    return bResult;
}

XBinary::ARCHIVERECORD XARJ::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        ARJ_ENTRY_INFO info = {};

        if (!readEntryInfo(this, pState->nCurrentOffset, &info) || info.bEndOfArchive) {
            return result;
        }

        QString sFileName = info.sFileName.replace("\\", "/");

        result.nStreamOffset = entryStreamOffset(info);
        result.nStreamSize = info.nCompressedSize;

        result.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, sFileName);
        result.mapProperties.insert(XBinary::FPART_PROP_STREAMOFFSET, result.nStreamOffset);
        result.mapProperties.insert(XBinary::FPART_PROP_STREAMSIZE, result.nStreamSize);
        result.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)info.nOriginalSize);
        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, (qint64)info.nCompressedSize);
        result.mapProperties.insert(XBinary::FPART_PROP_RESULTCRC, info.nCRC32);
        result.mapProperties.insert(XBinary::FPART_PROP_CRC_TYPE, info.nCRC32 != 0 ? XBinary::CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF : XBinary::CRC_TYPE_UNKNOWN);
        result.mapProperties.insert(XBinary::FPART_PROP_TYPE, (quint32)info.nMethod);

        result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, handleMethodForArjMethod(info.nMethod));

        if (info.nFlags & ARJ_FLAG_GARBLE) {
            result.mapProperties.insert(XBinary::FPART_PROP_PASSWORD_MODIFIER, (quint32)info.nPasswordModifier);
        }

        QDateTime dtMTime = dosDateTimeToDateTime(info.nDosDateTime);

        if (dtMTime.isValid()) {
            result.mapProperties.insert(XBinary::FPART_PROP_MTIME, dtMTime);
        }
    }

    return result;
}

bool XARJ::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        ARJ_ENTRY_INFO info = {};

        if (readEntryInfo(this, pState->nCurrentOffset, &info) && !info.bEndOfArchive) {
            pState->nCurrentOffset = entryEndOffset(info);
            pState->nCurrentIndex++;

            bResult = (pState->nCurrentIndex < pState->nNumberOfRecords);
        }
    }

    return bResult;
}

bool XARJ::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pState)
    Q_UNUSED(pPdStruct)

    return true;
}

QString XARJ::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XARJ_STRUCTID, xarjStructIdCount());
}

QString XARJ::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XARJ_STRUCTID, xarjStructIdCount());
}

quint32 XARJ::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XARJ_STRUCTID, xarjStructIdCount());
}

QList<XBinary::DATA_HEADER> XARJ::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

        qint64 nOffset = firstFileRecordOffset(this);
        qint64 nRealSize = nOffset;
        qint32 nCount = countFileRecords(this, nOffset, pPdStruct, &nRealSize);

        _dataHeadersOptions.nID = STRUCTID_RECORD;
        _dataHeadersOptions.nLocation = nOffset;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;
        _dataHeadersOptions.nCount = nCount;
        _dataHeadersOptions.nSize = nRealSize - nOffset;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_RECORD) {
                qint64 nCurrentOffset = nStartOffset;
                qint32 nCount = 0;

                while ((nCount < dataHeadersOptions.nCount) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                    ARJ_ENTRY_INFO info = {};

                    if (!readEntryInfo(this, nCurrentOffset, &info) || info.bEndOfArchive) {
                        break;
                    }

                    DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(STRUCTID_RECORD));
                    dataHeader.nSize = info.nHeaderSize + info.nCompressedSize;
                    appendArjDataRecords(this, &dataHeader.listRecords, dataHeadersOptions.pMemoryMap->endian);

                    if (info.nBasicHeaderSize > FIXED_HEADER_SIZE) {
                        dataHeader.listRecords.append(getDataRecord(ARJ_ENTRY_PREFIX_SIZE + FIXED_HEADER_SIZE, info.nBasicHeaderSize - FIXED_HEADER_SIZE,
                                                                    "Filename+Comment", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    }

                    if (info.nCompressedSize > 0) {
                        dataHeader.listRecords.append(
                            getDataRecord(info.nHeaderSize, info.nCompressedSize, "Compressed Data", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    }

                    listResult.append(dataHeader);

                    nCurrentOffset = entryEndOffset(info);
                    nCount++;
                }
            }
        }
    }

    return listResult;
}

QList<XBinary::XFHEADER> XARJ::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XBinary::XFHEADER> listResult;
    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if ((nStructID == STRUCTID_HEADER) || (nStructID == STRUCTID_RECORD)) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if (nHeaderOffset != -1) {
            qint64 nHeaderSize = xfStruct.nSize;

            if (nHeaderSize <= 0) {
                nHeaderSize = readEntryHeaderSize(this, nHeaderOffset);
            }

            if ((nHeaderSize > 0) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, nHeaderSize)) {
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
    }

    return listResult;
}

QList<XBinary::XFRECORD> XARJ::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if ((nStructID == STRUCTID_HEADER) || (nStructID == STRUCTID_RECORD)) {
        qint32 nNumberOfRecords = arjRecordFieldCount();

        for (qint32 i = 0; i < nNumberOfRecords; i++) {
            const ARJ_RECORD_FIELD &field = g_arjRecordFields[i];
            listResult.append({field.pszXFName, field.nOffset, field.nSize, field.nXFRecordFlags, field.valueType});
        }
    }

    return listResult;
}

QList<XBinary::FPART> XARJ::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    QList<FPART> listResult;

    qint64 nFileSize = getSize();
    qint64 nCurrentOffset = firstFileRecordOffset(this);
    qint64 nMaxOffset = 0;

    while ((nCurrentOffset < nFileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        ARJ_ENTRY_INFO info = {};

        if (!readEntryInfo(this, nCurrentOffset, &info) || info.bEndOfArchive) {
            break;
        }

        if (nFileParts & FILEPART_HEADER) {
            listResult.append(createFilePart(FILEPART_HEADER, nCurrentOffset, info.nHeaderSize, tr("Header")));
        }

        if (nFileParts & FILEPART_STREAM) {
            FPART record = createFilePart(FILEPART_STREAM, entryStreamOffset(info), info.nCompressedSize, info.sFileName);
            record.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)info.nOriginalSize);

            listResult.append(record);
        }

        if (nFileParts & FILEPART_REGION) {
            listResult.append(createFilePart(FILEPART_REGION, nCurrentOffset, info.nHeaderSize + info.nCompressedSize, info.sFileName));
        }

        nMaxOffset = entryEndOffset(info);
        nCurrentOffset = nMaxOffset;
    }

    if (nFileParts & FILEPART_OVERLAY) {
        if (nMaxOffset < nFileSize) {
            listResult.append(createFilePart(FILEPART_OVERLAY, nMaxOffset, nFileSize - nMaxOffset, tr("Overlay")));
        }
    }

    return listResult;
}

QString XARJ::cmethodToString(CMETHOD cmethod)
{
    QString sResult;

    switch (cmethod) {
        case CMETHOD_STORED: sResult = "Stored"; break;
        case CMETHOD_COMPRESSED_MOST: sResult = "Compressed (most)"; break;
        case CMETHOD_COMPRESSED: sResult = "Compressed"; break;
        case CMETHOD_COMPRESSED_FASTER: sResult = "Compressed (faster)"; break;
        case CMETHOD_COMPRESSED_FASTEST: sResult = "Compressed (fastest)"; break;
        case CMETHOD_NO_COMPRESSION_1: sResult = "No compression"; break;
        case CMETHOD_NO_COMPRESSION_2: sResult = "No compression (type 2)"; break;
        default: sResult = "Unknown"; break;
    }

    return sResult;
}

QList<QString> XARJ::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("60EA");

    return listResult;
}

XBinary *XARJ::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XARJ(pDevice);
}
