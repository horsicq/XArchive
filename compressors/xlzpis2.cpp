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
#include "xlzpis2.h"

#include <QFileInfo>

#include <memory>
#include <new>

#include "Algos/xlzpis2decoder.h"

static XBinary::XCONVERT _TABLE_XLZPIS2_STRUCTID[] = {{XLzpis2::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                      {XLzpis2::STRUCTID_LZPIS2_HEADER, "LZPIS2_HEADER", QString("LZPIS2_HEADER")},
                                                      {XLzpis2::STRUCTID_LZPIS2_CHUNK_HEADER, "LZPIS2_CHUNK_HEADER", QString("LZPIS2_CHUNK_HEADER")}};

static const qint64 LZPIS2_MAGIC_SIZE = 6;
static const qint64 LZPIS2_CHUNK_HEADER_SIZE = 4;
static const qint64 LZPIS2_MAX_CHUNK_SIZE = 0x1000;
static const qint32 LZPIS2_MAX_CHUNKS = 0x100000;

XLzpis2::XLzpis2(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XLzpis2::_scanChain(qint64 *pnUncompressedSize, qint32 *pnChunkCount, bool bTrialDecode, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedDevice = getDevice();
    if (!guardedDevice) return false;

    // Detection probes a device the caller still owns: remember where it was
    // and put it back before returning.
    const qint64 nSavedPosition = guardedDevice->pos();

    bool bResult = false;
    qint64 nTotalUncompressed = 0;
    qint32 nChunkCount = 0;

    const qint64 nSize = getSize();

    if (guardedDevice && (nSize >= (LZPIS2_MAGIC_SIZE + LZPIS2_CHUNK_HEADER_SIZE + 1))) {
        const QByteArray baMagic = read_array(0, (qint32)LZPIS2_MAGIC_SIZE);

        if (guardedDevice && (baMagic == QByteArray("LZPIS2", (qint32)LZPIS2_MAGIC_SIZE))) {
            qint64 nOffset = LZPIS2_MAGIC_SIZE;
            bool bChainOk = true;

            while (bChainOk && (nOffset < nSize)) {
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                    bChainOk = false;
                    break;
                }

                if ((nOffset + LZPIS2_CHUNK_HEADER_SIZE) > nSize) {
                    bChainOk = false;
                    break;
                }

                const qint64 nUnpacked = (qint64)read_uint16(nOffset);
                if (!guardedDevice) return false;
                const qint64 nPacked = (qint64)read_uint16(nOffset + 2);
                if (!guardedDevice) return false;

                if ((nUnpacked <= 0) || (nUnpacked > LZPIS2_MAX_CHUNK_SIZE) || (nPacked <= 0)) {
                    bChainOk = false;
                    break;
                }

                if ((nOffset + LZPIS2_CHUNK_HEADER_SIZE + nPacked) > nSize) {
                    bChainOk = false;
                    break;
                }

                if (bTrialDecode && (nChunkCount == 0)) {
                    const QByteArray baChunk = read_array(nOffset + LZPIS2_CHUNK_HEADER_SIZE, (qint32)nPacked);
                    if (!guardedDevice) return false;

                    if (baChunk.size() != (qint32)nPacked) {
                        bChainOk = false;
                        break;
                    }

                    QByteArray baPlain;
                    baPlain.resize((qint32)nUnpacked);

                    if (baPlain.size() != (qint32)nUnpacked) {
                        bChainOk = false;
                        break;
                    }

                    if (!XLzpis2Decoder::decodeChunk((const quint8 *)baChunk.constData(), nPacked, (quint8 *)baPlain.data(), nUnpacked)) {
                        bChainOk = false;
                        break;
                    }
                }

                nTotalUncompressed += nUnpacked;
                nChunkCount++;

                if (nChunkCount > LZPIS2_MAX_CHUNKS) {
                    bChainOk = false;
                    break;
                }

                nOffset += LZPIS2_CHUNK_HEADER_SIZE + nPacked;
            }

            // The chain has to tile the container exactly: no slack, no overlay.
            bResult = bChainOk && (nOffset == nSize) && (nChunkCount > 0) && (nTotalUncompressed > 0);
        }
    }

    if (guardedDevice) guardedDevice->seek(nSavedPosition);

    if (!guardedDevice) return false;

    if (bResult) {
        if (pnUncompressedSize) *pnUncompressedSize = nTotalUncompressed;
        if (pnChunkCount) *pnChunkCount = nChunkCount;
    }

    return bResult;
}

bool XLzpis2::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLzpis2 xlzpis2(pDevice);

    return xlzpis2.isValid(pPdStruct);
}

bool XLzpis2::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    return _scanChain(nullptr, nullptr, true, pPdStruct);
}

XBinary::FT XLzpis2::getFileType()
{
    return XBinary::FT_LZPIS2;
}

XBinary::MODE XLzpis2::getMode()
{
    return XBinary::MODE_DATA;
}

QString XLzpis2::getMIMEString()
{
    return "application/x-lzpis2";
}

qint32 XLzpis2::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

XBinary::ENDIAN XLzpis2::getEndian()
{
    return XBinary::ENDIAN_LITTLE;
}

QString XLzpis2::getArch()
{
    return QString();
}

QString XLzpis2::getFileFormatExt()
{
    return QStringLiteral("lzpis2");
}

QString XLzpis2::getFileFormatExtsString()
{
    return QStringLiteral("LZPIS2 compressed file (*.??$)");
}

qint64 XLzpis2::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

bool XLzpis2::isSigned()
{
    return false;
}

XBinary::OSNAME XLzpis2::getOsName()
{
    return XBinary::OSNAME_MSDOS;
}

QString XLzpis2::getOsVersion()
{
    return QString();
}

QString XLzpis2::getVersion()
{
    return QString("2");
}

bool XLzpis2::isEncrypted()
{
    return false;
}

QList<XBinary::MAPMODE> XLzpis2::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XLzpis2::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.mode = getMode();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());
    result.sArch = getArch();
    result.nBinarySize = getSize();

    qint64 nUncompressedSize = 0;
    qint32 nChunkCount = 0;

    if (!_scanChain(&nUncompressedSize, &nChunkCount, false, pPdStruct)) {
        return result;
    }

    const qint64 nTotalSize = getSize();
    qint32 nIndex = 0;

    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = XADDR_MAX;
    recHeader.nOffset = 0;
    recHeader.nSize = LZPIS2_MAGIC_SIZE;
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = QString("LZPIS2 ") + tr("Header");
    result.listRecords.append(recHeader);

    if (nTotalSize > LZPIS2_MAGIC_SIZE) {
        _MEMORY_RECORD recData = {};
        recData.nAddress = XADDR_MAX;
        recData.nOffset = LZPIS2_MAGIC_SIZE;
        recData.nSize = nTotalSize - LZPIS2_MAGIC_SIZE;
        recData.nIndex = nIndex++;
        recData.filePart = FILEPART_REGION;
        recData.sName = tr("Compressed Data");
        result.listRecords.append(recData);
    }

    _handleOverlay(&result);

    return result;
}

QString XLzpis2::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XLZPIS2_STRUCTID, sizeof(_TABLE_XLZPIS2_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XLzpis2::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XLZPIS2_STRUCTID, sizeof(_TABLE_XLZPIS2_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XLzpis2::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XLZPIS2_STRUCTID, sizeof(_TABLE_XLZPIS2_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XLzpis2::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    const quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_LZPIS2_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));

        if (getSize() >= (LZPIS2_MAGIC_SIZE + LZPIS2_CHUNK_HEADER_SIZE)) {
            XFSTRUCT _xfChunk = xfStruct;
            _xfChunk.nStructID = STRUCTID_LZPIS2_CHUNK_HEADER;
            _xfChunk.xLoc = offsetToLoc(LZPIS2_MAGIC_SIZE);
            listResult.append(getXFHeaders(_xfChunk, pPdStruct));
        }
    } else if (nStructID == STRUCTID_LZPIS2_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_LZPIS2_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = sizeof(LZPIS2_HEADER);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_LZPIS2_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_LZPIS2_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    } else if (nStructID == STRUCTID_LZPIS2_CHUNK_HEADER) {
        XLOC chunkLoc = xfStruct.xLoc;
        if (chunkLoc.locType == LT_UNKNOWN) {
            chunkLoc = offsetToLoc(LZPIS2_MAGIC_SIZE);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_LZPIS2_CHUNK_HEADER);
        xfHeader.xLoc = chunkLoc;
        xfHeader.nSize = sizeof(LZPIS2_CHUNK_HEADER);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_LZPIS2_CHUNK_HEADER, chunkLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_LZPIS2_CHUNK_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XLzpis2::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_LZPIS2_HEADER) {
        listResult.append({"signature", (qint32)offsetof(LZPIS2_HEADER, signature), 6, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
    } else if (nStructID == STRUCTID_LZPIS2_CHUNK_HEADER) {
        listResult.append({"uncompressed_size", (qint32)offsetof(LZPIS2_CHUNK_HEADER, uncompressed_size), 2, XFRECORD_FLAG_SIZE, VT_UINT16});
        listResult.append({"compressed_size", (qint32)offsetof(LZPIS2_CHUNK_HEADER, compressed_size), 2, XFRECORD_FLAG_SIZE, VT_UINT16});
    }

    return listResult;
}

static bool lzpis2CanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XLzpis2::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    if (nFileParts == 0) {
        return listResult;
    }

    qint64 nUncompressedSize = 0;
    qint32 nChunkCount = 0;

    if (!_scanChain(&nUncompressedSize, &nChunkCount, false, pPdStruct)) {
        return listResult;
    }

    const qint64 nTotalSize = getSize();

    if ((nFileParts & FILEPART_HEADER) && lzpis2CanAppend(nLimit, listResult)) {
        FPART record = {};

        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = LZPIS2_MAGIC_SIZE;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");

        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && lzpis2CanAppend(nLimit, listResult)) {
        // The decoder walks the chunk chain itself and needs the magic in
        // front of it, so the data part is the whole container.
        FPART record = {};

        record.filePart = FILEPART_REGION;
        record.nFileOffset = 0;
        record.nFileSize = nTotalSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Compressed Data");
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZPIS2);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nTotalSize);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);

        listResult.append(record);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XLzpis2::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XLzpis2::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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

        qint64 nUncompressedSize = 0;
        qint32 nChunkCount = 0;

        const bool bValid = _scanChain(&nUncompressedSize, &nChunkCount, false, pPdStruct);
        if (!bValid) {
            releaseUnpackSource(pState);
            return false;
        }

        const qint64 nSize = getSize();
        LZPIS2_UNPACK_CONTEXT *pContext = new (std::nothrow) LZPIS2_UNPACK_CONTEXT;
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
            // The container stores no name.  The packer overwrote the LAST
            // character of the original extension with '$' and kept no copy of
            // it, so the original name is unrecoverable: keep the container's
            // own name, exactly as the reference extractor does.
            sName = QFileInfo(sName).fileName();
        }
        if (sName.isEmpty()) sName = QStringLiteral("lzpis2_data");

        pContext->sFileName = sName;
        pContext->nTotalSize = nSize;
        pContext->nUncompressedSize = nUncompressedSize;
        pContext->nChunkCount = nChunkCount;

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

XBinary::ARCHIVERECORD XLzpis2::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

    LZPIS2_UNPACK_CONTEXT *pContext = reinterpret_cast<LZPIS2_UNPACK_CONTEXT *>(pState->pContext);

    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nTotalSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nTotalSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZPIS2);

    return result;
}

bool XLzpis2::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
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

    LZPIS2_UNPACK_CONTEXT *pContext = reinterpret_cast<LZPIS2_UNPACK_CONTEXT *>(pState->pContext);

    const qint64 nFileSize = getSize();
    if (!guardedSource) return false;

    if ((nFileSize != pContext->nTotalSize) || (nFileSize <= LZPIS2_MAGIC_SIZE)) {
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
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZPIS2);
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
        state.mapUnpackProperties = pState->mapUnpackProperties;
        state.spOutputBudget = pState->spOutputBudget;
        state.pDeviceInput = &sd;
        state.pDeviceOutput = pStage.get();
        state.nInputOffset = 0;
        state.nInputLimit = nFileSize;
        state.nProcessedOffset = 0;
        state.nProcessedLimit = pContext->nUncompressedSize;

        bResult = XLzpis2Decoder::decompress(&state, pPdStruct) && guardedOutput && guardedSource &&
                  (state.nCountOutput == pContext->nUncompressedSize);

        sd.close();
    }

    return bResult && guardedOutput && guardedSource && isUnpackSourceCurrent(pState, pPdStruct) && publishUnpackOutput(pStage.get(), guardedOutput, pState, pPdStruct);
}

bool XLzpis2::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    pState->nCurrentIndex++;

    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XLzpis2::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    LZPIS2_UNPACK_CONTEXT *pContext = static_cast<LZPIS2_UNPACK_CONTEXT *>(pState->pContext);
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

QList<QString> XLzpis2::getSearchSignatures()
{
    return {"'LZPIS2'"};
}

XBinary *XLzpis2::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XLzpis2(pDevice);
}

bool XLzpis2::handleInternalInfo(PDSTRUCT *pPdStruct)
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

void *XLzpis2::getInternalInfo(PDSTRUCT *pPdStruct)
{
    const bool bHandled = handleInternalInfo(pPdStruct);
    if (!bHandled) return nullptr;

    return &m_internalInfo;
}

void XLzpis2::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
