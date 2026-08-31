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
#include "xshar.h"

#include <QBuffer>
#include <QPointer>

#include <new>

namespace {
const qint64 SHAR_MAX_PREAMBLE = Q_INT64_C(16) * 1024;
const qint64 SHAR_MAX_LINE = Q_INT64_C(1024) * 1024;
const qint32 SHAR_MAX_ENTRIES = 100000;
const qint32 SHAR_MAX_START_LINE = 4096;
const qint64 SHAR_MAX_DECODED = Q_INT64_C(512) * 1024 * 1024;
const qint32 SHAR_SIZECHECK_LINES = 40;

bool sharIsBlank(char c)
{
    return (c == ' ') || (c == '\t');
}

QString sharDiagnosticTarget(const QByteArray &baRawTarget)
{
    QByteArray baEncoded = baRawTarget.toPercentEncoding();
    const qint32 nMaximumLength = 512;
    if (baEncoded.size() > nMaximumLength) {
        baEncoded = baEncoded.left(nMaximumLength - 3) + QByteArrayLiteral("...");
    }
    return QString::fromLatin1(baEncoded);
}
}  // namespace

XSHAR::XSHAR(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSHAR::~XSHAR()
{
}

QByteArray XSHAR::_stripTrailingCR(const QByteArray &line)
{
    if (line.endsWith('\r')) return line.left(line.size() - 1);
    return line;
}

bool XSHAR::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XSHAR> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());
    if (!guardedDevice || guardedDevice->isSequential()) return false;
    const bool bResult = _scanArchive(nullptr, pPdStruct);
    return guardedThis && bResult;
}

bool XSHAR::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSHAR shar(pDevice);
    return shar.isValid(pPdStruct);
}

QString XSHAR::getFileFormatExt()
{
    return "shar";
}

QString XSHAR::getFileFormatExtsString()
{
    return "Shell archive (*.shar *.sh)";
}

QString XSHAR::getMIMEString()
{
    return "application/x-shar";
}

XBinary::FT XSHAR::getFileType()
{
    return FT_SHAR;
}

XBinary::MODE XSHAR::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSHAR::getEndian()
{
    return ENDIAN_UNKNOWN;
}

QList<QString> XSHAR::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append("'# This is a shell archive'");
    listResult.append("'#! /bin/sh'");
    return listResult;
}

XBinary *XSHAR::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XSHAR(pDevice);
}

bool XSHAR::_readPhysicalLine(qint64 nOffset, QByteArray *pLine, qint64 *pNextOffset, bool *pHadNewline, bool *pTooLong, PDSTRUCT *pPdStruct)
{
    QPointer<XSHAR> guardedThis(this);
    if (!pLine || !pNextOffset || !pHadNewline || !pTooLong || (nOffset < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    pLine->clear();
    *pNextOffset = nOffset;
    *pHadNewline = false;
    *pTooLong = false;

    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nOffset >= nTotalSize)) return false;

    qint64 nCurrentOffset = nOffset;
    while ((nCurrentOffset < nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nChunkSize = (qint32)qMin<qint64>(0x4000, nTotalSize - nCurrentOffset);
        if (nChunkSize <= 0) return false;

        const QByteArray chunk = read_array_process(nCurrentOffset, nChunkSize, pPdStruct);
        if (!guardedThis || (chunk.size() != nChunkSize)) return false;

        const qint32 nNewline = chunk.indexOf('\n');
        if (nNewline >= 0) {
            if ((pLine->size() + (qint64)nNewline) > SHAR_MAX_LINE) {
                *pTooLong = true;
                return false;
            }
            pLine->append(chunk.constData(), nNewline);
            *pNextOffset = nCurrentOffset + nNewline + 1;
            *pHadNewline = true;
            *pLine = _stripTrailingCR(*pLine);
            return XBinary::isPdStructNotCanceled(pPdStruct);
        }

        if ((pLine->size() + (qint64)chunk.size()) > SHAR_MAX_LINE) {
            *pTooLong = true;
            return false;
        }
        pLine->append(chunk);
        nCurrentOffset += nChunkSize;
    }

    if ((nCurrentOffset == nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        *pNextOffset = nTotalSize;
        *pHadNewline = false;
        *pLine = _stripTrailingCR(*pLine);
        return true;
    }

    return false;
}

QList<XSHAR::SHAR_TOKEN> XSHAR::_tokenize(const QByteArray &line)
{
    QList<SHAR_TOKEN> result;
    const qint32 n = line.size();
    qint32 i = 0;

    while (i < n) {
        const char c = line.at(i);
        if (sharIsBlank(c)) {
            i++;
            continue;
        }
        if (c == '<') {
            if ((i + 1 < n) && (line.at(i + 1) == '<')) {
                if ((i + 2 < n) && (line.at(i + 2) == '-')) {
                    result.append({TOKEN_HEREDOC_DASH, QByteArray()});
                    i += 3;
                } else {
                    result.append({TOKEN_HEREDOC, QByteArray()});
                    i += 2;
                }
            } else {
                result.append({TOKEN_OTHER, QByteArray("<")});
                i++;
            }
            continue;
        }
        if (c == '>') {
            if ((i + 1 < n) && (line.at(i + 1) == '>')) {
                result.append({TOKEN_REDIR_APPEND, QByteArray()});
                i += 2;
            } else {
                result.append({TOKEN_REDIR_OUT, QByteArray()});
                i++;
            }
            continue;
        }
        if (c == '&') {
            if ((i + 1 < n) && (line.at(i + 1) == '&')) {
                result.append({TOKEN_AND, QByteArray()});
                i += 2;
            } else {
                result.append({TOKEN_OTHER, QByteArray("&")});
                i++;
            }
            continue;
        }
        if (c == ';') {
            result.append({TOKEN_SEMI, QByteArray()});
            i++;
            continue;
        }
        if (c == '|') {
            result.append({TOKEN_OTHER, QByteArray("|")});
            i++;
            continue;
        }

        // A word: preserve exact source bytes, honouring quotes and backslash.
        QByteArray raw;
        while (i < n) {
            const char w = line.at(i);
            if (sharIsBlank(w) || (w == '<') || (w == '>') || (w == '&') || (w == ';') || (w == '|')) {
                break;
            }
            if (w == '\\') {
                raw.append(w);
                i++;
                if (i < n) {
                    raw.append(line.at(i));
                    i++;
                }
                continue;
            }
            if (w == '\'') {
                raw.append(w);
                i++;
                while ((i < n) && (line.at(i) != '\'')) {
                    raw.append(line.at(i));
                    i++;
                }
                if (i < n) {
                    raw.append('\'');
                    i++;
                }
                continue;
            }
            if (w == '"') {
                raw.append(w);
                i++;
                while ((i < n) && (line.at(i) != '"')) {
                    if ((line.at(i) == '\\') && (i + 1 < n)) {
                        raw.append(line.at(i));
                        i++;
                        raw.append(line.at(i));
                        i++;
                        continue;
                    }
                    raw.append(line.at(i));
                    i++;
                }
                if (i < n) {
                    raw.append('"');
                    i++;
                }
                continue;
            }
            raw.append(w);
            i++;
        }
        result.append({TOKEN_WORD, raw});
    }

    return result;
}

QByteArray XSHAR::_unquoteWord(const QByteArray &raw)
{
    if ((raw.size() >= 2) && (raw.at(0) == '\'') && raw.endsWith('\'')) {
        return raw.mid(1, raw.size() - 2);
    }
    if ((raw.size() >= 2) && (raw.at(0) == '"') && raw.endsWith('"')) {
        return raw.mid(1, raw.size() - 2);
    }
    return raw;
}

QByteArray XSHAR::_unquoteDelimiter(const QByteArray &raw)
{
    if (raw.startsWith('\\')) {
        return raw.mid(1);
    }
    return _unquoteWord(raw);
}

bool XSHAR::_parseSedProgram(const QByteArray &program, QByteArray *pStripPrefix)
{
    if (!pStripPrefix) return false;
    if (!program.startsWith("s/^") || !program.endsWith("//") || (program.size() < 5)) {
        return false;
    }

    const QByteArray middle = program.mid(3, program.size() - 5);
    if (middle.contains('/')) return false;

    if (middle == "X") {
        *pStripPrefix = QByteArray("X");
    } else if (middle == "\\X") {
        *pStripPrefix = QByteArray("X");
    } else if (middle == QByteArray("\tX")) {
        *pStripPrefix = QByteArray("\tX");
    } else if (middle == "@") {
        *pStripPrefix = QByteArray("@");
    } else {
        return false;
    }

    return true;
}

bool XSHAR::_parseMemberStart(const QByteArray &line, SHAR_MEMBER_START *pResult)
{
    if (!pResult) return false;
    *pResult = SHAR_MEMBER_START();
    pResult->bIsMember = false;

    if ((line.size() > SHAR_MAX_START_LINE) || line.isEmpty()) return false;

    const QList<SHAR_TOKEN> tokens = _tokenize(line);
    if (tokens.isEmpty() || (tokens.constFirst().type != TOKEN_WORD)) return false;

    const QByteArray command = tokens.constFirst().baRaw;

    // Redirect-bearing member: cat/sed with exactly one ">" redirect and one
    // here-document (order of the two operators is not fixed).
    if ((command == "cat") || (command == "sed")) {
        qint32 idx = 1;
        MEMBER_KIND kind = MEMBER_KIND_CAT;
        QByteArray baStripPrefix;

        if (command == "sed") {
            kind = MEMBER_KIND_SED;
            if ((idx < tokens.count()) && (tokens.at(idx).type == TOKEN_WORD) && (_unquoteWord(tokens.at(idx).baRaw) == "-e")) {
                idx++;
            }
            if ((idx >= tokens.count()) || (tokens.at(idx).type != TOKEN_WORD) || !_parseSedProgram(_unquoteWord(tokens.at(idx).baRaw), &baStripPrefix)) {
                return false;
            }
            idx++;
        }

        qint32 nRedirects = 0;
        qint32 nHeredocs = 0;
        QByteArray baRawName;
        QByteArray baDelim;
        bool bDash = false;
        bool bClean = true;

        for (; idx < tokens.count(); idx++) {
            const SHAR_TOKEN &token = tokens.at(idx);
            if (token.type == TOKEN_REDIR_OUT) {
                if ((idx + 1 >= tokens.count()) || (tokens.at(idx + 1).type != TOKEN_WORD)) {
                    bClean = false;
                    break;
                }
                baRawName = tokens.at(idx + 1).baRaw;
                nRedirects++;
                idx++;
            } else if ((token.type == TOKEN_HEREDOC) || (token.type == TOKEN_HEREDOC_DASH)) {
                if ((idx + 1 >= tokens.count()) || (tokens.at(idx + 1).type != TOKEN_WORD)) {
                    bClean = false;
                    break;
                }
                bDash = (token.type == TOKEN_HEREDOC_DASH);
                baDelim = _unquoteDelimiter(tokens.at(idx + 1).baRaw);
                nHeredocs++;
                idx++;
            } else if ((token.type == TOKEN_AND) || (token.type == TOKEN_SEMI)) {
                // GNU shar terminates the member-start line with "&&" or ";".
            } else {
                bClean = false;
                break;
            }
        }

        if (bClean && (nRedirects == 1) && (nHeredocs == 1) && !baDelim.isEmpty()) {
            pResult->bIsMember = true;
            pResult->kind = kind;
            pResult->baStripPrefix = baStripPrefix;
            pResult->baDelim = baDelim;
            pResult->bDash = bDash;
            pResult->baRawName = baRawName;
            return true;
        }
        // Fall through: a "cat <<EOF" message here-document has no redirect and
        // is examined as a possible uuencoded payload below.
    }

    // uu-in-here-document member: a here-document with NO ">" redirect.  The
    // command word is unrestricted (e.g. "$unpacker <<D").  Confirmation that
    // the body is uuencoded is left to the caller's begin-line peek.
    {
        qint32 nHeredocs = 0;
        QByteArray baDelim;
        bool bDash = false;
        bool bClean = true;

        for (qint32 idx = 1; idx < tokens.count(); idx++) {
            const SHAR_TOKEN &token = tokens.at(idx);
            if ((token.type == TOKEN_HEREDOC) || (token.type == TOKEN_HEREDOC_DASH)) {
                if ((idx + 1 >= tokens.count()) || (tokens.at(idx + 1).type != TOKEN_WORD)) {
                    bClean = false;
                    break;
                }
                bDash = (token.type == TOKEN_HEREDOC_DASH);
                baDelim = _unquoteDelimiter(tokens.at(idx + 1).baRaw);
                nHeredocs++;
                idx++;
            } else if (token.type == TOKEN_WORD) {
                // Command arguments preceding the here-document are permitted.
            } else if ((token.type == TOKEN_AND) || (token.type == TOKEN_SEMI)) {
                // Trailing operators are permitted.
            } else {
                // A ">"/">>" redirect or other operator disqualifies the uu form.
                bClean = false;
                break;
            }
        }

        if (bClean && (nHeredocs == 1) && !baDelim.isEmpty()) {
            pResult->bIsMember = true;
            pResult->kind = MEMBER_KIND_UU;
            pResult->baDelim = baDelim;
            pResult->bDash = bDash;
            return true;
        }
    }

    return false;
}

bool XSHAR::_makeSafeName(const QByteArray &rawTarget, QString *pName)
{
    if (!pName) return false;

    QByteArray name = rawTarget;
    if ((name.size() >= 2) && (((name.at(0) == '\'') && name.endsWith('\'')) || ((name.at(0) == '"') && name.endsWith('"')))) {
        name = name.mid(1, name.size() - 2);
    }

    if (name.isEmpty() || (name.size() > 255)) return false;

    for (char c : name) {
        const quint8 nValue = (quint8)c;
        if (nValue < 0x20) return false;
        if ((c == '$') || (c == '`') || (c == '\\')) return false;
    }

    if (name.startsWith('/')) return false;

    const QList<QByteArray> parts = name.split('/');
    for (const QByteArray &part : parts) {
        if (part.isEmpty() || (part == "..")) return false;
    }

    *pName = QString::fromUtf8(name.constData(), name.size());
    return true;
}

bool XSHAR::_parseUuBegin(const QByteArray &line, QString *pName)
{
    if (!pName) return false;
    if (!line.startsWith("begin ")) return false;

    const QByteArray rest = line.mid(6);
    qint32 nDigits = 0;
    while ((nDigits < rest.size()) && (rest.at(nDigits) >= '0') && (rest.at(nDigits) <= '7')) {
        nDigits++;
    }
    if ((nDigits < 3) || (nDigits > 4) || (nDigits >= rest.size()) || (rest.at(nDigits) != ' ')) {
        return false;
    }

    const QByteArray name = rest.mid(nDigits + 1);
    return _makeSafeName(name, pName);
}

bool XSHAR::_uudecodeBody(const QList<QByteArray> &listLines, qint64 nCap, QByteArray *pOutput, qint64 *pnSize)
{
    // listLines.at(0) is the validated "begin" header; data follows.
    if (listLines.isEmpty() || (nCap < 0)) return false;

    QByteArray output;
    bool bSawZeroLine = false;
    bool bComplete = false;

    for (qint32 nLine = 1; nLine < listLines.count(); nLine++) {
        const QByteArray &line = listLines.at(nLine);
        if (bSawZeroLine) {
            bComplete = (line == "end");
            break;
        }
        if (line.isEmpty()) break;

        const quint8 nFirst = (quint8)line.at(0);
        if ((nFirst < 0x20) || (nFirst > 0x60)) break;
        const qint32 nDecoded = (nFirst - 0x20) & 0x3f;
        if (nDecoded > 45) break;
        if (nDecoded == 0) {
            bSawZeroLine = true;
            continue;
        }

        const qint32 nEncoded = ((nDecoded + 2) / 3) * 4;
        if (line.size() < (1 + nEncoded)) break;

        qint32 nWritten = 0;
        bool bLineValid = true;
        for (qint32 i = 0; (i < nEncoded) && (nWritten < nDecoded); i += 4) {
            quint8 values[4] = {};
            for (qint32 j = 0; j < 4; j++) {
                const quint8 c = (quint8)line.at(1 + i + j);
                if ((c < 0x20) || (c > 0x60)) {
                    bLineValid = false;
                    break;
                }
                values[j] = (c - 0x20) & 0x3f;
            }
            if (!bLineValid) break;
            const char decoded[3] = {(char)((values[0] << 2) | (values[1] >> 4)), (char)((values[1] << 4) | (values[2] >> 2)), (char)((values[2] << 6) | values[3])};
            for (qint32 j = 0; (j < 3) && (nWritten < nDecoded); j++, nWritten++) {
                if ((qint64)output.size() >= nCap) return false;
                output.append(decoded[j]);
            }
        }
        if (!bLineValid) break;
    }

    if (!bComplete) return false;

    if (pOutput) *pOutput = output;
    if (pnSize) *pnSize = output.size();
    return true;
}

bool XSHAR::_matchSizeTest(const QByteArray &line, const QString &sName, qint64 *pnSize)
{
    if (!pnSize) return false;
    if (!line.contains("-ne") || !line.contains("wc -c")) return false;
    if (!line.contains(sName.toUtf8())) return false;

    const qint32 nTest = line.indexOf("test");
    if (nTest < 0) return false;

    qint32 j = nTest + 4;
    while ((j < line.size()) && sharIsBlank(line.at(j))) j++;
    const qint32 nStart = j;
    while ((j < line.size()) && (line.at(j) >= '0') && (line.at(j) <= '9')) j++;
    if (j == nStart) return false;

    bool bOk = false;
    const qint64 nValue = line.mid(nStart, j - nStart).toLongLong(&bOk);
    if (!bOk) return false;

    qint32 k = j;
    while ((k < line.size()) && sharIsBlank(line.at(k))) k++;
    if (!line.mid(k).startsWith("-ne")) return false;

    *pnSize = nValue;
    return true;
}

bool XSHAR::_readMemberBody(qint64 nBodyOffset, const QByteArray &baDelim, bool bDash, MEMBER_KIND kind, const QByteArray &baStripPrefix, qint64 nCap,
                            qint64 *pnBodyEndOffset, qint64 *pnNextOffset, qint64 *pnDecodedSize, QByteArray *pTextOutput, QList<QByteArray> *pUuLines,
                            bool *pbTerminated, bool *pbTooLong, bool *pbOversize, PDSTRUCT *pPdStruct)
{
    QPointer<XSHAR> guardedThis(this);
    if (!pnBodyEndOffset || !pnNextOffset || !pnDecodedSize || !pbTerminated || !pbTooLong || !pbOversize) return false;

    *pnBodyEndOffset = nBodyOffset;
    *pnNextOffset = nBodyOffset;
    *pnDecodedSize = 0;
    *pbTerminated = false;
    *pbTooLong = false;
    *pbOversize = false;
    if (pTextOutput) pTextOutput->clear();
    if (pUuLines) pUuLines->clear();

    const qint64 nTotalSize = getSize();
    if (!guardedThis) return false;

    qint64 nOffset = nBodyOffset;
    qint64 nDecodedSize = 0;

    while ((nOffset < nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        QByteArray line;
        qint64 nNextOffset = 0;
        bool bHadNewline = false;
        bool bTooLong = false;
        const bool bRead = _readPhysicalLine(nOffset, &line, &nNextOffset, &bHadNewline, &bTooLong, pPdStruct);
        if (!guardedThis) return false;
        if (bTooLong) {
            *pbTooLong = true;
            return false;
        }
        if (!bRead || (nNextOffset <= nOffset)) {
            // Truncated before the terminator: report the point reached.
            *pnBodyEndOffset = nOffset;
            *pnNextOffset = nOffset;
            if (kind != MEMBER_KIND_UU) *pnDecodedSize = nDecodedSize;
            return guardedThis && XBinary::isPdStructNotCanceled(pPdStruct);
        }

        QByteArray compare = line;
        if (bDash) {
            qint32 nLeading = 0;
            while ((nLeading < compare.size()) && (compare.at(nLeading) == '\t')) nLeading++;
            compare = compare.mid(nLeading);
        }
        if (compare == baDelim) {
            *pbTerminated = true;
            *pnBodyEndOffset = nOffset;
            *pnNextOffset = nNextOffset;
            if (kind != MEMBER_KIND_UU) *pnDecodedSize = nDecodedSize;
            return true;
        }

        if (kind == MEMBER_KIND_UU) {
            if (pUuLines) pUuLines->append(line);
        } else {
            QByteArray baEmit = line;
            if ((kind == MEMBER_KIND_SED) && !baStripPrefix.isEmpty() && baEmit.startsWith(baStripPrefix)) {
                baEmit = baEmit.mid(baStripPrefix.size());
            }
            nDecodedSize += (qint64)baEmit.size() + 1;
            if (nDecodedSize > nCap) {
                *pbOversize = true;
                return false;
            }
            if (pTextOutput) {
                pTextOutput->append(baEmit);
                pTextOutput->append('\n');
            }
        }

        nOffset = nNextOffset;
    }

    // Reached end of device without a terminator (truncation salvage).
    *pnBodyEndOffset = nOffset;
    *pnNextOffset = nOffset;
    if (kind != MEMBER_KIND_UU) *pnDecodedSize = nDecodedSize;
    return guardedThis && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XSHAR::_findMarker(PDSTRUCT *pPdStruct)
{
    QPointer<XSHAR> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize <= 0)) return false;

    qint64 nOffset = 0;
    while ((nOffset < nTotalSize) && (nOffset < SHAR_MAX_PREAMBLE) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        QByteArray line;
        qint64 nNextOffset = 0;
        bool bHadNewline = false;
        bool bTooLong = false;
        const bool bRead = _readPhysicalLine(nOffset, &line, &nNextOffset, &bHadNewline, &bTooLong, pPdStruct);
        if (!guardedThis || bTooLong) return false;
        if (!bRead || (nNextOffset <= nOffset)) break;

        qint32 nLeading = 0;
        while ((nLeading < line.size()) && sharIsBlank(line.at(nLeading))) nLeading++;
        const QByteArray trimmed = line.mid(nLeading);
        if (trimmed.startsWith('#')) {
            const QByteArray lower = trimmed.toLower();
            if (lower.contains("shell archive") || lower.contains("is a shar")) {
                return true;
            }
        }

        nOffset = nNextOffset;
    }

    return false;
}

bool XSHAR::_scanArchive(QList<SHAR_ENTRY> *pEntries, PDSTRUCT *pPdStruct)
{
    QPointer<XSHAR> guardedThis(this);
    if (pEntries) pEntries->clear();
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize <= 0)) return false;

    if (!_findMarker(pPdStruct) || !guardedThis) return false;

    QList<SHAR_ENTRY> entries;
    qint64 nOffset = 0;
    qint32 nPendingIndex = -1;
    QString sPendingName;
    qint32 nPendingLines = 0;
    bool bRejectWhole = false;

    while ((nOffset < nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        QByteArray line;
        qint64 nNextOffset = 0;
        bool bHadNewline = false;
        bool bTooLong = false;
        const bool bRead = _readPhysicalLine(nOffset, &line, &nNextOffset, &bHadNewline, &bTooLong, pPdStruct);
        if (!guardedThis) return false;
        if (bTooLong) {
            bRejectWhole = true;
            break;
        }
        if (!bRead || (nNextOffset <= nOffset)) break;

        SHAR_MEMBER_START memberStart;
        if (_parseMemberStart(line, &memberStart)) {
            nPendingIndex = -1;  // A new member-start ends the size-check window.

            const qint64 nBodyOffset = nNextOffset;

            if (memberStart.kind == MEMBER_KIND_UU) {
                QByteArray firstBody;
                qint64 nFirstNext = 0;
                bool bFirstNewline = false;
                bool bFirstTooLong = false;
                const bool bPeek = _readPhysicalLine(nBodyOffset, &firstBody, &nFirstNext, &bFirstNewline, &bFirstTooLong, pPdStruct);
                if (!guardedThis) return false;
                if (bFirstTooLong) {
                    bRejectWhole = true;
                    break;
                }
                QString uuProbe;
                if (!bPeek || !_parseUuBegin(firstBody, &uuProbe)) {
                    // Not a uuencoded payload: treat as an ordinary line.
                    nOffset = nNextOffset;
                    continue;
                }
            }

            QList<QByteArray> uuLines;
            qint64 nBodyEndOffset = 0;
            qint64 nAfterTerminator = 0;
            qint64 nDecodedSize = 0;
            bool bTerminated = false;
            bool bBodyTooLong = false;
            bool bOversize = false;
            const bool bBody = _readMemberBody(nBodyOffset, memberStart.baDelim, memberStart.bDash, memberStart.kind, memberStart.baStripPrefix, SHAR_MAX_DECODED,
                                               &nBodyEndOffset, &nAfterTerminator, &nDecodedSize, nullptr, (memberStart.kind == MEMBER_KIND_UU) ? &uuLines : nullptr,
                                               &bTerminated, &bBodyTooLong, &bOversize, pPdStruct);
            if (!guardedThis) return false;
            if (bBodyTooLong) {
                bRejectWhole = true;
                break;
            }
            if (!bBody) {
                // An oversized member or a read failure stops the scan; members
                // already fully scanned are kept as salvage.
                break;
            }
            if (!bTerminated) {
                // Truncation mid-member: keep members already scanned, stop.
                break;
            }

            if (entries.count() >= SHAR_MAX_ENTRIES) break;

            if (memberStart.kind == MEMBER_KIND_UU) {
                QString uuName;
                qint64 uuSize = 0;
                if (_parseUuBegin(uuLines.value(0), &uuName) && _uudecodeBody(uuLines, SHAR_MAX_DECODED, nullptr, &uuSize)) {
                    SHAR_ENTRY entry = {};
                    entry.kind = MEMBER_KIND_UU;
                    entry.sFileName = uuName;
                    entry.baRawTarget = memberStart.baRawName;
                    entry.baDelim = memberStart.baDelim;
                    entry.bDash = memberStart.bDash;
                    entry.nHeaderOffset = nOffset;
                    entry.nHeaderSize = nNextOffset - nOffset;
                    entry.nBodyOffset = nBodyOffset;
                    entry.nBodyEndOffset = nBodyEndOffset;
                    entry.nDecodedSize = uuSize;
                    entry.nDeclaredSize = -1;
                    entries.append(entry);
                    nPendingIndex = entries.count() - 1;
                    sPendingName = uuName;
                    nPendingLines = SHAR_SIZECHECK_LINES;
                }
            } else {
                QString name;
                if (_makeSafeName(memberStart.baRawName, &name)) {
                    SHAR_ENTRY entry = {};
                    entry.kind = memberStart.kind;
                    entry.sFileName = name;
                    entry.baRawTarget = memberStart.baRawName;
                    entry.baStripPrefix = memberStart.baStripPrefix;
                    entry.baDelim = memberStart.baDelim;
                    entry.bDash = memberStart.bDash;
                    entry.nHeaderOffset = nOffset;
                    entry.nHeaderSize = nNextOffset - nOffset;
                    entry.nBodyOffset = nBodyOffset;
                    entry.nBodyEndOffset = nBodyEndOffset;
                    entry.nDecodedSize = nDecodedSize;
                    entry.nDeclaredSize = -1;
                    entries.append(entry);
                    nPendingIndex = entries.count() - 1;
                    sPendingName = name;
                    nPendingLines = SHAR_SIZECHECK_LINES;
                } else {
                    // Keep shell-expanded or otherwise unsafe redirect targets
                    // visible.  They cannot be mapped to a safe output path,
                    // but silently dropping them makes a partial extraction
                    // look complete.
                    SHAR_ENTRY entry = {};
                    entry.kind = memberStart.kind;
                    entry.sFileName = QStringLiteral("__unsafe_target_%1__").arg(static_cast<qulonglong>(nOffset), 0, 16);
                    entry.baRawTarget = memberStart.baRawName;
                    entry.baStripPrefix = memberStart.baStripPrefix;
                    entry.baDelim = memberStart.baDelim;
                    entry.bDash = memberStart.bDash;
                    entry.bUnsafeTarget = true;
                    entry.nHeaderOffset = nOffset;
                    entry.nHeaderSize = nNextOffset - nOffset;
                    entry.nBodyOffset = nBodyOffset;
                    entry.nBodyEndOffset = nBodyEndOffset;
                    entry.nDecodedSize = nDecodedSize;
                    entry.nDeclaredSize = -1;
                    entries.append(entry);
                }
            }

            nOffset = nAfterTerminator;
            continue;
        }

        if ((nPendingIndex >= 0) && (nPendingLines > 0)) {
            qint64 nDeclaredSize = -1;
            if (_matchSizeTest(line, sPendingName, &nDeclaredSize)) {
                if ((nPendingIndex < entries.count())) {
                    entries[nPendingIndex].nDeclaredSize = nDeclaredSize;
                }
                nPendingIndex = -1;
            } else {
                nPendingLines--;
                if (nPendingLines <= 0) nPendingIndex = -1;
            }
        }

        nOffset = nNextOffset;
    }

    if (bRejectWhole || !guardedThis || entries.isEmpty()) {
        return false;
    }

    if (pEntries) *pEntries = entries;
    return true;
}

bool XSHAR::_decodeMember(const SHAR_ENTRY &entry, qint64 nCap, QByteArray *pOutput, PDSTRUCT *pPdStruct)
{
    QPointer<XSHAR> guardedThis(this);
    if (!pOutput) return false;
    pOutput->clear();

    qint64 nBodyEndOffset = 0;
    qint64 nAfterTerminator = 0;
    qint64 nDecodedSize = 0;
    bool bTerminated = false;
    bool bTooLong = false;
    bool bOversize = false;

    if (entry.kind == MEMBER_KIND_UU) {
        QList<QByteArray> uuLines;
        const bool bBody = _readMemberBody(entry.nBodyOffset, entry.baDelim, entry.bDash, MEMBER_KIND_UU, QByteArray(), nCap, &nBodyEndOffset, &nAfterTerminator,
                                           &nDecodedSize, nullptr, &uuLines, &bTerminated, &bTooLong, &bOversize, pPdStruct);
        if (!guardedThis || !bBody || bTooLong || bOversize || !bTerminated) return false;
        qint64 nSize = 0;
        if (!_uudecodeBody(uuLines, nCap, pOutput, &nSize)) return false;
    } else {
        const bool bBody = _readMemberBody(entry.nBodyOffset, entry.baDelim, entry.bDash, entry.kind, entry.baStripPrefix, nCap, &nBodyEndOffset, &nAfterTerminator,
                                           &nDecodedSize, pOutput, nullptr, &bTerminated, &bTooLong, &bOversize, pPdStruct);
        if (!guardedThis || !bBody || bTooLong || bOversize || !bTerminated) return false;
    }

    return true;
}

bool XSHAR::_entryMatches(const SHAR_ENTRY &left, const SHAR_ENTRY &right)
{
    return (left.kind == right.kind) && (left.sFileName == right.sFileName) && (left.baRawTarget == right.baRawTarget) &&
           (left.baStripPrefix == right.baStripPrefix) && (left.baDelim == right.baDelim) && (left.bDash == right.bDash) &&
           (left.bUnsafeTarget == right.bUnsafeTarget) && (left.nHeaderOffset == right.nHeaderOffset) && (left.nHeaderSize == right.nHeaderSize) &&
           (left.nBodyOffset == right.nBodyOffset) && (left.nBodyEndOffset == right.nBodyEndOffset) && (left.nDecodedSize == right.nDecodedSize) &&
           (left.nDeclaredSize == right.nDeclaredSize);
}

QMap<XBinary::UNPACK_PROP, QVariant> XSHAR::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSHAR::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSHAR> guardedThis(this);
    if (m_bUnpackOperationInProgress) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }

    SHAR_UNPACK_CONTEXT *pOldContext = static_cast<SHAR_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !isPdStructNotCanceled(pPdStruct)) return false;

    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;
    pState->mapUnpackProperties = mapProperties;

    QList<SHAR_ENTRY> listEntries;
    const bool bScanned = _scanArchive(&listEntries, pPdStruct);
    if (!guardedThis) return false;
    if (!bScanned || !isPdStructNotCanceled(pPdStruct)) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    const qint64 nTotalSize = getSize();
    if (!guardedThis) return false;

    SHAR_UNPACK_CONTEXT *pContext = new (std::nothrow) SHAR_UNPACK_CONTEXT;
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

XBinary::ARCHIVERECORD XSHAR::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XSHAR> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    if (!pState || !pState->pContext) return result;

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return result;

    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nTotalSize != nCurrentSize)) return result;

    SHAR_UNPACK_CONTEXT *pContext = static_cast<SHAR_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->listEntries.count()) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pContext->listEntries.count())) {
        return result;
    }

    const qint32 nExpectedIndex = pState->nCurrentIndex;
    const qint32 nExpectedCount = pState->nNumberOfRecords;
    const SHAR_ENTRY entry = pContext->listEntries.at(nExpectedIndex);

    QList<SHAR_ENTRY> verifiedEntries;
    const bool bRescanned = _scanArchive(&verifiedEntries, pPdStruct);
    if (!guardedThis || !bRescanned || (pState->pContext != pContext) || (pState->nCurrentIndex != nExpectedIndex) || (pState->nNumberOfRecords != nExpectedCount) ||
        (verifiedEntries.count() != nExpectedCount)) {
        return ARCHIVERECORD();
    }

    const SHAR_ENTRY verified = verifiedEntries.at(nExpectedIndex);
    if (!_entryMatches(verified, entry)) return ARCHIVERECORD();

    const bool bSourceStillCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceStillCurrent || (pState->pContext != pContext) || (pState->nCurrentIndex != nExpectedIndex) ||
        (pState->nNumberOfRecords != nExpectedCount)) {
        return ARCHIVERECORD();
    }

    if ((entry.nBodyOffset < 0) || (entry.nBodyEndOffset < entry.nBodyOffset) || (entry.nBodyEndOffset > nCurrentSize) || (entry.nDecodedSize < 0)) {
        return result;
    }

    const qint64 nRawSize = entry.nBodyEndOffset - entry.nBodyOffset;
    result.nStreamOffset = entry.nBodyOffset;
    result.nStreamSize = nRawSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, entry.nDecodedSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nRawSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    if (entry.bUnsafeTarget) {
        result.mapProperties.insert(FPART_PROP_INFO, tr("Unsafe or shell-expanded target was not extracted: %1").arg(sharDiagnosticTarget(entry.baRawTarget)));
    }
    if (!XBinary::markArchiveStreamRecord(&result, nExpectedIndex)) {
        return ARCHIVERECORD();
    }

    return result;
}

bool XSHAR::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XSHAR> guardedThis(this);
    if (!operationGuard.isAcquired() || !pState || !pDevice || !isUnpackSourceCurrent(pState, pPdStruct) || devicesAlias(getDevice(), pDevice)) return false;

    SHAR_UNPACK_CONTEXT *pContext = static_cast<SHAR_UNPACK_CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords) || (pState->nNumberOfRecords != pContext->listEntries.count())) {
        return false;
    }

    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nTotalSize != nCurrentSize)) return false;

    const SHAR_ENTRY entry = pContext->listEntries.at(pState->nCurrentIndex);
    if (entry.bUnsafeTarget) {
        XBinary::setPdStructErrorString(pPdStruct,
                                        tr("Unsafe or shell-expanded SHAR target cannot be extracted: %1").arg(sharDiagnosticTarget(entry.baRawTarget)));
        return false;
    }

    qint64 nConfiguredLimit = -1;
    if (!getUnpackOutputLimit(pState->mapUnpackProperties, &nConfiguredLimit)) return false;
    const qint64 nCap = (nConfiguredLimit < 0) ? SHAR_MAX_DECODED : qMin(SHAR_MAX_DECODED, nConfiguredLimit);

    QByteArray decoded;
    if (!_decodeMember(entry, nCap, &decoded, pPdStruct) || !guardedThis) return false;

    const qint64 nDecodedSize = decoded.size();
    if (!XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nDecodedSize)) return false;

    // The decode path bypasses the base decode chain, so it charges the
    // operation budget itself: one entry, nDecodedSize produced bytes
    // (publishUnpackOutput never debits the copy).
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, entry.sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(nDecodedSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    QBuffer buffer;
    buffer.setData(decoded);
    if (!buffer.open(QIODevice::ReadOnly)) return false;

    const bool bResult = publishUnpackOutput(&buffer, pDevice, pState, pPdStruct);
    if (!guardedThis || !bResult) return false;

    pState->nCurrentOffset = 0;
    return true;
}

bool XSHAR::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XSHAR> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext) {
        return false;
    }

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return false;

    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nTotalSize != nCurrentSize)) return false;

    SHAR_UNPACK_CONTEXT *pContext = static_cast<SHAR_UNPACK_CONTEXT *>(pState->pContext);
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

bool XSHAR::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    Q_UNUSED(pPdStruct)

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }

    SHAR_UNPACK_CONTEXT *pContext = static_cast<SHAR_UNPACK_CONTEXT *>(pState->pContext);
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

QList<XBinary::FPART_PROP> XSHAR::getAvailableFPARTProperties()
{
    QList<FPART_PROP> listResult;
    listResult.append(FPART_PROP_ORIGINALNAME);
    listResult.append(FPART_PROP_UNCOMPRESSEDSIZE);
    listResult.append(FPART_PROP_COMPRESSEDSIZE);
    listResult.append(FPART_PROP_STREAMOFFSET);
    listResult.append(FPART_PROP_STREAMSIZE);
    listResult.append(FPART_PROP_INFO);
    return listResult;
}

bool XSHAR::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XSHAR> guardedThis(this);
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

void *XSHAR::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XSHAR> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;
    return &guardedThis->m_internalInfo;
}

void XSHAR::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
