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
#include "xgashuff.h"

#include <QFileInfo>

#include <memory>
#include <new>

#include "Algos/xgashuffdecoder.h"

static XBinary::XCONVERT _TABLE_XGASHUFF_STRUCTID[] = {{XGasHuff::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                       {XGasHuff::STRUCTID_GAS_HEADER, "GAS_HEADER", QString("GAS_HEADER")}};

static const qint64 GAS_HEADER_SIZE = 8;
// isValid() must not slurp an arbitrarily large candidate: the node table plus
// the trial-decode window never reach beyond this prefix.
static const qint64 GAS_PROBE_READ_SIZE = 0x4000;

XGasHuff::XGasHuff(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XGasHuff::_readAndCheckHeader(quint32 *pnUncompressedSize, quint32 *pnNodeCount, quint32 *pnRootIndex, qint64 *pnTreeEndOffset, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    QIODevice *guardedDevice = getDevice();
    if (!guardedDevice) return false;

    // Detection probes a device the caller still owns: remember where it was
    // and put it back before returning.
    const qint64 nSavedPosition = guardedDevice->pos();

    const qint64 nSize = getSize();
    if (!guardedDevice) return false;

    bool bResult = false;

    if ((nSize > (GAS_HEADER_SIZE + 2)) && (nSize <= XGasHuffDecoder::GAS_MAX_INPUT_SIZE)) {
        const qint64 nProbeSize = qMin<qint64>(nSize, GAS_PROBE_READ_SIZE);
        const QByteArray baProbe = read_array(0, (qint32)nProbeSize);

        if (guardedDevice && (baProbe.size() == (qint32)nProbeSize)) {
            quint32 nUncompressedSize = 0;
            quint32 nNodeCount = 0;
            quint32 nRootIndex = 0;
            qint64 nPayloadBitOffset = 0;

            // The probe may be a prefix of the container; checkStream() takes
            // the real container length separately so that every size relation
            // is still tested against the whole file.
            bResult = XGasHuffDecoder::checkStream(baProbe.constData(), baProbe.size(), nSize, &nUncompressedSize, &nNodeCount, &nRootIndex, nullptr,
                                                   &nPayloadBitOffset);

            if (bResult) {
                if (pnUncompressedSize) *pnUncompressedSize = nUncompressedSize;
                if (pnNodeCount) *pnNodeCount = nNodeCount;
                if (pnRootIndex) *pnRootIndex = nRootIndex;
                if (pnTreeEndOffset) *pnTreeEndOffset = (nPayloadBitOffset + 7) / 8;
            }
        }
    }

    if (guardedDevice) guardedDevice->seek(nSavedPosition);

    return guardedDevice && bResult;
}

bool XGasHuff::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XGasHuff xgashuff(pDevice);

    return xgashuff.isValid(pPdStruct);
}

bool XGasHuff::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    return _readAndCheckHeader(nullptr, nullptr, nullptr, nullptr, pPdStruct);
}

XGasHuff::GAS_HEADER XGasHuff::_read_GAS_HEADER(qint64 nOffset)
{
    GAS_HEADER header = {};
    header.uncompressed_size = read_uint32(nOffset + 0, false);
    header.node_count = read_uint16(nOffset + 4, false);
    header.root_index = read_uint16(nOffset + 6, false);
    return header;
}

XBinary::FT XGasHuff::getFileType()
{
    return XBinary::FT_GAS_HUFF;
}

XBinary::MODE XGasHuff::getMode()
{
    return XBinary::MODE_DATA;
}

QString XGasHuff::getMIMEString()
{
    return "application/x-gas-huff";
}

qint32 XGasHuff::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

XBinary::ENDIAN XGasHuff::getEndian()
{
    return XBinary::ENDIAN_LITTLE;
}

QString XGasHuff::getArch()
{
    return QString();
}

QString XGasHuff::getFileFormatExt()
{
    return "huf";
}

QString XGasHuff::getFileFormatExtsString()
{
    return "GAS Huffman (*.huf *.hum)";
}

qint64 XGasHuff::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

bool XGasHuff::isSigned()
{
    return false;
}

XBinary::OSNAME XGasHuff::getOsName()
{
    return XBinary::OSNAME_MULTIPLATFORM;
}

QString XGasHuff::getOsVersion()
{
    return QString();
}

QString XGasHuff::getVersion()
{
    return QString();
}

bool XGasHuff::isEncrypted()
{
    return false;
}

QList<XBinary::MAPMODE> XGasHuff::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XGasHuff::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.mode = getMode();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());
    result.sArch = getArch();
    result.nBinarySize = getSize();

    quint32 nUncompressedSize = 0;
    quint32 nNodeCount = 0;
    quint32 nRootIndex = 0;
    qint64 nTreeEndOffset = 0;

    if (!_readAndCheckHeader(&nUncompressedSize, &nNodeCount, &nRootIndex, &nTreeEndOffset, pPdStruct)) {
        return result;
    }

    const qint64 nTotalSize = getSize();
    qint32 nIndex = 0;

    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = XADDR_MAX;
    recHeader.nOffset = 0;
    recHeader.nSize = GAS_HEADER_SIZE;
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = tr("Header");
    result.listRecords.append(recHeader);

    if ((nTreeEndOffset > GAS_HEADER_SIZE) && (nTreeEndOffset <= nTotalSize)) {
        _MEMORY_RECORD recTree = {};
        recTree.nAddress = XADDR_MAX;
        recTree.nOffset = GAS_HEADER_SIZE;
        recTree.nSize = nTreeEndOffset - GAS_HEADER_SIZE;
        recTree.nIndex = nIndex++;
        recTree.filePart = FILEPART_TABLE;
        recTree.sName = tr("Table");
        result.listRecords.append(recTree);
    } else {
        nTreeEndOffset = GAS_HEADER_SIZE;
    }

    if (nTotalSize > nTreeEndOffset) {
        _MEMORY_RECORD recData = {};
        recData.nAddress = XADDR_MAX;
        recData.nOffset = nTreeEndOffset;
        recData.nSize = nTotalSize - nTreeEndOffset;
        recData.nIndex = nIndex++;
        recData.filePart = FILEPART_REGION;
        recData.sName = tr("Compressed Data");
        result.listRecords.append(recData);
    }

    _handleOverlay(&result);

    return result;
}

QString XGasHuff::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XGASHUFF_STRUCTID, sizeof(_TABLE_XGASHUFF_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XGasHuff::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XGASHUFF_STRUCTID, sizeof(_TABLE_XGASHUFF_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XGasHuff::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XGASHUFF_STRUCTID, sizeof(_TABLE_XGASHUFF_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XGasHuff::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    const quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_GAS_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_GAS_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_GAS_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = sizeof(GAS_HEADER);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_GAS_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_GAS_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XGasHuff::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_GAS_HEADER) {
        listResult.append({"uncompressed_size", (qint32)offsetof(GAS_HEADER, uncompressed_size), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"node_count", (qint32)offsetof(GAS_HEADER, node_count), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"root_index", (qint32)offsetof(GAS_HEADER, root_index), 2, XFRECORD_FLAG_NONE, VT_UINT16});
    }

    return listResult;
}

static bool gasCanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XGasHuff::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    if (nFileParts == 0) {
        return listResult;
    }

    quint32 nUncompressedSize = 0;
    quint32 nNodeCount = 0;
    quint32 nRootIndex = 0;
    qint64 nTreeEndOffset = 0;

    if (!_readAndCheckHeader(&nUncompressedSize, &nNodeCount, &nRootIndex, &nTreeEndOffset, pPdStruct)) {
        return listResult;
    }

    const qint64 nTotalSize = getSize();

    if ((nFileParts & FILEPART_HEADER) && gasCanAppend(nLimit, listResult)) {
        FPART record = {};

        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = GAS_HEADER_SIZE;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");

        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && gasCanAppend(nLimit, listResult)) {
        // The decoder needs the 8-byte header (uncompressed size, node count,
        // root index) as well as the node table, so the data part is the whole
        // container.
        FPART record = {};

        record.filePart = FILEPART_REGION;
        record.nFileOffset = 0;
        record.nFileSize = nTotalSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Compressed Data");
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_GAS_HUFF);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nTotalSize);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)nUncompressedSize);

        listResult.append(record);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XGasHuff::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XGasHuff::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState && !m_bUnpackOperationInProgress && ((!pState->pContext && pState->baUnpackSourceToken.isEmpty()) || ownsUnpackSource(pState))) {
        if (!finishUnpack(pState, nullptr)) return false;
        UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
        if (!operationGuard.isAcquired()) return false;

        if (!isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        const bool bBound = bindUnpackSource(pState, pPdStruct);
        if (!bBound) return false;

        quint32 nUncompressedSize = 0;
        quint32 nNodeCount = 0;
        quint32 nRootIndex = 0;
        qint64 nTreeEndOffset = 0;

        const bool bValid = _readAndCheckHeader(&nUncompressedSize, &nNodeCount, &nRootIndex, &nTreeEndOffset, pPdStruct);
        if (!bValid) {
            releaseUnpackSource(pState);
            return false;
        }

        const qint64 nSize = getSize();
        GAS_UNPACK_CONTEXT *pContext = new (std::nothrow) GAS_UNPACK_CONTEXT;
        if (!pContext) {
            releaseUnpackSource(pState);
            return false;
        }

        QIODevice *guardedSource = getDevice();
        if (!guardedSource) {
            releaseUnpackSource(pState);
            delete pContext;
            return false;
        }

        QString sName = XBinary::getDeviceFileName(guardedSource);
        if (!guardedSource) {
            releaseUnpackSource(pState);
            delete pContext;
            return false;
        }
        if (!sName.isEmpty()) {
            // The container stores no name; the payload keeps the original file
            // name and extension, so hand the archive's own name through.
            sName = QFileInfo(sName).fileName();
        }
        if (sName.isEmpty()) sName = QStringLiteral("gas_data");

        pContext->sFileName = sName;
        pContext->nTotalSize = nSize;
        pContext->nUncompressedSize = (qint64)nUncompressedSize;

        pState->nCurrentOffset = 0;
        pState->nTotalSize = nSize;
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 1;
        pState->pContext = pContext;
        pState->mapUnpackProperties = mapProperties;

        bResult = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
        if (!bResult) {
            pState->pContext = nullptr;
            releaseUnpackSource(pState);
            delete pContext;
            *pState = UNPACK_STATE();
        }
    }

    return bResult;
}

XBinary::ARCHIVERECORD XGasHuff::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    XBinary::ARCHIVERECORD result = {};

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct)) {
        return result;
    }

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    GAS_UNPACK_CONTEXT *pContext = reinterpret_cast<GAS_UNPACK_CONTEXT *>(pState->pContext);

    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nTotalSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nTotalSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_GAS_HUFF);

    return result;
}

bool XGasHuff::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    if (!pState || !pState->pContext || !pDevice) return false;
    QIODevice *guardedOutput = pDevice;
    QIODevice *guardedSource = getDevice();
    if (!guardedOutput || !guardedSource || !isUnpackOutputSupported(guardedOutput) || XBinary::devicesAlias(guardedSource, guardedOutput) || !isUnpackSourceCurrent(pState, pPdStruct) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    GAS_UNPACK_CONTEXT *pContext = reinterpret_cast<GAS_UNPACK_CONTEXT *>(pState->pContext);

    const qint64 nFileSize = getSize();
    if (!guardedSource) return false;

    if ((nFileSize != pContext->nTotalSize) || (nFileSize <= GAS_HEADER_SIZE)) {
        return false;
    }

    if ((pContext->nUncompressedSize <= 0) || !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, pContext->nUncompressedSize)) {
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
    if (!pStage || !guardedOutput || !guardedSource || !isUnpackSourceCurrent(pState, pPdStruct)) return false;

    SubDevice sd(guardedSource, 0, nFileSize);

    bool bResult = false;

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_GAS_HUFF);
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
        state.mapUnpackProperties = pState->mapUnpackProperties;
        state.spOutputBudget = pState->spOutputBudget;
        state.pDeviceInput = &sd;
        state.pDeviceOutput = pStage.get();
        state.nInputOffset = 0;
        state.nInputLimit = nFileSize;
        state.nProcessedOffset = 0;
        state.nProcessedLimit = pContext->nUncompressedSize;

        bResult = XGasHuffDecoder::decompress(&state, pPdStruct) && guardedOutput && guardedSource &&
                  (state.nCountOutput == pContext->nUncompressedSize);

        sd.close();
    }

    return bResult && guardedOutput && guardedSource && isUnpackSourceCurrent(pState, pPdStruct) && publishUnpackOutput(pStage.get(), guardedOutput, pState, pPdStruct);
}

bool XGasHuff::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    pState->nCurrentIndex++;

    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XGasHuff::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    GAS_UNPACK_CONTEXT *pContext = static_cast<GAS_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    return true;
}

QList<QString> XGasHuff::getSearchSignatures()
{
    // Headerless: no magic to scan for.
    return QList<QString>();
}

XBinary *XGasHuff::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XGasHuff(pDevice);
}

bool XGasHuff::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XArchive::handleInternalInfo(pPdStruct);
        if (!bResult) return false;
        XArchive::INTERNAL_INFO *pInfo = static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
        if (!pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) = *pInfo;
    }

    return bResult;
}

void *XGasHuff::getInternalInfo(PDSTRUCT *pPdStruct)
{
    const bool bHandled = handleInternalInfo(pPdStruct);
    if (!bHandled) return nullptr;

    return &m_internalInfo;
}

void XGasHuff::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
