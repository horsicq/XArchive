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
#include "xlz4.h"
#include "Algos/xlz4decoder.h"

#include <limits>
#include <memory>
#include <new>

namespace {
const quint32 LZ4_STANDARD_MAGIC = 0x184D2204U;
const quint32 LZ4_LEGACY_MAGIC = 0x184C2102U;
const quint32 LZ4_SKIPPABLE_START = 0x184D2A50U;
const quint32 LZ4_SKIPPABLE_MASK = 0xFFFFFFF0U;

class Lz4DiscardDevice : public QIODevice {
protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *, qint64 nSize) override { return (nSize >= 0) ? nSize : -1; }
};

bool measureLz4Stream(QIODevice *pDevice, qint64 nFileSize, qint64 *pnCompressedSize, qint64 *pnUncompressedSize,
                      XBinary::PDSTRUCT *pPdStruct,
                      const QMap<XBinary::UNPACK_PROP, QVariant> *pUnpackProperties = nullptr)
{
    if (pnCompressedSize) *pnCompressedSize = 0;
    if (pnUncompressedSize) *pnUncompressedSize = 0;
    if (!pDevice || (nFileSize <= 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    SubDevice input(pDevice, 0, nFileSize);
    Lz4DiscardDevice output;
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

    const bool bResult = XLZ4Decoder::decompress(&state, pPdStruct) &&
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

XBinary::XCONVERT _TABLE_XLZ4_STRUCTID[] = {{XLZ4::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                            {XLZ4::STRUCTID_LZ4_FRAME_HEADER, "LZ4_FRAME_HEADER", QString("LZ4 frame header")}};

XLZ4::XLZ4(QIODevice *pDevice) : XArchive(pDevice)
{
}

XLZ4::~XLZ4()
{
}

bool XLZ4::isValid(PDSTRUCT *pPdStruct)
{
    const qint64 nFileSize = getSize();
    qint64 nOffset = 0;

    while (XBinary::isPdStructNotCanceled(pPdStruct) && (nOffset >= 0) && (nOffset <= nFileSize - 4)) {
        const quint32 nMagic = read_uint32(nOffset, false);
        if ((nMagic == LZ4_STANDARD_MAGIC) || (nMagic == LZ4_LEGACY_MAGIC)) return true;
        if ((nMagic & LZ4_SKIPPABLE_MASK) != LZ4_SKIPPABLE_START) return false;
        if (nOffset > nFileSize - 8) return false;

        const quint32 nPayloadSize = read_uint32(nOffset + 4, false);
        const qint64 nRemaining = nFileSize - nOffset - 8;
        if (static_cast<quint64>(nPayloadSize) > static_cast<quint64>(nRemaining)) return false;
        nOffset += 8 + static_cast<qint64>(nPayloadSize);
    }

    return false;
}

bool XLZ4::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLZ4 lz4(pDevice);

    return lz4.isValid(pPdStruct);
}

XBinary::MODE XLZ4::getMode()
{
    return MODE_DATA;
}

qint32 XLZ4::getType()
{
    return TYPE_LZ4;
}

QString XLZ4::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    if (nType == TYPE_LZ4) {
        sResult = QString("LZ4");
    }

    return sResult;
}

XBinary::ENDIAN XLZ4::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XLZ4::getFileFormatExt()
{
    return "lz4";
}

QString XLZ4::getFileFormatExtsString()
{
    return "lz4;tlz4";
}

XBinary::FT XLZ4::getFileType()
{
    return FT_LZ4;
}

qint64 XLZ4::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QString XLZ4::getMIMEString()
{
    return "application/x-lz4";
}

XBinary::OSNAME XLZ4::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QList<XBinary::MAPMODE> XLZ4::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XLZ4::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XLZ4::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XLZ4_STRUCTID, sizeof(_TABLE_XLZ4_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XLZ4::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XLZ4_STRUCTID, sizeof(_TABLE_XLZ4_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XLZ4::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XLZ4_STRUCTID, sizeof(_TABLE_XLZ4_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// QList<XBinary::DATA_HEADER> XLZ4::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
//         _dataHeadersOptions.nID = STRUCTID_LZ4_FRAME_HEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         if (isPdStructNotCanceled(pPdStruct)) {
//             listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//         }
//     } else if (dataHeadersOptions.nID == STRUCTID_LZ4_FRAME_HEADER) {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XLZ4::structIDToString(dataHeadersOptions.nID));
//             dataHeader.nSize = sizeof(LZ4_FRAME_HEADER);
//             dataHeader.listRecords.append(getDataRecord(offsetof(LZ4_FRAME_HEADER, nMagic), 4, "nMagic", VT_UINT32, DRF_UNKNOWN, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(getDataRecord(offsetof(LZ4_FRAME_HEADER, nFLG), 1, "nFLG", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(getDataRecord(offsetof(LZ4_FRAME_HEADER, nBD), 1, "nBD", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(
//                 getDataRecord(offsetof(LZ4_FRAME_HEADER, nHeaderChecksum), 1, "nHeaderChecksum", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
//             listResult.append(dataHeader);
//         }
//     }

//     return listResult;
// }

QList<XBinary::XFHEADER> XLZ4::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XBinary::XFHEADER> listResult;
    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_LZ4_FRAME_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_LZ4_FRAME_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if ((nHeaderOffset != -1) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, sizeof(LZ4_FRAME_HEADER))) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_LZ4_FRAME_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = sizeof(LZ4_FRAME_HEADER);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_LZ4_FRAME_HEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_LZ4_FRAME_HEADER), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XLZ4::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_LZ4_FRAME_HEADER) {
        listResult.append({"nMagic", (qint32)offsetof(LZ4_FRAME_HEADER, nMagic), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"nFLG", (qint32)offsetof(LZ4_FRAME_HEADER, nFLG), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"nBD", (qint32)offsetof(LZ4_FRAME_HEADER, nBD), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"nHeaderChecksum", (qint32)offsetof(LZ4_FRAME_HEADER, nHeaderChecksum), 1, XFRECORD_FLAG_NONE, VT_UINT8});
    }

    return listResult;
}

static bool lz4CanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XLZ4::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    const qint64 nFileSize = getSize();
    if (nFileSize <= 0) return listResult;

    if ((nFileParts & FILEPART_HEADER) && lz4CanAppend(nLimit, listResult)) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = qMin<qint64>(static_cast<qint64>(sizeof(LZ4_FRAME_HEADER)), nFileSize);
        header.nVirtualAddress = XADDR_MAX;
        header.sName = tr("Header");
        listResult.append(header);
        if (!lz4CanAppend(nLimit, listResult)) return listResult;
    }

    if (!(nFileParts & (FILEPART_STREAM | FILEPART_DATA | FILEPART_OVERLAY))) return listResult;

    qint64 nCompressedSize = 0;
    qint64 nUncompressedSize = 0;
    if (!measureLz4Stream(getDevice(), nFileSize, &nCompressedSize, &nUncompressedSize, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_STREAM) && lz4CanAppend(nLimit, listResult)) {
        FPART region = {};
        region.filePart = FILEPART_STREAM;
        region.nFileOffset = 0;
        region.nFileSize = nCompressedSize;
        region.nVirtualAddress = XADDR_MAX;
        region.sName = tr("Stream");
        region.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZ4);
        region.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
        region.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nCompressedSize);
        listResult.append(region);
    }

    if ((nFileParts & FILEPART_DATA) && lz4CanAppend(nLimit, listResult)) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = 0;
        data.nFileSize = nCompressedSize;
        data.nVirtualAddress = XADDR_MAX;
        data.sName = tr("Data");
        listResult.append(data);
    }

    if ((nFileParts & FILEPART_OVERLAY) && lz4CanAppend(nLimit, listResult) && (nCompressedSize < nFileSize)) {
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

QMap<XBinary::UNPACK_PROP, QVariant> XLZ4::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XLZ4::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XLZ4> guardedArchive(this);
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
    const bool bMeasured = measureLz4Stream(guardedSource.data(), nFileSize, &nCompressedSize, &nUncompressedSize,
                                            pPdStruct, &mapProperties);
    if (!guardedArchive || !guardedSource) return false;
    if (!bMeasured) {
        guardedArchive->releaseUnpackSource(pState);
        return false;
    }

    LZ4_UNPACK_CONTEXT *pContext = new (std::nothrow) LZ4_UNPACK_CONTEXT;
    if (!pContext) {
        guardedArchive->releaseUnpackSource(pState);
        return false;
    }
    pContext->nHeaderSize = qMin<qint64>(static_cast<qint64>(sizeof(LZ4_FRAME_HEADER)), nCompressedSize);
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

XBinary::ARCHIVERECORD XLZ4::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<XLZ4> guardedArchive(this);

    XBinary::ARCHIVERECORD result = {};
    if (!pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    LZ4_UNPACK_CONTEXT *pContext = static_cast<LZ4_UNPACK_CONTEXT *>(pState->pContext);
    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZ4);

    return result;
}

bool XLZ4::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XLZ4> guardedArchive(this);

    if (!pState || !pState->pContext || !pDevice) return false;
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
    if (!guardedOutput || !guardedSource || !guardedArchive->isUnpackOutputSupported(guardedOutput.data()) || !guardedArchive ||
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) || !guardedArchive ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;

    LZ4_UNPACK_CONTEXT *pContext = static_cast<LZ4_UNPACK_CONTEXT *>(pState->pContext);
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
        bResult = XLZ4Decoder::decompress(&state, pPdStruct) && guardedArchive && guardedOutput && guardedSource &&
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

bool XLZ4::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XLZ4> guardedArchive(this);

    if (!pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) ++pState->nCurrentIndex;
    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool XLZ4::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XLZ4> guardedArchive(this);
    Q_UNUSED(pPdStruct)

    if (!pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState)) return false;

    LZ4_UNPACK_CONTEXT *pContext = static_cast<LZ4_UNPACK_CONTEXT *>(pState->pContext);
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

QList<XBinary::FPART_PROP> XLZ4::getAvailableFPARTProperties()
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

QList<QString> XLZ4::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("04224D18");
    listResult.append("02214C18");

    return listResult;
}

XBinary *XLZ4::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XLZ4(pDevice);
}

bool XLZ4::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XLZ4> guardedThis(this);
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

void *XLZ4::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XLZ4> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XLZ4::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
