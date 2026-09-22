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
#include "xtnefarchive.h"

#include <QBuffer>
#include <QtEndian>

#include <new>

namespace {
const quint32 TNEF_SIGNATURE = 0x223e9f78;
const qint64 TNEF_HEADER_SIZE = 6;
const qint64 TNEF_ATTRIBUTE_HEADER_SIZE = 9;
const qint64 TNEF_CHECKSUM_SIZE = 2;
const qint32 TNEF_MAX_MEMBERS = 100000;
const qint64 TNEF_MAX_ATTRIBUTE_SIZE = 0x10000000;
const qint64 TNEF_MAX_VALUE_COUNT = 0x100000;

const quint32 TNEF_ATT_ATTACH_RENDDATA = 0x00069002;
const quint32 TNEF_ATT_ATTACH_TITLE = 0x00018010;
const quint32 TNEF_ATT_ATTACH_DATA = 0x0006800f;
const quint32 TNEF_ATT_ATTACHMENT = 0x00069005;

// Message-level attributes are matched on the attName half only: the attType
// half varies between writers for the same attribute (attBody appears as both
// 0x0001800C and 0x0002800C in the reference corpus), and it is the name that
// identifies the attribute.
const quint16 TNEF_ATT_NAME_BODY = 0x800c;
const quint16 TNEF_ATT_NAME_MAPI_PROPS = 0x9003;

const quint16 TNEF_PR_ATTACH_DATA_OBJ = 0x3701;
const quint16 TNEF_PR_ATTACH_LONG_FILENAME = 0x3707;
const quint16 TNEF_PR_BODY = 0x1000;
const quint16 TNEF_PR_RTF_COMPRESSED = 0x1009;
const quint16 TNEF_PR_BODY_HTML = 0x1013;
const quint16 TNEF_PR_BODY_HTML_A = 0x1014;

// The rendered body is held in memory, so both the compressed-RTF input and
// its expansion are bounded well below the generic attribute cap.
const qint64 TNEF_MAX_BODY_SOURCE_SIZE = 32LL * 1024 * 1024;
const qint64 TNEF_MAX_BODY_SIZE = 64LL * 1024 * 1024;
const QString TNEF_BODY_MEMBER_NAME = QStringLiteral("Content.txt");

const quint16 TNEF_PT_UNSPECIFIED = 0x0000;
const quint16 TNEF_PT_NULL = 0x0001;
const quint16 TNEF_PT_OBJECT = 0x000d;
const quint16 TNEF_PT_STRING8 = 0x001e;
const quint16 TNEF_PT_UNICODE = 0x001f;
const quint16 TNEF_PT_BINARY = 0x0102;

bool tnefRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

qint64 tnefPad4(qint64 nValue)
{
    if ((nValue < 0) || (nValue > (Q_INT64_C(0x7fffffffffffffff) - 3))) return -1;
    return (nValue + 3) & ~Q_INT64_C(3);
}

bool tnefIsVariableType(quint16 nBaseType)
{
    return (nBaseType == TNEF_PT_STRING8) || (nBaseType == TNEF_PT_UNICODE) || (nBaseType == TNEF_PT_BINARY) || (nBaseType == TNEF_PT_OBJECT);
}

// -1 for a type that has no fixed width.
qint32 tnefFixedTypeSize(quint16 nBaseType)
{
    if (nBaseType == 0x0002) return 2;   // PT_I2
    if (nBaseType == 0x0003) return 4;   // PT_LONG
    if (nBaseType == 0x0004) return 4;   // PT_R4
    if (nBaseType == 0x0005) return 8;   // PT_DOUBLE
    if (nBaseType == 0x0006) return 8;   // PT_CURRENCY
    if (nBaseType == 0x0007) return 8;   // PT_APPTIME
    if (nBaseType == 0x000a) return 4;   // PT_ERROR
    if (nBaseType == 0x000b) return 2;   // PT_BOOLEAN
    if (nBaseType == 0x0014) return 8;   // PT_I8
    if (nBaseType == 0x0040) return 8;   // PT_SYSTIME
    if (nBaseType == 0x0048) return 16;  // PT_CLSID
    return -1;
}

QString tnefLatin1UntilZero(const char *pData, qint64 nSize)
{
    QByteArray baText;
    for (qint64 i = 0; i < nSize; ++i) {
        if (pData[i] == (char)0) break;
        baText.append(pData[i]);
    }

    return QString::fromLatin1(baText);
}

// ---------------------------------------------------------------------------
// PR_RTF_COMPRESSED (LZFu, MS-OXRTFCP)
//
//   u32 compressed size (the stream length minus these four bytes)
//   u32 uncompressed size
//   u32 0x75465A4C "LZFu" compressed, 0x414C454D "MELA" stored
//   u32 CRC
//
// then control bytes, low bit first: a clear bit is one literal byte, a set bit
// is a 2-byte back reference (12-bit offset, 4-bit length biased by 2) into a
// 4096-byte ring dictionary preloaded with the fixed 207-byte RTF preamble
// below and written at 207.  A reference whose offset equals the write pointer
// ends the stream.
const quint32 TNEF_LZFU_COMPRESSED = 0x75465a4c;
const quint32 TNEF_LZFU_UNCOMPRESSED = 0x414c454d;
const qint32 TNEF_LZFU_DICTIONARY_SIZE = 4096;
const char TNEF_LZFU_INIT[] =
    "{\\rtf1\\ansi\\mac\\deff0\\deftab720{\\fonttbl;}{\\f0\\fnil \\froman \\fswiss \\fmodern \\fscript \\fdecor MS Sans SerifSymbolArialTimes New "
    "RomanCourier{\\colortbl\\red0\\green0\\blue0\r\n\\par \\pard\\plain\\f0\\fs20\\b\\i\\u\\tab\\tx";

bool tnefDecompressRtf(const QByteArray &baStream, QByteArray *pbaRtf)
{
    if (!pbaRtf) return false;
    pbaRtf->clear();
    if (baStream.size() < 16) return false;

    const uchar *pStream = (const uchar *)baStream.constData();
    const quint32 nCompressedSize = qFromLittleEndian<quint32>(pStream);
    const quint32 nRawSize = qFromLittleEndian<quint32>(pStream + 4);
    const quint32 nCompressionType = qFromLittleEndian<quint32>(pStream + 8);
    if ((qint64)nRawSize > TNEF_MAX_BODY_SIZE) return false;

    if (nCompressionType == TNEF_LZFU_UNCOMPRESSED) {
        const qint64 nAvailable = baStream.size() - 16;
        const qint64 nSize = qMin<qint64>((qint64)nRawSize, nAvailable);
        if (nSize <= 0) return false;
        *pbaRtf = baStream.mid(16, (qint32)nSize);
        return !pbaRtf->isEmpty();
    }
    if (nCompressionType != TNEF_LZFU_COMPRESSED) return false;

    // The declared compressed size is advisory; the stream is the authority.
    qint64 nEnd = baStream.size();
    if ((qint64)nCompressedSize + 4 < nEnd) nEnd = (qint64)nCompressedSize + 4;

    const qint32 nInitSize = (qint32)(sizeof(TNEF_LZFU_INIT) - 1);
    QByteArray baDictionary(TNEF_LZFU_DICTIONARY_SIZE, '\0');
    if (baDictionary.size() != TNEF_LZFU_DICTIONARY_SIZE) return false;
    if ((nInitSize <= 0) || (nInitSize > TNEF_LZFU_DICTIONARY_SIZE)) return false;
    memcpy(baDictionary.data(), TNEF_LZFU_INIT, (size_t)nInitSize);

    uchar *pDictionary = (uchar *)baDictionary.data();
    qint32 nWrite = nInitSize;
    QByteArray baOutput;
    baOutput.reserve((qint32)qMin<qint64>((qint64)nRawSize, TNEF_MAX_BODY_SIZE));
    qint64 nPosition = 16;

    while (((qint64)baOutput.size() < (qint64)nRawSize) && (nPosition < nEnd)) {
        const quint8 nControl = pStream[nPosition];
        ++nPosition;
        for (qint32 nBit = 0; nBit < 8; ++nBit) {
            if ((qint64)baOutput.size() >= (qint64)nRawSize) break;
            if ((nControl >> nBit) & 1) {
                if ((nPosition + 2) > nEnd) {
                    *pbaRtf = baOutput;
                    return !pbaRtf->isEmpty();
                }
                const quint32 nFirst = pStream[nPosition];
                const quint32 nSecond = pStream[nPosition + 1];
                nPosition += 2;
                const qint32 nOffset = (qint32)(((nFirst << 4) | (nSecond >> 4)) & 0xfff);
                const qint32 nLength = (qint32)((nSecond & 0x0f) + 2);
                if (nOffset == nWrite) {
                    *pbaRtf = baOutput;
                    return !pbaRtf->isEmpty();
                }
                for (qint32 i = 0; i < nLength; ++i) {
                    if ((qint64)baOutput.size() >= (qint64)nRawSize) break;
                    const char nByte = (char)pDictionary[(nOffset + i) & (TNEF_LZFU_DICTIONARY_SIZE - 1)];
                    baOutput.append(nByte);
                    pDictionary[nWrite] = (uchar)nByte;
                    nWrite = (nWrite + 1) & (TNEF_LZFU_DICTIONARY_SIZE - 1);
                }
            } else {
                if (nPosition >= nEnd) break;
                const char nByte = (char)pStream[nPosition];
                ++nPosition;
                baOutput.append(nByte);
                pDictionary[nWrite] = (uchar)nByte;
                nWrite = (nWrite + 1) & (TNEF_LZFU_DICTIONARY_SIZE - 1);
            }
        }
    }

    *pbaRtf = baOutput;
    return !pbaRtf->isEmpty();
}

// Destinations whose contents are markup bookkeeping rather than message text.
bool tnefIsSkippedDestination(const QByteArray &baWord)
{
    static const char *const s_arrNames[] = {"fonttbl",  "colortbl", "stylesheet", "info",     "pict",      "object",   "header",   "footer",
                                             "headerl",  "headerr",  "footerl",    "footerr",  "generator", "filetbl",  "listtable", "listoverridetable",
                                             "revtbl",   "rsidtbl",  "xmlnstbl",   "panose",   "falt",      "fname",    "author",   "operator",
                                             "company",  "creatim",  "revtim",     "printim",  "buptim",    "title",    "subject",  "keywords",
                                             "comment",  "doccomm",  "userprops",  "themedata", "colorschememapping", "datastore", "latentstyles",
                                             "fldinst",  "nonshppict"};
    const qint32 nCount = (qint32)(sizeof(s_arrNames) / sizeof(s_arrNames[0]));
    for (qint32 i = 0; i < nCount; ++i) {
        if (baWord == s_arrNames[i]) return true;
    }
    return false;
}

// A deliberately small RTF-to-text pass: it keeps literal text and the few
// control words that carry layout, and drops the rest along with the
// bookkeeping destinations above.  It is a rendering, not a parser.
QByteArray tnefRtfToText(const QByteArray &baRtf)
{
    QByteArray baText;
    const qint64 nSize = baRtf.size();
    const uchar *pRtf = (const uchar *)baRtf.constData();
    qint64 nPosition = 0;
    qint32 nDepth = 0;
    qint32 nSkipDepth = -1;   // group depth to skip out of, -1 when not skipping
    bool bSkipping = false;
    bool bIgnorable = false;
    qint32 nUnicodeSkip = 1;  // \ucN: fallback characters that follow a \uN
    qint32 nPendingSkip = 0;

    while (nPosition < nSize) {
        const uchar nCharacter = pRtf[nPosition];

        if (nCharacter == '\\') {
            ++nPosition;
            if (nPosition >= nSize) break;
            const uchar nNext = pRtf[nPosition];

            if (nNext == '\'') {
                // \'hh - one literal byte in the document code page
                if ((nPosition + 2) < nSize) {
                    bool bOk = false;
                    const quint32 nValue = QByteArray(baRtf.constData() + nPosition + 1, 2).toUInt(&bOk, 16);
                    if (bOk && !bSkipping) {
                        if (nPendingSkip > 0) --nPendingSkip;
                        else baText.append((char)(quint8)nValue);
                    }
                    nPosition += 3;
                } else {
                    ++nPosition;
                }
                continue;
            }
            if (nNext == '*') {
                bIgnorable = true;
                ++nPosition;
                continue;
            }
            if ((nNext == '\\') || (nNext == '{') || (nNext == '}')) {
                if (!bSkipping) {
                    if (nPendingSkip > 0) --nPendingSkip;
                    else baText.append((char)nNext);
                }
                ++nPosition;
                continue;
            }
            if ((nNext == '\r') || (nNext == '\n')) {
                ++nPosition;
                continue;
            }
            if (nNext == '~') {
                if (!bSkipping) baText.append(' ');
                ++nPosition;
                continue;
            }

            qint64 nWordEnd = nPosition;
            while ((nWordEnd < nSize) && (((pRtf[nWordEnd] >= 'a') && (pRtf[nWordEnd] <= 'z')) || ((pRtf[nWordEnd] >= 'A') && (pRtf[nWordEnd] <= 'Z')))) {
                ++nWordEnd;
            }
            if (nWordEnd == nPosition) {
                // an unknown one-character control symbol
                ++nPosition;
                bIgnorable = false;
                continue;
            }
            const QByteArray baWord = QByteArray(baRtf.constData() + nPosition, (qint32)(nWordEnd - nPosition));
            qint64 nNumberEnd = nWordEnd;
            bool bNegative = false;
            if ((nNumberEnd < nSize) && (pRtf[nNumberEnd] == '-')) {
                bNegative = true;
                ++nNumberEnd;
            }
            qint64 nNumber = 0;
            bool bHasNumber = false;
            while ((nNumberEnd < nSize) && (pRtf[nNumberEnd] >= '0') && (pRtf[nNumberEnd] <= '9')) {
                if (nNumber < 0x7fffffff) nNumber = (nNumber * 10) + (pRtf[nNumberEnd] - '0');
                bHasNumber = true;
                ++nNumberEnd;
            }
            if ((nNumberEnd < nSize) && (pRtf[nNumberEnd] == ' ')) ++nNumberEnd;
            nPosition = nNumberEnd;

            if (bIgnorable || tnefIsSkippedDestination(baWord)) {
                if (!bSkipping) {
                    bSkipping = true;
                    nSkipDepth = nDepth - 1;
                }
                bIgnorable = false;
                continue;
            }
            bIgnorable = false;
            if (bSkipping) continue;

            if ((baWord == "par") || (baWord == "line") || (baWord == "sect")) {
                baText.append('\r');
            } else if (baWord == "tab") {
                baText.append('\t');
            } else if (baWord == "uc") {
                if (bHasNumber && !bNegative && (nNumber >= 0) && (nNumber <= 0xff)) nUnicodeSkip = (qint32)nNumber;
            } else if (baWord == "u") {
                if (bHasNumber) {
                    qint64 nCodePoint = bNegative ? (65536 - nNumber) : nNumber;
                    if ((nCodePoint > 0) && (nCodePoint <= 0x10ffff)) {
                        baText.append(QString(QChar((uint)nCodePoint)).toUtf8());
                    }
                    nPendingSkip = nUnicodeSkip;
                }
            } else if ((baWord == "lquote") || (baWord == "rquote")) {
                baText.append('\'');
            } else if ((baWord == "ldblquote") || (baWord == "rdblquote")) {
                baText.append('"');
            } else if ((baWord == "emdash") || (baWord == "endash")) {
                baText.append('-');
            } else if (baWord == "bullet") {
                baText.append('*');
            }
            continue;
        }

        if (nCharacter == '{') {
            ++nDepth;
            ++nPosition;
            continue;
        }
        if (nCharacter == '}') {
            --nDepth;
            if (bSkipping && (nDepth <= nSkipDepth)) {
                bSkipping = false;
                nSkipDepth = -1;
            }
            ++nPosition;
            continue;
        }
        if ((nCharacter == '\r') || (nCharacter == '\n')) {
            // Literal line breaks in an RTF file are formatting, not text.
            ++nPosition;
            continue;
        }
        if (!bSkipping) {
            if (nPendingSkip > 0) --nPendingSkip;
            else baText.append((char)nCharacter);
        }
        ++nPosition;
    }

    return baText;
}

// PR_BODY_HTML is real HTML rather than RTF-encapsulated HTML, so it gets its
// own equally small rendering.
QByteArray tnefHtmlToText(const QByteArray &baHtml)
{
    QByteArray baText;
    const qint64 nSize = baHtml.size();
    const char *pHtml = baHtml.constData();
    qint64 nPosition = 0;
    bool bInScript = false;

    while (nPosition < nSize) {
        if (pHtml[nPosition] == '<') {
            const qint32 nTagEnd = baHtml.indexOf('>', (qint32)nPosition);
            if (nTagEnd < 0) break;
            QByteArray baTag = QByteArray(pHtml + nPosition + 1, (qint32)(nTagEnd - nPosition - 1)).toLower();
            const bool bClosing = baTag.startsWith('/');
            if (bClosing) baTag.remove(0, 1);
            const qint32 nNameEnd = baTag.indexOf(' ');
            const QByteArray baName = (nNameEnd >= 0) ? baTag.left(nNameEnd) : baTag;
            if ((baName == "script") || (baName == "style")) bInScript = !bClosing;
            if (!bInScript && ((baName == "br") || (baName == "p") || (baName == "div") || (baName == "tr") || (baName == "li"))) {
                // HTML collapses run-in whitespace, so a line does not keep the
                // spaces that only separated it from the tag that broke it.
                while (!baText.isEmpty() && ((baText.at(baText.size() - 1) == ' ') || (baText.at(baText.size() - 1) == '\t'))) baText.chop(1);
                baText.append("\r\n", 2);
            }
            nPosition = nTagEnd + 1;
            continue;
        }
        if ((pHtml[nPosition] == '\r') || (pHtml[nPosition] == '\n')) {
            // HTML source line breaks are not text either.
            ++nPosition;
            continue;
        }
        if (!bInScript) baText.append(pHtml[nPosition]);
        ++nPosition;
    }

    baText.replace("&nbsp;", " ");
    baText.replace("&amp;", "&");
    baText.replace("&lt;", "<");
    baText.replace("&gt;", ">");
    baText.replace("&quot;", "\"");
    baText.replace("&#39;", "'");

    return baText;
}

}  // namespace

XTNEFArchive::XTNEFArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XTNEFArchive::~XTNEFArchive()
{
}

// A faithful walk of the MAPI property stream: the offsets only stay in step
// with the file if every property is measured, including the ones nothing here
// is interested in.
//
// One walk serves both levels.  The attachment call site fills only the
// attachment fields of the sink and the message call site only the body ones,
// so the attachment result is produced by exactly the same code as before.
bool XTNEFArchive::scanAttachmentProperties(const QByteArray &baStream, qint64 *pnDataOffset, qint64 *pnDataSize, QString *psFileName)
{
    if (!pnDataOffset || !pnDataSize || !psFileName) return false;
    PROPERTY_SINK sink = {};
    sink.pnDataOffset = pnDataOffset;
    sink.pnDataSize = pnDataSize;
    sink.psFileName = psFileName;
    return scanProperties(baStream, sink);
}

bool XTNEFArchive::scanProperties(const QByteArray &baStream, const PROPERTY_SINK &sink)
{
    if (sink.pnDataOffset) *sink.pnDataOffset = -1;
    if (sink.pnDataSize) *sink.pnDataSize = -1;
    if (sink.psFileName) sink.psFileName->clear();
    if (sink.pbaRtfCompressed) sink.pbaRtfCompressed->clear();
    if (sink.pbaBodyHtml) sink.pbaBodyHtml->clear();
    if (sink.pbaBody) sink.pbaBody->clear();

    const qint64 nStreamSize = baStream.size();
    if (nStreamSize < 4) return false;
    const uchar *pStream = (const uchar *)baStream.constData();

    const qint64 nPropertyCount = (qint64)qFromLittleEndian<quint32>(pStream);
    if ((nPropertyCount < 0) || (nPropertyCount > TNEF_MAX_VALUE_COUNT)) return false;

    qint64 nPosition = 4;
    for (qint64 nProperty = 0; nProperty < nPropertyCount; ++nProperty) {
        if ((nPosition + 4) > nStreamSize) break;
        const quint16 nType = qFromLittleEndian<quint16>(pStream + nPosition);
        const quint16 nId = qFromLittleEndian<quint16>(pStream + nPosition + 2);
        nPosition += 4;

        if (nId >= 0x8000) {
            if ((nPosition + 20) > nStreamSize) break;
            nPosition += 16;
            const quint32 nKind = qFromLittleEndian<quint32>(pStream + nPosition);
            nPosition += 4;
            if (nKind == 0) {
                nPosition += 4;
            } else if (nKind == 1) {
                if ((nPosition + 4) > nStreamSize) break;
                const qint64 nNameLength = (qint64)qFromLittleEndian<quint32>(pStream + nPosition);
                const qint64 nPaddedName = tnefPad4(nNameLength);
                if (nPaddedName < 0) break;
                nPosition += 4 + nPaddedName;
            } else {
                break;
            }
        }

        const quint16 nBaseType = nType & 0x0fff;
        const bool bMulti = ((nType & 0x1000) != 0);
        const bool bVariable = tnefIsVariableType(nBaseType);
        const qint32 nFixedSize = tnefFixedTypeSize(nBaseType);

        qint64 nValueCount = 1;
        if (bMulti || bVariable) {
            if ((nPosition + 4) > nStreamSize) break;
            nValueCount = (qint64)qFromLittleEndian<quint32>(pStream + nPosition);
            nPosition += 4;
        }
        if ((nValueCount < 0) || (nValueCount > TNEF_MAX_VALUE_COUNT)) break;

        for (qint64 nValue = 0; nValue < nValueCount; ++nValue) {
            if (nFixedSize >= 0) {
                const qint64 nPadded = tnefPad4(nFixedSize);
                if ((nPadded < 0) || ((nPosition + nPadded) > nStreamSize)) return true;
                nPosition += nPadded;
                continue;
            }
            if (bVariable) {
                if ((nPosition + 4) > nStreamSize) return true;
                const qint64 nLength = (qint64)qFromLittleEndian<quint32>(pStream + nPosition);
                nPosition += 4;
                if ((nLength < 0) || ((nPosition + nLength) > nStreamSize)) return true;

                qint64 nValueOffset = nPosition;
                qint64 nValueSize = nLength;
                // A PT_OBJECT value keeps a 16-byte interface GUID in front of
                // the real payload; the reference strips it.
                if ((nBaseType == TNEF_PT_OBJECT) && (nValueSize >= 16)) {
                    nValueOffset += 16;
                    nValueSize -= 16;
                }
                if ((nId == TNEF_PR_ATTACH_DATA_OBJ) && sink.pnDataOffset && sink.pnDataSize && (*sink.pnDataOffset < 0)) {
                    *sink.pnDataOffset = nValueOffset;
                    *sink.pnDataSize = nValueSize;
                } else if ((nId == TNEF_PR_ATTACH_LONG_FILENAME) && sink.psFileName && sink.psFileName->isEmpty() && (nValueSize > 0)) {
                    *sink.psFileName = tnefLatin1UntilZero(baStream.constData() + nValueOffset, nValueSize);
                } else if ((nId == TNEF_PR_RTF_COMPRESSED) && sink.pbaRtfCompressed && sink.pbaRtfCompressed->isEmpty() && (nValueSize > 0) &&
                           (nValueSize <= TNEF_MAX_BODY_SOURCE_SIZE)) {
                    *sink.pbaRtfCompressed = baStream.mid((qint32)nValueOffset, (qint32)nValueSize);
                } else if (((nId == TNEF_PR_BODY_HTML) || (nId == TNEF_PR_BODY_HTML_A)) && sink.pbaBodyHtml && sink.pbaBodyHtml->isEmpty() &&
                           (nValueSize > 0) && (nValueSize <= TNEF_MAX_BODY_SOURCE_SIZE)) {
                    *sink.pbaBodyHtml = baStream.mid((qint32)nValueOffset, (qint32)nValueSize);
                } else if ((nId == TNEF_PR_BODY) && sink.pbaBody && sink.pbaBody->isEmpty() && (nValueSize > 0) && (nValueSize <= TNEF_MAX_BODY_SOURCE_SIZE)) {
                    *sink.pbaBody = baStream.mid((qint32)nValueOffset, (qint32)nValueSize);
                }

                const qint64 nPadded = tnefPad4(nLength);
                if (nPadded < 0) return true;
                nPosition += nPadded;
            } else if ((nBaseType == TNEF_PT_UNSPECIFIED) || (nBaseType == TNEF_PT_NULL)) {
                // no payload
            } else {
                // An unknown fixed width makes the rest of the stream
                // unparseable; keep whatever was already recovered.
                return true;
            }
        }
    }

    return true;
}

bool XTNEFArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < TNEF_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, TNEF_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != TNEF_HEADER_SIZE)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();
    if (qFromLittleEndian<quint32>(pHeader) != TNEF_SIGNATURE) return false;
    if (qFromLittleEndian<quint16>(pHeader + 4) == 0) return false;

    bool bHasCurrent = false;
    MEMBER current = {};
    qint64 nOffset = TNEF_HEADER_SIZE;
    QByteArray baRtfCompressed;
    QByteArray baBodyHtml;
    QByteArray baMapiBody;
    QByteArray baPlainBody;

    while ((nOffset + TNEF_ATTRIBUTE_HEADER_SIZE) <= context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= TNEF_MAX_MEMBERS) break;

        const QByteArray baAttribute = read_array_process(nOffset, TNEF_ATTRIBUTE_HEADER_SIZE, pPdStruct);
        if (!guardedSource || (baAttribute.size() != TNEF_ATTRIBUTE_HEADER_SIZE)) return false;
        const uchar *pAttribute = (const uchar *)baAttribute.constData();
        const quint8 nLevel = pAttribute[0];
        const quint32 nAttId = qFromLittleEndian<quint32>(pAttribute + 1);
        const qint64 nLength = (qint32)qFromLittleEndian<quint32>(pAttribute + 5);

        const qint64 nBodyOffset = nOffset + TNEF_ATTRIBUTE_HEADER_SIZE;
        if (((nLevel != 1) && (nLevel != 2)) || (nLength < 0) || !tnefRangeWithin(context.nInputSize, nBodyOffset, nLength)) break;
        nOffset = nBodyOffset + nLength + TNEF_CHECKSUM_SIZE;

        if (nLevel == 1) {
            // Message level.  Only the body sources are read here; nothing at
            // this level can reach the attachment list.
            const quint16 nAttName = (quint16)(nAttId & 0xffff);
            if ((nAttName == TNEF_ATT_NAME_BODY) && baPlainBody.isEmpty() && (nLength > 0) && (nLength <= TNEF_MAX_BODY_SOURCE_SIZE)) {
                baPlainBody = read_array_process(nBodyOffset, nLength, pPdStruct);
                if (!guardedSource || (baPlainBody.size() != nLength)) return false;
            } else if ((nAttName == TNEF_ATT_NAME_MAPI_PROPS) && (nLength > 0) && (nLength <= TNEF_MAX_ATTRIBUTE_SIZE)) {
                const QByteArray baStream = read_array_process(nBodyOffset, nLength, pPdStruct);
                if (!guardedSource || (baStream.size() != nLength)) return false;
                QByteArray baRtf;
                QByteArray baHtml;
                QByteArray baBody;
                PROPERTY_SINK sink = {};
                sink.pbaRtfCompressed = &baRtf;
                sink.pbaBodyHtml = &baHtml;
                sink.pbaBody = &baBody;
                if (scanProperties(baStream, sink)) {
                    if (baRtfCompressed.isEmpty()) baRtfCompressed = baRtf;
                    if (baBodyHtml.isEmpty()) baBodyHtml = baHtml;
                    if (baMapiBody.isEmpty()) baMapiBody = baBody;
                }
            }
            continue;
        }

        if (nLevel != 2) continue;

        if (nAttId == TNEF_ATT_ATTACH_RENDDATA) {
            if (bHasCurrent) context.listMembers.append(current);
            current = MEMBER();
            current.nHeaderOffset = nBodyOffset;
            current.nDataOffset = -1;
            current.nSize = 0;
            bHasCurrent = true;
        } else if (!bHasCurrent) {
            current = MEMBER();
            current.nHeaderOffset = nBodyOffset;
            current.nDataOffset = -1;
            current.nSize = 0;
            bHasCurrent = true;
        }

        if (nAttId == TNEF_ATT_ATTACH_TITLE) {
            if (nLength > 0) {
                const QByteArray baTitle = read_array_process(nBodyOffset, nLength, pPdStruct);
                if (!guardedSource || (baTitle.size() != nLength)) return false;
                current.sFileName = tnefLatin1UntilZero(baTitle.constData(), baTitle.size());
            }
        } else if (nAttId == TNEF_ATT_ATTACH_DATA) {
            current.nDataOffset = nBodyOffset;
            current.nSize = nLength;
        } else if (nAttId == TNEF_ATT_ATTACHMENT) {
            if ((nLength > 0) && (nLength <= TNEF_MAX_ATTRIBUTE_SIZE)) {
                const QByteArray baStream = read_array_process(nBodyOffset, nLength, pPdStruct);
                if (!guardedSource || (baStream.size() != nLength)) return false;
                qint64 nValueOffset = -1;
                qint64 nValueSize = -1;
                QString sLongName;
                if (scanAttachmentProperties(baStream, &nValueOffset, &nValueSize, &sLongName)) {
                    if ((current.nDataOffset < 0) && (nValueOffset >= 0) && (nValueSize >= 0)) {
                        current.nDataOffset = nBodyOffset + nValueOffset;
                        current.nSize = nValueSize;
                    }
                    if (!sLongName.isEmpty()) current.sFileName = sLongName;
                }
            }
        }
    }

    if (bHasCurrent) context.listMembers.append(current);

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        MEMBER &member = context.listMembers[i];
        if (member.nDataOffset < 0) {
            // An attachment the walker saw but that carries no payload; the
            // reference emits it as an empty file.
            member.nDataOffset = 0;
            member.nSize = 0;
        }
        if (member.sFileName.isEmpty()) member.sFileName = QStringLiteral("attachment%1").arg(i + 1);
    }

    // The message body is not stored as an archive member by the format: it
    // lives in the message-level attributes as compressed RTF, HTML or plain
    // text.  It is rendered here and appended as ONE synthetic member, after
    // every attachment, so no attachment name, index, offset or byte changes.
    // Compressed RTF wins where several sources are present, matching the
    // reference: "triples.tnef" carries both attBody and PR_RTF_COMPRESSED and
    // the reference emits the RTF rendering.
    QByteArray baBodyText;
    if (!baRtfCompressed.isEmpty()) {
        QByteArray baRtf;
        if (tnefDecompressRtf(baRtfCompressed, &baRtf)) baBodyText = tnefRtfToText(baRtf);
    }
    if (baBodyText.isEmpty() && !baBodyHtml.isEmpty()) baBodyText = tnefHtmlToText(baBodyHtml);
    if (baBodyText.isEmpty() && !baMapiBody.isEmpty()) baBodyText = baMapiBody;
    if (baBodyText.isEmpty() && !baPlainBody.isEmpty()) baBodyText = baPlainBody;
    // A rendering that ends in the RTF stream's terminating NUL is trimmed:
    // the NUL belongs to the container, not to the text.
    while (!baBodyText.isEmpty() && (baBodyText.at(baBodyText.size() - 1) == '\0')) baBodyText.chop(1);

    if (!baBodyText.isEmpty() && (baBodyText.size() <= TNEF_MAX_BODY_SIZE) && (context.listMembers.size() < TNEF_MAX_MEMBERS)) {
        QString sBodyName = TNEF_BODY_MEMBER_NAME;
        for (qint32 nSuffix = 1; nSuffix < 1000; ++nSuffix) {
            bool bTaken = false;
            for (qint32 i = 0; i < context.listMembers.size(); ++i) {
                if (context.listMembers.at(i).sFileName.compare(sBodyName, Qt::CaseInsensitive) == 0) {
                    bTaken = true;
                    break;
                }
            }
            if (!bTaken) break;
            sBodyName = QStringLiteral("Content_%1.txt").arg(nSuffix);
        }

        MEMBER body = {};
        // The bytes are computed, so the record carries no extent in the file;
        // unpackCurrent() serves them from baInlineData instead.
        body.nHeaderOffset = 0;
        body.nDataOffset = 0;
        body.nSize = baBodyText.size();
        body.sFileName = sBodyName;
        body.bSynthetic = true;
        body.baInlineData = baBodyText;
        context.listMembers.append(body);
    }

    context.nArchiveSize = qMin(nOffset, context.nInputSize);
    *pContext = context;

    return true;
}

bool XTNEFArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XTNEFArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XTNEFArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XTNEFArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XTNEFArchive(pDevice);
}

QList<QString> XTNEFArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("789F3E22");
}

XBinary::FT XTNEFArchive::getFileType()
{
    return FT_TNEF;
}

XBinary::MODE XTNEFArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XTNEFArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XTNEFArchive::getArch()
{
    return QString();
}

qint32 XTNEFArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XTNEFArchive::getFileFormatExt()
{
    return QStringLiteral("dat");
}

QString XTNEFArchive::getFileFormatExtsString()
{
    return QStringLiteral("TNEF (*.dat;*.tnef)");
}

QString XTNEFArchive::getMIMEString()
{
    return QStringLiteral("application/vnd.ms-tnef");
}

QString XTNEFArchive::getVersion()
{
    return QString();
}

qint64 XTNEFArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XTNEFArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XTNEFArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XTNEFArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XTNEFArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = TNEF_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        // The rendered body is not a region of the file, so it has no stream
        // part; the member list is where it belongs.
        if (member.bSynthetic) continue;

        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_OVERLAY) && (context.nArchiveSize < context.nInputSize) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XTNEFArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XTNEFArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    // An attachment-free message is a legitimate TNEF, so an empty member list
    // is not a parse failure here.
    if (!parseContext(pContext, pPdStruct) || !guardedSource) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = pContext->listMembers.isEmpty() ? pContext->nArchiveSize : pContext->listMembers.at(0).nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XTNEFArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XTNEFArchive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    // Which member this is decides which implementation runs, and that has to
    // be read BEFORE the operation guard is taken: XArchive::unpackCurrent()
    // acquires the same guard, and acquiring it twice fails.  Everything the
    // decision rests on is re-checked under the guard below.
    if (!pState) return false;
    CONTEXT *pPeekContext = (CONTEXT *)pState->pContext;
    if (!pPeekContext || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pPeekContext->listMembers.size())) return false;
    if (!pPeekContext->listMembers.at(pState->nCurrentIndex).bSynthetic) {
        // Every attachment takes the inherited stored-stream path unchanged.
        return XArchive::unpackCurrent(pState, pDevice, pPdStruct);
    }

    QIODevice *guardedOutput = pDevice;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !guardedOutput || !isUnpackOutputSupported(guardedOutput) || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedOutput || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (!member.bSynthetic || (member.baInlineData.size() != member.nSize)) return false;
    if (!XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, member.nSize)) {
        XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
        return false;
    }
    if (pState->spOutputBudget && !pState->spOutputBudget->beginEntry(pState->nCurrentIndex, member.sFileName)) {
        if (pState->spOutputBudget->isEnforcing()) return false;
        XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
    }
    if (pState->spOutputBudget && !pState->spOutputBudget->debit(member.nSize)) {
        if (pState->spOutputBudget->isEnforcing()) return false;
        XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
    }

    QByteArray baBody = member.baInlineData;
    QBuffer bufferBody(&baBody);
    if (!bufferBody.open(QIODevice::ReadOnly)) return false;
    const bool bResult = publishUnpackOutput(&bufferBody, guardedOutput, pState, pPdStruct);
    bufferBody.close();

    return bResult && guardedOutput;
}

bool XTNEFArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nDataOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;

    return false;
}

bool XTNEFArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}

QList<XBinary::FPART_PROP> XTNEFArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
