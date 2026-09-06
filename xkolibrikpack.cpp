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
#include "xkolibrikpack.h"

#include <QFileInfo>

#include <memory>
#include <new>

#include "Algos/xkolibrikpackdecoder.h"

static XBinary::XCONVERT _TABLE_XKOLIBRIKPACK_STRUCTID[] = {{XKolibriKPack::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                            {XKolibriKPack::STRUCTID_KPACK_HEADER, "KPACK_HEADER", QString("KPACK_HEADER")}};

static const qint64 KPACK_HEADER_SIZE = 12;
static const qint64 KPACK_CALLTRICK_TRAILER_SIZE = 5;
static const quint32 KPACK_FLAG_CALLTRICK1 = 0x40;
static const quint32 KPACK_FLAG_CALLTRICK2 = 0x80;

XKolibriKPack::XKolibriKPack(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XKolibriKPack::_readAndCheckHeader(quint32 *pnUnpackedSize, quint32 *pnFlags, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QPointer<XKolibriKPack> guardedArchive(this);

    QPointer<QIODevice> guardedDevice(guardedArchive->getDevice());
    if (!guardedArchive || !guardedDevice) return false;

    // Detection probes a device the caller still owns: remember where it was
    // and put it back before returning.
    const qint64 nSavedPosition = guardedDevice->pos();

    const qint64 nSize = guardedArchive->getSize();
    if (!guardedArchive || !guardedDevice) return false;

    bool bResult = false;

    if (nSize >= (KPACK_HEADER_SIZE + 4)) {
        const QByteArray baHeader = guardedArchive->read_array(0, (qint32)KPACK_HEADER_SIZE);

        if (guardedArchive && guardedDevice && (baHeader.size() == (qint32)KPACK_HEADER_SIZE)) {
            bResult = XKolibriKPackDecoder::checkHeader(baHeader.constData(), baHeader.size(), nSize, pnUnpackedSize, pnFlags);
        }
    }

    if (guardedDevice) guardedDevice->seek(nSavedPosition);

    return guardedArchive && guardedDevice && bResult;
}

bool XKolibriKPack::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XKolibriKPack xkolibrikpack(pDevice);

    return xkolibrikpack.isValid(pPdStruct);
}

bool XKolibriKPack::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    return _readAndCheckHeader(nullptr, nullptr, pPdStruct);
}

XKolibriKPack::KPACK_HEADER XKolibriKPack::_read_KPACK_HEADER(qint64 nOffset)
{
    KPACK_HEADER header = {};
    read_array(nOffset, reinterpret_cast<char *>(&header), sizeof(KPACK_HEADER));
    return header;
}

XBinary::FT XKolibriKPack::getFileType()
{
    return XBinary::FT_KOLIBRI_KPACK;
}

XBinary::MODE XKolibriKPack::getMode()
{
    return XBinary::MODE_DATA;
}

QString XKolibriKPack::getMIMEString()
{
    return "application/x-kolibri-kpack";
}

qint32 XKolibriKPack::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

XBinary::ENDIAN XKolibriKPack::getEndian()
{
    return XBinary::ENDIAN_LITTLE;
}

QString XKolibriKPack::getArch()
{
    return QString();
}

QString XKolibriKPack::getFileFormatExt()
{
    return "kpack";
}

QString XKolibriKPack::getFileFormatExtsString()
{
    return "KolibriOS kpack (*.kpack)";
}

qint64 XKolibriKPack::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

bool XKolibriKPack::isSigned()
{
    return false;
}

XBinary::OSNAME XKolibriKPack::getOsName()
{
    return XBinary::OSNAME_MULTIPLATFORM;
}

QString XKolibriKPack::getOsVersion()
{
    return QString();
}

QString XKolibriKPack::getVersion()
{
    return QString();
}

bool XKolibriKPack::isEncrypted()
{
    return false;
}

QList<XBinary::MAPMODE> XKolibriKPack::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XKolibriKPack::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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
    quint32 nFlags = 0;

    if (!_readAndCheckHeader(&nUnpackedSize, &nFlags, pPdStruct)) {
        return result;
    }

    const qint64 nTotalSize = getSize();
    qint32 nIndex = 0;

    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = XADDR_MAX;
    recHeader.nOffset = 0;
    recHeader.nSize = KPACK_HEADER_SIZE;
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = QString("KPCK ") + tr("Header");
    result.listRecords.append(recHeader);

    qint64 nStreamSize = nTotalSize - KPACK_HEADER_SIZE;

    if (nFlags & (KPACK_FLAG_CALLTRICK1 | KPACK_FLAG_CALLTRICK2)) {
        nStreamSize -= KPACK_CALLTRICK_TRAILER_SIZE;
    }

    if (nStreamSize > 0) {
        _MEMORY_RECORD recData = {};
        recData.nAddress = XADDR_MAX;
        recData.nOffset = KPACK_HEADER_SIZE;
        recData.nSize = nStreamSize;
        recData.nIndex = nIndex++;
        recData.filePart = FILEPART_REGION;
        recData.sName = tr("Compressed Data");
        result.listRecords.append(recData);
    }

    if (nFlags & (KPACK_FLAG_CALLTRICK1 | KPACK_FLAG_CALLTRICK2)) {
        _MEMORY_RECORD recTrailer = {};
        recTrailer.nAddress = XADDR_MAX;
        recTrailer.nOffset = nTotalSize - KPACK_CALLTRICK_TRAILER_SIZE;
        recTrailer.nSize = KPACK_CALLTRICK_TRAILER_SIZE;
        recTrailer.nIndex = nIndex++;
        recTrailer.filePart = FILEPART_FOOTER;
        recTrailer.sName = tr("Footer");
        result.listRecords.append(recTrailer);
    }

    _handleOverlay(&result);

    return result;
}

QString XKolibriKPack::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XKOLIBRIKPACK_STRUCTID, sizeof(_TABLE_XKOLIBRIKPACK_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XKolibriKPack::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XKOLIBRIKPACK_STRUCTID, sizeof(_TABLE_XKOLIBRIKPACK_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XKolibriKPack::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XKOLIBRIKPACK_STRUCTID, sizeof(_TABLE_XKOLIBRIKPACK_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XKolibriKPack::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    const quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_KPACK_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_KPACK_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_KPACK_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = sizeof(KPACK_HEADER);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_KPACK_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_KPACK_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XKolibriKPack::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_KPACK_HEADER) {
        listResult.append({"signature", (qint32)offsetof(KPACK_HEADER, signature), 4, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"unpacked_size", (qint32)offsetof(KPACK_HEADER, unpacked_size), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"flags", (qint32)offsetof(KPACK_HEADER, flags), 4, XFRECORD_FLAG_NONE, VT_UINT32});
    }

    return listResult;
}

static bool kpackCanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XKolibriKPack::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    if (nFileParts == 0) {
        return listResult;
    }

    quint32 nUnpackedSize = 0;
    quint32 nFlags = 0;

    if (!_readAndCheckHeader(&nUnpackedSize, &nFlags, pPdStruct)) {
        return listResult;
    }

    const qint64 nTotalSize = getSize();

    if ((nFileParts & FILEPART_HEADER) && kpackCanAppend(nLimit, listResult)) {
        FPART record = {};

        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = KPACK_HEADER_SIZE;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");

        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && kpackCanAppend(nLimit, listResult)) {
        // The decoder consumes the complete container (the call-trick
        // parameters live in the trailer), so the data part is the whole file.
        FPART record = {};

        record.filePart = FILEPART_REGION;
        record.nFileOffset = 0;
        record.nFileSize = nTotalSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Compressed Data");
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_KOLIBRI_KPACK);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nTotalSize);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)nUnpackedSize);

        listResult.append(record);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XKolibriKPack::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XKolibriKPack::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XKolibriKPack> guardedArchive(this);
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
        quint32 nFlags = 0;

        const bool bValid = guardedArchive->_readAndCheckHeader(&nUnpackedSize, &nFlags, pPdStruct);
        if (!guardedArchive) return false;
        if (!bValid) {
            guardedArchive->releaseUnpackSource(pState);
            return false;
        }

        const qint64 nSize = guardedArchive->getSize();
        if (!guardedArchive) return false;

        KPACK_UNPACK_CONTEXT *pContext = new (std::nothrow) KPACK_UNPACK_CONTEXT;
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
            // kpack keeps the original file name; the container stores none.
            sName = QFileInfo(sName).fileName();
            if (sName.endsWith(".kpack", Qt::CaseInsensitive)) {
                sName = sName.left(sName.size() - 6);
            } else if (sName.endsWith(".kpck", Qt::CaseInsensitive)) {
                sName = sName.left(sName.size() - 5);
            }
        }
        if (sName.isEmpty()) sName = QStringLiteral("kpack_data");

        pContext->sFileName = sName;
        pContext->nTotalSize = nSize;
        pContext->nUncompressedSize = (qint64)nUnpackedSize;
        pContext->nFlags = nFlags;

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

XBinary::ARCHIVERECORD XKolibriKPack::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<XKolibriKPack> guardedArchive(this);

    XBinary::ARCHIVERECORD result = {};

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive) {
        return result;
    }

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    KPACK_UNPACK_CONTEXT *pContext = reinterpret_cast<KPACK_UNPACK_CONTEXT *>(pState->pContext);

    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nTotalSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nTotalSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_KOLIBRI_KPACK);

    return result;
}

bool XKolibriKPack::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XKolibriKPack> guardedArchive(this);

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

    KPACK_UNPACK_CONTEXT *pContext = reinterpret_cast<KPACK_UNPACK_CONTEXT *>(pState->pContext);

    const qint64 nFileSize = guardedArchive->getSize();
    if (!guardedArchive || !guardedSource) return false;

    if ((nFileSize != pContext->nTotalSize) || (nFileSize <= KPACK_HEADER_SIZE)) {
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

    SubDevice sd(guardedSource.data(), 0, nFileSize);

    bool bResult = false;

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_KOLIBRI_KPACK);
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
        state.mapUnpackProperties = pState->mapUnpackProperties;
        state.spOutputBudget = pState->spOutputBudget;
        state.pDeviceInput = &sd;
        state.pDeviceOutput = pStage.get();
        state.nInputOffset = 0;
        state.nInputLimit = nFileSize;
        state.nProcessedOffset = 0;
        state.nProcessedLimit = pContext->nUncompressedSize;

        bResult = XKolibriKPackDecoder::decompress(&state, pPdStruct) && guardedArchive && guardedOutput && guardedSource &&
                  (state.nCountOutput == pContext->nUncompressedSize);

        sd.close();
    }

    return bResult && guardedArchive && guardedOutput && guardedSource && guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) && guardedArchive &&
           guardedArchive->publishUnpackOutput(pStage.get(), guardedOutput.data(), pState, pPdStruct);
}

bool XKolibriKPack::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XKolibriKPack> guardedArchive(this);

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    pState->nCurrentIndex++;

    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XKolibriKPack::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XKolibriKPack> guardedArchive(this);

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState)) return false;

    KPACK_UNPACK_CONTEXT *pContext = static_cast<KPACK_UNPACK_CONTEXT *>(pState->pContext);
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

QList<QString> XKolibriKPack::getSearchSignatures()
{
    return {"'KPCK'"};
}

XBinary *XKolibriKPack::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XKolibriKPack(pDevice);
}

bool XKolibriKPack::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XKolibriKPack> guardedThis(this);
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

void *XKolibriKPack::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XKolibriKPack> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XKolibriKPack::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
