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
#include "xrpm.h"

#include <new>
#include "subdevice.h"
#include "xcpio.h"
#include "xgzip.h"

static XBinary::XCONVERT _TABLE_XRPM_STRUCTID[] = {{XRPM::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                   {XRPM::STRUCTID_LEAD, "LEAD", QString("LEAD")},
                                                   {XRPM::STRUCTID_HEADER, "HEADER", QString("HEADER")}};

static const qint64 N_RPM_LEAD_SIZE = 96;
static const qint64 N_RPM_HEADER_INTRO_SIZE = 16;
static const quint32 N_RPMTAG_PAYLOADCOMPRESSOR = 1125;

namespace {
XBinary::HANDLE_METHOD rpmCompressorNameToMethod(const QString &sName)
{
    const QString sValue = sName.trimmed().toLower();
    if (sValue == QLatin1String("gzip") || sValue == QLatin1String("gz")) return XBinary::HANDLE_METHOD_DEFLATE;
    if (sValue == QLatin1String("xz")) return XBinary::HANDLE_METHOD_XZ;
    if (sValue == QLatin1String("zstd") || sValue == QLatin1String("zst")) return XBinary::HANDLE_METHOD_ZSTD;
    if (sValue == QLatin1String("bzip2") || sValue == QLatin1String("bzip")) return XBinary::HANDLE_METHOD_BZIP2;
    if (sValue == QLatin1String("lzma")) return XBinary::HANDLE_METHOD_LZMA;
    if (sValue == QLatin1String("none") || sValue == QLatin1String("store")) return XBinary::HANDLE_METHOD_STORE;
    return XBinary::HANDLE_METHOD_UNKNOWN;
}

bool hasCpioMagic(XBinary *pBinary, qint64 nOffset, qint64 nSize)
{
    if (!pBinary || (nOffset < 0) || (nSize < 2)) return false;

    const QByteArray baMagic = pBinary->read_array_process(nOffset, qMin<qint64>(6, nSize), nullptr);
    if (baMagic.size() >= 6) {
        const QByteArray baAscii = baMagic.left(6);
        if ((baAscii == QByteArrayLiteral("070701")) || (baAscii == QByteArrayLiteral("070702")) ||
            (baAscii == QByteArrayLiteral("070707"))) {
            return true;
        }
    }
    return (baMagic.size() >= 2) &&
           ((((quint8)baMagic.at(0) == 0xC7) && ((quint8)baMagic.at(1) == 0x71)) ||
            (((quint8)baMagic.at(0) == 0x71) && ((quint8)baMagic.at(1) == 0xC7)));
}
}  // namespace

XRPM::XRPM(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XRPM::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    if ((getSize() < N_RPM_LEAD_SIZE) || (read_uint8(0) != 0xED) || (read_uint8(1) != 0xAB) ||
        (read_uint8(2) != 0xEE) || (read_uint8(3) != 0xDB)) {
        return false;
    }

    const quint8 nMajor = read_uint8(4);
    if ((nMajor != 3) && (nMajor != 4)) {
        return false;
    }

    const qint64 nPayloadOffset = getPayloadOffset(pPdStruct);
    if ((nPayloadOffset < 0) || (nPayloadOffset >= getSize()) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const HANDLE_METHOD method = getPayloadCompression(nPayloadOffset);
    if (method == HANDLE_METHOD_UNKNOWN) {
        return false;
    }
    if (method == HANDLE_METHOD_STORE) {
        if (!hasCpioMagic(this, nPayloadOffset, getSize() - nPayloadOffset)) {
            return false;
        }
        SubDevice payloadDevice(getDevice(), nPayloadOffset, getSize() - nPayloadOffset);
        if (!payloadDevice.open(QIODevice::ReadOnly)) {
            return false;
        }
        XCPIO cpio(&payloadDevice);
        const bool bValid = cpio.isValid(pPdStruct);
        payloadDevice.close();
        return bValid && XBinary::isPdStructNotCanceled(pPdStruct);
    }

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XRPM::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRPM xrpm(pDevice);

    return xrpm.isValid(pPdStruct);
}

qint64 XRPM::_readHeaderSize(qint64 nOffset)
{
    // Header intro: magic(3) 8E AD E8, version(1), reserved(4), nindex(4 BE), hsize(4 BE)
    const qint64 nFileSize = getSize();
    if ((nOffset < 0) || (nOffset > nFileSize) || (N_RPM_HEADER_INTRO_SIZE > (nFileSize - nOffset))) {
        return -1;
    }

    if ((read_uint8(nOffset) != 0x8E) || (read_uint8(nOffset + 1) != 0xAD) ||
        (read_uint8(nOffset + 2) != 0xE8) || (read_uint8(nOffset + 3) != 1) ||
        (read_uint32(nOffset + 4, true) != 0)) {
        return -1;
    }

    quint32 nIndexCount = read_uint32(nOffset + 8, true);
    quint32 nDataSize = read_uint32(nOffset + 12, true);

    // Sanity limits to avoid absurd values from corrupt files
    if ((nIndexCount > 0x100000) || (nDataSize > 0x10000000)) {
        return -1;
    }

    const qint64 nHeaderSize = N_RPM_HEADER_INTRO_SIZE + (qint64)nIndexCount * 16 + (qint64)nDataSize;
    if (nHeaderSize > (nFileSize - nOffset)) {
        return -1;
    }

    const qint64 nIndexStart = nOffset + N_RPM_HEADER_INTRO_SIZE;
    for (quint32 i = 0; i < nIndexCount; i++) {
        const qint64 nEntryOffset = nIndexStart + (qint64)i * 16;
        const quint32 nType = read_uint32(nEntryOffset + 4, true);
        const quint32 nDataOffset = read_uint32(nEntryOffset + 8, true);
        const quint32 nCount = read_uint32(nEntryOffset + 12, true);
        if ((nType > 9) || ((qint64)nDataOffset > nDataSize) || (nCount > 0x1000000)) {
            return -1;
        }

        qint64 nElementSize = 0;
        if ((nType == 1) || (nType == 2)) nElementSize = 1;
        else if (nType == 3) nElementSize = 2;
        else if (nType == 4) nElementSize = 4;
        else if (nType == 5) nElementSize = 8;
        else if (nType == 7) nElementSize = 1;

        if ((nElementSize > 0) && ((qint64)nCount > (((qint64)nDataSize - nDataOffset) / nElementSize))) {
            return -1;
        }
        if (((nType == 6) || (nType == 8) || (nType == 9)) && (nCount > 0) && ((qint64)nDataOffset >= nDataSize)) {
            return -1;
        }
    }

    return nHeaderSize;
}

qint64 XRPM::_getMainHeaderOffset()
{
    const qint64 nSigSize = _readHeaderSize(N_RPM_LEAD_SIZE);
    if (nSigSize < 0) {
        return -1;
    }

    qint64 nMainOffset = N_RPM_LEAD_SIZE + nSigSize;
    const qint64 nPadding = (8 - (nMainOffset & 7)) & 7;
    if ((nMainOffset > getSize()) || (nPadding > (getSize() - nMainOffset))) {
        return -1;
    }

    for (qint64 i = 0; i < nPadding; i++) {
        if (read_uint8(nMainOffset + i) != 0) {
            return -1;
        }
    }

    return nMainOffset + nPadding;
}

qint64 XRPM::getPayloadOffset(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return -1;
    }

    const qint64 nMainOffset = _getMainHeaderOffset();
    const qint64 nMainSize = _readHeaderSize(nMainOffset);
    if (nMainSize < 0) {
        return -1;
    }

    const qint64 nPayloadOffset = nMainOffset + nMainSize;
    return XBinary::isPdStructNotCanceled(pPdStruct) ? nPayloadOffset : -1;
}

QString XRPM::_readPayloadCompressorTag(qint64 nHeaderOffset)
{
    const qint64 nHeaderSize = _readHeaderSize(nHeaderOffset);
    if (nHeaderSize < 0) {
        return QString();
    }

    quint32 nIndexCount = read_uint32(nHeaderOffset + 8, true);
    quint32 nDataSize = read_uint32(nHeaderOffset + 12, true);

    if ((nIndexCount > 0x100000) || (nDataSize > 0x10000000)) {
        return QString();
    }

    qint64 nIndexStart = nHeaderOffset + N_RPM_HEADER_INTRO_SIZE;
    qint64 nDataStart = nIndexStart + (qint64)nIndexCount * 16;

    for (quint32 i = 0; i < nIndexCount; i++) {
        qint64 nEntry = nIndexStart + (qint64)i * 16;
        quint32 nTag = read_uint32(nEntry, true);

        if (nTag == N_RPMTAG_PAYLOADCOMPRESSOR) {
            quint32 nType = read_uint32(nEntry + 4, true);
            quint32 nDataOffset = read_uint32(nEntry + 8, true);
            quint32 nCount = read_uint32(nEntry + 12, true);

            // Type 6 = STRING
            if ((nType == 6) && (nCount == 1) && ((qint64)nDataOffset < nDataSize)) {
                const qint64 nAvailable = (qint64)nDataSize - nDataOffset;
                const QByteArray baValue = read_array_process(nDataStart + nDataOffset, qMin<qint64>(64, nAvailable), nullptr);
                const qint32 nTerminator = baValue.indexOf('\0');
                if (nTerminator >= 0) {
                    return QString::fromLatin1(baValue.constData(), nTerminator);
                }
            }
            return QString();
        }
    }

    return QString();
}

XBinary::HANDLE_METHOD XRPM::getPayloadCompression(qint64 nPayloadOffset)
{
    if ((nPayloadOffset < 0) || (nPayloadOffset > getSize()) || ((getSize() - nPayloadOffset) < 2)) {
        return HANDLE_METHOD_UNKNOWN;
    }

    quint8 b0 = read_uint8(nPayloadOffset);
    quint8 b1 = read_uint8(nPayloadOffset + 1);
    quint8 b2 = ((getSize() - nPayloadOffset) >= 3) ? read_uint8(nPayloadOffset + 2) : 0;
    quint8 b3 = ((getSize() - nPayloadOffset) >= 4) ? read_uint8(nPayloadOffset + 3) : 0;

    HANDLE_METHOD sniffedMethod = HANDLE_METHOD_STORE;

    if ((b0 == 0x1F) && (b1 == 0x8B)) {
        sniffedMethod = HANDLE_METHOD_DEFLATE;  // gzip; header stripped separately
    } else if ((b0 == 0xFD) && (b1 == 0x37) && (b2 == 0x7A) && (b3 == 0x58)) {
        sniffedMethod = HANDLE_METHOD_XZ;
    } else if ((b0 == 0x28) && (b1 == 0xB5) && (b2 == 0x2F) && (b3 == 0xFD)) {
        sniffedMethod = HANDLE_METHOD_ZSTD;
    } else if ((b0 == 0x42) && (b1 == 0x5A) && (b2 == 0x68)) {
        sniffedMethod = HANDLE_METHOD_BZIP2;
    } else if ((b0 == 0x5D) && (b1 == 0x00) && (b2 == 0x00)) {
        sniffedMethod = HANDLE_METHOD_LZMA;
    }

    const qint64 nMainOffset = _getMainHeaderOffset();
    const QString sCompressor = _readPayloadCompressorTag(nMainOffset);
    const HANDLE_METHOD taggedMethod = sCompressor.isEmpty() ? HANDLE_METHOD_UNKNOWN : rpmCompressorNameToMethod(sCompressor);
    if (!sCompressor.isEmpty() && (taggedMethod == HANDLE_METHOD_UNKNOWN)) {
        return HANDLE_METHOD_UNKNOWN;
    }
    if (taggedMethod != HANDLE_METHOD_UNKNOWN) {
        return (taggedMethod == sniffedMethod) ? taggedMethod : HANDLE_METHOD_UNKNOWN;
    }

    return sniffedMethod;
}

XBinary::FT XRPM::getFileType()
{
    return FT_RPM;
}

XBinary::MODE XRPM::getMode()
{
    return MODE_DATA;
}

QString XRPM::getMIMEString()
{
    return "application/x-rpm";
}

qint32 XRPM::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XRPM::getEndian()
{
    return ENDIAN_BIG;
}

QString XRPM::getArch()
{
    return QString();
}

QString XRPM::getFileFormatExt()
{
    return "rpm";
}

QString XRPM::getFileFormatExtsString()
{
    return "RPM (*.rpm)";
}

qint64 XRPM::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

XBinary::OSNAME XRPM::getOsName()
{
    return OSNAME_LINUX;
}

QString XRPM::getVersion()
{
    return QString("%1.%2").arg(read_uint8(4)).arg(read_uint8(5));
}

QList<XBinary::MAPMODE> XRPM::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XRPM::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};
    if (!XBinary::isPdStructNotCanceled(pPdStruct) || !isValid(pPdStruct)) {
        return result;
    }
    result.fileType = getFileType();
    result.mode = getMode();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());
    result.sArch = getArch();
    result.nBinarySize = getSize();

    qint32 nIndex = 0;

    _MEMORY_RECORD recLead = {};
    recLead.nAddress = XADDR_MAX;
    recLead.nOffset = 0;
    recLead.nSize = N_RPM_LEAD_SIZE;
    recLead.nIndex = nIndex++;
    recLead.filePart = FILEPART_HEADER;
    recLead.sName = QString("RPM ") + tr("Lead");
    result.listRecords.append(recLead);

    qint64 nSigOffset = N_RPM_LEAD_SIZE;
    qint64 nSigSize = _readHeaderSize(nSigOffset);

    if (nSigSize > 0) {
        _MEMORY_RECORD recSig = {};
        recSig.nAddress = XADDR_MAX;
        recSig.nOffset = nSigOffset;
        recSig.nSize = nSigSize;
        recSig.nIndex = nIndex++;
        recSig.filePart = FILEPART_HEADER;
        recSig.sName = tr("Signature");
        result.listRecords.append(recSig);

        qint64 nMainOffset = nSigOffset + nSigSize;
        if (nMainOffset % 8) {
            nMainOffset += 8 - (nMainOffset % 8);
        }

        qint64 nMainSize = _readHeaderSize(nMainOffset);

        if (nMainSize > 0) {
            _MEMORY_RECORD recMain = {};
            recMain.nAddress = XADDR_MAX;
            recMain.nOffset = nMainOffset;
            recMain.nSize = nMainSize;
            recMain.nIndex = nIndex++;
            recMain.filePart = FILEPART_HEADER;
            recMain.sName = tr("Header");
            result.listRecords.append(recMain);

            qint64 nPayloadOffset = nMainOffset + nMainSize;

            if (nPayloadOffset < getSize()) {
                _MEMORY_RECORD recPayload = {};
                recPayload.nAddress = XADDR_MAX;
                recPayload.nOffset = nPayloadOffset;
                recPayload.nSize = getSize() - nPayloadOffset;
                recPayload.nIndex = nIndex++;
                recPayload.filePart = FILEPART_REGION;
                recPayload.sName = tr("Payload");
                result.listRecords.append(recPayload);
            }
        }
    }

    _handleOverlay(&result);

    return result;
}

QString XRPM::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XRPM_STRUCTID, sizeof(_TABLE_XRPM_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XRPM::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XRPM_STRUCTID, sizeof(_TABLE_XRPM_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XRPM::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XRPM_STRUCTID, sizeof(_TABLE_XRPM_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XRPM::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_LEAD;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_LEAD) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_LEAD);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = sizeof(RPMLEAD);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_LEAD, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_LEAD), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XRPM::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_LEAD) {
        listResult.append({"magic", (qint32)offsetof(RPMLEAD, magic), 4, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"major", (qint32)offsetof(RPMLEAD, major), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"minor", (qint32)offsetof(RPMLEAD, minor), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"type", (qint32)offsetof(RPMLEAD, type), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"archnum", (qint32)offsetof(RPMLEAD, archnum), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"name", (qint32)offsetof(RPMLEAD, name), 66, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"osnum", (qint32)offsetof(RPMLEAD, osnum), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"signature_type", (qint32)offsetof(RPMLEAD, signature_type), 2, XFRECORD_FLAG_NONE, VT_UINT16});
    }

    return listResult;
}

static bool _rpmCanAppend(XBinary::PDSTRUCT *pPdStruct, qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return XBinary::isPdStructNotCanceled(pPdStruct) && ((nLimit == -1) || (listResult.size() < nLimit));
}

QList<XBinary::FPART> XRPM::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || !XBinary::isPdStructNotCanceled(pPdStruct) || !isValid(pPdStruct)) {
        return listResult;
    }

    qint64 nPayloadOffset = getPayloadOffset(pPdStruct);

    if ((nFileParts & FILEPART_HEADER) && _rpmCanAppend(pPdStruct, nLimit, listResult)) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = (nPayloadOffset > 0) ? nPayloadOffset : N_RPM_LEAD_SIZE;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && _rpmCanAppend(pPdStruct, nLimit, listResult) && (nPayloadOffset > 0) && (nPayloadOffset < getSize())) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = nPayloadOffset;
        record.nFileSize = getSize() - nPayloadOffset;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Payload");
        listResult.append(record);
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        listResult.clear();
    }
    return listResult;
}

QList<QString> XRPM::getSearchSignatures()
{
    return {"EDABEEDB"};
}

XBinary *XRPM::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XRPM(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XRPM::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XRPM::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (m_bUnpackOperationInProgress) {
        return false;
    }
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XRPM> guardedArchive(this);

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !guardedArchive->ownsUnpackSource(pState)) {
        return false;
    }
    RPM_UNPACK_CONTEXT *pOldContext =
        static_cast<RPM_UNPACK_CONTEXT *>(pState->pContext);
    guardedArchive->releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pOldContext;
    if (!guardedArchive) return false;
    *pState = UNPACK_STATE();
    if (!isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const bool bBound = guardedArchive->bindUnpackSource(pState, pPdStruct);
    if (!guardedArchive || !bBound) return false;

    const bool bValid = guardedArchive->isValid(pPdStruct);
    if (!guardedArchive) return false;
    if (!bValid) {
        guardedArchive->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    qint64 nPayloadOffset = guardedArchive->getPayloadOffset(pPdStruct);
    if (!guardedArchive) return false;
    const qint64 nFileSize = guardedArchive->getSize();
    if (!guardedArchive) return false;
    if ((nPayloadOffset <= 0) || (nPayloadOffset >= nFileSize)) {
        guardedArchive->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    RPM_UNPACK_CONTEXT *pContext = new (std::nothrow) RPM_UNPACK_CONTEXT;
    if (!pContext) {
        guardedArchive->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    pContext->nPayloadOffset = nPayloadOffset;
    pContext->nPayloadSize = nFileSize - nPayloadOffset;
    pContext->nUncompressedSize = -1;
    pContext->nCRC32 = 0;
    pContext->bHasCRC32 = false;
    pContext->compressMethod = guardedArchive->getPayloadCompression(nPayloadOffset);
    if (!guardedArchive) {
        delete pContext;
        return false;
    }
    if ((pContext->compressMethod == HANDLE_METHOD_UNKNOWN) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        guardedArchive->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    // For gzip, strip the gzip header so the DEFLATE stream starts cleanly.
    if (pContext->compressMethod == HANDLE_METHOD_DEFLATE) {
        QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
        if (!guardedArchive || !guardedSource) {
            delete pContext;
            return false;
        }
        SubDevice payloadDevice(guardedSource.data(), nPayloadOffset, pContext->nPayloadSize);
        XBinary::UNPACK_STATE gzipState = {};
        bool bGzipValid = payloadDevice.open(QIODevice::ReadOnly);
        XGzip gzip(&payloadDevice);
        if (bGzipValid) {
            bGzipValid = gzip.initUnpack(&gzipState, gzip.getDefaultUnpackProperties(), pPdStruct);
        }
        if (!guardedArchive || !guardedSource) {
            gzip.finishUnpack(&gzipState, nullptr);
            delete pContext;
            return false;
        }

        XBinary::ARCHIVERECORD gzipRecord = {};
        if (bGzipValid) {
            gzipRecord = gzip.infoCurrent(&gzipState, pPdStruct);
            if (!guardedArchive || !guardedSource) {
                gzip.finishUnpack(&gzipState, nullptr);
                delete pContext;
                return false;
            }
            const qint64 nStreamOffset = gzipRecord.nStreamOffset;
            const qint64 nStreamSize = gzipRecord.nStreamSize;
            bGzipValid = (nStreamOffset >= 10) && (nStreamSize > 0) &&
                         (nStreamOffset <= pContext->nPayloadSize) &&
                         (nStreamSize <= (pContext->nPayloadSize - nStreamOffset)) &&
                         ((nStreamOffset + nStreamSize) <= (pContext->nPayloadSize - 8));
            // An RPM gzip payload owns one complete member.  Reject trailing
            // bytes instead of silently treating a second member or junk as
            // unauthenticated package data.
            bGzipValid = bGzipValid && ((nStreamOffset + nStreamSize + 8) == pContext->nPayloadSize);
        }

        gzip.finishUnpack(&gzipState, nullptr);
        payloadDevice.close();

        if (!bGzipValid || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            guardedArchive->releaseUnpackSource(pState);
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }

        pContext->nPayloadOffset = nPayloadOffset + gzipRecord.nStreamOffset;
        pContext->nPayloadSize = gzipRecord.nStreamSize;
        pContext->nUncompressedSize = gzipRecord.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE, -1).toLongLong();
        if (gzipRecord.mapProperties.contains(FPART_PROP_RESULTCRC)) {
            pContext->nCRC32 = gzipRecord.mapProperties.value(FPART_PROP_RESULTCRC).toUInt();
            pContext->bHasCRC32 = true;
        }
    } else if (pContext->compressMethod == HANDLE_METHOD_STORE) {
        QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
        if (!guardedArchive || !guardedSource) {
            delete pContext;
            return false;
        }
        SubDevice payloadDevice(guardedSource.data(), nPayloadOffset, pContext->nPayloadSize);
        const bool bOpened = payloadDevice.open(QIODevice::ReadOnly);
        XCPIO cpio(&payloadDevice);
        const bool bCpioValid = bOpened && cpio.isValid(pPdStruct);
        payloadDevice.close();
        if (!guardedArchive || !guardedSource) {
            delete pContext;
            return false;
        }
        if (!bCpioValid || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            guardedArchive->releaseUnpackSource(pState);
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        pContext->nUncompressedSize = pContext->nPayloadSize;
    }

    QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
    if (!guardedArchive || !guardedSource) {
        delete pContext;
        return false;
    }
    QString sBaseName = XBinary::getDeviceFileBaseName(guardedSource.data());
    if (!guardedArchive || !guardedSource) {
        if (guardedArchive) guardedArchive->releaseUnpackSource(pState);
        delete pContext;
        return false;
    }
    if (sBaseName.isEmpty()) {
        sBaseName = "package";
    }
    pContext->sFileName = sBaseName + ".cpio";

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->nCurrentOffset = pContext->nPayloadOffset;
    pState->nTotalSize = nFileSize;
    pState->mapUnpackProperties = mapProperties;

    if (!guardedArchive->validateAndFinalizeUnpackSource(
            pState, pContext, pPdStruct)) {
        if (!guardedArchive) return false;
        pState->pContext = nullptr;
        guardedArchive->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XRPM::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<XRPM> guardedArchive(this);

    ARCHIVERECORD result = {};

    if (!pState || !pState->pContext || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }
    const qint64 nCurrentSize = guardedArchive->getSize();
    if (!guardedArchive || (pState->nTotalSize != nCurrentSize)) return result;

    RPM_UNPACK_CONTEXT *pContext = (RPM_UNPACK_CONTEXT *)pState->pContext;

    if ((pContext->nPayloadOffset < 0) || (pContext->nPayloadSize < 0) ||
        (pContext->nPayloadOffset > nCurrentSize) || (pContext->nPayloadSize > (nCurrentSize - pContext->nPayloadOffset)) ||
        (pContext->compressMethod == HANDLE_METHOD_UNKNOWN)) {
        return ARCHIVERECORD();
    }

    result.nStreamOffset = pContext->nPayloadOffset;
    result.nStreamSize = pContext->nPayloadSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nPayloadSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, pContext->compressMethod);
    if (pContext->nUncompressedSize >= 0) {
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    }
    if (pContext->bHasCRC32) {
        result.mapProperties.insert(FPART_PROP_RESULTCRC, pContext->nCRC32);
        result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    }

    return result;
}

bool XRPM::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XRPM> guardedArchive(this);

    if (!pState || !pState->pContext || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    const qint64 nCurrentSize = guardedArchive->getSize();
    if (!guardedArchive || (pState->nTotalSize != nCurrentSize)) return false;

    pState->nCurrentIndex++;

    return false;
}

bool XRPM::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XRPM> guardedArchive(this);

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !guardedArchive->ownsUnpackSource(pState)) return false;
    guardedArchive->releaseUnpackSource(pState);
    if (pState->pContext) {
        RPM_UNPACK_CONTEXT *pContext = (RPM_UNPACK_CONTEXT *)pState->pContext;
        pState->pContext = nullptr;
        delete pContext;
        if (!guardedArchive) return false;
    }

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    return true;
}

bool XRPM::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XRPM> guardedThis(this);
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

void *XRPM::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XRPM> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XRPM::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
