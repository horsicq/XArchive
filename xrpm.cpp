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
#include "xgzip.h"

static XBinary::XCONVERT _TABLE_XRPM_STRUCTID[] = {{XRPM::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                   {XRPM::STRUCTID_LEAD, "LEAD", QString("LEAD")},
                                                   {XRPM::STRUCTID_HEADER, "HEADER", QString("HEADER")}};

static const qint64 N_RPM_LEAD_SIZE = 96;
static const qint64 N_RPM_HEADER_INTRO_SIZE = 16;
static const quint32 N_RPMTAG_PAYLOADCOMPRESSOR = 1125;

XRPM::XRPM(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XRPM::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (getSize() >= N_RPM_LEAD_SIZE) {
        // Lead magic ED AB EE DB, major version 3 or 4
        if ((read_uint8(0) == 0xED) && (read_uint8(1) == 0xAB) && (read_uint8(2) == 0xEE) && (read_uint8(3) == 0xDB)) {
            quint8 nMajor = read_uint8(4);
            if ((nMajor == 3) || (nMajor == 4)) {
                bResult = true;
            }
        }
    }

    return bResult;
}

bool XRPM::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRPM xrpm(pDevice);

    return xrpm.isValid(pPdStruct);
}

qint64 XRPM::_readHeaderSize(qint64 nOffset)
{
    // Header intro: magic(3) 8E AD E8, version(1), reserved(4), nindex(4 BE), hsize(4 BE)
    if ((nOffset < 0) || (getSize() < nOffset + N_RPM_HEADER_INTRO_SIZE)) {
        return -1;
    }

    if ((read_uint8(nOffset) != 0x8E) || (read_uint8(nOffset + 1) != 0xAD) || (read_uint8(nOffset + 2) != 0xE8)) {
        return -1;
    }

    quint32 nIndexCount = read_uint32(nOffset + 8, true);
    quint32 nDataSize = read_uint32(nOffset + 12, true);

    // Sanity limits to avoid absurd values from corrupt files
    if ((nIndexCount > 0x100000) || (nDataSize > 0x10000000)) {
        return -1;
    }

    return N_RPM_HEADER_INTRO_SIZE + (qint64)nIndexCount * 16 + (qint64)nDataSize;
}

qint64 XRPM::getPayloadOffset(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    // Signature header starts right after the 96-byte lead.
    qint64 nSigOffset = N_RPM_LEAD_SIZE;
    qint64 nSigSize = _readHeaderSize(nSigOffset);
    if (nSigSize < 0) {
        return -1;
    }

    // The signature header is padded to an 8-byte boundary.
    qint64 nMainOffset = nSigOffset + nSigSize;
    if (nMainOffset % 8) {
        nMainOffset += 8 - (nMainOffset % 8);
    }

    qint64 nMainSize = _readHeaderSize(nMainOffset);
    if (nMainSize < 0) {
        return -1;
    }

    return nMainOffset + nMainSize;
}

QString XRPM::_readPayloadCompressorTag(qint64 nHeaderOffset)
{
    if (nHeaderOffset < 0) {
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

            // Type 6 = STRING
            if ((nType == 6) && ((qint64)nDataOffset < nDataSize)) {
                return read_ansiString(nDataStart + nDataOffset, 32);
            }
        }
    }

    return QString();
}

XBinary::HANDLE_METHOD XRPM::getPayloadCompression(qint64 nPayloadOffset)
{
    if (nPayloadOffset < 0) {
        return HANDLE_METHOD_UNKNOWN;
    }

    quint8 b0 = read_uint8(nPayloadOffset);
    quint8 b1 = read_uint8(nPayloadOffset + 1);
    quint8 b2 = read_uint8(nPayloadOffset + 2);
    quint8 b3 = read_uint8(nPayloadOffset + 3);

    if ((b0 == 0x1F) && (b1 == 0x8B)) {
        return HANDLE_METHOD_DEFLATE;  // gzip; header stripped separately
    } else if ((b0 == 0xFD) && (b1 == 0x37) && (b2 == 0x7A) && (b3 == 0x58)) {
        return HANDLE_METHOD_XZ;
    } else if ((b0 == 0x28) && (b1 == 0xB5) && (b2 == 0x2F) && (b3 == 0xFD)) {
        return HANDLE_METHOD_ZSTD;
    } else if ((b0 == 0x42) && (b1 == 0x5A) && (b2 == 0x68)) {
        return HANDLE_METHOD_BZIP2;
    } else if ((b0 == 0x5D) && (b1 == 0x00) && (b2 == 0x00)) {
        return HANDLE_METHOD_LZMA;
    }

    return HANDLE_METHOD_STORE;
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
    Q_UNUSED(pPdStruct)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.mode = getMode();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());
    result.sArch = getArch();
    result.nBinarySize = getSize();

    qint32 nIndex = 0;

    _MEMORY_RECORD recLead = {};
    recLead.nAddress = -1;
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
        recSig.nAddress = -1;
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
            recMain.nAddress = -1;
            recMain.nOffset = nMainOffset;
            recMain.nSize = nMainSize;
            recMain.nIndex = nIndex++;
            recMain.filePart = FILEPART_HEADER;
            recMain.sName = tr("Header");
            result.listRecords.append(recMain);

            qint64 nPayloadOffset = nMainOffset + nMainSize;

            if (nPayloadOffset < getSize()) {
                _MEMORY_RECORD recPayload = {};
                recPayload.nAddress = -1;
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

QList<XBinary::FPART> XRPM::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    QList<FPART> listResult;

    qint64 nPayloadOffset = getPayloadOffset(pPdStruct);

    if (nFileParts & FILEPART_HEADER) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = (nPayloadOffset > 0) ? nPayloadOffset : N_RPM_LEAD_SIZE;
        record.nVirtualAddress = -1;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && (nPayloadOffset > 0) && (nPayloadOffset < getSize())) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = nPayloadOffset;
        record.nFileSize = getSize() - nPayloadOffset;
        record.nVirtualAddress = -1;
        record.sName = tr("Payload");
        listResult.append(record);
    }

    return listResult;
}

QList<QString> XRPM::getSearchSignatures()
{
    return {"'EDABEEDB'"};
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
    if (!pState || !isValid(pPdStruct)) {
        return false;
    }

    pState->mapUnpackProperties = mapProperties;

    qint64 nPayloadOffset = getPayloadOffset(pPdStruct);
    if ((nPayloadOffset <= 0) || (nPayloadOffset >= getSize())) {
        return false;
    }

    RPM_UNPACK_CONTEXT *pContext = new RPM_UNPACK_CONTEXT;
    pContext->nPayloadOffset = nPayloadOffset;
    pContext->nPayloadSize = getSize() - nPayloadOffset;
    pContext->compressMethod = getPayloadCompression(nPayloadOffset);

    // For gzip, strip the gzip header so the DEFLATE stream starts cleanly.
    if (pContext->compressMethod == HANDLE_METHOD_DEFLATE) {
        XGzip xgzip(getDevice());
        // XGzip reads from offset 0; wrap the payload via a temporary check.
        quint8 nFlags = read_uint8(nPayloadOffset + 3);
        qint64 nGzipHeader = 10;
        // FEXTRA(4), FNAME, FCOMMENT, FHCRC handling
        if (nFlags & 0x04) {  // FEXTRA
            quint16 nXlen = read_uint16(nPayloadOffset + nGzipHeader);
            nGzipHeader += 2 + nXlen;
        }
        if (nFlags & 0x08) {  // FNAME
            while ((nPayloadOffset + nGzipHeader < getSize()) && (read_uint8(nPayloadOffset + nGzipHeader) != 0)) nGzipHeader++;
            nGzipHeader++;
        }
        if (nFlags & 0x10) {  // FCOMMENT
            while ((nPayloadOffset + nGzipHeader < getSize()) && (read_uint8(nPayloadOffset + nGzipHeader) != 0)) nGzipHeader++;
            nGzipHeader++;
        }
        if (nFlags & 0x02) {  // FHCRC
            nGzipHeader += 2;
        }
        Q_UNUSED(xgzip)
        pContext->nPayloadOffset = nPayloadOffset + nGzipHeader;
        pContext->nPayloadSize = getSize() - pContext->nPayloadOffset;
    }

    QString sBaseName = XBinary::getDeviceFileBaseName(getDevice());
    if (sBaseName.isEmpty()) {
        sBaseName = "package";
    }
    pContext->sFileName = sBaseName + ".cpio";

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->nCurrentOffset = pContext->nPayloadOffset;
    pState->nTotalSize = getSize();

    return true;
}

XBinary::ARCHIVERECORD XRPM::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    ARCHIVERECORD result = {};

    if (!pState || !pState->pContext || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    RPM_UNPACK_CONTEXT *pContext = (RPM_UNPACK_CONTEXT *)pState->pContext;

    result.nStreamOffset = pContext->nPayloadOffset;
    result.nStreamSize = pContext->nPayloadSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nPayloadSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, pContext->compressMethod);

    return result;
}

bool XRPM::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState || !pState->pContext) {
        return false;
    }

    pState->nCurrentIndex++;

    return false;
}

bool XRPM::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if (pState->pContext) {
        RPM_UNPACK_CONTEXT *pContext = (RPM_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
    }

    return true;
}

bool XRPM::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XArchive::handleInternalInfo(pPdStruct);
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) =
            *static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
    }

    return bResult;
}

void *XRPM::getInternalInfo(PDSTRUCT *pPdStruct)
{
    handleInternalInfo(pPdStruct);

    return &m_internalInfo;
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
