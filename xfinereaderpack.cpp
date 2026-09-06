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
#include "xfinereaderpack.h"

#include <QFileInfo>

#include <memory>
#include <new>

#include "Algos/xlzhdecoder.h"

static XBinary::XCONVERT _TABLE_XFINEREADERPACK_STRUCTID[] = {{XFineReaderPack::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                              {XFineReaderPack::STRUCTID_FINEAR_HEADER, "FINEAR_HEADER", QString("FINEAR_HEADER")}};

static const qint64 FINEAR_HEADER_SIZE = 17;
// The member is decoded through the -lh1- whole-buffer path, which itself
// refuses anything above 256 MiB. Reject absurd sizes before that so a random
// 17-byte prefix match cannot make the reader reserve memory.
static const qint64 FINEAR_MAX_UNPACKED_SIZE = 256LL * 1024 * 1024;

XFineReaderPack::XFineReaderPack(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XFineReaderPack::_readAndCheckHeader(quint32 *pnUnpackedSize, quint32 *pnCrc, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QPointer<XFineReaderPack> guardedArchive(this);

    QPointer<QIODevice> guardedDevice(guardedArchive->getDevice());
    if (!guardedArchive || !guardedDevice) return false;

    // Detection probes a device the caller still owns: remember where it was
    // and put it back before returning.
    const qint64 nSavedPosition = guardedDevice->pos();

    const qint64 nSize = guardedArchive->getSize();
    if (!guardedArchive || !guardedDevice) return false;

    bool bResult = false;

    // A 17-byte header plus at least one byte of coded stream.
    if (nSize > FINEAR_HEADER_SIZE) {
        const QByteArray baHeader = guardedArchive->read_array(0, (qint32)FINEAR_HEADER_SIZE);

        if (guardedArchive && guardedDevice && (baHeader.size() == (qint32)FINEAR_HEADER_SIZE)) {
            const quint8 *pHeader = reinterpret_cast<const quint8 *>(baHeader.constData());

            // 72 bits of fixed signature: "FINEAR" DD 88 DD.
            if ((memcmp(pHeader, "FINEAR", 6) == 0) && (pHeader[6] == 0xddU) && (pHeader[7] == 0x88U) && (pHeader[8] == 0xddU)) {
                const quint32 nCrc = qFromLittleEndian<quint32>(pHeader + 9);
                const quint32 nUnpackedSize = qFromLittleEndian<quint32>(pHeader + 13);

                // The stored CRC is a CRC-16/ARC widened to 32 bits, so the
                // high half is always zero; and an empty member is never
                // written by this packer.
                if ((nCrc <= 0xffffU) && (nUnpackedSize > 0) && ((qint64)nUnpackedSize <= FINEAR_MAX_UNPACKED_SIZE)) {
                    bResult = true;

                    if (pnUnpackedSize) *pnUnpackedSize = nUnpackedSize;
                    if (pnCrc) *pnCrc = nCrc;
                }
            }
        }
    }

    if (guardedDevice) guardedDevice->seek(nSavedPosition);

    return guardedArchive && guardedDevice && bResult;
}

bool XFineReaderPack::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XFineReaderPack xfinereaderpack(pDevice);

    return xfinereaderpack.isValid(pPdStruct);
}

bool XFineReaderPack::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    return _readAndCheckHeader(nullptr, nullptr, pPdStruct);
}

XFineReaderPack::FINEAR_HEADER XFineReaderPack::_read_FINEAR_HEADER(qint64 nOffset)
{
    FINEAR_HEADER header = {};
    read_array(nOffset, reinterpret_cast<char *>(&header), sizeof(FINEAR_HEADER));
    return header;
}

XBinary::FT XFineReaderPack::getFileType()
{
    return XBinary::FT_FINEREADER_PACK;
}

XBinary::MODE XFineReaderPack::getMode()
{
    return XBinary::MODE_DATA;
}

QString XFineReaderPack::getMIMEString()
{
    return "application/x-finereader-pack";
}

qint32 XFineReaderPack::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

XBinary::ENDIAN XFineReaderPack::getEndian()
{
    return XBinary::ENDIAN_LITTLE;
}

QString XFineReaderPack::getArch()
{
    return QString();
}

QString XFineReaderPack::getFileFormatExt()
{
    // The install media keeps the original name with the last extension
    // character replaced by '_', so there is no fixed extension.
    return QString();
}

QString XFineReaderPack::getFileFormatExtsString()
{
    return "FineReader packed file (*.*)";
}

qint64 XFineReaderPack::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

bool XFineReaderPack::isSigned()
{
    return false;
}

XBinary::OSNAME XFineReaderPack::getOsName()
{
    return XBinary::OSNAME_MULTIPLATFORM;
}

QString XFineReaderPack::getOsVersion()
{
    return QString();
}

QString XFineReaderPack::getVersion()
{
    return QString();
}

bool XFineReaderPack::isEncrypted()
{
    return false;
}

QList<XBinary::MAPMODE> XFineReaderPack::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XFineReaderPack::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.mode = getMode();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());
    result.sArch = getArch();
    result.nBinarySize = getSize();

    quint32 nUnpackedSize = 0;
    quint32 nCrc = 0;

    if (!_readAndCheckHeader(&nUnpackedSize, &nCrc, pPdStruct)) {
        return result;
    }

    const qint64 nTotalSize = getSize();
    qint32 nIndex = 0;

    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = XADDR_MAX;
    recHeader.nOffset = 0;
    recHeader.nSize = FINEAR_HEADER_SIZE;
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = QString("FINEAR ") + tr("Header");
    result.listRecords.append(recHeader);

    const qint64 nStreamSize = nTotalSize - FINEAR_HEADER_SIZE;

    if (nStreamSize > 0) {
        _MEMORY_RECORD recData = {};
        recData.nAddress = XADDR_MAX;
        recData.nOffset = FINEAR_HEADER_SIZE;
        recData.nSize = nStreamSize;
        recData.nIndex = nIndex++;
        recData.filePart = FILEPART_REGION;
        recData.sName = tr("Compressed Data");
        result.listRecords.append(recData);
    }

    _handleOverlay(&result);

    return result;
}

QString XFineReaderPack::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XFINEREADERPACK_STRUCTID, sizeof(_TABLE_XFINEREADERPACK_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XFineReaderPack::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XFINEREADERPACK_STRUCTID, sizeof(_TABLE_XFINEREADERPACK_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XFineReaderPack::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XFINEREADERPACK_STRUCTID, sizeof(_TABLE_XFINEREADERPACK_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XFineReaderPack::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    const quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_FINEAR_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_FINEAR_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_FINEAR_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = sizeof(FINEAR_HEADER);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_FINEAR_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_FINEAR_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XFineReaderPack::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_FINEAR_HEADER) {
        listResult.append({"signature", (qint32)offsetof(FINEAR_HEADER, signature), 6, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"magic", (qint32)offsetof(FINEAR_HEADER, magic), 3, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"crc", (qint32)offsetof(FINEAR_HEADER, crc), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"unpacked_size", (qint32)offsetof(FINEAR_HEADER, unpacked_size), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
    }

    return listResult;
}

static bool finearCanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XFineReaderPack::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    if (nFileParts == 0) {
        return listResult;
    }

    quint32 nUnpackedSize = 0;
    quint32 nCrc = 0;

    if (!_readAndCheckHeader(&nUnpackedSize, &nCrc, pPdStruct)) {
        return listResult;
    }

    const qint64 nTotalSize = getSize();
    const qint64 nStreamSize = nTotalSize - FINEAR_HEADER_SIZE;

    if (nStreamSize <= 0) {
        return listResult;
    }

    if ((nFileParts & FILEPART_HEADER) && finearCanAppend(nLimit, listResult)) {
        FPART record = {};

        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = FINEAR_HEADER_SIZE;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");

        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && finearCanAppend(nLimit, listResult)) {
        // Everything after the 17-byte header is exactly one -lh1- (LZHUF)
        // stream; the packer leaves no trailing bytes.
        FPART record = {};

        record.filePart = FILEPART_REGION;
        record.nFileOffset = FINEAR_HEADER_SIZE;
        record.nFileSize = nStreamSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Compressed Data");
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZH1);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nStreamSize);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)nUnpackedSize);

        listResult.append(record);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XFineReaderPack::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XFineReaderPack::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XFineReaderPack> guardedArchive(this);
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

        quint32 nUnpackedSize = 0;
        quint32 nCrc = 0;

        const bool bValid = guardedArchive->_readAndCheckHeader(&nUnpackedSize, &nCrc, pPdStruct);
        if (!guardedArchive) return false;
        if (!bValid) {
            guardedArchive->releaseUnpackSource(pState);
            return false;
        }

        const qint64 nSize = guardedArchive->getSize();
        if (!guardedArchive) return false;

        FINEAR_UNPACK_CONTEXT *pContext = new (std::nothrow) FINEAR_UNPACK_CONTEXT;
        if (!pContext) {
            guardedArchive->releaseUnpackSource(pState);
            return false;
        }

        QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
        if (!guardedArchive || !guardedSource) {
            if (guardedArchive) guardedArchive->releaseUnpackSource(pState);
            delete pContext;
            return false;
        }

        QString sName = XBinary::getDeviceFileName(guardedSource.data());
        if (!guardedArchive || !guardedSource) {
            if (guardedArchive) guardedArchive->releaseUnpackSource(pState);
            delete pContext;
            return false;
        }
        if (!sName.isEmpty()) {
            // The container stores no name. The install media keeps the
            // original one with the final extension character replaced by '_',
            // and that character is not recoverable, so the container name is
            // reused verbatim.
            sName = QFileInfo(sName).fileName();
        }
        if (sName.isEmpty()) sName = QStringLiteral("finear_data");

        pContext->sFileName = sName;
        pContext->nTotalSize = nSize;
        pContext->nCompressedSize = nSize - FINEAR_HEADER_SIZE;
        pContext->nUncompressedSize = (qint64)nUnpackedSize;
        pContext->nCrc = nCrc;

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

XBinary::ARCHIVERECORD XFineReaderPack::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<XFineReaderPack> guardedArchive(this);

    XBinary::ARCHIVERECORD result = {};

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive) {
        return result;
    }

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    FINEAR_UNPACK_CONTEXT *pContext = reinterpret_cast<FINEAR_UNPACK_CONTEXT *>(pState->pContext);

    result.nStreamOffset = FINEAR_HEADER_SIZE;
    result.nStreamSize = pContext->nCompressedSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZH1);

    return result;
}

bool XFineReaderPack::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XFineReaderPack> guardedArchive(this);

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

    FINEAR_UNPACK_CONTEXT *pContext = reinterpret_cast<FINEAR_UNPACK_CONTEXT *>(pState->pContext);

    const qint64 nFileSize = guardedArchive->getSize();
    if (!guardedArchive || !guardedSource) return false;

    if ((nFileSize != pContext->nTotalSize) || (nFileSize <= FINEAR_HEADER_SIZE) || (pContext->nCompressedSize != (nFileSize - FINEAR_HEADER_SIZE))) {
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
    if (!guardedArchive || !pStage || !guardedOutput || !guardedSource || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive) return false;

    SubDevice sd(guardedSource.data(), FINEAR_HEADER_SIZE, pContext->nCompressedSize);

    bool bResult = false;

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZH1);
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
        state.mapUnpackProperties = pState->mapUnpackProperties;
        state.spOutputBudget = pState->spOutputBudget;
        state.pDeviceInput = &sd;
        state.pDeviceOutput = pStage.get();
        state.nInputOffset = 0;
        state.nInputLimit = pContext->nCompressedSize;
        state.nProcessedOffset = 0;
        state.nProcessedLimit = pContext->nUncompressedSize;

        bResult = XLZHDecoder::decompress(&state, 1, pPdStruct) && guardedArchive && guardedOutput && guardedSource &&
                  (state.nCountOutput == pContext->nUncompressedSize);

        sd.close();
    }

    return bResult && guardedArchive && guardedOutput && guardedSource && guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) && guardedArchive &&
           guardedArchive->publishUnpackOutput(pStage.get(), guardedOutput.data(), pState, pPdStruct);
}

bool XFineReaderPack::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XFineReaderPack> guardedArchive(this);

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    pState->nCurrentIndex++;

    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XFineReaderPack::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XFineReaderPack> guardedArchive(this);

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState)) return false;

    FINEAR_UNPACK_CONTEXT *pContext = static_cast<FINEAR_UNPACK_CONTEXT *>(pState->pContext);
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

QList<QString> XFineReaderPack::getSearchSignatures()
{
    return {"'FINEAR'DD88DD"};
}

XBinary *XFineReaderPack::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XFineReaderPack(pDevice);
}

bool XFineReaderPack::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XFineReaderPack> guardedThis(this);
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

void *XFineReaderPack::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XFineReaderPack> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XFineReaderPack::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
