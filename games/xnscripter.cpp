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
#include "xnscripter.h"

#include <QBuffer>
#include <QTemporaryFile>
#include <QTextCodec>
#include <QtEndian>
#include <cstring>
#include <new>

#include "xbzip2decoder.h"

namespace {
const qint64 MaxIndexSize = 16LL * 1024 * 1024;
const qint64 MaxPackedSize = 512LL * 1024 * 1024;
const qint64 MaxOutputSize = 1024LL * 1024 * 1024;
const qint64 ProbeWindow = 64LL * 1024;
const qint32 MaxNameLength = 1024;
const qint32 MaxMembers = 65535;
const qint32 SpbMaxDimension = 16384;

quint32 be32(const uchar *pData)
{
    return qFromBigEndian<quint32>(pData);
}

quint16 be16(const uchar *pData)
{
    return qFromBigEndian<quint16>(pData);
}

quint32 le32(const uchar *pData)
{
    return qFromLittleEndian<quint32>(pData);
}

bool isNameByteAllowed(uchar nByte)
{
    return (nByte >= 0x20) && (nByte != 0x7F);
}

QString decodeMemberName(const QByteArray &baName)
{
    QTextCodec *pCodec = QTextCodec::codecForName("Shift-JIS");
    QString sName = pCodec ? pCodec->toUnicode(baName) : QString::fromLatin1(baName);
    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return sName;
}

bool isMemberNameSafe(const QString &sName)
{
    if (sName.isEmpty() || sName.startsWith(QLatin1Char('/')) || sName.contains(QLatin1Char(':'))) return false;
    const QStringList listParts = sName.split(QLatin1Char('/'));
    const qint32 nCount = listParts.size();
    for (qint32 i = 0; i < nCount; i++) {
        const QString &sPart = listParts.at(i);
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) || (sPart == QLatin1String(".."))) return false;
    }
    return true;
}

// MSB-first bit reader over a whole buffer.  Running past the end is an
// error, never a stream of zeros: a truncated member fails closed.
struct BIT_READER {
    const uchar *pData;
    qint64 nSize;
    qint64 nPos;
    qint32 nMask;
    uchar nCurrent;
    bool bError;

    BIT_READER(const uchar *pBuffer, qint64 nBufferSize) : pData(pBuffer), nSize(nBufferSize), nPos(0), nMask(0), nCurrent(0), bError(false)
    {
    }

    quint32 read(qint32 nBits)
    {
        quint32 nResult = 0;
        for (qint32 i = 0; i < nBits; i++) {
            if (nMask == 0) {
                if (nPos >= nSize) {
                    bError = true;
                    return 0;
                }
                nCurrent = pData[nPos++];
                nMask = 0x80;
            }
            nResult <<= 1;
            if (nCurrent & nMask) nResult |= 1;
            nMask >>= 1;
        }
        return nResult;
    }
};

// Captured state for the unpack path, held by reference the way the other
// standalone readers in this tree carry their per-call scratch.
struct NSCRIPTER_CANCELED {
    XNScripterArchive *owner;
    QIODevice *source;
    QIODevice *output;
    XBinary::PDSTRUCT *pPdStruct;
    NSCRIPTER_CANCELED(XNScripterArchive *ownerRef, QIODevice *sourceRef, QIODevice *outputRef,
                       XBinary::PDSTRUCT *pPd)
        : owner(ownerRef), source(sourceRef), output(outputRef), pPdStruct(pPd)
    {
    }
    bool operator()() const
    {
        return !owner || !source || !output || !XBinary::isPdStructNotCanceled(pPdStruct);
    }
};
}  // namespace

XNScripterArchive::XNScripterArchive(QIODevice *pDevice, FT fileType) : XArchive(pDevice), m_fileType(fileType)
{
}

XNScripterArchive::~XNScripterArchive()
{
}

XBinary::FT XNScripterArchive::getFileType()
{
    return m_fileType;
}

XBinary::MODE XNScripterArchive::getMode()
{
    return MODE_DATA;
}

qint32 XNScripterArchive::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XNScripterArchive::getEndian()
{
    return (m_fileType == FT_NSCRIPTER_NS2) ? ENDIAN_LITTLE : ENDIAN_BIG;
}

QString XNScripterArchive::getArch()
{
    return QString();
}

QString XNScripterArchive::getVersion()
{
    return QString();
}

QString XNScripterArchive::getFileFormatExt()
{
    if (m_fileType == FT_NSCRIPTER_SAR) return QStringLiteral("sar");
    if (m_fileType == FT_NSCRIPTER_NS2) return QStringLiteral("ns2");
    return QStringLiteral("nsa");
}

QString XNScripterArchive::getFileFormatExtsString()
{
    return QStringLiteral("NScripter archive (*.nsa *.ns2 *.sar)");
}

QString XNScripterArchive::getMIMEString()
{
    return QStringLiteral("application/octet-stream");
}

qint64 XNScripterArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context;
    return readContext(m_fileType, &context, pPdStruct) ? context.nArchiveEnd : 0;
}

QList<QString> XNScripterArchive::getSearchSignatures()
{
    return QList<QString>();
}

XBinary *XNScripterArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XNScripterArchive(pDevice, m_fileType);
}

bool XNScripterArchive::isValid(PDSTRUCT *pPdStruct)
{
    CONTEXT context;
    if ((m_fileType == FT_NSCRIPTER_NSA) || (m_fileType == FT_NSCRIPTER_SAR) || (m_fileType == FT_NSCRIPTER_NS2)) {
        return readContext(m_fileType, &context, pPdStruct);
    }
    return false;
}

bool XNScripterArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    return detectFileType(pDevice, pPdStruct) != FT_UNKNOWN;
}

XBinary::FT XNScripterArchive::detectFileType(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XNScripterArchive archive(pDevice, FT_NSCRIPTER_NSA);
    CONTEXT context;
    if (archive.readContext(FT_NSCRIPTER_NSA, &context, pPdStruct)) return FT_NSCRIPTER_NSA;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return FT_UNKNOWN;
    if (archive.readContext(FT_NSCRIPTER_SAR, &context, pPdStruct)) return FT_NSCRIPTER_SAR;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return FT_UNKNOWN;
    if (archive.readContext(FT_NSCRIPTER_NS2, &context, pPdStruct)) return FT_NSCRIPTER_NS2;
    return FT_UNKNOWN;
}

bool XNScripterArchive::readContext(FT fileType, CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    QIODevice *source = getDevice();
    if (!pContext || !source || !source->isOpen() || !source->isReadable() || source->isSequential() || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const qint64 nTotalSize = getSize();
    if (!source || (nTotalSize < 12)) return false;

    CONTEXT parsed;
    parsed.fileType = fileType;
    XNScripterArchive *owner = this;
    bool bResult = false;
    try {
        if (fileType == FT_NSCRIPTER_NS2) {
            bResult = readNs2Archive(nTotalSize, &parsed, pPdStruct);
        } else if ((fileType == FT_NSCRIPTER_NSA) || (fileType == FT_NSCRIPTER_SAR)) {
            bResult = readTableArchive(fileType == FT_NSCRIPTER_NSA, nTotalSize, &parsed, pPdStruct);
        }
        if (bResult && owner) bResult = resolveCodecSizes(&parsed, pPdStruct);
    } catch (const std::bad_alloc &) {
        return false;
    }
    if (!bResult || !isPdStructNotCanceled(pPdStruct)) return false;

    *pContext = parsed;
    return true;
}

bool XNScripterArchive::readTableArchive(bool bNsa, qint64 nTotalSize, CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    const QByteArray baHeader = read_array_process(0, 6, pPdStruct);
    if ((baHeader.size() != 6)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    const qint32 nCount = be16(pHeader);
    const qint64 nBase = be32(pHeader + 2);
    // Smallest entry: one name byte, NUL, the fixed fields.
    const qint64 nMinEntry = bNsa ? (2 + 13) : (2 + 8);
    if ((nCount < 1) || (nCount > MaxMembers) || (nBase < 6 + (qint64)nCount * nMinEntry) || (nBase > nTotalSize) ||
        (nBase - 6 > MaxIndexSize)) {
        return false;
    }
    const qint64 nIndexSize = nBase - 6;
    const qint64 nDataSize = nTotalSize - nBase;

    qint64 nWindow = qMin(nIndexSize, ProbeWindow);
    QByteArray baIndex = read_array_process(6, nWindow, pPdStruct);
    if ((baIndex.size() != nWindow)) return false;
    PARSE_RESULT result = parseTable(reinterpret_cast<const uchar *>(baIndex.constData()), nWindow, nIndexSize, bNsa, nCount, nBase, nDataSize, pContext);
    if ((result == PARSE_NEED_MORE) && (nWindow < nIndexSize)) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        baIndex = read_array_process(6, nIndexSize, pPdStruct);
        if ((baIndex.size() != nIndexSize)) return false;
        pContext->listMembers.clear();
        result = parseTable(reinterpret_cast<const uchar *>(baIndex.constData()), nIndexSize, nIndexSize, bNsa, nCount, nBase, nDataSize, pContext);
    }
    return result == PARSE_OK;
}

XNScripterArchive::PARSE_RESULT XNScripterArchive::parseTable(const uchar *pIndex, qint64 nAvailable, qint64 nIndexSize, bool bNsa, qint32 nCount,
                                                              qint64 nBase, qint64 nDataSize, CONTEXT *pContext)
{
    const qint64 nFixed = bNsa ? 13 : 8;
    qint64 nPos = 0;
    qint64 nPrevOffset = 0;
    qint64 nMaxEnd = 0;
    QList<MEMBER> listMembers;
    for (qint32 i = 0; i < nCount; i++) {
        qint64 nNameEnd = -1;
        for (qint64 j = nPos; j < nAvailable; j++) {
            if (j - nPos > MaxNameLength) return PARSE_INVALID;
            if (pIndex[j] == 0) {
                nNameEnd = j;
                break;
            }
            if (!isNameByteAllowed(pIndex[j])) return PARSE_INVALID;
        }
        if (nNameEnd < 0) {
            return (nAvailable < nIndexSize) ? PARSE_NEED_MORE : PARSE_INVALID;
        }
        if (nNameEnd == nPos) return PARSE_INVALID;  // empty name
        if (nNameEnd + 1 + nFixed > nIndexSize) return PARSE_INVALID;
        if (nNameEnd + 1 + nFixed > nAvailable) return PARSE_NEED_MORE;
        const QByteArray baName(reinterpret_cast<const char *>(pIndex + nPos), (int)(nNameEnd - nPos));
        const uchar *pFixed = pIndex + nNameEnd + 1;
        MEMBER member;
        qint64 nOffset = 0;
        if (bNsa) {
            member.nCodec = pFixed[0];
            nOffset = be32(pFixed + 1);
            member.nPackedSize = be32(pFixed + 5);
            member.nUnpackedSize = be32(pFixed + 9);
            if ((member.nCodec != CODEC_NONE) && (member.nCodec != CODEC_SPB) && (member.nCodec != CODEC_LZSS) && (member.nCodec != CODEC_NBZ)) {
                return PARSE_INVALID;
            }
        } else {
            member.nCodec = CODEC_NONE;
            nOffset = be32(pFixed);
            member.nPackedSize = be32(pFixed + 4);
            member.nUnpackedSize = member.nPackedSize;
        }
        if ((nOffset < nPrevOffset) || (nOffset > nDataSize) || (member.nPackedSize > nDataSize - nOffset)) return PARSE_INVALID;
        nPrevOffset = nOffset;
        nMaxEnd = qMax(nMaxEnd, nOffset + member.nPackedSize);
        member.nDataOffset = nBase + nOffset;
        member.sName = decodeMemberName(baName);
        if (!isMemberNameSafe(member.sName)) return PARSE_INVALID;
        if (bNsa && (member.nCodec == CODEC_NONE) && member.sName.endsWith(QLatin1String(".nbz"), Qt::CaseInsensitive)) {
            member.nCodec = CODEC_NBZ;
        }
        if (member.nCodec == CODEC_NONE) member.nUnpackedSize = member.nPackedSize;
        listMembers.append(member);
        nPos = nNameEnd + 1 + nFixed;
    }
    // The table must end exactly at base and the data area must be exactly
    // covered: with no magic, this extent proof is the recognition.
    if (nPos != nIndexSize) return PARSE_INVALID;
    if (nMaxEnd != nDataSize) return PARSE_INVALID;
    pContext->nBase = nBase;
    pContext->nArchiveEnd = nBase + nDataSize;
    pContext->listMembers = listMembers;
    return PARSE_OK;
}

bool XNScripterArchive::readNs2Archive(qint64 nTotalSize, CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    const QByteArray baHeader = read_array_process(0, 4, pPdStruct);
    if ((baHeader.size() != 4)) return false;
    const qint64 nBase = le32(reinterpret_cast<const uchar *>(baHeader.constData()));
    // Smallest index: '"' name '"' size 'e' = 8 bytes after the base field.
    if ((nBase < 4 + 8) || (nBase > nTotalSize) || (nBase - 4 > MaxIndexSize)) return false;
    const qint64 nIndexSize = nBase - 4;
    const qint64 nDataSize = nTotalSize - nBase;

    qint64 nWindow = qMin(nIndexSize, ProbeWindow);
    QByteArray baIndex = read_array_process(4, nWindow, pPdStruct);
    if ((baIndex.size() != nWindow)) return false;
    PARSE_RESULT result = parseNs2Index(reinterpret_cast<const uchar *>(baIndex.constData()), nWindow, nIndexSize, nBase, nDataSize, pContext);
    if ((result == PARSE_NEED_MORE) && (nWindow < nIndexSize)) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        baIndex = read_array_process(4, nIndexSize, pPdStruct);
        if ((baIndex.size() != nIndexSize)) return false;
        pContext->listMembers.clear();
        result = parseNs2Index(reinterpret_cast<const uchar *>(baIndex.constData()), nIndexSize, nIndexSize, nBase, nDataSize, pContext);
    }
    return result == PARSE_OK;
}

XNScripterArchive::PARSE_RESULT XNScripterArchive::parseNs2Index(const uchar *pIndex, qint64 nAvailable, qint64 nIndexSize, qint64 nBase, qint64 nDataSize,
                                                                 CONTEXT *pContext)
{
    qint64 nPos = 0;
    qint64 nRunning = 0;
    QList<MEMBER> listMembers;
    while (nPos < nIndexSize - 1) {
        if (nPos >= nAvailable) return PARSE_NEED_MORE;
        if (pIndex[nPos] != '"') return PARSE_INVALID;
        qint64 nNameEnd = -1;
        for (qint64 j = nPos + 1; j < nAvailable; j++) {
            if (j - nPos - 1 > MaxNameLength) return PARSE_INVALID;
            if (pIndex[j] == '"') {
                nNameEnd = j;
                break;
            }
            if (!isNameByteAllowed(pIndex[j])) return PARSE_INVALID;
        }
        if (nNameEnd < 0) return (nAvailable < nIndexSize) ? PARSE_NEED_MORE : PARSE_INVALID;
        if (nNameEnd == nPos + 1) return PARSE_INVALID;
        if (nNameEnd + 1 + 4 > nIndexSize - 1) return PARSE_INVALID;
        if (nNameEnd + 1 + 4 > nAvailable) return PARSE_NEED_MORE;
        if (listMembers.size() >= MaxMembers) return PARSE_INVALID;
        const QByteArray baName(reinterpret_cast<const char *>(pIndex + nPos + 1), (int)(nNameEnd - nPos - 1));
        MEMBER member;
        member.nCodec = CODEC_NONE;
        member.nPackedSize = le32(pIndex + nNameEnd + 1);
        if (member.nPackedSize > nDataSize - nRunning) return PARSE_INVALID;
        member.nUnpackedSize = member.nPackedSize;
        member.nDataOffset = nBase + nRunning;
        member.sName = decodeMemberName(baName);
        if (!isMemberNameSafe(member.sName)) return PARSE_INVALID;
        nRunning += member.nPackedSize;
        listMembers.append(member);
        nPos = nNameEnd + 1 + 4;
    }
    if (nPos != nIndexSize - 1) return PARSE_INVALID;
    if (nIndexSize - 1 >= nAvailable) return PARSE_NEED_MORE;
    if (pIndex[nIndexSize - 1] != 'e') return PARSE_INVALID;
    if (listMembers.isEmpty() || (nRunning != nDataSize)) return PARSE_INVALID;
    pContext->nBase = nBase;
    pContext->nArchiveEnd = nBase + nDataSize;
    pContext->listMembers = listMembers;
    return PARSE_OK;
}

// NBZ and SPB members declare their real output size inside the data, not
// in the table (a *.nbz member with codec byte 0 carries its packed size in
// both table fields).  Read the four-byte prefixes so the listing is exact.
bool XNScripterArchive::resolveCodecSizes(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    const qint32 nCount = pContext->listMembers.size();
    for (qint32 i = 0; i < nCount; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        MEMBER &member = pContext->listMembers[i];
        if ((member.nCodec != CODEC_NBZ) && (member.nCodec != CODEC_SPB)) continue;
        member.nUnpackedSize = -1;
        if (member.nPackedSize < 4) continue;
        const QByteArray baPrefix = read_array_process(member.nDataOffset, 4, pPdStruct);
        if (baPrefix.size() != 4) continue;
        const uchar *pPrefix = reinterpret_cast<const uchar *>(baPrefix.constData());
        if (member.nCodec == CODEC_NBZ) {
            member.nUnpackedSize = be32(pPrefix);
        } else {
            qint64 nSize = -1;
            if (spbOutputSize(pPrefix, 4, &nSize)) member.nUnpackedSize = nSize;
        }
    }
    return true;
}

bool XNScripterArchive::spbOutputSize(const uchar *pHeader, qint64 nHeaderSize, qint64 *pnSize)
{
    if (nHeaderSize < 4) return false;
    const qint32 nWidth = be16(pHeader);
    const qint32 nHeight = be16(pHeader + 2);
    if ((nWidth < 1) || (nHeight < 1) || (nWidth > SpbMaxDimension) || (nHeight > SpbMaxDimension)) return false;
    const qint64 nStride = ((qint64)nWidth * 3 + 3) & ~(qint64)3;
    *pnSize = 54 + nStride * nHeight;
    return true;
}

// NScripter LZSS: 256-byte ring initialised to zero, write cursor starting
// at 239 (N - F with F = 17), MSB-first bit stream; flag 1 = 8-bit literal,
// flag 0 = 8-bit ring position + 4-bit (length - 2), copied byte by byte.
bool XNScripterArchive::decodeLzss(const QByteArray &baPacked, qint64 nUnpackedSize, QByteArray *pResult)
{
    if ((nUnpackedSize < 0) || (nUnpackedSize > MaxOutputSize)) return false;
    QByteArray baOut;
    baOut.resize((int)nUnpackedSize);
    uchar *pOut = reinterpret_cast<uchar *>(baOut.data());
    uchar ring[256];
    memset(ring, 0, sizeof(ring));
    quint32 nRingPos = 239;
    BIT_READER reader(reinterpret_cast<const uchar *>(baPacked.constData()), baPacked.size());
    qint64 nProduced = 0;
    while (nProduced < nUnpackedSize) {
        const quint32 nFlag = reader.read(1);
        if (reader.bError) return false;
        if (nFlag) {
            const uchar nByte = (uchar)reader.read(8);
            if (reader.bError) return false;
            pOut[nProduced++] = nByte;
            ring[nRingPos] = nByte;
            nRingPos = (nRingPos + 1) & 255;
        } else {
            const quint32 nSource = reader.read(8);
            const quint32 nLength = reader.read(4) + 2;
            if (reader.bError) return false;
            for (quint32 k = 0; (k < nLength) && (nProduced < nUnpackedSize); k++) {
                const uchar nByte = ring[(nSource + k) & 255];
                pOut[nProduced++] = nByte;
                ring[nRingPos] = nByte;
                nRingPos = (nRingPos + 1) & 255;
            }
        }
    }
    *pResult = baOut;
    return true;
}

// SPB: BE u16 width, BE u16 height, then three planes (B, G, R) of
// width*height values in serpentine scan order.  The first value of a plane
// is raw; then groups of four values selected by a 3-bit code n: 0 = repeat
// the current value four times, 7 = one extra bit chooses 1 or 2 delta bits,
// otherwise n + 2 delta bits (8 = raw byte).  A delta k adds (k >> 1) + 1
// when odd and subtracts k >> 1 when even.  Output is the bottom-up 24-bit
// BMP (54-byte header, biSizeImage 0) that the reference decoders write.
bool XNScripterArchive::decodeSpb(const QByteArray &baPacked, QByteArray *pResult)
{
    const uchar *pData = reinterpret_cast<const uchar *>(baPacked.constData());
    qint64 nTotalSize = 0;
    if ((baPacked.size() < 4) || !spbOutputSize(pData, 4, &nTotalSize) || (nTotalSize > MaxOutputSize)) return false;
    const qint32 nWidth = be16(pData);
    const qint32 nHeight = be16(pData + 2);
    const qint64 nStride = ((qint64)nWidth * 3 + 3) & ~(qint64)3;
    const qint64 nPixels = (qint64)nWidth * nHeight;

    QByteArray baOut((int)nTotalSize, 0);
    uchar *pOut = reinterpret_cast<uchar *>(baOut.data());
    pOut[0] = 'B';
    pOut[1] = 'M';
    qToLittleEndian<quint32>((quint32)nTotalSize, pOut + 2);
    qToLittleEndian<quint32>(54, pOut + 10);
    qToLittleEndian<quint32>(40, pOut + 14);
    qToLittleEndian<quint32>((quint32)nWidth, pOut + 18);
    qToLittleEndian<quint32>((quint32)nHeight, pOut + 22);
    qToLittleEndian<quint16>(1, pOut + 26);
    qToLittleEndian<quint16>(24, pOut + 28);

    BIT_READER reader(pData, baPacked.size());
    reader.read(32);  // width and height
    QByteArray baPlane;
    baPlane.resize((int)(nPixels + 4));
    uchar *pPlane = reinterpret_cast<uchar *>(baPlane.data());
    for (qint32 nPlaneIndex = 0; nPlaneIndex < 3; nPlaneIndex++) {
        qint64 nCount = 0;
        uchar nCurrent = (uchar)reader.read(8);
        if (reader.bError) return false;
        pPlane[nCount++] = nCurrent;
        while (nCount < nPixels) {
            const quint32 nCode = reader.read(3);
            if (reader.bError) return false;
            if (nCode == 0) {
                pPlane[nCount] = nCurrent;
                pPlane[nCount + 1] = nCurrent;
                pPlane[nCount + 2] = nCurrent;
                pPlane[nCount + 3] = nCurrent;
                nCount += 4;
                continue;
            }
            qint32 nBits = 0;
            if (nCode == 7) {
                nBits = (qint32)reader.read(1) + 1;
            } else {
                nBits = (qint32)nCode + 2;
            }
            for (qint32 j = 0; j < 4; j++) {
                if (nBits == 8) {
                    nCurrent = (uchar)reader.read(8);
                } else {
                    const quint32 nDelta = reader.read(nBits);
                    if (nDelta & 1) {
                        nCurrent = (uchar)(nCurrent + (nDelta >> 1) + 1);
                    } else {
                        nCurrent = (uchar)(nCurrent - (nDelta >> 1));
                    }
                }
                pPlane[nCount + j] = nCurrent;
            }
            if (reader.bError) return false;
            nCount += 4;
        }
        qint64 nSource = 0;
        for (qint32 y = 0; y < nHeight; y++) {
            uchar *pRow = pOut + 54 + nStride * (nHeight - 1 - y);
            if (y & 1) {
                for (qint32 x = nWidth - 1; x >= 0; x--) pRow[(qint64)x * 3 + nPlaneIndex] = pPlane[nSource++];
            } else {
                for (qint32 x = 0; x < nWidth; x++) pRow[(qint64)x * 3 + nPlaneIndex] = pPlane[nSource++];
            }
        }
    }
    *pResult = baOut;
    return true;
}

bool XNScripterArchive::decodeNbz(const QByteArray &baPacked, QByteArray *pResult, PDSTRUCT *pPdStruct)
{
    if (baPacked.size() < 4) return false;
    const qint64 nUnpackedSize = be32(reinterpret_cast<const uchar *>(baPacked.constData()));
    if (nUnpackedSize > MaxOutputSize) return false;
    QBuffer bufferIn;
    bufferIn.setData(baPacked.constData() + 4, baPacked.size() - 4);
    if (!bufferIn.open(QIODevice::ReadOnly)) return false;
    QByteArray baOut;
    QBuffer bufferOut(&baOut);
    if (!bufferOut.open(QIODevice::ReadWrite)) return false;
    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = &bufferIn;
    state.pDeviceOutput = &bufferOut;
    state.nInputOffset = 0;
    state.nInputLimit = bufferIn.size();
    state.nProcessedOffset = 0;
    state.nProcessedLimit = nUnpackedSize;
    state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nUnpackedSize);
    const bool bDecoded = XBZIP2Decoder::decompress(&state, pPdStruct);
    bufferOut.close();
    bufferIn.close();
    if (!bDecoded || state.bReadError || state.bWriteError || (baOut.size() != nUnpackedSize)) return false;
    *pResult = baOut;
    return true;
}

bool XNScripterArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !pState || ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState))) return false;
    CONTEXT *pOld = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    delete pOld;
    *pState = UNPACK_STATE();
    if (!isPdStructNotCanceled(pPdStruct)) return false;
    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!bBound) return false;
    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    OUTPUT_POLICY policy = {};
    XNScripterArchive *owner = this;
    const bool bValid = pContext && resolveUnpackOutputPolicy(mapProperties, &policy) && readContext(m_fileType, pContext, pPdStruct);
    if (!owner) {
        delete pContext;
        return false;
    }
    if (!bValid) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    pState->pContext = pContext;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->nCurrentIndex = 0;
    pState->nTotalSize = pContext->nArchiveEnd;
    pState->mapUnpackProperties = mapProperties;
    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XNScripterArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    ARCHIVERECORD record = {};
    if (!guard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct)) return record;
    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    const qint32 nCount = pContext->listMembers.size();
    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= nCount) || (pState->nNumberOfRecords != nCount)) return record;
    const MEMBER member = pContext->listMembers.at(pState->nCurrentIndex);
    record.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sName);
    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUnpackedSize);
    record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
    record.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    QString sMethod = QStringLiteral("Store");
    if (member.nCodec == CODEC_LZSS) sMethod = QStringLiteral("NScripter LZSS");
    else if (member.nCodec == CODEC_NBZ) sMethod = QStringLiteral("NBZ (BZip2)");
    else if (member.nCodec == CODEC_SPB) sMethod = QStringLiteral("SPB image");
    record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, sMethod);
    if (!markArchiveStreamRecord(&record, pState->nCurrentIndex)) return ARCHIVERECORD();
    return record;
}

bool XNScripterArchive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QIODevice *source = getDevice();
    QIODevice *output = pDevice;
    XNScripterArchive *owner = this;
    if (!guard.isAcquired() || !pState || !pState->pContext || !source || !output || !isUnpackSourceCurrent(pState, pPdStruct) || !source ||
        !output) {
        return false;
    }
    const bool bSupported = isUnpackOutputSupported(output);
    if (!source || !output || !bSupported) return false;
    const bool bAliases = devicesAlias(source, output);
    if (!source || !output || bAliases) return false;
    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    const qint32 nCount = pContext->listMembers.size();
    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= nCount) || (pState->nNumberOfRecords != nCount)) return false;
    const MEMBER member = pContext->listMembers.at(pState->nCurrentIndex);
    const QMap<UNPACK_PROP, QVariant> mapProperties = pState->mapUnpackProperties;
    const QSharedPointer<OUTPUT_BUDGET> spBudget = pState->spOutputBudget;
    // Every codec here declares its output size up front; a member whose
    // size is not knowable (bad NBZ/SPB prefix) fails closed.
    if ((member.nUnpackedSize < 0) || (member.nUnpackedSize > MaxOutputSize) || (member.nPackedSize > MaxPackedSize)) return false;
    OUTPUT_POLICY policy = {};
    if (!resolveUnpackOutputPolicy(mapProperties, &policy) || ((policy.nMaxEntryOutputSize >= 0) && (member.nUnpackedSize > policy.nMaxEntryOutputSize))) {
        return false;
    }
    if (spBudget && !spBudget->beginEntry(pState->nCurrentIndex, member.sName)) {
        if (spBudget->isEnforcing()) return false;
        OUTPUT_BUDGET::noteShadowRefusal(spBudget.data());
    }
    if (spBudget && spBudget->isEnforcing() && (spBudget->totalLimit() >= 0) &&
        ((spBudget->totalWritten() > spBudget->totalLimit()) || (member.nUnpackedSize > spBudget->totalLimit() - spBudget->totalWritten()))) {
        return false;
    }

    const NSCRIPTER_CANCELED canceled(owner, source, output, pPdStruct);
    QByteArray baDecoded;
    try {
        const QByteArray baPacked = read_array_process(member.nDataOffset, member.nPackedSize, pPdStruct);
        if (canceled() || (baPacked.size() != member.nPackedSize)) return false;
        if (member.nCodec == CODEC_NONE) {
            baDecoded = baPacked;
        } else if (member.nCodec == CODEC_LZSS) {
            if (!decodeLzss(baPacked, member.nUnpackedSize, &baDecoded)) return false;
        } else if (member.nCodec == CODEC_NBZ) {
            if (!decodeNbz(baPacked, &baDecoded, pPdStruct)) return false;
        } else if (member.nCodec == CODEC_SPB) {
            if (!decodeSpb(baPacked, &baDecoded)) return false;
        } else {
            return false;
        }
    } catch (const std::bad_alloc &) {
        return false;
    }
    if (canceled() || (baDecoded.size() != member.nUnpackedSize)) return false;

    QTemporaryFile stage;
    if (!stage.open()) return false;
    DATAPROCESS_STATE writer = {};
    writer.pDeviceOutput = &stage;
    writer.nProcessedLimit = -1;
    writer.mapUnpackProperties = mapProperties;
    writer.spOutputBudget = spBudget;
    const qint64 nSize = baDecoded.size();
    for (qint64 nDone = 0; nDone < nSize;) {
        const qint32 nTake = (qint32)qMin<qint64>(65536, nSize - nDone);
        if (canceled() || (XBinary::_writeDevice(baDecoded.constData() + nDone, nTake, &writer) != nTake)) return false;
        nDone += nTake;
    }
    if (canceled() || (stage.size() != member.nUnpackedSize) || !stage.flush() || !stage.seek(0) || !isUnpackSourceCurrent(pState, pPdStruct) || canceled()) {
        return false;
    }
    const bool bPublished = publishUnpackOutput(&stage, output, pState, pPdStruct);
    return output && bPublished;
}

bool XNScripterArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct)) return false;
    const qint64 nCount = static_cast<CONTEXT *>(pState->pContext)->listMembers.size();
    if ((pState->nNumberOfRecords != nCount) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= nCount)) return false;
    ++pState->nCurrentIndex;
    return pState->nCurrentIndex < nCount;
}

bool XNScripterArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !pState || ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState))) return false;
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    pState->pContext = nullptr;
    releaseUnpackSource(pState);
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
