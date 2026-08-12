/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xlzo.h"
#include "Algos/xlzodecoder.h"

#include <new>

namespace {
class LzoDiscardDevice : public QIODevice {
protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *, qint64 nSize) override { return (nSize >= 0) ? nSize : -1; }
};

bool measureLzoStream(QIODevice *pDevice, qint64 nFileSize, qint64 *pnCompressedSize, qint64 *pnUncompressedSize,
                      XBinary::PDSTRUCT *pPdStruct)
{
    if (pnCompressedSize) *pnCompressedSize = 0;
    if (pnUncompressedSize) *pnUncompressedSize = 0;
    if (!pDevice || (nFileSize <= 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    SubDevice input(pDevice, 0, nFileSize);
    LzoDiscardDevice output;
    if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly)) {
        if (input.isOpen()) input.close();
        if (output.isOpen()) output.close();
        return false;
    }

    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = &input;
    state.pDeviceOutput = &output;
    state.nInputOffset = 0;
    state.nInputLimit = nFileSize;
    state.nProcessedLimit = -1;

    const bool bResult = XLZODecoder::decompress(&state, pPdStruct) &&
                         (state.nCountInput >= 0) && (state.nCountInput <= nFileSize) &&
                         (state.nCountOutput >= 0) && XBinary::isPdStructNotCanceled(pPdStruct);
    if (bResult) {
        if (pnCompressedSize) *pnCompressedSize = state.nCountInput;
        if (pnUncompressedSize) *pnUncompressedSize = state.nCountOutput;
    }

    output.close();
    input.close();
    return bResult;
}
}  // namespace

XBinary::XCONVERT _TABLE_XLzo_STRUCTID[] = {{XLzo::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                            {XLzo::STRUCTID_LZO_HEADER, "LZO_HEADER", QString("LZOP header")}};

XLzo::XLzo(QIODevice *pDevice) : XArchive(pDevice)
{
}

XLzo::~XLzo()
{
}

bool XLzo::isValid(PDSTRUCT *pPdStruct)
{
    return isValid(getDevice(), pPdStruct);
}

bool XLzo::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pDevice && XBinary::isPdStructNotCanceled(pPdStruct) && pDevice->seek(0)) {
        quint8 magic[9];
        if (pDevice->read((char *)magic, 9) == 9) {
            const quint8 nExpected[] = {0x89, 0x4C, 0x5A, 0x4F, 0x00, 0x0D, 0x0A, 0x1A, 0x0A};
            bResult = (memcmp(magic, nExpected, 9) == 0);
        }
    }

    return bResult;
}

XBinary::MODE XLzo::getMode()
{
    return MODE_DATA;
}

qint32 XLzo::getType()
{
    return TYPE_LZO;
}

XBinary::ENDIAN XLzo::getEndian()
{
    return ENDIAN_BIG;
}

QString XLzo::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_LZO: sResult = QString("LZO"); break;
    }

    return sResult;
}

QString XLzo::getFileFormatExt()
{
    return "lzo";
}

XBinary::FT XLzo::getFileType()
{
    return FT_LZO;
}

QString XLzo::getFileFormatExtsString()
{
    return "lzo";
}

qint64 XLzo::getFileFormatSize(XBinary::PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QString XLzo::getMIMEString()
{
    return "application/x-lzop";
}

XBinary::OSNAME XLzo::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QList<XBinary::MAPMODE> XLzo::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XLzo::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XLzo::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XLzo_STRUCTID, sizeof(_TABLE_XLzo_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XLzo::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XLzo_STRUCTID, sizeof(_TABLE_XLzo_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XLzo::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XLzo_STRUCTID, sizeof(_TABLE_XLzo_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XLzo::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_LZO_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_LZO_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_LZO_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_LZO_HEADER, headerLoc);

        if (!xfHeader.listFields.isEmpty()) {
            const XFRECORD &lastField = xfHeader.listFields.last();
            xfHeader.nSize = lastField.nOffset + lastField.nSize;
        }

        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_LZO_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XLzo::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_LZO_HEADER) {
        // lzop file header (big-endian)
        listResult.append({"Magic", 0, 9, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});

        quint16 nVersion = read_uint16(xLoc.nLocation + 9, true);

        listResult.append({"Version", 9, 2, XFRECORD_FLAG_BE | XFRECORD_FLAG_VERSION, VT_UINT16});
        listResult.append({"LibVersion", 11, 2, XFRECORD_FLAG_BE | XFRECORD_FLAG_VERSION, VT_UINT16});

        if (nVersion >= 0x0940) {
            listResult.append({"VersionNeeded", 13, 2, XFRECORD_FLAG_BE | XFRECORD_FLAG_VERSION, VT_UINT16});
            listResult.append({"Method", 15, 1, XFRECORD_FLAG_NONE, VT_UINT8});
            listResult.append({"Level", 16, 1, XFRECORD_FLAG_NONE, VT_UINT8});

            quint32 nFlags = read_uint32(xLoc.nLocation + 17, true);
            listResult.append({"Flags", 17, 4, XFRECORD_FLAG_BE, VT_UINT32});

            qint32 nCurrentOffset = 21;

            if (nFlags & 0x00000800) {  // F_H_FILTER
                listResult.append({"Filter", nCurrentOffset, 4, XFRECORD_FLAG_BE, VT_UINT32});
                nCurrentOffset += 4;
            }

            listResult.append({"Mode", nCurrentOffset, 4, XFRECORD_FLAG_BE, VT_UINT32});
            nCurrentOffset += 4;
            listResult.append({"MTimeLow", nCurrentOffset, 4, XFRECORD_FLAG_BE | XFRECORD_FLAG_UNIXTIME, VT_UINT32});
            nCurrentOffset += 4;
            listResult.append({"MTimeHigh", nCurrentOffset, 4, XFRECORD_FLAG_BE, VT_UINT32});
            nCurrentOffset += 4;

            quint8 nNameLength = read_uint8(xLoc.nLocation + nCurrentOffset);
            listResult.append({"NameLength", nCurrentOffset, 1, XFRECORD_FLAG_SIZE, VT_UINT8});
            nCurrentOffset += 1;

            if (nNameLength > 0) {
                listResult.append({"Name", nCurrentOffset, (qint32)nNameLength, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
            }
        } else {
            // Old layout: no VersionNeeded/Level/MTimeHigh
            listResult.append({"Method", 13, 1, XFRECORD_FLAG_NONE, VT_UINT8});
            listResult.append({"Flags", 14, 4, XFRECORD_FLAG_BE, VT_UINT32});
            listResult.append({"Mode", 18, 4, XFRECORD_FLAG_BE, VT_UINT32});
            listResult.append({"MTimeLow", 22, 4, XFRECORD_FLAG_BE | XFRECORD_FLAG_UNIXTIME, VT_UINT32});

            quint8 nNameLength = read_uint8(xLoc.nLocation + 26);
            listResult.append({"NameLength", 26, 1, XFRECORD_FLAG_SIZE, VT_UINT8});

            if (nNameLength > 0) {
                listResult.append({"Name", 27, (qint32)nNameLength, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
            }
        }
    }

    return listResult;
}

// QList<XBinary::DATA_HEADER> XLzo::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
//         _dataHeadersOptions.nID = STRUCTID_LZO_HEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         if (isPdStructNotCanceled(pPdStruct)) {
//             listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//         }
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             if (dataHeadersOptions.nID == STRUCTID_LZO_HEADER) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XLzo::structIDToString(dataHeadersOptions.nID));
//                 dataHeader.nSize = 9;

//                 dataHeader.listRecords.append(getDataRecord(0, 9, "magic", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);
//             }
//         }
//     }

//     return listResult;
// }

QList<XBinary::FPART> XLzo::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    const auto canAppend = [&]() -> bool { return (nLimit == -1) || (listResult.size() < nLimit); };

    const qint64 nFileSize = getSize();
    if (nFileSize <= 0) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppend()) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = qMin<qint64>(9, nFileSize);
        header.nVirtualAddress = -1;
        header.sName = tr("Header");
        listResult.append(header);
        if (!canAppend()) return listResult;
    }

    if (!(nFileParts & (FILEPART_STREAM | FILEPART_DATA | FILEPART_OVERLAY))) return listResult;

    qint64 nCompressedSize = 0;
    qint64 nUncompressedSize = 0;
    if (!measureLzoStream(getDevice(), nFileSize, &nCompressedSize, &nUncompressedSize, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_STREAM) && canAppend()) {
        FPART region = {};
        region.filePart = FILEPART_STREAM;
        region.nFileOffset = 0;
        region.nFileSize = nCompressedSize;
        region.nVirtualAddress = -1;
        region.sName = tr("Stream");
        region.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZOP);
        region.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
        region.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nCompressedSize);
        listResult.append(region);
    }

    if ((nFileParts & FILEPART_DATA) && canAppend()) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = 0;
        data.nFileSize = nCompressedSize;
        data.nVirtualAddress = -1;
        data.sName = tr("Data");
        listResult.append(data);
    }

    if ((nFileParts & FILEPART_OVERLAY) && canAppend()) {
        if (nCompressedSize < nFileSize) {
            FPART ov = {};
            ov.filePart = FILEPART_OVERLAY;
            ov.nFileOffset = nCompressedSize;
            ov.nFileSize = nFileSize - nCompressedSize;
            ov.nVirtualAddress = -1;
            ov.sName = tr("Overlay");
            listResult.append(ov);
        }
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XLzo::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XLzo::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (!pState) return false;
    finishUnpack(pState, nullptr);
    if (!XBinary::isPdStructNotCanceled(pPdStruct) || !isValid(pPdStruct)) return false;

    const qint64 nFileSize = getSize();
    qint64 nCompressedSize = 0;
    qint64 nUncompressedSize = 0;
    if (!measureLzoStream(getDevice(), nFileSize, &nCompressedSize, &nUncompressedSize, pPdStruct)) return false;

    LZO_UNPACK_CONTEXT *pContext = new (std::nothrow) LZO_UNPACK_CONTEXT;
    if (!pContext) return false;
    pContext->nCompressedSize = nCompressedSize;
    pContext->nUncompressedSize = nUncompressedSize;
    pContext->sFileName = XBinary::getDeviceFileBaseName(getDevice());

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = nFileSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;
    return true;
}

XBinary::ARCHIVERECORD XLzo::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return result;
    }

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    LZO_UNPACK_CONTEXT *pContext = (LZO_UNPACK_CONTEXT *)pState->pContext;

    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nCompressedSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZOP);

    return result;
}

bool XLzo::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext || !pDevice || !pDevice->isWritable() ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;

    if (pDevice->isSequential()) {
        if (pDevice->pos() != 0) return false;
    } else if (!pDevice->seek(0) || ((pDevice->size() != 0) && !XBinary::resize(pDevice, 0))) {
        return false;
    }

    LZO_UNPACK_CONTEXT *pContext = static_cast<LZO_UNPACK_CONTEXT *>(pState->pContext);
    SubDevice input(getDevice(), 0, pContext->nCompressedSize);
    bool bResult = false;

    if (input.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
        state.pDeviceInput = &input;
        state.pDeviceOutput = pDevice;
        state.nInputOffset = 0;
        state.nInputLimit = pContext->nCompressedSize;
        state.nProcessedLimit = -1;
        bResult = XLZODecoder::decompress(&state, pPdStruct) &&
                  (state.nCountInput == pContext->nCompressedSize) &&
                  (state.nCountOutput == pContext->nUncompressedSize) && XBinary::isPdStructNotCanceled(pPdStruct);
        input.close();
    }

    if (!bResult && !pDevice->isSequential()) {
        XBinary::resize(pDevice, 0);
        pDevice->seek(0);
    }
    pState->nCurrentOffset = bResult ? pContext->nCompressedSize : 0;
    return bResult;
}

bool XLzo::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) ++pState->nCurrentIndex;
    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool XLzo::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if (pState->pContext) {
        LZO_UNPACK_CONTEXT *pContext = (LZO_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
    }

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    return true;
}

QList<XBinary::FPART_PROP> XLzo::getAvailableFPARTProperties()
{
    QList<XBinary::FPART_PROP> listResult;

    listResult.append(FPART_PROP_ORIGINALNAME);
    listResult.append(FPART_PROP_COMPRESSEDSIZE);
    listResult.append(FPART_PROP_UNCOMPRESSEDSIZE);
    listResult.append(FPART_PROP_HANDLEMETHOD);
    listResult.append(FPART_PROP_STREAMOFFSET);
    listResult.append(FPART_PROP_STREAMSIZE);

    return listResult;
}

QList<QString> XLzo::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("894C5A4F000D0A1A0A");

    return listResult;
}

XBinary *XLzo::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XLzo(pDevice);
}

bool XLzo::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XArchive::handleInternalInfo(pPdStruct);
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) =
            *static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
    }

    return bResult;
}

void *XLzo::getInternalInfo(PDSTRUCT *pPdStruct)
{
    handleInternalInfo(pPdStruct);

    return &m_internalInfo;
}

void XLzo::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
