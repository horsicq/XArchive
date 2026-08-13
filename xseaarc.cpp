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
#include "xseaarc.h"
#include "Algos/xstoredecoder.h"

XBinary::XCONVERT _TABLE_XSEAARC_STRUCTID[] = {
    {XSEAARC::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XSEAARC::STRUCTID_HEADER, "HEADER", QString("Header")},
    {XSEAARC::STRUCTID_RECORD, "RECORD", QString("Record")},
};

static quint16 seaReadLe16(const QByteArray &baData, qint32 nOffset)
{
    if ((nOffset < 0) || ((nOffset + 2) > baData.size())) return 0;
    return static_cast<quint16>(
        static_cast<quint8>(baData.at(nOffset)) |
        (static_cast<quint16>(static_cast<quint8>(
             baData.at(nOffset + 1))) << 8));
}

static quint32 seaReadLe32(const QByteArray &baData, qint32 nOffset)
{
    if ((nOffset < 0) || ((nOffset + 4) > baData.size())) return 0;
    return static_cast<quint32>(
        static_cast<quint8>(baData.at(nOffset)) |
        (static_cast<quint32>(static_cast<quint8>(
             baData.at(nOffset + 1))) << 8) |
        (static_cast<quint32>(static_cast<quint8>(
             baData.at(nOffset + 2))) << 16) |
        (static_cast<quint32>(static_cast<quint8>(
             baData.at(nOffset + 3))) << 24));
}

XSEAARC::XSEAARC(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XSEAARC::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    // ARC archive: starts with 0x1A followed by method byte (1-9)
    // and 13-byte null-terminated filename
    if (getSize() >= 29) {  // Minimum: 1 (marker) + 1 (method) + 13 (name) + 4 (compressed size) + 2 (date) + 2 (time) + 2 (crc) + 4 (original size) = 29
        quint8 nMarker = read_uint8(0);
        quint8 nMethod = read_uint8(1);

        if ((nMarker == 0x1A) && (nMethod >= 1) && (nMethod <= 9)) {
            // Read filename (13 bytes, null-terminated ASCII)
            QByteArray baFileName = read_array(2, 13);

            // First byte of filename must be printable ASCII
            if (baFileName.size() >= 1) {
                quint8 nFirstChar = (quint8)baFileName.at(0);

                if ((nFirstChar >= 0x21) && (nFirstChar <= 0x7E)) {
                    // Verify all chars up to null are printable ASCII
                    bool bValidName = true;

                    for (qint32 i = 0; i < baFileName.size(); i++) {
                        quint8 nChar = (quint8)baFileName.at(i);

                        if (nChar == 0) {
                            break;  // Null terminator found
                        }

                        if ((nChar < 0x20) || (nChar > 0x7E)) {
                            bValidName = false;
                            break;
                        }
                    }

                    if (bValidName) {
                        bResult = true;
                    }
                }
            }
        }
    }

    return bResult;
}

bool XSEAARC::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSEAARC xseaarc(pDevice);

    return xseaarc.isValid(pPdStruct);
}

qint64 XSEAARC::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QList<XBinary::MAPMODE> XSEAARC::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XSEAARC::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

XBinary::FT XSEAARC::getFileType()
{
    return FT_ARC;
}

QString XSEAARC::getFileFormatExt()
{
    return "arc";
}

QString XSEAARC::getFileFormatExtsString()
{
    return "ARC (*.arc)";
}

QString XSEAARC::getMIMEString()
{
    return "application/x-arc";
}

QString XSEAARC::getVersion()
{
    // Return highest method version found in archive
    quint8 nMaxMethod = 0;
    qint64 nOffset = 0;
    qint64 nFileSize = getSize();

    while (nOffset < nFileSize) {
        quint8 nMarker = read_uint8(nOffset);
        quint8 nMethod = read_uint8(nOffset + 1);

        if ((nMarker != 0x1A) || (nMethod == CMETHOD_END)) {
            break;
        }

        if (!_isValidMethod(nMethod)) {
            break;
        }

        if (nMethod > nMaxMethod) {
            nMaxMethod = nMethod;
        }

        qint32 nHeaderSize = _getHeaderSize(nMethod);
        quint32 nCompressedSize = read_uint32(nOffset + 15, false);

        nOffset += nHeaderSize + nCompressedSize;
    }

    return QString::number(nMaxMethod);
}

QString XSEAARC::getArch()
{
    return QString();
}

XBinary::ENDIAN XSEAARC::getEndian()
{
    return ENDIAN_LITTLE;
}

XBinary::MODE XSEAARC::getMode()
{
    return MODE_DATA;
}

QMap<XBinary::UNPACK_PROP, QVariant> XSEAARC::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XSEAARC::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSEAARC> guardedArchive(this);
    if (!pState || m_bUnpackOperationInProgress ||
        ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState))) {
        return false;
    }
    if (!guardedArchive->finishUnpack(pState, nullptr) || !guardedArchive) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const bool bBound = guardedArchive->bindUnpackSource(pState, pPdStruct);
    if (!guardedArchive || !bBound) return false;

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = guardedArchive->getSize();
    if (!guardedArchive) {
        *pState = UNPACK_STATE();
        return false;
    }
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->pContext = nullptr;

    qint64 nOffset = 0;
    qint64 nFileSize = pState->nTotalSize;

    while ((nOffset < nFileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if ((nFileSize - nOffset) < 2) {
            break;
        }

        const QByteArray baPrefix = guardedArchive->read_array(nOffset, 2);
        if (!guardedArchive) {
            *pState = UNPACK_STATE();
            return false;
        }
        if (baPrefix.size() != 2) break;
        quint8 nMarker = static_cast<quint8>(baPrefix.at(0));
        quint8 nMethod = static_cast<quint8>(baPrefix.at(1));

        if ((nMarker != 0x1A) || (nMethod == CMETHOD_END)) {
            break;
        }

        if (!_isValidMethod(nMethod)) {
            break;
        }

        qint32 nHeaderSize = _getHeaderSize(nMethod);

        if ((nFileSize - nOffset) < nHeaderSize) {
            break;
        }

        const QByteArray baSize = guardedArchive->read_array(nOffset + 15, 4);
        if (!guardedArchive) {
            *pState = UNPACK_STATE();
            return false;
        }
        if (baSize.size() != 4) break;
        quint32 nCompressedSize = seaReadLe32(baSize, 0);
        qint64 nAvailableData = nFileSize - nOffset - nHeaderSize;

        if ((nAvailableData < 0) || (nCompressedSize > (quint64)nAvailableData)) {
            break;
        }

        pState->nNumberOfRecords++;

        nOffset += nHeaderSize + (qint64)nCompressedSize;
    }

    if ((pState->nNumberOfRecords > 0) &&
        XBinary::isPdStructNotCanceled(pPdStruct)) {
        bResult = guardedArchive->validateAndFinalizeUnpackSource(
            pState, pPdStruct);
        if (!guardedArchive) {
            *pState = UNPACK_STATE();
            return false;
        }
    }
    if (!bResult) {
        guardedArchive->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
    }

    return bResult;
}

XBinary::ARCHIVERECORD XSEAARC::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<XSEAARC> guardedArchive(this);

    XBinary::ARCHIVERECORD result = {};

    if (XBinary::isPdStructNotCanceled(pPdStruct) && pState && guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) && guardedArchive &&
        (pState->nCurrentIndex >= 0) &&
        (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        const QByteArray baPrefix = guardedArchive->read_array(
            pState->nCurrentOffset, 2);
        if (!guardedArchive || (baPrefix.size() != 2)) {
            return XBinary::ARCHIVERECORD();
        }
        quint8 nMethod = static_cast<quint8>(baPrefix.at(1));
        qint32 nHeaderSize = _getHeaderSize(nMethod);
        const QByteArray baHeader = guardedArchive->read_array(
            pState->nCurrentOffset, nHeaderSize);
        if (!guardedArchive || (baHeader.size() != nHeaderSize)) {
            return XBinary::ARCHIVERECORD();
        }
        quint32 nCompressedSize = seaReadLe32(baHeader, 15);
        quint32 nUncompressedSize = nCompressedSize;  // Default for method 1

        if (nMethod >= CMETHOD_STORE) {
            nUncompressedSize = seaReadLe32(baHeader, 25);
        }

        // Read filename (13 bytes null-terminated)
        const QByteArray baFileName = baHeader.mid(2, 13);
        const qint32 nTerminator = baFileName.indexOf('\0');
        QString sFileName = QString::fromLatin1(
            baFileName.constData(),
            (nTerminator >= 0) ? nTerminator : baFileName.size());

        quint16 nCRC16 = seaReadLe16(baHeader, 23);
        quint16 nDate = seaReadLe16(baHeader, 19);
        quint16 nTime = seaReadLe16(baHeader, 21);

        result.nStreamOffset = pState->nCurrentOffset + nHeaderSize;
        result.nStreamSize = nCompressedSize;

        result.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, sFileName);
        result.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)nUncompressedSize);
        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, (qint64)nCompressedSize);
        result.mapProperties.insert(XBinary::FPART_PROP_RESULTCRC, (quint32)nCRC16);
        result.mapProperties.insert(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_CRC16ARC);
        result.mapProperties.insert(XBinary::FPART_PROP_TYPE, (quint32)nMethod);

        // Determine handle method
        XBinary::HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;

        if ((nMethod == CMETHOD_STORE_OLD) || (nMethod == CMETHOD_STORE)) {
            compressMethod = HANDLE_METHOD_STORE;
        }
        // Methods 3-9 are various LZW/RLE/Huffman variants - marked as unknown for now

        result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

        // Convert DOS date/time to QDateTime
        // DOS date: bits 15-9=year(from 1980), bits 8-5=month, bits 4-0=day
        // DOS time: bits 15-11=hour, bits 10-5=minute, bits 4-0=second/2
        qint32 nYear = ((nDate >> 9) & 0x7F) + 1980;
        qint32 nMonth = (nDate >> 5) & 0x0F;
        qint32 nDay = nDate & 0x1F;
        qint32 nHour = (nTime >> 11) & 0x1F;
        qint32 nMinute = (nTime >> 5) & 0x3F;
        qint32 nSecond = (nTime & 0x1F) * 2;

        QDateTime dtMTime(QDate(nYear, nMonth, nDay), QTime(nHour, nMinute, nSecond));

        if (dtMTime.isValid()) {
            result.mapProperties.insert(XBinary::FPART_PROP_MTIME, dtMTime);
        }
    }

    return result;
}

bool XSEAARC::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XSEAARC> guardedArchive(this);

    bool bResult = false;

    if (XBinary::isPdStructNotCanceled(pPdStruct) && pState && guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) && guardedArchive &&
        (pState->nCurrentIndex >= 0) &&
        (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        const QByteArray baPrefix = guardedArchive->read_array(
            pState->nCurrentOffset, 2);
        if (!guardedArchive || (baPrefix.size() != 2)) return false;
        quint8 nMethod = static_cast<quint8>(baPrefix.at(1));
        qint32 nHeaderSize = _getHeaderSize(nMethod);
        const QByteArray baSize = guardedArchive->read_array(
            pState->nCurrentOffset + 15, 4);
        if (!guardedArchive || (baSize.size() != 4)) return false;
        quint32 nCompressedSize = seaReadLe32(baSize, 0);

        pState->nCurrentOffset += (nHeaderSize + nCompressedSize);
        pState->nCurrentIndex++;

        bResult = (pState->nCurrentIndex < pState->nNumberOfRecords);
    }

    return bResult;
}

bool XSEAARC::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    releaseUnpackSource(pState);

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->pContext = nullptr;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    return true;
}

QString XSEAARC::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XSEAARC_STRUCTID, sizeof(_TABLE_XSEAARC_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XSEAARC::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XSEAARC_STRUCTID, sizeof(_TABLE_XSEAARC_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XSEAARC::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if (nHeaderOffset != -1) {
            quint8 nMethod = read_uint8(nHeaderOffset + 1);

            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = _getHeaderSize(nMethod);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_HEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_HEADER), xfHeader.sParentTag);
            listResult.append(xfHeader);

            if (xfStruct.bIsParent) {
                XFSTRUCT _xfStruct = xfStruct;
                _xfStruct.sParent = xfHeader.sTag;
                _xfStruct.nStructID = STRUCTID_RECORD;
                _xfStruct.xLoc = offsetToLoc(0);
                listResult.append(getXFHeaders(_xfStruct, pPdStruct));
            }
        }
    } else if (nStructID == STRUCTID_RECORD) {
        qint64 nStartOffset = locToOffset(xfStruct.pMemoryMap, xfStruct.xLoc);

        if (nStartOffset == -1) {
            nStartOffset = 0;
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_RECORD);
        xfHeader.xLoc = offsetToLoc(nStartOffset);
        xfHeader.xfType = XFTYPE_TABLE;

        qint64 nFileSize = getSize();
        qint64 nCurrentOffset = nStartOffset;

        while ((nCurrentOffset < nFileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            if ((nFileSize - nCurrentOffset) < 2) {
                break;
            }

            quint8 nMarker = read_uint8(nCurrentOffset);
            quint8 nMethod = read_uint8(nCurrentOffset + 1);

            if ((nMarker != 0x1A) || (nMethod == CMETHOD_END)) {
                break;
            }

            if (!_isValidMethod(nMethod)) {
                break;
            }

            qint32 nHeaderSize = _getHeaderSize(nMethod);

            if ((nFileSize - nCurrentOffset) < nHeaderSize) {
                break;
            }

            xfHeader.listRowLocations.append(nCurrentOffset);

            quint32 nCompressedSize = read_uint32(nCurrentOffset + 15, false);
            nCurrentOffset += (nHeaderSize + nCompressedSize);
        }

        if (!xfHeader.listRowLocations.isEmpty()) {
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_RECORD, offsetToLoc(xfHeader.listRowLocations.first()));
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_RECORD), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XSEAARC::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)

    QList<XBinary::XFRECORD> listResult;

    if ((nStructID == STRUCTID_HEADER) || (nStructID == STRUCTID_RECORD)) {
        quint8 nMethod = read_uint8(xLoc.nLocation + 1);

        listResult.append({"Marker", 0, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"Method", 1, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"Name", 2, 13, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"CompressedSize", 15, 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"Date", 19, 2, XFRECORD_FLAG_DOSDATE, VT_UINT16});
        listResult.append({"Time", 21, 2, XFRECORD_FLAG_DOSTIME, VT_UINT16});
        listResult.append({"CRC16", 23, 2, XFRECORD_FLAG_NONE, VT_UINT16});

        if (nMethod != CMETHOD_STORE_OLD) {
            listResult.append({"OriginalSize", 25, 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        }
    }

    return listResult;
}

quint32 XSEAARC::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XSEAARC_STRUCTID, sizeof(_TABLE_XSEAARC_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// QList<XBinary::DATA_HEADER> XSEAARC::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

//         qint64 nRealSize = 0;
//         qint32 nCount = 0;
//         qint64 nOffset = 0;
//         qint64 nFileSize = getSize();

//         while ((nOffset < nFileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
//             if ((nFileSize - nOffset) < 2) {
//                 break;
//             }

//             quint8 nMarker = read_uint8(nOffset);
//             quint8 nMethod = read_uint8(nOffset + 1);

//             if ((nMarker != 0x1A) || (nMethod == CMETHOD_END)) {
//                 break;
//             }

//             if (!_isValidMethod(nMethod)) {
//                 break;
//             }

//             qint32 nHeaderSize = _getHeaderSize(nMethod);
//             quint32 nCompressedSize = read_uint32(nOffset + 15, false);

//             nCount++;
//             nRealSize = nOffset + nHeaderSize + nCompressedSize;

//             nOffset += (nHeaderSize + nCompressedSize);
//         }

//         _dataHeadersOptions.nID = STRUCTID_RECORD;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;
//         _dataHeadersOptions.nCount = nCount;
//         _dataHeadersOptions.nSize = nRealSize;

//         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             if (dataHeadersOptions.nID == STRUCTID_RECORD) {
//                 qint64 nCurrentOffset = nStartOffset;
//                 qint32 nCount = 0;

//                 while ((nCount < dataHeadersOptions.nCount) && XBinary::isPdStructNotCanceled(pPdStruct)) {
//                     quint8 nMarker = read_uint8(nCurrentOffset);
//                     quint8 nMethod = read_uint8(nCurrentOffset + 1);

//                     if ((nMarker != 0x1A) || (nMethod == CMETHOD_END) || !_isValidMethod(nMethod)) {
//                         break;
//                     }

//                     qint32 nHeaderSize = _getHeaderSize(nMethod);
//                     quint32 nCompressedSize = read_uint32(nCurrentOffset + 15, false);

//                     DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(STRUCTID_RECORD));
//                     dataHeader.nSize = nHeaderSize + nCompressedSize;

//                     dataHeader.listRecords.append(getDataRecord(0, 1, "Marker", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                     dataHeader.listRecords.append(getDataRecord(1, 1, "Method", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                     dataHeader.listRecords.append(getDataRecord(2, 13, "File Name", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                     dataHeader.listRecords.append(getDataRecord(15, 4, "Compressed Size", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
//                     dataHeader.listRecords.append(getDataRecord(19, 2, "Date", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                     dataHeader.listRecords.append(getDataRecord(21, 2, "Time", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                     dataHeader.listRecords.append(getDataRecord(23, 2, "CRC16", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                     if (nMethod >= CMETHOD_STORE) {
//                         dataHeader.listRecords.append(getDataRecord(25, 4, "Original Size", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
//                     }

//                     if (nCompressedSize > 0) {
//                         dataHeader.listRecords.append(
//                             getDataRecord(nHeaderSize, nCompressedSize, "Compressed Data", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                     }

//                     listResult.append(dataHeader);

//                     nCurrentOffset += (nHeaderSize + nCompressedSize);
//                     nCount++;
//                 }
//             }
//         }
//     }

//     return listResult;
// }

static bool seaCanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XSEAARC::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    qint64 nFileSize = getSize();
    qint64 nCurrentOffset = 0;
    qint64 nMaxOffset = 0;

    while ((nCurrentOffset < nFileSize) && seaCanAppend(nLimit, listResult) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if ((nFileSize - nCurrentOffset) < 2) {
            break;
        }

        quint8 nMarker = read_uint8(nCurrentOffset);
        quint8 nMethod = read_uint8(nCurrentOffset + 1);

        if ((nMarker != 0x1A) || (nMethod == CMETHOD_END)) {
            break;
        }

        if (!_isValidMethod(nMethod)) {
            break;
        }

        qint32 nHeaderSize = _getHeaderSize(nMethod);
        quint32 nCompressedSize = read_uint32(nCurrentOffset + 15, false);
        quint32 nUncompressedSize = nCompressedSize;

        if (nMethod >= CMETHOD_STORE) {
            nUncompressedSize = read_uint32(nCurrentOffset + 25, false);
        }

        QString sFileName = read_ansiString(nCurrentOffset + 2, 13);

        if ((nFileParts & FILEPART_HEADER) && seaCanAppend(nLimit, listResult)) {
            FPART record = {};

            record.filePart = FILEPART_HEADER;
            record.nFileOffset = nCurrentOffset;
            record.nFileSize = nHeaderSize;
            record.nVirtualAddress = XADDR_MAX;
            record.sName = tr("Header");

            listResult.append(record);
        }

        if ((nFileParts & FILEPART_STREAM) && seaCanAppend(nLimit, listResult)) {
            FPART record = {};

            record.filePart = FILEPART_STREAM;
            record.nFileOffset = nCurrentOffset + nHeaderSize;
            record.nFileSize = nCompressedSize;
            record.nVirtualAddress = XADDR_MAX;
            record.sName = sFileName;
            record.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)nUncompressedSize);

            listResult.append(record);
        }

        if ((nFileParts & FILEPART_REGION) && seaCanAppend(nLimit, listResult)) {
            FPART record = {};

            record.filePart = FILEPART_REGION;
            record.nFileOffset = nCurrentOffset;
            record.nFileSize = nHeaderSize + nCompressedSize;
            record.nVirtualAddress = XADDR_MAX;
            record.sName = sFileName;

            listResult.append(record);
        }

        nMaxOffset = nCurrentOffset + nHeaderSize + nCompressedSize;
        nCurrentOffset += (nHeaderSize + nCompressedSize);
    }

    // Add overlay if any
    if ((nFileParts & FILEPART_OVERLAY) && seaCanAppend(nLimit, listResult) && (nMaxOffset < nFileSize)) {
        FPART record = {};

        record.filePart = FILEPART_OVERLAY;
        record.nFileOffset = nMaxOffset;
        record.nFileSize = nFileSize - nMaxOffset;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Overlay");

        listResult.append(record);
    }

    return listResult;
}

QString XSEAARC::cmethodToString(CMETHOD cmethod)
{
    QString sResult = "Unknown";

    switch (cmethod) {
        case CMETHOD_END: sResult = "End"; break;
        case CMETHOD_STORE_OLD: sResult = "Stored (old)"; break;
        case CMETHOD_STORE: sResult = "Stored"; break;
        case CMETHOD_PACKED: sResult = "Packed (RLE)"; break;
        case CMETHOD_SQUEEZED: sResult = "Squeezed (Huffman)"; break;
        case CMETHOD_CRUNCHED1: sResult = "Crunched (LZW 9-bit)"; break;
        case CMETHOD_CRUNCHED2: sResult = "Crunched (LZW 9-12 bit)"; break;
        case CMETHOD_CRUNCHED3: sResult = "Crunched with pack"; break;
        case CMETHOD_CRUNCHED4: sResult = "Crunched (LZW dynamic)"; break;
        case CMETHOD_SQUASHED: sResult = "Squashed (LZW 13-bit)"; break;
    }

    return sResult;
}

qint32 XSEAARC::_getHeaderSize(quint8 nMethod)
{
    // Method 1 (old store): no original size field = 25 bytes header
    // Methods 2-9: has original size field = 29 bytes header
    if (nMethod == CMETHOD_STORE_OLD) {
        return 25;
    }

    return 29;
}

bool XSEAARC::_isValidMethod(quint8 nMethod)
{
    return (nMethod >= CMETHOD_STORE_OLD) && (nMethod <= CMETHOD_SQUASHED);
}

QList<QString> XSEAARC::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("1A");

    return listResult;
}

XBinary *XSEAARC::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XSEAARC(pDevice);
}

bool XSEAARC::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XSEAARC> guardedThis(this);
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo =
            static_cast<XArchive::INTERNAL_INFO *>(
                guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(
            guardedThis->m_internalInfo) = *pInfo;
    }

    return guardedThis && bResult;
}

void *XSEAARC::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XSEAARC> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XSEAARC::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
