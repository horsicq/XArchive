/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 *
 * Format knowledge: RFC 5322 (message syntax), RFC 2045/2046 (MIME bodies,
 * base64 and quoted-printable), RFC 2047 (encoded words), RFC 2231
 * (parameter value encoding) and RFC 4155 (mbox).  All decoders below are
 * original; nothing was taken from any other implementation.
 */
#include "xmimemail.h"

#include <QPointer>

#include <limits>
#include <memory>
#include <new>

namespace {

const qint64 MIME_MAX_SOURCE = Q_INT64_C(256) * 1024 * 1024;
const qint32 MIME_MAX_HEADER_BLOCK = 1024 * 1024;
const qint32 MIME_PROBE_SIZE = 64 * 1024;
const qint32 MIME_MAX_DEPTH = 32;
const qint32 MIME_MAX_ITEMS = 65536;
const qint32 MIME_MAX_MESSAGES = 65536;

bool isHeaderNameChar(char c)
{
    return ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')) || ((c >= '0') && (c <= '9')) || (c == '-') || (c == '_') || (c == '.');
}

qint32 hexValue(char c)
{
    if ((c >= '0') && (c <= '9')) return c - '0';
    if ((c >= 'a') && (c <= 'f')) return c - 'a' + 10;
    if ((c >= 'A') && (c <= 'F')) return c - 'A' + 10;
    return -1;
}

qint32 base64Value(char c)
{
    if ((c >= 'A') && (c <= 'Z')) return c - 'A';
    if ((c >= 'a') && (c <= 'z')) return c - 'a' + 26;
    if ((c >= '0') && (c <= '9')) return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

// Headers that identify a mail message rather than an arbitrary "Key: value" text.
bool isStrongMailHeader(const QByteArray &baName)
{
    return (baName == "from") || (baName == "received") || (baName == "return-path") || (baName == "message-id") || (baName == "mime-version") ||
           (baName == "date") || (baName == "delivered-to") || (baName == "x-mailer") || (baName == "snapshot-content-location");
}

bool isMailHeader(const QByteArray &baName)
{
    return isStrongMailHeader(baName) || (baName == "to") || (baName == "subject") || (baName == "cc") || (baName == "content-type") || (baName == "sender") ||
           (baName == "reply-to") || (baName == "in-reply-to") || (baName == "references") || (baName == "x-priority") || (baName == "content-transfer-encoding");
}

}  // namespace

XMimeMail::XMimeMail(QIODevice *pDevice, FT fileTypeHint) : XArchive(pDevice), m_fileTypeHint(fileTypeHint)
{
}

// Returns the offset of the line terminator (or nEnd) for the line starting at
// nPos; *pnNext receives the offset of the following line.
qint32 XMimeMail::lineEnd(const QByteArray &baData, qint32 nPos, qint32 nEnd, qint32 *pnNext)
{
    qint32 nLF = baData.indexOf('\n', nPos);
    if ((nLF < 0) || (nLF >= nEnd)) {
        *pnNext = nEnd;
        return nEnd;
    }
    *pnNext = nLF + 1;
    if ((nLF > nPos) && (baData.at(nLF - 1) == '\r')) return nLF - 1;
    return nLF;
}

bool XMimeMail::parseHeaders(const QByteArray &baData, qint32 nStart, qint32 nEnd, QList<HEADER> *pHeaders, qint32 *pnBodyStart, bool bStrict)
{
    if (!pHeaders || !pnBodyStart || (nStart < 0) || (nEnd > baData.size()) || (nStart > nEnd)) return false;
    pHeaders->clear();
    qint32 nPos = nStart;
    *pnBodyStart = nEnd;
    while (nPos < nEnd) {
        qint32 nNext = 0;
        const qint32 nLineEnd = lineEnd(baData, nPos, nEnd, &nNext);
        if ((nPos - nStart) > MIME_MAX_HEADER_BLOCK) return false;
        if (nLineEnd == nPos) {
            // Empty line: the header block is complete.
            *pnBodyStart = nNext;
            return true;
        }
        const char first = baData.at(nPos);
        if ((first == ' ') || (first == '\t')) {
            // Continuation of the previous field (unfolding keeps the whitespace).
            if (pHeaders->isEmpty()) {
                if (bStrict) return false;
                *pnBodyStart = nPos;
                return true;
            }
            (*pHeaders)[pHeaders->size() - 1].baValue += baData.mid(nPos, nLineEnd - nPos);
            nPos = nNext;
            continue;
        }
        qint32 nColon = nPos;
        while ((nColon < nLineEnd) && isHeaderNameChar(baData.at(nColon))) ++nColon;
        if ((nColon == nPos) || (nColon >= nLineEnd) || (baData.at(nColon) != ':')) {
            // Not a header line: a strict block is refused, a lenient one
            // (a body part without headers) starts its body here.
            if (bStrict) return false;
            *pnBodyStart = nPos;
            return true;
        }
        HEADER header;
        header.baName = baData.mid(nPos, nColon - nPos).toLower();
        header.baValue = baData.mid(nColon + 1, nLineEnd - nColon - 1);
        pHeaders->append(header);
        nPos = nNext;
    }
    // Headers up to the end of the entity: the body is empty.
    return true;
}

QByteArray XMimeMail::headerValue(const QList<HEADER> &listHeaders, const char *pszName)
{
    const QByteArray baName(pszName);
    for (qint32 i = 0; i < listHeaders.size(); ++i) {
        if (listHeaders.at(i).baName == baName) return listHeaders.at(i).baValue.trimmed();
    }
    return QByteArray();
}

QByteArray XMimeMail::mediaType(const QByteArray &baHeaderValue)
{
    const qint32 nSemicolon = baHeaderValue.indexOf(';');
    QByteArray baType = (nSemicolon < 0) ? baHeaderValue : baHeaderValue.left(nSemicolon);
    baType = baType.trimmed().toLower();
    for (qint32 i = 0; i < baType.size(); ++i) {
        const char c = baType.at(i);
        if ((c <= ' ') || (c == '"') || (c == '(') || (c == ')') || (c == '<') || (c == '>') || (c == '@') || (c == ',') || (c == ':') || (c == '\\') || (c == '[') ||
            (c == ']') || (c == '?') || (c == '='))
            return QByteArray();
    }
    return baType;
}

// RFC 2045 parameters with RFC 2231 continuations (name*0=, name*1*=) and
// charset'lang'percent-encoded extended values.
QString XMimeMail::parameter(const QByteArray &baHeaderValue, const QByteArray &baName, bool *pbFound)
{
    if (pbFound) *pbFound = false;
    QList<QByteArray> listNames;
    QList<QByteArray> listValues;
    qint32 nPos = baHeaderValue.indexOf(';');
    while ((nPos >= 0) && (nPos < baHeaderValue.size())) {
        ++nPos;
        while ((nPos < baHeaderValue.size()) && ((baHeaderValue.at(nPos) == ' ') || (baHeaderValue.at(nPos) == '\t') || (baHeaderValue.at(nPos) == '\r') ||
                                                 (baHeaderValue.at(nPos) == '\n')))
            ++nPos;
        const qint32 nEqual = baHeaderValue.indexOf('=', nPos);
        if (nEqual < 0) break;
        const QByteArray baKey = baHeaderValue.mid(nPos, nEqual - nPos).trimmed().toLower();
        nPos = nEqual + 1;
        while ((nPos < baHeaderValue.size()) && ((baHeaderValue.at(nPos) == ' ') || (baHeaderValue.at(nPos) == '\t'))) ++nPos;
        QByteArray baValue;
        if ((nPos < baHeaderValue.size()) && (baHeaderValue.at(nPos) == '"')) {
            ++nPos;
            while (nPos < baHeaderValue.size()) {
                const char c = baHeaderValue.at(nPos);
                if (c == '"') {
                    ++nPos;
                    break;
                }
                if ((c == '\\') && ((nPos + 1) < baHeaderValue.size())) {
                    baValue.append(baHeaderValue.at(nPos + 1));
                    nPos += 2;
                    continue;
                }
                baValue.append(c);
                ++nPos;
            }
            nPos = baHeaderValue.indexOf(';', nPos);
        } else {
            const qint32 nSemicolon = baHeaderValue.indexOf(';', nPos);
            baValue = ((nSemicolon < 0) ? baHeaderValue.mid(nPos) : baHeaderValue.mid(nPos, nSemicolon - nPos)).trimmed();
            nPos = nSemicolon;
        }
        if (!baKey.isEmpty()) {
            listNames.append(baKey);
            listValues.append(baValue);
        }
    }

    // Collect the plain value and the numbered/extended segments in order.
    QByteArray baJoined;
    QByteArray baCharset;
    bool bExtended = false;
    bool bFound = false;
    qint32 nSegment = 0;
    QByteArray baPlain;
    bool bPlain = false;
    for (qint32 i = 0; i < listNames.size(); ++i) {
        if (listNames.at(i) == baName) {
            baPlain = listValues.at(i);
            bPlain = true;
        }
    }
    for (;;) {
        bool bSegmentFound = false;
        const QByteArray baNumbered = baName + "*" + QByteArray::number(nSegment);
        for (qint32 i = 0; i < listNames.size(); ++i) {
            const QByteArray &baKey = listNames.at(i);
            const bool bSegmentExtended = (baKey == (baNumbered + "*")) || ((nSegment == 0) && (baKey == (baName + "*")));
            if ((baKey == baNumbered) || bSegmentExtended) {
                QByteArray baSegment = listValues.at(i);
                if (bSegmentExtended) {
                    if (!bExtended) {
                        // charset'language'value
                        const qint32 nQuote1 = baSegment.indexOf('\'');
                        const qint32 nQuote2 = (nQuote1 < 0) ? -1 : baSegment.indexOf('\'', nQuote1 + 1);
                        if (nQuote2 >= 0) {
                            baCharset = baSegment.left(nQuote1).toLower();
                            baSegment = baSegment.mid(nQuote2 + 1);
                        }
                        bExtended = true;
                    }
                    QByteArray baDecoded;
                    for (qint32 j = 0; j < baSegment.size(); ++j) {
                        if ((baSegment.at(j) == '%') && ((j + 2) < baSegment.size()) && (hexValue(baSegment.at(j + 1)) >= 0) && (hexValue(baSegment.at(j + 2)) >= 0)) {
                            baDecoded.append(static_cast<char>((hexValue(baSegment.at(j + 1)) << 4) | hexValue(baSegment.at(j + 2))));
                            j += 2;
                        } else {
                            baDecoded.append(baSegment.at(j));
                        }
                    }
                    baSegment = baDecoded;
                }
                baJoined += baSegment;
                bSegmentFound = true;
                bFound = true;
                break;
            }
        }
        if (!bSegmentFound || (nSegment > 4096)) break;
        ++nSegment;
    }
    if (bFound) {
        if (pbFound) *pbFound = true;
        return bExtended ? decodeCharset(baJoined, baCharset) : QString::fromUtf8(baJoined);
    }
    if (bPlain) {
        if (pbFound) *pbFound = true;
        return QString::fromUtf8(baPlain);
    }
    return QString();
}

QString XMimeMail::decodeCharset(const QByteArray &baBytes, const QByteArray &baCharset)
{
    const QByteArray baLower = baCharset.toLower();
    if (baLower.isEmpty() || (baLower == "utf-8") || (baLower == "utf8") || (baLower == "us-ascii") || (baLower == "ascii")) return QString::fromUtf8(baBytes);
    if ((baLower == "iso-8859-1") || (baLower == "latin1") || (baLower == "latin-1") || (baLower == "iso8859-1") || (baLower == "windows-1252") || (baLower == "cp1252"))
        return QString::fromLatin1(baBytes);
    // Other charsets are outside the implemented scope: keep the bytes as Latin-1
    // so that nothing is lost, and let the caller see them.
    return QString::fromLatin1(baBytes);
}

// RFC 2047: =?charset?B|Q?text?= words; whitespace between two adjacent
// encoded words is dropped.
QString XMimeMail::decodeEncodedWords(const QString &sText)
{
    if (!sText.contains(QStringLiteral("=?"))) return sText;
    QString sResult;
    qint32 nPos = 0;
    bool bLastWasWord = false;
    QString sPendingSpace;
    while (nPos < sText.size()) {
        const qint32 nStart = sText.indexOf(QStringLiteral("=?"), nPos);
        if (nStart < 0) {
            sResult += sPendingSpace + sText.mid(nPos);
            break;
        }
        const qint32 nCharsetEnd = sText.indexOf(QLatin1Char('?'), nStart + 2);
        const qint32 nEncodingEnd = (nCharsetEnd < 0) ? -1 : sText.indexOf(QLatin1Char('?'), nCharsetEnd + 1);
        const qint32 nWordEnd = (nEncodingEnd < 0) ? -1 : sText.indexOf(QStringLiteral("?="), nEncodingEnd + 1);
        if ((nWordEnd < 0) || ((nEncodingEnd - nCharsetEnd) != 2)) {
            sResult += sPendingSpace + sText.mid(nPos, nStart + 2 - nPos);
            sPendingSpace.clear();
            bLastWasWord = false;
            nPos = nStart + 2;
            continue;
        }
        const QString sBetween = sText.mid(nPos, nStart - nPos);
        const bool bOnlySpace = !sBetween.isEmpty() && (sBetween.trimmed().isEmpty());
        if (!(bLastWasWord && bOnlySpace)) sResult += sPendingSpace + sBetween;
        sPendingSpace.clear();
        QByteArray baCharset = sText.mid(nStart + 2, nCharsetEnd - nStart - 2).toLatin1();
        const qint32 nStar = baCharset.indexOf('*');  // RFC 2231 language tag inside a word
        if (nStar >= 0) baCharset = baCharset.left(nStar);
        const QChar cEncoding = sText.at(nCharsetEnd + 1).toUpper();
        const QByteArray baEncoded = sText.mid(nEncodingEnd + 1, nWordEnd - nEncodingEnd - 1).toLatin1();
        QByteArray baBytes;
        if (cEncoding == QLatin1Char('B')) {
            baBytes = decodeBase64(baEncoded);
        } else if (cEncoding == QLatin1Char('Q')) {
            for (qint32 i = 0; i < baEncoded.size(); ++i) {
                const char c = baEncoded.at(i);
                if (c == '_') baBytes.append(' ');
                else if ((c == '=') && ((i + 2) < baEncoded.size()) && (hexValue(baEncoded.at(i + 1)) >= 0) && (hexValue(baEncoded.at(i + 2)) >= 0)) {
                    baBytes.append(static_cast<char>((hexValue(baEncoded.at(i + 1)) << 4) | hexValue(baEncoded.at(i + 2))));
                    i += 2;
                } else baBytes.append(c);
            }
        } else {
            sResult += sText.mid(nStart, nWordEnd + 2 - nStart);
            bLastWasWord = false;
            nPos = nWordEnd + 2;
            continue;
        }
        sResult += decodeCharset(baBytes, baCharset);
        bLastWasWord = true;
        nPos = nWordEnd + 2;
    }
    return sResult;
}

// Lenient base64: characters outside the alphabet are ignored, '=' ends the data.
QByteArray XMimeMail::decodeBase64(const QByteArray &baData)
{
    QByteArray baResult;
    baResult.reserve((baData.size() / 4) * 3 + 3);
    quint32 nAccumulator = 0;
    qint32 nBits = 0;
    for (qint32 i = 0; i < baData.size(); ++i) {
        const char c = baData.at(i);
        if (c == '=') break;
        const qint32 nValue = base64Value(c);
        if (nValue < 0) continue;
        nAccumulator = (nAccumulator << 6) | static_cast<quint32>(nValue);
        nBits += 6;
        if (nBits >= 8) {
            nBits -= 8;
            baResult.append(static_cast<char>((nAccumulator >> nBits) & 0xFFU));
        }
    }
    return baResult;
}

// RFC 2045 quoted-printable, with the same tolerance as common decoders: an
// '=' that is not followed by two hex digits or a line break is literal.
QByteArray XMimeMail::decodeQuotedPrintable(const QByteArray &baData)
{
    QByteArray baResult;
    baResult.reserve(baData.size());
    const qint32 nSize = baData.size();
    qint32 i = 0;
    while (i < nSize) {
        const char c = baData.at(i);
        if (c != '=') {
            baResult.append(c);
            ++i;
            continue;
        }
        ++i;
        if (i >= nSize) break;  // a trailing '=' with nothing after it is dropped
        const char n = baData.at(i);
        if ((n == '\r') || (n == '\n')) {
            // Soft line break: skip to the end of the line terminator.
            if (n == '\r') {
                while ((i < nSize) && (baData.at(i) != '\n')) ++i;
            }
            if (i < nSize) ++i;
        } else if (((i + 1) < nSize) && (hexValue(n) >= 0) && (hexValue(baData.at(i + 1)) >= 0)) {
            baResult.append(static_cast<char>((hexValue(n) << 4) | hexValue(baData.at(i + 1))));
            i += 2;
        } else {
            baResult.append('=');
        }
    }
    return baResult;
}

QString XMimeMail::sanitizeName(const QString &sName)
{
    QString sResult = sName;
    sResult.replace(QLatin1Char('\\'), QLatin1Char('/'));
    const qint32 nSlash = sResult.lastIndexOf(QLatin1Char('/'));
    if (nSlash >= 0) sResult = sResult.mid(nSlash + 1);
    sResult = sResult.trimmed();
    static const QString sIllegal = QStringLiteral("<>:\"/\\|?*");
    for (qint32 i = 0; i < sResult.size(); ++i) {
        const QChar c = sResult.at(i);
        if ((c.unicode() < 0x20) || sIllegal.contains(c)) sResult[i] = QLatin1Char('_');
    }
    return sResult;
}

QString XMimeMail::extensionFor(const QByteArray &baMediaType)
{
    static const char *const s_ppszMap[] = {
        "text/plain", "txt", "text/html", "html", "text/css", "css", "text/xml", "xml", "application/xml", "xml", "application/json", "json", "text/csv", "csv",
        "text/calendar", "ics", "text/javascript", "js", "application/javascript", "js", "image/jpeg", "jpg", "image/png", "png", "image/gif", "gif",
        "image/svg+xml", "svg", "image/webp", "webp", "image/bmp", "bmp", "image/tiff", "tif", "application/pdf", "pdf", "application/zip", "zip",
        "application/gzip", "gz", "application/x-tar", "tar", "application/rtf", "rtf", "application/msword", "doc", "message/rfc822", "eml", "audio/mpeg", "mp3",
        "video/mp4", "mp4", "application/octet-stream", "bin", nullptr, nullptr};
    for (qint32 i = 0; s_ppszMap[i]; i += 2) {
        if (baMediaType == s_ppszMap[i]) return QString::fromLatin1(s_ppszMap[i + 1]);
    }
    return QStringLiteral("bin");
}

bool XMimeMail::walkEntity(const QByteArray &baData, qint32 nStart, qint32 nEnd, qint32 nDepth, WALK *pWalk)
{
    if (!pWalk || !pWalk->pItems || (nDepth > MIME_MAX_DEPTH) || !XBinary::isPdStructNotCanceled(pWalk->pPdStruct)) return false;
    QList<HEADER> listHeaders;
    qint32 nBodyStart = nEnd;
    if (!parseHeaders(baData, nStart, nEnd, &listHeaders, &nBodyStart, false)) return false;

    const QByteArray baContentTypeValue = headerValue(listHeaders, "content-type");
    const bool bHasContentType = !baContentTypeValue.isEmpty();
    QByteArray baType = bHasContentType ? mediaType(baContentTypeValue) : QByteArray("text/plain");
    if (baType.isEmpty()) baType = "text/plain";

    if (baType.startsWith("multipart/")) {
        bool bBoundaryFound = false;
        const QString sBoundary = parameter(baContentTypeValue, "boundary", &bBoundaryFound);
        const QByteArray baDelimiter = "--" + sBoundary.toLatin1();
        if (bBoundaryFound && !sBoundary.isEmpty()) {
            // Locate the delimiter lines; a part spans from the line after a
            // delimiter to (excluding) the line terminator before the next one.
            qint32 nPos = nBodyStart;
            qint32 nPartStart = -1;
            bool bAnyDelimiter = false;
            while (nPos < nEnd) {
                if (!XBinary::isPdStructNotCanceled(pWalk->pPdStruct)) return false;
                qint32 nNext = 0;
                const qint32 nLineEnd = lineEnd(baData, nPos, nEnd, &nNext);
                bool bDelimiter = false;
                bool bClosing = false;
                if (((nLineEnd - nPos) >= baDelimiter.size()) && (baData.mid(nPos, baDelimiter.size()) == baDelimiter)) {
                    qint32 nTail = nPos + baDelimiter.size();
                    if (((nLineEnd - nTail) >= 2) && (baData.at(nTail) == '-') && (baData.at(nTail + 1) == '-')) {
                        bClosing = true;
                        nTail += 2;
                    }
                    bDelimiter = true;
                    while (nTail < nLineEnd) {
                        if ((baData.at(nTail) != ' ') && (baData.at(nTail) != '\t')) {
                            bDelimiter = false;
                            break;
                        }
                        ++nTail;
                    }
                }
                if (bDelimiter) {
                    bAnyDelimiter = true;
                    if (nPartStart >= 0) {
                        // Drop the terminator that belongs to the delimiter.
                        qint32 nPartEnd = nPos;
                        if ((nPartEnd > nPartStart) && (baData.at(nPartEnd - 1) == '\n')) {
                            --nPartEnd;
                            if ((nPartEnd > nPartStart) && (baData.at(nPartEnd - 1) == '\r')) --nPartEnd;
                        }
                        if (!walkEntity(baData, nPartStart, nPartEnd, nDepth + 1, pWalk)) return false;
                    }
                    nPartStart = bClosing ? -1 : nNext;
                    if (bClosing) break;
                }
                nPos = nNext;
            }
            if (nPartStart >= 0) {
                // No closing delimiter: the last part runs to the end of the entity.
                if (!walkEntity(baData, nPartStart, nEnd, nDepth + 1, pWalk)) return false;
            }
            if (bAnyDelimiter) return true;
        }
        // No usable boundary: fall through and expose the raw body as one part.
    }

    // Leaf part.
    ++pWalk->nIndex;
    if (pWalk->pItems->size() >= MIME_MAX_ITEMS) return false;
    const QByteArray baEncoding = headerValue(listHeaders, "content-transfer-encoding").toLower();
    QByteArray baBody = baData.mid(nBodyStart, nEnd - nBodyStart);
    QString sMethod = QStringLiteral("Store");
    if (baEncoding == "base64") {
        baBody = decodeBase64(baBody);
        sMethod = QStringLiteral("base64");
    } else if (baEncoding == "quoted-printable") {
        baBody = decodeQuotedPrintable(baBody);
        sMethod = QStringLiteral("quoted-printable");
    } else if (!baEncoding.isEmpty() && (baEncoding != "7bit") && (baEncoding != "8bit") && (baEncoding != "binary")) {
        sMethod = QStringLiteral("Store (") + QString::fromLatin1(baEncoding) + QLatin1Char(')');
    }

    bool bFound = false;
    QString sName = parameter(headerValue(listHeaders, "content-disposition"), "filename", &bFound);
    if (!bFound || sName.isEmpty()) sName = parameter(baContentTypeValue, "name", &bFound);
    sName = sanitizeName(decodeEncodedWords(sName));
    if (sName.isEmpty()) sName = QStringLiteral("part%1.%2").arg(pWalk->nIndex).arg(extensionFor(baType));
    if (pWalk->setUsed.contains(sName.toLower())) sName = QStringLiteral("part%1_%2").arg(pWalk->nIndex).arg(sName);
    pWalk->setUsed.insert(sName.toLower());

    ITEM item;
    item.sName = pWalk->sPrefix + sName;
    item.baData = baBody;
    item.sMethod = sMethod;
    item.sContentType = QString::fromLatin1(baType);
    pWalk->pItems->append(item);
    return true;
}

XBinary::FT XMimeMail::classify(const QByteArray &baData)
{
    const qint32 nProbeEnd = qMin(baData.size(), MIME_PROBE_SIZE);
    if (nProbeEnd < 16) return FT_UNKNOWN;
    qint32 nHeaderStart = 0;
    bool bMbox = false;
    if (baData.startsWith("From ")) {
        // "From <sender> <date>": the separator line must carry at least a
        // sender token, and a header block must follow it.
        qint32 nNext = 0;
        const qint32 nLineEnd = lineEnd(baData, 0, nProbeEnd, &nNext);
        const QByteArray baSeparator = baData.mid(5, nLineEnd - 5).trimmed();
        if (baSeparator.isEmpty() || (nNext >= nProbeEnd)) return FT_UNKNOWN;
        nHeaderStart = nNext;
        bMbox = true;
    }
    QList<HEADER> listHeaders;
    qint32 nBodyStart = 0;
    if (!parseHeaders(baData, nHeaderStart, nProbeEnd, &listHeaders, &nBodyStart, true)) return FT_UNKNOWN;
    if (nBodyStart >= nProbeEnd && (nProbeEnd < baData.size())) return FT_UNKNOWN;  // no blank line within the probe window
    qint32 nStrong = 0;
    qint32 nMail = 0;
    QByteArray baContentType;
    for (qint32 i = 0; i < listHeaders.size(); ++i) {
        const HEADER &header = listHeaders.at(i);
        if (isStrongMailHeader(header.baName)) ++nStrong;
        if (isMailHeader(header.baName)) ++nMail;
        if ((header.baName == "content-type") && baContentType.isEmpty()) baContentType = mediaType(header.baValue.trimmed());
    }
    if ((listHeaders.size() < 2) || (nStrong < 1) || (nMail < 2)) return FT_UNKNOWN;
    if (bMbox) return FT_MBOX;
    if (baContentType == "multipart/related") return FT_MHTML;
    return FT_EML;
}

bool XMimeMail::decode(const QByteArray &baSource, FT fileTypeHint, FT *pFileType, QList<ITEM> *pItems, PDSTRUCT *pPdStruct)
{
    if (!pFileType || !pItems) return false;
    pItems->clear();
    *pFileType = FT_UNKNOWN;
    const FT detected = classify(baSource);
    if (detected == FT_UNKNOWN) return false;
    FT fileType = detected;
    if (fileTypeHint != FT_UNKNOWN) {
        const bool bHintMbox = (fileTypeHint == FT_MBOX);
        if (bHintMbox != (detected == FT_MBOX)) return false;
        if ((fileTypeHint != FT_EML) && (fileTypeHint != FT_MHTML) && (fileTypeHint != FT_MBOX)) return false;
        fileType = fileTypeHint;
    }

    if (fileType == FT_MBOX) {
        // RFC 4155: any line starting with "From " separates messages; the
        // empty line before a separator belongs to the mailbox, not the message.
        QList<qint32> listSeparatorStarts;
        QList<qint32> listSeparatorEnds;
        qint32 nPos = 0;
        const qint32 nSize = baSource.size();
        while (nPos < nSize) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            qint32 nNext = 0;
            lineEnd(baSource, nPos, nSize, &nNext);
            if (((nSize - nPos) >= 5) && (baSource.at(nPos) == 'F') && (baSource.mid(nPos, 5) == "From ")) {
                listSeparatorStarts.append(nPos);
                listSeparatorEnds.append(nNext);
                if (listSeparatorStarts.size() > MIME_MAX_MESSAGES) return false;
            }
            nPos = nNext;
        }
        for (qint32 m = 0; m < listSeparatorStarts.size(); ++m) {
            const qint32 nMessageStart = listSeparatorEnds.at(m);
            qint32 nStop = ((m + 1) < listSeparatorStarts.size()) ? listSeparatorStarts.at(m + 1) : nSize;
            if (((nStop - nMessageStart) >= 2) && (baSource.at(nStop - 1) == '\n') && (baSource.at(nStop - 2) == '\n')) nStop -= 1;
            else if (((nStop - nMessageStart) >= 3) && (baSource.at(nStop - 1) == '\n') && (baSource.at(nStop - 2) == '\r') && (baSource.at(nStop - 3) == '\n')) nStop -= 2;
            const QString sPrefix = QStringLiteral("msg%1/").arg(m + 1);
            ITEM raw;
            raw.sName = sPrefix + QStringLiteral("message.eml");
            raw.baData = baSource.mid(nMessageStart, nStop - nMessageStart);
            raw.sMethod = QStringLiteral("Store");
            raw.sContentType = QStringLiteral("message/rfc822");
            pItems->append(raw);
            WALK walk;
            walk.pItems = pItems;
            walk.sPrefix = sPrefix;
            walk.pPdStruct = pPdStruct;
            if (!walkEntity(baSource, nMessageStart, nStop, 0, &walk)) return false;
        }
    } else {
        WALK walk;
        walk.pItems = pItems;
        walk.pPdStruct = pPdStruct;
        if (!walkEntity(baSource, 0, baSource.size(), 0, &walk)) return false;
    }
    if (pItems->isEmpty()) return false;
    *pFileType = fileType;
    return XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XMimeMail::readSource(QByteArray *pData, PDSTRUCT *pPdStruct)
{
    if (!pData || !isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<XMimeMail> guardedThis(this);
    const qint64 nSize = getSize();
    if (!guardedThis || (nSize < 16) || (nSize > MIME_MAX_SOURCE) || (nSize > (std::numeric_limits<int>::max)())) return false;
    // Classify from a bounded probe before materialising the whole source.
    const QByteArray baProbe = read_array_process(0, qMin<qint64>(nSize, MIME_PROBE_SIZE), pPdStruct);
    if (!guardedThis || (baProbe.size() < 16) || !isPdStructNotCanceled(pPdStruct)) return false;
    if ((nSize > baProbe.size()) && (classify(baProbe) == FT_UNKNOWN)) return false;
    *pData = (nSize == baProbe.size()) ? baProbe : read_array_process(0, nSize, pPdStruct);
    return guardedThis && (pData->size() == nSize) && isPdStructNotCanceled(pPdStruct);
}

bool XMimeMail::isValid(QIODevice *pDevice, FT fileTypeHint, PDSTRUCT *pPdStruct)
{
    XMimeMail archive(pDevice, fileTypeHint);
    return archive.isValid(pPdStruct);
}

bool XMimeMail::isValid(PDSTRUCT *pPdStruct)
{
    QByteArray baSource;
    FT fileType = FT_UNKNOWN;
    QList<ITEM> listItems;
    return readSource(&baSource, pPdStruct) && decode(baSource, m_fileTypeHint, &fileType, &listItems, pPdStruct) && isPdStructNotCanceled(pPdStruct);
}

XBinary::FT XMimeMail::detectFileType(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMimeMail archive(pDevice);
    QByteArray baSource;
    FT fileType = FT_UNKNOWN;
    QList<ITEM> listItems;
    if (!archive.readSource(&baSource, pPdStruct) || !decode(baSource, FT_UNKNOWN, &fileType, &listItems, pPdStruct)) return FT_UNKNOWN;
    return fileType;
}

XBinary::FT XMimeMail::getFileType()
{
    return (m_fileTypeHint != FT_UNKNOWN) ? m_fileTypeHint : detectFileType(getDevice(), nullptr);
}
XBinary::MODE XMimeMail::getMode()
{
    return MODE_DATA;
}
qint32 XMimeMail::getType()
{
    return TYPE_ARCHIVE;
}
XBinary::ENDIAN XMimeMail::getEndian()
{
    return ENDIAN_LITTLE;
}
QString XMimeMail::getFileFormatExt()
{
    const FT fileType = getFileType();
    if (fileType == FT_MHTML) return QStringLiteral("mht");
    if (fileType == FT_MBOX) return QStringLiteral("mbox");
    return QStringLiteral("eml");
}
QString XMimeMail::getFileFormatExtsString()
{
    const FT fileType = getFileType();
    if (fileType == FT_MHTML) return QStringLiteral("MIME HTML archive (*.mht *.mhtml)");
    if (fileType == FT_MBOX) return QStringLiteral("Unix mailbox (*.mbox *.mbx)");
    return QStringLiteral("E-mail message (*.eml)");
}
QString XMimeMail::getMIMEString()
{
    const FT fileType = getFileType();
    if (fileType == FT_MHTML) return QStringLiteral("multipart/related");
    if (fileType == FT_MBOX) return QStringLiteral("application/mbox");
    return QStringLiteral("message/rfc822");
}
qint64 XMimeMail::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return isValid(pPdStruct) ? getSize() : 0;
}
XBinary::OSNAME XMimeMail::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}
QString XMimeMail::getVersion()
{
    return QString();
}
QList<QString> XMimeMail::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'MIME-Version: '") << QStringLiteral("'Return-Path: '") << QStringLiteral("'Received: '");
}
XBinary *XMimeMail::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XMimeMail(pDevice, m_fileTypeHint);
}

bool XMimeMail::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XMimeMail> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !bindUnpackSource(pState, pPdStruct)) return false;

    QByteArray baSource;
    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext || !readSource(&baSource, pPdStruct) || !guardedThis || !decode(baSource, m_fileTypeHint, &pContext->fileType, &pContext->listItems, pPdStruct) ||
        pContext->listItems.isEmpty()) {
        delete pContext;
        if (guardedThis) releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listItems.size();
    pState->nCurrentOffset = 0;
    pState->nTotalSize = baSource.size();
    pState->mapUnpackProperties = mapProperties;
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

XBinary::ARCHIVERECORD XMimeMail::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XMimeMail> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return ARCHIVERECORD();
    const UNPACK_CONTEXT *pContext = static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    if (pContext->listItems.size() != pState->nNumberOfRecords) return ARCHIVERECORD();
    const ITEM &item = pContext->listItems.at(pState->nCurrentIndex);
    ARCHIVERECORD result = {};
    result.nStreamOffset = 0;
    result.nStreamSize = pState->nTotalSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, item.sName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pState->nTotalSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, static_cast<qint64>(item.baData.size()));
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, item.sMethod);
    result.mapProperties.insert(FPART_PROP_TYPE, item.sContentType);
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();
    return result;
}

bool XMimeMail::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XMimeMail> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !pDevice || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) || devicesAlias(getDevice(), pDevice))
        return false;

    QPointer<QIODevice> guardedOutput(pDevice);
    const UNPACK_CONTEXT *pContext = static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    if (pContext->listItems.size() != pState->nNumberOfRecords) return false;
    const ITEM &item = pContext->listItems.at(pState->nCurrentIndex);
    const qint64 nSize = item.baData.size();
    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nSize)) return false;

    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, item.sName) && pState->spOutputBudget->isEnforcing()) return false;
        if (!pState->spOutputBudget->debit(nSize) && pState->spOutputBudget->isEnforcing()) return false;
    }

    std::unique_ptr<QIODevice> pStage(createFileBuffer(nSize, pPdStruct));
    if (!pStage || !guardedThis || !guardedOutput || ((nSize > 0) && (pStage->write(item.baData) != nSize)) || !pStage->seek(0) ||
        !isUnpackSourceCurrent(pState, pPdStruct))
        return false;
    const bool bResult = publishUnpackOutput(pStage.get(), guardedOutput.data(), pState, pPdStruct);
    if (bResult && guardedThis) pState->nCurrentOffset = nSize;
    return bResult && guardedThis;
}

bool XMimeMail::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XMimeMail> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    ++pState->nCurrentIndex;
    pState->nCurrentOffset = (pState->nCurrentIndex == pState->nNumberOfRecords) ? pState->nTotalSize : 0;
    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool XMimeMail::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}

QList<XBinary::FPART_PROP> XMimeMail::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD, FPART_PROP_REPORTEDMETHOD, FPART_PROP_TYPE};
}
