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
#include "xlzma.h"

#include "Algos/xlzmadecoder.h"
#include "subdevice.h"

#include <QPointer>

#include <limits>
#include <new>

XBinary::XCONVERT _TABLE_XLZMA_STRUCTID[] = {{XLZMA::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                             {XLZMA::STRUCTID_LZMA_ALONE_HEADER, "LZMA_ALONE_HEADER", QString("LZMA alone header")}};

static bool _isValidLZMAProperties(quint8 nProperties)
{
    quint8 nValue = nProperties;
    quint8 nLC = nValue % 9;
    nValue /= 9;
    quint8 nLP = nValue % 5;
    quint8 nPB = nValue / 5;

    return (nLC <= 8) && (nLP <= 4) && (nPB <= 4);
}

namespace {

const qint64 LZMA_ALONE_HEADER_SIZE = 13;
const quint32 LZMA_MAX_DICTIONARY_SIZE = 512U * 1024U * 1024U;

quint32 readLzmaLE32(const char *pData)
{
    return (quint32)(quint8)pData[0] | ((quint32)(quint8)pData[1] << 8) | ((quint32)(quint8)pData[2] << 16) | ((quint32)(quint8)pData[3] << 24);
}

quint64 readLzmaLE64(const char *pData)
{
    quint64 nResult = 0;
    for (qint32 i = 0; i < 8; ++i) {
        nResult |= ((quint64)(quint8)pData[i]) << (i * 8);
    }
    return nResult;
}

bool parseLzmaAloneHeader(const QByteArray &baHeader, QByteArray *pbaProperties, qint64 *pnDeclaredSize)
{
    if (pbaProperties) pbaProperties->clear();
    if (pnDeclaredSize) *pnDeclaredSize = -1;
    if (!pbaProperties || !pnDeclaredSize || (baHeader.size() != LZMA_ALONE_HEADER_SIZE)) {
        return false;
    }

    const quint8 nProperties = (quint8)baHeader.at(0);
    const quint32 nDictionarySize = readLzmaLE32(baHeader.constData() + 1);
    const quint64 nDeclaredSize = readLzmaLE64(baHeader.constData() + 5);
    if (!_isValidLZMAProperties(nProperties) || (nDictionarySize > LZMA_MAX_DICTIONARY_SIZE) ||
        ((nDeclaredSize != Q_UINT64_C(0xFFFFFFFFFFFFFFFF)) && (nDeclaredSize > (quint64)(std::numeric_limits<qint64>::max)()))) {
        return false;
    }

    *pbaProperties = baHeader.left(5);
    if (nDeclaredSize != Q_UINT64_C(0xFFFFFFFFFFFFFFFF)) {
        *pnDeclaredSize = (qint64)nDeclaredSize;
    }
    return true;
}

// LZMA-alone has no magic number, so accepting every syntactically valid
// properties tuple produces many false positives.  Mirror libarchive's bidder
// rules here: all property bytes remain decodable when the type is selected
// explicitly, while automatic detection also requires a dictionary size that
// real LZMA SDK/XZ encoders commonly write.
bool isPlausibleLzmaAloneHeader(const QByteArray &baHeader)
{
    if (baHeader.size() != LZMA_ALONE_HEADER_SIZE) return false;

    const quint8 nProperties = (quint8)baHeader.at(0);
    if (!_isValidLZMAProperties(nProperties)) return false;

    qint32 nBitsChecked = 0;
    if ((nProperties == 0x5D) || (nProperties == 0x5E)) {
        nBitsChecked += 8;
    }

    const quint64 nDeclaredSize = readLzmaLE64(baHeader.constData() + 5);
    if (nDeclaredSize == Q_UINT64_C(0xFFFFFFFFFFFFFFFF)) {
        nBitsChecked += 64;
    }

    const quint32 nDictionarySize = readLzmaLE32(baHeader.constData() + 1);
    switch (nDictionarySize) {
        case 0x00001000U:
        case 0x00002000U:
        case 0x00004000U:
        case 0x00008000U:
        case 0x00010000U:
        case 0x00020000U:
        case 0x00040000U:
        case 0x00080000U:
        case 0x00100000U:
        case 0x00200000U:
        case 0x00400000U:
        case 0x00800000U:
        case 0x01000000U:
        case 0x02000000U:
        case 0x04000000U:
        case 0x08000000U: nBitsChecked += 32; break;
        default:
            // XZ Utils may lower the requested dictionary in whole-MiB steps
            // when the encoder cannot reserve enough memory.  libarchive only
            // accepts that weaker signature with the two common property bytes
            // and an unknown uncompressed size.
            if ((nDictionarySize >= 0x00300000U) && (nDictionarySize <= 0x03F00000U) && ((nDictionarySize & 0x000FFFFFU) == 0) && (nBitsChecked == 72)) {
                nBitsChecked += 32;
            } else {
                return false;
            }
            break;
    }

    return nBitsChecked > 0;
}

class LzmaDiscardDevice : public QIODevice {
protected:
    qint64 readData(char *, qint64) override
    {
        return -1;
    }

    qint64 writeData(const char *pData, qint64 nSize) override
    {
        if ((nSize < 0) || ((nSize > 0) && !pData)) return -1;
        return nSize;
    }
};

bool measureLzmaAloneStream(QIODevice *pDevice, qint64 nFileSize, const QByteArray &baProperties, qint64 nDeclaredSize, qint64 *pnCompressedSize,
                            qint64 *pnUncompressedSize, XBinary::PDSTRUCT *pPdStruct, const QMap<XBinary::UNPACK_PROP, QVariant> *pUnpackProperties = nullptr)
{
    if (pnCompressedSize) *pnCompressedSize = 0;
    if (pnUncompressedSize) *pnUncompressedSize = 0;
    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice || !pnCompressedSize || !pnUncompressedSize || (nFileSize <= LZMA_ALONE_HEADER_SIZE) || (baProperties.size() != 5) || (nDeclaredSize < -1) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint64 nPayloadSize = nFileSize - LZMA_ALONE_HEADER_SIZE;
    SubDevice input(guardedDevice.data(), LZMA_ALONE_HEADER_SIZE, nPayloadSize);
    if (!guardedDevice) return false;
    LzmaDiscardDevice output;
    const bool bInputOpened = input.open(QIODevice::ReadOnly);
    if (!guardedDevice || !bInputOpened) return false;
    const bool bOutputOpened = output.open(QIODevice::WriteOnly);
    if (!guardedDevice || !bOutputOpened) {
        input.close();
        return false;
    }

    XBinary::DATAPROCESS_STATE state = {};
    if (pUnpackProperties) state.mapUnpackProperties = *pUnpackProperties;
    state.pDeviceInput = &input;
    state.pDeviceOutput = &output;
    state.nInputOffset = 0;
    state.nInputLimit = nPayloadSize;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;
    state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_LZMA);
    state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSPROPERTIES, baProperties);
    if (nDeclaredSize >= 0) {
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nDeclaredSize);
    }

    const bool bDecoded = XLZMADecoder::decompress(&state, baProperties, pPdStruct);
    const bool bResult = guardedDevice && bDecoded && (state.nCountInput > 0) && (state.nCountInput <= nPayloadSize) && (state.nCountOutput >= 0) &&
                         ((nDeclaredSize < 0) || (state.nCountOutput == nDeclaredSize)) && XBinary::isPdStructNotCanceled(pPdStruct);
    if (bResult) {
        *pnCompressedSize = state.nCountInput;
        *pnUncompressedSize = state.nCountOutput;
    }

    output.close();
    input.close();
    return bResult && guardedDevice;
}

}  // namespace

XLZMA::XLZMA(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XLZMA::failUnpackInitialization(XLZMA *pArchive, UNPACK_STATE *pState)
{
    if (pArchive) pArchive->releaseUnpackSource(pState);
    if (pState) *pState = UNPACK_STATE();
    return false;
}

XLZMA::~XLZMA()
{
}

bool XLZMA::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XLZMA> guardedThis(this);
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const qint64 nSize = guardedThis->getSize();
    if (!guardedThis || (nSize <= LZMA_ALONE_HEADER_SIZE)) return false;
    const QByteArray baHeader = guardedThis->read_array_process(0, LZMA_ALONE_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QByteArray baProperties;
    qint64 nDeclaredSize = -1;
    return isPlausibleLzmaAloneHeader(baHeader) && parseLzmaAloneHeader(baHeader, &baProperties, &nDeclaredSize);
}

bool XLZMA::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLZMA lzma(pDevice);

    return lzma.isValid(pPdStruct);
}

XBinary::MODE XLZMA::getMode()
{
    return MODE_DATA;
}

qint32 XLZMA::getType()
{
    return TYPE_LZMA_ALONE;
}

QString XLZMA::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    if (nType == TYPE_LZMA_ALONE) {
        sResult = QString("LZMA");
    }

    return sResult;
}

XBinary::ENDIAN XLZMA::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XLZMA::getFileFormatExt()
{
    return "lzma";
}

QString XLZMA::getFileFormatExtsString()
{
    return "lzma";
}

XBinary::FT XLZMA::getFileType()
{
    return FT_LZMA;
}

qint64 XLZMA::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QString XLZMA::getMIMEString()
{
    return "application/x-lzma";
}

XBinary::OSNAME XLZMA::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QList<XBinary::MAPMODE> XLZMA::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XLZMA::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XLZMA::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XLZMA_STRUCTID, sizeof(_TABLE_XLZMA_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XLZMA::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XLZMA_STRUCTID, sizeof(_TABLE_XLZMA_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XLZMA::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XLZMA_STRUCTID, sizeof(_TABLE_XLZMA_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// QList<XBinary::DATA_HEADER> XLZMA::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
//         _dataHeadersOptions.nID = STRUCTID_LZMA_ALONE_HEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         if (isPdStructNotCanceled(pPdStruct)) {
//             listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//         }
//     } else if (dataHeadersOptions.nID == STRUCTID_LZMA_ALONE_HEADER) {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XLZMA::structIDToString(dataHeadersOptions.nID));
//             dataHeader.nSize = sizeof(LZMA_ALONE_HEADER);
//             dataHeader.listRecords.append(getDataRecord(offsetof(LZMA_ALONE_HEADER, nProperties), 1, "nProperties", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(
//                 getDataRecord(offsetof(LZMA_ALONE_HEADER, nDictionarySize), 4, "nDictionarySize", VT_UINT32, DRF_SIZE, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(
//                 getDataRecord(offsetof(LZMA_ALONE_HEADER, nUncompressedSize), 8, "nUncompressedSize", VT_UINT64, DRF_UNKNOWN, ENDIAN_LITTLE));
//             listResult.append(dataHeader);
//         }
//     }

//     return listResult;
// }

QList<XBinary::XFHEADER> XLZMA::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XBinary::XFHEADER> listResult;
    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_LZMA_ALONE_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_LZMA_ALONE_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if ((nHeaderOffset != -1) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, sizeof(LZMA_ALONE_HEADER))) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_LZMA_ALONE_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = sizeof(LZMA_ALONE_HEADER);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_LZMA_ALONE_HEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_LZMA_ALONE_HEADER), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XLZMA::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_LZMA_ALONE_HEADER) {
        listResult.append({"nProperties", (qint32)offsetof(LZMA_ALONE_HEADER, nProperties), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"nDictionarySize", (qint32)offsetof(LZMA_ALONE_HEADER, nDictionarySize), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"nUncompressedSize", (qint32)offsetof(LZMA_ALONE_HEADER, nUncompressedSize), 8, XFRECORD_FLAG_NONE, VT_UINT64});
    }

    return listResult;
}

static bool _lzmaCanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XLZMA::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    qint64 nFileSize = getSize();

    if ((nFileParts & FILEPART_HEADER) && _lzmaCanAppend(nLimit, listResult) && (nFileSize > 0)) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = qMin<qint64>((qint64)sizeof(LZMA_ALONE_HEADER), nFileSize);
        header.nVirtualAddress = XADDR_MAX;
        header.sName = tr("Header");
        listResult.append(header);
    }

    if ((nFileParts & FILEPART_STREAM) && _lzmaCanAppend(nLimit, listResult) && (nFileSize > (qint64)sizeof(LZMA_ALONE_HEADER))) {
        FPART stream = {};
        stream.filePart = FILEPART_STREAM;
        stream.nFileOffset = sizeof(LZMA_ALONE_HEADER);
        stream.nFileSize = nFileSize - sizeof(LZMA_ALONE_HEADER);
        stream.nVirtualAddress = XADDR_MAX;
        stream.sName = tr("Stream");
        listResult.append(stream);
    }

    return listResult;
}

QList<QString> XLZMA::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("5D000000");

    return listResult;
}

XBinary *XLZMA::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XLZMA(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XLZMA::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XLZMA::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XLZMA> guardedThis(this);
    if (!pState || m_bUnpackOperationInProgress || ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedThis->ownsUnpackSource(pState))) {
        return false;
    }

    const bool bFinished = guardedThis->finishUnpack(pState, nullptr);
    if (!guardedThis || !bFinished) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const bool bBound = guardedThis->bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) {
        *pState = UNPACK_STATE();
        return false;
    }
    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedThis || !guardedSource) return failUnpackInitialization(guardedThis.data(), pState);
    const qint64 nFileSize = guardedSource->size();
    if (!guardedThis || !guardedSource || (nFileSize <= LZMA_ALONE_HEADER_SIZE)) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }

    const QByteArray baHeader = guardedThis->read_array_process(0, LZMA_ALONE_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }
    QByteArray baProperties;
    qint64 nDeclaredSize = -1;
    if (!parseLzmaAloneHeader(baHeader, &baProperties, &nDeclaredSize)) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }

    qint64 nCompressedSize = 0;
    qint64 nUncompressedSize = 0;
    const bool bMeasured =
        measureLzmaAloneStream(guardedSource.data(), nFileSize, baProperties, nDeclaredSize, &nCompressedSize, &nUncompressedSize, pPdStruct, &mapProperties);
    if (!guardedThis || !guardedSource || !bMeasured || (nCompressedSize <= 0) || (nCompressedSize > (nFileSize - LZMA_ALONE_HEADER_SIZE)) || (nUncompressedSize < 0) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }
    const bool bSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !guardedSource || !bSourceCurrent) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }

    QString sFileName = XBinary::getDeviceFileBaseName(guardedSource.data());
    if (!guardedThis || !guardedSource) return failUnpackInitialization(guardedThis.data(), pState);
    if (sFileName.isEmpty()) sFileName = QStringLiteral("stream");

    LZMA_UNPACK_CONTEXT *pContext = new (std::nothrow) LZMA_UNPACK_CONTEXT();
    if (!pContext) return failUnpackInitialization(guardedThis.data(), pState);
    pContext->nCompressedSize = nCompressedSize;
    pContext->nUncompressedSize = nUncompressedSize;
    pContext->baProperties = baProperties;
    pContext->sFileName = sFileName;

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = nFileSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;
    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedThis) return false;
    if (!bFinalized) {
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XLZMA::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XLZMA> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return ARCHIVERECORD();
    }
    const bool bSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent || (pState->nCurrentIndex != 0) || (pState->nNumberOfRecords != 1) || (pState->nTotalSize <= LZMA_ALONE_HEADER_SIZE)) {
        return ARCHIVERECORD();
    }

    LZMA_UNPACK_CONTEXT *pContext = static_cast<LZMA_UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->nCompressedSize <= 0) || (pContext->nCompressedSize > (pState->nTotalSize - LZMA_ALONE_HEADER_SIZE)) || (pContext->nUncompressedSize < 0) ||
        (pContext->baProperties.size() != 5)) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = LZMA_ALONE_HEADER_SIZE;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZMA);
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, pContext->baProperties);
    return result;
}

bool XLZMA::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XLZMA> guardedThis(this);
    if (!pState || !pState->pContext || !pDevice || !guardedThis->ownsUnpackSource(pState) || (pState->nCurrentIndex != 0) || (pState->nNumberOfRecords != 1)) {
        return false;
    }
    const qint64 nEndOffset = LZMA_ALONE_HEADER_SIZE + static_cast<LZMA_UNPACK_CONTEXT *>(pState->pContext)->nCompressedSize;
    const bool bResult = XArchive::unpackCurrent(pState, pDevice, pPdStruct);
    if (!guardedThis) return false;
    if (bResult) pState->nCurrentOffset = nEndOffset;
    return bResult;
}

bool XLZMA::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XLZMA> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const bool bSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent || (pState->nCurrentIndex != 0) || (pState->nNumberOfRecords != 1) || (pState->nTotalSize <= LZMA_ALONE_HEADER_SIZE)) {
        return false;
    }

    LZMA_UNPACK_CONTEXT *pContext = static_cast<LZMA_UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->nCompressedSize <= 0) || (pContext->nCompressedSize > (pState->nTotalSize - LZMA_ALONE_HEADER_SIZE))) {
        return false;
    }
    pState->nCurrentOffset = LZMA_ALONE_HEADER_SIZE + pContext->nCompressedSize;
    ++pState->nCurrentIndex;
    return false;
}

bool XLZMA::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    QPointer<XLZMA> guardedThis(this);
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedThis->ownsUnpackSource(pState)) {
        return false;
    }

    LZMA_UNPACK_CONTEXT *pContext = static_cast<LZMA_UNPACK_CONTEXT *>(pState->pContext);
    guardedThis->releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    if (!guardedThis) return false;
    *pState = UNPACK_STATE();
    return true;
}

QList<XBinary::FPART_PROP> XLZMA::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME,       FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD,
            FPART_PROP_COMPRESSPROPERTIES, FPART_PROP_STREAMOFFSET,   FPART_PROP_STREAMSIZE};
}

bool XLZMA::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XLZMA> guardedThis(this);
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

void *XLZMA::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XLZMA> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XLZMA::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
