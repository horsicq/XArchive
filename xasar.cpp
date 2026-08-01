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
#include "xasar.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

static XBinary::XCONVERT _TABLE_XASAR_STRUCTID[] = {{XASAR::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                    {XASAR::STRUCTID_HEADER, "HEADER", QString("HEADER")}};

XASAR::XASAR(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XASAR::_readHeader(qint64 *pnJsonOffset, qint64 *pnJsonSize, qint64 *pnBlobOffset)
{
    if (getSize() < 16) {
        return false;
    }

    // Pickle: sizeOfHeaderPickle(4)=4, sizeOfHeader(4), jsonStrLenField(4), jsonByteLen(4)
    quint32 nField0 = read_uint32(0);
    quint32 nHeaderSize = read_uint32(4);
    quint32 nJsonStrSize = read_uint32(8);
    quint32 nJsonSize = read_uint32(12);

    // The first pickle payload is always 4 (it encodes a single uint32).
    if (nField0 != 4) {
        return false;
    }

    // Consistency checks between the three size fields.
    if ((nHeaderSize < nJsonStrSize) || (nJsonStrSize < nJsonSize) || (nJsonSize == 0)) {
        return false;
    }

    qint64 nJsonOffset = 16;

    if (nJsonOffset + (qint64)nJsonSize > getSize()) {
        return false;
    }

    // Blob starts after the pickle-aligned header. Header total = 8 + nHeaderSize,
    // rounded up to a 4-byte boundary.
    qint64 nBlobOffset = 8 + (qint64)nHeaderSize;
    if (nBlobOffset % 4) {
        nBlobOffset += 4 - (nBlobOffset % 4);
    }

    if (pnJsonOffset) *pnJsonOffset = nJsonOffset;
    if (pnJsonSize) *pnJsonSize = (qint64)nJsonSize;
    if (pnBlobOffset) *pnBlobOffset = nBlobOffset;

    return true;
}

bool XASAR::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    qint64 nJsonOffset = 0;
    qint64 nJsonSize = 0;
    qint64 nBlobOffset = 0;

    if (!_readHeader(&nJsonOffset, &nJsonSize, &nBlobOffset)) {
        return false;
    }

    // The JSON directory must parse and contain a "files" object.
    QByteArray baJson = read_array(nJsonOffset, nJsonSize);
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(baJson, &parseError);

    if ((parseError.error != QJsonParseError::NoError) || !doc.isObject()) {
        return false;
    }

    return doc.object().contains("files");
}

bool XASAR::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XASAR xasar(pDevice);

    return xasar.isValid(pPdStruct);
}

void XASAR::_walkTree(const QJsonObject &objFiles, const QString &sParent, qint64 nBlobOffset, QList<ASAR_RECORD> *pListRecords)
{
    const QStringList listKeys = objFiles.keys();

    for (const QString &sKey : listKeys) {
        QJsonObject objEntry = objFiles.value(sKey).toObject();
        QString sPath = sParent.isEmpty() ? sKey : (sParent + QLatin1Char('/') + sKey);

        if (objEntry.contains("files")) {
            ASAR_RECORD folderRecord;
            folderRecord.sFileName = sPath;
            folderRecord.nOffset = 0;
            folderRecord.nSize = 0;
            folderRecord.bIsFolder = true;
            pListRecords->append(folderRecord);

            _walkTree(objEntry.value("files").toObject(), sPath, nBlobOffset, pListRecords);
        } else if (objEntry.contains("offset")) {
            // offset is a string (may exceed 32-bit range)
            bool bOk = false;
            qint64 nRelOffset = objEntry.value("offset").toString().toLongLong(&bOk);
            qint64 nSize = (qint64)objEntry.value("size").toDouble();

            if (bOk) {
                ASAR_RECORD fileRecord;
                fileRecord.sFileName = sPath;
                fileRecord.nOffset = nBlobOffset + nRelOffset;
                fileRecord.nSize = nSize;
                fileRecord.bIsFolder = false;
                pListRecords->append(fileRecord);
            }
        }
    }
}

XBinary::FT XASAR::getFileType()
{
    return FT_ASAR;
}

XBinary::MODE XASAR::getMode()
{
    return MODE_DATA;
}

QString XASAR::getMIMEString()
{
    return "application/x-asar";
}

qint32 XASAR::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XASAR::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XASAR::getArch()
{
    return QString();
}

QString XASAR::getFileFormatExt()
{
    return "asar";
}

QString XASAR::getFileFormatExtsString()
{
    return "ASAR (*.asar)";
}

qint64 XASAR::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

XBinary::OSNAME XASAR::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QString XASAR::getVersion()
{
    return QString();
}

QList<XBinary::MAPMODE> XASAR::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XASAR::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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
    qint64 nJsonOffset = 0;
    qint64 nJsonSize = 0;
    qint64 nBlobOffset = 0;

    _readHeader(&nJsonOffset, &nJsonSize, &nBlobOffset);

    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = -1;
    recHeader.nOffset = 0;
    recHeader.nSize = (nBlobOffset > 0) ? nBlobOffset : 16;
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = QString("ASAR ") + tr("Header");
    result.listRecords.append(recHeader);

    if ((nBlobOffset > 0) && (nBlobOffset < getSize())) {
        _MEMORY_RECORD recData = {};
        recData.nAddress = -1;
        recData.nOffset = nBlobOffset;
        recData.nSize = getSize() - nBlobOffset;
        recData.nIndex = nIndex++;
        recData.filePart = FILEPART_REGION;
        recData.sName = tr("Files");
        result.listRecords.append(recData);
    }

    _handleOverlay(&result);

    return result;
}

QString XASAR::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XASAR_STRUCTID, sizeof(_TABLE_XASAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XASAR::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XASAR_STRUCTID, sizeof(_TABLE_XASAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XASAR::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XASAR_STRUCTID, sizeof(_TABLE_XASAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XASAR::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = 16;
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XASAR::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_HEADER) {
        listResult.append({"pickle_size", 0, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"header_size", 4, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"json_str_size", 8, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"json_size", 12, 4, XFRECORD_FLAG_SIZE, VT_UINT32});
    }

    return listResult;
}

QList<XBinary::FPART> XASAR::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;

    qint64 nJsonOffset = 0;
    qint64 nJsonSize = 0;
    qint64 nBlobOffset = 0;

    _readHeader(&nJsonOffset, &nJsonSize, &nBlobOffset);

    if (nFileParts & FILEPART_HEADER) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = (nBlobOffset > 0) ? nBlobOffset : 16;
        record.nVirtualAddress = -1;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && (nBlobOffset > 0) && (nBlobOffset < getSize())) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = nBlobOffset;
        record.nFileSize = getSize() - nBlobOffset;
        record.nVirtualAddress = -1;
        record.sName = tr("Files");
        listResult.append(record);
    }

    return listResult;
}

QList<QString> XASAR::getSearchSignatures()
{
    return {};
}

XBinary *XASAR::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XASAR(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XASAR::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XASAR::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState || !isValid(pPdStruct)) {
        return false;
    }

    pState->mapUnpackProperties = mapProperties;

    qint64 nJsonOffset = 0;
    qint64 nJsonSize = 0;
    qint64 nBlobOffset = 0;

    if (!_readHeader(&nJsonOffset, &nJsonSize, &nBlobOffset)) {
        return false;
    }

    QByteArray baJson = read_array(nJsonOffset, nJsonSize);
    QJsonDocument doc = QJsonDocument::fromJson(baJson);

    if (!doc.isObject() || !doc.object().contains("files")) {
        return false;
    }

    ASAR_UNPACK_CONTEXT *pContext = new ASAR_UNPACK_CONTEXT;
    _walkTree(doc.object().value("files").toObject(), QString(), nBlobOffset, &(pContext->listRecords));

    if (pContext->listRecords.isEmpty()) {
        delete pContext;
        return false;
    }

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listRecords.count();
    pState->nTotalSize = getSize();
    pState->nCurrentOffset = 0;

    return true;
}

XBinary::ARCHIVERECORD XASAR::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    ARCHIVERECORD result = {};

    if (!pState || !pState->pContext || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    ASAR_UNPACK_CONTEXT *pContext = (ASAR_UNPACK_CONTEXT *)pState->pContext;

    if (pState->nCurrentIndex >= pContext->listRecords.count()) {
        return result;
    }

    const ASAR_RECORD &record = pContext->listRecords.at(pState->nCurrentIndex);

    result.nStreamOffset = record.nOffset;
    result.nStreamSize = record.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, record.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, record.nSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, record.nSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);

    if (record.bIsFolder) {
        result.mapProperties.insert(FPART_PROP_ISFOLDER, true);
    }

    return result;
}

bool XASAR::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState || !pState->pContext) {
        return false;
    }

    pState->nCurrentIndex++;

    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XASAR::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if (pState->pContext) {
        ASAR_UNPACK_CONTEXT *pContext = (ASAR_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
    }

    return true;
}

bool XASAR::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XArchive::handleInternalInfo(pPdStruct);
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) =
            *static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
    }

    return bResult;
}

void *XASAR::getInternalInfo(PDSTRUCT *pPdStruct)
{
    handleInternalInfo(pPdStruct);

    return &m_internalInfo;
}

void XASAR::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
