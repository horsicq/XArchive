/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
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
#include "xlzip.h"
#include "xlzmadecoder.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace {
class LzipCRC32OutputDevice : public QIODevice {
public:
    explicit LzipCRC32OutputDevice(QIODevice *pOutputDevice) : m_pOutputDevice(pOutputDevice), m_nCRC32(0xFFFFFFFF)
    {
    }

    bool isSequential() const override
    {
        return true;
    }

    quint32 getCRC32() const
    {
        return m_nCRC32 ^ 0xFFFFFFFF;
    }

protected:
    qint64 readData(char *pData, qint64 nMaxSize) override
    {
        Q_UNUSED(pData)
        Q_UNUSED(nMaxSize)

        return -1;
    }

    qint64 writeData(const char *pData, qint64 nMaxSize) override
    {
        if (!m_pOutputDevice || (nMaxSize < 0) || ((nMaxSize > 0) && !pData)) {
            return -1;
        }

        qint64 nWrittenTotal = 0;
        while (nWrittenTotal < nMaxSize) {
            const qint64 nWritten = m_pOutputDevice->write(pData + nWrittenTotal, nMaxSize - nWrittenTotal);
            if (!m_pOutputDevice || (nWritten <= 0) || (nWritten > (nMaxSize - nWrittenTotal))) {
                return nWrittenTotal ? nWrittenTotal : -1;
            }
            qint64 nCRCOffset = 0;
            while (nCRCOffset < nWritten) {
                const qint32 nCRCSize = (qint32)(std::min)(nWritten - nCRCOffset,
                                                          (qint64)(std::numeric_limits<qint32>::max)());
                m_nCRC32 = XBinary::_getCRC32(pData + nWrittenTotal + nCRCOffset, nCRCSize, m_nCRC32,
                                              XBinary::_getCRC32Table_EDB88320());
                nCRCOffset += nCRCSize;
            }
            nWrittenTotal += nWritten;
        }

        return nWrittenTotal;
    }

private:
    QPointer<QIODevice> m_pOutputDevice;
    quint32 m_nCRC32;
};

bool lzipClearOutputDevice(QIODevice *pDevice)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice || !guardedDevice->isWritable() || !guardedDevice) return false;
    if (guardedDevice->isSequential()) {
        return guardedDevice && (guardedDevice->pos() == 0) && guardedDevice;
    }
    return guardedDevice && XBinary::resize(guardedDevice.data(), 0) && guardedDevice &&
           guardedDevice->seek(0) && guardedDevice;
}
}  // namespace

XBinary::XCONVERT _TABLE_XLZIP_STRUCTID[] = {{XLzip::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                             {XLzip::STRUCTID_LZIP_HEADER, "LZIP_HEADER", QString("LZIP header")},
                                             {XLzip::STRUCTID_MEMBER_HEADER, "MEMBER_HEADER", QString("Member header")},
                                             {XLzip::STRUCTID_COMPRESSED_DATA, "COMPRESSED_DATA", QString("Compressed data")},
                                             {XLzip::STRUCTID_FOOTER, "FOOTER", QString("Footer")}};

XLzip::XLzip(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XLzip::isValid(PDSTRUCT *pPdStruct)
{
    QList<LZIP_MEMBER> listMembers;
    return getMembers(&listMembers, pPdStruct);
}

bool XLzip::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pDevice || pDevice->isSequential()) return false;

    const qint64 nPosition = pDevice->pos();
    if (nPosition < 0) return false;

    XLzip lzip(pDevice);
    const bool bResult = lzip.isValid(pPdStruct);
    if (!pDevice->seek(nPosition)) return false;
    return bResult;
}

bool XLzip::getMembers(QList<LZIP_MEMBER> *pMembers, PDSTRUCT *pPdStruct)
{
    QPointer<XLzip> guardedThis(this);
    if (!pMembers) return false;
    pMembers->clear();

    const qint64 nFileSize = getSize();
    qint64 nMemberEnd = nFileSize;
    const qint32 nMaximumMembers = 1000000;

    while (nMemberEnd > 0) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) || ((nMemberEnd < 36)) ||
            (pMembers->size() >= nMaximumMembers)) {
            pMembers->clear();
            return false;
        }

        const quint64 nMemberSize64 = guardedThis->read_uint64(nMemberEnd - 8);
        if (!guardedThis) {
            pMembers->clear();
            return false;
        }
        if ((nMemberSize64 < 36) || (nMemberSize64 > (quint64)nMemberEnd) ||
            (nMemberSize64 > (quint64)(std::numeric_limits<qint64>::max)())) {
            pMembers->clear();
            return false;
        }

        const qint64 nMemberSize = (qint64)nMemberSize64;
        const qint64 nMemberOffset = nMemberEnd - nMemberSize;
        const LZIP_HEADER header = guardedThis->_read_LZIP_HEADER(nMemberOffset);
        if (!guardedThis) {
            pMembers->clear();
            return false;
        }
        const quint32 nDictionarySize = guardedThis->_getDictionarySize(header.nDictSizeCode);
        const quint64 nUncompressedSize64 = guardedThis->read_uint64(nMemberEnd - 16);
        if (!guardedThis) {
            pMembers->clear();
            return false;
        }
        if ((std::memcmp(header.magic, "LZIP", 4) != 0) || (header.nVersion != 1) ||
            (nDictionarySize == 0) || (nUncompressedSize64 > (quint64)(std::numeric_limits<qint64>::max)())) {
            pMembers->clear();
            return false;
        }

        LZIP_MEMBER member = {};
        member.nOffset = nMemberOffset;
        member.nCompressedOffset = nMemberOffset + 6;
        member.nCompressedSize = nMemberSize - 26;
        member.nUncompressedSize = (qint64)nUncompressedSize64;
        member.nMemberSize = nMemberSize;
        member.nCRC32 = guardedThis->read_uint32(nMemberEnd - 20);
        if (!guardedThis) {
            pMembers->clear();
            return false;
        }
        member.nDictSizeCode = header.nDictSizeCode;
        if (member.nCompressedSize < 10) {
            pMembers->clear();
            return false;
        }

        pMembers->append(member);
        nMemberEnd = nMemberOffset;
    }

    if (pMembers->isEmpty() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        pMembers->clear();
        return false;
    }

    std::reverse(pMembers->begin(), pMembers->end());
    return true;
}

XBinary::MODE XLzip::getMode()
{
    return MODE_DATA;
}

qint32 XLzip::getType()
{
    return TYPE_LZ;
}

QString XLzip::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_LZ: sResult = QString("LZ"); break;
    }

    return sResult;
}

QString XLzip::getFileFormatExt()
{
    return "lz";
}

XBinary::FT XLzip::getFileType()
{
    return FT_LZIP;
}

QString XLzip::getFileFormatExtsString()
{
    return "lz";
}

qint64 XLzip::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QString XLzip::getMIMEString()
{
    return "application/x-lzip";
}

XBinary::ENDIAN XLzip::getEndian()
{
    return ENDIAN_LITTLE;
}

XBinary::OSNAME XLzip::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QList<XBinary::MAPMODE> XLzip::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XLzip::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XLzip::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XLZIP_STRUCTID, sizeof(_TABLE_XLZIP_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XLzip::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XLZIP_STRUCTID, sizeof(_TABLE_XLZIP_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XLzip::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XLZIP_STRUCTID, sizeof(_TABLE_XLZIP_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// QList<XBinary::DATA_HEADER> XLzip::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
//         _dataHeadersOptions.nID = STRUCTID_LZIP_HEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         if (isPdStructNotCanceled(pPdStruct)) {
//             listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//         }
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             if (dataHeadersOptions.nID == STRUCTID_LZIP_HEADER) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XLzip::structIDToString(dataHeadersOptions.nID));
//                 dataHeader.nSize = 6;  // Minimum header size

//                 dataHeader.listRecords.append(getDataRecord(0, 4, "magic", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(4, 1, "version", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(5, 1, "dictSizeCode", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);
//             }
//         }
//     }

//     return listResult;
// }

QList<XBinary::XFHEADER> XLzip::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XBinary::XFHEADER> listResult;
    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_LZIP_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_LZIP_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if ((nHeaderOffset != -1) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, sizeof(LZIP_HEADER))) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_LZIP_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = sizeof(LZIP_HEADER);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_LZIP_HEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_LZIP_HEADER), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XLzip::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_LZIP_HEADER) {
        listResult.append({"magic", static_cast<qint32>(offsetof(LZIP_HEADER, magic)), 4, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"nVersion", static_cast<qint32>(offsetof(LZIP_HEADER, nVersion)), 1, XFRECORD_FLAG_VERSION, VT_UINT8});
        listResult.append({"nDictSizeCode", static_cast<qint32>(offsetof(LZIP_HEADER, nDictSizeCode)), 1, XFRECORD_FLAG_NONE, VT_UINT8});
    }

    return listResult;
}

QList<XBinary::FPART> XLzip::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    const auto canAppend = [&]() -> bool { return (nLimit == -1) || (listResult.size() < nLimit); };

    const qint64 nFileSize = getSize();
    if (!isValid(pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppend()) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = 6;
        header.nVirtualAddress = -1;
        header.sName = tr("Header");
        listResult.append(header);
    }

    // Without decoding the first LZMA stream, the final footer cannot delimit
    // the first member of a concatenated file. Do not publish a knowingly wrong
    // stream range through the generic file-parts API.
    if (read_uint64(nFileSize - 8) != (quint64)nFileSize) {
        if ((nFileParts & FILEPART_DATA) && canAppend()) {
            FPART data = {};
            data.filePart = FILEPART_DATA;
            data.nFileOffset = 6;
            data.nFileSize = nFileSize - 6;
            data.nVirtualAddress = -1;
            data.sName = tr("Concatenated members");
            listResult.append(data);
        }

        return listResult;
    }

    qint64 nDataStart = 6;
    qint64 nDataSize = nFileSize - 26;  // Excluding header (6) and footer (20)

    if ((nFileParts & FILEPART_STREAM) && canAppend()) {
        FPART region = {};
        region.filePart = FILEPART_STREAM;
        region.nFileOffset = nDataStart;
        region.nFileSize = nDataSize;
        region.nVirtualAddress = -1;
        region.sName = tr("Stream");
        region.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZMA);
        listResult.append(region);
    }

    if ((nFileParts & FILEPART_DATA) && canAppend()) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = nDataStart;
        data.nFileSize = nDataSize;
        data.nVirtualAddress = -1;
        data.sName = tr("Data");
        listResult.append(data);
    }

    if ((nFileParts & FILEPART_OVERLAY) && canAppend()) {
        if (nFileSize > nDataStart + nDataSize) {
            FPART ov = {};
            ov.filePart = FILEPART_OVERLAY;
            ov.nFileOffset = nDataStart + nDataSize;
            ov.nFileSize = nFileSize - (nDataStart + nDataSize);
            ov.nVirtualAddress = -1;
            ov.sName = tr("Footer");
            listResult.append(ov);
        }
    }

    return listResult;
}

XLzip::LZIP_HEADER XLzip::_read_LZIP_HEADER(qint64 nOffset)
{
    QPointer<XLzip> guardedThis(this);
    LZIP_HEADER result = {};

    guardedThis->read_array(nOffset, result.magic, 4);
    if (!guardedThis) return LZIP_HEADER();
    result.nVersion = guardedThis->read_uint8(nOffset + 4);
    if (!guardedThis) return LZIP_HEADER();
    result.nDictSizeCode = guardedThis->read_uint8(nOffset + 5);
    if (!guardedThis) return LZIP_HEADER();

    return result;
}

quint32 XLzip::_getDictionarySize(quint8 nDictSizeCode)
{
    // Lzip stores a power-of-two base in the low five bits and an optional
    // reduction in sixteenths in the high three bits.
    const quint8 nExponent = nDictSizeCode & 0x1F;
    const quint8 nFraction = nDictSizeCode >> 5;

    if (nExponent < 12 || nExponent > 29) {
        return 0;  // Invalid
    }

    const quint32 nBaseSize = 1U << nExponent;
    const quint32 nDictionarySize = nBaseSize - ((nBaseSize / 16) * nFraction);

    if ((nDictionarySize < (1U << 12)) || (nDictionarySize > (1U << 29))) {
        return 0;
    }

    return nDictionarySize;
}

QMap<XBinary::UNPACK_PROP, QVariant> XLzip::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XLzip::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XLzip> guardedThis(this);
    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (!pState || m_bUnpackOperationInProgress ||
        ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
         !guardedThis->ownsUnpackSource(pState))) return false;

    // initUnpack may be called repeatedly with the same state.  Always release
    // the old format context and reset published state before validating the
    // new input.
    const bool bFinished = guardedThis->finishUnpack(pState, pPdStruct);
    if (!guardedThis || !bFinished) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const bool bBound = guardedThis->bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;

    QList<LZIP_MEMBER> listMembers;
    const bool bMembers = guardedThis->getMembers(&listMembers, pPdStruct);
    if (!guardedThis) return false;
    if (!bMembers || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        guardedThis->releaseUnpackSource(pState);
        return false;
    }

    qint64 nUncompressedSize = 0;
    for (const LZIP_MEMBER &member : listMembers) {
        if (member.nUncompressedSize > ((std::numeric_limits<qint64>::max)() - nUncompressedSize)) {
            guardedThis->releaseUnpackSource(pState);
            return false;
        }
        nUncompressedSize += member.nUncompressedSize;
    }

    LZIP_UNPACK_CONTEXT *pContext = new (std::nothrow) LZIP_UNPACK_CONTEXT;
    if (!pContext) {
        guardedThis->releaseUnpackSource(pState);
        return false;
    }

    const qint64 nFileSize = guardedThis->getSize();
    pContext->nHeaderSize = 0;
    pContext->nCompressedSize = nFileSize;
    pContext->nUncompressedSize = nUncompressedSize;
    pContext->nMemberSize = nFileSize;
    pContext->nCRC32 = (listMembers.size() == 1) ? listMembers.first().nCRC32 : 0;
    pContext->nDictSizeCode = listMembers.first().nDictSizeCode;
    pContext->bFooterValid = listMembers.size() == 1;
    pContext->listMembers = listMembers;

    pState->nCurrentOffset = 0;
    pState->nTotalSize = nFileSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->mapUnpackProperties = mapProperties;
    pState->pContext = pContext;

    if (!guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) return false;
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XLzip::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XLzip> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return result;
    const bool bSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) return result;

    LZIP_UNPACK_CONTEXT *pContext = static_cast<LZIP_UNPACK_CONTEXT *>(pState->pContext);

    // Concatenated lzip members form one logical data stream.
    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties[XBinary::FPART_PROP_ORIGINALNAME] =
        XBinary::getDeviceFileBaseName(guardedThis->getDevice());
    result.mapProperties[XBinary::FPART_PROP_COMPRESSEDSIZE] = pContext->nCompressedSize;
    result.mapProperties[XBinary::FPART_PROP_UNCOMPRESSEDSIZE] = pContext->nUncompressedSize;
    result.mapProperties[XBinary::FPART_PROP_HANDLEMETHOD] = XBinary::HANDLE_METHOD_LZIP;

    // There is no single standard CRC value for the concatenation.  Publish
    // the trailer checksum only when this record represents one member.
    if (pContext->bFooterValid) {
        result.mapProperties[XBinary::FPART_PROP_RESULTCRC] = pContext->nCRC32;
        result.mapProperties[XBinary::FPART_PROP_CRC_TYPE] = XBinary::CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF;
    }

    return result;
}

bool XLzip::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XLzip> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState || !pState->pContext || !pDevice) return false;
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedOutput || !guardedSource) return false;
    const bool bOutputSupported =
        guardedThis->isUnpackOutputSupported(guardedOutput.data());
    if (!guardedThis || !guardedOutput || !guardedSource ||
        !bOutputSupported) return false;
    const bool bAliases =
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data());
    if (!guardedThis || !guardedOutput || !guardedSource || bAliases) return false;
    const bool bSourceCurrent =
        guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !guardedOutput || !guardedSource ||
        !bSourceCurrent || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;

    LZIP_UNPACK_CONTEXT *pContext = static_cast<LZIP_UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->nUncompressedSize < 0) ||
        (pContext->nMemberSize < 0)) return false;
    const qint64 nMemberSize = pContext->nMemberSize;
    std::unique_ptr<QIODevice> pStage(XBinary::createFileBuffer(
        pContext->nUncompressedSize, pPdStruct));
    if (!guardedThis || !pStage || !guardedOutput || !guardedSource) return false;
    const bool bStageSourceCurrent =
        guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bStageSourceCurrent) return false;

    const bool bCheckCRC = XBinary::isUnpackCRCEnabled(pState->mapUnpackProperties,
                                                       XBinary::CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    bool bResult = true;

    for (const LZIP_MEMBER &member : pContext->listMembers) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            bResult = false;
            break;
        }

        const quint32 nDictSize = _getDictionarySize(member.nDictSizeCode);
        QByteArray baProperty(5, 0);
        baProperty[0] = static_cast<char>(0x5D);  // lc=3, lp=0, pb=2
        baProperty[1] = static_cast<char>(nDictSize & 0xFF);
        baProperty[2] = static_cast<char>((nDictSize >> 8) & 0xFF);
        baProperty[3] = static_cast<char>((nDictSize >> 16) & 0xFF);
        baProperty[4] = static_cast<char>((nDictSize >> 24) & 0xFF);

        if (!guardedOutput || !guardedSource) {
            bResult = false;
            break;
        }
        SubDevice inputDevice(guardedSource.data(), member.nCompressedOffset, member.nCompressedSize);
        LzipCRC32OutputDevice outputDevice(pStage.get());
        const bool bInputOpen = inputDevice.open(QIODevice::ReadOnly);
        if (!guardedThis || !bInputOpen) {
            if (inputDevice.isOpen()) inputDevice.close();
            bResult = false;
            break;
        }
        const bool bOutputOpen = outputDevice.open(QIODevice::WriteOnly);
        if (!guardedThis || !bOutputOpen) {
            if (inputDevice.isOpen()) inputDevice.close();
            if (outputDevice.isOpen()) outputDevice.close();
            bResult = false;
            break;
        }

        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZMA);
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
        state.mapUnpackProperties = pState->mapUnpackProperties;
        state.pDeviceInput = &inputDevice;
        state.pDeviceOutput = &outputDevice;
        state.nInputOffset = 0;
        state.nInputLimit = member.nCompressedSize;
        state.nProcessedOffset = 0;
        state.nProcessedLimit = -1;

        bResult = XLZMADecoder::decompress(&state, baProperty, pPdStruct) &&
                  guardedThis &&
                  guardedOutput && guardedSource &&
                  (state.nCountInput == member.nCompressedSize) &&
                  (state.nCountOutput == member.nUncompressedSize) &&
                  XBinary::isPdStructNotCanceled(pPdStruct);
        inputDevice.close();
        outputDevice.close();
        if (!guardedThis) return false;

        if (bResult && bCheckCRC && (outputDevice.getCRC32() != member.nCRC32)) {
            XBinary::setPdStructErrorString(
                pPdStruct, guardedThis->tr("CRC check failed"));
            bResult = false;
        }
        if (!bResult) break;
    }

    if (!bResult || !guardedThis || !guardedOutput || !guardedSource) return false;
    const bool bFinalSourceCurrent =
        guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !guardedOutput || !guardedSource ||
        !bFinalSourceCurrent) return false;
    const bool bPublished = guardedThis->publishUnpackOutput(
        pStage.get(), guardedOutput.data(), pState, pPdStruct);
    if (!guardedThis || !bPublished) return false;

    pState->nCurrentOffset = nMemberSize;
    return true;
}

bool XLzip::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XLzip> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState || !pState->pContext ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const bool bSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;

    pState->nCurrentIndex++;
    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool XLzip::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XLzip> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !guardedThis->ownsUnpackSource(pState)) return false;

    LZIP_UNPACK_CONTEXT *pContext =
        static_cast<LZIP_UNPACK_CONTEXT *>(pState->pContext);
    pState->pContext = nullptr;
    guardedThis->releaseUnpackSource(pState);
    if (!guardedThis) return false;
    delete pContext;
    if (!guardedThis) return false;

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    return true;
}

QList<QString> XLzip::getSearchSignatures()
{
    return {"'LZIP'"};
}

XBinary *XLzip::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XLzip(pDevice);
}

bool XLzip::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XArchive::handleInternalInfo(pPdStruct);
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) =
            *static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
    }

    return bResult;
}

void *XLzip::getInternalInfo(PDSTRUCT *pPdStruct)
{
    handleInternalInfo(pPdStruct);

    return &m_internalInfo;
}

void XLzip::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
