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
#include "xmtree.h"

#include <QTimeZone>

#include <limits>
#include <new>

namespace {
const qint32 MTREE_MAX_LINE_SIZE = 1024 * 1024;
const qint32 MTREE_MAX_CONTINUATIONS = 1024;
const qint32 MTREE_MAX_ENTRIES = 100000;
const qint32 MTREE_MAX_LINES = 1000000;
const qint32 MTREE_MAX_NAME_SIZE = 32768;

bool mtreeIsKnownOptionKey(const QByteArray &key)
{
    static const char *const options[] = {"checkfs",      "cksum",        "content", "contents",   "device", "flags",        "gid",      "gname",        "ignore",
                                          "inode",        "link",         "md5",     "md5digest",  "mode",   "nlink",        "nochange", "optional",     "resdevice",
                                          "rmd160",       "rmd160digest", "sha1",    "sha1digest", "sha256", "sha256digest", "sha384",   "sha384digest", "sha512",
                                          "sha512digest", "size",         "tags",    "time",       "type",   "uid",          "uname"};

    for (const char *pOption : options) {
        if (key == pOption) return true;
    }
    return false;
}

bool mtreeIsFlagOption(const QByteArray &key)
{
    return (key == "checkfs") || (key == "ignore") || (key == "nochange") || (key == "optional");
}

bool mtreeIsHex(const QByteArray &value, qint32 nExpectedSize)
{
    if (value.size() != nExpectedSize) return false;
    for (char c : value) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            return false;
        }
    }
    return true;
}
}  // namespace

XMTree::XMTree(QIODevice *pDevice) : XArchive(pDevice)
{
}

XMTree::~XMTree()
{
}

bool XMTree::isValid(PDSTRUCT *pPdStruct)
{
    return _scanArchive(nullptr, nullptr, pPdStruct);
}

bool XMTree::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMTree mtree(pDevice);
    return mtree.isValid(pPdStruct);
}

QString XMTree::getFileFormatExt()
{
    return "mtree";
}

QString XMTree::getFileFormatExtsString()
{
    return "mtree specification (*.mtree)";
}

QString XMTree::getMIMEString()
{
    return "application/x-mtree";
}

XBinary::FT XMTree::getFileType()
{
    return FT_MTREE;
}

XBinary::ENDIAN XMTree::getEndian()
{
    return ENDIAN_UNKNOWN;
}

QList<QString> XMTree::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append("'#mtree'");
    return listResult;
}

XBinary *XMTree::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XMTree(pDevice);
}

bool XMTree::_readPhysicalLine(qint64 nOffset, QByteArray *pLine, qint64 *pNextOffset, bool *pHadNewline, PDSTRUCT *pPdStruct)
{
    QPointer<XMTree> guardedThis(this);
    if (!pLine || !pNextOffset || !pHadNewline || (nOffset < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    pLine->clear();
    *pNextOffset = nOffset;
    *pHadNewline = false;

    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nOffset >= nTotalSize)) return false;

    qint64 nCurrentOffset = nOffset;
    while ((nCurrentOffset < nTotalSize) && (pLine->size() < MTREE_MAX_LINE_SIZE) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nChunkSize = (qint32)qMin<qint64>(qMin<qint64>(0x4000, nTotalSize - nCurrentOffset), MTREE_MAX_LINE_SIZE - pLine->size());
        if (nChunkSize <= 0) return false;

        const QByteArray chunk = read_array_process(nCurrentOffset, nChunkSize, pPdStruct);
        if (!guardedThis || (chunk.size() != nChunkSize)) return false;

        const qint32 nNewline = chunk.indexOf('\n');
        if (nNewline >= 0) {
            pLine->append(chunk.constData(), nNewline);
            *pNextOffset = nCurrentOffset + nNewline + 1;
            *pHadNewline = true;
            if (pLine->endsWith('\r')) pLine->chop(1);
            return XBinary::isPdStructNotCanceled(pPdStruct);
        }

        pLine->append(chunk);
        nCurrentOffset += nChunkSize;
    }

    if ((nCurrentOffset == nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        *pNextOffset = nTotalSize;
        return true;
    }

    return false;
}

bool XMTree::_readLogicalLine(qint64 nOffset, QByteArray *pLine, qint64 *pNextOffset, PDSTRUCT *pPdStruct)
{
    QPointer<XMTree> guardedThis(this);
    if (!pLine || !pNextOffset || (nOffset < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    pLine->clear();
    *pNextOffset = nOffset;
    qint64 nCurrentOffset = nOffset;

    for (qint32 nContinuation = 0; nContinuation <= MTREE_MAX_CONTINUATIONS; nContinuation++) {
        QByteArray physicalLine;
        qint64 nPhysicalNext = 0;
        bool bHadNewline = false;
        const bool bRead = _readPhysicalLine(nCurrentOffset, &physicalLine, &nPhysicalNext, &bHadNewline, pPdStruct);
        if (!guardedThis || !bRead || (nPhysicalNext <= nCurrentOffset) || ((pLine->size() + physicalLine.size()) > MTREE_MAX_LINE_SIZE)) {
            return false;
        }

        qint32 nTrailingBackslashes = 0;
        for (qint32 i = physicalLine.size() - 1; (i >= 0) && (physicalLine.at(i) == '\\'); i--) {
            nTrailingBackslashes++;
        }
        const bool bContinues = (nTrailingBackslashes & 1) != 0;
        if (bContinues) physicalLine.chop(1);
        pLine->append(physicalLine);
        nCurrentOffset = nPhysicalNext;

        if (!bContinues) {
            *pNextOffset = nCurrentOffset;
            return XBinary::isPdStructNotCanceled(pPdStruct);
        }
        if (!bHadNewline) return false;
    }

    return false;
}

QList<QByteArray> XMTree::_splitTokens(const QByteArray &line)
{
    QList<QByteArray> result;
    qint32 nOffset = 0;

    while (nOffset < line.size()) {
        while ((nOffset < line.size()) && ((line.at(nOffset) == ' ') || (line.at(nOffset) == '\t'))) {
            nOffset++;
        }
        if (nOffset >= line.size()) break;

        qint32 nEnd = nOffset;
        while ((nEnd < line.size()) && (line.at(nEnd) != ' ') && (line.at(nEnd) != '\t')) {
            nEnd++;
        }
        result.append(line.mid(nOffset, nEnd - nOffset));
        nOffset = nEnd;
    }

    return result;
}

bool XMTree::_decodeEscapes(const QByteArray &value, QByteArray *pResult)
{
    if (!pResult) return false;
    pResult->clear();
    pResult->reserve(value.size());

    qint32 i = 0;
    while (i < value.size()) {
        const char c = value.at(i++);
        if (c != '\\') {
            pResult->append(c);
            continue;
        }

        if (i >= value.size()) return false;
        const char escaped = value.at(i);

        if ((escaped >= '0') && (escaped <= '3')) {
            if ((i + 2 >= value.size()) || (value.at(i + 1) < '0') || (value.at(i + 1) > '7') || (value.at(i + 2) < '0') || (value.at(i + 2) > '7')) {
                return false;
            }
            const quint8 nValue = (quint8)(((escaped - '0') << 6) | ((value.at(i + 1) - '0') << 3) | (value.at(i + 2) - '0'));
            pResult->append((char)nValue);
            i += 3;
            continue;
        }

        switch (escaped) {
            case 'a':
                pResult->append('\a');
                i++;
                break;
            case 'b':
                pResult->append('\b');
                i++;
                break;
            case 'f':
                pResult->append('\f');
                i++;
                break;
            case 'n':
                pResult->append('\n');
                i++;
                break;
            case 'r':
                pResult->append('\r');
                i++;
                break;
            case 's':
                pResult->append(' ');
                i++;
                break;
            case 't':
                pResult->append('\t');
                i++;
                break;
            case 'v':
                pResult->append('\v');
                i++;
                break;
            case '\\':
                pResult->append('\\');
                i++;
                break;
            default:
                // libarchive preserves the backslash for unknown escapes.
                pResult->append('\\');
                break;
        }
    }

    return true;
}

bool XMTree::_parseOption(const QByteArray &token, MTREE_OPTION *pOption, QByteArray *pKey)
{
    if (!pOption || !pKey || token.isEmpty()) return false;

    const qint32 nEquals = token.indexOf('=');
    const QByteArray key = (nEquals < 0) ? token : token.left(nEquals);
    if (key.isEmpty() || !mtreeIsKnownOptionKey(key)) return false;

    MTREE_OPTION option = {};
    option.bHasValue = nEquals >= 0;
    if (option.bHasValue) option.baValue = token.mid(nEquals + 1);

    if (mtreeIsFlagOption(key)) {
        if (option.bHasValue) return false;
    } else if (!option.bHasValue || option.baValue.isEmpty()) {
        return false;
    }

    *pKey = key;
    *pOption = option;
    return true;
}

bool XMTree::_parseUnsigned(const QByteArray &value, quint32 nBase, quint64 nMaximum, quint64 *pResult)
{
    if (!pResult || value.isEmpty() || ((nBase != 8) && (nBase != 10))) {
        return false;
    }

    quint64 nResult = 0;
    for (char c : value) {
        if ((c < '0') || (c > '9')) return false;
        const quint32 nDigit = (quint32)(c - '0');
        if (nDigit >= nBase) return false;
        if (nResult > ((nMaximum - nDigit) / nBase)) return false;
        nResult = nResult * nBase + nDigit;
    }

    *pResult = nResult;
    return true;
}

bool XMTree::_parseTime(const QByteArray &value, QDateTime *pResult)
{
    if (!pResult || value.isEmpty()) return false;

    const qint32 nDot = value.indexOf('.');
    if ((nDot >= 0) && (value.indexOf('.', nDot + 1) >= 0)) return false;

    const QByteArray secondsText = (nDot < 0) ? value : value.left(nDot);
    const QByteArray fraction = (nDot < 0) ? QByteArray() : value.mid(nDot + 1);
    if (secondsText.isEmpty() || ((nDot >= 0) && fraction.isEmpty())) {
        return false;
    }
    for (char c : fraction) {
        if ((c < '0') || (c > '9')) return false;
    }

    bool bNegative = false;
    qint32 nOffset = 0;
    if (secondsText.at(0) == '-') {
        bNegative = true;
        nOffset = 1;
    }
    if (nOffset >= secondsText.size()) return false;

    const quint64 nPositiveMaximum = (quint64)(std::numeric_limits<qint64>::max)();
    const quint64 nMaximum = bNegative ? nPositiveMaximum + 1 : nPositiveMaximum;
    quint64 nMagnitude = 0;
    for (; nOffset < secondsText.size(); nOffset++) {
        const char c = secondsText.at(nOffset);
        if ((c < '0') || (c > '9')) return false;
        const quint32 nDigit = (quint32)(c - '0');
        if (nMagnitude > ((nMaximum - nDigit) / 10)) return false;
        nMagnitude = nMagnitude * 10 + nDigit;
    }

    qint64 nSeconds = 0;
    if (bNegative) {
        if (nMagnitude == nPositiveMaximum + 1) {
            nSeconds = (std::numeric_limits<qint64>::min)();
        } else {
            nSeconds = -(qint64)nMagnitude;
        }
    } else {
        nSeconds = (qint64)nMagnitude;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    const QDateTime result = QDateTime::fromSecsSinceEpoch(nSeconds, QTimeZone(0));
#else
    if ((nSeconds > ((std::numeric_limits<qint64>::max)() / 1000)) || (nSeconds < ((std::numeric_limits<qint64>::min)() / 1000))) {
        return false;
    }
    const QDateTime result = QDateTime::fromMSecsSinceEpoch(nSeconds * 1000, Qt::UTC);
#endif
    if (!result.isValid()) return false;
    *pResult = result;
    return true;
}

bool XMTree::_validateOption(const QByteArray &key, const MTREE_OPTION &option)
{
    if ((key == "content") || (key == "contents") || (key == "checkfs") || (key == "link") || (key == "device") || (key == "resdevice")) {
        return false;
    }

    if ((key == "ignore") || (key == "nochange") || (key == "optional")) {
        return !option.bHasValue;
    }
    if (!option.bHasValue || option.baValue.isEmpty()) return false;

    quint64 nValue = 0;
    if (key == "type") {
        return (option.baValue == "file") || (option.baValue == "dir");
    }
    if (key == "size") {
        return _parseUnsigned(option.baValue, 10, (quint64)(std::numeric_limits<qint64>::max)(), &nValue) && (nValue == 0);
    }
    if (key == "mode") {
        return _parseUnsigned(option.baValue, 8, 07777, &nValue);
    }
    if ((key == "uid") || (key == "gid") || (key == "nlink")) {
        return _parseUnsigned(option.baValue, 10, (std::numeric_limits<quint32>::max)(), &nValue);
    }
    if ((key == "inode") || (key == "cksum")) {
        return _parseUnsigned(option.baValue, 10, (std::numeric_limits<quint64>::max)(), &nValue);
    }
    if (key == "time") {
        QDateTime dateTime;
        return _parseTime(option.baValue, &dateTime);
    }
    if ((key == "md5") || (key == "md5digest")) {
        return mtreeIsHex(option.baValue, 32);
    }
    if ((key == "rmd160") || (key == "rmd160digest") || (key == "sha1") || (key == "sha1digest")) {
        return mtreeIsHex(option.baValue, 40);
    }
    if ((key == "sha256") || (key == "sha256digest")) {
        return mtreeIsHex(option.baValue, 64);
    }
    if ((key == "sha384") || (key == "sha384digest")) {
        return mtreeIsHex(option.baValue, 96);
    }
    if ((key == "sha512") || (key == "sha512digest")) {
        return mtreeIsHex(option.baValue, 128);
    }

    return (key == "flags") || (key == "gname") || (key == "tags") || (key == "uname");
}

bool XMTree::_makeSafePath(const QByteArray &rawName, const QString &sCurrentDirectory, QString *pPath, bool *pIsFull)
{
    if (!pPath || !pIsFull || rawName.isEmpty() || (rawName.size() > MTREE_MAX_NAME_SIZE)) {
        return false;
    }

    QByteArray decoded;
    if (!_decodeEscapes(rawName, &decoded) || decoded.isEmpty() || decoded.contains('\0')) {
        return false;
    }

    const QString decodedName = QString::fromUtf8(decoded.constData(), decoded.size());
    if (decodedName.toUtf8() != decoded) return false;

    for (QChar c : decodedName) {
        if ((c.unicode() < 0x20) || (c.unicode() == 0x7f) || (c == QLatin1Char('\\'))) {
            return false;
        }
    }

    const bool bFull = (rawName == ".") || rawName.contains('/');
    *pIsFull = bFull;
    if (decodedName == QLatin1String(".")) {
        *pPath = decodedName;
        return true;
    }

    QString sPath;
    if (bFull) {
        sPath = decodedName;
        if (sPath.startsWith(QLatin1String("./"))) sPath.remove(0, 2);
    } else if (sCurrentDirectory.isEmpty()) {
        sPath = decodedName;
    } else {
        sPath = sCurrentDirectory + QLatin1Char('/') + decodedName;
    }

    if (sPath.isEmpty() || sPath.startsWith(QLatin1Char('/'))) return false;
    const QStringList parts = sPath.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &part : parts) {
        if (part.isEmpty() || (part == QLatin1String(".")) || (part == QLatin1String(".."))) {
            return false;
        }
    }

    sPath = sPath.normalized(QString::NormalizationForm_C);
    if (XBinary::fixFileName(sPath) != sPath) return false;

    *pPath = sPath;
    return true;
}

bool XMTree::_scanArchive(QList<MTREE_ENTRY> *pEntries, qint64 *pArchiveEnd, PDSTRUCT *pPdStruct)
{
    QPointer<XMTree> guardedThis(this);
    if (pEntries) pEntries->clear();
    if (pArchiveEnd) *pArchiveEnd = 0;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < 6)) return false;

    QByteArray signatureLine;
    qint64 nOffset = 0;
    const bool bSignatureRead = _readLogicalLine(0, &signatureLine, &nOffset, pPdStruct);
    if (!guardedThis || !bSignatureRead || (signatureLine != "#mtree") || (nOffset <= 0)) {
        return false;
    }

    QMap<QByteArray, MTREE_OPTION> defaults;
    QMap<QByteArray, MTREE_OPTION> rootOptions;
    QMap<QString, qint32> pathIndexes;
    QList<MTREE_ENTRY> resolvedEntries;
    QList<QMap<QByteArray, MTREE_OPTION> > resolvedOptions;
    QString sCurrentDirectory;
    qint32 nEntryCount = 0;
    qint32 nLineCount = 1;

    while ((nOffset < nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (nLineCount >= MTREE_MAX_LINES) {
            if (pEntries) pEntries->clear();
            return false;
        }

        const qint64 nLineOffset = nOffset;
        QByteArray line;
        qint64 nNextOffset = 0;
        const bool bRead = _readLogicalLine(nLineOffset, &line, &nNextOffset, pPdStruct);
        if (!guardedThis || !bRead || (nNextOffset <= nLineOffset) || (nNextOffset > nTotalSize)) {
            if (pEntries) pEntries->clear();
            return false;
        }
        nOffset = nNextOffset;
        nLineCount++;

        qint32 nLeading = 0;
        while ((nLeading < line.size()) && ((line.at(nLeading) == ' ') || (line.at(nLeading) == '\t'))) {
            nLeading++;
        }
        line = line.mid(nLeading);
        if (line.isEmpty() || line.startsWith('#')) continue;

        for (char c : line) {
            const quint8 nCharacter = (quint8)c;
            if (((nCharacter < 0x20) && (nCharacter != '\t')) || (nCharacter == 0x7f)) {
                if (pEntries) pEntries->clear();
                return false;
            }
        }

        const QList<QByteArray> tokens = _splitTokens(line);
        if (tokens.isEmpty()) continue;

        if (tokens.constFirst() == "/set") {
            if (tokens.count() < 2) {
                if (pEntries) pEntries->clear();
                return false;
            }
            for (qint32 i = 1; i < tokens.count(); i++) {
                QByteArray key;
                MTREE_OPTION option = {};
                if (!_parseOption(tokens.at(i), &option, &key) || !_validateOption(key, option)) {
                    if (pEntries) pEntries->clear();
                    return false;
                }
                defaults.insert(key, option);
            }
            continue;
        }

        if (tokens.constFirst() == "/unset") {
            if (tokens.count() < 2) {
                if (pEntries) pEntries->clear();
                return false;
            }
            for (qint32 i = 1; i < tokens.count(); i++) {
                const QByteArray key = tokens.at(i);
                if (key.contains('=')) {
                    if (pEntries) pEntries->clear();
                    return false;
                }
                if (key == "all") {
                    defaults.clear();
                } else if (mtreeIsKnownOptionKey(key)) {
                    defaults.remove(key);
                } else {
                    if (pEntries) pEntries->clear();
                    return false;
                }
            }
            continue;
        }

        const QByteArray rawName = tokens.constFirst();
        QByteArray decodedNameBytes;
        if (!_decodeEscapes(rawName, &decodedNameBytes) || decodedNameBytes.isEmpty() || decodedNameBytes.contains('\0')) {
            if (pEntries) pEntries->clear();
            return false;
        }
        const QString decodedName = QString::fromUtf8(decodedNameBytes.constData(), decodedNameBytes.size());
        if (decodedName.toUtf8() != decodedNameBytes) {
            if (pEntries) pEntries->clear();
            return false;
        }

        if (decodedName == QLatin1String("..")) {
            if (tokens.count() != 1) {
                if (pEntries) pEntries->clear();
                return false;
            }
            const qint32 nSlash = sCurrentDirectory.lastIndexOf('/');
            sCurrentDirectory = (nSlash < 0) ? QString() : sCurrentDirectory.left(nSlash);
            continue;
        }

        QMap<QByteArray, MTREE_OPTION> options = defaults;
        for (qint32 i = 1; i < tokens.count(); i++) {
            QByteArray key;
            MTREE_OPTION option = {};
            if (!_parseOption(tokens.at(i), &option, &key) || !_validateOption(key, option)) {
                if (pEntries) pEntries->clear();
                return false;
            }
            options.insert(key, option);
        }

        QString sPath;
        bool bIsFull = false;
        if (!_makeSafePath(rawName, sCurrentDirectory, &sPath, &bIsFull)) {
            if (pEntries) pEntries->clear();
            return false;
        }

        if (nEntryCount >= MTREE_MAX_ENTRIES) {
            if (pEntries) pEntries->clear();
            return false;
        }
        nEntryCount++;

        if (sPath == QLatin1String(".")) {
            for (auto it = options.constBegin(); it != options.constEnd(); ++it) {
                rootOptions.insert(it.key(), it.value());
            }
            if (!rootOptions.contains("type") || (rootOptions.value("type").baValue != "dir")) {
                if (pEntries) pEntries->clear();
                return false;
            }
            continue;
        }

        const QString sPathKey = sPath.toCaseFolded();
        const qint32 nExistingIndex = pathIndexes.value(sPathKey, -1);
        QMap<QByteArray, MTREE_OPTION> mergedOptions;
        if (nExistingIndex >= 0) {
            if (nExistingIndex >= resolvedOptions.count()) {
                if (pEntries) pEntries->clear();
                return false;
            }
            mergedOptions = resolvedOptions.at(nExistingIndex);
        }
        for (auto it = options.constBegin(); it != options.constEnd(); ++it) {
            mergedOptions.insert(it.key(), it.value());
        }
        if (!mergedOptions.contains("type")) {
            if (pEntries) pEntries->clear();
            return false;
        }
        const bool bIsFolder = mergedOptions.value("type").baValue == "dir";

        MTREE_ENTRY entry = {};
        entry.nHeaderOffset = nLineOffset;
        entry.nHeaderSize = nNextOffset - nLineOffset;
        entry.nStreamOffset = nNextOffset;
        entry.sFileName = sPath;
        entry.bIsFolder = bIsFolder;

        quint64 nValue = 0;
        if (mergedOptions.contains("mode")) {
            if (!_parseUnsigned(mergedOptions.value("mode").baValue, 8, 07777, &nValue)) {
                if (pEntries) pEntries->clear();
                return false;
            }
            entry.bHasMode = true;
            entry.nMode = (quint32)nValue;
        }
        if (mergedOptions.contains("uid")) {
            if (!_parseUnsigned(mergedOptions.value("uid").baValue, 10, (std::numeric_limits<quint32>::max)(), &nValue)) {
                if (pEntries) pEntries->clear();
                return false;
            }
            entry.bHasUID = true;
            entry.nUID = (quint32)nValue;
        }
        if (mergedOptions.contains("gid")) {
            if (!_parseUnsigned(mergedOptions.value("gid").baValue, 10, (std::numeric_limits<quint32>::max)(), &nValue)) {
                if (pEntries) pEntries->clear();
                return false;
            }
            entry.bHasGID = true;
            entry.nGID = (quint32)nValue;
        }
        if (mergedOptions.contains("time")) {
            if (!_parseTime(mergedOptions.value("time").baValue, &entry.modified)) {
                if (pEntries) pEntries->clear();
                return false;
            }
            entry.bHasMTime = true;
        }

        if (nExistingIndex >= 0) {
            resolvedEntries[nExistingIndex] = entry;
            resolvedOptions[nExistingIndex] = mergedOptions;
        } else {
            pathIndexes.insert(sPathKey, resolvedEntries.count());
            resolvedEntries.append(entry);
            resolvedOptions.append(mergedOptions);
        }

        if (!bIsFull && bIsFolder) sCurrentDirectory = sPath;
    }

    const bool bResult = (nOffset == nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct);
    if (!bResult && pEntries) pEntries->clear();
    if (bResult && pEntries) *pEntries = resolvedEntries;
    if (bResult && pArchiveEnd) *pArchiveEnd = nOffset;
    return bResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XMTree::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XMTree::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XMTree> guardedThis(this);
    if (m_bUnpackOperationInProgress) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }

    MTREE_UNPACK_CONTEXT *pOldContext = static_cast<MTREE_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !isPdStructNotCanceled(pPdStruct)) return false;

    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;
    pState->mapUnpackProperties = mapProperties;

    QList<MTREE_ENTRY> listEntries;
    const bool bScanned = _scanArchive(&listEntries, nullptr, pPdStruct);
    if (!guardedThis) return false;
    if (!bScanned || !isPdStructNotCanceled(pPdStruct)) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    const qint64 nTotalSize = getSize();
    if (!guardedThis) return false;

    MTREE_UNPACK_CONTEXT *pContext = new (std::nothrow) MTREE_UNPACK_CONTEXT;
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
    pState->nCurrentOffset = listEntries.isEmpty() ? nTotalSize : listEntries.constFirst().nHeaderOffset;
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

XBinary::ARCHIVERECORD XMTree::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XMTree> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    if (!pState || !pState->pContext) return result;

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return result;

    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nTotalSize != nCurrentSize)) return result;

    MTREE_UNPACK_CONTEXT *pContext = static_cast<MTREE_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->listEntries.count()) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pContext->listEntries.count())) {
        return result;
    }

    const qint32 nExpectedIndex = pState->nCurrentIndex;
    const qint32 nExpectedCount = pState->nNumberOfRecords;
    const MTREE_ENTRY entry = pContext->listEntries.at(nExpectedIndex);

    QList<MTREE_ENTRY> verifiedEntries;
    const bool bRescanned = _scanArchive(&verifiedEntries, nullptr, pPdStruct);
    if (!guardedThis || !bRescanned || (pState->pContext != pContext) || (pState->nCurrentIndex != nExpectedIndex) || (pState->nNumberOfRecords != nExpectedCount) ||
        (verifiedEntries.count() != nExpectedCount)) {
        return ARCHIVERECORD();
    }

    const MTREE_ENTRY verified = verifiedEntries.at(nExpectedIndex);
    const bool bEntryMatches = (verified.nHeaderOffset == entry.nHeaderOffset) && (verified.nHeaderSize == entry.nHeaderSize) &&
                               (verified.nStreamOffset == entry.nStreamOffset) && (verified.sFileName == entry.sFileName) && (verified.bIsFolder == entry.bIsFolder) &&
                               (verified.bHasMode == entry.bHasMode) && (verified.nMode == entry.nMode) && (verified.bHasUID == entry.bHasUID) &&
                               (verified.nUID == entry.nUID) && (verified.bHasGID == entry.bHasGID) && (verified.nGID == entry.nGID) &&
                               (verified.bHasMTime == entry.bHasMTime) && (verified.modified == entry.modified);
    if (!bEntryMatches) return ARCHIVERECORD();

    const bool bSourceStillCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceStillCurrent || (pState->pContext != pContext) || (pState->nCurrentIndex != nExpectedIndex) ||
        (pState->nNumberOfRecords != nExpectedCount)) {
        return ARCHIVERECORD();
    }

    if ((entry.nHeaderOffset < 0) || (entry.nHeaderSize <= 0) || (entry.nHeaderOffset > nCurrentSize) || (entry.nHeaderSize > (nCurrentSize - entry.nHeaderOffset)) ||
        (entry.nStreamOffset < 0) || (entry.nStreamOffset > nCurrentSize)) {
        return result;
    }

    result.nStreamOffset = entry.nStreamOffset;
    result.nStreamSize = 0;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, (qint64)0);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_HEADER_OFFSET, entry.nHeaderOffset);
    result.mapProperties.insert(FPART_PROP_HEADER_SIZE, entry.nHeaderSize);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, entry.bIsFolder);
    if (entry.bHasMode) {
        result.mapProperties.insert(FPART_PROP_FILEMODE, entry.nMode);
    }
    if (entry.bHasUID) {
        result.mapProperties.insert(FPART_PROP_UID, entry.nUID);
    }
    if (entry.bHasGID) {
        result.mapProperties.insert(FPART_PROP_GID, entry.nGID);
    }
    if (entry.bHasMTime) {
        result.mapProperties.insert(FPART_PROP_MTIME, entry.modified);
    }

    return result;
}

bool XMTree::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XMTree> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext) {
        return false;
    }

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return false;

    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nTotalSize != nCurrentSize)) return false;

    MTREE_UNPACK_CONTEXT *pContext = static_cast<MTREE_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->listEntries.count()) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    pState->nCurrentIndex++;
    pContext->nCurrentRecord = pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listEntries.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }

    pState->nCurrentOffset = pState->nTotalSize;
    return false;
}

bool XMTree::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    Q_UNUSED(pPdStruct)

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }

    MTREE_UNPACK_CONTEXT *pContext = static_cast<MTREE_UNPACK_CONTEXT *>(pState->pContext);
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

QList<XBinary::FPART_PROP> XMTree::getAvailableFPARTProperties()
{
    QList<FPART_PROP> listResult;
    listResult.append(FPART_PROP_ORIGINALNAME);
    listResult.append(FPART_PROP_UNCOMPRESSEDSIZE);
    listResult.append(FPART_PROP_STREAMOFFSET);
    listResult.append(FPART_PROP_STREAMSIZE);
    listResult.append(FPART_PROP_ISFOLDER);
    listResult.append(FPART_PROP_FILEMODE);
    listResult.append(FPART_PROP_UID);
    listResult.append(FPART_PROP_GID);
    listResult.append(FPART_PROP_MTIME);
    return listResult;
}

bool XMTree::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XMTree> guardedThis(this);
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

void *XMTree::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XMTree> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;
    return &guardedThis->m_internalInfo;
}

void XMTree::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
