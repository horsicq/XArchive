/* Copyright (c) 2017-2026 hors<horsic@gmail.com>
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

#include "xsevenzip.h"

#include <climits>
#include <new>
#include <QSet>

XBinary::XCONVERT _TABLE_XSevenZip_STRUCTID[] = {{XSevenZip::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                 {XSevenZip::STRUCTID_SIGNATUREHEADER, "SIGNATUREHEADER", QString("SIGNATUREHEADER")},
                                                 {XSevenZip::STRUCTID_HEADER, "HEADER", QObject::tr("Header")}};

XBinary::XIDSTRING _TABLE_XSevenZip_EIdEnum[] = {
    {XSevenZip::k7zIdEnd, "End"},
    {XSevenZip::k7zIdHeader, "Header"},
    {XSevenZip::k7zIdArchiveProperties, "ArchiveProperties"},
    {XSevenZip::k7zIdAdditionalStreamsInfo, "AdditionalStreamsInfo"},
    {XSevenZip::k7zIdMainStreamsInfo, "MainStreamsInfo"},
    {XSevenZip::k7zIdFilesInfo, "FilesInfo"},
    {XSevenZip::k7zIdPackInfo, "PackInfo"},
    {XSevenZip::k7zIdUnpackInfo, "UnpackInfo"},
    {XSevenZip::k7zIdSubStreamsInfo, "SubStreamsInfo"},
    {XSevenZip::k7zIdSize, "Size"},
    {XSevenZip::k7zIdCRC, "CRC"},
    {XSevenZip::k7zIdFolder, "Folder"},
    {XSevenZip::k7zIdCodersUnpackSize, "CodersUnpackSize"},
    {XSevenZip::k7zIdNumUnpackStream, "NumUnpackStream"},
    {XSevenZip::k7zIdEmptyStream, "EmptyStream"},
    {XSevenZip::k7zIdEmptyFile, "EmptyFile"},
    {XSevenZip::k7zIdAnti, "Anti"},
    {XSevenZip::k7zIdName, "Name"},
    {XSevenZip::k7zIdCTime, "CTime"},
    {XSevenZip::k7zIdATime, "ATime"},
    {XSevenZip::k7zIdMTime, "MTime"},
    {XSevenZip::k7zIdWinAttrib, "WinAttrib"},
    {XSevenZip::k7zIdComment, "Comment"},
    {XSevenZip::k7zIdEncodedHeader, "EncodedHeader"},
    {XSevenZip::k7zIdStartPos, "StartPos"},
    {XSevenZip::k7zIdDummy, "Dummy"},
};

static const XBinary::XFIXEDFIELD _TABLE_XSevenZip_STRUCTID_SIGNATUREHEADER[] = {
    {"kSignature", (qint32)offsetof(XSevenZip::SIGNATUREHEADER, kSignature), (qint32)sizeof(((XSevenZip::SIGNATUREHEADER *)0)->kSignature), XBinary::XFRECORD_FLAG_NONE,
     XBinary::VT_BYTE_ARRAY},
    {"Major", (qint32)offsetof(XSevenZip::SIGNATUREHEADER, Major), (qint32)sizeof(((XSevenZip::SIGNATUREHEADER *)0)->Major), XBinary::XFRECORD_FLAG_VERSION_MAJOR,
     XBinary::VT_UINT8},
    {"Minor", (qint32)offsetof(XSevenZip::SIGNATUREHEADER, Minor), (qint32)sizeof(((XSevenZip::SIGNATUREHEADER *)0)->Minor), XBinary::XFRECORD_FLAG_VERSION_MINOR,
     XBinary::VT_UINT8},
    {"StartHeaderCRC", (qint32)offsetof(XSevenZip::SIGNATUREHEADER, StartHeaderCRC), (qint32)sizeof(((XSevenZip::SIGNATUREHEADER *)0)->StartHeaderCRC),
     XBinary::XFRECORD_FLAG_NONE, XBinary::VT_UINT32},
    {"NextHeaderOffset", (qint32)offsetof(XSevenZip::SIGNATUREHEADER, NextHeaderOffset), (qint32)sizeof(((XSevenZip::SIGNATUREHEADER *)0)->NextHeaderOffset),
     XBinary::XFRECORD_FLAG_RELATIVE_OFFSET, XBinary::VT_UINT64},
    {"NextHeaderSize", (qint32)offsetof(XSevenZip::SIGNATUREHEADER, NextHeaderSize), (qint32)sizeof(((XSevenZip::SIGNATUREHEADER *)0)->NextHeaderSize),
     XBinary::XFRECORD_FLAG_SIZE, XBinary::VT_UINT64},
    {"NextHeaderCRC", (qint32)offsetof(XSevenZip::SIGNATUREHEADER, NextHeaderCRC), (qint32)sizeof(((XSevenZip::SIGNATUREHEADER *)0)->NextHeaderCRC),
     XBinary::XFRECORD_FLAG_NONE, XBinary::VT_UINT32},
};

const QString XSevenZip::PREFIX_k7zId = "k7zId";
static const qint64 SEVENZIP_MAX_NEXT_HEADER_SIZE = 512LL * 1024 * 1024;
static const quint64 SEVENZIP_MAX_ITEM_COUNT = 1000000;

namespace {
struct SevenZipHashProgressBridge {
    XBinary::PDSTRUCT *pOriginal;
    XBinary::PDSTRUCTLIFETIME originalLifetime;
};

void sevenZipHashProgressCallback(void *pUserData,
                                  XBinary::PDSTRUCT *pLocalProgress)
{
    SevenZipHashProgressBridge *pBridge =
        static_cast<SevenZipHashProgressBridge *>(pUserData);
    if (!pBridge || !pLocalProgress) return;

    if (!XBinary::isPdStructLifetimeAlive(pBridge->originalLifetime) ||
        !XBinary::isPdStructNotCanceled(pBridge->pOriginal)) {
        XBinary::setPdStructStopped(pLocalProgress);
    }
}
}  // namespace

static bool getNextHeaderRange(const XSevenZip::SIGNATUREHEADER &signatureHeader, qint64 nFileSize, qint64 *pOffset, qint64 *pSize)
{
    if (nFileSize < (qint64)sizeof(XSevenZip::SIGNATUREHEADER)) {
        return false;
    }

    quint64 nFileSize64 = (quint64)nFileSize;
    quint64 nBase = sizeof(XSevenZip::SIGNATUREHEADER);

    if (signatureHeader.NextHeaderOffset > (nFileSize64 - nBase)) {
        return false;
    }

    quint64 nOffset = nBase + signatureHeader.NextHeaderOffset;

    if (signatureHeader.NextHeaderSize > (nFileSize64 - nOffset)) {
        return false;
    }

    if (pOffset) {
        *pOffset = (qint64)nOffset;
    }

    if (pSize) {
        *pSize = (qint64)signatureHeader.NextHeaderSize;
    }

    return true;
}

static bool sevenZipCheckCRC32(QIODevice *pDevice, qint64 nOffset, qint64 nSize, quint32 nExpectedCRC, XBinary::PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice) return false;
    const qint64 nDeviceSize = guardedDevice->size();
    if (!guardedDevice || (nOffset < 0) || (nSize < 0) ||
        (nOffset > nDeviceSize) || (nSize > nDeviceSize - nOffset)) {
        return false;
    }

    const qint32 nChunkCapacity = 1024 * 1024;
    QByteArray baChunk(qMin<qint64>(nChunkCapacity, qMax<qint64>(1, nSize)), 0);
    quint32 nCRC = 0xFFFFFFFF;
    qint64 nRemaining = nSize;
    qint64 nCurrentOffset = nOffset;

    while ((nRemaining > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint32 nChunkSize = (qint32)qMin<qint64>(baChunk.size(), nRemaining);

        if (XBinary::read_array_process(guardedDevice.data(), nCurrentOffset,
                                        baChunk.data(), nChunkSize,
                                        pPdStruct) != nChunkSize ||
            !guardedDevice) {
            return false;
        }

        nCRC = XBinary::_getCRC32(baChunk.constData(), nChunkSize, nCRC, XBinary::_getCRC32Table_EDB88320());
        nCurrentOffset += nChunkSize;
        nRemaining -= nChunkSize;
    }

    return (nRemaining == 0) && XBinary::isPdStructNotCanceled(pPdStruct) && ((nCRC ^ 0xFFFFFFFF) == nExpectedCRC);
}

static bool sevenZipParseSignatureHeader(const QByteArray &baHeader,
                                         XSevenZip::SIGNATUREHEADER *pHeader)
{
    if (!pHeader ||
        (baHeader.size() != (qint64)sizeof(XSevenZip::SIGNATUREHEADER))) {
        return false;
    }

    XSevenZip::SIGNATUREHEADER result = {};
    const char *pData = baHeader.constData();
    memcpy(result.kSignature, pData, sizeof(result.kSignature));
    result.Major = XBinary::_read_uint8(pData + 6);
    result.Minor = XBinary::_read_uint8(pData + 7);
    result.StartHeaderCRC = XBinary::_read_uint32(pData + 8);
    result.NextHeaderOffset = XBinary::_read_uint64(pData + 12);
    result.NextHeaderSize = XBinary::_read_uint64(pData + 20);
    result.NextHeaderCRC = XBinary::_read_uint32(pData + 28);
    *pHeader = result;
    return true;
}

static bool sevenZipSignatureMatches(const XSevenZip::SIGNATUREHEADER &signatureHeader)
{
    static const quint8 kSignature[6] = {0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C};
    return (memcmp(signatureHeader.kSignature, kSignature, sizeof(kSignature)) == 0) && (signatureHeader.Major == 0);
}

static XBinary::PM_INFO createPMInfo(XBinary::HANDLE_METHOD hm0, XBinary::HANDLE_METHOD hm1 = XBinary::HANDLE_METHOD_UNKNOWN,
                                     XBinary::HANDLE_METHOD hm2 = XBinary::HANDLE_METHOD_UNKNOWN, XBinary::HANDLE_METHOD hm3 = XBinary::HANDLE_METHOD_UNKNOWN)
{
    XBinary::PM_INFO result = {};

    result.hm[0] = hm0;
    result.hm[1] = hm1;
    result.hm[2] = hm2;
    result.hm[3] = hm3;

    return result;
}

// Resolve a folder in-stream index to its global pack-stream index. Tries a direct
// lookup first; if that fails, follows one coder bond to handle AES-encrypted
// sub-streams (the encrypted BCJ2 layout). When resolved via a bond, *pAESCoderIdx
// receives that producer coder's local index.
static qint32 sevenzipResolveInStream(const QMap<qint32, qint32> &mapInStreamToGlobal, const QMap<qint32, qint32> &mapBondInToCoderOut,
                                      const QList<qint32> &listInStreamOffsets, qint32 nNumCoders, qint32 nInStream, qint32 *pAESCoderIdx)
{
    if (pAESCoderIdx) *pAESCoderIdx = -1;
    if (mapInStreamToGlobal.contains(nInStream)) {
        return mapInStreamToGlobal.value(nInStream);
    }
    if (mapBondInToCoderOut.contains(nInStream)) {
        qint32 nProducerCoder = mapBondInToCoderOut.value(nInStream);
        if (nProducerCoder >= 0 && nProducerCoder < nNumCoders) {
            qint32 nProducerInStream = listInStreamOffsets.at(nProducerCoder);
            if (mapInStreamToGlobal.contains(nProducerInStream)) {
                if (pAESCoderIdx) *pAESCoderIdx = nProducerCoder;
                return mapInStreamToGlobal.value(nProducerInStream);
            }
        }
    }
    return -1;
}

QMap<quint64, QString> XSevenZip::get_k7zId()
{
    return XBinary::XIDSTRING_createMapPrefix(_TABLE_XSevenZip_EIdEnum, sizeof(_TABLE_XSevenZip_EIdEnum) / sizeof(XBinary::XIDSTRING), PREFIX_k7zId);
}

QMap<quint64, QString> XSevenZip::get_k7zId_s()
{
    return XBinary::XIDSTRING_createMap(_TABLE_XSevenZip_EIdEnum, sizeof(_TABLE_XSevenZip_EIdEnum) / sizeof(XBinary::XIDSTRING));
}

XSevenZip::XSevenZip(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XSevenZip::_loadValidatedNextHeader(QByteArray *pData, qint64 *pNextHeaderOffset, PDSTRUCT *pPdStruct)
{
    QPointer<XSevenZip> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pData || !guardedSource ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    pData->clear();
    const qint64 nFileSize = guardedSource->size();
    if (!guardedArchive || !guardedSource ||
        (nFileSize < (qint64)sizeof(SIGNATUREHEADER))) return false;

    const QByteArray baSignature = XBinary::read_array_process(
        guardedSource.data(), 0, sizeof(SIGNATUREHEADER), pPdStruct);
    SIGNATUREHEADER signatureHeader = {};
    if (!guardedArchive || !guardedSource ||
        !sevenZipParseSignatureHeader(baSignature, &signatureHeader)) {
        return false;
    }
    qint64 nNextHeaderOffset = 0;
    qint64 nNextHeaderSize = 0;

    if (!sevenZipSignatureMatches(signatureHeader) ||
        !getNextHeaderRange(signatureHeader, nFileSize, &nNextHeaderOffset, &nNextHeaderSize) ||
        (nNextHeaderSize > SEVENZIP_MAX_NEXT_HEADER_SIZE) ||
        !sevenZipCheckCRC32(guardedSource.data(), 12, 20,
                            signatureHeader.StartHeaderCRC, pPdStruct) ||
        !guardedArchive || !guardedSource ||
        !sevenZipCheckCRC32(guardedSource.data(), nNextHeaderOffset,
                            nNextHeaderSize, signatureHeader.NextHeaderCRC,
                            pPdStruct) ||
        !guardedArchive || !guardedSource) {
        return false;
    }

    if (nNextHeaderSize > 0) {
        // nNextHeaderSize is already bounded by SEVENZIP_MAX_NEXT_HEADER_SIZE above.
        pData->resize((qint32)nNextHeaderSize);
        if (pData->size() != (qint32)nNextHeaderSize) {
            pData->clear();
            return false;
        }

        if (XBinary::read_array_process(guardedSource.data(),
                                        nNextHeaderOffset, pData->data(),
                                        nNextHeaderSize, pPdStruct) !=
                nNextHeaderSize ||
            !guardedArchive || !guardedSource) {
            pData->clear();
            return false;
        }

        quint8 nHeaderId = (quint8)pData->at(0);
        if ((nHeaderId != (quint8)k7zIdHeader) && (nHeaderId != (quint8)k7zIdEncodedHeader)) {
            pData->clear();
            return false;
        }
    }

    if (pNextHeaderOffset) {
        *pNextHeaderOffset = nNextHeaderOffset;
    }

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

QList<XBinary::PM_INFO> XSevenZip::unpackImplemented()
{
    QList<XBinary::PM_INFO> listResult;

    static const HANDLE_METHOD g_7zUnpackMethods[] = {
        HANDLE_METHOD_STORE, HANDLE_METHOD_LZMA, HANDLE_METHOD_LZMA2, HANDLE_METHOD_PPMD7, HANDLE_METHOD_BZIP2, HANDLE_METHOD_DEFLATE, HANDLE_METHOD_DEFLATE64,
    };

    static const HANDLE_METHOD g_7zFilters[] = {
        HANDLE_METHOD_BCJ, HANDLE_METHOD_ARM64_BCJ, HANDLE_METHOD_ARM_BCJ,  HANDLE_METHOD_ARMT_BCJ,
        HANDLE_METHOD_PPC_BCJ, HANDLE_METHOD_SPARC_BCJ, HANDLE_METHOD_IA64_BCJ, HANDLE_METHOD_DELTA,
    };

    const qint32 nNumberOfMethods = sizeof(g_7zUnpackMethods) / sizeof(g_7zUnpackMethods[0]);
    const qint32 nNumberOfFilters = sizeof(g_7zFilters) / sizeof(g_7zFilters[0]);

    for (qint32 i = 0; i < nNumberOfMethods; i++) {
        const HANDLE_METHOD method = g_7zUnpackMethods[i];

        listResult.append(createPMInfo(method));
        listResult.append(createPMInfo(method, HANDLE_METHOD_7Z_AES));

        for (qint32 j = 0; j < nNumberOfFilters; j++) {
            const HANDLE_METHOD filter = g_7zFilters[j];

            listResult.append(createPMInfo(filter, method));
            listResult.append(createPMInfo(filter, method, HANDLE_METHOD_7Z_AES));
        }
    }

    listResult.append(createPMInfo(HANDLE_METHOD_BCJ2));
    listResult.append(createPMInfo(HANDLE_METHOD_BCJ2, HANDLE_METHOD_7Z_AES));

    return listResult;
}

bool XSevenZip::isValid(PDSTRUCT *pPdStruct)
{
    QByteArray baHeader;
    return _loadValidatedNextHeader(&baHeader, nullptr, pPdStruct);
}

bool XSevenZip::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSevenZip xsevenzip(pDevice);

    return xsevenzip.isValid(pPdStruct);
}

QString XSevenZip::getVersion()
{
    return QString("%1.%2").arg(read_uint8(6)).arg(read_uint8(7), 1, 10, QChar('0'));
}

bool XSevenZip::isEncrypted()
{
    QByteArray baData;
    qint64 nNextHeaderOffset = 0;

    if (!_loadValidatedNextHeader(&baData, &nNextHeaderOffset, nullptr) || baData.isEmpty()) {
        return false;
    }

    bool bIsEncodedHeader = ((quint8)baData.at(0) == (quint8)k7zIdEncodedHeader);

    if (bIsEncodedHeader) {
        QList<SZRECORD> listRecords;
        SZSTATE encodedState = {};
        encodedState.pData = baData.data();
        encodedState.nSize = baData.size();

        bool bEncodedHeaderParsed = _handleId(&listRecords, k7zIdEncodedHeader, &encodedState, 1, true, nullptr, IMPTYPE_UNKNOWN);

        if (!bEncodedHeaderParsed || encodedState.bIsError || (encodedState.nCurrentOffset != encodedState.nSize) ||
            !_validateEncodedHeader(&encodedState, nNextHeaderOffset)) {
            return false;
        }

        if (encodedState.bIsEncrypted) {
            return true;
        }

        baData.clear();
        QBuffer bufferOut;
        bufferOut.setBuffer(&baData);

        if (!bufferOut.open(QIODevice::ReadWrite)) {
            return false;
        }

        QMap<UNPACK_PROP, QVariant> mapProperties;
        bool bHeaderDecompressed = decompressHeader(mapProperties, &bufferOut, &encodedState, nullptr);
        bufferOut.close();

        if (!bHeaderDecompressed) {
            return false;
        }
    }

    QList<SZRECORD> listRecords;
    SZSTATE state = {};
    state.pData = baData.data();
    state.nSize = baData.size();

    bool bHeaderParsed = _handleId(&listRecords, k7zIdHeader, &state, 1, true, nullptr, IMPTYPE_UNKNOWN);
    return bHeaderParsed && !state.bIsError && (state.nCurrentOffset == state.nSize) &&
           _validateParsedHeader(&state, nNextHeaderOffset, nullptr) && state.bIsEncrypted;
}

qint64 XSevenZip::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    SIGNATUREHEADER signatureHeader = _read_SIGNATUREHEADER(0);
    qint64 nNextHeaderOffset = 0;
    qint64 nNextHeaderSize = 0;

    if (sevenZipSignatureMatches(signatureHeader) &&
        getNextHeaderRange(signatureHeader, getSize(), &nNextHeaderOffset, &nNextHeaderSize) &&
        (nNextHeaderSize <= SEVENZIP_MAX_NEXT_HEADER_SIZE) &&
        sevenZipCheckCRC32(getDevice(), 12, 20, signatureHeader.StartHeaderCRC, pPdStruct) &&
        sevenZipCheckCRC32(getDevice(), nNextHeaderOffset, nNextHeaderSize, signatureHeader.NextHeaderCRC, pPdStruct) &&
        (nNextHeaderSize <= (LLONG_MAX - nNextHeaderOffset))) {
        return nNextHeaderOffset + nNextHeaderSize;
    }

    return 0;
}

QString XSevenZip::getFileFormatExt()
{
    return "7z";
}

QString XSevenZip::getFileFormatExtsString()
{
    return "7-Zip (*.7z)";
}

XBinary::MODE XSevenZip::getMode()
{
    return XBinary::MODE_DATA;
}

QString XSevenZip::getMIMEString()
{
    return "application/x-7z-compressed";
}

bool XSevenZip::isCommentPresent()
{
    return !getComment().isEmpty();
}

QString XSevenZip::getComment()
{
    QByteArray baData;
    qint64 nNextHeaderOffset = 0;

    if (!_loadValidatedNextHeader(&baData, &nNextHeaderOffset, nullptr) || baData.isEmpty()) {
        return QString();
    }

    if ((quint8)baData.at(0) == (quint8)k7zIdEncodedHeader) {
        QList<SZRECORD> encodedRecords;
        SZSTATE encodedState = {};
        encodedState.pData = baData.data();
        encodedState.nSize = baData.size();

        if (!_handleId(&encodedRecords, k7zIdEncodedHeader, &encodedState, 1, true, nullptr, IMPTYPE_UNKNOWN) ||
            encodedState.bIsError || encodedState.bIsEncrypted || (encodedState.nCurrentOffset != encodedState.nSize) ||
            !_validateEncodedHeader(&encodedState, nNextHeaderOffset)) {
            return QString();
        }

        baData.clear();
        QBuffer bufferOut(&baData);
        if (!bufferOut.open(QIODevice::ReadWrite)) {
            return QString();
        }

        QMap<UNPACK_PROP, QVariant> mapProperties;
        bool bDecompressed = decompressHeader(mapProperties, &bufferOut, &encodedState, nullptr);
        bufferOut.close();
        if (!bDecompressed) {
            return QString();
        }
    }

    QList<SZRECORD> records;
    SZSTATE state = {};
    state.pData = baData.data();
    state.nSize = baData.size();

    if (!_handleId(&records, k7zIdHeader, &state, 1, true, nullptr, IMPTYPE_UNKNOWN) || state.bIsError ||
        (state.nCurrentOffset != state.nSize) || !_validateParsedHeader(&state, nNextHeaderOffset, nullptr) || state.bIsEncrypted) {
        return QString();
    }

    return state.baComment.isEmpty() ? QString() : QString::fromUtf8(state.baComment);
}

QString XSevenZip::getArch()
{
    return QString();
}

XSevenZip::SIGNATUREHEADER XSevenZip::_read_SIGNATUREHEADER(qint64 nOffset)
{
    SIGNATUREHEADER result = {};

    read_array(nOffset, (char *)result.kSignature, 6);
    result.Major = read_uint8(nOffset + 6);
    result.Minor = read_uint8(nOffset + 7);
    result.StartHeaderCRC = read_uint32(nOffset + 8);
    result.NextHeaderOffset = read_uint64(nOffset + 12);
    result.NextHeaderSize = read_uint64(nOffset + 20);
    result.NextHeaderCRC = read_uint32(nOffset + 28);

    return result;
}

XBinary::ENDIAN XSevenZip::getEndian()
{
    return ENDIAN_LITTLE;
}

QList<XBinary::MAPMODE> XSevenZip::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_DATA);
    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XSevenZip::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;

    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_REGION | FILEPART_OVERLAY, pPdStruct);
    }

    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

XBinary::FT XSevenZip::getFileType()
{
    return FT_7Z;
}

QString XSevenZip::idToSring(XSevenZip::EIdEnum id)
{
    return XBinary::XIDSTRING_idToString((quint32)id, _TABLE_XSevenZip_EIdEnum, sizeof(_TABLE_XSevenZip_EIdEnum) / sizeof(XBinary::XIDSTRING));
}

XBinary::HANDLE_METHOD XSevenZip::coderToCompressMethod(const QByteArray &baCodec)
{
    HANDLE_METHOD result = HANDLE_METHOD_UNKNOWN;

    if (baCodec.isEmpty()) {
        return result;
    }

    // Check 1-byte codecs first
    if (baCodec.size() == 1) {
        if (baCodec[0] == '\x00') {
            result = HANDLE_METHOD_STORE;  // Copy (uncompressed)
        } else if (baCodec[0] == '\x21') {
            result = HANDLE_METHOD_LZMA2;  // LZMA2
        } else if (baCodec[0] == '\x03') {
            result = HANDLE_METHOD_DELTA;  // Delta filter (1-byte codec ID)
        } else if (baCodec[0] == '\x04') {
            result = HANDLE_METHOD_BCJ;  // x86 branch filter (1-byte codec ID)
        } else if (baCodec[0] == '\x05') {
            result = HANDLE_METHOD_PPC_BCJ;  // PPC branch filter (1-byte codec ID)
        } else if (baCodec[0] == '\x06') {
            result = HANDLE_METHOD_IA64_BCJ;  // IA64 branch filter (1-byte codec ID)
        } else if (baCodec[0] == '\x07') {
            result = HANDLE_METHOD_ARM_BCJ;  // ARM branch filter (1-byte codec ID)
        } else if (baCodec[0] == '\x08') {
            result = HANDLE_METHOD_ARMT_BCJ;  // ARM Thumb branch filter (1-byte codec ID)
        } else if (baCodec[0] == '\x09') {
            result = HANDLE_METHOD_SPARC_BCJ;  // SPARC branch filter (1-byte codec ID)
        } else if (baCodec[0] == '\x0A') {
            result = HANDLE_METHOD_ARM64_BCJ;  // ARM64 branch filter (1-byte codec ID)
        }
    } else if (baCodec.size() >= 3) {
        // 7-Zip codec IDs are typically 3+ bytes
        // Common codecs (from 7-Zip specification)
        if (baCodec.startsWith(QByteArray("\x00", 1))) {
            result = HANDLE_METHOD_STORE;  // Copy (uncompressed)
        } else if (baCodec.startsWith(QByteArray("\x03\x01\x01", 3))) {
            result = HANDLE_METHOD_LZMA;  // LZMA
        } else if (baCodec.startsWith(QByteArray("\x04\x01\x08", 3))) {
            result = HANDLE_METHOD_DEFLATE;  // Deflate
        } else if (baCodec.startsWith(QByteArray("\x04\x01\x09", 3))) {
            result = HANDLE_METHOD_DEFLATE64;  // Deflate64
        } else if (baCodec.startsWith(QByteArray("\x04\x02\x02", 3))) {
            result = HANDLE_METHOD_BZIP2;  // BZip2
        } else if (baCodec.startsWith(QByteArray("\x03\x04\x01", 3))) {
            result = HANDLE_METHOD_PPMD7;  // PPMd (actual codec from 7z)
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x01\x03", 4))) {
            result = HANDLE_METHOD_BCJ;  // BCJ (x86 E8/E9 filter, single stream)
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x01\x1b", 4))) {
            result = HANDLE_METHOD_BCJ2;  // BCJ2 (x86 4-stream filter) - fix for issue 1
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x01\x0A", 4))) {
            result = HANDLE_METHOD_ARM64_BCJ;  // ARM64 branch/call/jump filter (4-byte codec variant)
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x01\x01", 4))) {
            result = HANDLE_METHOD_PPMD7;  // PPMd (alternative codec)
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x02\x05", 4))) {
            result = HANDLE_METHOD_PPC_BCJ;  // PPC (big-endian) branch filter (legacy 4-byte codec ID)
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x04\x01", 4))) {
            result = HANDLE_METHOD_IA64_BCJ;  // IA64 branch filter (legacy 4-byte codec ID)
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x05\x01", 4))) {
            result = HANDLE_METHOD_ARM_BCJ;  // ARM (little-endian) branch filter (legacy 4-byte codec ID)
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x07\x01", 4))) {
            result = HANDLE_METHOD_ARMT_BCJ;  // ARM Thumb (little-endian) branch filter (legacy 4-byte codec ID)
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x08\x05", 4))) {
            result = HANDLE_METHOD_SPARC_BCJ;  // SPARC branch filter (legacy 4-byte codec ID)
        } else if (baCodec.startsWith(QByteArray("\x06\xF1\x07\x01", 4))) {
            result = HANDLE_METHOD_7Z_AES;  // AES encryption
        } else {
#ifdef QT_DEBUG
            qDebug() << "[CODEC] Unknown codec:" << baCodec.toHex() << "size=" << baCodec.size();
#endif
        }
    }

    return result;
}

QString XSevenZip::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XSevenZip_STRUCTID, sizeof(_TABLE_XSevenZip_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XSevenZip::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XSevenZip_STRUCTID, sizeof(_TABLE_XSevenZip_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XSevenZip::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XSevenZip_STRUCTID, sizeof(_TABLE_XSevenZip_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// QList<XBinary::DATA_HEADER> XSevenZip::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

//         _dataHeadersOptions.nID = STRUCTID_SIGNATUREHEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             if (dataHeadersOptions.nID == STRUCTID_SIGNATUREHEADER) {
//                 DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XSevenZip::structIDToString(dataHeadersOptions.nID));
//                 dataHeader.nSize = sizeof(SIGNATUREHEADER);

//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(SIGNATUREHEADER, kSignature), 6, "kSignature", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(SIGNATUREHEADER, Major), 1, "Major", VT_UINT8, DRF_UNKNOWN,
//                 dataHeadersOptions.pMemoryMap->endian)); dataHeader.listRecords.append(getDataRecord(offsetof(SIGNATUREHEADER, Minor), 1, "Minor", VT_UINT8,
//                 DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian)); dataHeader.listRecords.append(
//                     getDataRecord(offsetof(SIGNATUREHEADER, StartHeaderCRC), 4, "StartHeaderCRC", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(SIGNATUREHEADER, NextHeaderOffset), 8, "NextHeaderOffset", VT_UINT64, DRF_OFFSET, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(SIGNATUREHEADER, NextHeaderSize), 8, "NextHeaderSize", VT_UINT64, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(SIGNATUREHEADER, NextHeaderCRC), 4, "NextHeaderCRC", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);

//                 if (dataHeadersOptions.bChildren) {
//                     SIGNATUREHEADER signatureHeader = _read_SIGNATUREHEADER(nStartOffset);
//                     qint64 nNextHeaderFileOffset = 0;
//                     qint64 nNextHeaderSize = 0;
//                     bool bNextHeaderValid = getNextHeaderRange(signatureHeader, getSize(), &nNextHeaderFileOffset, &nNextHeaderSize);
//                     // Add hex for StartHeader (the 3 fields after StartHeaderCRC)
//                     {
//                         const qint64 startHeaderHexOff = nStartOffset + 12;  // bytes 12..31
//                         const qint64 startHeaderHexSize = 20;
//                         if (isOffsetAndSizeValid(dataHeadersOptions.pMemoryMap, startHeaderHexOff, startHeaderHexSize)) {
//                             DATA_HEADER hexStart = _dataHeaderHex(dataHeadersOptions, QString("%1").arg("StartHeader (hex)"), dataHeader.dsID, XBinary::STRUCTID_HEX,
//                                                                   startHeaderHexOff, startHeaderHexSize);
//                             listResult.append(hexStart);
//                         }
//                     }

//                     if (bNextHeaderValid) {
//                         qint64 nNextHeaderDelta = nNextHeaderFileOffset - nStartOffset;

//                         if (nNextHeaderDelta >= 0) {
//                             DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//                             _dataHeadersOptions.nLocation += nNextHeaderDelta;
//                             _dataHeadersOptions.nSize = nNextHeaderSize;
//                             _dataHeadersOptions.dsID_parent = dataHeader.dsID;
//                             _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//                             _dataHeadersOptions.nID = STRUCTID_HEADER;
//                             listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//                         }
//                     }

//                     // Add hex view for NextHeader block
//                     if (bNextHeaderValid && (nNextHeaderSize > 0) &&
//                         isOffsetAndSizeValid(dataHeadersOptions.pMemoryMap, nNextHeaderFileOffset, nNextHeaderSize)) {
//                         DATA_HEADER hexNext = _dataHeaderHex(dataHeadersOptions, QString("%1").arg("NextHeader (hex)"), dataHeader.dsID, XBinary::STRUCTID_HEX,
//                                                              nNextHeaderFileOffset, nNextHeaderSize);
//                         listResult.append(hexNext);
//                     }
//                 }
//             } else if (dataHeadersOptions.nID == STRUCTID_HEADER) {
//                 DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XSevenZip::structIDToString(dataHeadersOptions.nID));
//                 dataHeader.nSize = dataHeadersOptions.nSize;

//                 char *pData = new char[dataHeadersOptions.nSize];
//                 qint64 nBytesRead = read_array_process(nStartOffset, pData, dataHeadersOptions.nSize, pPdStruct);

//                 QList<XSevenZip::SZRECORD> listRecords;
//                 if (nBytesRead == dataHeadersOptions.nSize) {
//                     listRecords = _handleData(pData, dataHeadersOptions.nSize, pPdStruct);
//                 }

//                 qint32 nNumberOfRecords = listRecords.count();

//                 for (qint32 i = 0; i < nNumberOfRecords; i++) {
//                     XSevenZip::SZRECORD szRecord = listRecords.at(i);

//                     DATA_RECORD dataRecord = {};
//                     dataRecord.nRelOffset = szRecord.nRelOffset;
//                     dataRecord.nSize = szRecord.nSize;
//                     dataRecord.sName = szRecord.sName;
//                     dataRecord.valType = szRecord.valType;
//                     dataRecord.nFlags = szRecord.nFlags;
//                     dataRecord.endian = dataHeadersOptions.pMemoryMap->endian;

//                     if (szRecord.srType == SRTYPE_ID) {
//                         DATAVALUESET dataValueSet;
//                         dataValueSet.mapValues = get_k7zId_s();
//                         dataValueSet.vlType = VL_TYPE_LIST;
//                         dataValueSet.nMask = 0xFFFFFFFFFFFFFFFF;
//                         dataRecord.listDataValueSets.append(dataValueSet);
//                     }

//                     dataHeader.listRecords.append(dataRecord);
//                 }

//                 listResult.append(dataHeader);

//                 if (dataHeadersOptions.bChildren && (dataHeadersOptions.nSize > 0)) {
//                     // Also add hex view for this parsed header block
//                     DATA_HEADER hexHdr = _dataHeaderHex(dataHeadersOptions, QString("%1").arg("Header (hex)"), dataHeader.dsID, XBinary::STRUCTID_HEX, nStartOffset,
//                                                         dataHeadersOptions.nSize);
//                     listResult.append(hexHdr);
//                 }

//                 delete[] pData;
//             }
//         }
//     }

//     return listResult;
// }

QList<XBinary::XFHEADER> XSevenZip::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_SIGNATUREHEADER;
        _xfStruct.xLoc = offsetToLoc(0);

        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_SIGNATUREHEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }
        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if ((nHeaderOffset != -1) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, sizeof(SIGNATUREHEADER))) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_SIGNATUREHEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = sizeof(SIGNATUREHEADER);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_SIGNATUREHEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_SIGNATUREHEADER), xfHeader.sParentTag);

            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XSevenZip::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_SIGNATUREHEADER) {
        listResult =
            XBinary::XFIXEDFIELD_toXFRecords(_TABLE_XSevenZip_STRUCTID_SIGNATUREHEADER, sizeof(_TABLE_XSevenZip_STRUCTID_SIGNATUREHEADER) / sizeof(XBinary::XFIXEDFIELD));
    }

    return listResult;
}

static bool sevenZipCanAppend(qint32 nLimit, const QList<XBinary::FPART> *pListResult)
{
    return (nLimit == -1) || (pListResult->size() < nLimit);
}

QList<XBinary::FPART> XSevenZip::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    const qint64 nFileSize = getSize();
    if (nFileSize < (qint64)sizeof(SIGNATUREHEADER)) return listResult;

    SIGNATUREHEADER sh = _read_SIGNATUREHEADER(0);
    const qint64 nBase = sizeof(SIGNATUREHEADER);
    qint64 nextHeaderOffset = 0;
    qint64 nextHeaderSize = 0;

    qint64 nMaxOffset = nBase;

    if (getNextHeaderRange(sh, nFileSize, &nextHeaderOffset, &nextHeaderSize)) {
        nMaxOffset = nextHeaderOffset + nextHeaderSize;
    }

    if ((nFileParts & FILEPART_HEADER) && sevenZipCanAppend(nLimit, &listResult)) {
        // Signature header
        FPART hdr = {};
        hdr.filePart = FILEPART_HEADER;
        hdr.nFileOffset = 0;
        hdr.nFileSize = qMin<qint64>((qint64)sizeof(SIGNATUREHEADER), nFileSize);
        hdr.nVirtualAddress = XADDR_MAX;
        hdr.sName = tr("Header");
        listResult.append(hdr);
    }

    if ((nFileParts & FILEPART_REGION) && sevenZipCanAppend(nLimit, &listResult)) {
        // Packed streams between signature header and next header
        qint64 nDataOff = nBase;
        qint64 nDataSize = 0;
        if (nextHeaderOffset > nBase) {
            nDataSize = nextHeaderOffset - nBase;

            FPART data = {};
            data.filePart = FILEPART_REGION;
            data.nFileOffset = nDataOff;
            data.nFileSize = nDataSize;
            data.nVirtualAddress = XADDR_MAX;
            data.sName = tr("Data");
            listResult.append(data);
        }
    }

    if ((nFileParts & FILEPART_HEADER) && sevenZipCanAppend(nLimit, &listResult)) {
        // Next header block
        if ((nextHeaderSize > 0) && (nextHeaderOffset >= 0) && (nextHeaderOffset + nextHeaderSize) <= nFileSize) {
            FPART nh = {};
            nh.filePart = FILEPART_HEADER;
            nh.nFileOffset = nextHeaderOffset;
            nh.nFileSize = nextHeaderSize;
            nh.nVirtualAddress = XADDR_MAX;
            nh.sName = QString("NEXT_HEADER");
            listResult.append(nh);
        }
    }

    if ((nFileParts & FILEPART_DATA) && sevenZipCanAppend(nLimit, &listResult)) {
        FPART nh = {};
        nh.filePart = FILEPART_DATA;
        nh.nFileOffset = 0;
        nh.nFileSize = nMaxOffset;
        nh.nVirtualAddress = XADDR_MAX;
        nh.sName = tr("Data");
        listResult.append(nh);
    }

    if ((nFileParts & FILEPART_OVERLAY) && sevenZipCanAppend(nLimit, &listResult)) {
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

qint64 XSevenZip::getImageSize()
{
    // Not an in-memory image; use file size
    return getSize();
}

bool XSevenZip::_isNextId(SZSTATE *pState, EIdEnum nextId)
{
    if (pState->bIsError || (pState->nCurrentOffset < 0) || (pState->nCurrentOffset >= pState->nSize)) {
        return false;
    }

    XBinary::PACKED_UINT nextTag =
        XBinary::_read_packedNumber(pState->pData + pState->nCurrentOffset, pState->nSize - pState->nCurrentOffset);
    return nextTag.bIsValid && (nextTag.nValue == (quint64)nextId);
}

bool XSevenZip::_handleId(QList<SZRECORD> *pListRecords, EIdEnum id, SZSTATE *pState, qint32 nCount, bool bCheck, PDSTRUCT *pPdStruct, IMPTYPE impType)
{
    // Early exit checks
    if (!pListRecords || !pState || isPdStructStopped(pPdStruct) || pState->bIsError) {
        return false;
    }

    if (!pState->pData || (pState->nCurrentOffset < 0) || (pState->nCurrentOffset >= pState->nSize)) {
        if (bCheck) {
            pState->bIsError = true;
            pState->sErrorString = tr("Unexpected end of 7z header");
        }
        return false;
    }

    bool bResult = false;

    XBinary::PACKED_UINT puTag = XBinary::_read_packedNumber(pState->pData + pState->nCurrentOffset, pState->nSize - pState->nCurrentOffset);

    if (!puTag.bIsValid) {
        if (bCheck) {
            pState->bIsError = true;
            pState->sErrorString = QString("%1: %2").arg(XBinary::valueToHexEx(pState->nCurrentOffset)).arg(tr("Invalid data"));
#ifdef QT_DEBUG
            qDebug("Invalid packed number at offset: 0x%llX", (qint64)pState->nCurrentOffset);
#endif
        }
        return false;
    }

    // Check if this tag matches the expected ID
    if (puTag.nValue != id) {
        if (bCheck) {
            pState->bIsError = true;
            pState->sErrorString = QString("%1: %2").arg(XBinary::valueToHexEx(pState->nCurrentOffset)).arg(tr("Invalid data"));
#ifdef QT_DEBUG
            qDebug("Invalid value: 0x%llX (expected: 0x%llX) at offset 0x%llX", (quint64)puTag.nValue, (quint64)id, pState->nCurrentOffset);
#endif
        }
        return false;
    }

    // Add ID record
    SZRECORD record = {};
    record.nRelOffset = pState->nCurrentOffset;
    record.nSize = puTag.nByteSize;
    record.varValue = puTag.nValue;
    record.srType = SRTYPE_ID;
    record.valType = VT_PACKEDNUMBER;
    record.sName = get_k7zId().value(puTag.nValue);
    pListRecords->append(record);

    pState->nCurrentOffset += puTag.nByteSize;

    // Process ID-specific data
    switch (id) {
        case XSevenZip::k7zIdHeader: {
            if (_isNextId(pState, XSevenZip::k7zIdArchiveProperties) &&
                !_handleId(pListRecords, XSevenZip::k7zIdArchiveProperties, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN)) {
                break;
            }

            if (_isNextId(pState, XSevenZip::k7zIdAdditionalStreamsInfo)) {
                SZSTATE additionalState = {};
                additionalState.pData = pState->pData;
                additionalState.nSize = pState->nSize;
                additionalState.nCurrentOffset = pState->nCurrentOffset;

                if (!_handleId(pListRecords, XSevenZip::k7zIdAdditionalStreamsInfo, &additionalState, 1, true, pPdStruct, IMPTYPE_UNKNOWN) ||
                    additionalState.bIsError) {
                    pState->bIsError = true;
                    pState->sErrorString = additionalState.sErrorString.isEmpty() ? tr("Invalid additional 7z metadata streams")
                                                                                 : additionalState.sErrorString;
                    break;
                }

                pState->nCurrentOffset = additionalState.nCurrentOffset;
                pState->bIsEncrypted = pState->bIsEncrypted || additionalState.bIsEncrypted;
                pState->nMaximumPackStreamEnd = qMax(pState->nMaximumPackStreamEnd, additionalState.nMaximumPackStreamEnd);
            }

            if (_isNextId(pState, XSevenZip::k7zIdMainStreamsInfo) &&
                !_handleId(pListRecords, XSevenZip::k7zIdMainStreamsInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN)) {
                break;
            }

            if (_isNextId(pState, XSevenZip::k7zIdFilesInfo) &&
                !_handleId(pListRecords, XSevenZip::k7zIdFilesInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN)) {
                break;
            }

            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);

            break;
        }

        case XSevenZip::k7zIdAdditionalStreamsInfo:
        case XSevenZip::k7zIdMainStreamsInfo: {
            if (_isNextId(pState, XSevenZip::k7zIdPackInfo) &&
                !_handleId(pListRecords, XSevenZip::k7zIdPackInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN)) {
                break;
            }

            if (_isNextId(pState, XSevenZip::k7zIdUnpackInfo) &&
                !_handleId(pListRecords, XSevenZip::k7zIdUnpackInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN)) {
                break;
            }

            if (_isNextId(pState, XSevenZip::k7zIdSubStreamsInfo) &&
                !_handleId(pListRecords, XSevenZip::k7zIdSubStreamsInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN)) {
                break;
            }

            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            break;
        }

        case XSevenZip::k7zIdArchiveProperties: {
            while (!pState->bIsError && isPdStructNotCanceled(pPdStruct)) {
                if (_isNextId(pState, XSevenZip::k7zIdEnd)) {
                    bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
                    break;
                }

                quint64 nPropertyId = _handleNumber(pListRecords, pState, pPdStruct, "ArchivePropertyId", DRF_UNKNOWN, IMPTYPE_UNKNOWN);
                Q_UNUSED(nPropertyId)
                quint64 nPropertySize = _handleNumber(pListRecords, pState, pPdStruct, "ArchivePropertySize", DRF_SIZE, IMPTYPE_UNKNOWN);

                if (pState->bIsError || (nPropertySize > (quint64)INT_MAX)) {
                    pState->bIsError = true;
                    pState->sErrorString = tr("Invalid 7z archive property");
                    break;
                }

                _handleArray(pListRecords, pState, (qint64)nPropertySize, pPdStruct, "ArchivePropertyData", IMPTYPE_UNKNOWN);
            }
            break;
        }

        case XSevenZip::k7zIdPackInfo: {
            quint64 nPackPosition = _handleNumber(pListRecords, pState, pPdStruct, "PackPosition", DRF_OFFSET, IMPTYPE_STREAMOFFSET);

            if (pState->bIsError || (nPackPosition > (quint64)(LLONG_MAX - (qint64)sizeof(SIGNATUREHEADER)))) {
                pState->bIsError = true;
                pState->sErrorString = tr("Invalid 7z pack position");
                break;
            }
            pState->nStreamsBegin = (qint64)sizeof(SIGNATUREHEADER) + (qint64)nPackPosition;

            quint64 nNumberOfPackStreams = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfPackStreams", DRF_COUNT, IMPTYPE_NUMBEROFPACKSTREAMS);

            if (pState->bIsError || (nNumberOfPackStreams > SEVENZIP_MAX_ITEM_COUNT)) {
                pState->bIsError = true;
                pState->sErrorString = tr("Excessive 7z pack-stream count");
                break;
            }

            for (int i = 0; i < nNumberOfPackStreams; i++) {
                SZINSTREAM szStream = {};
                pState->listInStreams.append(szStream);
            }

            if (!_handleId(pListRecords, XSevenZip::k7zIdSize, pState, (qint32)nNumberOfPackStreams, true, pPdStruct, IMPTYPE_STREAMSIZE)) {
                break;
            }
            if (_isNextId(pState, XSevenZip::k7zIdCRC) &&
                !_handleId(pListRecords, XSevenZip::k7zIdCRC, pState, (qint32)nNumberOfPackStreams, true, pPdStruct, IMPTYPE_STREAMCRC)) {
                break;
            }
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            break;
        }

        case XSevenZip::k7zIdUnpackInfo:
            if (!_handleId(pListRecords, XSevenZip::k7zIdFolder, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN) ||
                !_handleId(pListRecords, XSevenZip::k7zIdCodersUnpackSize, pState, (qint32)pState->nNumberOfCoders, true, pPdStruct,
                           IMPTYPE_CODERUNPACKEDSIZE)) {
                break;
            }
            if (_isNextId(pState, XSevenZip::k7zIdCRC) &&
                !_handleId(pListRecords, XSevenZip::k7zIdCRC, pState, pState->listOutStreams.count(), true, pPdStruct,
                           IMPTYPE_STREAMUNPACKEDCRC)) {
                break;
            }
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            break;

        case XSevenZip::k7zIdFolder: {
            quint64 nNumberOfFolders = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfFolders", DRF_COUNT, IMPTYPE_NUMBEROFFOLDERS);

            if (pState->bIsError || (nNumberOfFolders > SEVENZIP_MAX_ITEM_COUNT)) {
                pState->bIsError = true;
                pState->sErrorString = tr("Excessive 7z folder count");
                break;
            }

            pState->nNumberOfFolders = nNumberOfFolders;  // Store for SubStreamsInfo
            pState->nNumberOfCoders = 0;

            quint8 nExt = _handleByte(pListRecords, pState, pPdStruct, "ExternalByte", IMPTYPE_UNKNOWN);

            if (nExt == 0) {
                // Loop through all folders
                for (quint64 iFolderIndex = 0; iFolderIndex < nNumberOfFolders && !pState->bIsError; iFolderIndex++) {
                    SZFOLDER szFolder = {};

                    qint32 nNumInStreamsTotal = 0;
                    qint32 nNumOutStreamsTotal = 0;
                    quint64 nNumberOfCoders = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfCoders", DRF_COUNT, IMPTYPE_NUMBEROFCODERS);

                    if (pState->bIsError || (nNumberOfCoders == 0) || (nNumberOfCoders > SEVENZIP_MAX_ITEM_COUNT) ||
                        (pState->nNumberOfCoders > (SEVENZIP_MAX_ITEM_COUNT - nNumberOfCoders))) {
                        pState->bIsError = true;
                        pState->sErrorString = tr("Invalid or excessive 7z coder count");
                        break;
                    }

                    // Loop through all coders in this folder
                    for (quint64 iCoderIndex = 0; iCoderIndex < nNumberOfCoders && !pState->bIsError; iCoderIndex++) {
                        SZCODER coder = {};
                        coder.nNumOutStreams = 1;

                        quint8 nFlag = _handleByte(pListRecords, pState, pPdStruct, "Flag", IMPTYPE_UNKNOWN);

                        qint32 nCodecSize = nFlag & 0x0F;
                        coder.bIsComplex = (nFlag & 0x10) != 0;
                        bool bHasAttr = (nFlag & 0x20) != 0;

                        if ((nCodecSize == 0) || (nFlag & 0xC0)) {
                            pState->bIsError = true;
                            pState->sErrorString = tr("Invalid 7z coder flags");
                            break;
                        }

                        coder.baCoder = _handleArray(pListRecords, pState, nCodecSize, pPdStruct, "Coder", IMPTYPE_CODER);
                        if (!pState->bIsError && (coderToCompressMethod(coder.baCoder) == HANDLE_METHOD_7Z_AES)) {
                            pState->bIsEncrypted = true;
                        }

                        if (coder.bIsComplex) {
                            // Complex coders have bind pairs and packed streams
                            // Read the number of input and output streams
                            quint64 nNumInStreams = _handleNumber(pListRecords, pState, pPdStruct, "NumInStreams", DRF_COUNT, IMPTYPE_UNKNOWN);
                            quint64 nNumOutStreams = _handleNumber(pListRecords, pState, pPdStruct, "NumOutStreams", DRF_COUNT, IMPTYPE_UNKNOWN);

                            if (pState->bIsError || (nNumInStreams == 0) || (nNumOutStreams != 1) ||
                                (nNumInStreams > SEVENZIP_MAX_ITEM_COUNT) || (nNumOutStreams > SEVENZIP_MAX_ITEM_COUNT)) {
                                pState->bIsError = true;
                                pState->sErrorString = tr("Invalid, excessive, or unsupported 7z coder stream count");
                                break;
                            }

                            coder.nNumInStreams = (qint32)nNumInStreams;
                            coder.nNumOutStreams = (qint32)nNumOutStreams;
                        } else {
                            coder.nNumInStreams = 1;
                        }

                        if ((coder.nNumInStreams > (qint32)SEVENZIP_MAX_ITEM_COUNT - nNumInStreamsTotal) ||
                            (coder.nNumOutStreams > (qint32)SEVENZIP_MAX_ITEM_COUNT - nNumOutStreamsTotal)) {
                            pState->bIsError = true;
                            pState->sErrorString = tr("Excessive 7z folder stream count");
                            break;
                        }

                        nNumInStreamsTotal += coder.nNumInStreams;
                        nNumOutStreamsTotal += coder.nNumOutStreams;

                        if (bHasAttr && !pState->bIsError) {
                            quint64 nPropertySize = _handleNumber(pListRecords, pState, pPdStruct, "PropertiesSize", DRF_SIZE, IMPTYPE_UNKNOWN);
                            coder.baProperty = _handleArray(pListRecords, pState, nPropertySize, pPdStruct, "Property", IMPTYPE_CODERPROPERTY);
                        }

                        szFolder.listCoders.append(coder);
                    }

                    if (pState->bIsError) {
                        break;
                    }

                    // CodersUnpackSize has one value per output stream, not
                    // necessarily one value per coder.
                    if (pState->nNumberOfCoders > (SEVENZIP_MAX_ITEM_COUNT - (quint64)nNumOutStreamsTotal)) {
                        pState->bIsError = true;
                        pState->sErrorString = tr("Excessive 7z coder output count");
                        break;
                    }
                    pState->nNumberOfCoders += (quint64)nNumOutStreamsTotal;

                    if (nNumInStreamsTotal == 0) {
                        nNumInStreamsTotal = 1;
                    }

                    if (nNumOutStreamsTotal == 0) {
                        nNumOutStreamsTotal = 1;
                    }

                    qint32 nNumBindPairs = nNumOutStreamsTotal - 1;
                    QSet<qint32> setBoundInputs;
                    QSet<qint32> setBoundOutputs;

                    for (qint32 iBonds = 0; iBonds < nNumBindPairs; iBonds++) {
                        SZBOND szBond = {};
                        quint64 nInputIndex = _handleNumber(pListRecords, pState, pPdStruct, "InputIndex", DRF_UNKNOWN, IMPTYPE_UNKNOWN);
                        quint64 nOutputIndex = _handleNumber(pListRecords, pState, pPdStruct, "OutputIndex", DRF_UNKNOWN, IMPTYPE_UNKNOWN);

                        if (pState->bIsError || (nInputIndex >= (quint64)nNumInStreamsTotal) ||
                            (nOutputIndex >= (quint64)nNumOutStreamsTotal) || setBoundInputs.contains((qint32)nInputIndex) ||
                            setBoundOutputs.contains((qint32)nOutputIndex)) {
                            pState->bIsError = true;
                            pState->sErrorString = tr("Invalid 7z coder bond");
                            break;
                        }

                        szBond.nInputIndex = (qint32)nInputIndex;
                        szBond.nOutputIndex = (qint32)nOutputIndex;
                        setBoundInputs.insert(szBond.nInputIndex);
                        setBoundOutputs.insert(szBond.nOutputIndex);

                        szFolder.listBonds.append(szBond);
                    }

                    qint32 nNumPackedStreams = nNumInStreamsTotal - nNumBindPairs;

                    if ((nNumPackedStreams <= 0) || (nNumPackedStreams > (qint32)SEVENZIP_MAX_ITEM_COUNT)) {
                        pState->bIsError = true;
                        pState->sErrorString = tr("Invalid 7z packed-stream topology");
                        break;
                    }

                    if (nNumPackedStreams == 1) {
                        // A single packed stream omits its index. Infer the one
                        // input that is not the target of a bind pair.
                        qint32 nUnboundInput = -1;
                        for (qint32 i = 0; i < nNumInStreamsTotal; i++) {
                            if (!setBoundInputs.contains(i)) {
                                if (nUnboundInput != -1) {
                                    pState->bIsError = true;
                                    break;
                                }
                                nUnboundInput = i;
                            }
                        }

                        if (nUnboundInput < 0) {
                            pState->bIsError = true;
                            pState->sErrorString = tr("Invalid 7z implicit packed stream");
                            break;
                        }
                        szFolder.listStreamIndexes.append(nUnboundInput);
                    } else {
                        QSet<qint32> setPackedInputs;
                        for (qint32 iPacks = 0; iPacks < nNumPackedStreams; iPacks++) {
                            quint64 nStreamIndex = _handleNumber(pListRecords, pState, pPdStruct, "StreamIndex", DRF_UNKNOWN, IMPTYPE_UNKNOWN);

                            if (pState->bIsError || (nStreamIndex >= (quint64)nNumInStreamsTotal) ||
                                setBoundInputs.contains((qint32)nStreamIndex) || setPackedInputs.contains((qint32)nStreamIndex)) {
                                pState->bIsError = true;
                                pState->sErrorString = tr("Invalid 7z packed-stream index");
                                break;
                            }

                            setPackedInputs.insert((qint32)nStreamIndex);
                            szFolder.listStreamIndexes.append((qint32)nStreamIndex);
                        }
                    }

                    if (pState->bIsError) {
                        break;
                    }

                    pState->listFolders.append(szFolder);

                    // UnpackInfo stores one digest per folder, independently of
                    // how many packed input streams the folder consumes.
                    SZOUTSTREAM szOutStream = {};
                    pState->listOutStreams.append(szOutStream);
                }
            } else if (nExt == 1) {
                _handleNumber(pListRecords, pState, pPdStruct, QString("Data Stream Index"), DRF_COUNT, IMPTYPE_UNKNOWN);
                pState->bIsError = true;
                pState->sErrorString = tr("External 7z folder streams are not supported");
            } else {
                pState->bIsError = true;
                pState->sErrorString = QString("%1: %2").arg(XBinary::valueToHexEx(pState->nCurrentOffset)).arg(tr("Invalid data"));
            }

            bResult = true;
            break;
        }

        case XSevenZip::k7zIdSubStreamsInfo:
            // SubStreamsInfo structure (all fields are optional):
            // - k7zIdNumUnpackStream (optional): present if any folder has >1 file
            // - k7zIdSize (optional): unpacked sizes for files
            // - k7zIdCRC (optional): CRCs for files

            _handleId(pListRecords, XSevenZip::k7zIdNumUnpackStream, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);

            // Without NumUnpackStream each folder has one substream. Its
            // per-file digest is omitted when the folder digest is defined.
            if (pState->listNumUnpackedStreams.isEmpty()) {
                qint32 nNumberOfSubDigests = 0;

                for (qint32 i = 0; i < (qint32)pState->nNumberOfFolders; i++) {
                    if ((i >= pState->listOutStreams.count()) || !pState->listOutStreams.at(i).bCRCDefined) {
                        nNumberOfSubDigests++;
                    }
                }

                _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, nNumberOfSubDigests, false, pPdStruct, IMPTYPE_FILECRC);
            }

            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);

            break;

        case XSevenZip::k7zIdNumUnpackStream: {
            // NumUnpackStream: one value per folder, indicating how many files are in each solid block
            // Then Size section contains unpacked sizes for all files
            quint64 nNumberOfSubDigests = 0;
            quint64 nNumberOfSubstreamSizes = 0;

            for (quint64 i = 0; i < pState->nNumberOfFolders && isPdStructNotCanceled(pPdStruct); i++) {
                quint64 nNumStreamsInFolder =
                    _handleNumber(pListRecords, pState, pPdStruct, QString("NumUnpackStream%1").arg(i), DRF_COUNT, IMPTYPE_NUMBEROFUNPACKSTREAM);

                if (pState->bIsError || (nNumStreamsInFolder > SEVENZIP_MAX_ITEM_COUNT) ||
                    ((nNumStreamsInFolder > 0) && (nNumberOfSubstreamSizes > (SEVENZIP_MAX_ITEM_COUNT - (nNumStreamsInFolder - 1)))) ||
                    (nNumberOfSubDigests > (SEVENZIP_MAX_ITEM_COUNT - nNumStreamsInFolder))) {
                    pState->bIsError = true;
                    pState->sErrorString = tr("Excessive 7z substream count");
                    break;
                }

                pState->listNumUnpackedStreams.append(nNumStreamsInFolder);

                if (nNumStreamsInFolder > 0) {
                    nNumberOfSubstreamSizes += nNumStreamsInFolder - 1;
                }

                bool bFolderCRCDefined = (i < (quint64)pState->listOutStreams.count()) && pState->listOutStreams.at((qint32)i).bCRCDefined;

                if ((nNumStreamsInFolder != 1) || !bFolderCRCDefined) {
                    nNumberOfSubDigests += nNumStreamsInFolder;
                }
            }

            if (pState->bIsError || !isPdStructNotCanceled(pPdStruct)) {
                break;
            }

            // SubStreamsInfo Size section contains (N-1) sizes for each folder with N>1 files
            // For folders with only 1 file, no size is listed (use folder size)
            // Zero-substream folders contribute no size entries.
            _handleId(pListRecords, XSevenZip::k7zIdSize, pState, nNumberOfSubstreamSizes, false, pPdStruct, IMPTYPE_FILEUNPACKEDSIZE);
            _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, nNumberOfSubDigests, false, pPdStruct, IMPTYPE_FILECRC);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdEncodedHeader:
            if (!_handleId(pListRecords, XSevenZip::k7zIdPackInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN) ||
                !_handleId(pListRecords, XSevenZip::k7zIdUnpackInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN)) {
                break;
            }
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            break;

        case XSevenZip::k7zIdSize: {
            qint64 nCurrentOffset = 0;
            for (quint64 i = 0; (i < (quint64)nCount) && isPdStructNotCanceled(pPdStruct); i++) {
                quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("Size%1").arg(i), DRF_SIZE, impType);

                if (pState->bIsError || (nSize > (quint64)LLONG_MAX) || ((qint64)nSize > (LLONG_MAX - nCurrentOffset))) {
                    pState->bIsError = true;
                    pState->sErrorString = tr("Invalid or excessive 7z stream size");
                    break;
                }

                if (impType == IMPTYPE_STREAMSIZE) {
                    if (i < pState->listInStreams.count()) {
                        pState->listInStreams[i].nOffset = nCurrentOffset;
                        pState->listInStreams[i].nSize = (qint64)nSize;
                    }
                } else if (impType == IMPTYPE_FILEUNPACKEDSIZE) {
                    pState->listFileSizes.append((qint64)nSize);
                }

                nCurrentOffset += (qint64)nSize;
            }

            if (!pState->bIsError && (impType == IMPTYPE_STREAMSIZE)) {
                if ((pState->nStreamsBegin < 0) || (pState->nStreamsBegin > (LLONG_MAX - nCurrentOffset))) {
                    pState->bIsError = true;
                    pState->sErrorString = tr("Invalid 7z packed-data range");
                } else {
                    pState->nMaximumPackStreamEnd = qMax(pState->nMaximumPackStreamEnd, pState->nStreamsBegin + nCurrentOffset);
                }
            }
        }

            bResult = !pState->bIsError;
            break;

        case XSevenZip::k7zIdCodersUnpackSize: {
            qint64 nCurrentOffset = 0;
            for (quint64 i = 0; (i < (quint64)nCount) && isPdStructNotCanceled(pPdStruct); i++) {
                quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("CodersUnpackSize%1").arg(i), DRF_SIZE, impType);

                if (pState->bIsError || (nSize > (quint64)LLONG_MAX) || ((qint64)nSize > (LLONG_MAX - nCurrentOffset))) {
                    pState->bIsError = true;
                    pState->sErrorString = tr("Invalid or excessive 7z coder output size");
                    break;
                }

                pState->listCodersSizes.append(nSize);  // Store unpacked size for each coder (for debugging)

                nCurrentOffset += (qint64)nSize;
            }
        }

            bResult = !pState->bIsError;
            break;

        case XSevenZip::k7zIdCRC: {
            // CRC format: AllAreDefined byte + CRC data
            // If AllAreDefined == 1: nCount CRC32 values
            // If AllAreDefined == 0: bitmask + CRC32 values for set bits
            quint8 nAllAreDefined = _handleByte(pListRecords, pState, pPdStruct, "AllAreDefined", IMPTYPE_UNKNOWN);

            QByteArray baDefined;

            if (nAllAreDefined == 0) {
                qint32 nDefinedBytes = (nCount + 7) / 8;
                baDefined = _handleArray(pListRecords, pState, nDefinedBytes, pPdStruct, "CRCDefined", IMPTYPE_UNKNOWN);
            }

            for (qint32 i = 0; (i < nCount) && isPdStructNotCanceled(pPdStruct) && !pState->bIsError; i++) {
                bool bDefined = (nAllAreDefined != 0) || XBinary::_read_bool_safe_rev(baDefined.data(), baDefined.size(), i);
                quint32 nCRC = 0;

                if (bDefined) {
                    nCRC = _handleUINT32(pListRecords, pState, pPdStruct, QString("CRC%1").arg(i), impType);
                }

                if (impType == IMPTYPE_STREAMCRC) {
                    if (i < pState->listInStreams.count()) {
                        pState->listInStreams[i].nCRC = nCRC;
                        pState->listInStreams[i].bCRCDefined = bDefined;
                    }
                } else if (impType == IMPTYPE_STREAMUNPACKEDCRC) {
                    if (i < pState->listOutStreams.count()) {
                        pState->listOutStreams[i].nCRC = nCRC;
                        pState->listOutStreams[i].bCRCDefined = bDefined;
                    }
                } else if (impType == IMPTYPE_FILECRC) {
                    pState->listFileCRC.append(nCRC);
                    pState->listFileCRCDefined.append(bDefined);
                }
            }
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdFilesInfo: {
            quint64 nNumberOfFiles = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfFiles", DRF_COUNT, IMPTYPE_NUMBEROFFILES);

            if (pState->bIsError || (nNumberOfFiles > SEVENZIP_MAX_ITEM_COUNT)) {
                pState->bIsError = true;
                pState->sErrorString = tr("Excessive 7z file count");
                break;
            }

            // Store file count in state for later use
            pState->nNumberOfFiles = nNumberOfFiles;
            Q_UNUSED(nNumberOfFiles)

            // Loop through property IDs until we hit End marker
            bool bFoundEnd = false;
            while (!pState->bIsError && !bFoundEnd && isPdStructNotCanceled(pPdStruct)) {
                // Peek at next ID
                if (pState->nCurrentOffset >= pState->nSize) {
                    pState->bIsError = true;
                    pState->sErrorString = tr("Unexpected end of data");
                    break;
                }

                XBinary::PACKED_UINT nextPropertyTag =
                    XBinary::_read_packedNumber(pState->pData + pState->nCurrentOffset, pState->nSize - pState->nCurrentOffset);
                if (!nextPropertyTag.bIsValid) {
                    pState->bIsError = true;
                    pState->sErrorString = tr("Invalid 7z file property ID");
                    break;
                }
                quint64 nNextId = nextPropertyTag.nValue;

                // Check for End marker
                if (nNextId == XSevenZip::k7zIdEnd) {
                    _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
                    bFoundEnd = true;
                    break;
                }

                // Try to handle known property IDs
                bool bHandled = false;
                if (nNextId == XSevenZip::k7zIdDummy) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdDummy, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextId == XSevenZip::k7zIdEmptyStream) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdEmptyStream, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextId == XSevenZip::k7zIdEmptyFile) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdEmptyFile, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextId == XSevenZip::k7zIdAnti) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdAnti, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextId == XSevenZip::k7zIdName) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdName, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextId == XSevenZip::k7zIdMTime) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdMTime, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextId == XSevenZip::k7zIdCTime) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdCTime, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextId == XSevenZip::k7zIdATime) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdATime, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextId == XSevenZip::k7zIdWinAttrib) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdWinAttrib, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextId == XSevenZip::k7zIdComment) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdComment, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else {
                    // Unknown property - skip its packed ID, then its checked
                    // size and payload.
                    pState->nCurrentOffset += nextPropertyTag.nByteSize;

                    // Try to read the size field (most properties have this)
                    if (pState->nCurrentOffset < pState->nSize) {
                        XBinary::PACKED_UINT puSize = XBinary::_read_packedNumber(pState->pData + pState->nCurrentOffset, pState->nSize - pState->nCurrentOffset);
                        if (puSize.bIsValid) {
                            pState->nCurrentOffset += puSize.nByteSize;
                            quint64 nDataSize = puSize.nValue;
                            // Skip the data
                            qint64 nRemaining = pState->nSize - pState->nCurrentOffset;
                            if ((nRemaining >= 0) && (nDataSize <= (quint64)nRemaining)) {
                                pState->nCurrentOffset += (qint64)nDataSize;
                                bHandled = true;  // Successfully skipped
                            } else {
                                pState->bIsError = true;
                                pState->sErrorString = tr("Truncated 7z file property");
                                break;
                            }
                        } else {
                            pState->bIsError = true;
                            pState->sErrorString = tr("Invalid 7z file property size");
                            break;
                        }
                    } else {
                        pState->bIsError = true;
                        pState->sErrorString = tr("Truncated 7z file property");
                        break;
                    }
                }

                if (!bHandled && !pState->bIsError && isPdStructNotCanceled(pPdStruct)) {
                    pState->bIsError = true;
                    pState->sErrorString = tr("Invalid 7z file property");
                    break;
                }
            }

            if (!pState->bIsError && bFoundEnd) {
                qint32 nFiles = (qint32)nNumberOfFiles;
                qint32 nEmptyStreamBytes = (nFiles + 7) / 8;

                if (pState->listFileNames.count() != nFiles) {
                    pState->bIsError = true;
                    pState->sErrorString = tr("7z file-name count does not match FilesInfo");
                } else if (!pState->baEmptyStreams.isEmpty() && (pState->baEmptyStreams.size() != nEmptyStreamBytes)) {
                    pState->bIsError = true;
                    pState->sErrorString = tr("Invalid 7z empty-stream bitmap size");
                } else {
                    qint32 nEmptyStreams = XBinary::_getBitCount_safe(pState->baEmptyStreams.data(), pState->baEmptyStreams.size());
                    qint32 nEmptyFileBytes = (nEmptyStreams + 7) / 8;

                    if ((!pState->baEmptyFiles.isEmpty() && (pState->baEmptyFiles.size() != nEmptyFileBytes)) ||
                        (!pState->baAnti.isEmpty() && (pState->baAnti.size() != nEmptyFileBytes))) {
                        pState->bIsError = true;
                        pState->sErrorString = tr("Invalid 7z empty-file bitmap size");
                    }
                }
            }

            bResult = bFoundEnd && !pState->bIsError;
            break;
        }

        case XSevenZip::k7zIdDummy: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("DummySize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("DummyArray"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdName: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("NameSize"), DRF_SIZE, IMPTYPE_UNKNOWN);

            if (pState->bIsError || (nSize < 1) || (nSize > (quint64)(pState->nSize - pState->nCurrentOffset))) {
                pState->bIsError = true;
                pState->sErrorString = tr("Invalid 7z file-name property size");
                break;
            }

            quint8 nExt = _handleByte(pListRecords, pState, pPdStruct, "ExternalByte", IMPTYPE_UNKNOWN);

            if (nExt == 0) {
                // The data is a single block of null-terminated UTF-16LE strings.
                // The total size of this block is (nSize - 1) bytes.
                qint64 nNamesDataOffset = pState->nCurrentOffset;
                qint64 nNamesDataSize = nSize - 1;  // -1 for the ExternalByte

                if ((nNamesDataSize >= 0) && ((nNamesDataSize & 1) == 0) && (nNamesDataSize <= (pState->nSize - pState->nCurrentOffset))) {
                    qint32 nFileIndex = 0;
                    qint64 nRelativeOffset = 0;

                    while (nRelativeOffset < nNamesDataSize) {
                        qint64 nNameStartOffset = nNamesDataOffset + nRelativeOffset;

                        // Find the null terminator (2 bytes of 0)
                        qint64 nMaxLen = nNamesDataSize - nRelativeOffset;
                        qint64 nNameLenBytes = -1;

                        for (qint64 i = 0; i < nMaxLen; i += 2) {
                            if (pState->pData[nNameStartOffset + i] == 0 && pState->pData[nNameStartOffset + i + 1] == 0) {
                                nNameLenBytes = i;
                                break;
                            }
                        }

                        if (nNameLenBytes < 0) {
                            pState->bIsError = true;
                            pState->sErrorString = tr("Unterminated 7z file name");
                            break;
                        }

                        QString sFilename;
                        sFilename.reserve((qint32)(nNameLenBytes / 2));
                        bool bHighSurrogatePending = false;
                        for (qint64 i = 0; i < nNameLenBytes; i += 2) {
                            ushort nCharacter = (ushort)(quint8)pState->pData[nNameStartOffset + i] |
                                                ((ushort)(quint8)pState->pData[nNameStartOffset + i + 1] << 8);
                            bool bHighSurrogate = (nCharacter >= 0xD800) && (nCharacter <= 0xDBFF);
                            bool bLowSurrogate = (nCharacter >= 0xDC00) && (nCharacter <= 0xDFFF);

                            if ((bLowSurrogate && !bHighSurrogatePending) || (bHighSurrogatePending && !bLowSurrogate)) {
                                pState->bIsError = true;
                                pState->sErrorString = tr("Invalid UTF-16 in 7z file name");
                                break;
                            }

                            sFilename.append(QChar(nCharacter));
                            bHighSurrogatePending = bHighSurrogate;
                        }

                        if (pState->bIsError || bHighSurrogatePending) {
                            pState->bIsError = true;
                            pState->sErrorString = tr("Invalid UTF-16 in 7z file name");
                            break;
                        }

                        SZRECORD fileNameRecord = {};
                        fileNameRecord.nRelOffset = (qint32)nNameStartOffset;
                        fileNameRecord.nSize = (qint32)(nNameLenBytes + 2);
                        fileNameRecord.varValue = sFilename;
                        fileNameRecord.srType = SRTYPE_ARRAY;
                        fileNameRecord.valType = VT_STRING;
                        fileNameRecord.impType = IMPTYPE_FILENAME;
                        fileNameRecord.sName = QString("FileName[%1]").arg(nFileIndex);
                        pListRecords->append(fileNameRecord);

                        pState->listFileNames.append(sFilename);
                        nFileIndex++;
                        nRelativeOffset += nNameLenBytes + 2;
                    }

                    if (!pState->bIsError && (nRelativeOffset == nNamesDataSize)) {
                        pState->nCurrentOffset += nNamesDataSize;
                    }
                } else {
                    pState->bIsError = true;
                    pState->sErrorString = tr("Invalid 7z file-name data");
                }
            } else if (nExt == 1) {
                _handleNumber(pListRecords, pState, pPdStruct, QString("DataIndex"), DRF_COUNT, IMPTYPE_UNKNOWN);
                pState->bIsError = true;
                pState->sErrorString = tr("External 7z file-name streams are not supported");
            } else {
                pState->bIsError = true;
                pState->sErrorString = tr("Invalid 7z file-name external flag");
            }

            bResult = true;
            break;
        }

        case XSevenZip::k7zIdEmptyStream: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("EmptyStreamSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baEmptyStreams = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("EmptyStreamData"), IMPTYPE_EMPTYSTREAMDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdEmptyFile: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("EmptyFileSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baEmptyFiles = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("EmptyFileData"), IMPTYPE_EMPTYFILEDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdAnti: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("AntiSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baAnti = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("AntiData"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdCTime: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("CTimeSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baCTime = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("CTimeData"), IMPTYPE_CTIMEDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdATime: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("ATimeSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baATime = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("ATimeData"), IMPTYPE_ATIMEDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdMTime: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("MTimeSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baMTime = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("MTimeData"), IMPTYPE_MTIMEDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdWinAttrib: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("WinAttribSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baWinAttrib = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("WinAttribData"), IMPTYPE_WINATTRIBDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdComment: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("CommentSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baComment = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("CommentData"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdStartPos: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("StartPosSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baStartPos = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("StartPosData"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdEnd: bResult = true; break;

        default:
            // Unhandled ID type
            bResult = false;
            break;
    }

    return bResult;
}

quint64 XSevenZip::_handleNumber(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption, quint32 nFlags, IMPTYPE impType)
{
    // Early exit checks
    if (!pListRecords || !pState || isPdStructStopped(pPdStruct) || pState->bIsError) {
        return 0;
    }

    if (!pState->pData || (pState->nCurrentOffset < 0) || (pState->nCurrentOffset >= pState->nSize)) {
        pState->bIsError = true;
        pState->sErrorString = QString("%1: %2 (%3)").arg(XBinary::valueToHexEx(pState->nCurrentOffset)).arg(tr("Invalid data")).arg(sCaption);
        return 0;
    }

    XBinary::PACKED_UINT puNumber = XBinary::_read_packedNumber(pState->pData + pState->nCurrentOffset, pState->nSize - pState->nCurrentOffset);

    if (!puNumber.bIsValid) {
        pState->bIsError = true;
        pState->sErrorString = QString("%1: %2 (%3)").arg(XBinary::valueToHexEx(pState->nCurrentOffset)).arg(tr("Invalid data")).arg(sCaption);
#ifdef QT_DEBUG
        qDebug("Invalid packed number for '%s' at offset: 0x%llX", qPrintable(sCaption), (qint64)pState->nCurrentOffset);
#endif
        return 0;
    }

    // Add record
    SZRECORD record = {};
    record.nRelOffset = pState->nCurrentOffset;
    record.nSize = puNumber.nByteSize;
    record.varValue = puNumber.nValue;
    record.srType = SRTYPE_NUMBER;
    record.valType = VT_PACKEDNUMBER;
    record.nFlags = nFlags;
    record.impType = impType;
    record.sName = sCaption;
    pListRecords->append(record);

    pState->nCurrentOffset += puNumber.nByteSize;

    return puNumber.nValue;
}

quint8 XSevenZip::_handleByte(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption, IMPTYPE impType)
{
    // Early exit checks
    if (!pListRecords || !pState || isPdStructStopped(pPdStruct) || pState->bIsError) {
        return 0;
    }

    if (!pState->pData || (pState->nCurrentOffset < 0) || (pState->nCurrentOffset >= pState->nSize)) {
        pState->bIsError = true;
        pState->sErrorString = QString("%1: %2 (%3)").arg(XBinary::valueToHexEx(pState->nCurrentOffset)).arg(tr("Invalid data")).arg(sCaption);
        return 0;
    }

    quint8 nResult = _read_uint8(pState->pData + pState->nCurrentOffset);

    // Add record
    SZRECORD record = {};
    record.nRelOffset = pState->nCurrentOffset;
    record.nSize = 1;
    record.varValue = nResult;
    record.srType = SRTYPE_BYTE;
    record.valType = VT_BYTE;
    record.impType = impType;
    record.sName = sCaption;
    pListRecords->append(record);

    pState->nCurrentOffset++;

    return nResult;
}

quint32 XSevenZip::_handleUINT32(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption, IMPTYPE impType)
{
    // Early exit checks
    if (!pListRecords || !pState || isPdStructStopped(pPdStruct) || pState->bIsError) {
        return 0;
    }

    // Check if we have enough bytes for a UINT32
    if (!pState->pData || (pState->nCurrentOffset < 0) || (pState->nSize < 4) || (pState->nCurrentOffset > (pState->nSize - 4))) {
        pState->bIsError = true;
        pState->sErrorString = QString("%1: %2 (%3)").arg(XBinary::valueToHexEx(pState->nCurrentOffset)).arg(tr("Invalid data")).arg(sCaption);
#ifdef QT_DEBUG
        qDebug("Not enough bytes for UINT32 '%s' at offset: 0x%llX (need 4, have %lld)", qPrintable(sCaption), (qint64)pState->nCurrentOffset,
               pState->nSize - pState->nCurrentOffset);
#endif
        return 0;
    }

    quint32 nResult = _read_uint32(pState->pData + pState->nCurrentOffset);

    // Add record
    SZRECORD record = {};
    record.nRelOffset = pState->nCurrentOffset;
    record.nSize = 4;
    record.varValue = nResult;
    record.srType = SRTYPE_UINT32;
    record.valType = VT_UINT32;
    record.impType = impType;
    record.sName = sCaption;
    pListRecords->append(record);

    pState->nCurrentOffset += 4;

    return nResult;
}

QByteArray XSevenZip::_handleArray(QList<SZRECORD> *pListRecords, SZSTATE *pState, qint64 nSize, PDSTRUCT *pPdStruct, const QString &sCaption, IMPTYPE impType)
{
    QByteArray baResult;
    // Early exit checks
    if (!pListRecords || !pState || isPdStructStopped(pPdStruct) || pState->bIsError) {
        return baResult;
    }

    if (!pState->pData || (nSize < 0) || (nSize > INT_MAX) || (pState->nSize < 0) || (pState->nCurrentOffset < 0) ||
        (pState->nCurrentOffset > pState->nSize) || (nSize > (pState->nSize - pState->nCurrentOffset))) {
        pState->bIsError = true;
        pState->sErrorString = QString("%1: %2 (%3, size: %4)").arg(XBinary::valueToHexEx(pState->nCurrentOffset)).arg(tr("Invalid data")).arg(sCaption).arg(nSize);
        return baResult;
    }

    baResult = QByteArray(pState->pData + pState->nCurrentOffset, (qint32)nSize);

    // Add record
    SZRECORD record = {};
    record.nRelOffset = pState->nCurrentOffset;
    record.nSize = (qint32)nSize;
    record.srType = SRTYPE_ARRAY;
    record.valType = VT_BYTE_ARRAY;
    record.impType = impType;
    record.sName = sCaption;
    record.varValue = baResult;
    pListRecords->append(record);

    pState->nCurrentOffset += nSize;

    return baResult;
}

bool XSevenZip::_decode7zTimeValue(const QByteArray &baData, qint32 nNumFiles, qint32 nFileIndex, quint64 *pResult)
{
    if (baData.isEmpty() || nFileIndex < 0 || nFileIndex >= nNumFiles || pResult == nullptr) {
        return false;
    }

    const char *pData = baData.constData();
    qint32 nLen = baData.size();

    // Byte 0: AllAreDefined
    bool bAllDefined = ((quint8)pData[0] != 0);
    qint32 nBitmapBytes = bAllDefined ? 0 : ((nNumFiles + 7) / 8);

    // Byte 1 [+ optional bitmap]: External flag (0 = inline values follow)
    qint32 nExternalByteOffset = 1 + nBitmapBytes;
    if (nExternalByteOffset >= nLen) {
        return false;
    }
    bool bExternal = ((quint8)pData[nExternalByteOffset] != 0);
    if (bExternal) {
        return false;  // Values are in an external stream, cannot decode inline
    }
    qint32 nValuesOffset = nExternalByteOffset + 1;

    bool bDefined = false;
    qint32 nDefinedBefore = 0;

    if (bAllDefined) {
        bDefined = true;
        nDefinedBefore = nFileIndex;
    } else {
        const char *pBitmap = pData + 1;
        bDefined = XBinary::_read_bool_safe_rev(const_cast<char *>(pBitmap), nBitmapBytes, nFileIndex);

        for (qint32 k = 0; k < nFileIndex; k++) {
            if (XBinary::_read_bool_safe_rev(const_cast<char *>(pBitmap), nBitmapBytes, k)) {
                nDefinedBefore++;
            }
        }
    }

    if (!bDefined) {
        return false;
    }

    qint32 nOffset = nValuesOffset + nDefinedBefore * 8;
    if (nOffset + 8 > nLen) {
        return false;
    }

    quint64 nValue = 0;
    for (qint32 b = 0; b < 8; b++) {
        nValue |= ((quint64)(quint8)pData[nOffset + b]) << (b * 8);
    }

    *pResult = nValue;
    return true;
}

bool XSevenZip::_decode7zAttribValue(const QByteArray &baData, qint32 nNumFiles, qint32 nFileIndex, quint32 *pResult)
{
    if (baData.isEmpty() || nFileIndex < 0 || nFileIndex >= nNumFiles || pResult == nullptr) {
        return false;
    }

    const char *pData = baData.constData();
    qint32 nLen = baData.size();

    // Byte 0: AllAreDefined
    bool bAllDefined = ((quint8)pData[0] != 0);
    qint32 nBitmapBytes = bAllDefined ? 0 : ((nNumFiles + 7) / 8);

    // Byte 1 [+ optional bitmap]: External flag (0 = inline values follow)
    qint32 nExternalByteOffset = 1 + nBitmapBytes;
    if (nExternalByteOffset >= nLen) {
        return false;
    }
    bool bExternal = ((quint8)pData[nExternalByteOffset] != 0);
    if (bExternal) {
        return false;  // Values are in an external stream, cannot decode inline
    }
    qint32 nValuesOffset = nExternalByteOffset + 1;

    bool bDefined = false;
    qint32 nDefinedBefore = 0;

    if (bAllDefined) {
        bDefined = true;
        nDefinedBefore = nFileIndex;
    } else {
        const char *pBitmap = pData + 1;
        bDefined = XBinary::_read_bool_safe_rev(const_cast<char *>(pBitmap), nBitmapBytes, nFileIndex);

        for (qint32 k = 0; k < nFileIndex; k++) {
            if (XBinary::_read_bool_safe_rev(const_cast<char *>(pBitmap), nBitmapBytes, k)) {
                nDefinedBefore++;
            }
        }
    }

    if (!bDefined) {
        return false;
    }

    qint32 nOffset = nValuesOffset + nDefinedBefore * 4;
    if (nOffset + 4 > nLen) {
        return false;
    }

    *pResult = (quint32)(quint8)pData[nOffset] | ((quint32)(quint8)pData[nOffset + 1] << 8) | ((quint32)(quint8)pData[nOffset + 2] << 16) |
               ((quint32)(quint8)pData[nOffset + 3] << 24);

    return true;
}

bool XSevenZip::_validateEncodedHeader(SZSTATE *pState, qint64 nPackDataLimit)
{
    if (!pState || pState->bIsError || (nPackDataLimit < (qint64)sizeof(SIGNATUREHEADER)) ||
        (pState->listFolders.count() != 1) || (pState->listInStreams.count() != 1) ||
        (pState->listOutStreams.count() != 1)) {
        return false;
    }

    const SZFOLDER &folder = pState->listFolders.at(0);
    qint32 nCoderCount = folder.listCoders.count();
    if ((nCoderCount <= 0) || (nCoderCount > 2) || (folder.listStreamIndexes.count() != 1) ||
        (folder.listBonds.count() != (nCoderCount - 1)) || (pState->nNumberOfCoders != (quint64)nCoderCount) ||
        (pState->listCodersSizes.count() != nCoderCount)) {
        return false;
    }

    for (const SZCODER &coder : folder.listCoders) {
        if (coderToCompressMethod(coder.baCoder) == HANDLE_METHOD_UNKNOWN) {
            return false;
        }
    }

    const SZINSTREAM &stream = pState->listInStreams.at(0);
    if ((pState->nStreamsBegin < (qint64)sizeof(SIGNATUREHEADER)) || (pState->nStreamsBegin > nPackDataLimit) ||
        (stream.nOffset != 0) || (stream.nSize < 0) || (stream.nSize > (nPackDataLimit - pState->nStreamsBegin)) ||
        (pState->nMaximumPackStreamEnd > nPackDataLimit) ||
        (pState->listCodersSizes.at(nCoderCount - 1) > (quint64)SEVENZIP_MAX_NEXT_HEADER_SIZE)) {
        return false;
    }

    return true;
}

static bool sevenZipValidateBitmap(XBinary::PDSTRUCT *pPdStruct, const QByteArray &baBitmap, qint32 nBits)
{
    if (baBitmap.isEmpty()) {
        return true;
    }

    qint32 nExpectedBytes = (nBits + 7) / 8;
    if ((nBits < 0) || (baBitmap.size() != nExpectedBytes)) {
        return false;
    }

    for (qint32 i = nBits; i < (nExpectedBytes * 8); i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        if (XBinary::_read_bool_safe_rev(const_cast<char *>(baBitmap.constData()), baBitmap.size(), i)) {
            return false;
        }
    }
    return true;
}

bool XSevenZip::_validationFail(SZSTATE *pState, const QString &sMessage)
{
    pState->bIsError = true;
    pState->sErrorString = sMessage;
    return false;
}

bool XSevenZip::_validateParsedHeader(SZSTATE *pState, qint64 nPackDataLimit, PDSTRUCT *pPdStruct)
{
    if (!pState || pState->bIsError || (nPackDataLimit < (qint64)sizeof(SIGNATUREHEADER))) {
        return false;
    }

    if ((pState->nNumberOfFiles > SEVENZIP_MAX_ITEM_COUNT) ||
        (pState->listFileNames.count() != (qint32)pState->nNumberOfFiles) ||
        (pState->nNumberOfFolders != (quint64)pState->listFolders.count()) ||
        (pState->listFolders.count() != pState->listOutStreams.count()) ||
        (pState->nMaximumPackStreamEnd > nPackDataLimit)) {
        return _validationFail(pState, tr("Inconsistent 7z header counts or packed-data range"));
    }

    qint32 nNumberOfFiles = (qint32)pState->nNumberOfFiles;

    if (!sevenZipValidateBitmap(pPdStruct, pState->baEmptyStreams, nNumberOfFiles)) {
        return _validationFail(pState, tr("Invalid 7z empty-stream bitmap"));
    }

    qint32 nNumberOfEmptyStreams = 0;
    for (qint32 i = 0; i < nNumberOfFiles; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return _validationFail(pState, tr("7z parsing canceled"));
        }
        if (!pState->baEmptyStreams.isEmpty() &&
            XBinary::_read_bool_safe_rev(pState->baEmptyStreams.data(), pState->baEmptyStreams.size(), i)) {
            nNumberOfEmptyStreams++;
        }
    }

    if (!sevenZipValidateBitmap(pPdStruct, pState->baEmptyFiles, nNumberOfEmptyStreams) ||
        !sevenZipValidateBitmap(pPdStruct, pState->baAnti, nNumberOfEmptyStreams)) {
        return _validationFail(pState, tr("Invalid 7z empty-file bitmap"));
    }

    qint64 nExpectedDataFiles = (qint64)nNumberOfFiles - nNumberOfEmptyStreams;
    qint64 nDeclaredDataFiles = 0;
    qint32 nExplicitSizeIndex = 0;
    qint32 nCoderSizeIndex = 0;
    qint64 nConsumedPackStreams = 0;

    if (!pState->listNumUnpackedStreams.isEmpty() &&
        (pState->listNumUnpackedStreams.count() != pState->listFolders.count())) {
        return _validationFail(pState, tr("Invalid 7z per-folder substream count"));
    }

    for (qint32 nFolderIndex = 0; nFolderIndex < pState->listFolders.count(); nFolderIndex++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return _validationFail(pState, tr("7z parsing canceled"));
        }
        const SZFOLDER &folder = pState->listFolders.at(nFolderIndex);
        qint32 nCoderCount = folder.listCoders.count();
        if ((nCoderCount <= 0) || (folder.listBonds.count() != (nCoderCount - 1)) || folder.listStreamIndexes.isEmpty()) {
            return _validationFail(pState, tr("Invalid 7z folder topology"));
        }

        bool bHasBCJ2 = false;
        for (const SZCODER &coder : folder.listCoders) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                return _validationFail(pState, tr("7z parsing canceled"));
            }
            HANDLE_METHOD method = coderToCompressMethod(coder.baCoder);
            if (method == HANDLE_METHOD_UNKNOWN) {
                return _validationFail(pState, tr("Unsupported 7z coder"));
            }
            bHasBCJ2 = bHasBCJ2 || (method == HANDLE_METHOD_BCJ2);
        }
        if (!bHasBCJ2 && (nCoderCount > 3)) {
            return _validationFail(pState, tr("Unsupported 7z coder chain"));
        }

        if ((nCoderSizeIndex > pState->listCodersSizes.count() - nCoderCount) ||
            (pState->nNumberOfCoders != (quint64)pState->listCodersSizes.count())) {
            return _validationFail(pState, tr("Invalid 7z coder unpack-size count"));
        }

        qint64 nFolderUnpackedSize = (qint64)pState->listCodersSizes.at(nCoderSizeIndex + nCoderCount - 1);
        quint64 nFolderSubstreams = pState->listNumUnpackedStreams.isEmpty() ? 1 : pState->listNumUnpackedStreams.at(nFolderIndex);
        if (nFolderSubstreams > SEVENZIP_MAX_ITEM_COUNT) {
            return _validationFail(pState, tr("Excessive 7z folder substream count"));
        }
        nDeclaredDataFiles += (qint64)nFolderSubstreams;
        if (nDeclaredDataFiles > SEVENZIP_MAX_ITEM_COUNT) {
            return _validationFail(pState, tr("Excessive 7z data-file count"));
        }

        qint64 nExplicitSizeTotal = 0;
        for (quint64 i = 1; i < nFolderSubstreams; i++) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                return _validationFail(pState, tr("7z parsing canceled"));
            }
            if (nExplicitSizeIndex >= pState->listFileSizes.count()) {
                return _validationFail(pState, tr("Missing 7z substream size"));
            }

            qint64 nSubstreamSize = pState->listFileSizes.at(nExplicitSizeIndex++);
            if ((nSubstreamSize < 0) || (nSubstreamSize > (nFolderUnpackedSize - nExplicitSizeTotal))) {
                return _validationFail(pState, tr("Invalid 7z substream sizes"));
            }
            nExplicitSizeTotal += nSubstreamSize;
        }

        nCoderSizeIndex += nCoderCount;
        nConsumedPackStreams += folder.listStreamIndexes.count();
        if (nConsumedPackStreams > pState->listInStreams.count()) {
            return _validationFail(pState, tr("Invalid 7z pack-stream allocation"));
        }
    }

    if ((nDeclaredDataFiles != nExpectedDataFiles) || (nExplicitSizeIndex != pState->listFileSizes.count()) ||
        (nCoderSizeIndex != pState->listCodersSizes.count()) || (nConsumedPackStreams != pState->listInStreams.count())) {
        return _validationFail(pState, tr("Inconsistent 7z file, folder, or stream counts"));
    }

    if (!pState->listInStreams.isEmpty()) {
        if ((pState->nStreamsBegin < (qint64)sizeof(SIGNATUREHEADER)) || (pState->nStreamsBegin > nPackDataLimit)) {
            return _validationFail(pState, tr("Invalid 7z packed-data offset"));
        }

        qint64 nExpectedRelativeOffset = 0;
        for (const SZINSTREAM &stream : pState->listInStreams) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                return _validationFail(pState, tr("7z parsing canceled"));
            }
            if ((stream.nOffset != nExpectedRelativeOffset) || (stream.nSize < 0) ||
                (stream.nSize > (nPackDataLimit - pState->nStreamsBegin - nExpectedRelativeOffset))) {
                return _validationFail(pState, tr("Invalid 7z packed-data stream range"));
            }
            nExpectedRelativeOffset += stream.nSize;
        }
    }

    return true;
}

QMap<XBinary::UNPACK_PROP, QVariant> XSevenZip::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    if (isEncrypted()) {
        result.insert(XBinary::UNPACK_PROP_PASSWORD, QString());
    }

    return result;
}

bool XSevenZip::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSevenZip> guardedArchive(this);
    const XBinary::PDSTRUCTLIFETIME progressLifetime =
        pPdStruct ? XBinary::retainPdStructLifetime(pPdStruct) : XBinary::PDSTRUCTLIFETIME();
    if (!pState || m_bUnpackOperationInProgress ||
        ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
         !guardedArchive->ownsUnpackSource(pState))) {
        return false;
    }
    if (!guardedArchive->finishUnpack(pState, nullptr) || !guardedArchive)
        return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    bool bResult = false;
    SEVENZ_UNPACK_CONTEXT *pContext = nullptr;

    QString sMD5;

    if (pState) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        pState->nCurrentOffset = 0;
        pState->nTotalSize = guardedArchive->getSize();
        if (!guardedArchive) {
            *pState = UNPACK_STATE();
            return false;
        }
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 0;
        pState->pContext = nullptr;
        pState->mapUnpackProperties = mapProperties;

        const bool bBound = guardedArchive->bindUnpackSource(
            pState, pPdStruct);
        if (!guardedArchive || !bBound) {
            *pState = UNPACK_STATE();
            return false;
        }

        QByteArray baData;
        qint64 nNextHeaderOffset = 0;

        const bool bLoadedHeader = guardedArchive->_loadValidatedNextHeader(
            &baData, &nNextHeaderOffset, pPdStruct);
        if (!guardedArchive) {
            *pState = UNPACK_STATE();
            return false;
        }
        if (!bLoadedHeader || baData.isEmpty()) {
            guardedArchive->releaseUnpackSource(pState);
            *pState = UNPACK_STATE();
            return false;
        }
        qint64 nNextHeaderSize = baData.size();

        // Create context
        pContext = new (std::nothrow) SEVENZ_UNPACK_CONTEXT;
        if (!pContext) {
            guardedArchive->releaseUnpackSource(pState);
            *pState = UNPACK_STATE();
            return false;
        }
        // pContext->nSignatureSize = sizeof(SIGNATUREHEADER);

        pState->pContext = pContext;
        if (!guardedArchive->registerUnpackContextCleanup(
                pState, pContext,
                &deleteUnpackContext<SEVENZ_UNPACK_CONTEXT>)) {
            pState->pContext = nullptr;
            guardedArchive->releaseUnpackSource(pState);
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }

        // Parse archive structure directly using streaming approach
        {
            // char *pData = new char[nNextHeaderSize];
            // char *pUnpackedData = nullptr;
            // char *pHeaderData = nullptr;
            qint64 nHeaderSize = nNextHeaderSize;

            if (XBinary::isPdStructNotCanceled(pPdStruct)) {
                bool bHeader = false;
                bool bIsEncodedHeader = false;
                if (nNextHeaderSize > 0) {
                    quint8 nFirstByte = (quint8)baData.data()[0];
                    bIsEncodedHeader = (nFirstByte == (quint8)k7zIdEncodedHeader);
                }

                if (bIsEncodedHeader) {
                    QList<XSevenZip::SZRECORD> listRecords;

                    SZSTATE state = {};
                    state.pData = baData.data();
                    state.nSize = nNextHeaderSize;
                    state.nCurrentOffset = 0;
                    state.bIsError = false;
                    state.sErrorString = QString();

                    bool bEncodedHeaderParsed =
                        guardedArchive->_handleId(
                            &listRecords, XSevenZip::k7zIdEncodedHeader,
                            &state, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
                    if (!guardedArchive) return false;

                    bool bEncodedHeaderValid = false;
                    if (bEncodedHeaderParsed && !state.bIsError &&
                        (state.nCurrentOffset == state.nSize)) {
                        bEncodedHeaderValid =
                            guardedArchive->_validateEncodedHeader(
                                &state, nNextHeaderOffset);
                        if (!guardedArchive) return false;
                    }
                    if (bEncodedHeaderValid &&
                        XBinary::isPdStructNotCanceled(pPdStruct)) {
                        baData.clear();

                        QBuffer bufferOut;
                        bufferOut.setBuffer(&baData);

                        if (bufferOut.open(QIODevice::ReadWrite)) {
                            bHeader = guardedArchive->decompressHeader(
                                mapProperties, &bufferOut, &state,
                                pPdStruct);
                            if (!guardedArchive) return false;
                            bufferOut.close();
                            // Update nHeaderSize to actual decompressed size
                            nHeaderSize = baData.size();
                        }
                    }
                } else {
                    bHeader = true;
                }

                if (bHeader) {
                    QList<XSevenZip::SZRECORD> listRecords;

                    SZSTATE state = {};
                    state.pData = baData.data();
                    state.nSize = nHeaderSize;
                    state.nCurrentOffset = 0;
                    state.bIsError = false;
                    state.sErrorString = QString();

                    bool bHeaderParsed = guardedArchive->_handleId(
                        &listRecords, XSevenZip::k7zIdHeader, &state, 1,
                        true, pPdStruct, IMPTYPE_UNKNOWN);
                    if (!guardedArchive) return false;
                    // _printRecords(&listRecords);

                    bool bParsedHeaderValid = false;
                    if (bHeaderParsed && !state.bIsError &&
                        (state.nCurrentOffset == state.nSize)) {
                        bParsedHeaderValid =
                            guardedArchive->_validateParsedHeader(
                                &state, nNextHeaderOffset, pPdStruct);
                        if (!guardedArchive) return false;
                    }
                    if (bParsedHeaderValid &&
                        XBinary::isPdStructNotCanceled(pPdStruct)) {
                        qint32 nNumberOfFiles = state.listFileNames.count();
                        qint32 nNumberOfFolders = state.listFolders.count();
                        qint32 nNumberOfEmptyStreams = XBinary::_getBitCount_safe(state.baEmptyStreams.data(), state.baEmptyStreams.size());
                        bool bEmptyFilesPresent = (state.baEmptyFiles.size() > 0);

                    qint64 nCurrentCompressedOffset = 0;
                    Q_UNUSED(nCurrentCompressedOffset)
                    qint64 nCurrentUncompressedOffset = 0;
                    qint32 nCurrentFolder = 0;
                    qint32 nCurrentEmptyStreamIndex = 0;

                    // Build per-folder file counts
                    QList<qint32> listFolderFileCounts;
                    for (qint32 nFi = 0; nFi < nNumberOfFolders; nFi++) {
                        qint32 nFolderFileCount = 1;
                        if (nFi < state.listNumUnpackedStreams.count()) {
                            nFolderFileCount = (qint32)state.listNumUnpackedStreams.at(nFi);
                        }
                        listFolderFileCounts.append(nFolderFileCount);
                    }

                    // Build per-folder global pack-stream start index.
                    // listStreamIndexes stores relative indices within each folder's pack-stream
                    // allocation; folders consume pack streams sequentially.
                    QList<qint32> listFolderStreamOffset;
                    {
                        qint32 nRunningStreamOffset = 0;
                        for (qint32 nFi = 0; nFi < nNumberOfFolders; nFi++) {
                            listFolderStreamOffset.append(nRunningStreamOffset);
                            if (nFi < (qint32)state.listFolders.count()) {
                                nRunningStreamOffset += state.listFolders.at(nFi).listStreamIndexes.count();
                            } else {
                                nRunningStreamOffset++;
                            }
                        }
                    }

                    qint32 nFileSizeIndex = 0;
                    qint32 nFileCRCIndex = 0;
                    qint32 nFileIndexInCurrentFolder = 0;
                    QPointer<QIODevice> guardedHashDevice(guardedArchive->getDevice());
                    SevenZipHashProgressBridge hashBridge = {pPdStruct, progressLifetime};
                    XBinary::PDSTRUCT hashProgress = XBinary::getPdStructSnapshot(pPdStruct);
                    if (pPdStruct) {
                        XBinary::setPdStructCallback(&hashProgress,
                                                     sevenZipHashProgressCallback,
                                                     &hashBridge);
                    }
                    sMD5 = XBinary::getHash(XBinary::HASH_MD5,
                                            guardedHashDevice.data(),
                                            &hashProgress);
                    if (!guardedArchive || !guardedHashDevice ||
                        (pPdStruct && !XBinary::isPdStructLifetimeAlive(progressLifetime))) {
                        return false;
                    }
                    const bool bHashReady = !sMD5.isEmpty();

                    for (qint32 nCurrentFileIndex = 0;
                         (nCurrentFileIndex < nNumberOfFiles) && bHashReady &&
                         XBinary::isPdStructNotCanceled(pPdStruct);
                         nCurrentFileIndex++) {
                        ARCHIVERECORD record = {};
                        record.mapProperties.insert(FPART_PROP_FILEMD5, sMD5);

                        // Determine if this file has no data stream (empty dir or empty file)
                        bool bCurrentFileIsEmpty = false;
                        if (state.baEmptyStreams.size() > 0) {
                            bCurrentFileIsEmpty = XBinary::_read_bool_safe_rev(state.baEmptyStreams.data(), state.baEmptyStreams.size(), nCurrentFileIndex);
                        }

                        if (!bCurrentFileIsEmpty) {
                            while ((nCurrentFolder < nNumberOfFolders) && (nCurrentFolder < listFolderFileCounts.count()) &&
                                   (listFolderFileCounts.at(nCurrentFolder) == 0)) {
                                nCurrentFolder++;
                                nFileIndexInCurrentFolder = 0;
                                nCurrentUncompressedOffset = 0;
                            }
                        }

                        if (bCurrentFileIsEmpty) {
                            bool bIsFile = false;
                            if (bEmptyFilesPresent) {
                                if (XBinary::_read_bool_safe_rev(state.baEmptyFiles.data(), state.baEmptyFiles.size(), nCurrentEmptyStreamIndex)) {
                                    bIsFile = true;
                                }
                            }
                            nCurrentEmptyStreamIndex++;

                            record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (quint32)HANDLE_METHOD_STORE);
                            record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0);
                            record.mapProperties.insert(FPART_PROP_ISFOLDER, !bIsFile);
                            record.mapProperties.insert(FPART_PROP_ISSOLID, false);
                        } else if (nCurrentFolder < nNumberOfFolders) {
                            // This file has a data stream; map it to the current folder
                            qint32 nFilesInFolder = (nCurrentFolder < listFolderFileCounts.count()) ? listFolderFileCounts.at(nCurrentFolder) : 1;
                            bool bIsSolid = (nFilesInFolder > 1);

                            // Resolve pack stream index for this folder using the global offset
                            qint32 nFolderStreamBase = (nCurrentFolder < listFolderStreamOffset.count()) ? listFolderStreamOffset.at(nCurrentFolder) : nCurrentFolder;
                            // listStreamIndexes contains coder input IDs. Its
                            // position, not its value, maps to the sequential
                            // global PackInfo stream list.
                            qint32 nStreamListIndex = nFolderStreamBase;

                            qint64 nStreamOffset = 0;
                            qint64 nStreamSize = 0;
                            if (nStreamListIndex < (qint32)state.listInStreams.count()) {
                                nStreamOffset = state.nStreamsBegin + state.listInStreams.at(nStreamListIndex).nOffset;
                                nStreamSize = state.listInStreams.at(nStreamListIndex).nSize;
                            }

                            // Compute this folder's total decompressed size (last coder's output)
                            qint32 nCoderSizesOffset = 0;
                            for (qint32 nFi = 0; nFi < nCurrentFolder; nFi++) {
                                if (nFi < (qint32)state.listFolders.count()) {
                                    nCoderSizesOffset += state.listFolders.at(nFi).listCoders.count();
                                } else {
                                    nCoderSizesOffset++;
                                }
                            }
                            qint32 nFolderCoderCount = 1;
                            if (nCurrentFolder < (qint32)state.listFolders.count()) {
                                nFolderCoderCount = state.listFolders.at(nCurrentFolder).listCoders.count();
                            }
                            qint64 nFolderDecompressedSize = 0;
                            if ((nCoderSizesOffset + nFolderCoderCount - 1) < (qint32)state.listCodersSizes.count()) {
                                nFolderDecompressedSize = (qint64)state.listCodersSizes.at(nCoderSizesOffset + nFolderCoderCount - 1);
                            }

                            // Resolve compression method and properties from this folder's coders
                            HANDLE_METHOD cm = HANDLE_METHOD_STORE;
                            QByteArray baCoderProperty;
                            HANDLE_METHOD cm2 = HANDLE_METHOD_STORE;
                            QByteArray baCoderProperty2;
                            HANDLE_METHOD cm3 = HANDLE_METHOD_STORE;
                            QByteArray baCoderProperty3;
                            bool bHasSecondCoder = false;
                            bool bHasThirdCoder = false;
                            qint32 nSecondCoderSizeIdx = -1;  // index into listCodersSizes for cm2's output size
                            bool bBCJ2Resolved = false;
                            qint64 nBCJ2MainOffset = 0;
                            qint64 nBCJ2MainSize = 0;
                            HANDLE_METHOD cmBCJ2Main = HANDLE_METHOD_STORE;
                            QByteArray baBCJ2MainProp;
                            qint64 nBCJ2MainUnpack = 0;
                            qint64 nBCJ2CallOffset = 0;
                            qint64 nBCJ2CallSize = 0;
                            HANDLE_METHOD cmBCJ2Call = HANDLE_METHOD_STORE;
                            QByteArray baBCJ2CallProp;
                            qint64 nBCJ2CallUnpack = 0;
                            qint64 nBCJ2JmpOffset = 0;
                            qint64 nBCJ2JmpSize = 0;
                            HANDLE_METHOD cmBCJ2Jmp = HANDLE_METHOD_STORE;
                            QByteArray baBCJ2JmpProp;
                            qint64 nBCJ2JmpUnpack = 0;
                            qint64 nBCJ2RangeOffset = 0;
                            qint64 nBCJ2RangeSize = 0;
                            qint64 nBCJ2OutputSize = 0;
                            // Per-stream AES properties for AES-encrypted BCJ2 archives
                            QByteArray baBCJ2MainAESProp;
                            qint64 nBCJ2MainAESUnpack = 0;
                            QByteArray baBCJ2CallAESProp;
                            qint64 nBCJ2CallAESUnpack = 0;
                            QByteArray baBCJ2JmpAESProp;
                            qint64 nBCJ2JmpAESUnpack = 0;
                            QByteArray baBCJ2RangeAESProp;
                            qint64 nBCJ2RangeAESUnpack = 0;
                            if (nCurrentFolder < (qint32)state.listFolders.count()) {
                                const SZFOLDER &folder = state.listFolders.at(nCurrentFolder);
                                qint32 nNumCoders = folder.listCoders.count();
                                if (nNumCoders >= 1) {
                                    qint32 nLastCoder = nNumCoders - 1;
                                    cm = coderToCompressMethod(folder.listCoders.at(nLastCoder).baCoder);
                                    baCoderProperty = folder.listCoders.at(nLastCoder).baProperty;
                                    // BCJ2 is a 4-stream filter listed as coder[0]; scan for it and
                                    // override cm so the BCJ2 path in unpackCurrent fires correctly.
                                    for (qint32 nCi = 0; nCi < nNumCoders; nCi++) {
                                        if (coderToCompressMethod(folder.listCoders.at(nCi).baCoder) == HANDLE_METHOD_BCJ2) {
                                            cm = HANDLE_METHOD_BCJ2;
                                            break;
                                        }
                                    }
                                    if (nNumCoders >= 2 && cm != HANDLE_METHOD_BCJ2) {
                                        // For N coders, decompression order is coder[N-1] → coder[N-2] → ... → coder[0]
                                        // cm  = coder[N-1] (innermost, applied last — e.g. BCJ filter)
                                        // cm2 = coder[N-2] (middle — e.g. LZMA2)
                                        // cm3 = coder[N-3] / coder[0] (outermost, applied first — e.g. AES decrypt)
                                        cm2 = coderToCompressMethod(folder.listCoders.at(nLastCoder - 1).baCoder);
                                        baCoderProperty2 = folder.listCoders.at(nLastCoder - 1).baProperty;
                                        bHasSecondCoder = true;
                                        nSecondCoderSizeIdx = nCoderSizesOffset + (nLastCoder - 1);
                                    }
                                    if (nNumCoders >= 3 && cm != HANDLE_METHOD_BCJ2) {
                                        cm3 = coderToCompressMethod(folder.listCoders.at(0).baCoder);
                                        baCoderProperty3 = folder.listCoders.at(0).baProperty;
                                        bHasThirdCoder = true;
                                    }
                                    if (cm == HANDLE_METHOD_BCJ2) {
                                        // Resolve the 4 BCJ2 stream coordinates at parse time so that
                                        // XDecompress::decompress() can handle BCJ2 as a normal single-method.
                                        // Two layouts are supported:
                                        //   Classic: 4 coders (BCJ2 + LZMA2 + LZMA + LZMA), 3 bonds, 4 pack streams
                                        //   Compact: 2 coders (BCJ2 + LZMA2), 1 bond, 4 pack streams
                                        //            calls/jumps streams stored raw (STORE) in pack data
                                        qint32 nLocalPackCount = folder.listStreamIndexes.count();
                                        qint32 nLocalBondCount = folder.listBonds.count();
                                        if (nLocalPackCount >= 4) {
                                            // Map folder InStream index → global pack-stream index
                                            QMap<qint32, qint32> mapInStreamToGlobal;
                                            for (qint32 k = 0; k < nLocalPackCount; k++) {
                                                qint32 nInStreamIdx = folder.listStreamIndexes.at(k);
                                                mapInStreamToGlobal[nInStreamIdx] = nFolderStreamBase + k;
                                            }
                                            // Cumulative InStream offsets per coder within this folder
                                            QList<qint32> listInStreamOffsets;
                                            qint32 nRunningInStream = 0;
                                            for (qint32 ci = 0; ci < nNumCoders; ci++) {
                                                listInStreamOffsets.append(nRunningInStream);
                                                nRunningInStream += folder.listCoders.at(ci).nNumInStreams;
                                            }
                                            // Find BCJ2 coder local index
                                            qint32 nBCJ2LocalIdx = -1;
                                            for (qint32 ci = 0; ci < nNumCoders; ci++) {
                                                if (coderToCompressMethod(folder.listCoders.at(ci).baCoder) == HANDLE_METHOD_BCJ2) {
                                                    nBCJ2LocalIdx = ci;
                                                    break;
                                                }
                                            }
                                            if (nBCJ2LocalIdx >= 0) {
                                                qint32 nBCJ2InStreamBase = listInStreamOffsets.at(nBCJ2LocalIdx);

                                                // Build bond map: in-stream index → source coder index
                                                QMap<qint32, qint32> mapBondInToCoderOut;
                                                for (qint32 bi = 0; bi < nLocalBondCount; bi++) {
                                                    mapBondInToCoderOut[folder.listBonds.at(bi).nInputIndex] = folder.listBonds.at(bi).nOutputIndex;
                                                }

                                                // Resolve range stream (BCJ2.in[3])
                                                qint32 nRangeAESCoderIdx = -1;
                                                qint32 nRangeGlobal = sevenzipResolveInStream(mapInStreamToGlobal, mapBondInToCoderOut, listInStreamOffsets, nNumCoders, nBCJ2InStreamBase + 3, &nRangeAESCoderIdx);

                                                if (nRangeGlobal >= 0 && nRangeGlobal < state.listInStreams.count()) {
                                                    nBCJ2RangeOffset = state.nStreamsBegin + state.listInStreams.at(nRangeGlobal).nOffset;
                                                    nBCJ2RangeSize = state.listInStreams.at(nRangeGlobal).nSize;
                                                    if (nRangeAESCoderIdx >= 0) {
                                                        baBCJ2RangeAESProp = folder.listCoders.at(nRangeAESCoderIdx).baProperty;
                                                        nBCJ2RangeAESUnpack = (nCoderSizesOffset + nRangeAESCoderIdx < state.listCodersSizes.count())
                                                                                  ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nRangeAESCoderIdx)
                                                                                  : 0;
                                                    }

                                                    // Scan bonds for BCJ2 inputs 0=main, 1=calls, 2=jumps
                                                    qint32 nMainLZMALocal = -1;
                                                    qint32 nCallLZMALocal = -1;
                                                    qint32 nJmpLZMALocal = -1;
                                                    for (qint32 bi = 0; bi < nLocalBondCount; bi++) {
                                                        qint32 nInIdx = folder.listBonds.at(bi).nInputIndex;
                                                        qint32 nOutIdx = folder.listBonds.at(bi).nOutputIndex;
                                                        qint32 nBCJ2LocalInput = nInIdx - nBCJ2InStreamBase;
                                                        if (nBCJ2LocalInput == 0) {
                                                            nMainLZMALocal = nOutIdx;
                                                        } else if (nBCJ2LocalInput == 1) {
                                                            nCallLZMALocal = nOutIdx;
                                                        } else if (nBCJ2LocalInput == 2) {
                                                            nJmpLZMALocal = nOutIdx;
                                                        }
                                                    }

                                                    if (nMainLZMALocal >= 0) {
                                                        qint32 nMainAESCoderIdx = -1;
                                                        qint32 nMainGlobal = sevenzipResolveInStream(mapInStreamToGlobal, mapBondInToCoderOut, listInStreamOffsets, nNumCoders, listInStreamOffsets.at(nMainLZMALocal), &nMainAESCoderIdx);
                                                        if (nMainGlobal >= 0 && nMainGlobal < state.listInStreams.count()) {
                                                            nBCJ2MainOffset = state.nStreamsBegin + state.listInStreams.at(nMainGlobal).nOffset;
                                                            nBCJ2MainSize = state.listInStreams.at(nMainGlobal).nSize;
                                                            cmBCJ2Main = coderToCompressMethod(folder.listCoders.at(nMainLZMALocal).baCoder);
                                                            baBCJ2MainProp = folder.listCoders.at(nMainLZMALocal).baProperty;
                                                            nBCJ2MainUnpack = (nCoderSizesOffset + nMainLZMALocal < state.listCodersSizes.count())
                                                                                  ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nMainLZMALocal)
                                                                                  : 0;
                                                            nBCJ2OutputSize = (nCoderSizesOffset + nBCJ2LocalIdx < state.listCodersSizes.count())
                                                                                  ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nBCJ2LocalIdx)
                                                                                  : 0;
                                                            if (nMainAESCoderIdx >= 0) {
                                                                baBCJ2MainAESProp = folder.listCoders.at(nMainAESCoderIdx).baProperty;
                                                                nBCJ2MainAESUnpack = (nCoderSizesOffset + nMainAESCoderIdx < state.listCodersSizes.count())
                                                                                         ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nMainAESCoderIdx)
                                                                                         : 0;
                                                            }

                                                            // Resolve calls stream
                                                            bool bCallOk = false;
                                                            if (nCallLZMALocal >= 0) {
                                                                qint32 nCallAESCoderIdx = -1;
                                                                qint32 nCallGlobal = sevenzipResolveInStream(mapInStreamToGlobal, mapBondInToCoderOut, listInStreamOffsets, nNumCoders, listInStreamOffsets.at(nCallLZMALocal), &nCallAESCoderIdx);
                                                                if (nCallGlobal >= 0 && nCallGlobal < state.listInStreams.count()) {
                                                                    nBCJ2CallOffset = state.nStreamsBegin + state.listInStreams.at(nCallGlobal).nOffset;
                                                                    nBCJ2CallSize = state.listInStreams.at(nCallGlobal).nSize;
                                                                    cmBCJ2Call = coderToCompressMethod(folder.listCoders.at(nCallLZMALocal).baCoder);
                                                                    baBCJ2CallProp = folder.listCoders.at(nCallLZMALocal).baProperty;
                                                                    nBCJ2CallUnpack = (nCoderSizesOffset + nCallLZMALocal < state.listCodersSizes.count())
                                                                                          ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nCallLZMALocal)
                                                                                          : 0;
                                                                    if (nCallAESCoderIdx >= 0) {
                                                                        baBCJ2CallAESProp = folder.listCoders.at(nCallAESCoderIdx).baProperty;
                                                                        nBCJ2CallAESUnpack = (nCoderSizesOffset + nCallAESCoderIdx < state.listCodersSizes.count())
                                                                                                 ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nCallAESCoderIdx)
                                                                                                 : 0;
                                                                    }
                                                                    bCallOk = true;
                                                                }
                                                            } else {
                                                                // Compact layout: BCJ2.in[1] (calls) is a raw/encrypted direct stream
                                                                qint32 nCallAESCoderIdx = -1;
                                                                qint32 nCallGlobal = sevenzipResolveInStream(mapInStreamToGlobal, mapBondInToCoderOut, listInStreamOffsets, nNumCoders, nBCJ2InStreamBase + 1, &nCallAESCoderIdx);
                                                                if (nCallGlobal >= 0 && nCallGlobal < state.listInStreams.count()) {
                                                                    nBCJ2CallOffset = state.nStreamsBegin + state.listInStreams.at(nCallGlobal).nOffset;
                                                                    nBCJ2CallSize = state.listInStreams.at(nCallGlobal).nSize;
                                                                    cmBCJ2Call = HANDLE_METHOD_STORE;
                                                                    nBCJ2CallUnpack = nBCJ2CallSize;
                                                                    if (nCallAESCoderIdx >= 0) {
                                                                        baBCJ2CallAESProp = folder.listCoders.at(nCallAESCoderIdx).baProperty;
                                                                        nBCJ2CallAESUnpack = (nCoderSizesOffset + nCallAESCoderIdx < state.listCodersSizes.count())
                                                                                                 ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nCallAESCoderIdx)
                                                                                                 : 0;
                                                                    }
                                                                    bCallOk = true;
                                                                }
                                                            }

                                                            // Resolve jumps stream
                                                            bool bJmpOk = false;
                                                            if (nJmpLZMALocal >= 0) {
                                                                qint32 nJmpAESCoderIdx = -1;
                                                                qint32 nJmpGlobal = sevenzipResolveInStream(mapInStreamToGlobal, mapBondInToCoderOut, listInStreamOffsets, nNumCoders, listInStreamOffsets.at(nJmpLZMALocal), &nJmpAESCoderIdx);
                                                                if (nJmpGlobal >= 0 && nJmpGlobal < state.listInStreams.count()) {
                                                                    nBCJ2JmpOffset = state.nStreamsBegin + state.listInStreams.at(nJmpGlobal).nOffset;
                                                                    nBCJ2JmpSize = state.listInStreams.at(nJmpGlobal).nSize;
                                                                    cmBCJ2Jmp = coderToCompressMethod(folder.listCoders.at(nJmpLZMALocal).baCoder);
                                                                    baBCJ2JmpProp = folder.listCoders.at(nJmpLZMALocal).baProperty;
                                                                    nBCJ2JmpUnpack = (nCoderSizesOffset + nJmpLZMALocal < state.listCodersSizes.count())
                                                                                         ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nJmpLZMALocal)
                                                                                         : 0;
                                                                    if (nJmpAESCoderIdx >= 0) {
                                                                        baBCJ2JmpAESProp = folder.listCoders.at(nJmpAESCoderIdx).baProperty;
                                                                        nBCJ2JmpAESUnpack = (nCoderSizesOffset + nJmpAESCoderIdx < state.listCodersSizes.count())
                                                                                                ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nJmpAESCoderIdx)
                                                                                                : 0;
                                                                    }
                                                                    bJmpOk = true;
                                                                }
                                                            } else {
                                                                // Compact layout: BCJ2.in[2] (jumps) is a raw/encrypted direct stream
                                                                qint32 nJmpAESCoderIdx = -1;
                                                                qint32 nJmpGlobal = sevenzipResolveInStream(mapInStreamToGlobal, mapBondInToCoderOut, listInStreamOffsets, nNumCoders, nBCJ2InStreamBase + 2, &nJmpAESCoderIdx);
                                                                if (nJmpGlobal >= 0 && nJmpGlobal < state.listInStreams.count()) {
                                                                    nBCJ2JmpOffset = state.nStreamsBegin + state.listInStreams.at(nJmpGlobal).nOffset;
                                                                    nBCJ2JmpSize = state.listInStreams.at(nJmpGlobal).nSize;
                                                                    cmBCJ2Jmp = HANDLE_METHOD_STORE;
                                                                    nBCJ2JmpUnpack = nBCJ2JmpSize;
                                                                    if (nJmpAESCoderIdx >= 0) {
                                                                        baBCJ2JmpAESProp = folder.listCoders.at(nJmpAESCoderIdx).baProperty;
                                                                        nBCJ2JmpAESUnpack = (nCoderSizesOffset + nJmpAESCoderIdx < state.listCodersSizes.count())
                                                                                                ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nJmpAESCoderIdx)
                                                                                                : 0;
                                                                    }
                                                                    bJmpOk = true;
                                                                }
                                                            }

                                                            if (bCallOk && bJmpOk) {
                                                                bBCJ2Resolved = true;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Determine this file's uncompressed size
                            qint64 nFileSize = 0;
                            if (bIsSolid && (nFileIndexInCurrentFolder < (nFilesInFolder - 1))) {
                                // Not the last file in this solid folder; use explicit size
                                if (nFileSizeIndex < (qint32)state.listFileSizes.count()) {
                                    nFileSize = state.listFileSizes.at(nFileSizeIndex);
                                }
                                nFileSizeIndex++;
                            } else {
                                // Last (or only) file: remainder of decompressed block
                                nFileSize = nFolderDecompressedSize - nCurrentUncompressedOffset;
                            }

                            // A single-substream folder inherits its defined
                            // folder CRC. Other folders consume the aligned raw
                            // SubStreamsInfo digest list, including undefined slots.
                            quint32 nFileCRC = 0;
                            bool bFileCRCDefined = false;
                            bool bUseFolderCRC = (nFilesInFolder == 1) && (nCurrentFolder < (qint32)state.listOutStreams.count()) &&
                                                 state.listOutStreams.at(nCurrentFolder).bCRCDefined;

                            if (bUseFolderCRC) {
                                nFileCRC = state.listOutStreams.at(nCurrentFolder).nCRC;
                                bFileCRCDefined = true;
                            } else if (nFileCRCIndex < (qint32)state.listFileCRC.count()) {
                                nFileCRC = state.listFileCRC.at(nFileCRCIndex);
                                bFileCRCDefined = state.listFileCRCDefined.value(nFileCRCIndex, false);
                                nFileCRCIndex++;
                            }

                            record.nStreamOffset = bBCJ2Resolved ? nBCJ2MainOffset : nStreamOffset;
                            record.nStreamSize = bBCJ2Resolved ? nBCJ2MainSize : nStreamSize;
                            record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (quint32)cm);
                            record.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, bBCJ2Resolved ? baBCJ2MainProp : baCoderProperty);
                            if (bHasSecondCoder) {
                                record.mapProperties.insert(FPART_PROP_HANDLEMETHOD2, (quint32)cm2);
                                record.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES2, baCoderProperty2);
                                // AES output size = coder[N-2] output = intermediate size needed for proper truncation
                                if (nSecondCoderSizeIdx >= 0 && nSecondCoderSizeIdx < (qint32)state.listCodersSizes.count()) {
                                    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE2, (qint64)state.listCodersSizes.at(nSecondCoderSizeIdx));
                                }
                            }
                            if (bHasThirdCoder) {
                                record.mapProperties.insert(FPART_PROP_HANDLEMETHOD3, (quint32)cm3);
                                record.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES3, baCoderProperty3);
                                // Outermost coder (coder[0]) output size
                                if (nCoderSizesOffset < (qint32)state.listCodersSizes.count()) {
                                    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE3, (qint64)state.listCodersSizes.at(nCoderSizesOffset));
                                }
                            }
                            if (bBCJ2Resolved) {
                                record.mapProperties.insert(FPART_PROP_HANDLEMETHOD4, (quint32)cmBCJ2Main);
                                record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE4, nBCJ2MainUnpack);
                                record.mapProperties.insert(FPART_PROP_HANDLEMETHOD2, (quint32)cmBCJ2Call);
                                record.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES2, baBCJ2CallProp);
                                record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE2, nBCJ2CallUnpack);
                                record.mapProperties.insert(FPART_PROP_STREAMOFFSET2, nBCJ2CallOffset);
                                record.mapProperties.insert(FPART_PROP_STREAMSIZE2, nBCJ2CallSize);
                                record.mapProperties.insert(FPART_PROP_HANDLEMETHOD3, (quint32)cmBCJ2Jmp);
                                record.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES3, baBCJ2JmpProp);
                                record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE3, nBCJ2JmpUnpack);
                                record.mapProperties.insert(FPART_PROP_STREAMOFFSET3, nBCJ2JmpOffset);
                                record.mapProperties.insert(FPART_PROP_STREAMSIZE3, nBCJ2JmpSize);
                                record.mapProperties.insert(FPART_PROP_STREAMOFFSET4, nBCJ2RangeOffset);
                                record.mapProperties.insert(FPART_PROP_STREAMSIZE4, nBCJ2RangeSize);
                                // AES encryption properties for BCJ2 sub-streams (encrypted BCJ2 layout)
                                if (!baBCJ2MainAESProp.isEmpty()) {
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_PROPS_0, baBCJ2MainAESProp);
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_UNPACK_0, nBCJ2MainAESUnpack);
                                }
                                if (!baBCJ2CallAESProp.isEmpty()) {
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_PROPS_1, baBCJ2CallAESProp);
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_UNPACK_1, nBCJ2CallAESUnpack);
                                }
                                if (!baBCJ2JmpAESProp.isEmpty()) {
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_PROPS_2, baBCJ2JmpAESProp);
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_UNPACK_2, nBCJ2JmpAESUnpack);
                                }
                                if (!baBCJ2RangeAESProp.isEmpty()) {
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_PROPS_3, baBCJ2RangeAESProp);
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_UNPACK_3, nBCJ2RangeAESUnpack);
                                }
                            }
                            record.mapProperties.insert(FPART_PROP_STREAMUNPACKEDSIZE, nFolderDecompressedSize);
                            record.mapProperties.insert(FPART_PROP_SUBSTREAMOFFSET, nCurrentUncompressedOffset);
                            record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nFileSize);
                            if (bFileCRCDefined) {
                                record.mapProperties.insert(FPART_PROP_CRC_TYPE, (quint32)CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
                                record.mapProperties.insert(FPART_PROP_RESULTCRC, nFileCRC);
                            }
                            // For solid folders, also store the whole-folder unpack CRC for post-decompression block verification
                            if (bIsSolid && nCurrentFolder < (qint32)state.listOutStreams.count()) {
                                quint32 nFolderCRC = state.listOutStreams.at(nCurrentFolder).nCRC;
                                if (state.listOutStreams.at(nCurrentFolder).bCRCDefined) {
                                    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDCRC, nFolderCRC);
                                }
                            }
                            record.mapProperties.insert(FPART_PROP_ISFOLDER, false);
                            record.mapProperties.insert(FPART_PROP_ISSOLID, bIsSolid);
                            record.mapProperties.insert(FPART_PROP_SOLIDFOLDERINDEX, (qint64)nCurrentFolder);

                            nCurrentUncompressedOffset += nFileSize;
                            nFileIndexInCurrentFolder++;

                            // Advance to next folder when current folder's files are exhausted
                            if (nFileIndexInCurrentFolder >= nFilesInFolder) {
                                nCurrentFolder++;
                                nFileIndexInCurrentFolder = 0;
                                nCurrentUncompressedOffset = 0;
                            }
                        }

                        record.mapProperties.insert(FPART_PROP_ORIGINALNAME, state.listFileNames.at(nCurrentFileIndex));

                        // Decode per-file timestamps and Windows attributes
                        quint64 nWinFileTime = 0;
                        if (_decode7zTimeValue(state.baMTime, nNumberOfFiles, nCurrentFileIndex, &nWinFileTime)) {
                            QDateTime dtMTime = XBinary::winFileTimeToQDateTime(nWinFileTime);
                            if (dtMTime.isValid()) {
                                record.mapProperties.insert(FPART_PROP_MTIME, dtMTime);
                            }
                        }
                        nWinFileTime = 0;
                        if (_decode7zTimeValue(state.baCTime, nNumberOfFiles, nCurrentFileIndex, &nWinFileTime)) {
                            QDateTime dtCTime = XBinary::winFileTimeToQDateTime(nWinFileTime);
                            if (dtCTime.isValid()) {
                                record.mapProperties.insert(FPART_PROP_CTIME, dtCTime);
                            }
                        }
                        nWinFileTime = 0;
                        if (_decode7zTimeValue(state.baATime, nNumberOfFiles, nCurrentFileIndex, &nWinFileTime)) {
                            QDateTime dtATime = XBinary::winFileTimeToQDateTime(nWinFileTime);
                            if (dtATime.isValid()) {
                                record.mapProperties.insert(FPART_PROP_ATIME, dtATime);
                            }
                        }
                        quint32 nWinAttrib = 0;
                        if (_decode7zAttribValue(state.baWinAttrib, nNumberOfFiles, nCurrentFileIndex, &nWinAttrib)) {
                            record.mapProperties.insert(FPART_PROP_ISREADONLY, (nWinAttrib & 0x01) != 0);
                            record.mapProperties.insert(FPART_PROP_ISHIDDEN, (nWinAttrib & 0x02) != 0);
                            record.mapProperties.insert(FPART_PROP_ISSYSTEM, (nWinAttrib & 0x04) != 0);
                            record.mapProperties.insert(FPART_PROP_ISARCHIVE, (nWinAttrib & 0x20) != 0);
                        }

                        pContext->listArchiveRecords.append(record);
                    }

                        if ((pContext->listArchiveRecords.count() != nNumberOfFiles) ||
                            (nCurrentEmptyStreamIndex != nNumberOfEmptyStreams) ||
                            !XBinary::isPdStructNotCanceled(pPdStruct)) {
                            pContext->listArchiveRecords.clear();
                            guardedArchive->_errorMessage(
                                tr("Invalid or canceled 7z file map"),
                                pPdStruct);
                            if (!guardedArchive) return false;
                        }
                    } else {
                        guardedArchive->_errorMessage(
                            tr("Invalid format data"), pPdStruct);
                        if (!guardedArchive) return false;
                    }
                } else {
                    guardedArchive->_errorMessage(
                        tr("Cannot unpack data"), pPdStruct);
                    if (!guardedArchive) return false;
                }
            } else {
                guardedArchive->_errorMessage(
                    tr("Invalid format data"), pPdStruct);
                if (!guardedArchive) return false;
            }

            pState->nNumberOfRecords = pContext->listArchiveRecords.count();
            bResult = (pState->nNumberOfRecords > 0);

            if (bResult) {
                pState->mapArchiveProperties.insert(FPART_PROP_FILEMD5, sMD5);
            }

        }  // End if next header is present

        if (bResult) {
            bResult = guardedArchive->validateAndFinalizeUnpackSource(
                pState, pContext, pPdStruct);
        }
        if (!guardedArchive) return false;

        if (!bResult && pContext && (pState->pContext == pContext)) {
            pState->pContext = nullptr;
        }
    }  // End outer scope

    if (!bResult && pState) {
        guardedArchive->releaseUnpackSource(pState);
        pState->pContext = nullptr;
        delete pContext;
        pContext = nullptr;
        if (!guardedArchive) return false;
        *pState = UNPACK_STATE();
    }

    return bResult;
}

XBinary::ARCHIVERECORD XSevenZip::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();

    QPointer<XSevenZip> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    XBinary::ARCHIVERECORD result = {};

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext ||
        !guardedSource) {
        return result;
    }
    if (!guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedArchive || !guardedSource) return result;

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    SEVENZ_UNPACK_CONTEXT *pContext = (SEVENZ_UNPACK_CONTEXT *)pState->pContext;

    // Return pre-parsed archive record
    if (pState->nCurrentIndex < pContext->listArchiveRecords.count()) {
        result = pContext->listArchiveRecords.at(pState->nCurrentIndex);
    }

    return result;
}

bool XSevenZip::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    QPointer<XSevenZip> guardedArchive(this);
    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !guardedArchive->ownsUnpackSource(pState)) return false;

    SEVENZ_UNPACK_CONTEXT *pContext =
        static_cast<SEVENZ_UNPACK_CONTEXT *>(pState->pContext);
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

bool XSevenZip::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    QPointer<XSevenZip> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    bool bResult = false;

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext ||
        !guardedSource || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    if (!guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedArchive || !guardedSource) return false;

    // SEVENZ_UNPACK_CONTEXT *pContext = (SEVENZ_UNPACK_CONTEXT *)pState->pContext;

    // Move to next record
    pState->nCurrentIndex++;

    // Check if more records available
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        bResult = true;
    }

    return bResult;
}

QList<XBinary::FPART_PROP> XSevenZip::getAvailableFPARTProperties()
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

void XSevenZip::_printRecords(QList<SZRECORD> *pListRecords)
{
    if (!pListRecords) {
        return;
    }

    qDebug("=== SZRECORD list: %d records ===", pListRecords->count());

    for (qint32 nI = 0; nI < pListRecords->count(); nI++) {
        const SZRECORD &rec = pListRecords->at(nI);

        QString sSrType;
        if (rec.srType == SRTYPE_UNKNOWN) sSrType = "UNKNOWN";
        else if (rec.srType == SRTYPE_ID) sSrType = "ID";
        else if (rec.srType == SRTYPE_NUMBER) sSrType = "NUMBER";
        else if (rec.srType == SRTYPE_BYTE) sSrType = "BYTE";
        else if (rec.srType == SRTYPE_UINT32) sSrType = "UINT32";
        else if (rec.srType == SRTYPE_ARRAY) sSrType = "ARRAY";
        else sSrType = QString::number((qint32)rec.srType);

        QString sValType;
        if (rec.valType == VT_UNKNOWN) sValType = "UNKNOWN";
        else if (rec.valType == VT_BYTE) sValType = "BYTE";
        else if (rec.valType == VT_UINT16) sValType = "UINT16";
        else if (rec.valType == VT_UINT32) sValType = "UINT32";
        else if (rec.valType == VT_UINT64) sValType = "UINT64";
        else if (rec.valType == VT_INT8) sValType = "INT8";
        else if (rec.valType == VT_INT16) sValType = "INT16";
        else if (rec.valType == VT_INT32) sValType = "INT32";
        else if (rec.valType == VT_INT64) sValType = "INT64";
        else if (rec.valType == VT_PACKEDNUMBER) sValType = "PACKEDNUMBER";
        else if (rec.valType == VT_STRING) sValType = "STRING";
        else if (rec.valType == VT_BYTE_ARRAY) sValType = "BYTE_ARRAY";
        else if (rec.valType == VT_CHAR_ARRAY) sValType = "CHAR_ARRAY";
        else sValType = QString::number((qint32)rec.valType);

        QString sImpType;
        if (rec.impType == IMPTYPE_UNKNOWN) sImpType = "UNKNOWN";
        else if (rec.impType == IMPTYPE_NUMBEROFFOLDERS) sImpType = "NUMBEROFFOLDERS";
        else if (rec.impType == IMPTYPE_NUMBEROFFILES) sImpType = "NUMBEROFFILES";
        else if (rec.impType == IMPTYPE_NUMBEROFCODERS) sImpType = "NUMBEROFCODERS";
        else if (rec.impType == IMPTYPE_STREAMCRC) sImpType = "STREAMCRC";
        else if (rec.impType == IMPTYPE_STREAMOFFSET) sImpType = "STREAMOFFSET";
        else if (rec.impType == IMPTYPE_STREAMSIZE) sImpType = "STREAMSIZE";
        else if (rec.impType == IMPTYPE_CODERUNPACKEDSIZE) sImpType = "CODERUNPACKEDSIZE";
        else if (rec.impType == IMPTYPE_STREAMUNPACKEDCRC) sImpType = "STREAMUNPACKEDCRC";
        else if (rec.impType == IMPTYPE_NUMBEROFPACKSTREAMS) sImpType = "NUMBEROFPACKSTREAMS";
        else if (rec.impType == IMPTYPE_CODER) sImpType = "CODER";
        else if (rec.impType == IMPTYPE_CODERPROPERTY) sImpType = "CODERPROPERTY";
        else if (rec.impType == IMPTYPE_FILENAME) sImpType = "FILENAME";
        else if (rec.impType == IMPTYPE_FILECRC) sImpType = "FILECRC";
        else if (rec.impType == IMPTYPE_FILEATTRIBUTES) sImpType = "FILEATTRIBUTES";
        else if (rec.impType == IMPTYPE_FILETIME) sImpType = "FILETIME";
        else if (rec.impType == IMPTYPE_FILEPACKEDSIZE) sImpType = "FILEPACKEDSIZE";
        else if (rec.impType == IMPTYPE_FILEUNPACKEDSIZE) sImpType = "FILEUNPACKEDSIZE";
        else if (rec.impType == IMPTYPE_NUMBEROFUNPACKSTREAM) sImpType = "NUMBEROFUNPACKSTREAM";
        else if (rec.impType == IMPTYPE_EMPTYSTREAMDATA) sImpType = "EMPTYSTREAMDATA";
        else if (rec.impType == IMPTYPE_EMPTYFILEDATA) sImpType = "EMPTYFILEDATA";
        else if (rec.impType == IMPTYPE_CTIMEDATA) sImpType = "CTIMEDATA";
        else if (rec.impType == IMPTYPE_ATIMEDATA) sImpType = "ATIMEDATA";
        else if (rec.impType == IMPTYPE_MTIMEDATA) sImpType = "MTIMEDATA";
        else if (rec.impType == IMPTYPE_WINATTRIBDATA) sImpType = "WINATTRIBDATA";
        else sImpType = QString::number((qint32)rec.impType);

        QString sValue;
        if (rec.srType == SRTYPE_ARRAY) {
            QByteArray baValue = rec.varValue.toByteArray();
            sValue = QString("(%1 bytes) %2").arg(baValue.size()).arg(QString(baValue.toHex()));
        } else {
            sValue = rec.varValue.toString();
        }

        qDebug("[%d] offset=0x%X size=0x%X name=\"%s\" srType=%s valType=%s impType=%s flags=0x%X value=%s", nI, (quint32)rec.nRelOffset, (quint32)rec.nSize,
               rec.sName.toUtf8().constData(), sSrType.toUtf8().constData(), sValType.toUtf8().constData(), sImpType.toUtf8().constData(), rec.nFlags,
               sValue.toUtf8().constData());
    }

    qDebug("=== end SZRECORD list ===");
}

QList<XSevenZip::SZRECORD> XSevenZip::_handleData(char *pData, qint64 nSize, PDSTRUCT *pPdStruct)
{
    QList<XSevenZip::SZRECORD> listResult;

    // Validate input parameters
    if ((nSize <= 0) || (pData == nullptr) || isPdStructStopped(pPdStruct)) {
        return listResult;
    }

    // Initialize state
    SZSTATE state = {};
    state.pData = pData;
    state.nSize = nSize;
    state.nCurrentOffset = 0;
    state.bIsError = false;
    state.sErrorString = QString();

    // Check if the first byte indicates an encoded header
    bool bIsEncodedHeader = (nSize > 0 && pData[0] == XSevenZip::k7zIdEncodedHeader);

    if (bIsEncodedHeader) {
        _handleId(&listResult, XSevenZip::k7zIdEncodedHeader, &state, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
    } else {
        _handleId(&listResult, XSevenZip::k7zIdHeader, &state, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
    }

    return listResult;
}

bool XSevenZip::decompressHeader(const QMap<UNPACK_PROP, QVariant> &mapUnpackProperties, QIODevice *pDeviceOut, SZSTATE *pState, PDSTRUCT *pPdStruct)
{
    bool bResult = false;
    QPointer<XSevenZip> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    QPointer<QIODevice> guardedOutput(pDeviceOut);

    if (!pState || !guardedSource || !guardedOutput ||
        !guardedOutput->isWritable() || pState->bIsError ||
        !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (pState->listFolders.count() != 1) || pState->listInStreams.isEmpty() || (pState->listOutStreams.count() != 1)) {
        return false;
    }

    XDecompress xDecompress;
    connect(&xDecompress, &XDecompress::errorMessage, this, &XBinary::errorMessage);
    connect(&xDecompress, &XDecompress::infoMessage, this, &XBinary::infoMessage);

    if (pState->listFolders.count() == 1) {
        qint32 nFolderIndex = 0;

        const SZFOLDER &folder = pState->listFolders.at(nFolderIndex);
        qint32 nNumCoders = folder.listCoders.count();

        if ((folder.listStreamIndexes.count() != 1) || (nNumCoders <= 0) || (nNumCoders > 2) ||
            (pState->listCodersSizes.count() != nNumCoders)) {
            return false;
        }

        if (pState->listInStreams.count() != 1) {
            return false;
        }

        const SZINSTREAM &inStream = pState->listInStreams.at(0);
        if ((pState->nStreamsBegin < 0) || (inStream.nOffset < 0) || (inStream.nSize < 0) ||
            (pState->nStreamsBegin > (LLONG_MAX - inStream.nOffset))) {
            return false;
        }

        qint64 nStreamOffset = pState->nStreamsBegin + inStream.nOffset;
        qint64 nStreamSize = inStream.nSize;
        const qint64 nFileSize = guardedSource->size();
        if (!guardedArchive || !guardedSource || !guardedOutput ||
            (nStreamOffset > nFileSize) ||
            (nStreamSize > (nFileSize - nStreamOffset))) {
            return false;
        }

        quint64 nHeaderOutputSize = pState->listCodersSizes.at(nNumCoders - 1);
        if (nHeaderOutputSize > (quint64)SEVENZIP_MAX_NEXT_HEADER_SIZE) {
            return false;
        }

        // Compute the index into listCodersSizes for the first coder of this folder
        qint32 nCoderSizesOffset = 0;
        for (qint32 i = 0; i < nFolderIndex; i++) {
            nCoderSizesOffset += pState->listFolders.at(i).listCoders.count();
        }

        // Build mapProperties BEFORE assigning to state.
        // multiDecompress processes methods in reverse order:
        //   i=last uses HANDLEMETHOD2 -> reads raw stream  (= coder[0])
        //   i=0    uses HANDLEMETHOD  -> writes final output (= coder[last])
        QMap<FPART_PROP, QVariant> mapProperties;
        mapProperties.insert(FPART_PROP_STREAMOFFSET, nStreamOffset);
        mapProperties.insert(FPART_PROP_STREAMSIZE, nStreamSize);

        if ((nFolderIndex < pState->listOutStreams.count()) && pState->listOutStreams.at(nFolderIndex).bCRCDefined) {
            mapProperties.insert(FPART_PROP_CRC_TYPE, (quint32)CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
            mapProperties.insert(FPART_PROP_RESULTCRC, pState->listOutStreams.at(nFolderIndex).nCRC);
        }

        if (nNumCoders >= 1) {
            qint32 nLastCoder = nNumCoders - 1;
            mapProperties.insert(FPART_PROP_HANDLEMETHOD, (quint32)coderToCompressMethod(folder.listCoders.at(nLastCoder).baCoder));
            mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, folder.listCoders.at(nLastCoder).baProperty);
            if ((nCoderSizesOffset + nLastCoder) < pState->listCodersSizes.count()) {
                mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)pState->listCodersSizes.at(nCoderSizesOffset + nLastCoder));
            }
        }

        if (nNumCoders >= 2) {
            mapProperties.insert(FPART_PROP_HANDLEMETHOD2, (quint32)coderToCompressMethod(folder.listCoders.at(0).baCoder));
            mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES2, folder.listCoders.at(0).baProperty);
            if (nCoderSizesOffset < pState->listCodersSizes.count()) {
                mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE2, (qint64)pState->listCodersSizes.at(nCoderSizesOffset));
            }
        }

        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties = mapProperties;
        state.mapUnpackProperties = mapUnpackProperties;
        state.pDeviceInput = guardedSource.data();
        state.pDeviceOutput = guardedOutput.data();
        state.nInputOffset = nStreamOffset;
        state.nInputLimit = nStreamSize;
        state.nProcessedOffset = 0;
        // Limit final output to the expected unpack size so each decoder is properly bounded
        if (nNumCoders >= 1) {
            qint32 nLastCoder = nNumCoders - 1;
            if ((nCoderSizesOffset + nLastCoder) < pState->listCodersSizes.count()) {
                state.nProcessedLimit = (qint64)pState->listCodersSizes.at(nCoderSizesOffset + nLastCoder);
            } else {
                state.nProcessedLimit = -1;
            }
        } else {
            state.nProcessedLimit = -1;
        }

        bResult = xDecompress.multiDecompress(&state, pPdStruct);
    }

    return bResult && guardedArchive && guardedSource && guardedOutput;
}

QList<QString> XSevenZip::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("'7z'BCAF271C");

    return listResult;
}

XBinary *XSevenZip::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XSevenZip(pDevice);
}

bool XSevenZip::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XSevenZip> guardedThis(this);
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

void *XSevenZip::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XSevenZip> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XSevenZip::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
