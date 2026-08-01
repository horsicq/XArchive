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
#include "xxar.h"
#include "Algos/xdeflatedecoder.h"

#include <QBuffer>
#include <QStack>
#include <QXmlStreamReader>

static XBinary::XCONVERT _TABLE_XXAR_STRUCTID[] = {{XXAR::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                   {XXAR::STRUCTID_HEADER, "HEADER", QString("HEADER")}};

XXAR::XXAR(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XXAR::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (getSize() >= (qint64)sizeof(XAR_HEADER)) {
        // 'xar!' = 78 61 72 21
        if ((read_uint8(0) == 0x78) && (read_uint8(1) == 0x61) && (read_uint8(2) == 0x72) && (read_uint8(3) == 0x21)) {
            quint16 nHeaderSize = read_uint16(4, true);
            if ((nHeaderSize >= (quint16)sizeof(XAR_HEADER)) && (nHeaderSize <= 1024)) {
                bResult = true;
            }
        }
    }

    return bResult;
}

bool XXAR::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XXAR xxar(pDevice);

    return xxar.isValid(pPdStruct);
}

qint64 XXAR::_getHeapOffset()
{
    quint16 nHeaderSize = read_uint16(4, true);
    quint64 nTocCompressed = read_uint64(8, true);

    return (qint64)nHeaderSize + (qint64)nTocCompressed;
}

QByteArray XXAR::_readTOC(PDSTRUCT *pPdStruct)
{
    QByteArray baResult;

    quint16 nHeaderSize = read_uint16(4, true);
    quint64 nTocCompressed = read_uint64(8, true);
    quint64 nTocUncompressed = read_uint64(16, true);

    if ((nTocCompressed == 0) || (nTocCompressed > 0x4000000) || (nTocUncompressed > 0x8000000)) {
        return baResult;
    }

    if ((qint64)nHeaderSize + (qint64)nTocCompressed > getSize()) {
        return baResult;
    }

    QBuffer bufferOut(&baResult);
    if (!bufferOut.open(QIODevice::WriteOnly)) {
        return baResult;
    }

    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = getDevice();
    state.pDeviceOutput = &bufferOut;
    state.nInputOffset = nHeaderSize;
    state.nInputLimit = (qint64)nTocCompressed;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;

    XDeflateDecoder::decompress_zlib(&state, pPdStruct);

    bufferOut.close();

    return baResult;
}

XBinary::HANDLE_METHOD XXAR::_encodingToMethod(const QString &sStyle)
{
    if (sStyle.contains("gzip") || sStyle.contains("zlib")) {
        return HANDLE_METHOD_ZLIB;
    } else if (sStyle.contains("bzip2")) {
        return HANDLE_METHOD_BZIP2;
    } else if (sStyle.contains("x-xz")) {
        return HANDLE_METHOD_XZ;
    } else if (sStyle.contains("lzma")) {
        return HANDLE_METHOD_LZMA;
    } else if (sStyle.contains("octet-stream") || sStyle.isEmpty()) {
        return HANDLE_METHOD_STORE;
    }

    return HANDLE_METHOD_STORE;
}

bool XXAR::_parseTOC(const QByteArray &baXML, qint64 nHeapOffset, QList<XAR_RECORD> *pListRecords)
{
    struct FRAME {
        QString sName;
        QString sType;
        qint64 nOffset;
        qint64 nLength;
        qint64 nSize;
        QString sEncoding;
        bool bHasData;
    };

    QStack<FRAME> stackFrames;
    QXmlStreamReader xml(baXML);

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement) {
            QString sElement = xml.name().toString();

            if (sElement == "file") {
                FRAME frame = {};
                frame.nOffset = -1;
                frame.nLength = 0;
                frame.nSize = 0;
                frame.bHasData = false;
                stackFrames.push(frame);
            } else if (sElement == "name") {
                if (!stackFrames.isEmpty()) {
                    stackFrames.top().sName = xml.readElementText();
                }
            } else if (sElement == "type") {
                if (!stackFrames.isEmpty()) {
                    stackFrames.top().sType = xml.readElementText();
                }
            } else if (sElement == "data") {
                if (!stackFrames.isEmpty()) {
                    stackFrames.top().bHasData = true;
                }
            } else if (sElement == "offset") {
                if (!stackFrames.isEmpty()) {
                    stackFrames.top().nOffset = xml.readElementText().toLongLong();
                }
            } else if (sElement == "length") {
                if (!stackFrames.isEmpty()) {
                    stackFrames.top().nLength = xml.readElementText().toLongLong();
                }
            } else if (sElement == "size") {
                if (!stackFrames.isEmpty()) {
                    stackFrames.top().nSize = xml.readElementText().toLongLong();
                }
            } else if (sElement == "encoding") {
                if (!stackFrames.isEmpty()) {
                    stackFrames.top().sEncoding = xml.attributes().value("style").toString();
                }
            }

        } else if (token == QXmlStreamReader::EndElement) {
            QString sElement = xml.name().toString();

            if (sElement == "file") {
                if (stackFrames.isEmpty()) {
                    continue;
                }

                // The full path is the chain of frame names currently on the stack
                // (parents first), including this frame at the top.
                QStringList listFullPath;
                for (qint32 i = 0; i < stackFrames.size(); i++) {
                    const QString &sFrameName = stackFrames.at(i).sName;
                    listFullPath.append(sFrameName.isEmpty() ? QString("unnamed") : sFrameName);
                }
                QString sFullPath = listFullPath.join(QLatin1Char('/'));

                FRAME frame = stackFrames.pop();

                XAR_RECORD record;
                record.sFileName = sFullPath;
                record.bIsFolder = (frame.sType == "directory");
                record.nSize = frame.nSize;
                record.nLength = frame.nLength;
                record.compressMethod = _encodingToMethod(frame.sEncoding);

                if (frame.bHasData && (frame.nOffset >= 0)) {
                    record.nOffset = nHeapOffset + frame.nOffset;
                } else {
                    record.nOffset = 0;
                    record.nLength = 0;
                    if (!record.bIsFolder) {
                        record.nSize = 0;  // metadata-only entry
                    }
                }

                pListRecords->append(record);
            }
        }
    }

    return !pListRecords->isEmpty();
}

XBinary::FT XXAR::getFileType()
{
    return FT_XAR;
}

XBinary::MODE XXAR::getMode()
{
    return MODE_DATA;
}

QString XXAR::getMIMEString()
{
    return "application/x-xar";
}

qint32 XXAR::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XXAR::getEndian()
{
    return ENDIAN_BIG;
}

QString XXAR::getArch()
{
    return QString();
}

QString XXAR::getFileFormatExt()
{
    return "xar";
}

QString XXAR::getFileFormatExtsString()
{
    return "XAR (*.xar *.pkg)";
}

qint64 XXAR::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

XBinary::OSNAME XXAR::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QString XXAR::getVersion()
{
    return QString::number(read_uint16(6, true));
}

QList<XBinary::MAPMODE> XXAR::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XXAR::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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
    quint16 nHeaderSize = read_uint16(4, true);
    quint64 nTocCompressed = read_uint64(8, true);

    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = -1;
    recHeader.nOffset = 0;
    recHeader.nSize = nHeaderSize;
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = QString("XAR ") + tr("Header");
    result.listRecords.append(recHeader);

    if ((nTocCompressed > 0) && ((qint64)nHeaderSize + (qint64)nTocCompressed <= getSize())) {
        _MEMORY_RECORD recToc = {};
        recToc.nAddress = -1;
        recToc.nOffset = nHeaderSize;
        recToc.nSize = (qint64)nTocCompressed;
        recToc.nIndex = nIndex++;
        recToc.filePart = FILEPART_HEADER;
        recToc.sName = tr("TOC");
        result.listRecords.append(recToc);

        qint64 nHeapOffset = (qint64)nHeaderSize + (qint64)nTocCompressed;
        if (nHeapOffset < getSize()) {
            _MEMORY_RECORD recHeap = {};
            recHeap.nAddress = -1;
            recHeap.nOffset = nHeapOffset;
            recHeap.nSize = getSize() - nHeapOffset;
            recHeap.nIndex = nIndex++;
            recHeap.filePart = FILEPART_REGION;
            recHeap.sName = tr("Heap");
            result.listRecords.append(recHeap);
        }
    }

    _handleOverlay(&result);

    return result;
}

QString XXAR::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XXAR_STRUCTID, sizeof(_TABLE_XXAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XXAR::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XXAR_STRUCTID, sizeof(_TABLE_XXAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XXAR::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XXAR_STRUCTID, sizeof(_TABLE_XXAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XXAR::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
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
        xfHeader.nSize = sizeof(XAR_HEADER);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XXAR::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_HEADER) {
        listResult.append({"magic", (qint32)offsetof(XAR_HEADER, magic), 4, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"header_size", (qint32)offsetof(XAR_HEADER, header_size), 2, XFRECORD_FLAG_BE, VT_UINT16});
        listResult.append({"version", (qint32)offsetof(XAR_HEADER, version), 2, XFRECORD_FLAG_BE, VT_UINT16});
        listResult.append({"toc_length_compressed", (qint32)offsetof(XAR_HEADER, toc_length_compressed), 8, XFRECORD_FLAG_BE, VT_UINT64});
        listResult.append({"toc_length_uncompressed", (qint32)offsetof(XAR_HEADER, toc_length_uncompressed), 8, XFRECORD_FLAG_BE, VT_UINT64});
        listResult.append({"cksum_alg", (qint32)offsetof(XAR_HEADER, cksum_alg), 4, XFRECORD_FLAG_BE, VT_UINT32});
    }

    return listResult;
}

QList<XBinary::FPART> XXAR::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;

    qint64 nHeapOffset = _getHeapOffset();

    if (nFileParts & FILEPART_HEADER) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = (nHeapOffset > 0) ? nHeapOffset : (qint64)sizeof(XAR_HEADER);
        record.nVirtualAddress = -1;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && (nHeapOffset > 0) && (nHeapOffset < getSize())) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = nHeapOffset;
        record.nFileSize = getSize() - nHeapOffset;
        record.nVirtualAddress = -1;
        record.sName = tr("Heap");
        listResult.append(record);
    }

    return listResult;
}

QList<QString> XXAR::getSearchSignatures()
{
    return {"'xar!'"};
}

XBinary *XXAR::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XXAR(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XXAR::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XXAR::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState || !isValid(pPdStruct)) {
        return false;
    }

    pState->mapUnpackProperties = mapProperties;

    QByteArray baTOC = _readTOC(pPdStruct);
    if (baTOC.isEmpty()) {
        return false;
    }

    XAR_UNPACK_CONTEXT *pContext = new XAR_UNPACK_CONTEXT;

    if (!_parseTOC(baTOC, _getHeapOffset(), &(pContext->listRecords))) {
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

XBinary::ARCHIVERECORD XXAR::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    ARCHIVERECORD result = {};

    if (!pState || !pState->pContext || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    XAR_UNPACK_CONTEXT *pContext = (XAR_UNPACK_CONTEXT *)pState->pContext;

    if (pState->nCurrentIndex >= pContext->listRecords.count()) {
        return result;
    }

    const XAR_RECORD &record = pContext->listRecords.at(pState->nCurrentIndex);

    result.nStreamOffset = record.nOffset;
    result.nStreamSize = record.nLength;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, record.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, record.nSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, record.nLength);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, record.compressMethod);

    if (record.bIsFolder) {
        result.mapProperties.insert(FPART_PROP_ISFOLDER, true);
    }

    return result;
}

bool XXAR::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState || !pState->pContext) {
        return false;
    }

    pState->nCurrentIndex++;

    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XXAR::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if (pState->pContext) {
        XAR_UNPACK_CONTEXT *pContext = (XAR_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
    }

    return true;
}

bool XXAR::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XArchive::handleInternalInfo(pPdStruct);
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) =
            *static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
    }

    return bResult;
}

void *XXAR::getInternalInfo(PDSTRUCT *pPdStruct)
{
    handleInternalInfo(pPdStruct);

    return &m_internalInfo;
}

void XXAR::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
