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
#include "xbzip2.h"
#include "Algos/xbzip2decoder.h"
#include "xdecompress.h"

#include <new>

namespace {
class Bzip2DiscardDevice : public QIODevice {
protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *, qint64 nSize) override { return (nSize >= 0) ? nSize : -1; }
};

bool measureBzip2Stream(QIODevice *pDevice, qint64 nFileSize, qint64 *pnCompressedSize, qint64 *pnUncompressedSize,
                        XBinary::PDSTRUCT *pPdStruct)
{
    if (pnCompressedSize) *pnCompressedSize = 0;
    if (pnUncompressedSize) *pnUncompressedSize = 0;
    if (!pDevice || (nFileSize <= 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    SubDevice input(pDevice, 0, nFileSize);
    Bzip2DiscardDevice output;
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

    const bool bResult = XBZIP2Decoder::decompress(&state, pPdStruct) &&
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

XBinary::XCONVERT _TABLE_XBZIP2_STRUCTID[] = {{XBZIP2::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                              {XBZIP2::STRUCTID_BZIP2_HEADER, "BZIP2_HEADER", QString("BZip2 header")},
                                              {XBZIP2::STRUCTID_BLOCK_HEADER, "BLOCK_HEADER", QString("Block header")}};

XBZIP2::XBZIP2(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XBZIP2::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XBZIP2> guardedThis(this);
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const qint64 nSize = getSize();
    if (!guardedThis || (nSize < 14)) return false;
    _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
    if (!guardedThis) return false;
    const bool bPrimary =
        compareSignature(&memoryMap, "'BZh'..314159265359", 0, pPdStruct);
    if (!guardedThis || bPrimary) return guardedThis && bPrimary;
    const bool bAlternate = compareSignature(
        &memoryMap, "'BZh'..17724538509000000000", 0, pPdStruct);
    return guardedThis && bAlternate;
}

bool XBZIP2::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBZIP2 bzip2(pDevice);

    return bzip2.isValid(pPdStruct);
}

XBinary::MODE XBZIP2::getMode()
{
    return MODE_DATA;
}

qint32 XBZIP2::getType()
{
    return TYPE_BZ2;
}

XBinary::ENDIAN XBZIP2::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XBZIP2::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_BZ2: sResult = QString("BZ2"); break;
    }

    return sResult;
}

QString XBZIP2::getFileFormatExt()
{
    return "bz2";
}

XBinary::FT XBZIP2::getFileType()
{
    return FT_BZIP2;
}

QString XBZIP2::getFileFormatExtsString()
{
    return "bz2";
}

qint64 XBZIP2::getFileFormatSize(XBinary::PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QString XBZIP2::getMIMEString()
{
    return "application/x-bzip2";
}

XBinary::OSNAME XBZIP2::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QList<XBinary::MAPMODE> XBZIP2::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XBZIP2::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XBZIP2::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XBZIP2_STRUCTID, sizeof(_TABLE_XBZIP2_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XBZIP2::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XBZIP2_STRUCTID, sizeof(_TABLE_XBZIP2_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XBZIP2::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XBZIP2_STRUCTID, sizeof(_TABLE_XBZIP2_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// QList<XBinary::DATA_HEADER> XBZIP2::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
//         _dataHeadersOptions.nID = STRUCTID_BZIP2_HEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         if (isPdStructNotCanceled(pPdStruct)) {
//             listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//         }
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             if (dataHeadersOptions.nID == STRUCTID_BZIP2_HEADER) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XBZIP2::structIDToString(dataHeadersOptions.nID));
//                 dataHeader.nSize = sizeof(BZIP2_HEADER);

//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(BZIP2_HEADER, magic), 3, "magic", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(BZIP2_HEADER, blockSize), 1, "blockSize", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);
//             }
//         }
//     }

//     return listResult;
// }

QList<XBinary::XFHEADER> XBZIP2::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XBinary::XFHEADER> listResult;
    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_BZIP2_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_BZIP2_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if ((nHeaderOffset != -1) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, sizeof(BZIP2_HEADER))) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_BZIP2_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = sizeof(BZIP2_HEADER);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_BZIP2_HEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_BZIP2_HEADER), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XBZIP2::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_BZIP2_HEADER) {
        listResult.append({"magic", static_cast<qint32>(offsetof(BZIP2_HEADER, magic)), 3, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"blockSize", static_cast<qint32>(offsetof(BZIP2_HEADER, blockSize)), 1, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
    }

    return listResult;
}

QList<XBinary::FPART> XBZIP2::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
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
        header.nFileSize = qMin<qint64>(4, nFileSize);
        header.nVirtualAddress = -1;
        header.sName = tr("Header");
        listResult.append(header);
        if (!canAppend()) return listResult;
    }

    if (!(nFileParts & (FILEPART_STREAM | FILEPART_DATA | FILEPART_OVERLAY))) return listResult;

    qint64 nCompressedSize = 0;
    qint64 nUncompressedSize = 0;
    if (!measureBzip2Stream(getDevice(), nFileSize, &nCompressedSize, &nUncompressedSize, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_STREAM) && canAppend()) {
        FPART region = {};
        region.filePart = FILEPART_STREAM;
        region.nFileOffset = 0;
        region.nFileSize = nCompressedSize;
        region.nVirtualAddress = -1;
        region.sName = tr("Stream");
        region.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_BZIP2);
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

XBZIP2::BZIP2_HEADER XBZIP2::_read_BZIP2_HEADER(qint64 nOffset)
{
    BZIP2_HEADER result = {};

    read_array(nOffset, reinterpret_cast<char *>(&result.magic), 3);
    result.blockSize = read_uint8(nOffset + 3);

    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XBZIP2::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XBZIP2::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XBZIP2> guardedThis(this);
    if (!pState || m_bUnpackOperationInProgress ||
        ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState))) {
        return false;
    }
    const bool bFinished = finishUnpack(pState, nullptr);
    if (!guardedThis || !bFinished) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;
    const bool bValid = isValid(pPdStruct);
    if (!guardedThis) return false;
    if (!bValid) {
        releaseUnpackSource(pState);
        return false;
    }

    const qint64 nFileSize = getSize();
    if (!guardedThis) return false;
    qint64 nCompressedSize = 0;
    qint64 nUncompressedSize = 0;
    QPointer<QIODevice> guardedSource(getDevice());
    const bool bMeasured = guardedSource && measureBzip2Stream(
        guardedSource.data(), nFileSize, &nCompressedSize,
        &nUncompressedSize, pPdStruct);
    if (!guardedThis) return false;
    if (!bMeasured) {
        releaseUnpackSource(pState);
        return false;
    }
    const QString sFileName = XBinary::getDeviceFileBaseName(guardedSource.data());
    if (!guardedThis) return false;

    BZIP2_UNPACK_CONTEXT *pContext = new (std::nothrow) BZIP2_UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    pContext->nHeaderSize = 4;
    pContext->nCompressedSize = nCompressedSize;
    pContext->nUncompressedSize = nUncompressedSize;
    pContext->sFileName = sFileName;

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = nFileSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;
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

XBinary::ARCHIVERECORD XBZIP2::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XBZIP2> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return result;
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return result;

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    BZIP2_UNPACK_CONTEXT *pContext = reinterpret_cast<BZIP2_UNPACK_CONTEXT *>(pState->pContext);

    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nCompressedSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_BZIP2);

    return result;
}

bool XBZIP2::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XBZIP2> guardedThis(this);
    if (!pState || !pState->pContext || !pDevice ||
        !ownsUnpackSource(pState) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;

    const qint64 nCompressedSize =
        static_cast<BZIP2_UNPACK_CONTEXT *>(pState->pContext)
            ->nCompressedSize;
    // The base implementation owns the operation guard and performs the final
    // source/output validation after publication.  Do not make another
    // callback-bearing source check after that guard has been released.
    const bool bResult = XArchive::unpackCurrent(
        pState, pDevice, pPdStruct);
    if (!guardedThis) return false;
    if (bResult) pState->nCurrentOffset = nCompressedSize;
    return bResult;
}

bool XBZIP2::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XBZIP2> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState || !pState->pContext ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) ++pState->nCurrentIndex;
    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool XBZIP2::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XBZIP2> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    BZIP2_UNPACK_CONTEXT *pContext =
        static_cast<BZIP2_UNPACK_CONTEXT *>(pState->pContext);
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

QList<XBinary::FPART_PROP> XBZIP2::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD, FPART_PROP_STREAMOFFSET, FPART_PROP_STREAMSIZE};
}

QList<QString> XBZIP2::getSearchSignatures()
{
    return {"'BZh'..314159265359", "'BZh'..17724538509000000000"};
}

XBinary *XBZIP2::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XBZIP2(pDevice);
}

bool XBZIP2::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XArchive::handleInternalInfo(pPdStruct);
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) =
            *static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
    }

    return bResult;
}

void *XBZIP2::getInternalInfo(PDSTRUCT *pPdStruct)
{
    handleInternalInfo(pPdStruct);

    return &m_internalInfo;
}

void XBZIP2::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
