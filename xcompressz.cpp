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
#include "xcompressz.h"
#include "Algos/xcompressdecoder.h"

#include <memory>
#include <new>

namespace {
class CompressZDiscardDevice : public QIODevice {
protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *, qint64 nSize) override { return nSize; }
};
}  // namespace

XBinary::XCONVERT _TABLE_XCompressZ_STRUCTID[] = {{XCompressZ::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                  {XCompressZ::STRUCTID_COMPRESSZ_HEADER, "COMPRESSZ_HEADER", QString("Compress (.Z) header")}};

XCompressZ::XCompressZ(QIODevice *pDevice) : XArchive(pDevice)
{
}

XCompressZ::~XCompressZ()
{
}

bool XCompressZ::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XCompressZ> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    const bool bResult = isValid(guardedSource.data(), pPdStruct);
    return guardedThis && bResult;
}

bool XCompressZ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;
    QPointer<QIODevice> guardedDevice(pDevice);

    if (XBinary::isPdStructNotCanceled(pPdStruct) && guardedDevice &&
        guardedDevice->seek(0) && guardedDevice) {
        quint8 header[3];
        if (guardedDevice->read((char *)header, sizeof(header)) == sizeof(header) &&
            guardedDevice) {
            const qint32 nMaxBits = header[2] & 0x1f;
            bResult = (header[0] == 0x1f) && (header[1] == 0x9d) && ((header[2] & 0x60) == 0) &&
                      (nMaxBits >= 9) && (nMaxBits <= 16);
        }
    }

    return bResult;
}

XBinary::MODE XCompressZ::getMode()
{
    return MODE_DATA;
}

qint32 XCompressZ::getType()
{
    return TYPE_Z;
}

XBinary::ENDIAN XCompressZ::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XCompressZ::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_Z: sResult = QString("Z"); break;
    }

    return sResult;
}

QString XCompressZ::getFileFormatExt()
{
    return "Z";
}

XBinary::FT XCompressZ::getFileType()
{
    return FT_COMPRESS;
}

QString XCompressZ::getFileFormatExtsString()
{
    return "Z";
}

qint64 XCompressZ::getFileFormatSize(XBinary::PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QString XCompressZ::getMIMEString()
{
    return "application/x-compress";
}

XBinary::OSNAME XCompressZ::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QList<XBinary::MAPMODE> XCompressZ::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XCompressZ::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XCompressZ::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XCompressZ_STRUCTID, sizeof(_TABLE_XCompressZ_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XCompressZ::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XCompressZ_STRUCTID, sizeof(_TABLE_XCompressZ_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XCompressZ::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XCompressZ_STRUCTID, sizeof(_TABLE_XCompressZ_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// QList<XBinary::DATA_HEADER> XCompressZ::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
//         _dataHeadersOptions.nID = STRUCTID_COMPRESSZ_HEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         if (isPdStructNotCanceled(pPdStruct)) {
//             listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//         }
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             if (dataHeadersOptions.nID == STRUCTID_COMPRESSZ_HEADER) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCompressZ::structIDToString(dataHeadersOptions.nID));
//                 dataHeader.nSize = sizeof(COMPRESSZ_HEADER);

//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(COMPRESSZ_HEADER, nMagic0), 1, "nMagic0", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(COMPRESSZ_HEADER, nMagic1), 1, "nMagic1", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(COMPRESSZ_HEADER, nFlags), 1, "nFlags", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);
//             }
//         }
//     }

//     return listResult;
// }

QList<XBinary::XFHEADER> XCompressZ::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XBinary::XFHEADER> listResult;
    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_COMPRESSZ_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_COMPRESSZ_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if ((nHeaderOffset != -1) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, sizeof(COMPRESSZ_HEADER))) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_COMPRESSZ_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = sizeof(COMPRESSZ_HEADER);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_COMPRESSZ_HEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_COMPRESSZ_HEADER), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XCompressZ::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_COMPRESSZ_HEADER) {
        listResult.append({"nMagic0", (qint32)offsetof(COMPRESSZ_HEADER, nMagic0), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"nMagic1", (qint32)offsetof(COMPRESSZ_HEADER, nMagic1), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"nFlags", (qint32)offsetof(COMPRESSZ_HEADER, nFlags), 1, XFRECORD_FLAG_NONE, VT_UINT8});
    }

    return listResult;
}

static bool compresszCanAppendPart(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XCompressZ::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    const qint64 nFileSize = getSize();
    if (nFileSize <= 0) return listResult;

    if ((nFileParts & FILEPART_HEADER) && compresszCanAppendPart(nLimit, listResult)) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = qMin<qint64>(3, nFileSize);
        header.nVirtualAddress = XADDR_MAX;
        header.sName = tr("Header");
        listResult.append(header);
    }

    qint64 nMaxOffset = 0;

    {
        SubDevice sd(getDevice(), 0, nFileSize);

        if (sd.open(QIODevice::ReadOnly)) {
            CompressZDiscardDevice output;
            if (output.open(QIODevice::WriteOnly)) {
                XBinary::DATAPROCESS_STATE decompressState = {};
                decompressState.pDeviceInput = &sd;
                decompressState.pDeviceOutput = &output;
                decompressState.nInputOffset = 0;
                decompressState.nInputLimit = nFileSize;

                if (XCompressDecoder::decompress(&decompressState, pPdStruct)) {
                    nMaxOffset = decompressState.nCountInput;

                    if ((nFileParts & FILEPART_STREAM) && compresszCanAppendPart(nLimit, listResult)) {
                        FPART region = {};
                        region.filePart = FILEPART_STREAM;
                        region.nFileOffset = 0;
                        region.nFileSize = nMaxOffset;
                        region.nVirtualAddress = XADDR_MAX;
                        region.sName = tr("Stream");
                        region.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_COMPRESS);
                        region.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, decompressState.nCountOutput);
                        region.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, decompressState.nCountInput);

                        listResult.append(region);
                    }
                }

                output.close();
            }

            sd.close();
        }
    }

    if ((nFileParts & FILEPART_DATA) && compresszCanAppendPart(nLimit, listResult)) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = 0;
        data.nFileSize = nMaxOffset;
        data.nVirtualAddress = XADDR_MAX;
        data.sName = tr("Data");
        listResult.append(data);
    }

    if ((nFileParts & FILEPART_OVERLAY) && compresszCanAppendPart(nLimit, listResult)) {
        if (nMaxOffset < nFileSize) {
            FPART ov = {};
            ov.filePart = FILEPART_OVERLAY;
            ov.nFileOffset = nMaxOffset;
            ov.nFileSize = nFileSize - nMaxOffset;
            ov.nVirtualAddress = XADDR_MAX;
            ov.sName = tr("Overlay");
            listResult.append(ov);
        }
    }

    return listResult;
}

XCompressZ::COMPRESSZ_HEADER XCompressZ::_read_COMPRESSZ_HEADER(qint64 nOffset)
{
    COMPRESSZ_HEADER result = {};

    result.nMagic0 = read_uint8(nOffset + 0);
    result.nMagic1 = read_uint8(nOffset + 1);
    result.nFlags = read_uint8(nOffset + 2);

    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XCompressZ::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XCompressZ::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XCompressZ> guardedThis(this);
    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (!pState || m_bUnpackOperationInProgress ||
        ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState))) {
        return false;
    }

    const bool bFinished = finishUnpack(pState, pPdStruct);
    if (!guardedThis || !bFinished) {
        return false;
    }
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

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
    bool bDecompressed = false;
    SubDevice sd(getDevice(), 0, nFileSize);

    if (sd.open(QIODevice::ReadOnly)) {
        CompressZDiscardDevice output;
        if (output.open(QIODevice::WriteOnly)) {
            XBinary::DATAPROCESS_STATE decompressState = {};
            decompressState.mapUnpackProperties = mapProperties;
            decompressState.pDeviceInput = &sd;
            decompressState.pDeviceOutput = &output;
            decompressState.nInputOffset = 0;
            decompressState.nInputLimit = nFileSize;
            decompressState.nProcessedLimit = -1;

            bDecompressed = XCompressDecoder::decompress(&decompressState, pPdStruct);
            if (!guardedThis) return false;
            if (bDecompressed) {
                nCompressedSize = decompressState.nCountInput;
                nUncompressedSize = decompressState.nCountOutput;
            }

            output.close();
        }

        sd.close();
    }

    if (!bDecompressed) {
        releaseUnpackSource(pState);
        return false;
    }

    COMPRESSZ_UNPACK_CONTEXT *pContext = new (std::nothrow) COMPRESSZ_UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    pContext->nCompressedSize = nCompressedSize;
    pContext->nUncompressedSize = nUncompressedSize;
    pContext->sFileName = XBinary::getDeviceFileBaseName(getDevice());

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

XBinary::ARCHIVERECORD XCompressZ::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XCompressZ> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();

    XBinary::ARCHIVERECORD result = {};

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || !pState ||
        !pState->pContext) {
        return result;
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return result;

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    COMPRESSZ_UNPACK_CONTEXT *pContext = (COMPRESSZ_UNPACK_CONTEXT *)pState->pContext;

    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nCompressedSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_COMPRESS);

    return result;
}

bool XCompressZ::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XCompressZ> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState || !pState->pContext || !pDevice) return false;
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedOutput || !guardedSource ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const bool bOutputSupported =
        isUnpackOutputSupported(guardedOutput.data());
    if (!guardedThis || !guardedOutput || !bOutputSupported) return false;
    const bool bAliases =
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data());
    if (!guardedThis || !guardedSource || !guardedOutput || bAliases)
        return false;
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return false;

    if ((pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    COMPRESSZ_UNPACK_CONTEXT *pContext =
        static_cast<COMPRESSZ_UNPACK_CONTEXT *>(pState->pContext);
    const qint64 nFileSize = getSize();
    if (!guardedThis) return false;
    if ((nFileSize < 0) || (pContext->nUncompressedSize < 0) ||
        !guardedSource ||
        !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                            pContext->nUncompressedSize)) {
        return false;
    }
    // This override bypasses the base decode chain's per-entry gate;
    // account the member here. Produced bytes are charged by
    // _writeDevice through decompressState.spOutputBudget.
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex,
                                                pContext->sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(
                    pPdStruct,
                    tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }
    std::unique_ptr<QIODevice> pStage(XBinary::createFileBuffer(
        pContext->nUncompressedSize, pPdStruct));
    if (!guardedThis || !pStage || !guardedSource || !guardedOutput)
        return false;
    const bool bStageSourceCurrent =
        isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bStageSourceCurrent) return false;

    SubDevice sd(guardedSource.data(), 0, nFileSize);
    bool bResult = false;

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE decompressState = {};
        decompressState.mapUnpackProperties = pState->mapUnpackProperties;
        decompressState.spOutputBudget = pState->spOutputBudget;
        decompressState.pDeviceInput = &sd;
        decompressState.pDeviceOutput = pStage.get();
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = nFileSize;
        decompressState.nProcessedLimit = -1;

        bResult = XCompressDecoder::decompress(&decompressState, pPdStruct);
        if (!guardedThis) return false;
        bResult = bResult && guardedOutput && guardedSource &&
                  (decompressState.nCountOutput == pContext->nUncompressedSize);

        sd.close();
    }

    if (!bResult || !guardedOutput || !guardedSource) return false;
    const bool bFinalSourceCurrent =
        isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bFinalSourceCurrent || !guardedOutput ||
        !guardedSource) return false;
    const bool bPublished = publishUnpackOutput(
        pStage.get(), guardedOutput.data(), pState, pPdStruct);
    return guardedThis && bPublished;
}

bool XCompressZ::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XCompressZ> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    bool bResult = false;

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || !pState ||
        !pState->pContext) {
        return false;
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;

    pState->nCurrentIndex++;

    bResult = false;

    return bResult;
}

bool XCompressZ::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XCompressZ> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    COMPRESSZ_UNPACK_CONTEXT *pContext =
        static_cast<COMPRESSZ_UNPACK_CONTEXT *>(pState->pContext);
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

QList<XBinary::FPART_PROP> XCompressZ::getAvailableFPARTProperties()
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

QList<QString> XCompressZ::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("1F9D");

    return listResult;
}

XBinary *XCompressZ::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XCompressZ(pDevice);
}

bool XCompressZ::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XCompressZ> guardedThis(this);
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

void *XCompressZ::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XCompressZ> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XCompressZ::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
