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
#include <QCryptographicHash>
#include <QStack>
#include <QXmlStreamReader>
#include <limits>
#include <memory>
#include <new>

static XBinary::XCONVERT _TABLE_XXAR_STRUCTID[] = {{XXAR::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                   {XXAR::STRUCTID_HEADER, "HEADER", QString("HEADER")}};

XXAR::XXAR(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XXAR::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QPointer<XXAR> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const qint64 nFileSize = guardedSource->size();
    if (!guardedArchive || !guardedSource ||
        (nFileSize < (qint64)sizeof(XAR_HEADER))) {
        return false;
    }

    QByteArray baHeader = XBinary::read_array_process(
        guardedSource.data(), 0, sizeof(XAR_HEADER), pPdStruct);
    if (!guardedArchive || !guardedSource ||
        (baHeader.size() != (qint64)sizeof(XAR_HEADER)) ||
        (memcmp(baHeader.constData(), "xar!", 4) != 0)) {
        return false;
    }

    char *pHeader = baHeader.data();
    const quint16 nHeaderSize = XBinary::_read_uint16(pHeader + 4, true);
    const quint16 nVersion = XBinary::_read_uint16(pHeader + 6, true);
    const quint64 nTocCompressed = XBinary::_read_uint64(pHeader + 8, true);
    const quint64 nTocUncompressed = XBinary::_read_uint64(pHeader + 16, true);
    const quint32 nChecksumAlgorithm = XBinary::_read_uint32(pHeader + 24, true);

    if ((nHeaderSize < (quint16)sizeof(XAR_HEADER)) || (nHeaderSize > 1024) || (nVersion > 1) ||
        (nTocCompressed < 6) || (nTocCompressed > 0x4000000ULL) ||
        (nTocUncompressed == 0) || (nTocUncompressed > 0x8000000ULL) || (nChecksumAlgorithm > 4) ||
        ((quint64)nHeaderSize > (quint64)nFileSize) || (nTocCompressed > ((quint64)nFileSize - nHeaderSize))) {
        return false;
    }

    const quint32 nDigestSize = (nChecksumAlgorithm == 1) ? 20 : (nChecksumAlgorithm == 2) ? 16 :
                                (nChecksumAlgorithm == 3) ? 32 : (nChecksumAlgorithm == 4) ? 64 : 0;
    const quint64 nHeapOffset = (quint64)nHeaderSize + nTocCompressed;
    return (nDigestSize == 0) || ((quint64)nDigestSize <= ((quint64)nFileSize - nHeapOffset));
}

bool XXAR::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XXAR xxar(pDevice);

    return xxar.isValid(pPdStruct);
}

qint64 XXAR::_getHeapOffset()
{
    QPointer<XXAR> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return -1;
    const qint64 nSignedFileSize = guardedSource->size();
    if (!guardedArchive || !guardedSource ||
        (nSignedFileSize < (qint64)sizeof(XAR_HEADER))) return -1;

    QByteArray baHeader = XBinary::read_array(
        guardedSource.data(), 0, sizeof(XAR_HEADER));
    if (!guardedArchive || !guardedSource ||
        (baHeader.size() != (qint64)sizeof(XAR_HEADER))) return -1;
    const quint16 nHeaderSize = XBinary::_read_uint16(
        baHeader.data() + 4, true);
    const quint64 nTocCompressed = XBinary::_read_uint64(
        baHeader.data() + 8, true);
    const quint64 nFileSize = (quint64)nSignedFileSize;
    if ((quint64)nHeaderSize > nFileSize || nTocCompressed > (nFileSize - nHeaderSize) ||
        nTocCompressed > (quint64)(std::numeric_limits<qint64>::max)() - nHeaderSize) {
        return -1;
    }

    return (qint64)((quint64)nHeaderSize + nTocCompressed);
}

QByteArray XXAR::_readTOC(PDSTRUCT *pPdStruct)
{
    QByteArray baResult;
    QPointer<XXAR> guardedArchive(this);
    QPointer<QIODevice> guardedSource(getDevice());

    if (!guardedSource || !guardedArchive->isValid(pPdStruct) ||
        !guardedArchive || !guardedSource) {
        return baResult;
    }

    QByteArray baHeader = XBinary::read_array_process(
        guardedSource.data(), 0, sizeof(XAR_HEADER), pPdStruct);
    if (!guardedArchive || !guardedSource ||
        (baHeader.size() != (qint64)sizeof(XAR_HEADER))) {
        return QByteArray();
    }
    char *pHeader = baHeader.data();
    const quint16 nHeaderSize = XBinary::_read_uint16(pHeader + 4, true);
    const quint64 nTocCompressed = XBinary::_read_uint64(pHeader + 8, true);
    const quint64 nTocUncompressed = XBinary::_read_uint64(pHeader + 16, true);
    const quint32 nChecksumAlgorithm = XBinary::_read_uint32(pHeader + 24, true);

    if (nChecksumAlgorithm != 0) {
        QCryptographicHash::Algorithm algorithm = QCryptographicHash::Sha1;
        qint32 nDigestSize = 20;
        if (nChecksumAlgorithm == 2) {
            algorithm = QCryptographicHash::Md5;
            nDigestSize = 16;
        } else if (nChecksumAlgorithm == 3) {
            algorithm = QCryptographicHash::Sha256;
            nDigestSize = 32;
        } else if (nChecksumAlgorithm == 4) {
            algorithm = QCryptographicHash::Sha512;
            nDigestSize = 64;
        }

        QCryptographicHash hash(algorithm);
        const qint32 nBufferCapacity = 0x10000;
        std::unique_ptr<char[]> pBuffer(new (std::nothrow) char[nBufferCapacity]);
        if (!pBuffer) {
            return QByteArray();
        }
        qint64 nCurrentOffset = nHeaderSize;
        qint64 nRemaining = (qint64)nTocCompressed;
        while ((nRemaining > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            const qint32 nChunkSize = (qint32)qMin<qint64>(nBufferCapacity, nRemaining);
            if (XBinary::read_array_process(guardedSource.data(), nCurrentOffset,
                                            pBuffer.get(), nChunkSize,
                                            pPdStruct) != nChunkSize ||
                !guardedArchive || !guardedSource) {
                return QByteArray();
            }
            hash.addData(pBuffer.get(), nChunkSize);
            nCurrentOffset += nChunkSize;
            nRemaining -= nChunkSize;
        }

        const qint64 nHeapOffset = (qint64)nHeaderSize +
                                   (qint64)nTocCompressed;
        const QByteArray baExpectedDigest = XBinary::read_array_process(
            guardedSource.data(), nHeapOffset, nDigestSize, pPdStruct);
        if (!guardedArchive || !guardedSource ||
            !XBinary::isPdStructNotCanceled(pPdStruct) || (nRemaining != 0) ||
            (baExpectedDigest.size() != nDigestSize) || (hash.result() != baExpectedDigest)) {
            return QByteArray();
        }
    }

    QBuffer bufferOut(&baResult);
    // The strict zlib decoder authenticates the Adler32 by reading the
    // completed output device, so the temporary TOC buffer must be readable
    // as well as writable.
    if (!bufferOut.open(QIODevice::ReadWrite)) {
        return baResult;
    }

    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = guardedSource.data();
    state.pDeviceOutput = &bufferOut;
    state.nInputOffset = nHeaderSize;
    state.nInputLimit = (qint64)nTocCompressed;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = (qint64)nTocUncompressed;
    state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)nTocUncompressed);

    const bool bDecompressed = XDeflateDecoder::decompress_zlib(&state, pPdStruct);

    bufferOut.close();

    if (!guardedArchive || !guardedSource || !bDecompressed ||
        !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (state.nCountInput != (qint64)nTocCompressed) || (state.nCountOutput != (qint64)nTocUncompressed) ||
        (baResult.size() != (qint64)nTocUncompressed)) {
        baResult.clear();
    }

    return baResult;
}

XBinary::HANDLE_METHOD XXAR::_encodingToMethod(const QString &sStyle)
{
    const QString sValue = sStyle.trimmed().toLower();
    if ((sValue == QLatin1String("application/x-gzip")) || (sValue == QLatin1String("application/gzip")) ||
        (sValue == QLatin1String("application/zlib")) || (sValue == QLatin1String("application/x-zlib"))) {
        return HANDLE_METHOD_ZLIB;
    } else if ((sValue == QLatin1String("application/x-bzip2")) || (sValue == QLatin1String("application/bzip2"))) {
        return HANDLE_METHOD_BZIP2;
    } else if ((sValue == QLatin1String("application/x-xz")) || (sValue == QLatin1String("application/xz"))) {
        return HANDLE_METHOD_XZ;
    } else if ((sValue == QLatin1String("application/x-lzma")) || (sValue == QLatin1String("application/lzma"))) {
        return HANDLE_METHOD_LZMA;
    } else if ((sValue == QLatin1String("application/octet-stream")) || sValue.isEmpty()) {
        return HANDLE_METHOD_STORE;
    }

    return HANDLE_METHOD_UNKNOWN;
}

bool XXAR::_parseTOC(const QByteArray &baXML, qint64 nHeapOffset, QList<XAR_RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    struct FRAME {
        QString sName;
        QString sType;
        qint64 nOffset;
        qint64 nLength;
        qint64 nSize;
        QString sEncoding;
        bool bHasData;
        bool bInsideData;
        bool bOffsetValid;
        bool bLengthValid;
        bool bSizeValid;
        bool bEncodingSeen;
    };

    if (!pListRecords || baXML.isEmpty() || (nHeapOffset < 0) || (nHeapOffset > getSize()) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QList<XAR_RECORD> listRecords;
    QStack<FRAME> stackFrames;
    QStack<QString> stackElements;
    QXmlStreamReader xml(baXML);
    bool bRootSeen = false;
    bool bInToc = false;
    bool bTocSeen = false;
    bool bSemanticError = false;
    const qint32 nMaxDepth = 1024;
    const qint32 nMaxRecords = 0x40000;

    const auto readUnsignedValue = [](const QString &sValue, qint64 *pValue) -> bool {
        if (!pValue) return false;
        bool bOk = false;
        const quint64 nValue = sValue.trimmed().toULongLong(&bOk, 10);
        if (!bOk || (nValue > (quint64)(std::numeric_limits<qint64>::max)())) return false;
        *pValue = (qint64)nValue;
        return true;
    };

    while (!xml.atEnd() && !xml.hasError() && !bSemanticError && XBinary::isPdStructNotCanceled(pPdStruct)) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement) {
            QString sElement = xml.name().toString();
            const QString sParentElement = stackElements.isEmpty() ? QString() : stackElements.top();
            stackElements.push(sElement);

            if (!bRootSeen) {
                bRootSeen = true;
                if ((sElement != QLatin1String("xar")) || !sParentElement.isEmpty()) {
                    bSemanticError = true;
                    continue;
                }
            }

            if (sElement == QLatin1String("toc")) {
                if (bInToc || bTocSeen || (sParentElement != QLatin1String("xar"))) {
                    bSemanticError = true;
                } else {
                    bInToc = true;
                    bTocSeen = true;
                }
                continue;
            }

            if (!bInToc) {
                continue;
            }

            if (sElement == "file") {
                if (((sParentElement != QLatin1String("toc")) && (sParentElement != QLatin1String("file"))) ||
                    (stackFrames.size() >= nMaxDepth) || (listRecords.size() >= nMaxRecords)) {
                    bSemanticError = true;
                    continue;
                }
                FRAME frame = {};
                frame.nOffset = -1;
                frame.nLength = 0;
                frame.nSize = 0;
                frame.bHasData = false;
                frame.bInsideData = false;
                frame.bOffsetValid = false;
                frame.bLengthValid = false;
                frame.bSizeValid = false;
                frame.bEncodingSeen = false;
                stackFrames.push(frame);
            } else if (sElement == "name") {
                if (!stackFrames.isEmpty() && (sParentElement == QLatin1String("file"))) {
                    stackFrames.top().sName = xml.readElementText();
                    stackElements.pop();
                }
            } else if (sElement == "type") {
                if (!stackFrames.isEmpty() && (sParentElement == QLatin1String("file"))) {
                    stackFrames.top().sType = xml.readElementText();
                    stackElements.pop();
                }
            } else if (sElement == "data") {
                if (sParentElement != QLatin1String("file")) {
                    continue;
                }
                if (stackFrames.isEmpty() || stackFrames.top().bHasData) {
                    bSemanticError = true;
                } else {
                    stackFrames.top().bHasData = true;
                    stackFrames.top().bInsideData = true;
                }
            } else if (sElement == "offset") {
                if (!stackFrames.isEmpty() && stackFrames.top().bInsideData &&
                    (sParentElement == QLatin1String("data"))) {
                    if (stackFrames.top().bOffsetValid) {
                        bSemanticError = true;
                        continue;
                    }
                    stackFrames.top().bOffsetValid = readUnsignedValue(xml.readElementText(), &stackFrames.top().nOffset);
                    stackElements.pop();
                    bSemanticError = bSemanticError || !stackFrames.top().bOffsetValid;
                }
            } else if (sElement == "length") {
                if (!stackFrames.isEmpty() && stackFrames.top().bInsideData &&
                    (sParentElement == QLatin1String("data"))) {
                    if (stackFrames.top().bLengthValid) {
                        bSemanticError = true;
                        continue;
                    }
                    stackFrames.top().bLengthValid = readUnsignedValue(xml.readElementText(), &stackFrames.top().nLength);
                    stackElements.pop();
                    bSemanticError = bSemanticError || !stackFrames.top().bLengthValid;
                }
            } else if (sElement == "size") {
                if (!stackFrames.isEmpty() && stackFrames.top().bInsideData &&
                    (sParentElement == QLatin1String("data"))) {
                    if (stackFrames.top().bSizeValid) {
                        bSemanticError = true;
                        continue;
                    }
                    stackFrames.top().bSizeValid = readUnsignedValue(xml.readElementText(), &stackFrames.top().nSize);
                    stackElements.pop();
                    bSemanticError = bSemanticError || !stackFrames.top().bSizeValid;
                }
            } else if (sElement == "encoding") {
                if (!stackFrames.isEmpty() && stackFrames.top().bInsideData &&
                    (sParentElement == QLatin1String("data"))) {
                    if (stackFrames.top().bEncodingSeen) {
                        bSemanticError = true;
                        continue;
                    }
                    stackFrames.top().bEncodingSeen = true;
                    stackFrames.top().sEncoding = xml.attributes().value("style").toString();
                }
            }

        } else if (token == QXmlStreamReader::EndElement) {
            QString sElement = xml.name().toString();
            if (stackElements.isEmpty() || (stackElements.top() != sElement)) {
                bSemanticError = true;
                continue;
            }
            stackElements.pop();
            const QString sParentElement = stackElements.isEmpty() ? QString() : stackElements.top();

            if (sElement == QLatin1String("toc")) {
                if (!stackFrames.isEmpty()) {
                    bSemanticError = true;
                }
                bInToc = false;
                continue;
            }

            if (sElement == QLatin1String("data")) {
                if (sParentElement != QLatin1String("file")) {
                    continue;
                }
                if (stackFrames.isEmpty() || !stackFrames.top().bInsideData) {
                    bSemanticError = true;
                } else {
                    stackFrames.top().bInsideData = false;
                }
                continue;
            }

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
                if (sFullPath.size() > 0x10000) {
                    bSemanticError = true;
                    continue;
                }

                FRAME frame = stackFrames.pop();

                XAR_RECORD record;
                record.sFileName = sFullPath;
                record.bIsFolder = (frame.sType == "directory");
                record.nSize = frame.nSize;
                record.nLength = frame.nLength;
                record.compressMethod = _encodingToMethod(frame.sEncoding);

                if (frame.bHasData) {
                    if (!frame.bOffsetValid || !frame.bLengthValid || !frame.bSizeValid ||
                        (frame.nOffset > (getSize() - nHeapOffset))) {
                        bSemanticError = true;
                        continue;
                    }
                    record.nOffset = nHeapOffset + frame.nOffset;
                    if (frame.nLength > (getSize() - record.nOffset)) {
                        bSemanticError = true;
                        continue;
                    }
                    if ((record.compressMethod == HANDLE_METHOD_UNKNOWN) ||
                        ((record.compressMethod == HANDLE_METHOD_STORE) && (record.nLength != record.nSize))) {
                        bSemanticError = true;
                        continue;
                    }
                } else {
                    record.nOffset = 0;
                    record.nLength = 0;
                    if (!record.bIsFolder) {
                        record.nSize = 0;  // metadata-only entry
                    }
                }

                listRecords.append(record);
            }
        }
    }

    if (xml.hasError() || bSemanticError || !bRootSeen || !bTocSeen || bInToc || !stackFrames.isEmpty() ||
        !stackElements.isEmpty() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        pListRecords->clear();
        return false;
    }

    *pListRecords = listRecords;
    return true;
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
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || !XBinary::isPdStructNotCanceled(pPdStruct) || !isValid(pPdStruct)) {
        return listResult;
    }

    const auto canAppend = [&]() -> bool {
        return XBinary::isPdStructNotCanceled(pPdStruct) && ((nLimit == -1) || (listResult.size() < nLimit));
    };

    qint64 nHeapOffset = _getHeapOffset();
    if (nHeapOffset < 0) {
        return listResult;
    }

    if ((nFileParts & FILEPART_HEADER) && canAppend()) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = (nHeapOffset > 0) ? nHeapOffset : (qint64)sizeof(XAR_HEADER);
        record.nVirtualAddress = -1;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && canAppend() && (nHeapOffset > 0) && (nHeapOffset < getSize())) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = nHeapOffset;
        record.nFileSize = getSize() - nHeapOffset;
        record.nVirtualAddress = -1;
        record.sName = tr("Heap");
        listResult.append(record);
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        listResult.clear();
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
    QPointer<XXAR> guardedArchive(this);
    if (!pState || m_bUnpackOperationInProgress ||
        ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState))) {
        return false;
    }
    if (!guardedArchive->finishUnpack(pState, nullptr) || !guardedArchive) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const bool bBound = guardedArchive->bindUnpackSource(pState, pPdStruct);
    if (!guardedArchive || !bBound) return false;

    const bool bValid = guardedArchive->isValid(pPdStruct);
    if (!guardedArchive) return false;
    if (!bValid) {
        guardedArchive->releaseUnpackSource(pState);
        return false;
    }

    QByteArray baTOC = guardedArchive->_readTOC(pPdStruct);
    if (!guardedArchive) return false;
    if (baTOC.isEmpty()) {
        guardedArchive->releaseUnpackSource(pState);
        return false;
    }

    XAR_UNPACK_CONTEXT *pContext = new (std::nothrow) XAR_UNPACK_CONTEXT;
    if (!pContext) {
        guardedArchive->releaseUnpackSource(pState);
        return false;
    }

    const qint64 nHeapOffset = guardedArchive->_getHeapOffset();
    if (!guardedArchive) {
        delete pContext;
        return false;
    }
    const bool bParsed = guardedArchive->_parseTOC(
        baTOC, nHeapOffset, &(pContext->listRecords), pPdStruct);
    if (!guardedArchive) {
        delete pContext;
        return false;
    }
    if (!bParsed || !isPdStructNotCanceled(pPdStruct)) {
        guardedArchive->releaseUnpackSource(pState);
        delete pContext;
        return false;
    }

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listRecords.count();
    pState->nTotalSize = guardedArchive->getSize();
    if (!guardedArchive) {
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    pState->nCurrentOffset = 0;
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

XBinary::ARCHIVERECORD XXAR::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<XXAR> guardedArchive(this);

    ARCHIVERECORD result = {};

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }
    const qint64 nCurrentSize = guardedArchive->getSize();
    if (!guardedArchive || (pState->nTotalSize != nCurrentSize)) return result;

    XAR_UNPACK_CONTEXT *pContext = (XAR_UNPACK_CONTEXT *)pState->pContext;

    if (pState->nCurrentIndex >= pContext->listRecords.count()) {
        return result;
    }

    const XAR_RECORD &record = pContext->listRecords.at(pState->nCurrentIndex);

    if ((record.nOffset < 0) || (record.nLength < 0) || (record.nOffset > nCurrentSize) ||
        (record.nLength > (nCurrentSize - record.nOffset)) || (record.nSize < 0) ||
        (record.compressMethod == HANDLE_METHOD_UNKNOWN)) {
        return ARCHIVERECORD();
    }

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
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XXAR> guardedArchive(this);

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    const qint64 nCurrentSize = guardedArchive->getSize();
    if (!guardedArchive || (pState->nTotalSize != nCurrentSize)) return false;

    pState->nCurrentIndex++;

    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XXAR::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XXAR> guardedArchive(this);

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState)) return false;

    XAR_UNPACK_CONTEXT *pContext =
        static_cast<XAR_UNPACK_CONTEXT *>(pState->pContext);
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
