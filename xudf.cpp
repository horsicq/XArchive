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
#include "xudf.h"
#include "Algos/xstoredecoder.h"

#include <new>
#include <QPointer>

static XBinary::XCONVERT _TABLE_XUDF_STRUCTID[] = {{XUDF::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                   {XUDF::STRUCTID_TAG, "TAG", QString("Tag")},
                                                   {XUDF::STRUCTID_ANCHOR_VOLUME_DESCRIPTOR, "ANCHOR_VOLUME_DESCRIPTOR", QString("Anchor Volume Descriptor")},
                                                   {XUDF::STRUCTID_PRIMARY_VOLUME_DESCRIPTOR, "PRIMARY_VOLUME_DESCRIPTOR", QString("Primary Volume Descriptor")},
                                                   {XUDF::STRUCTID_FILE_ENTRY, "FILE_ENTRY", QString("File Entry")}};

XUDF::XUDF(QIODevice *pDevice) : XArchive(pDevice)
{
    QPointer<XUDF> guardedThis(this);
    QPointer<QIODevice> guardedDevice(pDevice);
    const bool bValid = guardedThis && guardedThis->isValid();
    if (!guardedThis || !guardedDevice || !bValid) {
        return;
    }

    const QString sVolumeIdentifier = guardedThis->getVolumeIdentifier();
    if (!guardedThis || !guardedDevice) {
        return;
    }
    guardedThis->m_sVolumeIdentifier = sVolumeIdentifier;

    const QString sVolumeSetIdentifier = guardedThis->getVolumeSetIdentifier();
    if (guardedThis && guardedDevice) {
        guardedThis->m_sVolumeSetIdentifier = sVolumeSetIdentifier;
    }
}

XUDF::~XUDF()
{
}

bool XUDF::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;
    QPointer<XUDF> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());

    // A UDF volume has to be large enough for the Volume Recognition Sequence
    // (ECMA-167 2/9.1, first byte at 32768) plus at least one more logical
    // sector.  Everything beyond that is decided by _getAnchorVolumeDescriptorOffset(),
    // which only reports an offset when a *checksum-validated* ECMA-167 Anchor
    // Volume Descriptor Pointer is recorded there.  A bare "the 16-bit word at
    // this offset happens to be 2" test used to accept arbitrary binaries -
    // ordinary ELF objects among them - because the tag checksum, the reserved
    // byte, the descriptor version, the tag location and the descriptor CRC
    // were never looked at.
    if (XBinary::isPdStructNotCanceled(pPdStruct) && (getSize() >= (0x8000 + 2048)) && guardedDevice) {
        const qint64 nAnchorOffset = _getAnchorVolumeDescriptorOffset();
        if (!guardedThis || !guardedDevice) {
            return false;
        }

        bResult = (nAnchorOffset != -1);
    }

    return bResult;
}

bool XUDF::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    XUDF xudf(pDevice);
    if (!guardedDevice) {
        return false;
    }
    return xudf.isValid(pPdStruct);
}

QString XUDF::getFileFormatExt()
{
    return "udf";
}

QString XUDF::getFileFormatExtsString()
{
    return "UDF files (*.udf *.iso *.img)";
}

qint64 XUDF::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

QString XUDF::getMIMEString()
{
    return "application/x-udf";
}

XBinary::FT XUDF::getFileType()
{
    return FT_UDF;
}

QList<XBinary::MAPMODE> XUDF::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XUDF::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XUDF::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XUDF_STRUCTID, sizeof(_TABLE_XUDF_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XUDF::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XUDF_STRUCTID, sizeof(_TABLE_XUDF_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XUDF::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XUDF_STRUCTID, sizeof(_TABLE_XUDF_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XUDF::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;
    QPointer<XUDF> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        qint64 nAnchorOffset = _getAnchorVolumeDescriptorOffset();
        if (!guardedThis || !guardedDevice) {
            return listResult;
        }

        if (nAnchorOffset != -1) {
            XFSTRUCT _xfStruct = xfStruct;
            _xfStruct.nStructID = STRUCTID_ANCHOR_VOLUME_DESCRIPTOR;
            _xfStruct.xLoc = offsetToLoc(nAnchorOffset);
            listResult.append(getXFHeaders(_xfStruct, pPdStruct));
        }
    } else if (nStructID == STRUCTID_ANCHOR_VOLUME_DESCRIPTOR) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            const qint64 nAnchorOffset = _getAnchorVolumeDescriptorOffset();
            if (!guardedThis || !guardedDevice) {
                return listResult;
            }
            headerLoc = offsetToLoc(nAnchorOffset);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if (nHeaderOffset != -1) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_ANCHOR_VOLUME_DESCRIPTOR);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = sizeof(UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_ANCHOR_VOLUME_DESCRIPTOR, headerLoc);
            if (!guardedThis || !guardedDevice) {
                return listResult;
            }
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_ANCHOR_VOLUME_DESCRIPTOR), xfHeader.sParentTag);
            if (!guardedThis || !guardedDevice) {
                return listResult;
            }
            listResult.append(xfHeader);

            if (xfStruct.bIsParent) {
                XFSTRUCT _xfStruct = xfStruct;
                _xfStruct.sParent = xfHeader.sTag;
                _xfStruct.nStructID = STRUCTID_PRIMARY_VOLUME_DESCRIPTOR;
                _xfStruct.xLoc = offsetToLoc(-1);
                listResult.append(getXFHeaders(_xfStruct, pPdStruct));
            }
        }
    } else if (nStructID == STRUCTID_PRIMARY_VOLUME_DESCRIPTOR) {
        qint64 nPvdOffset = locToOffset(xfStruct.pMemoryMap, xfStruct.xLoc);

        if (nPvdOffset == -1) {
            // Scan the main Volume Descriptor Sequence for the PVD
            qint64 nAnchorOffset = _getAnchorVolumeDescriptorOffset();
            if (!guardedThis || !guardedDevice) {
                return listResult;
            }

            if (nAnchorOffset != -1) {
                UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER avdp = {};
                if (!_readAnchorVolumeDescriptor(nAnchorOffset, &avdp) || !guardedThis || !guardedDevice) {
                    return listResult;
                }

                qint32 nBlockSize = _getBlockSize();
                qint64 nVdsOffset = (qint64)avdp.mainVolumeDescriptorSequenceExtent.nLocation * nBlockSize;
                qint32 nNumberOfBlocks = (qint32)(avdp.mainVolumeDescriptorSequenceExtent.nLength / nBlockSize);

                for (qint32 i = 0; (i < nNumberOfBlocks) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
                    qint64 nCurrentOffset = nVdsOffset + (qint64)i * nBlockSize;

                    if (!_isValidTag(nCurrentOffset, pPdStruct)) {
                        break;
                    }
                    if (!guardedThis || !guardedDevice) {
                        return listResult;
                    }

                    UDF_TAG tag = {};
                    if (!_readTag(nCurrentOffset, &tag) || !guardedThis || !guardedDevice) {
                        return listResult;
                    }

                    if (tag.nTagIdentifier == TAG_PRIMARY_VOLUME_DESCRIPTOR) {
                        nPvdOffset = nCurrentOffset;
                        break;
                    } else if (tag.nTagIdentifier == TAG_TERMINATING_DESCRIPTOR) {
                        break;
                    }
                }
            }
        }

        if (nPvdOffset != -1) {
            XLOC pvdLoc = offsetToLoc(nPvdOffset);

            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_PRIMARY_VOLUME_DESCRIPTOR);
            xfHeader.xLoc = pvdLoc;
            xfHeader.nSize = sizeof(UDF_PRIMARY_VOLUME_DESCRIPTOR);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_PRIMARY_VOLUME_DESCRIPTOR, pvdLoc);
            if (!guardedThis || !guardedDevice) {
                return listResult;
            }
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_PRIMARY_VOLUME_DESCRIPTOR), xfHeader.sParentTag);
            if (!guardedThis || !guardedDevice) {
                return listResult;
            }
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

void XUDF::_addTagRecords(QList<XFRECORD> *pList, qint32 nBaseOffset)
{
    pList->append({"tag.nTagIdentifier", nBaseOffset + (qint32)offsetof(UDF_TAG, nTagIdentifier), 2, XFRECORD_FLAG_NONE, VT_UINT16});
    pList->append({"tag.nDescriptorVersion", nBaseOffset + (qint32)offsetof(UDF_TAG, nDescriptorVersion), 2, XFRECORD_FLAG_VERSION, VT_UINT16});
    pList->append({"tag.nChecksum", nBaseOffset + (qint32)offsetof(UDF_TAG, nChecksum), 1, XFRECORD_FLAG_NONE, VT_UINT8});
    pList->append({"tag.nTagSerialNumber", nBaseOffset + (qint32)offsetof(UDF_TAG, nTagSerialNumber), 2, XFRECORD_FLAG_NONE, VT_UINT16});
    pList->append({"tag.nDescriptorCRC", nBaseOffset + (qint32)offsetof(UDF_TAG, nDescriptorCRC), 2, XFRECORD_FLAG_NONE, VT_UINT16});
    pList->append({"tag.nDescriptorCRCLength", nBaseOffset + (qint32)offsetof(UDF_TAG, nDescriptorCRCLength), 2, XFRECORD_FLAG_SIZE, VT_UINT16});
    pList->append({"tag.nTagLocation", nBaseOffset + (qint32)offsetof(UDF_TAG, nTagLocation), 4, XFRECORD_FLAG_NONE, VT_UINT32});
}

QList<XBinary::XFRECORD> XUDF::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_TAG) {
        _addTagRecords(&listResult, 0);
    } else if (nStructID == STRUCTID_ANCHOR_VOLUME_DESCRIPTOR) {
        _addTagRecords(&listResult, (qint32)offsetof(UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER, tag));
        listResult.append({"mainVolumeDescriptorSequenceExtent.nLength",
                           (qint32)(offsetof(UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER, mainVolumeDescriptorSequenceExtent) + offsetof(UDF_EXTENT_AD, nLength)), 4,
                           XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"mainVolumeDescriptorSequenceExtent.nLocation",
                           (qint32)(offsetof(UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER, mainVolumeDescriptorSequenceExtent) + offsetof(UDF_EXTENT_AD, nLocation)), 4,
                           XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"reserveVolumeDescriptorSequenceExtent.nLength",
                           (qint32)(offsetof(UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER, reserveVolumeDescriptorSequenceExtent) + offsetof(UDF_EXTENT_AD, nLength)), 4,
                           XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"reserveVolumeDescriptorSequenceExtent.nLocation",
                           (qint32)(offsetof(UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER, reserveVolumeDescriptorSequenceExtent) + offsetof(UDF_EXTENT_AD, nLocation)), 4,
                           XFRECORD_FLAG_NONE, VT_UINT32});
    } else if (nStructID == STRUCTID_PRIMARY_VOLUME_DESCRIPTOR) {
        _addTagRecords(&listResult, (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, tag));
        listResult.append({"nVolumeDescriptorSequenceNumber", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nVolumeDescriptorSequenceNumber), 4, XFRECORD_FLAG_COUNT,
                           VT_UINT32});
        listResult.append(
            {"nPrimaryVolumeDescriptorNumber", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nPrimaryVolumeDescriptorNumber), 4, XFRECORD_FLAG_COUNT, VT_UINT32});
        listResult.append({"szVolumeIdentifier", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, szVolumeIdentifier), 32, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"nVolumeSequenceNumber", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nVolumeSequenceNumber), 2, XFRECORD_FLAG_COUNT, VT_UINT16});
        listResult.append(
            {"nMaximumVolumeSequenceNumber", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nMaximumVolumeSequenceNumber), 2, XFRECORD_FLAG_COUNT, VT_UINT16});
        listResult.append({"nInterchangeLevel", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nInterchangeLevel), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"nMaximumInterchangeLevel", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nMaximumInterchangeLevel), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"nCharacterSetList", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nCharacterSetList), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"nMaximumCharacterSetList", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nMaximumCharacterSetList), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"szVolumeSetIdentifier", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, szVolumeSetIdentifier), 128, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append(
            {"nDescriptorCharacterSet", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nDescriptorCharacterSet), 64, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append(
            {"nExplanatoryCharacterSet", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nExplanatoryCharacterSet), 64, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"volumeAbstract.nLength", (qint32)(offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, volumeAbstract) + offsetof(UDF_EXTENT_AD, nLength)), 4,
                           XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"volumeAbstract.nLocation", (qint32)(offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, volumeAbstract) + offsetof(UDF_EXTENT_AD, nLocation)), 4,
                           XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"volumeCopyrightNotice.nLength", (qint32)(offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, volumeCopyrightNotice) + offsetof(UDF_EXTENT_AD, nLength)),
                           4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"volumeCopyrightNotice.nLocation",
                           (qint32)(offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, volumeCopyrightNotice) + offsetof(UDF_EXTENT_AD, nLocation)), 4, XFRECORD_FLAG_NONE,
                           VT_UINT32});
        listResult.append({"nApplicationIdentifier", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nApplicationIdentifier), 32, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"nRecordingDateAndTime", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nRecordingDateAndTime), 12, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append(
            {"nImplementationIdentifier", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nImplementationIdentifier), 32, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"nImplementationUse", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nImplementationUse), 64, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"nPredecessorVolumeDescriptorSequenceLocation",
                           (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nPredecessorVolumeDescriptorSequenceLocation), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"nFlags", (qint32)offsetof(UDF_PRIMARY_VOLUME_DESCRIPTOR, nFlags), 2, XFRECORD_FLAG_NONE, VT_UINT16});
    } else if (nStructID == STRUCTID_FILE_ENTRY) {
        _addTagRecords(&listResult, (qint32)offsetof(UDF_FILE_ENTRY, tag));
        listResult.append({"nICBTag", (qint32)offsetof(UDF_FILE_ENTRY, nICBTag), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"nUid", (qint32)offsetof(UDF_FILE_ENTRY, nUid), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"nGid", (qint32)offsetof(UDF_FILE_ENTRY, nGid), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"nPermissions", (qint32)offsetof(UDF_FILE_ENTRY, nPermissions), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"nFileLinkCount", (qint32)offsetof(UDF_FILE_ENTRY, nFileLinkCount), 2, XFRECORD_FLAG_COUNT, VT_UINT16});
        listResult.append({"nRecordFormat", (qint32)offsetof(UDF_FILE_ENTRY, nRecordFormat), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"nRecordDisplayAttributes", (qint32)offsetof(UDF_FILE_ENTRY, nRecordDisplayAttributes), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"nRecordLength", (qint32)offsetof(UDF_FILE_ENTRY, nRecordLength), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"nInformationLength", (qint32)offsetof(UDF_FILE_ENTRY, nInformationLength), 8, XFRECORD_FLAG_SIZE, VT_UINT64});
        listResult.append({"nLogicalBlocksRecorded", (qint32)offsetof(UDF_FILE_ENTRY, nLogicalBlocksRecorded), 8, XFRECORD_FLAG_COUNT, VT_UINT64});
        listResult.append({"nAccessTime", (qint32)offsetof(UDF_FILE_ENTRY, nAccessTime), 12, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"nModificationTime", (qint32)offsetof(UDF_FILE_ENTRY, nModificationTime), 12, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"nAttributeTime", (qint32)offsetof(UDF_FILE_ENTRY, nAttributeTime), 12, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"nCheckpoint", (qint32)offsetof(UDF_FILE_ENTRY, nCheckpoint), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"nExtendedAttributeICB", (qint32)offsetof(UDF_FILE_ENTRY, nExtendedAttributeICB), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"nImplementationIdentifier", (qint32)offsetof(UDF_FILE_ENTRY, nImplementationIdentifier), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"nUniqueID", (qint32)offsetof(UDF_FILE_ENTRY, nUniqueID), 8, XFRECORD_FLAG_NONE, VT_UINT64});
        listResult.append({"nLengthOfExtendedAttributes", (qint32)offsetof(UDF_FILE_ENTRY, nLengthOfExtendedAttributes), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"nLengthOfAllocationDescriptors", (qint32)offsetof(UDF_FILE_ENTRY, nLengthOfAllocationDescriptors), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
    }

    return listResult;
}

// QList<XBinary::DATA_HEADER> XUDF::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

//         _dataHeadersOptions.nID = STRUCTID_ANCHOR_VOLUME_DESCRIPTOR;
//         _dataHeadersOptions.nLocation = _getAnchorVolumeDescriptorOffset();
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//     } else if (dataHeadersOptions.nID == STRUCTID_ANCHOR_VOLUME_DESCRIPTOR) {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XUDF::structIDToString(dataHeadersOptions.nID));
//             dataHeader.nSize = 512;

//             dataHeader.listRecords.append(getDataRecord(0, 16, "Tag", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//             dataHeader.listRecords.append(
//                 getDataRecord(16, 8, "Main Volume Descriptor Sequence Extent", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//             dataHeader.listRecords.append(
//                 getDataRecord(24, 8, "Reserve Volume Descriptor Sequence Extent", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//             listResult.append(dataHeader);
//         }
//     }

//     return listResult;
// }

static bool _udfCanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XUDF::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    QPointer<XUDF> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    qint64 nTotalSize = getSize();
    qint64 nFormatSize = getFileFormatSize(pPdStruct);
    if (!guardedThis || !guardedDevice) {
        return listResult;
    }

    if ((nFileParts & FILEPART_HEADER) && _udfCanAppend(nLimit, listResult)) {
        qint64 nAnchorOffset = _getAnchorVolumeDescriptorOffset();
        if (!guardedThis || !guardedDevice) {
            return listResult;
        }

        if (nAnchorOffset != -1) {
            FPART record = {};
            record.filePart = FILEPART_HEADER;
            record.nFileOffset = nAnchorOffset;
            record.nFileSize = 512;
            record.nVirtualAddress = XADDR_MAX;
            record.sName = tr("Anchor Volume Descriptor");

            listResult.append(record);
        }
    }

    if ((nFileParts & FILEPART_STREAM) && _udfCanAppend(nLimit, listResult)) {
        qint64 nAnchorOffset = _getAnchorVolumeDescriptorOffset();
        if (!guardedThis || !guardedDevice) {
            return listResult;
        }
        qint64 nStreamOffset = 0;
        qint64 nStreamSize = nTotalSize;

        if (nAnchorOffset != -1) {
            nStreamSize = nAnchorOffset;  // Data before anchor
        }

        if (nStreamSize > 0) {
            FPART record = {};
            record.filePart = FILEPART_STREAM;
            record.nFileOffset = nStreamOffset;
            record.nFileSize = nStreamSize;
            record.nVirtualAddress = XADDR_MAX;
            record.sName = tr("Data");

            listResult.append(record);
        }
    }

    if ((nFileParts & FILEPART_OVERLAY) && _udfCanAppend(nLimit, listResult)) {
        if (nFormatSize > 0 && nFormatSize < nTotalSize) {
            FPART record = {};
            record.filePart = FILEPART_OVERLAY;
            record.nFileOffset = nFormatSize;
            record.nFileSize = nTotalSize - nFormatSize;
            record.nVirtualAddress = XADDR_MAX;
            record.sName = tr("Overlay");

            listResult.append(record);
        }
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XUDF::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XUDF::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XUDF> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());

    // Refuse a state that belongs to someone else before touching anything:
    // without this a foreign state's context would be deleted below, and a
    // caller holding a copy of it would then double-free.
    if (!pState || m_bUnpackOperationInProgress ||
        ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedThis->ownsUnpackSource(pState))) {
        return false;
    }
    if (!guardedThis->finishUnpack(pState, nullptr) || !guardedThis || !guardedDevice) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!isPdStructNotCanceled(pPdStruct) || !isValid(pPdStruct)) {
        return false;
    }
    if (!guardedThis || !guardedDevice) {
        return false;
    }

    // Binding is what lets every later call authenticate the state against
    // this device; it only takes effect once finalized below.
    const bool bBound = guardedThis->bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !guardedDevice || !bBound) return false;

    pState->mapUnpackProperties = mapProperties;

    UDF_UNPACK_CONTEXT *pContext = new (std::nothrow) UDF_UNPACK_CONTEXT;
    if (!pContext) {
        guardedThis->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    pContext->nBlockSize = _getBlockSize();
    pContext->listRecords = _parseFileSystem(pContext->nBlockSize, pPdStruct);
    if (!guardedThis || !guardedDevice) {
        delete pContext;
        return false;
    }
    pContext->nCurrentRecordIndex = 0;

    if (!isPdStructNotCanceled(pPdStruct)) {
        delete pContext;
        guardedThis->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listRecords.count();
    pState->nCurrentOffset = 0;
    pState->nTotalSize = getSize();

    // A bound source is not usable until it is finalized: isUnpackSourceCurrent
    // authenticates against the finalized token, so skipping this leaves every
    // later record check failing - listing would work while extraction quietly
    // produced nothing.
    bool bResult = guardedThis->validateAndFinalizeUnpackSource(pState, pPdStruct);
    if (!guardedThis) {
        *pState = UNPACK_STATE();
        return false;
    }

    if (!bResult) {
        delete pContext;
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
    }

    return bResult;
}

XBinary::ARCHIVERECORD XUDF::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    // The nested-authorized form: XArchive::unpackCurrent already holds the
    // operation guard when it calls this, so the single-argument form would
    // refuse and hand back an empty record.
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return ARCHIVERECORD();

    ARCHIVERECORD result = {};

    if (!isUnpackSourceCurrent(pState, pPdStruct)) return result;

    if (isPdStructNotCanceled(pPdStruct) && pState && pState->pContext && (pState->nCurrentIndex >= 0) &&
        (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        UDF_UNPACK_CONTEXT *pContext = (UDF_UNPACK_CONTEXT *)pState->pContext;

        if ((pContext->nCurrentRecordIndex >= 0) && (pContext->nCurrentRecordIndex < pContext->listRecords.count()) &&
            (pContext->nCurrentRecordIndex == pState->nCurrentIndex)) {
            result = pContext->listRecords.at(pContext->nCurrentRecordIndex);
        }
    }

    return result;
}

bool XUDF::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;
    QPointer<XUDF> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    QPointer<QIODevice> guardedOutput(pDevice);

    // Authenticate the state against this device. Without it a stale or
    // foreign state extracts a different image's bytes under the current
    // record's name.
    if (!isUnpackSourceCurrent(pState, pPdStruct)) return false;

    if (pState && pState->pContext && guardedSource && guardedOutput) {
        UDF_UNPACK_CONTEXT *pContext = (UDF_UNPACK_CONTEXT *)pState->pContext;

        if (pContext->nCurrentRecordIndex < pContext->listRecords.count()) {
            ARCHIVERECORD ar = pContext->listRecords.at(pContext->nCurrentRecordIndex);

            if (!ar.mapProperties.value(FPART_PROP_ISFOLDER).toBool()) {
                if (!XBinary::isUnpackOutputSizeAllowed(
                        pState->mapUnpackProperties, ar.nStreamSize)) {
                    return false;
                }
                XBinary::DATAPROCESS_STATE decompressState = {};
                decompressState.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, XArchive::HANDLE_METHOD_STORE);
                decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, ar.nStreamSize);
                decompressState.mapUnpackProperties = pState->mapUnpackProperties;
                decompressState.pDeviceInput = guardedSource.data();
                decompressState.pDeviceOutput = guardedOutput.data();
                decompressState.nInputOffset = ar.nStreamOffset;
                decompressState.nInputLimit = ar.nStreamSize;
                decompressState.nProcessedOffset = 0;
                decompressState.nProcessedLimit = -1;

                bResult = XStoreDecoder::decompress(&decompressState, pPdStruct);
                if (!guardedThis || !guardedSource || !guardedOutput) {
                    return false;
                }
            } else {
                bResult = true;
            }
        }
    }

    return bResult;
}

bool XUDF::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    bool bResult = false;

    if (!isUnpackSourceCurrent(pState, pPdStruct)) return false;

    if (isPdStructNotCanceled(pPdStruct) && pState && pState->pContext && (pState->nCurrentIndex >= 0) &&
        (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        UDF_UNPACK_CONTEXT *pContext = (UDF_UNPACK_CONTEXT *)pState->pContext;

        pContext->nCurrentRecordIndex++;
        pState->nCurrentIndex++;

        bResult = (pContext->nCurrentRecordIndex < pContext->listRecords.count());
    }

    return bResult;
}

bool XUDF::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    // The ownership gate has to precede the delete. UNPACK_STATE is copied by
    // value in production code (xtarcompressed.cpp:486), so finishing a state
    // this archive does not own would free a context its real owner still
    // holds, and the owner would then free it again.
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    if (pState->pContext) {
        UDF_UNPACK_CONTEXT *pContext = (UDF_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
    }

    releaseUnpackSource(pState);

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    return true;
}

XUDF::UDF_TAG XUDF::_readTag(qint64 nOffset)
{
    UDF_TAG tag = {};
    _readTag(nOffset, &tag);
    return tag;
}

XUDF::UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER XUDF::_readAnchorVolumeDescriptor(qint64 nOffset)
{
    UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER desc = {};
    _readAnchorVolumeDescriptor(nOffset, &desc);
    return desc;
}

XUDF::UDF_PRIMARY_VOLUME_DESCRIPTOR XUDF::_readPrimaryVolumeDescriptor(qint64 nOffset)
{
    UDF_PRIMARY_VOLUME_DESCRIPTOR desc = {};
    _readPrimaryVolumeDescriptor(nOffset, &desc);
    return desc;
}

bool XUDF::_readExact(qint64 nOffset, char *pData, qint64 nSize)
{
    if ((nOffset < 0) || !pData || (nSize <= 0)) {
        return false;
    }

    QPointer<XUDF> guardedThis(this);
    QPointer<QIODevice> guardedDevice(guardedThis ? guardedThis->getDevice() : nullptr);
    if (!guardedThis || !guardedDevice) {
        return false;
    }

    const qint64 nRead = guardedThis->read_array(nOffset, pData, nSize);
    return guardedThis && guardedDevice && (nRead == nSize);
}

bool XUDF::_readTag(qint64 nOffset, UDF_TAG *pTag)
{
    if (!pTag) {
        return false;
    }

    *pTag = {};
    return _readExact(nOffset, reinterpret_cast<char *>(pTag), sizeof(UDF_TAG));
}

bool XUDF::_readAnchorVolumeDescriptor(qint64 nOffset, UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER *pDescriptor)
{
    if (!pDescriptor) {
        return false;
    }

    *pDescriptor = {};
    return _readExact(nOffset, reinterpret_cast<char *>(pDescriptor), sizeof(UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER));
}

bool XUDF::_readPrimaryVolumeDescriptor(qint64 nOffset, UDF_PRIMARY_VOLUME_DESCRIPTOR *pDescriptor)
{
    if (!pDescriptor) {
        return false;
    }

    *pDescriptor = {};
    return _readExact(nOffset, reinterpret_cast<char *>(pDescriptor), sizeof(UDF_PRIMARY_VOLUME_DESCRIPTOR));
}

bool XUDF::_readUInt8(qint64 nOffset, quint8 *pValue)
{
    if (!pValue) {
        return false;
    }

    quint8 nValue = 0;
    if (!_readExact(nOffset, reinterpret_cast<char *>(&nValue), sizeof(nValue))) {
        return false;
    }

    *pValue = nValue;
    return true;
}

bool XUDF::_readUInt16(qint64 nOffset, quint16 *pValue)
{
    if (!pValue) {
        return false;
    }

    quint16 nValue = 0;
    if (!_readExact(nOffset, reinterpret_cast<char *>(&nValue), sizeof(nValue))) {
        return false;
    }

    *pValue = qFromLittleEndian(nValue);
    return true;
}

bool XUDF::_readUInt32(qint64 nOffset, quint32 *pValue)
{
    if (!pValue) {
        return false;
    }

    quint32 nValue = 0;
    if (!_readExact(nOffset, reinterpret_cast<char *>(&nValue), sizeof(nValue))) {
        return false;
    }

    *pValue = qFromLittleEndian(nValue);
    return true;
}

bool XUDF::_readUInt64(qint64 nOffset, quint64 *pValue)
{
    if (!pValue) {
        return false;
    }

    quint64 nValue = 0;
    if (!_readExact(nOffset, reinterpret_cast<char *>(&nValue), sizeof(nValue))) {
        return false;
    }

    *pValue = qFromLittleEndian(nValue);
    return true;
}

QString XUDF::getVolumeIdentifier()
{
    QString sResult;
    QPointer<XUDF> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());

    qint64 nAnchorOffset = _getAnchorVolumeDescriptorOffset();
    if (!guardedThis || !guardedDevice) {
        return sResult;
    }
    if (nAnchorOffset != -1) {
        UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER anchor = {};
        if (!_readAnchorVolumeDescriptor(nAnchorOffset, &anchor) || !guardedThis || !guardedDevice) {
            return sResult;
        }
        qint64 nVDSOffset = (qint64)anchor.mainVolumeDescriptorSequenceExtent.nLocation * 2048;

        if (nVDSOffset > 0 && nVDSOffset < getSize()) {
            UDF_TAG tag = {};
            if (!_readTag(nVDSOffset, &tag) || !guardedThis || !guardedDevice) {
                return sResult;
            }
            if (tag.nTagIdentifier == TAG_PRIMARY_VOLUME_DESCRIPTOR) {
                UDF_PRIMARY_VOLUME_DESCRIPTOR pvd = {};
                if (!_readPrimaryVolumeDescriptor(nVDSOffset, &pvd) || !guardedThis || !guardedDevice) {
                    return sResult;
                }
                sResult = QString::fromLatin1(pvd.szVolumeIdentifier, 32).trimmed();
            }
        }
    }

    return sResult;
}

QString XUDF::getVolumeSetIdentifier()
{
    QString sResult;
    QPointer<XUDF> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());

    qint64 nAnchorOffset = _getAnchorVolumeDescriptorOffset();
    if (!guardedThis || !guardedDevice) {
        return sResult;
    }
    if (nAnchorOffset != -1) {
        UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER anchor = {};
        if (!_readAnchorVolumeDescriptor(nAnchorOffset, &anchor) || !guardedThis || !guardedDevice) {
            return sResult;
        }
        qint64 nVDSOffset = (qint64)anchor.mainVolumeDescriptorSequenceExtent.nLocation * 2048;

        if (nVDSOffset > 0 && nVDSOffset < getSize()) {
            UDF_TAG tag = {};
            if (!_readTag(nVDSOffset, &tag) || !guardedThis || !guardedDevice) {
                return sResult;
            }
            if (tag.nTagIdentifier == TAG_PRIMARY_VOLUME_DESCRIPTOR) {
                UDF_PRIMARY_VOLUME_DESCRIPTOR pvd = {};
                if (!_readPrimaryVolumeDescriptor(nVDSOffset, &pvd) || !guardedThis || !guardedDevice) {
                    return sResult;
                }
                sResult = QString::fromLatin1(pvd.szVolumeSetIdentifier, 128).trimmed();
            }
        }
    }

    return sResult;
}

qint32 XUDF::_getBlockSize()
{
    // UDF typically uses 2048 byte blocks
    return 2048;
}

quint8 XUDF::_calculateTagChecksum(const quint8 *pTagBytes)
{
    // ECMA-167 4/7.2.3 "TagChecksum": the sum, modulo 256, of bytes 0-3 and
    // 5-15 of the 16-byte descriptor tag.  The result is stored in byte 4.
    quint32 nSum = 0;

    for (qint32 i = 0; i < 16; i++) {
        if (i != 4) {
            nSum += pTagBytes[i];
        }
    }

    return (quint8)(nSum & 0xFF);
}

quint16 XUDF::_calculateDescriptorCRC(const quint8 *pData, qint64 nSize)
{
    // ECMA-167 Annex A: CRC-ITU-T over the bytes that follow the descriptor
    // tag, generator polynomial x^16 + x^12 + x^5 + 1 (0x1021), initial value
    // 0, neither the input nor the output is reflected and there is no final
    // XOR.
    quint16 nCRC = 0;

    for (qint64 i = 0; i < nSize; i++) {
        nCRC = (quint16)(nCRC ^ ((quint16)pData[i] << 8));

        for (qint32 j = 0; j < 8; j++) {
            if (nCRC & 0x8000) {
                nCRC = (quint16)((quint16)(nCRC << 1) ^ 0x1021);
            } else {
                nCRC = (quint16)(nCRC << 1);
            }
        }
    }

    return nCRC;
}

bool XUDF::_isValidDescriptorTag(qint64 nOffset, quint16 nExpectedTagIdentifier, bool bStrict, UDF_TAG *pTag)
{
    if (pTag) {
        *pTag = {};
    }

    const qint32 nBlockSize = _getBlockSize();
    const qint64 nTagSize = (qint64)sizeof(UDF_TAG);
    const qint64 nDeviceSize = getSize();

    if ((nOffset < 0) || (nDeviceSize < nTagSize) || (nOffset > nDeviceSize - nTagSize)) {
        return false;
    }

    quint8 tagBytes[16] = {};
    if (!_readExact(nOffset, reinterpret_cast<char *>(tagBytes), (qint64)sizeof(tagBytes))) {
        return false;
    }

    UDF_TAG tag = {};
    tag.nTagIdentifier = (quint16)(tagBytes[0] | ((quint16)tagBytes[1] << 8));
    tag.nDescriptorVersion = (quint16)(tagBytes[2] | ((quint16)tagBytes[3] << 8));
    tag.nChecksum = tagBytes[4];
    tag.nReserved = tagBytes[5];
    tag.nTagSerialNumber = (quint16)(tagBytes[6] | ((quint16)tagBytes[7] << 8));
    tag.nDescriptorCRC = (quint16)(tagBytes[8] | ((quint16)tagBytes[9] << 8));
    tag.nDescriptorCRCLength = (quint16)(tagBytes[10] | ((quint16)tagBytes[11] << 8));
    tag.nTagLocation = (quint32)tagBytes[12] | ((quint32)tagBytes[13] << 8) | ((quint32)tagBytes[14] << 16) | ((quint32)tagBytes[15] << 24);

    if (tag.nTagIdentifier != nExpectedTagIdentifier) {
        return false;
    }

    // ECMA-167 4/7.2.2: the descriptor version is 2 (ECMA-167 2nd edition,
    // UDF up to 2.01) or 3 (3rd edition, UDF 2.50/2.60).
    if ((tag.nDescriptorVersion != 2) && (tag.nDescriptorVersion != 3)) {
        return false;
    }

    // ECMA-167 4/7.2.4: byte 5 is reserved and shall be #00.
    if (tag.nReserved != 0) {
        return false;
    }

    if (_calculateTagChecksum(tagBytes) != tag.nChecksum) {
        return false;
    }

    if (bStrict) {
        // ECMA-167 4/7.2.8: TagLocation is the number of the logical sector
        // the descriptor is recorded in.  XUDF addresses everything from the
        // start of the file, so an anchor that does not sit in the sector it
        // claims cannot be parsed by this class anyway.
        if ((nBlockSize <= 0) || ((qint64)tag.nTagLocation != (nOffset / nBlockSize))) {
            return false;
        }

        if (tag.nDescriptorCRCLength > 0) {
            if ((qint64)tag.nDescriptorCRCLength > (qint64)nBlockSize - nTagSize) {
                return false;
            }
            if (nOffset + nTagSize + (qint64)tag.nDescriptorCRCLength > nDeviceSize) {
                return false;
            }

            QByteArray baBody((qint32)tag.nDescriptorCRCLength, Qt::Uninitialized);
            if (!_readExact(nOffset + nTagSize, baBody.data(), (qint64)baBody.size())) {
                return false;
            }
            if (_calculateDescriptorCRC(reinterpret_cast<const quint8 *>(baBody.constData()), (qint64)baBody.size()) != tag.nDescriptorCRC) {
                return false;
            }
        }
    }

    if (pTag) {
        *pTag = tag;
    }

    return true;
}

bool XUDF::_isAnchorVolumeDescriptorPointer(qint64 nOffset, bool bStrict)
{
    UDF_TAG tag = {};

    if (!_isValidDescriptorTag(nOffset, TAG_ANCHOR_VOLUME_DESCRIPTOR_POINTER, bStrict, &tag)) {
        return false;
    }

    // ECMA-167 3/10.2: the AVDP body is two extent_ad structures.  The main
    // Volume Descriptor Sequence extent has to be a non-empty, logical-sector
    // aligned extent that lies inside the volume - without it there is nothing
    // for this class to parse.
    const qint32 nBlockSize = _getBlockSize();
    const qint64 nDeviceSize = getSize();

    quint32 nMainLength = 0;
    if (!_readUInt32(nOffset + (qint64)sizeof(UDF_TAG), &nMainLength)) {
        return false;
    }

    quint32 nMainLocation = 0;
    if (!_readUInt32(nOffset + (qint64)sizeof(UDF_TAG) + 4, &nMainLocation)) {
        return false;
    }

    if ((nBlockSize <= 0) || (nMainLength == 0) || ((nMainLength % (quint32)nBlockSize) != 0)) {
        return false;
    }

    const qint64 nMainOffset = (qint64)nMainLocation * nBlockSize;

    if ((nMainOffset <= 0) || (nMainOffset >= nDeviceSize)) {
        return false;
    }

    return true;
}

bool XUDF::_hasVolumeRecognitionSequence()
{
    // ECMA-167 2/9.1: the Volume Recognition Sequence starts at byte 32768 and
    // is a series of 2048-byte Volume Structure Descriptors, each of them
    //   byte 0     structure type
    //   bytes 1-5  standard identifier
    //   byte 6     structure version
    // A UDF volume announces itself with an "NSR02"/"NSR03" descriptor in the
    // extended area, that is after "BEA01" and before "TEA01".
    const qint64 nVRSOffset = 0x8000;
    const qint32 nSectorSize = 2048;
    const qint64 nDeviceSize = getSize();

    bool bExtendedArea = false;

    for (qint32 i = 0; i < 64; i++) {
        const qint64 nOffset = nVRSOffset + (qint64)i * nSectorSize;

        if ((nOffset < 0) || (nOffset + nSectorSize > nDeviceSize)) {
            break;
        }

        QByteArray baIdentifier(5, Qt::Uninitialized);
        if (!_readExact(nOffset + 1, baIdentifier.data(), (qint64)baIdentifier.size())) {
            break;
        }

        if (baIdentifier == "BEA01") {
            bExtendedArea = true;
        } else if (baIdentifier == "TEA01") {
            break;
        } else if (bExtendedArea && ((baIdentifier == "NSR02") || (baIdentifier == "NSR03"))) {
            return true;
        } else if ((baIdentifier != "CD001") && (baIdentifier != "CDW02") && (baIdentifier != "BOOT2")) {
            break;
        }
    }

    return false;
}

qint64 XUDF::_getAnchorVolumeDescriptorOffset()
{
    QPointer<XUDF> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());

    const qint32 nBlockSize = _getBlockSize();
    const qint64 nDeviceSize = getSize();

    if ((nBlockSize <= 0) || (nDeviceSize < nBlockSize)) {
        return -1;
    }

    // ECMA-167 3/8.4.2: an Anchor Volume Descriptor Pointer shall be recorded
    // at logical sector 256, at N (the last sector of the volume space) and/or
    // at N-256.  UDF 2.60 2.2.3 additionally allows sector 512.  Sector 256 and
    // the last sector are probed first so that the anchor picked here stays the
    // one the rest of the class used before.
    const qint64 nLastSector = (nDeviceSize / nBlockSize) - 1;

    qint64 pnCandidates[4] = {};
    qint32 nNumberOfCandidates = 0;

    pnCandidates[nNumberOfCandidates++] = (qint64)256 * nBlockSize;
    if (nLastSector >= 0) {
        pnCandidates[nNumberOfCandidates++] = nLastSector * nBlockSize;
    }
    if (nLastSector >= 256) {
        pnCandidates[nNumberOfCandidates++] = (nLastSector - 256) * nBlockSize;
    }
    pnCandidates[nNumberOfCandidates++] = (qint64)512 * nBlockSize;

    for (qint32 i = 0; i < nNumberOfCandidates; i++) {
        const bool bIsAnchor = _isAnchorVolumeDescriptorPointer(pnCandidates[i], true);
        if (!guardedThis || !guardedDevice) {
            return -1;
        }
        if (bIsAnchor) {
            return pnCandidates[i];
        }
    }

    // Fall back to a tag that passes the checksum but not the self-consistency
    // checks only when the Volume Recognition Sequence independently declares
    // the file to be a UDF volume.
    const bool bHasVRS = _hasVolumeRecognitionSequence();
    if (!guardedThis || !guardedDevice) {
        return -1;
    }

    if (bHasVRS) {
        for (qint32 i = 0; i < nNumberOfCandidates; i++) {
            const bool bIsAnchor = _isAnchorVolumeDescriptorPointer(pnCandidates[i], false);
            if (!guardedThis || !guardedDevice) {
                return -1;
            }
            if (bIsAnchor) {
                return pnCandidates[i];
            }
        }
    }

    return -1;
}

bool XUDF::_isValidTag(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    const qint64 nTagSize = static_cast<qint64>(sizeof(UDF_TAG));
    const qint64 nDeviceSize = getSize();

    if ((nOffset < 0) || (nDeviceSize < nTagSize) || (nOffset > nDeviceSize - nTagSize)) {
        return false;
    }

    UDF_TAG tag = {};
    if (!_readTag(nOffset, &tag)) {
        return false;
    }
    return (tag.nTagIdentifier > 0 && tag.nTagIdentifier < 300);
}

QList<XBinary::ARCHIVERECORD> XUDF::_parseFileSystem(qint32 nBlockSize, PDSTRUCT *pPdStruct)
{
    QList<ARCHIVERECORD> listResult;
    QPointer<XUDF> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());

    qint64 nAnchorOffset = _getAnchorVolumeDescriptorOffset();
    if (!guardedThis || !guardedDevice) {
        return listResult;
    }
    if (nAnchorOffset == -1) {
        return listResult;
    }

    UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER anchor = {};
    if (!_readAnchorVolumeDescriptor(nAnchorOffset, &anchor) || !guardedThis || !guardedDevice) {
        return listResult;
    }
    qint64 nVDSOffset = (qint64)anchor.mainVolumeDescriptorSequenceExtent.nLocation * nBlockSize;
    qint64 nVDSLength = (qint64)anchor.mainVolumeDescriptorSequenceExtent.nLength;

    if (nVDSOffset <= 0 || nVDSLength <= 0 || nVDSOffset >= getSize()) {
        return listResult;
    }

    // Scan Volume Descriptor Sequence for Logical Volume Descriptor (tag id 6)
    qint64 nFSDLocation = -1;
    qint64 nCurrentVDSOffset = nVDSOffset;
    qint64 nVDSEnd = nVDSOffset + nVDSLength;

    while (nCurrentVDSOffset + (qint64)sizeof(UDF_TAG) <= nVDSEnd && isPdStructNotCanceled(pPdStruct)) {
        UDF_TAG tag = {};
        if (!_readTag(nCurrentVDSOffset, &tag) || !guardedThis || !guardedDevice) {
            return listResult;
        }

        if (tag.nTagIdentifier == TAG_TERMINATING_DESCRIPTOR) {
            break;
        }

        if (tag.nTagIdentifier == TAG_LOGICAL_VOLUME_DESCRIPTOR) {
            // Logical Volume Descriptor: File Set Descriptor location is at offset
            // sizeof(UDF_TAG) + 440 bytes (integrity sequence extent) within the
            // LogicalVolumeContentsUse field.
            // Layout: tag(16) + VolumeDescriptorSequenceNumber(4) + DescriptorCharacterSet(64)
            // + LogicalVolumeIdentifier(128, dstring) + LogicalBlockSize(4)
            // + DomainIdentifier(32) + LogicalVolumeContentsUse(16) <-- FSD extent here
            // + MapTableLength(4) + NumberOfPartitionMaps(4)
            // LogicalVolumeContentsUse starts at offset 16+4+64+128+4+32 = 248
            quint32 nFSDLoc = 0;
            if (!_readUInt32(nCurrentVDSOffset + 248, &nFSDLoc) || !guardedThis || !guardedDevice) {
                return listResult;
            }
            // quint32 nFSDLen = read_uint32(nCurrentVDSOffset + 252);  // extent length
            if (nFSDLoc > 0) {
                nFSDLocation = (qint64)nFSDLoc * nBlockSize;
            }
        }

        nCurrentVDSOffset += nBlockSize;
    }

    if (nFSDLocation <= 0 || nFSDLocation >= getSize()) {
        return listResult;
    }

    // Read File Set Descriptor (tag id 256)
    UDF_TAG fsdTag = {};
    if (!_readTag(nFSDLocation, &fsdTag) || !guardedThis || !guardedDevice) {
        return listResult;
    }
    if (fsdTag.nTagIdentifier != TAG_FILE_SET_DESCRIPTOR) {
        return listResult;
    }

    // File Set Descriptor layout:
    // tag(16) + RecordingDateAndTime(12) + InterchangeLevel(2) + MaxInterchangeLevel(2)
    // + CharacterSetList(4) + MaxCharacterSetList(4) + FileSetNumber(4) + FileSetDescriptorNumber(4)
    // + LogicalVolumeIdentifierCharacterSet(64) + LogicalVolumeIdentifier(128)
    // + FileSetCharacterSet(64) + FileSetIdentifier(32) + ReferencingTimeStamp(12)
    // + (ICB exts: 16 bytes each) ...
    // Root Directory ICB is at offset 16+12+2+2+4+4+4+4+64+128+64+32+12 = 352
    // It is a long_ad: ExtentLength(4) + ExtentLocation: LogicalBlockNumber(4) + PartitionReferenceNumber(2) + ImplementationUse(6) = 16 bytes total
    quint32 nRootICBLocation = 0;
    if (!_readUInt32(nFSDLocation + 352 + 4, &nRootICBLocation) || !guardedThis || !guardedDevice) {
        return listResult;
    }
    quint16 nRootPartRef = 0;
    if (!_readUInt16(nFSDLocation + 352 + 4 + 4, &nRootPartRef) || !guardedThis || !guardedDevice) {
        return listResult;
    }
    Q_UNUSED(nRootPartRef)

    if (nRootICBLocation == 0) {
        return listResult;
    }

    qint64 nRootFileEntryOffset = (qint64)nRootICBLocation * nBlockSize;
    if (nRootFileEntryOffset >= getSize()) {
        return listResult;
    }

    // BFS traversal of directory tree
    struct DirEntry {
        qint64 nFileEntryOffset;
        QString sPath;
    };

    QList<DirEntry> listQueue;
    QSet<qint64> setVisited;

    DirEntry rootEntry;
    rootEntry.nFileEntryOffset = nRootFileEntryOffset;
    rootEntry.sPath = QString();
    listQueue.append(rootEntry);
    setVisited.insert(nRootFileEntryOffset);

    while (!listQueue.isEmpty() && isPdStructNotCanceled(pPdStruct)) {
        DirEntry dirInfo = listQueue.takeFirst();

        UDF_TAG feTag = {};
        if (!_readTag(dirInfo.nFileEntryOffset, &feTag) || !guardedThis || !guardedDevice) {
            return listResult;
        }
        if (feTag.nTagIdentifier != TAG_FILE_ENTRY && feTag.nTagIdentifier != TAG_EXTENDED_FILE_ENTRY) {
            continue;
        }

        // File Entry layout:
        // tag(16) + ICBTag(20) + UID(4) + GID(4) + Permissions(4) + FileLinkCount(2)
        // + RecordFormat(1) + RecordDisplayAttributes(1) + RecordLength(4)
        // + InformationLength(8) + LogicalBlocksRecorded(8)
        // + AccessTime(12) + ModificationTime(12) + AttributeTime(12) + Checkpoint(4)
        // + ExtendedAttributeICB(16) + ImplementationIdentifier(32) + UniqueID(8)
        // + LengthOfExtendedAttributes(4) + LengthOfAllocationDescriptors(4)
        // = 16+20+4+4+4+2+1+1+4+8+8+12+12+12+4+16+32+8+4+4 = 176
        quint32 nLenExtAttrs = 0;
        if (!_readUInt32(dirInfo.nFileEntryOffset + 168, &nLenExtAttrs) || !guardedThis || !guardedDevice) {
            return listResult;
        }
        quint32 nLenAllocDescs = 0;
        if (!_readUInt32(dirInfo.nFileEntryOffset + 172, &nLenAllocDescs) || !guardedThis || !guardedDevice) {
            return listResult;
        }
        // ICBTag file type is at offset 16+12 = 28 (within ICBTag at offset 16)
        quint8 nICBFileType = 0;
        if (!_readUInt8(dirInfo.nFileEntryOffset + 28, &nICBFileType) || !guardedThis || !guardedDevice) {
            return listResult;
        }
        // ICBTag flags (allocation type) at offset 16+18 = 34
        quint16 nICBFlags = 0;
        if (!_readUInt16(dirInfo.nFileEntryOffset + 34, &nICBFlags) || !guardedThis || !guardedDevice) {
            return listResult;
        }
        quint8 nAllocType = (quint8)(nICBFlags & 0x07);

        bool bIsDirectory = (nICBFileType == 4);

        qint64 nAllocDescsOffset = dirInfo.nFileEntryOffset + 176 + (qint64)nLenExtAttrs;

        if (!bIsDirectory) {
            // Regular file - read allocation descriptors to get data location
            quint64 nInfoLength = 0;
            if (!_readUInt64(dirInfo.nFileEntryOffset + 56, &nInfoLength) || !guardedThis || !guardedDevice) {
                return listResult;
            }

            ARCHIVERECORD record = {};
            record.mapProperties[FPART_PROP_ORIGINALNAME] = dirInfo.sPath;
            record.mapProperties[FPART_PROP_UNCOMPRESSEDSIZE] = (qint64)nInfoLength;
            record.mapProperties[FPART_PROP_COMPRESSEDSIZE] = (qint64)nInfoLength;
            record.mapProperties[FPART_PROP_HANDLEMETHOD] = HANDLE_METHOD_STORE;
            record.mapProperties[FPART_PROP_ISFOLDER] = false;

            if (nAllocType == 0 && nLenAllocDescs >= 8) {
                // Short allocation descriptor: ExtentLength(4) + ExtentPosition(4)
                quint32 nExtLength = 0;
                if (!_readUInt32(nAllocDescsOffset, &nExtLength) || !guardedThis || !guardedDevice) {
                    return listResult;
                }
                nExtLength &= 0x3FFFFFFF;
                quint32 nExtPos = 0;
                if (!_readUInt32(nAllocDescsOffset + 4, &nExtPos) || !guardedThis || !guardedDevice) {
                    return listResult;
                }
                record.nStreamOffset = (qint64)nExtPos * nBlockSize;
                record.nStreamSize = (qint64)nExtLength;
            } else if (nAllocType == 1 && nLenAllocDescs >= 16) {
                // Long allocation descriptor: ExtentLength(4) + ExtentLocation: LogicalBlockNum(4) + PartRef(2) + ImplUse(6)
                quint32 nExtLength = 0;
                if (!_readUInt32(nAllocDescsOffset, &nExtLength) || !guardedThis || !guardedDevice) {
                    return listResult;
                }
                nExtLength &= 0x3FFFFFFF;
                quint32 nExtPos = 0;
                if (!_readUInt32(nAllocDescsOffset + 4, &nExtPos) || !guardedThis || !guardedDevice) {
                    return listResult;
                }
                record.nStreamOffset = (qint64)nExtPos * nBlockSize;
                record.nStreamSize = (qint64)nExtLength;
            } else if (nAllocType == 3 && nLenAllocDescs > 0) {
                // Data stored directly in allocation descriptors (inline)
                record.nStreamOffset = nAllocDescsOffset;
                record.nStreamSize = (qint64)nLenAllocDescs;
            }

            listResult.append(record);
        } else {
            // Directory - add folder record and enqueue children
            if (!dirInfo.sPath.isEmpty()) {
                ARCHIVERECORD record = {};
                record.mapProperties[FPART_PROP_ORIGINALNAME] = dirInfo.sPath;
                record.mapProperties[FPART_PROP_UNCOMPRESSEDSIZE] = (qint64)0;
                record.mapProperties[FPART_PROP_COMPRESSEDSIZE] = (qint64)0;
                record.mapProperties[FPART_PROP_HANDLEMETHOD] = HANDLE_METHOD_STORE;
                record.mapProperties[FPART_PROP_ISFOLDER] = true;
                listResult.append(record);
            }

            // Read directory data to find File Identifier Descriptors
            qint64 nDirDataOffset = -1;
            qint64 nDirDataSize = 0;

            quint64 nInfoLength = 0;
            if (!_readUInt64(dirInfo.nFileEntryOffset + 56, &nInfoLength) || !guardedThis || !guardedDevice) {
                return listResult;
            }

            if (nAllocType == 0 && nLenAllocDescs >= 8) {
                quint32 nExtPos = 0;
                if (!_readUInt32(nAllocDescsOffset + 4, &nExtPos) || !guardedThis || !guardedDevice) {
                    return listResult;
                }
                nDirDataOffset = (qint64)nExtPos * nBlockSize;
                nDirDataSize = (qint64)nInfoLength;
            } else if (nAllocType == 1 && nLenAllocDescs >= 16) {
                quint32 nExtPos = 0;
                if (!_readUInt32(nAllocDescsOffset + 4, &nExtPos) || !guardedThis || !guardedDevice) {
                    return listResult;
                }
                nDirDataOffset = (qint64)nExtPos * nBlockSize;
                nDirDataSize = (qint64)nInfoLength;
            }

            if (nDirDataOffset <= 0 || nDirDataSize <= 0 || nDirDataOffset >= getSize()) {
                continue;
            }

            // Parse File Identifier Descriptors (tag id 257)
            qint64 nFIDOffset = nDirDataOffset;
            qint64 nFIDEnd = nDirDataOffset + nDirDataSize;

            while (nFIDOffset < nFIDEnd && isPdStructNotCanceled(pPdStruct)) {
                if (nFIDOffset + (qint64)sizeof(UDF_TAG) > getSize()) {
                    break;
                }

                UDF_TAG fidTag = {};
                if (!_readTag(nFIDOffset, &fidTag) || !guardedThis || !guardedDevice) {
                    return listResult;
                }

                if (fidTag.nTagIdentifier != TAG_FILE_IDENTIFIER_DESCRIPTOR) {
                    break;
                }

                // File Identifier Descriptor layout:
                // tag(16) + FileVersionNumber(2) + FileCharacteristics(1) + LengthOfFileIdentifier(1)
                // + ICB(16) + LengthOfImplementationUse(2) [+ ImplementationUse(var)] [+ FileIdentifier(var)] [+ padding]
                quint8 nFileCharacteristics = 0;
                if (!_readUInt8(nFIDOffset + 18, &nFileCharacteristics) || !guardedThis || !guardedDevice) {
                    return listResult;
                }
                quint8 nLenFileId = 0;
                if (!_readUInt8(nFIDOffset + 19, &nLenFileId) || !guardedThis || !guardedDevice) {
                    return listResult;
                }
                quint16 nLenImplUse = 0;
                if (!_readUInt16(nFIDOffset + 36, &nLenImplUse) || !guardedThis || !guardedDevice) {
                    return listResult;
                }

                // ICB (long_ad) of child: ExtentLength(4) + ExtentLocation: LogicalBlockNum(4) + PartRef(2) + ImplUse(6)
                quint32 nChildICBLocation = 0;
                if (!_readUInt32(nFIDOffset + 20 + 4, &nChildICBLocation) || !guardedThis || !guardedDevice) {
                    return listResult;
                }

                bool bIsParent = (nFileCharacteristics & 0x08) != 0;  // Parent directory
                bool bChildIsDir = (nFileCharacteristics & 0x02) != 0;
                Q_UNUSED(bChildIsDir)

                // Compute name
                QString sChildName;
                if (!bIsParent && nLenFileId > 0) {
                    qint64 nNameOffset = nFIDOffset + 38 + (qint64)nLenImplUse;
                    if (nNameOffset + nLenFileId <= getSize()) {
                        QByteArray baName(nLenFileId, Qt::Uninitialized);
                        if (!_readExact(nNameOffset, baName.data(), nLenFileId) || !guardedThis || !guardedDevice) {
                            return listResult;
                        }
                        // OSTA CS0 encoded: if first byte is 8, rest is ASCII; if 16, UTF-16BE
                        if (baName.size() > 0) {
                            quint8 nEncType = (quint8)baName.at(0);
                            if (nEncType == 16 && baName.size() >= 3) {
                                for (qint32 j = 1; (j + 1) < baName.size(); j += 2) {
                                    ushort nChar = ((ushort)(quint8)baName.at(j) << 8) | (ushort)(quint8)baName.at(j + 1);
                                    sChildName.append(QChar(nChar));
                                }
                            } else if (nEncType == 8 && baName.size() >= 2) {
                                sChildName = QString::fromLatin1(baName.constData() + 1, baName.size() - 1);
                            }
                        }
                    }
                }

                // Total FID size (must be 4-byte aligned)
                qint32 nFIDSize = 38 + (qint32)nLenImplUse + (qint32)nLenFileId;
                qint32 nFIDPadded = (nFIDSize + 3) & ~3;

                if (nFIDPadded <= 0) {
                    break;
                }

                if (!bIsParent && !sChildName.isEmpty() && nChildICBLocation > 0) {
                    QString sChildPath;
                    if (dirInfo.sPath.isEmpty()) {
                        sChildPath = sChildName;
                    } else {
                        sChildPath = dirInfo.sPath + "/" + sChildName;
                    }

                    qint64 nChildFileEntryOffset = (qint64)nChildICBLocation * nBlockSize;

                    if (nChildFileEntryOffset > 0 && nChildFileEntryOffset < getSize() && !setVisited.contains(nChildFileEntryOffset)) {
                        setVisited.insert(nChildFileEntryOffset);

                        DirEntry childEntry;
                        childEntry.nFileEntryOffset = nChildFileEntryOffset;
                        childEntry.sPath = sChildPath;
                        listQueue.append(childEntry);
                    }
                }

                nFIDOffset += nFIDPadded;
            }
        }
    }

    return listResult;
}

QString XUDF::_cleanFileName(const QString &sFileName)
{
    QString sResult = sFileName;

    // Remove null terminators and trim whitespace
    qint32 nIndex = sResult.indexOf(QChar('\0'));
    if (nIndex != -1) {
        sResult = sResult.left(nIndex);
    }

    sResult = sResult.trimmed();

    return sResult;
}

QList<QString> XUDF::getSearchSignatures()
{
    QList<QString> listResult;

    return listResult;
}

XBinary *XUDF::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XUDF(pDevice);
}

bool XUDF::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;
    QPointer<XUDF> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());

    if (!isInternalInfoHandled()) {
        bResult = XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !guardedDevice || !bResult) {
            return false;
        }

        XArchive::INTERNAL_INFO *pInfo = static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !guardedDevice || !pInfo) {
            return false;
        }

        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) = *pInfo;
    }

    return bResult;
}

void *XUDF::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XUDF> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());
    if (!handleInternalInfo(pPdStruct) || !guardedThis || !guardedDevice) {
        return nullptr;
    }

    return &m_internalInfo;
}

void XUDF::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
