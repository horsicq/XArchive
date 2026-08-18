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
#include "xwarc.h"

#include <limits>
#include <new>
#include <QDir>

namespace {
const qint32 WARC_MAX_HEADER_SIZE = 1024 * 1024;
const qint32 WARC_MAX_RECORDS = 100000;
const qint32 WARC_MAX_NAME_SIZE = 32768;

QByteArray warcTrimLeadingWhitespace(const QByteArray &value)
{
    qint32 nOffset = 0;
    while ((nOffset < value.size()) &&
           ((value.at(nOffset) == ' ') || (value.at(nOffset) == '\t'))) {
        nOffset++;
    }
    return value.mid(nOffset);
}

bool warcIsSafeRelativePath(const QString &path)
{
    QString normalized = QDir::fromNativeSeparators(path);
    if (normalized.isEmpty() || normalized.startsWith(QLatin1Char('/'))) {
        return false;
    }

    const QStringList parts =
        normalized.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &part : parts) {
        if (part.isEmpty() || (part == QLatin1String(".")) ||
            (part == QLatin1String(".."))) {
            return false;
        }
    }

    normalized = normalized.normalized(QString::NormalizationForm_C);
    return XBinary::fixFileName(normalized) == normalized;
}
}  // namespace

XWARC::XWARC(QIODevice *pDevice) : XArchive(pDevice)
{
}

XWARC::~XWARC()
{
}

bool XWARC::isValid(PDSTRUCT *pPdStruct)
{
    return _scanArchive(nullptr, nullptr, pPdStruct);
}

bool XWARC::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XWARC warc(pDevice);
    return warc.isValid(pPdStruct);
}

QString XWARC::getFileFormatExt()
{
    return "warc";
}

QString XWARC::getFileFormatExtsString()
{
    return "Web ARChive (*.warc)";
}

QString XWARC::getMIMEString()
{
    return "application/warc";
}

XBinary::FT XWARC::getFileType()
{
    return FT_WARC;
}

XBinary::ENDIAN XWARC::getEndian()
{
    return ENDIAN_UNKNOWN;
}

QList<QString> XWARC::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append("'WARC/'");
    return listResult;
}

XBinary *XWARC::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XWARC(pDevice);
}

bool XWARC::_parseVersion(const QByteArray &line)
{
    if (!line.startsWith("WARC/")) return false;

    const QByteArray version = line.mid(5);
    if ((version.size() != 3) && (version.size() != 4)) return false;
    if ((version.at(0) < '0') || (version.at(0) > '9') ||
        (version.at(1) != '.') ||
        (version.at(2) < '0') || (version.at(2) > '9') ||
        ((version.size() == 4) &&
         ((version.at(3) < '0') || (version.at(3) > '9')))) {
        return false;
    }

    const quint32 nMajor = (quint32)(version.at(0) - '0');
    quint32 nMinor = (quint32)(version.at(2) - '0');
    if (version.size() == 4) {
        nMinor = nMinor * 10 + (quint32)(version.at(3) - '0');
    }
    const quint32 nVersion = nMajor * 10000 + nMinor * 100;

    return (nVersion >= 1200) && (nVersion <= 10000);
}

bool XWARC::_parseUnsignedDecimal(const QByteArray &value, qint64 *pResult)
{
    if (!pResult || value.isEmpty()) return false;

    qint64 nResult = 0;
    const qint64 nMaximum = (std::numeric_limits<qint64>::max)();
    for (char c : value) {
        if ((c < '0') || (c > '9')) return false;
        const qint32 nDigit = c - '0';
        if (nResult > ((nMaximum - nDigit) / 10)) return false;
        nResult = nResult * 10 + nDigit;
    }

    *pResult = nResult;
    return true;
}

bool XWARC::_parseDate(const QByteArray &value, QDateTime *pResult)
{
    if (!pResult) return false;

    const QByteArray date = warcTrimLeadingWhitespace(value);
    if ((date.size() != 20) || (date.at(4) != '-') ||
        (date.at(7) != '-') || (date.at(10) != 'T') ||
        (date.at(13) != ':') || (date.at(16) != ':') ||
        (date.at(19) != 'Z')) {
        return false;
    }

    const qint32 positions[] = {0, 1, 2, 3, 5, 6, 8, 9,
                                11, 12, 14, 15, 17, 18};
    for (qint32 nPosition : positions) {
        if ((date.at(nPosition) < '0') || (date.at(nPosition) > '9')) {
            return false;
        }
    }

    const qint32 nYear = date.mid(0, 4).toInt();
    const qint32 nMonth = date.mid(5, 2).toInt();
    const qint32 nDay = date.mid(8, 2).toInt();
    const qint32 nHour = date.mid(11, 2).toInt();
    const qint32 nMinute = date.mid(14, 2).toInt();
    const qint32 nSecond = date.mid(17, 2).toInt();

    if ((nYear < 1583) || (nYear > 4095) ||
        (nHour < 0) || (nHour > 23) ||
        (nMinute < 0) || (nMinute > 59) ||
        (nSecond < 0) || (nSecond > 60)) {
        return false;
    }

    const QDate qDate(nYear, nMonth, nDay);
    const QTime qTime(nHour, nMinute, qMin(nSecond, 59));
    if (!qDate.isValid() || !qTime.isValid()) return false;

    QDateTime result(qDate, qTime, Qt::UTC);
    if (nSecond == 60) result = result.addSecs(1);
    if (!result.isValid()) return false;

    *pResult = result;
    return true;
}

bool XWARC::_mapTargetURI(const QByteArray &value, QString *pResult)
{
    if (!pResult) return false;

    const QByteArray uri = warcTrimLeadingWhitespace(value);
    if (uri.isEmpty()) return false;

    for (char c : uri) {
        const quint8 nCharacter = (quint8)c;
        if ((nCharacter == ' ') || (nCharacter == '\t') ||
            (nCharacter == '\r') || (nCharacter == '\n') ||
            (nCharacter == '\v') || (nCharacter == '\f')) {
            return false;
        }
    }

    const qint32 nSeparator = uri.indexOf("://");
    if (nSeparator < 3) return false;

    const QByteArray scheme = uri.left(nSeparator);
    const QByteArray remainder = uri.mid(nSeparator + 3);
    QByteArray path;

    if (scheme == "file") {
        path = remainder;
    } else if ((scheme == "http") || (scheme == "https") ||
               (scheme == "ftp")) {
        const qint32 nSlash = remainder.indexOf('/');
        if (nSlash < 0) return false;
        path = remainder.mid(nSlash + 1);
    } else {
        return false;
    }

    if (path.isEmpty() || path.endsWith('/') ||
        (path.size() > WARC_MAX_NAME_SIZE) || path.contains('\0')) {
        return false;
    }

    const QString result = QString::fromUtf8(path.constData(), path.size());
    if ((result.toUtf8() != path) || !warcIsSafeRelativePath(result)) {
        return false;
    }

    *pResult = result;
    return true;
}

bool XWARC::_readHeader(qint64 nOffset, QByteArray *pHeader,
                        qint64 *pDataOffset, PDSTRUCT *pPdStruct)
{
    QPointer<XWARC> guardedThis(this);
    if (!pHeader || !pDataOffset || (nOffset < 0) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    pHeader->clear();
    *pDataOffset = 0;

    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nOffset >= nTotalSize)) return false;

    qint64 nCurrentOffset = nOffset;
    qint32 nSearchOffset = 0;
    while ((pHeader->size() < WARC_MAX_HEADER_SIZE) &&
           (nCurrentOffset < nTotalSize) &&
           XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nChunkSize = (qint32)qMin<qint64>(
            qMin<qint64>(0x4000, nTotalSize - nCurrentOffset),
            WARC_MAX_HEADER_SIZE - pHeader->size());
        if (nChunkSize <= 0) return false;

        const QByteArray chunk = read_array_process(
            nCurrentOffset, nChunkSize, pPdStruct);
        if (!guardedThis || (chunk.size() != nChunkSize)) return false;

        const qint32 nOldSize = pHeader->size();
        pHeader->append(chunk);
        const qint32 nMarker = pHeader->indexOf("\r\n\r\n", nSearchOffset);
        if (nMarker >= 0) {
            const qint32 nHeaderSize = nMarker + 4;
            pHeader->truncate(nHeaderSize);
            *pDataOffset = nOffset + nHeaderSize;
            return XBinary::isPdStructNotCanceled(pPdStruct);
        }

        nSearchOffset = qMax(0, nOldSize - 3);
        nCurrentOffset += nChunkSize;
    }

    return false;
}

bool XWARC::_parseRecord(qint64 nOffset, WARC_ENTRY *pEntry,
                         bool *pVisible, PDSTRUCT *pPdStruct)
{
    QPointer<XWARC> guardedThis(this);
    if (!pEntry || !pVisible || (nOffset < 0) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    *pEntry = {};
    *pVisible = false;

    QByteArray header;
    qint64 nDataOffset = 0;
    const bool bRead = _readHeader(nOffset, &header, &nDataOffset, pPdStruct);
    if (!guardedThis || !bRead || (header.size() < 12)) return false;

    QMap<QByteArray, QByteArray> criticalFields;
    qint32 nLineOffset = 0;
    qint32 nLineNumber = 0;
    bool bSawTerminator = false;

    while (nLineOffset < header.size()) {
        const qint32 nLineEnd = header.indexOf("\r\n", nLineOffset);
        if (nLineEnd < 0) return false;

        const QByteArray line = header.mid(nLineOffset, nLineEnd - nLineOffset);
        nLineOffset = nLineEnd + 2;

        if (line.isEmpty()) {
            bSawTerminator = (nLineOffset == header.size());
            break;
        }

        for (char c : line) {
            const quint8 nCharacter = (quint8)c;
            if (((nCharacter < 0x20) && (nCharacter != '\t')) ||
                (nCharacter == 0x7f)) {
                return false;
            }
        }

        if (nLineNumber == 0) {
            if (!_parseVersion(line)) return false;
        } else {
            const qint32 nColon = line.indexOf(':');
            if (nColon <= 0) return false;

            const QByteArray key = line.left(nColon);
            for (char c : key) {
                const quint8 nCharacter = (quint8)c;
                if ((nCharacter <= 0x20) || (nCharacter >= 0x7f) ||
                    (nCharacter == ':')) {
                    return false;
                }
            }

            const bool bCritical =
                (key == "WARC-Type") || (key == "WARC-Target-URI") ||
                (key == "Content-Length") || (key == "WARC-Date") ||
                (key == "Last-Modified");
            if (bCritical) {
                if (criticalFields.contains(key)) return false;
                criticalFields.insert(key, line.mid(nColon + 1));
            }
        }

        nLineNumber++;
    }

    if (!bSawTerminator || (nLineNumber == 0) ||
        !criticalFields.contains("Content-Length") ||
        !criticalFields.contains("WARC-Date")) {
        return false;
    }

    qint64 nContentLength = -1;
    if (!_parseUnsignedDecimal(
            warcTrimLeadingWhitespace(criticalFields.value("Content-Length")),
            &nContentLength)) {
        return false;
    }

    QDateTime created;
    if (!_parseDate(criticalFields.value("WARC-Date"), &created)) {
        return false;
    }

    QDateTime modified = created;
    if (criticalFields.contains("Last-Modified")) {
        QDateTime candidate;
        if (_parseDate(criticalFields.value("Last-Modified"), &candidate)) {
            modified = candidate;
        }
    }

    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nDataOffset < nOffset) ||
        (nDataOffset > nTotalSize) ||
        ((nTotalSize - nDataOffset) < 4) ||
        (nContentLength > (nTotalSize - nDataOffset - 4))) {
        return false;
    }

    const qint64 nDataEnd = nDataOffset + nContentLength;
    const QByteArray separator = read_array_process(nDataEnd, 4, pPdStruct);
    if (!guardedThis || (separator != "\r\n\r\n") ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QString sFileName;
    const QByteArray type =
        warcTrimLeadingWhitespace(criticalFields.value("WARC-Type"));
    const bool bSupportedType =
        (type == "resource") || (type == "response");
    const bool bMapped = bSupportedType &&
        criticalFields.contains("WARC-Target-URI") &&
        _mapTargetURI(criticalFields.value("WARC-Target-URI"), &sFileName);

    pEntry->nHeaderOffset = nOffset;
    pEntry->nHeaderSize = nDataOffset - nOffset;
    pEntry->nDataOffset = nDataOffset;
    pEntry->nDataSize = nContentLength;
    pEntry->nNextOffset = nDataEnd + 4;
    pEntry->sFileName = sFileName;
    pEntry->created = created;
    pEntry->modified = modified;
    *pVisible = bMapped;

    return (pEntry->nNextOffset > nOffset) &&
           (pEntry->nNextOffset <= nTotalSize);
}

bool XWARC::_scanArchive(QList<WARC_ENTRY> *pEntries,
                         qint64 *pArchiveEnd, PDSTRUCT *pPdStruct)
{
    QPointer<XWARC> guardedThis(this);
    if (pEntries) pEntries->clear();
    if (pArchiveEnd) *pArchiveEnd = 0;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize <= 0)) return false;

    qint64 nOffset = 0;
    qint32 nPhysicalRecords = 0;
    while ((nOffset < nTotalSize) &&
           XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (nPhysicalRecords >= WARC_MAX_RECORDS) {
            if (pEntries) pEntries->clear();
            return false;
        }

        WARC_ENTRY entry = {};
        bool bVisible = false;
        const bool bParsed = _parseRecord(
            nOffset, &entry, &bVisible, pPdStruct);
        if (!guardedThis || !bParsed ||
            (entry.nNextOffset <= nOffset) ||
            (entry.nNextOffset > nTotalSize)) {
            if (pEntries) pEntries->clear();
            return false;
        }

        if (bVisible && pEntries) pEntries->append(entry);
        nOffset = entry.nNextOffset;
        nPhysicalRecords++;
    }

    const bool bResult = (nPhysicalRecords > 0) &&
                         (nOffset == nTotalSize) &&
                         XBinary::isPdStructNotCanceled(pPdStruct);
    if (!bResult && pEntries) pEntries->clear();
    if (bResult && pArchiveEnd) *pArchiveEnd = nOffset;
    return bResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XWARC::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XWARC::initUnpack(UNPACK_STATE *pState,
                       const QMap<UNPACK_PROP, QVariant> &mapProperties,
                       PDSTRUCT *pPdStruct)
{
    QPointer<XWARC> guardedThis(this);
    if (m_bUnpackOperationInProgress) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }

    WARC_UNPACK_CONTEXT *pOldContext =
        static_cast<WARC_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !isPdStructNotCanceled(pPdStruct)) return false;

    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;
    pState->mapUnpackProperties = mapProperties;

    QList<WARC_ENTRY> listEntries;
    const bool bScanned = _scanArchive(&listEntries, nullptr, pPdStruct);
    if (!guardedThis) return false;
    if (!bScanned || !isPdStructNotCanceled(pPdStruct)) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    const qint64 nTotalSize = getSize();
    if (!guardedThis) return false;

    WARC_UNPACK_CONTEXT *pContext =
        new (std::nothrow) WARC_UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pContext->listEntries = listEntries;
    pContext->nCurrentRecord = 0;
    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = listEntries.count();
    pState->nCurrentOffset = listEntries.isEmpty()
        ? nTotalSize
        : listEntries.constFirst().nHeaderOffset;
    pState->nTotalSize = nTotalSize;

    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XWARC::infoCurrent(UNPACK_STATE *pState,
                                          PDSTRUCT *pPdStruct)
{
    QPointer<XWARC> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    if (!pState || !pState->pContext) return result;

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return result;

    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nTotalSize != nCurrentSize)) return result;

    WARC_UNPACK_CONTEXT *pContext =
        static_cast<WARC_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pContext->listEntries.count())) {
        return result;
    }

    const WARC_ENTRY &entry =
        pContext->listEntries.at(pState->nCurrentIndex);
    WARC_ENTRY parsed = {};
    bool bVisible = false;
    const bool bParsed = _parseRecord(
        entry.nHeaderOffset, &parsed, &bVisible, pPdStruct);
    if (!guardedThis || !bParsed || !bVisible ||
        (parsed.nHeaderOffset != entry.nHeaderOffset) ||
        (parsed.nHeaderSize != entry.nHeaderSize) ||
        (parsed.nDataOffset != entry.nDataOffset) ||
        (parsed.nDataSize != entry.nDataSize) ||
        (parsed.nNextOffset != entry.nNextOffset) ||
        (parsed.sFileName != entry.sFileName) ||
        (parsed.created != entry.created) ||
        (parsed.modified != entry.modified)) {
        return ARCHIVERECORD();
    }

    result.nStreamOffset = entry.nDataOffset;
    result.nStreamSize = entry.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, entry.nDataSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, entry.nDataSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_HEADER_OFFSET, entry.nHeaderOffset);
    result.mapProperties.insert(FPART_PROP_HEADER_SIZE, entry.nHeaderSize);
    result.mapProperties.insert(FPART_PROP_FILEMODE, (quint32)0644);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_CTIME, entry.created);
    result.mapProperties.insert(FPART_PROP_MTIME, entry.modified);

    return result;
}

bool XWARC::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XWARC> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext) {
        return false;
    }

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return false;

    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nTotalSize != nCurrentSize)) return false;

    WARC_UNPACK_CONTEXT *pContext =
        static_cast<WARC_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    pState->nCurrentIndex++;
    pContext->nCurrentRecord = pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset =
            pContext->listEntries.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }

    pState->nCurrentOffset = pState->nTotalSize;
    return false;
}

bool XWARC::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    Q_UNUSED(pPdStruct)

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }

    WARC_UNPACK_CONTEXT *pContext =
        static_cast<WARC_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    delete pContext;

    return true;
}

QList<XBinary::FPART_PROP> XWARC::getAvailableFPARTProperties()
{
    QList<FPART_PROP> listResult;
    listResult.append(FPART_PROP_ORIGINALNAME);
    listResult.append(FPART_PROP_UNCOMPRESSEDSIZE);
    listResult.append(FPART_PROP_COMPRESSEDSIZE);
    listResult.append(FPART_PROP_STREAMOFFSET);
    listResult.append(FPART_PROP_STREAMSIZE);
    listResult.append(FPART_PROP_FILEMODE);
    listResult.append(FPART_PROP_CTIME);
    listResult.append(FPART_PROP_MTIME);
    return listResult;
}

bool XWARC::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XWARC> guardedThis(this);
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo =
            static_cast<XArchive::INTERNAL_INFO *>(
                guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(guardedThis->m_internalInfo) =
            *pInfo;
    }

    return guardedThis && bResult;
}

void *XWARC::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XWARC> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;
    return &guardedThis->m_internalInfo;
}

void XWARC::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(
            static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
