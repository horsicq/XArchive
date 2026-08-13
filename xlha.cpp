/* Copyright (c) 2023-2026 hors<horsicq@gmail.com>
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
#include "xlha.h"
#include "xdecompress.h"

XBinary::XCONVERT _TABLE_XLHA_STRUCTID[] = {
    {XLHA::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XLHA::STRUCTID_HEADER, "HEADER", QString("Header")},
    {XLHA::STRUCTID_RECORD, "RECORD", QString("Record")},
};

static quint16 lhaReadLe16(const QByteArray &baData, qint32 nOffset)
{
    if ((nOffset < 0) || ((nOffset + 2) > baData.size())) return 0;
    return static_cast<quint16>(
        static_cast<quint8>(baData.at(nOffset)) |
        (static_cast<quint16>(static_cast<quint8>(
             baData.at(nOffset + 1))) << 8));
}

static quint32 lhaReadLe32(const QByteArray &baData, qint32 nOffset)
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

XLHA::XLHA(QIODevice *pDevice) : XArchive(pDevice)
{
}

// Level 1 archives store skip_sz at bytes 7-10 which equals ext_headers + csz.
// The extended header chain starts at nOffset+nBaseHeaderSize and each entry
// ends with a LE16 "next entry size" (0 = end of chain).
// Returns the total byte count occupied by extended headers.
qint64 XLHA::_getLevel1ExtHeadersSize(qint64 nOffset, qint64 nBaseHeaderSize)
{
    QPointer<XLHA> guardedArchive(this);
    qint64 nExtTotal = 0;
    qint64 nFnLen = (qint64)guardedArchive->read_uint8(nOffset + 21);
    if (!guardedArchive) return -1;
    qint64 nNextHdrSize =
        (qint64)guardedArchive->read_uint16(nOffset + 25 + nFnLen, false);
    if (!guardedArchive) return -1;
    qint64 nExtOffset = nOffset + nBaseHeaderSize;
    while (nNextHdrSize > 0) {
        nExtTotal += nNextHdrSize;
        nExtOffset += nNextHdrSize;
        nNextHdrSize =
            (qint64)guardedArchive->read_uint16(nExtOffset - 2, false);
        if (!guardedArchive) return -1;
    }
    return nExtTotal;
}

bool XLHA::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (XBinary::isPdStructNotCanceled(pPdStruct) && (getSize() >= 12)) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

        if (compareSignature(&memoryMap, "....'-lh'..2d", 0, pPdStruct) || compareSignature(&memoryMap, "....'-lz'..2d", 0, pPdStruct) ||
            compareSignature(&memoryMap, "....'-pm'..2d", 0, pPdStruct)) {
            QString sMethod = read_ansiString(2, 5);

            if ((sMethod == "-lzs-") || (sMethod == "-lz2-") || (sMethod == "-lz3-") || (sMethod == "-lz4-") || (sMethod == "-lz5-") || (sMethod == "-lz7-") ||
                (sMethod == "-lz8-") || (sMethod == "-lh0-") || (sMethod == "-lh1-") || (sMethod == "-lh2-") || (sMethod == "-lh3-") || (sMethod == "-lh4-") ||
                (sMethod == "-lh5-") || (sMethod == "-lh6-") || (sMethod == "-lh7-") || (sMethod == "-lh8-") || (sMethod == "-lh9-") || (sMethod == "-lha-") ||
                (sMethod == "-lhb-") || (sMethod == "-lhc-") || (sMethod == "-lhe-") || (sMethod == "-lhd-") || (sMethod == "-lhx-") || (sMethod == "-pm0-") ||
                (sMethod == "-pm2-")) {
                bResult = true;
            }
            bResult = true;
        }
    }

    return bResult;
}

bool XLHA::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLHA xhla(pDevice);

    return xhla.isValid(pPdStruct);
}

qint64 XLHA::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QList<XBinary::MAPMODE> XLHA::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XLHA::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

XBinary::FT XLHA::getFileType()
{
    return FT_LHA;
}

QString XLHA::getFileFormatExt()
{
    QString sResult = "lha";
    QString _sVersion = getVersion().left(2);

    if (_sVersion == "lh") {
        sResult = "lha";
    } else if (_sVersion == "lz") {
        sResult = "lzs";
    } else if (_sVersion == "pm") {
        sResult = "pma";
    }

    return sResult;
}

QString XLHA::getFileFormatExtsString()
{
    return "LHA(lha, lzs, pma)";
}

QString XLHA::getMIMEString()
{
    return "application/x-lzh-compressed";
}

QString XLHA::getVersion()
{
    return read_ansiString(3, 3);
}

QString XLHA::getArch()
{
    return QString();
}

XBinary::ENDIAN XLHA::getEndian()
{
    return ENDIAN_LITTLE;  // LHA is little-endian
}

XBinary::MODE XLHA::getMode()
{
    return MODE_DATA;
}

QMap<XBinary::UNPACK_PROP, QVariant> XLHA::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XLHA::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (m_bUnpackOperationInProgress) {
        return false;
    }
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XLHA> guardedArchive(this);

    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
            !guardedArchive->ownsUnpackSource(pState)) return false;
        guardedArchive->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const bool bBound = guardedArchive->bindUnpackSource(
            pState, pPdStruct);
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

        while ((nFileSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (nFileSize < 21) break;
            const QByteArray baHeader = guardedArchive->read_array(
                nOffset, 21);
            if (!guardedArchive) {
                *pState = UNPACK_STATE();
                return false;
            }
            if (baHeader.size() != 21) break;
            const QByteArray baMethodPrefix = baHeader.mid(2, 3);
            if (((baMethodPrefix == "-lh") ||
                 (baMethodPrefix == "-lz") ||
                 (baMethodPrefix == "-pm")) &&
                (baHeader.at(6) == '-')) {
                quint8 nLevel = static_cast<quint8>(baHeader.at(20));
                qint64 nHeaderSize = (nLevel == 2)
                    ? (qint64)lhaReadLe16(baHeader, 0)
                    : (qint64)(static_cast<quint8>(baHeader.at(0)) + 2);
                qint64 nCompressedSize = lhaReadLe32(baHeader, 7);

                if (nHeaderSize < 21) {
                    break;
                }

                const qint64 nRecordSize = nHeaderSize + nCompressedSize;
                if ((nRecordSize <= 0) || (nRecordSize > nFileSize)) {
                    break;
                }

                pState->nNumberOfRecords++;

                nOffset += nRecordSize;
                nFileSize -= nRecordSize;
            } else {
                break;
            }
        }

        bResult = (pState->nNumberOfRecords > 0) && XBinary::isPdStructNotCanceled(pPdStruct);
        if (bResult) {
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
    }

    return bResult;
}

XBinary::ARCHIVERECORD XLHA::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<XLHA> guardedArchive(this);

    XBinary::ARCHIVERECORD result = {};

    if (pState && guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) && guardedArchive && (pState->nCurrentIndex >= 0) &&
        (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        const QByteArray baPrefix = guardedArchive->read_array(
            pState->nCurrentOffset, 22);
        if (!guardedArchive || (baPrefix.size() != 22)) {
            return XBinary::ARCHIVERECORD();
        }
        quint8 nLevel = static_cast<quint8>(baPrefix.at(20));
        qint64 nHeaderSize = (nLevel == 2)
            ? (qint64)lhaReadLe16(baPrefix, 0)
            : (qint64)(static_cast<quint8>(baPrefix.at(0)) + 2);
        if (nHeaderSize < 22) return result;
        const QByteArray baHeader = guardedArchive->read_array(
            pState->nCurrentOffset, nHeaderSize);
        if (!guardedArchive || (baHeader.size() != nHeaderSize)) {
            return XBinary::ARCHIVERECORD();
        }
        qint64 nSkipSize = (qint64)lhaReadLe32(baHeader, 7);
        qint64 nUncompressedSize = lhaReadLe32(baHeader, 11);
        const qint32 nFileNameLength =
            static_cast<quint8>(baHeader.at(21));
        if ((22 + nFileNameLength) > baHeader.size()) return result;
        QString sFileName = QString::fromLatin1(
            baHeader.constData() + 22, nFileNameLength);
        sFileName = sFileName.replace("\\", "/");

        // For Level 1: skip_sz = ext_headers + compressed_data; resolve actual offset/size.
        qint64 nExtSize = (nLevel == 1)
            ? guardedArchive->_getLevel1ExtHeadersSize(
                  pState->nCurrentOffset, nHeaderSize)
            : 0;
        if (!guardedArchive || (nExtSize < 0)) {
            return XBinary::ARCHIVERECORD();
        }
        qint64 nCompressedSize = nSkipSize - nExtSize;
        if (nCompressedSize < 0) return result;

        result.nStreamOffset = pState->nCurrentOffset + nHeaderSize + nExtSize;
        result.nStreamSize = nCompressedSize;
        // result.nDecompressedOffset = 0;
        // result.nDecompressedSize = nUncompressedSize;

        result.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, sFileName);

        // Get compression method
        QString sMethod = QString::fromLatin1(baHeader.constData() + 2, 5);
        XBinary::HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;

        if ((sMethod == "-lh0-") || (sMethod == "-lz4-") || (sMethod == "-lhd-")) {
            compressMethod = HANDLE_METHOD_STORE;
        } else if (sMethod == "-lh1-") {
            compressMethod = HANDLE_METHOD_LZH1;  // LArc-compatible: adaptive Huffman + 4 KiB LZSS (LZHUF)
        } else if (sMethod == "-lh5-") {
            compressMethod = HANDLE_METHOD_LZH5;
        } else if (sMethod == "-lh6-") {
            compressMethod = HANDLE_METHOD_LZH6;
        } else if (sMethod == "-lh7-") {
            compressMethod = HANDLE_METHOD_LZH7;
        }

        result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);
        result.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, nCompressedSize);

        // Level 0/1 place the data CRC after the filename; level 2 keeps it
        // in the fixed header. Expose LHA's stored CRC-16 independently from
        // the SEA ARC-specific option, although both use the same polynomial.
        qint64 nCRCOffsetInHeader =
            (nLevel == 2) ? 21 : (22 + nFileNameLength);

        if ((nCRCOffsetInHeader + 2) <= baHeader.size()) {
            result.mapProperties.insert(XBinary::FPART_PROP_RESULTCRC,
                                        (quint32)lhaReadLe16(
                                            baHeader,
                                            nCRCOffsetInHeader));
            result.mapProperties.insert(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_CRC16);
        }
    }

    return result;
}

bool XLHA::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XLHA> guardedArchive(this);

    bool bResult = false;

    if (pState && guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) && guardedArchive && (pState->nCurrentIndex >= 0) &&
        (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        const QByteArray baHeader = guardedArchive->read_array(
            pState->nCurrentOffset, 21);
        if (!guardedArchive || (baHeader.size() != 21)) return false;
        quint8 nLevel = static_cast<quint8>(baHeader.at(20));
        qint64 nHeaderSize = (nLevel == 2)
            ? (qint64)lhaReadLe16(baHeader, 0)
            : (qint64)(static_cast<quint8>(baHeader.at(0)) + 2);
        qint64 nCompressedSize = lhaReadLe32(baHeader, 7);

        pState->nCurrentOffset += (nHeaderSize + nCompressedSize);
        pState->nCurrentIndex++;

        bResult = (pState->nCurrentIndex < pState->nNumberOfRecords);
    }

    return bResult;
}

bool XLHA::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) return false;
    releaseUnpackSource(pState);
    *pState = UNPACK_STATE();

    return true;
}

QString XLHA::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XLHA_STRUCTID, sizeof(_TABLE_XLHA_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XLHA::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XLHA_STRUCTID, sizeof(_TABLE_XLHA_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XLHA::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XLHA_STRUCTID, sizeof(_TABLE_XLHA_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XLHA::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
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
            quint8 nLevel = read_uint8(nHeaderOffset + 20);
            qint64 nHeaderSize = (nLevel == 2) ? (qint64)read_uint16(nHeaderOffset) : (qint64)(read_uint8(nHeaderOffset) + 2);

            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = nHeaderSize;
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

        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (!(compareSignature(xfStruct.pMemoryMap, "....'-lh'..2d", nCurrentOffset) || compareSignature(xfStruct.pMemoryMap, "....'-lz'..2d", nCurrentOffset) ||
                  compareSignature(xfStruct.pMemoryMap, "....'-pm'..2d", nCurrentOffset))) {
                break;
            }

            quint8 nLevel = read_uint8(nCurrentOffset + 20);
            qint64 nHeaderSize = (nLevel == 2) ? (qint64)read_uint16(nCurrentOffset) : (qint64)(read_uint8(nCurrentOffset) + 2);
            qint64 nSkipSize = (qint64)(quint32)read_uint32(nCurrentOffset + 7);

            if (nHeaderSize < 21) {
                break;
            }

            xfHeader.listRowLocations.append(nCurrentOffset);

            nCurrentOffset += (nHeaderSize + nSkipSize);

            if (nCurrentOffset >= nFileSize) {
                break;
            }
        }

        if (!xfHeader.listRowLocations.isEmpty()) {
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_RECORD, offsetToLoc(xfHeader.listRowLocations.first()));
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_RECORD), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XLHA::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)

    QList<XBinary::XFRECORD> listResult;

    if ((nStructID == STRUCTID_HEADER) || (nStructID == STRUCTID_RECORD)) {
        quint8 nLevel = read_uint8(xLoc.nLocation + 20);

        if (nLevel == 2) {
            listResult.append({"HeaderSize", 0, 2, XFRECORD_FLAG_SIZE, VT_UINT16});
        } else {
            listResult.append({"HeaderSize", 0, 1, XFRECORD_FLAG_SIZE, VT_UINT8});
            listResult.append({"HeaderChecksum", 1, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        }

        listResult.append({"Method", 2, 5, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"CompressedSize", 7, 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"UncompressedSize", 11, 4, XFRECORD_FLAG_SIZE, VT_UINT32});

        if (nLevel == 2) {
            listResult.append({"LastModTime", 15, 4, XFRECORD_FLAG_UNIXTIME, VT_UINT32});
        } else {
            listResult.append({"LastModTime", 15, 2, XFRECORD_FLAG_DOSTIME, VT_UINT16});
            listResult.append({"LastModDate", 17, 2, XFRECORD_FLAG_DOSDATE, VT_UINT16});
        }

        listResult.append({"Attribute", 19, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"Level", 20, 1, XFRECORD_FLAG_NONE, VT_UINT8});

        if (nLevel != 2) {
            quint8 nNameLength = read_uint8(xLoc.nLocation + 21);
            listResult.append({"NameLength", 21, 1, XFRECORD_FLAG_SIZE, VT_UINT8});
            listResult.append({"FileName", 22, (qint32)nNameLength, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
            listResult.append({"CRC16", 22 + (qint32)nNameLength, 2, XFRECORD_FLAG_NONE, VT_UINT16});
        } else {
            listResult.append({"CRC16", 21, 2, XFRECORD_FLAG_NONE, VT_UINT16});
            listResult.append({"OSID", 23, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        }
    }

    return listResult;
}

// QList<XBinary::DATA_HEADER> XLHA::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

//         // Count records for table
//         qint64 nRealSize = 0;
//         qint32 nCount = 0;

//         qint64 nFileSize = getSize();
//         qint64 nOffset = 0;

//         while ((nFileSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
//             if (compareSignature(dataHeadersOptions.pMemoryMap, "....'-lh'..2d", nOffset) || compareSignature(dataHeadersOptions.pMemoryMap, "....'-lz'..2d", nOffset)
//             ||
//                 compareSignature(dataHeadersOptions.pMemoryMap, "....'-pm'..2d", nOffset)) {
//                 quint8 nLevel = read_uint8(nOffset + 20);
//                 qint64 nHeaderSize = (nLevel == 2) ? (qint64)read_uint16(nOffset) : (qint64)(read_uint8(nOffset) + 2);
//                 qint64 nDataSize = read_uint32(nOffset + 7);

//                 if (nHeaderSize < 21) {
//                     break;
//                 }

//                 nCount++;
//                 nRealSize = nOffset + nHeaderSize + nDataSize;

//                 nOffset += (nHeaderSize + nDataSize);
//                 nFileSize -= (nHeaderSize + nDataSize);
//             } else {
//                 break;
//             }
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
//                 // Table of records
//                 qint64 nCurrentOffset = nStartOffset;
//                 qint32 nCount = 0;

//                 while ((nCount < dataHeadersOptions.nCount) && XBinary::isPdStructNotCanceled(pPdStruct)) {
//                     if (compareSignature(dataHeadersOptions.pMemoryMap, "....'-lh'..2d", nCurrentOffset) ||
//                         compareSignature(dataHeadersOptions.pMemoryMap, "....'-lz'..2d", nCurrentOffset) ||
//                         compareSignature(dataHeadersOptions.pMemoryMap, "....'-pm'..2d", nCurrentOffset)) {
//                         quint8 nLevel = read_uint8(nCurrentOffset + 20);
//                         qint64 nHeaderSize = (nLevel == 2) ? (qint64)read_uint16(nCurrentOffset) : (qint64)(read_uint8(nCurrentOffset) + 2);
//                         qint64 nSkipSize = (qint64)(quint32)read_uint32(nCurrentOffset + 7);
//                         qint64 nExtSize = (nLevel == 1) ? _getLevel1ExtHeadersSize(nCurrentOffset, nHeaderSize) : 0;
//                         qint64 nDataSize = nSkipSize - nExtSize;
//                         QString sFileName = read_ansiString(nCurrentOffset + 22, read_uint8(nCurrentOffset + 21));

//                         DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(STRUCTID_RECORD));
//                         dataHeader.nSize = nHeaderSize + nSkipSize;

//                         // Record header fields
//                         dataHeader.listRecords.append(getDataRecord(0, 1, "Header Size", VT_UINT8, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(1, 1, "Header CRC", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(2, 5, "Compression Method", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(7, 4, "Compressed Size", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(11, 4, "Uncompressed Size", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(15, 2, "Last Mod Time", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(17, 2, "Last Mod Date", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(19, 1, "File Attribute", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(getDataRecord(20, 1, "Name Length", VT_UINT8, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                         dataHeader.listRecords.append(
//                             getDataRecord(21, read_uint8(nCurrentOffset + 21), "File Name", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                         if (nHeaderSize > 22 + read_uint8(nCurrentOffset + 21)) {
//                             dataHeader.listRecords.append(getDataRecord(22 + read_uint8(nCurrentOffset + 21), nHeaderSize - (22 + read_uint8(nCurrentOffset + 21)),
//                                                                         "Extended Header", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                         }

//                         if (nExtSize > 0) {
//                             dataHeader.listRecords.append(
//                                 getDataRecord(nHeaderSize, nExtSize, "Extended Headers", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                         }

//                         if (nDataSize > 0) {
//                             dataHeader.listRecords.append(
//                                 getDataRecord(nHeaderSize + nExtSize, nDataSize, "Compressed Data", VT_BYTE_ARRAY, DRF_UNKNOWN,
//                                 dataHeadersOptions.pMemoryMap->endian));
//                         }

//                         listResult.append(dataHeader);

//                         nCurrentOffset += (nHeaderSize + nSkipSize);
//                         nCount++;
//                     } else {
//                         break;
//                     }
//                 }
//             }
//         }
//     }

//     return listResult;
// }

static bool lhaCanAppendPart(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XLHA::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    qint64 nFileSize = getSize();
    qint64 nCurrentOffset = 0;
    qint64 nMaxOffset = 0;
    _MEMORY_MAP memoryMap = XBinary::getMemoryMap();

    // Iterate through all records and create file parts
    while ((nFileSize > 0) && lhaCanAppendPart(nLimit, listResult) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (compareSignature(&memoryMap, "....'-lh'..2d", nCurrentOffset) || compareSignature(&memoryMap, "....'-lz'..2d", nCurrentOffset) ||
            compareSignature(&memoryMap, "....'-pm'..2d", nCurrentOffset)) {
            quint8 nLevel = read_uint8(nCurrentOffset + 20);
            qint64 nHeaderSize = (nLevel == 2) ? (qint64)read_uint16(nCurrentOffset) : (qint64)(read_uint8(nCurrentOffset) + 2);
            qint64 nSkipSize = (qint64)(quint32)read_uint32(nCurrentOffset + 7);
            qint64 nExtSize = (nLevel == 1) ? _getLevel1ExtHeadersSize(nCurrentOffset, nHeaderSize) : 0;
            qint64 nDataSize = nSkipSize - nExtSize;
            QString sFileName = read_ansiString(nCurrentOffset + 22, read_uint8(nCurrentOffset + 21));

            if (nHeaderSize < 21) {
                break;
            }

            // Header part
            if ((nFileParts & FILEPART_HEADER) && lhaCanAppendPart(nLimit, listResult)) {
                FPART record = {};

                record.filePart = FILEPART_HEADER;
                record.nFileOffset = nCurrentOffset;
                record.nFileSize = nHeaderSize;
                record.nVirtualAddress = XADDR_MAX;
                record.sName = tr("Header");

                listResult.append(record);
            }

            // Data/Stream part
            if ((nFileParts & FILEPART_STREAM) && lhaCanAppendPart(nLimit, listResult)) {
                FPART record = {};

                record.filePart = FILEPART_STREAM;
                record.nFileOffset = nCurrentOffset + nHeaderSize + nExtSize;
                record.nFileSize = nDataSize;
                record.nVirtualAddress = XADDR_MAX;
                record.sName = sFileName;
                record.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, read_uint32(nCurrentOffset + 11));

                listResult.append(record);
            }

            // Region part (header + data)
            if ((nFileParts & FILEPART_REGION) && lhaCanAppendPart(nLimit, listResult)) {
                FPART record = {};

                record.filePart = FILEPART_REGION;
                record.nFileOffset = nCurrentOffset;
                record.nFileSize = nHeaderSize + nSkipSize;
                record.nVirtualAddress = XADDR_MAX;
                record.sName = sFileName;

                listResult.append(record);
            }

            nMaxOffset = nCurrentOffset + nHeaderSize + nSkipSize;
            nCurrentOffset += (nHeaderSize + nSkipSize);
            nFileSize -= (nHeaderSize + nSkipSize);
        } else {
            break;
        }
    }

    // Data part (all archive data)
    if ((nFileParts & FILEPART_DATA) && lhaCanAppendPart(nLimit, listResult)) {
        FPART record = {};

        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = nMaxOffset;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Data");

        listResult.append(record);
    }

    // Overlay part (any trailing data)
    if ((nFileParts & FILEPART_OVERLAY) && lhaCanAppendPart(nLimit, listResult)) {
        if (nMaxOffset < getSize()) {
            FPART record = {};

            record.filePart = FILEPART_OVERLAY;
            record.nFileOffset = nMaxOffset;
            record.nFileSize = getSize() - nMaxOffset;
            record.nVirtualAddress = XADDR_MAX;
            record.sName = tr("Overlay");

            listResult.append(record);
        }
    }

    return listResult;
}

QList<QString> XLHA::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("....'-lh'..2d");
    listResult.append("....'-lz'..2d");
    listResult.append("....'-pm'..2d");

    return listResult;
}

XBinary *XLHA::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XLHA(pDevice);
}

bool XLHA::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XLHA> guardedThis(this);
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

void *XLHA::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XLHA> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XLHA::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
