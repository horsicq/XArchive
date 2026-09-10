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
#include "xmscompresssz.h"

#include <new>

static XBinary::XCONVERT _TABLE_XMSCOMPRESSSZ_STRUCTID[] = {{XMSCompressSZ::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                            {XMSCompressSZ::STRUCTID_SZ_HEADER, "SZ_HEADER", QString("SZ_HEADER")}};

namespace {
// 53 5A 20 88 F0 27 33 D1 - fixed for every COMPRESS.EXE "SZ " file.
const char SZ_SIGNATURE[8] = {'S', 'Z', ' ', '\x88', '\xf0', '\x27', '\x33', '\xd1'};
const qint64 SZ_HEADER_SIZE = 12;
// Same ceiling the shared Okumura LZSS decoder enforces; a corrupt size field
// must not turn into an attacker-chosen allocation.
const qint64 SZ_MAX_UNPACKED_SIZE = 0x10000000;  // 256 MiB
}  // namespace

XMSCompressSZ::XMSCompressSZ(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XMSCompressSZ::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    QPointer<XMSCompressSZ> guardedThis(this);

    if (!pContext) return false;

    *pContext = CONTEXT();
    pContext->nHeaderSize = SZ_HEADER_SIZE;

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedThis || !guardedSource) return false;

    const qint64 nTotalSize = guardedThis->getSize();
    if (!guardedThis) return false;

    if (nTotalSize < SZ_HEADER_SIZE) return false;

    const QByteArray baSignature = guardedThis->read_array(0, 8);
    if (!guardedThis) return false;
    if (baSignature.size() != 8) return false;
    if (memcmp(baSignature.constData(), SZ_SIGNATURE, 8) != 0) return false;

    const quint32 nUncompressedSize = guardedThis->read_uint32(8);
    if (!guardedThis) return false;

    if (static_cast<qint64>(nUncompressedSize) > SZ_MAX_UNPACKED_SIZE) return false;

    pContext->nCompressedOffset = SZ_HEADER_SIZE;
    pContext->nCompressedSize = nTotalSize - SZ_HEADER_SIZE;
    pContext->nUncompressedSize = static_cast<qint64>(nUncompressedSize);

    // A non-empty member needs at least a flag byte plus one token byte.
    if ((pContext->nUncompressedSize > 0) && (pContext->nCompressedSize < 2)) return false;

    // The container carries no name.  COMPRESS.EXE overwrites the LAST
    // character of the extension on disk ("SETARGV.OBJ" -> "SETARGV.OB$"),
    // so the original name cannot be recovered from the stream: keep the
    // device's complete file name, extension included.
    const QString sDeviceFileName = XBinary::getDeviceFileName(guardedSource.data());
    if (!guardedThis || !guardedSource) return false;
    if (!sDeviceFileName.isEmpty()) {
        pContext->sFileName = QFileInfo(sDeviceFileName).fileName();
    }
    if (pContext->sFileName.isEmpty()) pContext->sFileName = QStringLiteral("sz_data");

    return true;
}

bool XMSCompressSZ::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XMSCompressSZ> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context = {};
    const bool bResult = guardedThis->parseContext(&context, pPdStruct);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    return guardedThis && bResult;
}

bool XMSCompressSZ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMSCompressSZ xmscompresssz(pDevice);

    return xmscompresssz.isValid(pPdStruct);
}

XMSCompressSZ::SZ_HEADER XMSCompressSZ::_read_SZ_HEADER(qint64 nOffset)
{
    SZ_HEADER header = {};
    read_array(nOffset, reinterpret_cast<char *>(&header), sizeof(SZ_HEADER));
    return header;
}

XBinary::FT XMSCompressSZ::getFileType()
{
    return XBinary::FT_MSCOMPRESS_SZ;
}

XBinary::MODE XMSCompressSZ::getMode()
{
    return XBinary::MODE_DATA;
}

QString XMSCompressSZ::getMIMEString()
{
    return "application/x-ms-compress";
}

qint32 XMSCompressSZ::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

XBinary::ENDIAN XMSCompressSZ::getEndian()
{
    return XBinary::ENDIAN_LITTLE;
}

QString XMSCompressSZ::getArch()
{
    return QString();
}

QString XMSCompressSZ::getFileFormatExt()
{
    return "SZ";
}

QString XMSCompressSZ::getFileFormatExtsString()
{
    return "MS Compress SZ (*.$ *._)";
}

qint64 XMSCompressSZ::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

bool XMSCompressSZ::isSigned()
{
    return false;
}

XBinary::OSNAME XMSCompressSZ::getOsName()
{
    return XBinary::OSNAME_MSDOS;
}

QString XMSCompressSZ::getOsVersion()
{
    return QString();
}

QString XMSCompressSZ::getVersion()
{
    return QString();
}

bool XMSCompressSZ::isEncrypted()
{
    return false;
}

QList<XBinary::MAPMODE> XMSCompressSZ::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XMSCompressSZ::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.mode = getMode();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());
    result.sArch = getArch();
    result.nBinarySize = getSize();

    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context = {};
    const bool bParsed = parseContext(&context, pPdStruct);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    if (!bParsed) {
        return result;
    }

    qint32 nIndex = 0;

    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = XADDR_MAX;
    recHeader.nOffset = 0;
    recHeader.nSize = context.nHeaderSize;
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = QString("SZ ") + tr("Header");
    result.listRecords.append(recHeader);

    if (context.nCompressedSize > 0) {
        _MEMORY_RECORD recData = {};
        recData.nAddress = XADDR_MAX;
        recData.nOffset = context.nCompressedOffset;
        recData.nSize = context.nCompressedSize;
        recData.nIndex = nIndex++;
        recData.filePart = FILEPART_REGION;
        recData.sName = tr("Compressed Data");
        result.listRecords.append(recData);
    }

    _handleOverlay(&result);

    return result;
}

QString XMSCompressSZ::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XMSCOMPRESSSZ_STRUCTID, sizeof(_TABLE_XMSCOMPRESSSZ_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XMSCompressSZ::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XMSCOMPRESSSZ_STRUCTID, sizeof(_TABLE_XMSCOMPRESSSZ_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XMSCompressSZ::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XMSCOMPRESSSZ_STRUCTID, sizeof(_TABLE_XMSCOMPRESSSZ_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XMSCompressSZ::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    const quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_SZ_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_SZ_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_SZ_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = sizeof(SZ_HEADER);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_SZ_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_SZ_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XMSCompressSZ::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_SZ_HEADER) {
        listResult.append({"signature", (qint32)offsetof(SZ_HEADER, signature), 8, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"uncompressed_size", (qint32)offsetof(SZ_HEADER, uncompressed_size), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
    }

    return listResult;
}

QList<XBinary::FPART> XMSCompressSZ::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || (nFileParts == 0)) {
        return listResult;
    }

    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context = {};
    const bool bParsed = parseContext(&context, pPdStruct);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    if (!bParsed) {
        return listResult;
    }

    if ((nFileParts & FILEPART_HEADER) && ((nLimit == -1) || (listResult.size() < nLimit))) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = context.nHeaderSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_STREAM) && (context.nCompressedSize > 0) && ((nLimit == -1) || (listResult.size() < nLimit))) {
        FPART record = {};
        record.filePart = FILEPART_STREAM;
        record.nFileOffset = context.nCompressedOffset;
        record.nFileSize = context.nCompressedSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = context.sFileName;
        record.mapProperties.insert(FPART_PROP_ORIGINALNAME, context.sFileName);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nCompressedSize);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        // Not HANDLE_METHOD_AMPK_LZSS: the codec is the same Okumura LZSS, but
        // SZ is the only container in which running out of input is NORMAL
        // termination rather than corruption (two corpus files are physically
        // truncated and the reference emits their partial output).  The
        // distinct method is what threads XAMPKDecoder::decodeLZSS's
        // bAllowTruncated flag from here without loosening AMPK.
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SZ_LZSS);
        record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QString("LZSS"));
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && (context.nCompressedSize > 0) && ((nLimit == -1) || (listResult.size() < nLimit))) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = context.nCompressedOffset;
        record.nFileSize = context.nCompressedSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Compressed Data");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_DATA) && ((nLimit == -1) || (listResult.size() < nLimit))) {
        FPART record = {};
        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = context.nHeaderSize + context.nCompressedSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Data");
        listResult.append(record);
    }

    return listResult;
}

bool XMSCompressSZ::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XMSCompressSZ> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }

    const bool bParsed = guardedThis->parseContext(pContext, pPdStruct);
    if (!bParsed || !guardedThis || !guardedSource) {
        if (guardedThis) guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Microsoft COMPRESS \"SZ \" container; Okumura LZSS (4 KiB window, F = 18)"));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = guardedThis->getSize();
    if (!guardedThis) {
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedThis || !guardedSource || !bFinalized) {
        if (!guardedThis) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XMSCompressSZ::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return ARCHIVERECORD();

    QPointer<XMSCompressSZ> guardedThis(this);

    if (!pState || !isPdStructNotCanceled(pPdStruct) || !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nCompressedOffset;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    // See getFileParts(): SZ needs the truncation-tolerant variant of the
    // shared Okumura LZSS decoder, AMPK must not have it.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SZ_LZSS);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QString("LZSS"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XMSCompressSZ::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    QPointer<XMSCompressSZ> guardedThis(this);

    if (!pState || !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) {
        return false;
    }

    ++pState->nCurrentIndex;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        return true;
    }

    pState->nCurrentOffset = pContext->nCompressedOffset + pContext->nCompressedSize;

    return false;
}

bool XMSCompressSZ::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}

QList<QString> XMSCompressSZ::getSearchSignatures()
{
    return {"'SZ '88F02733D1"};
}

XBinary *XMSCompressSZ::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XMSCompressSZ(pDevice);
}

bool XMSCompressSZ::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XMSCompressSZ> guardedThis(this);
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

void *XMSCompressSZ::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XMSCompressSZ> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XMSCompressSZ::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
