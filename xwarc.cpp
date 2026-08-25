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
#include <QCryptographicHash>
#include <QDir>
#include <QSet>
#include <QUrl>

namespace {
const qint32 WARC_MAX_HEADER_SIZE = 1024 * 1024;
const qint32 WARC_MAX_RECORDS = 100000;
const qint32 WARC_MAX_NAME_SIZE = 32768;

QByteArray warcTrimFieldWhitespace(const QByteArray &value)
{
    qint32 nStart = 0;
    qint32 nEnd = value.size();
    while ((nStart < nEnd) &&
           ((value.at(nStart) == ' ') || (value.at(nStart) == '\t'))) {
        nStart++;
    }
    while ((nEnd > nStart) &&
           ((value.at(nEnd - 1) == ' ') || (value.at(nEnd - 1) == '\t'))) {
        nEnd--;
    }
    return value.mid(nStart, nEnd - nStart);
}

QByteArray warcAsciiLower(const QByteArray &value)
{
    QByteArray result = value;
    for (qint32 i = 0; i < result.size(); i++) {
        const char c = result.at(i);
        if ((c >= 'A') && (c <= 'Z')) {
            result[i] = c + ('a' - 'A');
        }
    }
    return result;
}

bool warcIsHttpToken(const QByteArray &value)
{
    if (value.isEmpty()) return false;
    static const QByteArray allowedPunctuation("!#$%&'*+-.^_`|~");
    for (char c : value) {
        if (((c >= '0') && (c <= '9')) ||
            ((c >= 'A') && (c <= 'Z')) ||
            ((c >= 'a') && (c <= 'z')) ||
            allowedPunctuation.contains(c)) {
            continue;
        }
        return false;
    }
    return true;
}

bool warcIsHexDigit(char c)
{
    return ((c >= '0') && (c <= '9')) ||
           ((c >= 'A') && (c <= 'F')) ||
           ((c >= 'a') && (c <= 'f'));
}

bool warcIsValidRecordId(const QByteArray &value)
{
    const QByteArray recordId = warcTrimFieldWhitespace(value);
    if ((recordId.size() < 4) ||
        (recordId.size() > WARC_MAX_NAME_SIZE) ||
        (recordId.at(0) != '<') ||
        (recordId.at(recordId.size() - 1) != '>')) {
        return false;
    }

    const QByteArray uri = recordId.mid(1, recordId.size() - 2);
    const qint32 nColon = uri.indexOf(':');
    if ((nColon <= 0) ||
        !(((uri.at(0) >= 'A') && (uri.at(0) <= 'Z')) ||
          ((uri.at(0) >= 'a') && (uri.at(0) <= 'z')))) {
        return false;
    }
    for (qint32 i = 1; i < nColon; i++) {
        const char c = uri.at(i);
        if (!(((c >= 'A') && (c <= 'Z')) ||
              ((c >= 'a') && (c <= 'z')) ||
              ((c >= '0') && (c <= '9')) ||
              (c == '+') || (c == '-') || (c == '.'))) {
            return false;
        }
    }

    static const QByteArray uriPunctuation("-._~:/?#[]@!$&'()*+,;=");
    for (qint32 i = nColon + 1; i < uri.size(); i++) {
        const char c = uri.at(i);
        if (c == '%') {
            if ((i + 2 >= uri.size()) || !warcIsHexDigit(uri.at(i + 1)) ||
                !warcIsHexDigit(uri.at(i + 2))) {
                return false;
            }
            i += 2;
        } else if (!(((c >= 'A') && (c <= 'Z')) ||
                     ((c >= 'a') && (c <= 'z')) ||
                     ((c >= '0') && (c <= '9')) ||
                     uriPunctuation.contains(c))) {
            return false;
        }
    }
    const QString uriString = QString::fromUtf8(uri.constData(), uri.size());
    if (uriString.toUtf8() != uri) return false;
    const QUrl parsed(uriString, QUrl::StrictMode);
    return parsed.isValid() && !parsed.scheme().isEmpty();
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
    // ISO 28500 currently defines the 1.0 and 1.1 WARC revisions.  The old
    // numeric range calculation accidentally rejected 1.1 while accepting
    // unrelated pre-1.0 strings such as 0.12.
    return (line == "WARC/1.0") || (line == "WARC/1.1");
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

    const QByteArray date = warcTrimFieldWhitespace(value);
    const qint32 nSize = date.size();
    const bool bHasFraction = (nSize >= 22) && (nSize <= 30);
    if ((nSize != 4) && (nSize != 7) && (nSize != 10) &&
        (nSize != 17) && (nSize != 20) && !bHasFraction) {
        return false;
    }

    const auto areDigits = [&date](qint32 nOffset, qint32 nCount) {
        if ((nOffset < 0) || (nCount < 0) ||
            (nOffset > date.size()) || (nCount > date.size() - nOffset)) {
            return false;
        }
        for (qint32 i = 0; i < nCount; i++) {
            const char c = date.at(nOffset + i);
            if ((c < '0') || (c > '9')) return false;
        }
        return true;
    };

    if (!areDigits(0, 4)) return false;
    if (nSize >= 7) {
        if ((date.at(4) != '-') || !areDigits(5, 2)) return false;
    }
    if (nSize >= 10) {
        if ((date.at(7) != '-') || !areDigits(8, 2)) return false;
    }
    if (nSize >= 17) {
        if ((date.at(10) != 'T') || (date.at(13) != ':') ||
            !areDigits(11, 2) || !areDigits(14, 2)) {
            return false;
        }
    }
    if (nSize == 17) {
        if (date.at(16) != 'Z') return false;
    } else if (nSize >= 20) {
        if ((date.at(16) != ':') || !areDigits(17, 2)) return false;
        if (nSize == 20) {
            if (date.at(19) != 'Z') return false;
        } else if ((date.at(19) != '.') ||
                   (date.at(nSize - 1) != 'Z') ||
                   !areDigits(20, nSize - 21)) {
            return false;
        }
    }

    const qint32 nYear = date.mid(0, 4).toInt();
    const qint32 nMonth = (nSize >= 7) ? date.mid(5, 2).toInt() : 1;
    const qint32 nDay = (nSize >= 10) ? date.mid(8, 2).toInt() : 1;
    const qint32 nHour = (nSize >= 17) ? date.mid(11, 2).toInt() : 0;
    const qint32 nMinute = (nSize >= 17) ? date.mid(14, 2).toInt() : 0;
    const qint32 nSecond = (nSize >= 20) ? date.mid(17, 2).toInt() : 0;
    qint32 nMillisecond = 0;
    if (bHasFraction) {
        QByteArray milliseconds = date.mid(20, qMin(3, nSize - 21));
        while (milliseconds.size() < 3) milliseconds.append('0');
        nMillisecond = milliseconds.toInt();
    }

    if ((nYear < 1) || (nYear > 9999) ||
        (nHour < 0) || (nHour > 23) ||
        (nMinute < 0) || (nMinute > 59) ||
        (nSecond < 0) || (nSecond > 59)) {
        return false;
    }

    const QDate qDate(nYear, nMonth, nDay);
    const QTime qTime(nHour, nMinute, nSecond, nMillisecond);
    if (!qDate.isValid() || !qTime.isValid()) return false;

    QDateTime result(qDate, qTime, Qt::UTC);
    if (!result.isValid()) return false;

    *pResult = result;
    return true;
}

bool XWARC::_mapTargetURI(const QByteArray &value, QString *pResult)
{
    if (!pResult) return false;

    const QByteArray uri = warcTrimFieldWhitespace(value);
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

    const QByteArray scheme = warcAsciiLower(uri.left(nSeparator));
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
    QByteArray previousFieldKey;
    qint32 nLineOffset = 0;
    qint32 nLineNumber = 0;
    bool bSawTerminator = false;
    bool bVersion11 = false;

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
            bVersion11 = (line == "WARC/1.1");
        } else if ((line.at(0) == ' ') || (line.at(0) == '\t')) {
            // WARC named fields inherit the HTTP-style folding rule.  Unfold
            // critical values to one SP; continuations of extension fields
            // remain syntactically accepted without retaining unneeded data.
            if (previousFieldKey.isEmpty()) return false;
            if (criticalFields.contains(previousFieldKey)) {
                QByteArray value = criticalFields.value(previousFieldKey);
                value.append(' ');
                value.append(warcTrimFieldWhitespace(line));
                criticalFields.insert(previousFieldKey, value);
            }
        } else {
            const qint32 nColon = line.indexOf(':');
            if (nColon <= 0) return false;

            const QByteArray key = line.left(nColon);
            if (!warcIsHttpToken(key)) return false;

            const QByteArray normalizedKey = warcAsciiLower(key);
            previousFieldKey = normalizedKey;
            const bool bCritical =
                (normalizedKey == "warc-type") ||
                (normalizedKey == "warc-target-uri") ||
                (normalizedKey == "content-length") ||
                (normalizedKey == "warc-date") ||
                (normalizedKey == "warc-record-id") ||
                (normalizedKey == "last-modified");
            if (bCritical) {
                if (criticalFields.contains(normalizedKey)) return false;
                criticalFields.insert(normalizedKey, line.mid(nColon + 1));
            }
        }

        nLineNumber++;
    }

    if (!bSawTerminator || (nLineNumber == 0) ||
        !criticalFields.contains("warc-type") ||
        !criticalFields.contains("warc-record-id") ||
        !criticalFields.contains("content-length") ||
        !criticalFields.contains("warc-date")) {
        return false;
    }

    const QByteArray type = warcAsciiLower(
        warcTrimFieldWhitespace(criticalFields.value("warc-type")));
    const QByteArray recordId =
        warcTrimFieldWhitespace(criticalFields.value("warc-record-id"));
    if (!warcIsHttpToken(type) || !warcIsValidRecordId(recordId)) {
        return false;
    }

    qint64 nContentLength = -1;
    if (!_parseUnsignedDecimal(
            warcTrimFieldWhitespace(criticalFields.value("content-length")),
            &nContentLength)) {
        return false;
    }

    QDateTime created;
    if (!bVersion11 &&
        (warcTrimFieldWhitespace(criticalFields.value("warc-date")).size() !=
         20)) {
        return false;
    }
    if (!_parseDate(criticalFields.value("warc-date"), &created)) {
        return false;
    }

    QDateTime modified = created;
    if (criticalFields.contains("last-modified")) {
        QDateTime candidate;
        if (_parseDate(criticalFields.value("last-modified"), &candidate)) {
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
    const bool bSupportedType =
        (type == "resource") || (type == "response");
    const bool bMapped = bSupportedType &&
        criticalFields.contains("warc-target-uri") &&
        _mapTargetURI(criticalFields.value("warc-target-uri"), &sFileName);

    pEntry->nHeaderOffset = nOffset;
    pEntry->nHeaderSize = nDataOffset - nOffset;
    pEntry->nDataOffset = nDataOffset;
    pEntry->nDataSize = nContentLength;
    pEntry->nNextOffset = nDataEnd + 4;
    pEntry->baRecordIdHash = QCryptographicHash::hash(
        recordId, QCryptographicHash::Sha256);
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
    QSet<QByteArray> recordIds;
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
        if (recordIds.contains(entry.baRecordIdHash)) {
            if (pEntries) pEntries->clear();
            return false;
        }
        recordIds.insert(entry.baRecordIdHash);

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
        (parsed.baRecordIdHash != entry.baRecordIdHash) ||
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
