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
#include "xzstd.h"
#include "Algos/xzstddecoder.h"

#include <memory>
#include <new>

namespace {
const quint32 ZSTD_STANDARD_MAGIC = 0xFD2FB528U;
const quint32 ZSTD_LEGACY_MAGIC_V04 = 0xFD2FB524U;
const quint32 ZSTD_LEGACY_MAGIC_V07 = 0xFD2FB527U;
const quint32 ZSTD_SKIPPABLE_START = 0x184D2A50U;
const quint32 ZSTD_SKIPPABLE_MASK = 0xFFFFFFF0U;

bool isSupportedZstdDataMagic(quint32 nMagic)
{
    return (nMagic == ZSTD_STANDARD_MAGIC) ||
           ((nMagic >= ZSTD_LEGACY_MAGIC_V04) && (nMagic <= ZSTD_LEGACY_MAGIC_V07));
}

class ZstdDiscardDevice : public QIODevice {
protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *, qint64 nSize) override { return (nSize >= 0) ? nSize : -1; }
};

bool measureZstdStream(QIODevice *pDevice, qint64 nFileSize, qint64 *pnCompressedSize, qint64 *pnUncompressedSize,
                       XBinary::PDSTRUCT *pPdStruct,
                       const QMap<XBinary::UNPACK_PROP, QVariant> *pUnpackProperties = nullptr)
{
    if (pnCompressedSize) *pnCompressedSize = 0;
    if (pnUncompressedSize) *pnUncompressedSize = 0;
    if (!pDevice || (nFileSize <= 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    SubDevice input(pDevice, 0, nFileSize);
    ZstdDiscardDevice output;
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

    const bool bResult = XZstdDecoder::decompress(&state, pPdStruct) &&
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

XBinary::XCONVERT _TABLE_XZstd_STRUCTID[] = {{XZstd::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                             {XZstd::STRUCTID_ZSTD_HEADER, "ZSTD_HEADER", QString("Zstandard header")}};

XZstd::XZstd(QIODevice *pDevice) : XArchive(pDevice)
{
}

XZstd::~XZstd()
{
}

bool XZstd::isValid(PDSTRUCT *pPdStruct)
{
    const qint64 nFileSize = getSize();
    qint64 nOffset = 0;

    // A Zstandard stream may begin with one or more skippable frames.  Walk
    // their bounded payloads until a real data frame is found; an input made
    // only of skippable frames is not an extractable Zstandard stream.
    while (XBinary::isPdStructNotCanceled(pPdStruct) &&
           (nOffset >= 0) && (nOffset <= nFileSize - 4)) {
        const quint32 nMagic = read_uint32(nOffset, false);
        if (isSupportedZstdDataMagic(nMagic)) return true;
        if ((nMagic & ZSTD_SKIPPABLE_MASK) != ZSTD_SKIPPABLE_START) {
            return false;
        }
        if (nOffset > nFileSize - 8) return false;

        const quint32 nPayloadSize = read_uint32(nOffset + 4, false);
        const qint64 nRemaining = nFileSize - nOffset - 8;
        if (static_cast<quint64>(nPayloadSize) >
            static_cast<quint64>(nRemaining)) {
            return false;
        }
        nOffset += 8 + static_cast<qint64>(nPayloadSize);
    }

    return false;
}

bool XZstd::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZstd xzstd(pDevice);

    return xzstd.isValid(pPdStruct);
}

XBinary::MODE XZstd::getMode()
{
    return MODE_DATA;
}

qint32 XZstd::getType()
{
    return TYPE_ZST;
}

XBinary::ENDIAN XZstd::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XZstd::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_ZST: sResult = QString("ZST"); break;
    }

    return sResult;
}

QString XZstd::getFileFormatExt()
{
    return "zst";
}

XBinary::FT XZstd::getFileType()
{
    return FT_ZSTD;
}

QString XZstd::getFileFormatExtsString()
{
    return "zst;zstd;tzst;tzstd";
}

qint64 XZstd::getFileFormatSize(XBinary::PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QString XZstd::getMIMEString()
{
    return "application/zstd";
}

XBinary::OSNAME XZstd::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QList<XBinary::MAPMODE> XZstd::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XZstd::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XZstd::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XZstd_STRUCTID, sizeof(_TABLE_XZstd_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XZstd::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XZstd_STRUCTID, sizeof(_TABLE_XZstd_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XZstd::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XZstd_STRUCTID, sizeof(_TABLE_XZstd_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// QList<XBinary::DATA_HEADER> XZstd::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
//         _dataHeadersOptions.nID = STRUCTID_ZSTD_HEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         if (isPdStructNotCanceled(pPdStruct)) {
//             listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//         }
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             if (dataHeadersOptions.nID == STRUCTID_ZSTD_HEADER) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XZstd::structIDToString(dataHeadersOptions.nID));
//                 dataHeader.nSize = sizeof(ZSTD_HEADER);

//                 dataHeader.listRecords.append(getDataRecord(offsetof(ZSTD_HEADER, nMagic), 4, "nMagic", VT_UINT32, DRF_UNKNOWN,
//                 dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);
//             }
//         }
//     }

//     return listResult;
// }

QList<XBinary::XFHEADER> XZstd::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XBinary::XFHEADER> listResult;
    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_ZSTD_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_ZSTD_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if ((nHeaderOffset != -1) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, sizeof(ZSTD_HEADER))) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_ZSTD_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = sizeof(ZSTD_HEADER);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_ZSTD_HEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_ZSTD_HEADER), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XZstd::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_ZSTD_HEADER) {
        listResult.append({"nMagic", (qint32)offsetof(ZSTD_HEADER, nMagic), 4, XFRECORD_FLAG_NONE, VT_UINT32});
    }

    return listResult;
}

static bool zstdCanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XZstd::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    const qint64 nFileSize = getSize();
    if (nFileSize <= 0) return listResult;

    // Header: fixed 4 bytes (magic number)
    if ((nFileParts & FILEPART_HEADER) && zstdCanAppend(nLimit, listResult)) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = qMin<qint64>(4, nFileSize);
        header.nVirtualAddress = XADDR_MAX;
        header.sName = tr("Header");
        listResult.append(header);
        if (!zstdCanAppend(nLimit, listResult)) return listResult;
    }

    if (!(nFileParts & (FILEPART_STREAM | FILEPART_DATA | FILEPART_OVERLAY))) return listResult;

    qint64 nCompressedSize = 0;
    qint64 nUncompressedSize = 0;
    if (!measureZstdStream(getDevice(), nFileSize, &nCompressedSize, &nUncompressedSize, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_STREAM) && zstdCanAppend(nLimit, listResult)) {
        FPART region = {};
        region.filePart = FILEPART_STREAM;
        region.nFileOffset = 0;
        region.nFileSize = nCompressedSize;
        region.nVirtualAddress = XADDR_MAX;
        region.sName = tr("Stream");
        region.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZSTD);
        region.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
        region.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nCompressedSize);
        listResult.append(region);
    }

    // Data: entire file
    if ((nFileParts & FILEPART_DATA) && zstdCanAppend(nLimit, listResult)) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = 0;
        data.nFileSize = nCompressedSize;
        data.nVirtualAddress = XADDR_MAX;
        data.sName = tr("Data");
        listResult.append(data);
    }

    // Overlay: any trailing bytes
    if ((nFileParts & FILEPART_OVERLAY) && zstdCanAppend(nLimit, listResult)) {
        if (nCompressedSize < nFileSize) {
            FPART ov = {};
            ov.filePart = FILEPART_OVERLAY;
            ov.nFileOffset = nCompressedSize;
            ov.nFileSize = nFileSize - nCompressedSize;
            ov.nVirtualAddress = XADDR_MAX;
            ov.sName = tr("Overlay");
            listResult.append(ov);
        }
    }

    return listResult;
}

XZstd::ZSTD_HEADER XZstd::_read_ZSTD_HEADER(qint64 nOffset)
{
    ZSTD_HEADER result = {};

    result.nMagic = read_uint32(nOffset, false);

    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XZstd::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XZstd::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XZstd> guardedArchive(this);
    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (!pState || m_bUnpackOperationInProgress ||
        ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState))) return false;
    if (!guardedArchive->finishUnpack(pState, nullptr) || !guardedArchive) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
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
    const bool bMeasured = measureZstdStream(
        guardedSource.data(), nFileSize, &nCompressedSize,
        &nUncompressedSize, pPdStruct, &mapProperties);
    if (!guardedArchive || !guardedSource) return false;
    if (!bMeasured) {
        guardedArchive->releaseUnpackSource(pState);
        return false;
    }

    ZSTD_UNPACK_CONTEXT *pContext = new (std::nothrow) ZSTD_UNPACK_CONTEXT;
    if (!pContext) {
        guardedArchive->releaseUnpackSource(pState);
        return false;
    }
    pContext->nHeaderSize = 4;
    pContext->nCompressedSize = nCompressedSize;
    pContext->nUncompressedSize = nUncompressedSize;
    pContext->sFileName = XBinary::getDeviceFileBaseName(
        guardedSource.data());
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
    if (!guardedArchive->validateAndFinalizeUnpackSource(
            pState, pContext, pPdStruct)) {
        if (!guardedArchive) return false;
        pState->pContext = nullptr;
        guardedArchive->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XZstd::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<XZstd> guardedArchive(this);

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedArchive) {
        return result;
    }

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    ZSTD_UNPACK_CONTEXT *pContext = (ZSTD_UNPACK_CONTEXT *)pState->pContext;

    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nCompressedSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZSTD);

    return result;
}

bool XZstd::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XZstd> guardedArchive(this);

    if (!pState || !pState->pContext || !pDevice) return false;
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
    if (!guardedOutput || !guardedSource ||
        !guardedArchive->isUnpackOutputSupported(guardedOutput.data()) || !guardedArchive ||
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) ||
        !guardedArchive ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedArchive ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;

    ZSTD_UNPACK_CONTEXT *pContext = static_cast<ZSTD_UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->nCompressedSize < 0) ||
        (pContext->nUncompressedSize < 0)) return false;
    const qint64 nCompressedSize = pContext->nCompressedSize;
    const qint64 nUncompressedSize = pContext->nUncompressedSize;
    if (!XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                            nUncompressedSize)) return false;

    // This override bypasses the base decode chain's per-entry gate; account
    // the member here. Produced bytes are charged by _writeDevice through
    // state.spOutputBudget.
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, pContext->sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(XBinary::createFileBuffer(
        nUncompressedSize, pPdStruct));
    if (!guardedArchive || !pStage || !guardedOutput || !guardedSource ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedArchive) return false;

    SubDevice input(guardedSource.data(), 0, nCompressedSize);
    bool bResult = false;

    if (input.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
        state.mapUnpackProperties = pState->mapUnpackProperties;
        state.spOutputBudget = pState->spOutputBudget;
        state.pDeviceInput = &input;
        state.pDeviceOutput = pStage.get();
        state.nInputOffset = 0;
        state.nInputLimit = nCompressedSize;
        state.nProcessedLimit = -1;
        bResult = XZstdDecoder::decompress(&state, pPdStruct) &&
                  guardedArchive && guardedOutput && guardedSource &&
                  (state.nCountInput == nCompressedSize) &&
                  (state.nCountOutput == nUncompressedSize) &&
                  XBinary::isPdStructNotCanceled(pPdStruct);
        input.close();
    }

    bResult = bResult && guardedArchive && guardedOutput && guardedSource &&
              guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) &&
              guardedArchive &&
              guardedArchive->publishUnpackOutput(pStage.get(), guardedOutput.data(), pState,
                                  pPdStruct);
    if (bResult && guardedArchive) pState->nCurrentOffset = nCompressedSize;
    return bResult;
}

bool XZstd::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XZstd> guardedArchive(this);

    if (!pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) ++pState->nCurrentIndex;
    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool XZstd::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XZstd> guardedArchive(this);

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState)) return false;

    ZSTD_UNPACK_CONTEXT *pContext =
        static_cast<ZSTD_UNPACK_CONTEXT *>(pState->pContext);
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

QList<XBinary::FPART_PROP> XZstd::getAvailableFPARTProperties()
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

QList<QString> XZstd::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("24B52FFD");
    listResult.append("25B52FFD");
    listResult.append("26B52FFD");
    listResult.append("27B52FFD");
    listResult.append("28B52FFD");

    return listResult;
}

XBinary *XZstd::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XZstd(pDevice);
}

bool XZstd::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XZstd> guardedThis(this);
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

void *XZstd::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XZstd> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XZstd::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
