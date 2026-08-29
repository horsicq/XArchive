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
#include "xszdd.h"

#include <memory>
#include <new>
#include "Algos/xlzssdecoder.h"

static XBinary::XCONVERT _TABLE_XSZDD_STRUCTID[] = {{XSZDD::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                    {XSZDD::STRUCTID_SZDD_HEADER, "SZDD_HEADER", QString("SZDD_HEADER")}};

static const qint64 SZDD_STANDARD_HEADER_SIZE = 14;
static const qint64 SZDD_LEGACY_HEADER_SIZE = 12;

bool XSZDD::_isStandardSZDDSignature(const QByteArray &baSig)
{
    return (baSig.size() == 8) && (baSig.at(0) == 'S') && (baSig.at(1) == 'Z') && (baSig.at(2) == 'D') && (baSig.at(3) == 'D') && ((quint8)baSig.at(4) == 0x88) &&
           ((quint8)baSig.at(5) == 0xF0) && ((quint8)baSig.at(6) == 0x27) && (((quint8)baSig.at(7) == 0x33) || ((quint8)baSig.at(7) == 0x3A));
}

bool XSZDD::_isLegacySZDDSignature(const QByteArray &baSig)
{
    return (baSig.size() == 8) && (baSig.at(0) == 'Z') && (baSig.at(1) == 'D') && (baSig.at(2) == 'D') && ((quint8)baSig.at(3) == 0x88) &&
           ((quint8)baSig.at(4) == 0xF0) && ((quint8)baSig.at(5) == 0x27) && (((quint8)baSig.at(6) == 0x33) || ((quint8)baSig.at(6) == 0x3A)) &&
           ((quint8)baSig.at(7) == 0x41);
}

qint64 XSZDD::_getHeaderSize()
{
    QPointer<XSZDD> guardedArchive(this);
    const QByteArray baSig = guardedArchive->read_array(0, 8);
    if (!guardedArchive) return -1;

    if (_isLegacySZDDSignature(baSig)) {
        return SZDD_LEGACY_HEADER_SIZE;
    }

    return SZDD_STANDARD_HEADER_SIZE;
}

qint64 XSZDD::_getUncompressedSizeOffset()
{
    QPointer<XSZDD> guardedArchive(this);
    const QByteArray baSig = guardedArchive->read_array(0, 8);
    if (!guardedArchive) return -1;

    return _isLegacySZDDSignature(baSig) ? 8 : 10;
}

XSZDD::XSZDD(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XSZDD::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSZDD xszdd(pDevice);

    return xszdd.isValid(pPdStruct);
}

bool XSZDD::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XSZDD> guardedArchive(this);
    bool bResult = false;

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint64 nSize = guardedArchive->getSize();
    if (!guardedArchive) return false;
    if (nSize >= SZDD_LEGACY_HEADER_SIZE) {
        QByteArray baSig = guardedArchive->read_array(0, 8);
        if (!guardedArchive) return false;

        if (_isStandardSZDDSignature(baSig) && (nSize >= SZDD_STANDARD_HEADER_SIZE)) {
            bResult = true;
        }

        // Compact/legacy variant seen in tests: 5A 44 44 88 F0 27 33/3A 41  ("ZDD...")
        if (_isLegacySZDDSignature(baSig)) {
            bResult = true;
        }
    }

    return bResult;
}

XSZDD::SZDD_HEADER XSZDD::_read_SZDD_HEADER(qint64 nOffset)
{
    SZDD_HEADER header = {};
    read_array(nOffset, reinterpret_cast<char *>(&header), sizeof(SZDD_HEADER));
    return header;
}

XBinary::FT XSZDD::getFileType()
{
    return XBinary::FT_SZDD;
}

XBinary::MODE XSZDD::getMode()
{
    return XBinary::MODE_DATA;
}

QString XSZDD::getMIMEString()
{
    return "application/x-ms-compress";
}

qint32 XSZDD::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

XBinary::ENDIAN XSZDD::getEndian()
{
    return XBinary::ENDIAN_LITTLE;
}

QString XSZDD::getArch()
{
    return QString();
}

QString XSZDD::getFileFormatExt()
{
    return "SZDD";
}

QString XSZDD::getFileFormatExtsString()
{
    return "SZDD (*.szdd)";
}

qint64 XSZDD::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

bool XSZDD::isSigned()
{
    return false;
}

XBinary::OSNAME XSZDD::getOsName()
{
    return XBinary::OSNAME_MULTIPLATFORM;
}

QString XSZDD::getOsVersion()
{
    return QString();
}

QString XSZDD::getVersion()
{
    return QString();
}

bool XSZDD::isEncrypted()
{
    return false;
}

QList<XBinary::MAPMODE> XSZDD::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XSZDD::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)
    Q_UNUSED(pPdStruct)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.mode = getMode();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());
    result.sArch = getArch();
    result.nBinarySize = getSize();

    qint32 nIndex = 0;

    const qint64 nHeaderSize = _getHeaderSize();
    const qint64 nUncompressedOffset = _getUncompressedSizeOffset();

    if ((nHeaderSize < SZDD_LEGACY_HEADER_SIZE) || (getSize() < nHeaderSize) || (nUncompressedOffset < 0) || (nUncompressedOffset + 4 > getSize())) {
        return result;
    }

    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = XADDR_MAX;
    recHeader.nOffset = 0;
    recHeader.nSize = nHeaderSize;
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = QString("SZDD ") + tr("Header");
    result.listRecords.append(recHeader);

    SubDevice sd(getDevice(), nHeaderSize, -1);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZSS_SZDD);
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, read_uint32(nUncompressedOffset));
        state.pDeviceInput = &sd;
        state.pDeviceOutput = nullptr;
        state.nInputOffset = 0;
        state.nInputLimit = getSize() - nHeaderSize;
        state.nProcessedOffset = 0;
        state.nProcessedLimit = -1;

        bool bResult = XLZSSDecoder::decompress(&state, pPdStruct);

        Q_UNUSED(bResult)

        _MEMORY_RECORD memoryRecord = {};

        memoryRecord.nOffset = nHeaderSize;
        memoryRecord.nAddress = XADDR_MAX;
        memoryRecord.nSize = state.nCountInput;
        memoryRecord.filePart = FILEPART_REGION;

        result.listRecords.append(memoryRecord);

        sd.close();
    }

    _handleOverlay(&result);

    return result;
}

QString XSZDD::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XSZDD_STRUCTID, sizeof(_TABLE_XSZDD_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XSZDD::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XSZDD_STRUCTID, sizeof(_TABLE_XSZDD_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XSZDD::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XSZDD_STRUCTID, sizeof(_TABLE_XSZDD_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XSZDD::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_SZDD_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_SZDD_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_SZDD_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = sizeof(SZDD_HEADER);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_SZDD_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_SZDD_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XSZDD::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_SZDD_HEADER) {
        listResult.append({"signature", (qint32)offsetof(SZDD_HEADER, signature), 8, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"compression_mode", (qint32)offsetof(SZDD_HEADER, compression_mode), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"missing_char", (qint32)offsetof(SZDD_HEADER, missing_char), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"uncompressed_size", (qint32)offsetof(SZDD_HEADER, uncompressed_size), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
    }

    return listResult;
}

// QList<XBinary::DATA_HEADER> XSZDD::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     Q_UNUSED(pPdStruct)
//     QList<DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = DHMODE_HEADER;

//         _dataHeadersOptions.nID = STRUCTID_SZDD_HEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = LT_OFFSET;

//         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//     } else if (dataHeadersOptions.nID == STRUCTID_SZDD_HEADER) {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             DATA_HEADER dataHeader = {};
//             dataHeader.dsID_parent = dataHeadersOptions.dsID_parent;
//             dataHeader.dsID.sGUID = generateUUID();
//             dataHeader.dsID.fileType = dataHeadersOptions.pMemoryMap->fileType;
//             dataHeader.dsID.nID = dataHeadersOptions.nID;
//             dataHeader.locType = dataHeadersOptions.locType;
//             dataHeader.nLocation = dataHeadersOptions.nLocation;
//             dataHeader.sName = structIDToString(dataHeadersOptions.nID);
//             dataHeader.dhMode = dataHeadersOptions.dhMode;
//             dataHeader.nSize = sizeof(SZDD_HEADER);

//             dataHeader.listRecords.append(getDataRecord(offsetof(SZDD_HEADER, signature), 8, "signature", VT_BYTE_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(getDataRecord(offsetof(SZDD_HEADER, compression_mode), 1, "compression_mode", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(getDataRecord(offsetof(SZDD_HEADER, missing_char), 1, "missing_char", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(getDataRecord(offsetof(SZDD_HEADER, uncompressed_size), 4, "uncompressed_size", VT_UINT32, DRF_UNKNOWN, ENDIAN_LITTLE));

//             listResult.append(dataHeader);
//         }
//     }

//     return listResult;
// }

static bool szddCanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XSZDD::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    if (nFileParts == 0) {
        return listResult;
    }

    const qint64 nHeaderSize = _getHeaderSize();

    if ((nFileParts & FILEPART_HEADER) && szddCanAppend(nLimit, listResult)) {
        FPART record = {};

        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = nHeaderSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");

        listResult.append(record);
    }

    const qint64 nTotalSize = getSize();
    if (nTotalSize < nHeaderSize) {
        return listResult;
    }
    qint64 nDataOffset = nHeaderSize;

    if ((nFileParts & FILEPART_REGION) && szddCanAppend(nLimit, listResult)) {
        if (nDataOffset < nTotalSize) {
            FPART record = {};

            record.filePart = FILEPART_REGION;
            record.nFileOffset = nDataOffset;
            record.nFileSize = nTotalSize - nDataOffset;
            record.nVirtualAddress = XADDR_MAX;
            record.sName = tr("Compressed Data");

            listResult.append(record);
        }
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XSZDD::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XSZDD::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSZDD> guardedArchive(this);
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState && !m_bUnpackOperationInProgress && ((!pState->pContext && pState->baUnpackSourceToken.isEmpty()) || guardedArchive->ownsUnpackSource(pState))) {
        if (!guardedArchive->finishUnpack(pState, nullptr) || !guardedArchive) return false;
        UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
        if (!operationGuard.isAcquired()) return false;

        if (!isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        const bool bBound = guardedArchive->bindUnpackSource(pState, pPdStruct);
        if (!guardedArchive || !bBound) return false;

        const bool bValid = guardedArchive->isValid(pPdStruct);
        if (!guardedArchive) return false;
        if (!bValid) {
            guardedArchive->releaseUnpackSource(pState);
            return false;
        }

        const qint64 nHeaderSize = guardedArchive->_getHeaderSize();
        if (!guardedArchive) return false;
        const qint64 nUncompressedOffset = guardedArchive->_getUncompressedSizeOffset();
        if (!guardedArchive) return false;
        const qint64 nSize = guardedArchive->getSize();
        if (!guardedArchive) return false;
        if ((nHeaderSize < SZDD_LEGACY_HEADER_SIZE) || (nSize < nHeaderSize) || (nUncompressedOffset < 0) || (nUncompressedOffset + 4 > nSize)) {
            guardedArchive->releaseUnpackSource(pState);
            return false;
        }

        SZDD_UNPACK_CONTEXT *pContext = new (std::nothrow) SZDD_UNPACK_CONTEXT;
        if (!pContext) {
            guardedArchive->releaseUnpackSource(pState);
            return false;
        }
        pContext->nHeaderSize = nHeaderSize;
        QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
        if (!guardedArchive || !guardedSource) {
            if (guardedArchive) guardedArchive->releaseUnpackSource(pState);
            delete pContext;
            return false;
        }
        pContext->sFileName = XBinary::getDeviceFileBaseName(guardedSource.data());
        pContext->nCompressedSize = nSize - pContext->nHeaderSize;
        pContext->nUncompressedSize = guardedArchive->read_uint32(nUncompressedOffset);
        if (!guardedArchive || !guardedSource) {
            if (guardedArchive) guardedArchive->releaseUnpackSource(pState);
            delete pContext;
            return false;
        }

        // Some SZDD variants expose unreliable size fields in tiny samples.
        // Keep metadata conservative instead of reporting absurd values.
        if ((pContext->nUncompressedSize <= 0) || (pContext->nUncompressedSize > 0x40000000) ||
            ((pContext->nCompressedSize > 0) && (pContext->nUncompressedSize > (pContext->nCompressedSize * 1024)))) {
            pContext->nUncompressedSize = 0;
        }

        pState->nCurrentOffset = 0;
        pState->nTotalSize = nSize;
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 1;
        pState->pContext = pContext;
        pState->mapUnpackProperties = mapProperties;

        bResult = guardedArchive->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
        if (!guardedArchive) return false;
        if (!bResult) {
            pState->pContext = nullptr;
            guardedArchive->releaseUnpackSource(pState);
            delete pContext;
            *pState = UNPACK_STATE();
        }
    }

    return bResult;
}

XBinary::ARCHIVERECORD XSZDD::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<XSZDD> guardedArchive(this);

    XBinary::ARCHIVERECORD result = {};

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive) {
        return result;
    }

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    SZDD_UNPACK_CONTEXT *pContext = reinterpret_cast<SZDD_UNPACK_CONTEXT *>(pState->pContext);

    result.nStreamOffset = pContext->nHeaderSize;
    result.nStreamSize = pContext->nCompressedSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZSS_SZDD);

    return result;
}

bool XSZDD::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XSZDD> guardedArchive(this);

    if (!pState || !pState->pContext || !pDevice) return false;
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
    if (!guardedOutput || !guardedSource || !guardedArchive->isUnpackOutputSupported(guardedOutput.data()) || !guardedArchive ||
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    SZDD_UNPACK_CONTEXT *pContext = reinterpret_cast<SZDD_UNPACK_CONTEXT *>(pState->pContext);

    qint64 nFileSize = guardedArchive->getSize();
    if (!guardedArchive || !guardedSource) return false;
    if ((pContext->nCompressedSize <= 0) || (nFileSize <= pContext->nHeaderSize)) {
        return false;
    }
    if ((pContext->nUncompressedSize < 0) || !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, pContext->nUncompressedSize)) {
        return false;
    }
    // This override bypasses the base decode chain's per-entry gate;
    // account the member here. Produced bytes are charged by
    // _writeDevice through state.spOutputBudget.
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, pContext->sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }
    std::unique_ptr<QIODevice> pStage(XBinary::createFileBuffer(pContext->nUncompressedSize, pPdStruct));
    if (!guardedArchive || !pStage || !guardedOutput || !guardedSource || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive) return false;

    SubDevice sd(guardedSource.data(), pContext->nHeaderSize, nFileSize - pContext->nHeaderSize);

    bool bResult = false;
    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZSS_SZDD);
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
        state.mapUnpackProperties = pState->mapUnpackProperties;
        state.spOutputBudget = pState->spOutputBudget;
        state.pDeviceInput = &sd;
        state.pDeviceOutput = pStage.get();
        state.nInputOffset = 0;
        state.nInputLimit = pContext->nCompressedSize;
        state.nProcessedOffset = 0;
        state.nProcessedLimit = pContext->nUncompressedSize > 0 ? pContext->nUncompressedSize : -1;

        bResult = XLZSSDecoder::decompress(&state, pPdStruct) && guardedArchive && guardedOutput && guardedSource && (state.nCountOutput == pContext->nUncompressedSize);

        sd.close();
    }

    return bResult && guardedArchive && guardedOutput && guardedSource && guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) && guardedArchive &&
           guardedArchive->publishUnpackOutput(pStage.get(), guardedOutput.data(), pState, pPdStruct);
}

bool XSZDD::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XSZDD> guardedArchive(this);

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    pState->nCurrentIndex++;

    return false;
}

bool XSZDD::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XSZDD> guardedArchive(this);

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState)) return false;

    SZDD_UNPACK_CONTEXT *pContext = static_cast<SZDD_UNPACK_CONTEXT *>(pState->pContext);
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

QList<QString> XSZDD::getSearchSignatures()
{
    return {"'SZDD'88F027'33'", "'SZDD'88F027'3A'", "'ZDD'88F027'33''A'", "'ZDD'88F027'3A''A'"};
}

XBinary *XSZDD::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XSZDD(pDevice);
}

bool XSZDD::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XSZDD> guardedThis(this);
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

void *XSZDD::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XSZDD> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XSZDD::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
