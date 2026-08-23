/* Copyright (c) 2026 hors<horsicq@gmail.com>
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
#include "xlz5.h"
#include "Algos/xlz5decoder.h"

#include <limits>
#include <memory>
#include <new>

namespace {
const quint32 LZ5_STANDARD_MAGIC = 0x184D2205U;
const quint32 LZ5_SKIPPABLE_START = 0x184D2A50U;
const quint32 LZ5_SKIPPABLE_MASK = 0xFFFFFFF0U;

class Lz5DiscardDevice : public QIODevice {
protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *, qint64 nSize) override { return (nSize >= 0) ? nSize : -1; }
};

bool measureLz5Stream(QIODevice *pDevice, qint64 nFileSize, qint64 *pnCompressedSize, qint64 *pnUncompressedSize,
                      XBinary::PDSTRUCT *pPdStruct,
                      const QMap<XBinary::UNPACK_PROP, QVariant> *pUnpackProperties = nullptr)
{
    if (pnCompressedSize) *pnCompressedSize = 0;
    if (pnUncompressedSize) *pnUncompressedSize = 0;
    if (!pDevice || (nFileSize <= 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    SubDevice input(pDevice, 0, nFileSize);
    Lz5DiscardDevice output;
    if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly)) {
        if (input.isOpen()) input.close();
        if (output.isOpen()) output.close();
        return false;
    }

    XBinary::DATAPROCESS_STATE state = {};
    if (pUnpackProperties) state.mapUnpackProperties = *pUnpackProperties;
    state.pDeviceInput = &input;
    state.pDeviceOutput = &output;
    state.nInputOffset = 0;
    state.nInputLimit = nFileSize;
    state.nProcessedLimit = -1;

    const bool bResult = XLZ5Decoder::decompress(&state, pPdStruct) &&
                         (state.nCountInput == nFileSize) && (state.nCountOutput >= 0) &&
                         XBinary::isPdStructNotCanceled(pPdStruct);
    if (bResult) {
        if (pnCompressedSize) *pnCompressedSize = state.nCountInput;
        if (pnUncompressedSize) *pnUncompressedSize = state.nCountOutput;
    }

    output.close();
    input.close();
    return bResult;
}
}  // namespace

XBinary::XCONVERT _TABLE_XLZ5_STRUCTID[] = {{XLZ5::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                            {XLZ5::STRUCTID_LZ5_FRAME_HEADER, "LZ5_FRAME_HEADER", QString("LZ5 frame header")}};

XLZ5::XLZ5(QIODevice *pDevice) : XArchive(pDevice)
{
}

XLZ5::~XLZ5()
{
}

bool XLZ5::isValid(PDSTRUCT *pPdStruct)
{
    const qint64 nFileSize = getSize();
    qint64 nOffset = 0;

    while (XBinary::isPdStructNotCanceled(pPdStruct) && (nOffset >= 0) && (nOffset <= nFileSize - 4)) {
        const quint32 nMagic = read_uint32(nOffset, false);
        if (nMagic == LZ5_STANDARD_MAGIC) return true;
        if ((nMagic & LZ5_SKIPPABLE_MASK) != LZ5_SKIPPABLE_START) return false;
        if (nOffset > nFileSize - 8) return false;

        const quint32 nPayloadSize = read_uint32(nOffset + 4, false);
        const qint64 nRemaining = nFileSize - nOffset - 8;
        if (static_cast<quint64>(nPayloadSize) > static_cast<quint64>(nRemaining)) return false;
        nOffset += 8 + static_cast<qint64>(nPayloadSize);
    }

    return false;
}

bool XLZ5::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLZ5 lz5(pDevice);

    return lz5.isValid(pPdStruct);
}

XBinary::MODE XLZ5::getMode()
{
    return MODE_DATA;
}

qint32 XLZ5::getType()
{
    return TYPE_LZ5;
}

QString XLZ5::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    if (nType == TYPE_LZ5) {
        sResult = QString("LZ5");
    }

    return sResult;
}

XBinary::ENDIAN XLZ5::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XLZ5::getFileFormatExt()
{
    return "lz5";
}

QString XLZ5::getFileFormatExtsString()
{
    return "lz5;tlz5";
}

XBinary::FT XLZ5::getFileType()
{
    return FT_LZ5;
}

qint64 XLZ5::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QString XLZ5::getMIMEString()
{
    return "application/x-lz5";
}

XBinary::OSNAME XLZ5::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QList<XBinary::MAPMODE> XLZ5::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XLZ5::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XLZ5::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XLZ5_STRUCTID, sizeof(_TABLE_XLZ5_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XLZ5::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XLZ5_STRUCTID, sizeof(_TABLE_XLZ5_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XLZ5::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XLZ5_STRUCTID, sizeof(_TABLE_XLZ5_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// QList<XBinary::DATA_HEADER> XLZ5::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
//         _dataHeadersOptions.nID = STRUCTID_LZ5_FRAME_HEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         if (isPdStructNotCanceled(pPdStruct)) {
//             listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//         }
//     } else if (dataHeadersOptions.nID == STRUCTID_LZ5_FRAME_HEADER) {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XLZ5::structIDToString(dataHeadersOptions.nID));
//             dataHeader.nSize = sizeof(LZ5_FRAME_HEADER);
//             dataHeader.listRecords.append(getDataRecord(offsetof(LZ5_FRAME_HEADER, nMagic), 4, "nMagic", VT_UINT32, DRF_UNKNOWN, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(getDataRecord(offsetof(LZ5_FRAME_HEADER, nFLG), 1, "nFLG", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(getDataRecord(offsetof(LZ5_FRAME_HEADER, nBD), 1, "nBD", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(
//                 getDataRecord(offsetof(LZ5_FRAME_HEADER, nHeaderChecksum), 1, "nHeaderChecksum", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
//             listResult.append(dataHeader);
//         }
//     }

//     return listResult;
// }

QList<XBinary::XFHEADER> XLZ5::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XBinary::XFHEADER> listResult;
    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_LZ5_FRAME_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_LZ5_FRAME_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if ((nHeaderOffset != -1) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, sizeof(LZ5_FRAME_HEADER))) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_LZ5_FRAME_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = sizeof(LZ5_FRAME_HEADER);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_LZ5_FRAME_HEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_LZ5_FRAME_HEADER), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XLZ5::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_LZ5_FRAME_HEADER) {
        listResult.append({"nMagic", (qint32)offsetof(LZ5_FRAME_HEADER, nMagic), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"nFLG", (qint32)offsetof(LZ5_FRAME_HEADER, nFLG), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"nBD", (qint32)offsetof(LZ5_FRAME_HEADER, nBD), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"nHeaderChecksum", (qint32)offsetof(LZ5_FRAME_HEADER, nHeaderChecksum), 1, XFRECORD_FLAG_NONE, VT_UINT8});
    }

    return listResult;
}

static bool lz5CanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XLZ5::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    const qint64 nFileSize = getSize();
    if (nFileSize <= 0) return listResult;

    if ((nFileParts & FILEPART_HEADER) && lz5CanAppend(nLimit, listResult)) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = qMin<qint64>(static_cast<qint64>(sizeof(LZ5_FRAME_HEADER)), nFileSize);
        header.nVirtualAddress = XADDR_MAX;
        header.sName = tr("Header");
        listResult.append(header);
        if (!lz5CanAppend(nLimit, listResult)) return listResult;
    }

    if (!(nFileParts & (FILEPART_STREAM | FILEPART_DATA | FILEPART_OVERLAY))) return listResult;

    qint64 nCompressedSize = 0;
    qint64 nUncompressedSize = 0;
    if (!measureLz5Stream(getDevice(), nFileSize, &nCompressedSize, &nUncompressedSize, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_STREAM) && lz5CanAppend(nLimit, listResult)) {
        FPART region = {};
        region.filePart = FILEPART_STREAM;
        region.nFileOffset = 0;
        region.nFileSize = nCompressedSize;
        region.nVirtualAddress = XADDR_MAX;
        region.sName = tr("Stream");
        region.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZ5);
        region.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
        region.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nCompressedSize);
        listResult.append(region);
    }

    if ((nFileParts & FILEPART_DATA) && lz5CanAppend(nLimit, listResult)) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = 0;
        data.nFileSize = nCompressedSize;
        data.nVirtualAddress = XADDR_MAX;
        data.sName = tr("Data");
        listResult.append(data);
    }

    if ((nFileParts & FILEPART_OVERLAY) && lz5CanAppend(nLimit, listResult) && (nCompressedSize < nFileSize)) {
        FPART overlay = {};
        overlay.filePart = FILEPART_OVERLAY;
        overlay.nFileOffset = nCompressedSize;
        overlay.nFileSize = nFileSize - nCompressedSize;
        overlay.nVirtualAddress = XADDR_MAX;
        overlay.sName = tr("Overlay");
        listResult.append(overlay);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XLZ5::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XLZ5::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XLZ5> guardedArchive(this);
    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) pPdStruct = &pdStructEmpty;

    if (!pState || m_bUnpackOperationInProgress ||
        ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState))) return false;
    if (!guardedArchive->finishUnpack(pState, nullptr) || !guardedArchive) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const bool bBound = guardedArchive->bindUnpackSource(pState, pPdStruct);
    if (!guardedArchive || !bBound) return false;
    const bool bValid = guardedArchive->isValid(pPdStruct);
    if (!guardedArchive) return false;
    if (!bValid) {
        guardedArchive->releaseUnpackSource(pState);
        return false;
    }

    const qint64 nFileSize = guardedArchive->getSize();
    if (!guardedArchive) return false;
    qint64 nCompressedSize = 0;
    qint64 nUncompressedSize = 0;
    QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
    if (!guardedArchive || !guardedSource) return false;
    const bool bMeasured = measureLz5Stream(guardedSource.data(), nFileSize, &nCompressedSize, &nUncompressedSize,
                                            pPdStruct, &mapProperties);
    if (!guardedArchive || !guardedSource) return false;
    if (!bMeasured) {
        guardedArchive->releaseUnpackSource(pState);
        return false;
    }

    LZ5_UNPACK_CONTEXT *pContext = new (std::nothrow) LZ5_UNPACK_CONTEXT;
    if (!pContext) {
        guardedArchive->releaseUnpackSource(pState);
        return false;
    }
    pContext->nHeaderSize = qMin<qint64>(static_cast<qint64>(sizeof(LZ5_FRAME_HEADER)), nCompressedSize);
    pContext->nCompressedSize = nCompressedSize;
    pContext->nUncompressedSize = nUncompressedSize;
    pContext->sFileName = XBinary::getDeviceFileBaseName(guardedSource.data());
    if (!guardedArchive || !guardedSource) {
        if (guardedArchive) guardedArchive->releaseUnpackSource(pState);
        delete pContext;
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = nFileSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;
    if (!guardedArchive->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedArchive) return false;
        pState->pContext = nullptr;
        guardedArchive->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XLZ5::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<XLZ5> guardedArchive(this);

    XBinary::ARCHIVERECORD result = {};
    if (!pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    LZ5_UNPACK_CONTEXT *pContext = static_cast<LZ5_UNPACK_CONTEXT *>(pState->pContext);
    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZ5);

    return result;
}

bool XLZ5::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XLZ5> guardedArchive(this);

    if (!pState || !pState->pContext || !pDevice) return false;
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
    if (!guardedOutput || !guardedSource || !guardedArchive->isUnpackOutputSupported(guardedOutput.data()) || !guardedArchive ||
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) || !guardedArchive ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;

    LZ5_UNPACK_CONTEXT *pContext = static_cast<LZ5_UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->nCompressedSize < 0) || (pContext->nUncompressedSize < 0)) return false;
    const qint64 nCompressedSize = pContext->nCompressedSize;
    const qint64 nUncompressedSize = pContext->nUncompressedSize;
    if (!XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                            nUncompressedSize)) return false;
    std::unique_ptr<QIODevice> pStage(XBinary::createFileBuffer(nUncompressedSize, pPdStruct));
    if (!guardedArchive || !pStage || !guardedOutput || !guardedSource ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive) return false;

    SubDevice input(guardedSource.data(), 0, nCompressedSize);
    bool bResult = false;
    if (input.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
        state.mapUnpackProperties = pState->mapUnpackProperties;
        state.pDeviceInput = &input;
        state.pDeviceOutput = pStage.get();
        state.nInputOffset = 0;
        state.nInputLimit = nCompressedSize;
        state.nProcessedLimit = -1;
        bResult = XLZ5Decoder::decompress(&state, pPdStruct) && guardedArchive && guardedOutput && guardedSource &&
                  (state.nCountInput == nCompressedSize) && (state.nCountOutput == nUncompressedSize) &&
                  XBinary::isPdStructNotCanceled(pPdStruct);
        input.close();
    }

    bResult = bResult && guardedArchive && guardedOutput && guardedSource &&
              guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) && guardedArchive &&
              guardedArchive->publishUnpackOutput(pStage.get(), guardedOutput.data(), pState, pPdStruct);
    if (bResult && guardedArchive) pState->nCurrentOffset = nCompressedSize;
    return bResult;
}

bool XLZ5::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XLZ5> guardedArchive(this);

    if (!pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) ++pState->nCurrentIndex;
    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool XLZ5::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XLZ5> guardedArchive(this);
    Q_UNUSED(pPdStruct)

    if (!pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState)) return false;

    LZ5_UNPACK_CONTEXT *pContext = static_cast<LZ5_UNPACK_CONTEXT *>(pState->pContext);
    guardedArchive->releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    if (!guardedArchive) return false;

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    return true;
}

QList<XBinary::FPART_PROP> XLZ5::getAvailableFPARTProperties()
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

QList<QString> XLZ5::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("05224D18");
    listResult.append("502A4D18");

    return listResult;
}

XBinary *XLZ5::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XLZ5(pDevice);
}

bool XLZ5::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XLZ5> guardedThis(this);
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

void *XLZ5::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XLZ5> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XLZ5::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
