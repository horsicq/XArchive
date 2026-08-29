// DIE's modification 2017-2025
// Copyright (c) 2020-2026 hors<horsicq@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include "xdmg.h"

#include <limits>
#include <new>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTemporaryFile>
#include <QXmlStreamReader>
#include <zlib.h>

#include "Algos/xbzip2decoder.h"
#include "subdevice.h"

#ifdef Q_OS_WIN
#include <io.h>
#include <windows.h>
#elif defined(Q_OS_UNIX)
#include <sys/stat.h>
#endif

namespace {
const quint64 DMG_SECTOR_SIZE = 512;
const quint64 DMG_MAX_XML_SIZE = 64ULL * 1024ULL * 1024ULL;
const quint64 DMG_MAX_MISH_SIZE = 64ULL * 1024ULL * 1024ULL;
const quint64 DMG_MAX_RESOURCE_SIZE = 1ULL << 24;
const qint32 DMG_MAX_PARTITIONS = 65536;
const quint32 DMG_MAX_STRIPES_PER_PARTITION = 1000000;
const qint32 DMG_MAX_XML_DEPTH = 64;
const qint32 DMG_MAX_XML_BASE_CANDIDATES = 4096;

QString dmgFromMacRoman(const char *pData, qint32 nSize)
{
    static const ushort anMacRomanHigh[128] = {
        0x00C4, 0x00C5, 0x00C7, 0x00C9, 0x00D1, 0x00D6, 0x00DC, 0x00E1, 0x00E0, 0x00E2, 0x00E4, 0x00E3, 0x00E5, 0x00E7, 0x00E9, 0x00E8, 0x00EA, 0x00EB, 0x00ED,
        0x00EC, 0x00EE, 0x00EF, 0x00F1, 0x00F3, 0x00F2, 0x00F4, 0x00F6, 0x00F5, 0x00FA, 0x00F9, 0x00FB, 0x00FC, 0x2020, 0x00B0, 0x00A2, 0x00A3, 0x00A7, 0x2022,
        0x00B6, 0x00DF, 0x00AE, 0x00A9, 0x2122, 0x00B4, 0x00A8, 0x2260, 0x00C6, 0x00D8, 0x221E, 0x00B1, 0x2264, 0x2265, 0x00A5, 0x00B5, 0x2202, 0x2211, 0x220F,
        0x03C0, 0x222B, 0x00AA, 0x00BA, 0x03A9, 0x00E6, 0x00F8, 0x00BF, 0x00A1, 0x00AC, 0x221A, 0x0192, 0x2248, 0x2206, 0x00AB, 0x00BB, 0x2026, 0x00A0, 0x00C0,
        0x00C3, 0x00D5, 0x0152, 0x0153, 0x2013, 0x2014, 0x201C, 0x201D, 0x2018, 0x2019, 0x00F7, 0x25CA, 0x00FF, 0x0178, 0x2044, 0x20AC, 0x2039, 0x203A, 0xFB01,
        0xFB02, 0x2021, 0x00B7, 0x201A, 0x201E, 0x2030, 0x00C2, 0x00CA, 0x00C1, 0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF, 0x00CC, 0x00D3, 0x00D4, 0xF8FF, 0x00D2,
        0x00DA, 0x00DB, 0x00D9, 0x0131, 0x02C6, 0x02DC, 0x00AF, 0x02D8, 0x02D9, 0x02DA, 0x00B8, 0x02DD, 0x02DB, 0x02C7};

    QString sResult;
    sResult.reserve(nSize);

    for (qint32 i = 0; i < nSize; i++) {
        const uchar nValue = (uchar)pData[i];
        sResult.append(nValue < 0x80 ? QChar(nValue) : QChar(anMacRomanHigh[nValue - 0x80]));
    }

    return sResult;
}

bool dmgRangeWithin(quint64 nOffset, quint64 nLength, quint64 nContainerSize)
{
    return (nOffset <= nContainerSize) && (nLength <= (nContainerSize - nOffset));
}

class DMGXmlRangeDevice : public QIODevice {
public:
    DMGXmlRangeDevice(QIODevice *pDevice, qint64 nOffset, qint64 nLength, XBinary::PDSTRUCT *pPdStruct)
        : m_pDevice(pDevice), m_nOffset(nOffset), m_nLength(nLength), m_nPosition(0), m_pPdStruct(pPdStruct)
    {
    }

    bool isSequential() const override
    {
        return true;
    }

    qint64 bytesAvailable() const override
    {
        return qMax<qint64>(0, m_nLength - m_nPosition) + QIODevice::bytesAvailable();
    }

protected:
    qint64 readData(char *pData, qint64 nMaxSize) override
    {
        if (!m_pDevice || (nMaxSize < 0) || ((nMaxSize > 0) && !pData) || !XBinary::isPdStructNotCanceled(m_pPdStruct)) {
            return -1;
        }
        if ((nMaxSize == 0) || (m_nPosition == m_nLength)) return 0;

        // Keep XML prolog discovery lazy and give cancellation a bounded
        // polling interval even if QXmlStreamReader asks for a large buffer.
        const qint64 nReadSize = qMin<qint64>(0x10000, qMin(nMaxSize, m_nLength - m_nPosition));
        const qint64 nRead = XBinary::read_array_process(m_pDevice.data(), m_nOffset + m_nPosition, pData, nReadSize, m_pPdStruct);
        if (!m_pDevice || (nRead != nReadSize)) return -1;
        m_nPosition += nRead;
        return nRead;
    }

    qint64 writeData(const char *, qint64) override
    {
        return -1;
    }

private:
    QPointer<QIODevice> m_pDevice;
    qint64 m_nOffset;
    qint64 m_nLength;
    qint64 m_nPosition;
    XBinary::PDSTRUCT *m_pPdStruct;
};

bool dmgHasUnnamespacedPlistRoot(QIODevice *pDevice, qint64 nOffset, qint64 nLength, XBinary::PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice || (nOffset < 0) || (nLength <= 0) || ((quint64)nLength > DMG_MAX_XML_SIZE) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const bool bOpen = guardedDevice->isOpen();
    if (!guardedDevice || !bOpen) return false;
    const bool bReadable = guardedDevice->isReadable();
    if (!guardedDevice || !bReadable) return false;
    const qint64 nDeviceSize = guardedDevice->size();
    if (!guardedDevice || (nDeviceSize < 0) || (nOffset > nDeviceSize) || (nLength > nDeviceSize - nOffset)) {
        return false;
    }

    DMGXmlRangeDevice xmlRange(guardedDevice.data(), nOffset, nLength, pPdStruct);
    if (!xmlRange.open(QIODevice::ReadOnly)) return false;

    QXmlStreamReader reader(&xmlRange);
    bool bDtdSeen = false;
    while (!reader.atEnd() && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const QXmlStreamReader::TokenType token = reader.readNext();
        if (!guardedDevice) return false;
        if (token == QXmlStreamReader::DTD) {
            const QString sPublicId = reader.dtdPublicId().toString();
            const QString sSystemId = reader.dtdSystemId().toString();
            const bool bNoExternalId = sPublicId.isEmpty() && sSystemId.isEmpty();
            const bool bApplePlistId =
                ((sPublicId == QLatin1String("-//Apple//DTD PLIST 1.0//EN")) || (sPublicId == QLatin1String("-//Apple Computer//DTD PLIST 1.0//EN"))) &&
                (sSystemId == QLatin1String("http://www.apple.com/DTDs/PropertyList-1.0.dtd"));
            if (bDtdSeen || (reader.dtdName() != QLatin1String("plist")) || !reader.entityDeclarations().isEmpty() || !reader.notationDeclarations().isEmpty() ||
                (!bNoExternalId && !bApplePlistId)) {
                return false;
            }
            bDtdSeen = true;
        } else if (token == QXmlStreamReader::StartElement) {
            return reader.name() == QLatin1String("plist") && reader.prefix().isEmpty() && reader.namespaceUri().isEmpty();
        } else if ((token == QXmlStreamReader::Characters) && !reader.isWhitespace()) {
            return false;
        } else if ((token == QXmlStreamReader::EndElement) || (token == QXmlStreamReader::EntityReference) || (token == QXmlStreamReader::Invalid)) {
            return false;
        }
    }

    return false;
}

bool dmgChecksumDescriptorValid(const quint32 *pChecksum)
{
    if (!pChecksum || (pChecksum[1] > 1024)) return false;
    if (pChecksum[0] == 0) return pChecksum[1] == 0;
    if (pChecksum[0] == 2) return pChecksum[1] == 32;
    return true;
}

bool dmgChecksumIsCRC32(const quint32 *pChecksum)
{
    return dmgChecksumDescriptorValid(pChecksum) && (pChecksum[0] == 2) && (pChecksum[1] == 32);
}

quint32 dmgCrcMatrixTimes(const quint32 *pMatrix, quint32 nVector)
{
    quint32 nResult = 0;
    while (nVector != 0) {
        if (nVector & 1U) nResult ^= *pMatrix;
        nVector >>= 1;
        pMatrix++;
    }
    return nResult;
}

void dmgCrcMatrixSquare(quint32 *pSquare, const quint32 *pMatrix)
{
    for (qint32 i = 0; i < 32; i++) {
        pSquare[i] = dmgCrcMatrixTimes(pMatrix, pMatrix[i]);
    }
}

quint32 dmgCombineCRC32(quint32 nCRC1, quint32 nCRC2, quint64 nLength2)
{
    if (nLength2 == 0) return nCRC1;

    quint32 odd[32] = {};
    quint32 even[32] = {};
    odd[0] = 0xEDB88320U;
    quint32 nRow = 1;
    for (qint32 i = 1; i < 32; i++) {
        odd[i] = nRow;
        nRow <<= 1;
    }

    dmgCrcMatrixSquare(even, odd);
    dmgCrcMatrixSquare(odd, even);
    do {
        dmgCrcMatrixSquare(even, odd);
        if (nLength2 & 1ULL) nCRC1 = dmgCrcMatrixTimes(even, nCRC1);
        nLength2 >>= 1;
        if (nLength2 == 0) break;

        dmgCrcMatrixSquare(odd, even);
        if (nLength2 & 1ULL) nCRC1 = dmgCrcMatrixTimes(odd, nCRC1);
        nLength2 >>= 1;
    } while (nLength2 != 0);

    return nCRC1 ^ nCRC2;
}

quint32 dmgUpdateCRC32(quint32 nCRC, const char *pData, qint32 nSize)
{
    // XBinary exposes the running (uncomplemented) state, while the prefix
    // index stores zlib-compatible finalized CRCs so they can be combined.
    return XBinary::_getCRC32(pData, nSize, nCRC ^ 0xFFFFFFFFU, XBinary::_getCRC32Table_EDB88320()) ^ 0xFFFFFFFFU;
}

bool dmgHasFullPartitionCRC(const QList<XDMG::BLOCK_DATA> &listStripes)
{
    for (qint32 i = 0; i < listStripes.size(); i++) {
        if (listStripes.at(i).nType == XDMG::DMG_STRIPE_ZEROES) return false;
    }
    return true;
}

struct DMG_NAME_STATE {
    QSet<QString> stUsedNames;
    quint64 nNextSuffix = 2;
};

bool dmgTruncatePortable(const QString &sValue, qint32 nMaxUnits, qint32 nMaxUtf8Bytes, XBinary::PDSTRUCT *pPdStruct, QString *pResult)
{
    if (pResult) pResult->clear();
    if (!pResult || (nMaxUnits < 0) || (nMaxUtf8Bytes < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    // Count UTF-8 bytes while walking at most the accepted UTF-16 prefix.
    // Re-encoding the complete string after every one-unit chop made a legal
    // 4096-unit plist name quadratic before it reached this small public cap.
    qint32 nUnits = 0;
    qint32 nUtf8Bytes = 0;
    while ((nUnits < sValue.size()) && (nUnits < nMaxUnits)) {
        if (((nUnits & 0x3F) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const QChar cFirst = sValue.at(nUnits);
        qint32 nCharacterUnits = 1;
        qint32 nCharacterBytes = 0;
        if (cFirst.isHighSurrogate() && ((nUnits + 1) < sValue.size()) && sValue.at(nUnits + 1).isLowSurrogate()) {
            nCharacterUnits = 2;
            nCharacterBytes = 4;
        } else if (cFirst.unicode() < 0x80) {
            nCharacterBytes = 1;
        } else if (cFirst.unicode() < 0x800) {
            nCharacterBytes = 2;
        } else {
            // This also gives an isolated surrogate the three-byte replacement
            // budget used by Qt's UTF-8 conversion.  XML input itself cannot
            // contain an isolated surrogate.
            nCharacterBytes = 3;
        }

        if ((nCharacterUnits > (nMaxUnits - nUnits)) || (nCharacterBytes > (nMaxUtf8Bytes - nUtf8Bytes))) {
            break;
        }
        nUnits += nCharacterUnits;
        nUtf8Bytes += nCharacterBytes;
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    *pResult = sValue.left(nUnits);
    return true;
}

bool dmgSanitizePartitionName(QString sName, qint32 nIndex, DMG_NAME_STATE *pNameState, XBinary::PDSTRUCT *pPdStruct, QString *pResult)
{
    if (pResult) pResult->clear();
    if (!pResult || !pNameState || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    sName = sName.normalized(QString::NormalizationForm_C).trimmed();
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    for (qint32 i = 0; i < sName.size(); i++) {
        if (((i & 0xFF) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const QChar cValue = sName.at(i);
        if ((cValue.unicode() < 0x20) || (cValue.unicode() == 0x7F) || QStringLiteral("<>:\"/\\|?*").contains(cValue)) {
            sName[i] = QLatin1Char('_');
        }
    }
    while (sName.endsWith(QLatin1Char(' ')) || sName.endsWith(QLatin1Char('.'))) sName.chop(1);

    if ((sName == QLatin1String(".")) || (sName == QLatin1String(".."))) sName.clear();
    if (sName.isEmpty()) sName = QStringLiteral("partition%1").arg(nIndex);

    const qint32 nDot = sName.indexOf(QLatin1Char('.'));
    const QString sStem = (nDot < 0) ? sName : sName.left(nDot);
    const QString sUpperStem = sStem.toUpper();
    bool bReservedDevice = (sUpperStem == QLatin1String("CON")) || (sUpperStem == QLatin1String("PRN")) || (sUpperStem == QLatin1String("AUX")) ||
                           (sUpperStem == QLatin1String("NUL")) || (sUpperStem == QLatin1String("CLOCK$")) || (sUpperStem == QLatin1String("CONIN$")) ||
                           (sUpperStem == QLatin1String("CONOUT$"));
    if (!bReservedDevice && (sUpperStem.size() == 4) && (sUpperStem.startsWith(QLatin1String("COM")) || sUpperStem.startsWith(QLatin1String("LPT")))) {
        const QChar cIndex = sUpperStem.at(3);
        bReservedDevice = ((cIndex >= QLatin1Char('1')) && (cIndex <= QLatin1Char('9'))) || (cIndex.unicode() == 0x00B9) || (cIndex.unicode() == 0x00B2) ||
                          (cIndex.unicode() == 0x00B3);
    }
    if (bReservedDevice) sName.prepend(QLatin1Char('_'));

    // Reserve four units/bytes for the public ".img" suffix and keep every
    // filesystem component within both UTF-16 and UTF-8 portable limits.
    const qint32 nMaxNameLength = 240;
    const qint32 nMaxNameUtf8Bytes = 251;
    QString sTruncatedName;
    if (!dmgTruncatePortable(sName, nMaxNameLength, nMaxNameUtf8Bytes, pPdStruct, &sTruncatedName)) {
        return false;
    }
    sName = sTruncatedName;
    while (sName.endsWith(QLatin1Char(' ')) || sName.endsWith(QLatin1Char('.'))) sName.chop(1);
    if (sName.isEmpty()) sName = QStringLiteral("partition%1").arg(nIndex);

    const QString sBaseName = sName;
    QString sFoldedName = sName.toCaseFolded();
    while (pNameState->stUsedNames.contains(sFoldedName)) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) || (pNameState->nNextSuffix == (std::numeric_limits<quint64>::max)())) {
            return false;
        }

        // The suffix sequence is monotonic for the complete metadata batch,
        // so one already-used candidate can never be probed again by another
        // colliding base name.  Total QSet probes therefore remain linear even
        // when input names preoccupy generated suffixes.
        const QString sSuffix = QStringLiteral("_%1").arg(pNameState->nNextSuffix++);
        const qint32 nBaseLength = qMax(1, nMaxNameLength - sSuffix.size());
        const qint32 nBaseBytes = qMax(1, nMaxNameUtf8Bytes - sSuffix.size());
        QString sCandidateBase;
        if (!dmgTruncatePortable(sBaseName, nBaseLength, nBaseBytes, pPdStruct, &sCandidateBase)) {
            return false;
        }
        sName = sCandidateBase + sSuffix;
        sFoldedName = sName.toCaseFolded();
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    pNameState->stUsedNames.insert(sFoldedName);

    *pResult = sName;
    return true;
}

bool dmgCalculateCRC32(XBinary *pBinary, qint64 nOffset, qint64 nSize, XBinary::PDSTRUCT *pPdStruct, quint32 *pCRC32)
{
    if (pCRC32) *pCRC32 = 0;
    if (!pBinary || !pCRC32 || (nOffset < 0) || (nSize < 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XBinary> guardedBinary(pBinary);
    QByteArray baBuffer(0x10000, 0);
    quint32 nCRC = 0xFFFFFFFFU;
    qint64 nDone = 0;
    while ((nDone < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nChunk = (qint32)qMin<qint64>(baBuffer.size(), nSize - nDone);
        const qint64 nRead = guardedBinary->read_array_process(nOffset + nDone, baBuffer.data(), nChunk, pPdStruct);
        if (!guardedBinary || (nRead != nChunk)) {
            return false;
        }
        nCRC = XBinary::_getCRC32(baBuffer.constData(), nChunk, nCRC, XBinary::_getCRC32Table_EDB88320());
        nDone += nChunk;
    }

    if (!guardedBinary || (nDone != nSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    *pCRC32 = nCRC ^ 0xFFFFFFFFU;
    return true;
}

bool dmgWriteAll(QIODevice *pDevice, const char *pData, qint64 nSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDevice || (nSize < 0) || ((nSize > 0) && !pData)) return false;

    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice) return false;
    const bool bSeekable = !guardedDevice->isSequential();
    if (!guardedDevice) return false;
    const qint64 nStart = bSeekable ? guardedDevice->pos() : -1;
    const qint64 nMax = (std::numeric_limits<qint64>::max)();
    if (!guardedDevice || (bSeekable && (nStart < 0))) return false;

    qint64 nDone = 0;
    while ((nDone < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (!guardedDevice) return false;
        if (bSeekable) {
            if (nDone > nMax - nStart) return false;
            const bool bSeeked = guardedDevice->seek(nStart + nDone);
            if (!guardedDevice || !bSeeked) return false;
        }
        const qint64 nWritten = guardedDevice->write(pData + nDone, nSize - nDone);
        if (!guardedDevice) return false;
        if ((nWritten <= 0) || (nWritten > (nSize - nDone))) return false;
        nDone += nWritten;
    }
    if (!guardedDevice || (nDone != nSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (bSeekable) {
        if (nSize > nMax - nStart) return false;
        const bool bSeeked = guardedDevice->seek(nStart + nSize);
        if (!guardedDevice || !bSeeked) return false;
    }
    return true;
}

void dmgRollbackWrite(QIODevice *pDevice, qint64 nStartPosition)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    if (guardedDevice && !guardedDevice->isSequential() && guardedDevice && (nStartPosition >= 0) && XBinary::isResizeEnable(guardedDevice.data()) && guardedDevice) {
        XBinary::resize(guardedDevice.data(), nStartPosition);
        if (guardedDevice) guardedDevice->seek(nStartPosition);
    }
}

QIODevice *dmgUnwrapDevice(QIODevice *pDevice)
{
    QSet<QIODevice *> stVisited;
    while (pDevice && !stVisited.contains(pDevice)) {
        stVisited.insert(pDevice);
        SubDevice *pSubDevice = dynamic_cast<SubDevice *>(pDevice);
        if (!pSubDevice) break;
        QIODevice *pOriginal = pSubDevice->getOrigDevice();
        if (!pOriginal || (pOriginal == pDevice)) break;
        pDevice = pOriginal;
    }
    return pDevice;
}

bool dmgDevicesAlias(QIODevice *pSource, QIODevice *pDestination)
{
    pSource = dmgUnwrapDevice(pSource);
    pDestination = dmgUnwrapDevice(pDestination);
    if (!pSource || !pDestination) return false;
    QPointer<QIODevice> guardedSource(pSource);
    QPointer<QIODevice> guardedDestination(pDestination);
    if (pSource == pDestination) return true;

    QBuffer *pSourceBuffer = dynamic_cast<QBuffer *>(guardedSource.data());
    QBuffer *pDestinationBuffer = dynamic_cast<QBuffer *>(guardedDestination.data());
    if (pSourceBuffer && pDestinationBuffer) {
        QPointer<QBuffer> guardedSourceBuffer(pSourceBuffer);
        QPointer<QBuffer> guardedDestinationBuffer(pDestinationBuffer);
        if (!guardedSourceBuffer || !guardedDestinationBuffer) return true;
        QByteArray *pSourceBacking = &guardedSourceBuffer->buffer();
        if (!guardedSourceBuffer || !guardedDestinationBuffer) return true;
        QByteArray *pDestinationBacking = &guardedDestinationBuffer->buffer();
        if (!guardedSourceBuffer || !guardedDestinationBuffer) return true;
        if (pSourceBacking == pDestinationBacking) return true;
    }

    QFile *pSourceFile = dynamic_cast<QFile *>(guardedSource.data());
    QFile *pDestinationFile = dynamic_cast<QFile *>(guardedDestination.data());
    if (!pSourceFile || !pDestinationFile) return false;
    QPointer<QFile> guardedSourceFile(pSourceFile);
    QPointer<QFile> guardedDestinationFile(pDestinationFile);
    if (!guardedSourceFile || !guardedDestinationFile) return true;

    const QString sSourceFileName = guardedSourceFile->fileName();
    if (!guardedSourceFile || !guardedDestinationFile) return true;
    const QString sDestinationFileName = guardedDestinationFile->fileName();
    if (!guardedSourceFile || !guardedDestinationFile) return true;
    const QFileInfo sourceInfo(sSourceFileName);
    const QFileInfo destinationInfo(sDestinationFileName);
    QString sSourcePath = sourceInfo.canonicalFilePath();
    QString sDestinationPath = destinationInfo.canonicalFilePath();
    if (sSourcePath.isEmpty()) sSourcePath = QDir::cleanPath(sourceInfo.absoluteFilePath());
    if (sDestinationPath.isEmpty()) sDestinationPath = QDir::cleanPath(destinationInfo.absoluteFilePath());
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    const Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
#endif
    if (!sSourcePath.isEmpty() && !sDestinationPath.isEmpty() &&
        (QString::compare(QDir::fromNativeSeparators(sSourcePath), QDir::fromNativeSeparators(sDestinationPath), caseSensitivity) == 0)) {
        return true;
    }

    const int nSourceFileHandle = guardedSourceFile->handle();
    if (!guardedSourceFile || !guardedDestinationFile) return true;
    const int nDestinationFileHandle = guardedDestinationFile->handle();
    if (!guardedSourceFile || !guardedDestinationFile) return true;
    if ((nSourceFileHandle < 0) || (nDestinationFileHandle < 0)) return false;
#ifdef Q_OS_WIN
    const intptr_t nSourceHandle = _get_osfhandle(nSourceFileHandle);
    const intptr_t nDestinationHandle = _get_osfhandle(nDestinationFileHandle);
    if ((nSourceHandle == -1) || (nDestinationHandle == -1)) return false;
    BY_HANDLE_FILE_INFORMATION sourceFileInformation = {};
    BY_HANDLE_FILE_INFORMATION destinationFileInformation = {};
    return GetFileInformationByHandle(reinterpret_cast<HANDLE>(nSourceHandle), &sourceFileInformation) &&
           GetFileInformationByHandle(reinterpret_cast<HANDLE>(nDestinationHandle), &destinationFileInformation) &&
           (sourceFileInformation.dwVolumeSerialNumber == destinationFileInformation.dwVolumeSerialNumber) &&
           (sourceFileInformation.nFileIndexHigh == destinationFileInformation.nFileIndexHigh) &&
           (sourceFileInformation.nFileIndexLow == destinationFileInformation.nFileIndexLow);
#elif defined(Q_OS_UNIX)
    struct stat sourceStatus = {};
    struct stat destinationStatus = {};
    return (fstat(nSourceFileHandle, &sourceStatus) == 0) && (fstat(nDestinationFileHandle, &destinationStatus) == 0) &&
           (sourceStatus.st_dev == destinationStatus.st_dev) && (sourceStatus.st_ino == destinationStatus.st_ino);
#else
    return false;
#endif
}

bool dmgGetPartitionStorageInfo(const QList<XDMG::BLOCK_DATA> &listStripes, qint64 *pCompressedSize, XBinary::HANDLE_METHOD *pMethod)
{
    if (pCompressedSize) *pCompressedSize = 0;
    if (pMethod) *pMethod = XBinary::HANDLE_METHOD_UNKNOWN;
    if (!pCompressedSize || !pMethod) return false;

    quint64 nCompressedSize = 0;
    XBinary::HANDLE_METHOD method = XBinary::HANDLE_METHOD_UNKNOWN;
    bool bPhysicalDataSeen = false;
    bool bDataMethodSeen = false;
    bool bMethodIsKnown = true;

    for (qint32 i = 0; i < listStripes.size(); i++) {
        const XDMG::BLOCK_DATA &stripe = listStripes.at(i);
        XBinary::HANDLE_METHOD currentMethod = XBinary::HANDLE_METHOD_UNKNOWN;
        bool bHasPhysicalData = false;

        switch (stripe.nType) {
            case XDMG::DMG_STRIPE_EMPTY:
            case XDMG::DMG_STRIPE_ZEROES:
            case XDMG::DMG_STRIPE_SKIP:
            case XDMG::DMG_STRIPE_END: break;

            case XDMG::DMG_STRIPE_STORED:
                currentMethod = XBinary::HANDLE_METHOD_STORE;
                bHasPhysicalData = true;
                break;

            case XDMG::DMG_STRIPE_DEFLATE:
                currentMethod = XBinary::HANDLE_METHOD_ZLIB;
                bHasPhysicalData = true;
                break;

            case XDMG::DMG_STRIPE_BZ:
                currentMethod = XBinary::HANDLE_METHOD_BZIP2;
                bHasPhysicalData = true;
                break;

            case XDMG::DMG_STRIPE_ADC:
            case XDMG::DMG_STRIPE_LZFSE:
            case XDMG::DMG_STRIPE_XZ:
                bHasPhysicalData = true;
                bMethodIsKnown = false;
                break;

            default: return false;
        }

        if (!bHasPhysicalData) continue;
        bPhysicalDataSeen = true;
        if (stripe.nDataLength > ((quint64)(std::numeric_limits<qint64>::max)() - nCompressedSize)) {
            return false;
        }
        nCompressedSize += stripe.nDataLength;

        if (currentMethod == XBinary::HANDLE_METHOD_UNKNOWN) continue;
        if (!bDataMethodSeen) {
            method = currentMethod;
            bDataMethodSeen = true;
        } else if (method != currentMethod) {
            bMethodIsKnown = false;
        }
    }

    *pCompressedSize = (qint64)nCompressedSize;
    *pMethod = !bPhysicalDataSeen ? XBinary::HANDLE_METHOD_STORE : ((bMethodIsKnown && bDataMethodSeen) ? method : XBinary::HANDLE_METHOD_UNKNOWN);
    return true;
}

bool dmgDecodeBase64(const QString &sValue, QByteArray *pResult)
{
    if (!pResult) return false;
    pResult->clear();

    QByteArray baEncoded;
    baEncoded.reserve(sValue.size());
    for (QChar cValue : sValue) {
        const ushort nValue = cValue.unicode();
        if ((nValue == ' ') || (nValue == '\t') || (nValue == '\r') || (nValue == '\n')) continue;
        if (nValue > 0x7F) return false;
        baEncoded.append((char)nValue);
        if ((quint64)baEncoded.size() > ((DMG_MAX_MISH_SIZE * 4ULL) / 3ULL + 4ULL)) return false;
    }

    if (baEncoded.isEmpty() || ((baEncoded.size() & 3) != 0)) return false;

    qint32 nPadding = 0;
    for (qint32 i = 0; i < baEncoded.size(); i++) {
        const char cValue = baEncoded.at(i);
        const bool bAlphabet =
            ((cValue >= 'A') && (cValue <= 'Z')) || ((cValue >= 'a') && (cValue <= 'z')) || ((cValue >= '0') && (cValue <= '9')) || (cValue == '+') || (cValue == '/');
        if (cValue == '=') {
            nPadding++;
            if ((nPadding > 2) || (i < (baEncoded.size() - 2))) return false;
        } else if (!bAlphabet || (nPadding != 0)) {
            return false;
        }
    }

    const quint64 nDecodedUpperBound = ((quint64)baEncoded.size() / 4ULL) * 3ULL;
    if ((nDecodedUpperBound < (quint64)nPadding) || ((nDecodedUpperBound - (quint64)nPadding) > DMG_MAX_MISH_SIZE)) {
        return false;
    }

    QByteArray baDecoded = QByteArray::fromBase64(baEncoded);
    if (baDecoded.isEmpty() || ((quint64)baDecoded.size() != (nDecodedUpperBound - (quint64)nPadding))) {
        return false;
    }
    // QByteArray::fromBase64() deliberately accepts non-zero unused bits in
    // the final quantum.  Require the unique canonical spelling so malformed
    // plist data cannot alias a different, valid MISH byte sequence.
    if (baDecoded.toBase64() != baEncoded) return false;

    *pResult = baDecoded;
    return true;
}

bool dmgSkipCurrentElementBounded(QXmlStreamReader *pReader, qint32 nCurrentDepth, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pReader || (pReader->tokenType() != QXmlStreamReader::StartElement) || (nCurrentDepth <= 0) || (nCurrentDepth > DMG_MAX_XML_DEPTH)) {
        return false;
    }

    qint32 nDepth = nCurrentDepth;
    while (!pReader->atEnd() && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const QXmlStreamReader::TokenType token = pReader->readNext();
        if (token == QXmlStreamReader::StartElement) {
            nDepth++;
            if (nDepth > DMG_MAX_XML_DEPTH) return false;
        } else if (token == QXmlStreamReader::EndElement) {
            if (nDepth == nCurrentDepth) return true;
            nDepth--;
            if (nDepth < nCurrentDepth) return false;
        } else if ((token == QXmlStreamReader::DTD) || (token == QXmlStreamReader::EntityReference) || (token == QXmlStreamReader::Invalid)) {
            return false;
        }
    }

    return false;
}

bool dmgReadExactSnapshot(QIODevice *pDevice, qint64 nOffset, qint32 nSize, QByteArray *pResult)
{
    if (pResult) pResult->clear();
    if (!pResult || !pDevice || (nOffset < 0) || (nSize < 0)) return false;

    QPointer<QIODevice> guardedDevice(pDevice);
    QByteArray baData(nSize, 0);
    const qint64 nRead = XBinary::read_array_process(guardedDevice.data(), nOffset, baData.data(), nSize, nullptr);
    if (!guardedDevice || (nRead != nSize)) return false;

    *pResult = baData;
    return true;
}

quint32 dmgReadBE32(const QByteArray &baData, qint32 nOffset)
{
    const uchar *pData = reinterpret_cast<const uchar *>(baData.constData() + nOffset);
    return ((quint32)pData[0] << 24) | ((quint32)pData[1] << 16) | ((quint32)pData[2] << 8) | (quint32)pData[3];
}

quint64 dmgReadBE64(const QByteArray &baData, qint32 nOffset)
{
    return ((quint64)dmgReadBE32(baData, nOffset) << 32) | dmgReadBE32(baData, nOffset + 4);
}
}  // namespace

XBinary::XCONVERT _TABLE_XDMG_STRUCTID[] = {
    {XDMG::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XDMG::STRUCTID_KOLY_BLOCK, "KOLY_BLOCK", QString("Koly block")},
    {XDMG::STRUCTID_MISH_BLOCK, "MISH_BLOCK", QString("Mish block")},
    {XDMG::STRUCTID_STRIPE, "STRIPE", QString("Stripe")},
};

XDMG::XDMG(QIODevice *pDevice) : XArchive(pDevice)
{
}

XDMG::~XDMG()
{
}

bool XDMG::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    KOLY_BLOCK kolyBlock = {};
    QList<DMG_PARTITION_INFO> listPartitions;
    QList<MISH_BLOCK> listMishBlocks;
    const bool bMetadata = guardedThis->_loadPartitionMetadata(&kolyBlock, &listPartitions, pPdStruct);
    if (!guardedThis || !bMetadata) return false;
    const bool bPartitions = guardedThis->_parseAllPartitions(listPartitions, kolyBlock, &listMishBlocks, nullptr, pPdStruct);
    return guardedThis && bPartitions && !listMishBlocks.isEmpty();
}

bool XDMG::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XDMG xdmg(pDevice);
    return xdmg.isValid(pPdStruct);
}

QString XDMG::getArch()
{
    return QString("");
}

XBinary::FT XDMG::getFileType()
{
    return FT_DMG;
}

QString XDMG::getFileFormatExt()
{
    return QString("dmg");
}

QString XDMG::getFileFormatExtsString()
{
    return QString("*.dmg");
}

qint64 XDMG::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    const bool bValid = guardedThis->isValid(pPdStruct);
    if (!guardedThis || !bValid) return 0;
    const qint64 nSize = guardedThis->getSize();
    return guardedThis ? nSize : 0;
}

QString XDMG::getVersion()
{
    QPointer<XDMG> guardedThis(this);
    QString sResult;

    KOLY_BLOCK kolyBlock = {};
    const bool bLoaded = guardedThis->_loadKolyAndXml(&kolyBlock, nullptr, false, nullptr);
    if (!guardedThis) return QString();
    if (bLoaded) {
        sResult = QString::number(kolyBlock.nVersion);
    }

    return sResult;
}

bool XDMG::_loadKolyAndXml(KOLY_BLOCK *pKolyBlock, QByteArray *pXmlData, bool bRequireXml, PDSTRUCT *pPdStruct, qint64 *pKolyOffset, qint64 *pArchiveBase,
                           bool bAllowEmbeddedBase, bool bValidateDataForkCRC)
{
    QPointer<XDMG> guardedThis(this);
    if (pXmlData) pXmlData->clear();
    if (pKolyOffset) *pKolyOffset = -1;
    if (pArchiveBase) *pArchiveBase = -1;
    if (!pKolyBlock || !guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedSource) return false;
    const bool bOpen = guardedSource->isOpen();
    if (!guardedThis || !guardedSource || !bOpen) return false;
    const bool bReadable = guardedSource->isReadable();
    if (!guardedThis || !guardedSource || !bReadable) return false;
    const qint64 nSize = guardedSource->size();
    if (!guardedThis || !guardedSource) return false;
    if (nSize < 512) return false;

    const auto sourceIsCurrent = [&guardedThis, &guardedSource]() -> bool { return guardedThis && guardedSource && (guardedThis->getDevice() == guardedSource.data()); };

    qint64 nSelectedArchiveBase = -1;
    const auto tryCandidate = [&](qint64 nKolyOffset, bool bFrontKoly, KOLY_BLOCK *pCandidate, QByteArray *pCandidateXml) -> bool {
        if (!sourceIsCurrent()) return false;
        KOLY_BLOCK kolyBlock = readKolyBlock(guardedSource.data(), nKolyOffset);
        if (!sourceIsCurrent()) return false;
        if (bFrontKoly && ((nSize <= 512) || (kolyBlock.nDataForkOffset != 512))) return false;

        const quint64 nPayloadLimit = bFrontKoly ? (quint64)nSize : (quint64)nKolyOffset;
        if ((kolyBlock.nMagic != 0x6b6f6c79) || (kolyBlock.nVersion != 4) || (kolyBlock.nHeaderLength != 512) || (kolyBlock.nSegment > 1) ||
            (kolyBlock.nSegmentCount > 1) || !dmgRangeWithin(kolyBlock.nDataForkOffset, kolyBlock.nDataForkLength, nPayloadLimit) ||
            !dmgRangeWithin(kolyBlock.nResourceForkOffset, kolyBlock.nResourceForkLength, nPayloadLimit) ||
            !dmgRangeWithin(kolyBlock.nXmlOffset, kolyBlock.nXmlLength, nPayloadLimit) || (kolyBlock.nXmlLength > DMG_MAX_XML_SIZE) ||
            (kolyBlock.nSectorCount > ((quint64)(std::numeric_limits<qint64>::max)() / DMG_SECTOR_SIZE)) || (bRequireXml && (kolyBlock.nXmlLength == 0)) ||
            !dmgChecksumDescriptorValid(kolyBlock.dataChecksum) || !dmgChecksumDescriptorValid(kolyBlock.masterChecksum)) {
            return false;
        }

        // KOLY fork offsets are relative to the start of a carved/embedded
        // image.  Optional code-signature fields are deliberately excluded
        // from base discovery: old writers sometimes left a range-valid but
        // unrelated pair in those slots.
        quint64 nRequiredTop = 0;
        const auto updateTop = [&](quint64 nOffset, quint64 nLength) {
            if (nLength != 0) nRequiredTop = qMax(nRequiredTop, nOffset + nLength);
        };
        updateTop(kolyBlock.nDataForkOffset, kolyBlock.nDataForkLength);
        updateTop(kolyBlock.nResourceForkOffset, kolyBlock.nResourceForkLength);
        updateTop(kolyBlock.nXmlOffset, kolyBlock.nXmlLength);
        if (nRequiredTop > nPayloadLimit) return false;

        QList<quint64> listArchiveBases;
        QSet<quint64> setArchiveBases;
        const auto addArchiveBase = [&](quint64 nBase) {
            if ((nBase <= nPayloadLimit) && !setArchiveBases.contains(nBase)) {
                setArchiveBases.insert(nBase);
                listArchiveBases.append(nBase);
            }
        };

        if (bFrontKoly || !bAllowEmbeddedBase) {
            addArchiveBase(0);
        } else {
            // Index raw XML declarations once rather than probing an
            // overlapping fixed-size window after every occurrence.  The
            // candidate's exact declared XML range and authoritative parser
            // below decide whether it is the real document; this preserves
            // long prologs, comments and trailing whitespace.  Overflowing
            // the explicit storage cap fails closed.
            static const QByteArray baXmlPrefix("<?xml version");
            if ((kolyBlock.nXmlLength >= (quint64)baXmlPrefix.size()) && (kolyBlock.nXmlOffset <= nPayloadLimit - kolyBlock.nXmlLength)) {
                const qint64 nFirstXmlStart = (qint64)kolyBlock.nXmlOffset;
                const qint64 nLastXmlStart = (qint64)(nPayloadLimit - kolyBlock.nXmlLength);
                const qint64 nSearchEnd = nLastXmlStart + baXmlPrefix.size();
                qint64 nCursor = nFirstXmlStart;
                QList<quint64> listXmlBases;
                const qint32 nOverlap = baXmlPrefix.size() - 1;
                QByteArray baCarry;
                QByteArray baChunk(0x10000, 0);
                qint64 nLastDeclaration = -1;
                while ((nCursor < nSearchEnd) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                    const qint32 nReadSize = (qint32)qMin<qint64>(baChunk.size(), nSearchEnd - nCursor);
                    const qint64 nRead = XBinary::read_array_process(guardedSource.data(), nCursor, baChunk.data(), nReadSize, pPdStruct);
                    if (!sourceIsCurrent() || (nRead != nReadSize)) {
                        return false;
                    }

                    QByteArray baTokens = baCarry;
                    baTokens.append(baChunk.constData(), nReadSize);
                    const qint64 nTokensOffset = nCursor - baCarry.size();
                    for (qint32 nFound = baTokens.indexOf(baXmlPrefix); nFound >= 0;) {
                        const qint64 nDeclaration = nTokensOffset + nFound;
                        if ((nDeclaration > nLastDeclaration) && (nDeclaration <= nLastXmlStart)) {
                            if (listXmlBases.size() >= DMG_MAX_XML_BASE_CANDIDATES) {
                                return false;
                            }
                            listXmlBases.append((quint64)nDeclaration - kolyBlock.nXmlOffset);
                            nLastDeclaration = nDeclaration;
                        }
                        if (nFound == (std::numeric_limits<qint32>::max)()) {
                            return false;
                        }
                        nFound = baTokens.indexOf(baXmlPrefix, nFound + 1);
                    }

                    baCarry = (baTokens.size() > nOverlap) ? baTokens.right(nOverlap) : baTokens;
                    nCursor += nReadSize;
                }
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

                // A terminal image normally has only a small alignment gap,
                // so try the nearest declaration first without discarding an
                // otherwise valid middle candidate.
                for (auto it = listXmlBases.crbegin(); it != listXmlBases.crend(); ++it) {
                    addArchiveBase(*it);
                }
            }

            // Retain both conventional layouts as fallbacks for resource-fork
            // metadata and XML documents without a declaration.
            if ((kolyBlock.nCodeSignatureLength != 0) && dmgRangeWithin(kolyBlock.nCodeSignatureOffset, kolyBlock.nCodeSignatureLength, nPayloadLimit)) {
                const quint64 nOptionalTop = qMax(nRequiredTop, kolyBlock.nCodeSignatureOffset + kolyBlock.nCodeSignatureLength);
                // This candidate is still accepted only after the required
                // forks pass their integrity and metadata checks below.
                addArchiveBase(nPayloadLimit - nOptionalTop);
            }
            addArchiveBase(nPayloadLimit - nRequiredTop);
            addArchiveBase(0);
        }

        QMap<quint64, quint32> mapDataForkPrefixes;
        bool bDataHashIndexBuilt = false;
        bool bDataHashIndexValid = false;
        const auto ensureDataHashIndex = [&]() -> bool {
            if (bDataHashIndexBuilt) return bDataHashIndexValid;
            bDataHashIndexBuilt = true;

            // Metadata is checked before this lazy pass.  Once one candidate
            // is coherent, index every range-valid base in a single ascending
            // scan.  This preserves the old linear I/O bound without a shared
            // byte budget that can starve the genuine base after several
            // checksum-mismatching decoys.
            for (quint64 nBase : listArchiveBases) {
                if ((kolyBlock.nDataForkOffset > (std::numeric_limits<quint64>::max)() - nBase)) {
                    continue;
                }
                const quint64 nStart = nBase + kolyBlock.nDataForkOffset;
                if (!dmgRangeWithin(nStart, kolyBlock.nDataForkLength, nPayloadLimit)) {
                    continue;
                }
                mapDataForkPrefixes.insert(nStart, 0);
                mapDataForkPrefixes.insert(nStart + kolyBlock.nDataForkLength, 0);
            }
            if (mapDataForkPrefixes.isEmpty()) return false;

            QByteArray baHashBuffer(0x10000, 0);
            quint64 nCursor = mapDataForkPrefixes.constBegin().key();
            quint32 nPrefixCRC = 0;
            for (auto it = mapDataForkPrefixes.begin(); it != mapDataForkPrefixes.end(); ++it) {
                const quint64 nEnd = it.key();
                while ((nCursor < nEnd) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                    const qint32 nChunk = (qint32)qMin<quint64>((quint64)baHashBuffer.size(), nEnd - nCursor);
                    const qint64 nRead = XBinary::read_array_process(guardedSource.data(), (qint64)nCursor, baHashBuffer.data(), nChunk, pPdStruct);
                    if (!sourceIsCurrent() || (nRead != nChunk)) {
                        return false;
                    }
                    nPrefixCRC = dmgUpdateCRC32(nPrefixCRC, baHashBuffer.constData(), nChunk);
                    nCursor += (quint64)nChunk;
                }
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
                it.value() = nPrefixCRC;
            }

            bDataHashIndexValid = true;
            return true;
        };
        for (quint64 nArchiveBase : listArchiveBases) {
            KOLY_BLOCK candidate = kolyBlock;
            const auto normalizePair = [&](quint64 *pOffset, quint64 nLength) -> bool {
                if (!pOffset || (nLength == 0)) return true;
                if (*pOffset > ((std::numeric_limits<quint64>::max)() - nArchiveBase)) return false;
                *pOffset += nArchiveBase;
                return dmgRangeWithin(*pOffset, nLength, nPayloadLimit);
            };
            if (!normalizePair(&candidate.nDataForkOffset, candidate.nDataForkLength) || !normalizePair(&candidate.nResourceForkOffset, candidate.nResourceForkLength) ||
                !normalizePair(&candidate.nXmlOffset, candidate.nXmlLength)) {
                continue;
            }

            // Every required UDIF fork owns a distinct physical extent.  A
            // range can be individually in-bounds yet overlap another fork,
            // yielding contradictory metadata/file-part ownership.  Front
            // KOLY images also reserve their leading 512-byte header.
            QList<QPair<quint64, quint64>> listRequiredRanges;
            const auto addDisjointRange = [&listRequiredRanges](quint64 nStart, quint64 nLength) -> bool {
                if (nLength == 0) return true;
                if (nStart > (std::numeric_limits<quint64>::max)() - nLength) {
                    return false;
                }
                const quint64 nEnd = nStart + nLength;
                for (const QPair<quint64, quint64> &range : listRequiredRanges) {
                    if ((nStart < range.second) && (range.first < nEnd)) {
                        return false;
                    }
                }
                listRequiredRanges.append(qMakePair(nStart, nEnd));
                return true;
            };
            if ((bFrontKoly && !addDisjointRange(0, 512)) || !addDisjointRange(candidate.nDataForkOffset, candidate.nDataForkLength) ||
                !addDisjointRange(candidate.nResourceForkOffset, candidate.nResourceForkLength) || !addDisjointRange(candidate.nXmlOffset, candidate.nXmlLength)) {
                continue;
            }

            // A candidate base is accepted only when it leads to coherent
            // partition metadata.  Range checks alone are ambiguous in a
            // carrier that happens to contain a false XML marker.
            QByteArray baXml;
            QList<DMG_PARTITION_INFO> listCandidatePartitions;
            if (candidate.nXmlLength != 0) {
                if (candidate.nXmlLength > (quint64)(std::numeric_limits<qint32>::max)()) continue;
                // A raw declaration can occur inside a wrapper or inside the
                // genuine document.  Reject those shifted bases by streaming
                // only as far as the exact range's first element before
                // allocating and parsing the complete metadata.  The explicit
                // XML-size and candidate-count caps remain the hard bounds;
                // one false candidate can no longer consume a shared budget
                // that makes a later coherent base unreachable.
                const bool bHasPlistRoot = dmgHasUnnamespacedPlistRoot(guardedSource.data(), (qint64)candidate.nXmlOffset, (qint64)candidate.nXmlLength, pPdStruct);
                if (!sourceIsCurrent()) return false;
                if (!bHasPlistRoot) {
                    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
                    continue;
                }
                baXml.resize((qint32)candidate.nXmlLength);
                const qint64 nXmlRead = XBinary::read_array_process(guardedSource.data(), (qint64)candidate.nXmlOffset, baXml.data(), baXml.size(), pPdStruct);
                if (!sourceIsCurrent() || (nXmlRead != baXml.size()) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
                    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
                    continue;
                }
                listCandidatePartitions = guardedThis->_parseBlkxPartitions(baXml, pPdStruct);
                if (!sourceIsCurrent()) return false;
            } else if ((candidate.nResourceForkLength >= 0x100) && (candidate.nResourceForkLength <= DMG_MAX_RESOURCE_SIZE) &&
                       (candidate.nResourceForkLength <= (quint64)(std::numeric_limits<qint32>::max)())) {
                QByteArray baResource((qint32)candidate.nResourceForkLength, 0);
                const qint64 nResourceRead =
                    XBinary::read_array_process(guardedSource.data(), (qint64)candidate.nResourceForkOffset, baResource.data(), baResource.size(), pPdStruct);
                if (!sourceIsCurrent()) return false;
                if ((nResourceRead == baResource.size()) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                    listCandidatePartitions = guardedThis->_parseResourceForkPartitions(baResource, pPdStruct);
                    if (!sourceIsCurrent()) return false;
                }
            }
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            QList<MISH_BLOCK> listCandidateMishBlocks;
            if (listCandidatePartitions.isEmpty() ||
                !guardedThis->_parseAllPartitions(listCandidatePartitions, candidate, &listCandidateMishBlocks, nullptr, pPdStruct) ||
                listCandidateMishBlocks.isEmpty()) {
                if (!sourceIsCurrent()) return false;
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
                continue;
            }
            if (!sourceIsCurrent()) return false;

            // Parse bounded metadata before hashing a potentially enormous
            // fork.  Search/carving supplies an indexed exact CRC check and
            // disables this duplicate linear pass.
            if (bValidateDataForkCRC && dmgChecksumIsCRC32(candidate.dataChecksum)) {
                if (!ensureDataHashIndex()) {
                    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
                    return false;
                }
                const quint64 nDataEnd = candidate.nDataForkOffset + candidate.nDataForkLength;
                const auto itStart = mapDataForkPrefixes.constFind(candidate.nDataForkOffset);
                const auto itEnd = mapDataForkPrefixes.constFind(nDataEnd);
                if ((itStart == mapDataForkPrefixes.constEnd()) || (itEnd == mapDataForkPrefixes.constEnd())) continue;
                const quint32 nDataForkCRC = itEnd.value() ^ dmgCombineCRC32(itStart.value(), 0, candidate.nDataForkLength);
                if (nDataForkCRC != candidate.dataChecksum[2]) continue;
            }

            const bool bUseCodeSignature = (candidate.nCodeSignatureLength != 0) && normalizePair(&candidate.nCodeSignatureOffset, candidate.nCodeSignatureLength) &&
                                           addDisjointRange(candidate.nCodeSignatureOffset, candidate.nCodeSignatureLength);
            if (!bUseCodeSignature) {
                // Several old writers left garbage in this optional pair.  It
                // is non-authoritative and cannot weaken required-fork checks.
                candidate.nCodeSignatureOffset = 0;
                candidate.nCodeSignatureLength = 0;
            }

            *pCandidate = candidate;
            if (pCandidateXml) *pCandidateXml = baXml;
            nSelectedArchiveBase = (qint64)nArchiveBase;
            return true;
        }

        return false;
    };

    const qint64 nTrailerOffset = nSize - 512;
    KOLY_BLOCK kolyBlock = {};
    QByteArray baXml;
    qint64 nSelectedOffset = nTrailerOffset;

    // Once the terminal bytes identify a structural KOLY trailer, it is
    // authoritative.  In particular, an integrity/range failure must not be
    // downgraded to a separately crafted front header.
    const KOLY_BLOCK terminalKolyBlock = readKolyBlock(guardedSource.data(), nTrailerOffset);
    if (!sourceIsCurrent()) return false;
    const bool bTerminalKolyPresent = (terminalKolyBlock.nMagic == 0x6b6f6c79) && (terminalKolyBlock.nVersion == 4) && (terminalKolyBlock.nHeaderLength == 512);
    if (bTerminalKolyPresent) {
        if (!tryCandidate(nTrailerOffset, false, &kolyBlock, &baXml)) return false;
    } else {
        nSelectedOffset = 0;
        if ((nTrailerOffset == 0) || !tryCandidate(0, true, &kolyBlock, &baXml)) return false;
    }

    if (!sourceIsCurrent()) return false;
    *pKolyBlock = kolyBlock;
    if (pXmlData) *pXmlData = baXml;
    if (pKolyOffset) *pKolyOffset = nSelectedOffset;
    if (pArchiveBase) *pArchiveBase = nSelectedArchiveBase;
    return true;
}

bool XDMG::_loadPartitionMetadata(KOLY_BLOCK *pKolyBlock, QList<DMG_PARTITION_INFO> *pPartitions, PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    if (pPartitions) pPartitions->clear();
    if (!guardedThis || !pKolyBlock || !pPartitions || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedSource) return false;
    const auto sourceIsCurrent = [&guardedThis, &guardedSource]() -> bool { return guardedThis && guardedSource && (guardedThis->getDevice() == guardedSource.data()); };

    QByteArray baXml;
    const bool bLoaded = guardedThis->_loadKolyAndXml(pKolyBlock, &baXml, false, pPdStruct);
    if (!sourceIsCurrent() || !bLoaded) return false;

    if (pKolyBlock->nXmlLength != 0) {
        *pPartitions = guardedThis->_parseBlkxPartitions(baXml, pPdStruct);
        if (!sourceIsCurrent()) return false;
    } else {
        if ((pKolyBlock->nResourceForkLength < 0x100) || (pKolyBlock->nResourceForkLength > DMG_MAX_RESOURCE_SIZE) ||
            (pKolyBlock->nResourceForkLength > (quint64)(std::numeric_limits<qint32>::max)())) {
            return false;
        }
        QByteArray baResource((qint32)pKolyBlock->nResourceForkLength, 0);
        const qint64 nRead = XBinary::read_array_process(guardedSource.data(), (qint64)pKolyBlock->nResourceForkOffset, baResource.data(), baResource.size(), pPdStruct);
        if (!sourceIsCurrent() || (nRead != baResource.size()) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        *pPartitions = guardedThis->_parseResourceForkPartitions(baResource, pPdStruct);
        if (!sourceIsCurrent()) return false;
    }

    if (pPartitions->isEmpty() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        pPartitions->clear();
        return false;
    }

    DMG_NAME_STATE nameState;
    for (qint32 i = 0; (i < pPartitions->size()) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        QString sSanitizedName;
        if (!dmgSanitizePartitionName((*pPartitions)[i].sName, i, &nameState, pPdStruct, &sSanitizedName)) {
            pPartitions->clear();
            return false;
        }
        (*pPartitions)[i].sName = sSanitizedName;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        pPartitions->clear();
        return false;
    }
    return sourceIsCurrent();
}

XBinary::MODE XDMG::getMode()
{
    return MODE_UNKNOWN;
}

XBinary::ENDIAN XDMG::getEndian()
{
    return ENDIAN_BIG;
}

QString XDMG::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XDMG_STRUCTID, sizeof(_TABLE_XDMG_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XDMG::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XDMG_STRUCTID, sizeof(_TABLE_XDMG_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XDMG::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XDMG_STRUCTID, sizeof(_TABLE_XDMG_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XDMG::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    QList<XBinary::XFHEADER> listResult;
    const auto getKolyOffset = [&guardedThis, pPdStruct]() -> qint64 {
        if (!guardedThis) return -1;
        KOLY_BLOCK kolyBlock = {};
        qint64 nKolyOffset = -1;
        const bool bLoaded = guardedThis->_loadKolyAndXml(&kolyBlock, nullptr, false, pPdStruct, &nKolyOffset);
        return (guardedThis && bLoaded) ? nKolyOffset : -1;
    };

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        qint64 nKolyOffset = getKolyOffset();

        if (nKolyOffset >= 0) {
            XFSTRUCT _xfStruct = xfStruct;
            _xfStruct.nStructID = STRUCTID_KOLY_BLOCK;
            _xfStruct.xLoc = guardedThis->offsetToLoc(nKolyOffset);
            const QList<XFHEADER> listHeaders = guardedThis->getXFHeaders(_xfStruct, pPdStruct);
            if (!guardedThis) return QList<XFHEADER>();
            listResult.append(listHeaders);
        }
    } else if (nStructID == STRUCTID_KOLY_BLOCK) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            qint64 nKolyOffset = getKolyOffset();

            if (nKolyOffset < 0) {
                return listResult;
            }

            if (!guardedThis) return listResult;
            headerLoc = guardedThis->offsetToLoc(nKolyOffset);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_KOLY_BLOCK);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = 512;
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_KOLY_BLOCK, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_KOLY_BLOCK), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XDMG::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    // DMG structures are big-endian; offsets match readKolyBlock/readMishBlock
    if (nStructID == STRUCTID_KOLY_BLOCK) {
        listResult.append({"nMagic", 0, 4, XFRECORD_FLAG_BE, VT_UINT32});
        listResult.append({"nVersion", 4, 4, XFRECORD_FLAG_BE | XFRECORD_FLAG_VERSION, VT_UINT32});
        listResult.append({"nHeaderLength", 8, 4, XFRECORD_FLAG_BE | XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"nFlags", 12, 4, XFRECORD_FLAG_BE, VT_UINT32});
        listResult.append({"nRunningOffset", 16, 8, XFRECORD_FLAG_BE | XFRECORD_FLAG_OFFSET, VT_UINT64});
        listResult.append({"nDataForkOffset", 24, 8, XFRECORD_FLAG_BE | XFRECORD_FLAG_OFFSET, VT_UINT64});
        listResult.append({"nDataForkLength", 32, 8, XFRECORD_FLAG_BE | XFRECORD_FLAG_SIZE, VT_UINT64});
        listResult.append({"nResourceForkOffset", 40, 8, XFRECORD_FLAG_BE | XFRECORD_FLAG_OFFSET, VT_UINT64});
        listResult.append({"nResourceForkLength", 48, 8, XFRECORD_FLAG_BE | XFRECORD_FLAG_SIZE, VT_UINT64});
        listResult.append({"nSegment", 56, 4, XFRECORD_FLAG_BE, VT_UINT32});
        listResult.append({"nSegmentCount", 60, 4, XFRECORD_FLAG_BE | XFRECORD_FLAG_COUNT, VT_UINT32});
        listResult.append({"segmentID", 64, 16, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"dataChecksum", 80, 136, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"nXmlOffset", 216, 8, XFRECORD_FLAG_BE | XFRECORD_FLAG_OFFSET, VT_UINT64});
        listResult.append({"nXmlLength", 224, 8, XFRECORD_FLAG_BE | XFRECORD_FLAG_SIZE, VT_UINT64});
        listResult.append({"nCodeSignatureOffset", 296, 8, XFRECORD_FLAG_BE | XFRECORD_FLAG_OFFSET, VT_UINT64});
        listResult.append({"nCodeSignatureLength", 304, 8, XFRECORD_FLAG_BE | XFRECORD_FLAG_SIZE, VT_UINT64});
        listResult.append({"masterChecksum", 352, 136, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"nImageVariant", 488, 4, XFRECORD_FLAG_BE, VT_UINT32});
        listResult.append({"nSectorCount", 492, 8, XFRECORD_FLAG_BE | XFRECORD_FLAG_COUNT, VT_UINT64});
    } else if (nStructID == STRUCTID_MISH_BLOCK) {
        listResult.append({"nMagic", 0, 4, XFRECORD_FLAG_BE, VT_UINT32});
        listResult.append({"nVersion", 4, 4, XFRECORD_FLAG_BE | XFRECORD_FLAG_VERSION, VT_UINT32});
        listResult.append({"nStartSector", 8, 8, XFRECORD_FLAG_BE, VT_UINT64});
        listResult.append({"nSectorCount", 16, 8, XFRECORD_FLAG_BE | XFRECORD_FLAG_COUNT, VT_UINT64});
        listResult.append({"nDataOffset", 24, 8, XFRECORD_FLAG_BE | XFRECORD_FLAG_OFFSET, VT_UINT64});
        listResult.append({"nBufferCount", 32, 4, XFRECORD_FLAG_BE | XFRECORD_FLAG_COUNT, VT_UINT32});
        listResult.append({"nDescriptorBlocks", 36, 4, XFRECORD_FLAG_BE | XFRECORD_FLAG_COUNT, VT_UINT32});
        listResult.append({"nBlockDataCount", 200, 4, XFRECORD_FLAG_BE | XFRECORD_FLAG_COUNT, VT_UINT32});
    } else if (nStructID == STRUCTID_STRIPE) {
        listResult.append({"nType", 0, 4, XFRECORD_FLAG_BE, VT_UINT32});
        listResult.append({"nReserved", 4, 4, XFRECORD_FLAG_BE, VT_UINT32});
        listResult.append({"nStartSector", 8, 8, XFRECORD_FLAG_BE, VT_UINT64});
        listResult.append({"nSectorCount", 16, 8, XFRECORD_FLAG_BE | XFRECORD_FLAG_COUNT, VT_UINT64});
        listResult.append({"nDataOffset", 24, 8, XFRECORD_FLAG_BE | XFRECORD_FLAG_OFFSET, VT_UINT64});
        listResult.append({"nDataLength", 32, 8, XFRECORD_FLAG_BE | XFRECORD_FLAG_SIZE, VT_UINT64});
    }

    return listResult;
}

QList<XBinary::MAPMODE> XDMG::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);

    return listResult;
}

XBinary::_MEMORY_MAP XDMG::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_REGIONS;
    }

    if (mapMode == MAPMODE_REGIONS) {
        result = _getMemoryMap(FILEPART_HEADER | FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
    } else if (mapMode == MAPMODE_STREAMS) {
        result = _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }

    return result;
}

QList<XBinary::FPART> XDMG::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    QList<FPART> listResult;
    if (!guardedThis || (nLimit < -1) || (nLimit == 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedSource) return listResult;
    const qint64 nDeviceSize = guardedSource->size();
    if (!guardedThis || !guardedSource || (nDeviceSize < 0)) return listResult;
    const auto sourceIsCurrent = [&guardedThis, &guardedSource]() -> bool { return guardedThis && guardedSource && (guardedThis->getDevice() == guardedSource.data()); };

    const auto canAppend = [&]() -> bool { return sourceIsCurrent() && XBinary::isPdStructNotCanceled(pPdStruct) && ((nLimit == -1) || (listResult.size() < nLimit)); };
    const auto appendPart = [&](FILEPART filePart, qint64 nOffset, qint64 nSize, const QString &sName, qint64 nUncompressedSize = -1,
                                HANDLE_METHOD handleMethod = HANDLE_METHOD_UNKNOWN) {
        if (!canAppend() || (nOffset < 0) || (nSize <= 0) || (nSize > nDeviceSize) || (nOffset > nDeviceSize - nSize)) return;
        FPART part = guardedThis->getFPART(filePart, sName, nOffset, nSize, XADDR_MAX, 0);
        if (nUncompressedSize >= 0) {
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, handleMethod);
        }
        listResult.append(part);
    };

    KOLY_BLOCK kolyBlock = {};
    qint64 nKolyOffset = -1;
    if (!guardedThis->_loadKolyAndXml(&kolyBlock, nullptr, false, pPdStruct, &nKolyOffset) || !sourceIsCurrent()) {
        return listResult;
    }

    qint64 nKnownEnd = 0;
    const auto accountRange = [&](quint64 nOffset, quint64 nSize) {
        const quint64 nMaximum = (quint64)(std::numeric_limits<qint64>::max)();
        if ((nSize != 0) && (nOffset <= nMaximum) && (nSize <= (nMaximum - nOffset))) {
            nKnownEnd = qMax(nKnownEnd, (qint64)(nOffset + nSize));
        }
    };
    accountRange((quint64)nKolyOffset, 512);
    accountRange(kolyBlock.nDataForkOffset, kolyBlock.nDataForkLength);
    accountRange(kolyBlock.nResourceForkOffset, kolyBlock.nResourceForkLength);
    accountRange(kolyBlock.nXmlOffset, kolyBlock.nXmlLength);
    accountRange(kolyBlock.nCodeSignatureOffset, kolyBlock.nCodeSignatureLength);

    if ((nFileParts & FILEPART_HEADER) && canAppend()) {
        appendPart(FILEPART_HEADER, nKolyOffset, 512, tr("KOLY header"));
    }
    if ((nFileParts & FILEPART_DATA) && canAppend()) {
        if (kolyBlock.nDataForkLength != 0) {
            appendPart(FILEPART_DATA, (qint64)kolyBlock.nDataForkOffset, (qint64)kolyBlock.nDataForkLength, tr("Data fork"));
        }
        if ((kolyBlock.nResourceForkLength != 0) && canAppend()) {
            appendPart(FILEPART_DATA, (qint64)kolyBlock.nResourceForkOffset, (qint64)kolyBlock.nResourceForkLength, tr("Resource fork"));
        }
        if ((kolyBlock.nXmlLength != 0) && canAppend()) {
            appendPart(FILEPART_DATA, (qint64)kolyBlock.nXmlOffset, (qint64)kolyBlock.nXmlLength, tr("XML metadata"));
        }
        if ((kolyBlock.nCodeSignatureLength != 0) && canAppend()) {
            appendPart(FILEPART_DATA, (qint64)kolyBlock.nCodeSignatureOffset, (qint64)kolyBlock.nCodeSignatureLength, tr("Code signature"));
        }
    }

    if ((nFileParts & FILEPART_STREAM) && canAppend()) {
        QList<DMG_PARTITION_INFO> listPartitions;
        QList<MISH_BLOCK> listMishBlocks;
        QList<QList<BLOCK_DATA>> listStripes;
        if (!guardedThis->_loadPartitionMetadata(&kolyBlock, &listPartitions, pPdStruct) || !sourceIsCurrent() ||
            !guardedThis->_parseAllPartitions(listPartitions, kolyBlock, &listMishBlocks, &listStripes, pPdStruct) || !sourceIsCurrent()) {
            listResult.clear();
            return listResult;
        }

        for (qint32 i = 0; (i < listStripes.size()) && canAppend(); i++) {
            const MISH_BLOCK &mishBlock = listMishBlocks.at(i);
            const QList<BLOCK_DATA> &stripes = listStripes.at(i);
            for (qint32 k = 0; (k < stripes.size()) && canAppend(); k++) {
                const BLOCK_DATA &stripe = stripes.at(k);
                HANDLE_METHOD handleMethod = HANDLE_METHOD_UNKNOWN;
                bool bPhysicalStream = true;
                switch (stripe.nType) {
                    case DMG_STRIPE_STORED: handleMethod = HANDLE_METHOD_STORE; break;
                    case DMG_STRIPE_DEFLATE: handleMethod = HANDLE_METHOD_ZLIB; break;
                    case DMG_STRIPE_BZ: handleMethod = HANDLE_METHOD_BZIP2; break;

                    case DMG_STRIPE_ADC:
                    case DMG_STRIPE_LZFSE:
                    case DMG_STRIPE_XZ: break;

                    case DMG_STRIPE_EMPTY:
                    case DMG_STRIPE_ZEROES:
                    case DMG_STRIPE_SKIP:
                    case DMG_STRIPE_END: bPhysicalStream = false; break;

                    default: listResult.clear(); return listResult;
                }
                if (!bPhysicalStream) continue;

                const quint64 nMaximum = (quint64)(std::numeric_limits<qint64>::max)();
                if ((kolyBlock.nDataForkOffset > nMaximum) || (mishBlock.nDataOffset > (nMaximum - kolyBlock.nDataForkOffset))) {
                    listResult.clear();
                    return listResult;
                }
                const quint64 nPartitionDataOffset = kolyBlock.nDataForkOffset + mishBlock.nDataOffset;
                if ((stripe.nDataOffset > (nMaximum - nPartitionDataOffset)) || (stripe.nDataLength > nMaximum) || (stripe.nSectorCount > (nMaximum / DMG_SECTOR_SIZE))) {
                    listResult.clear();
                    return listResult;
                }
                const quint64 nAbsoluteOffset = nPartitionDataOffset + stripe.nDataOffset;
                const quint64 nUncompressedSize = stripe.nSectorCount * DMG_SECTOR_SIZE;
                appendPart(FILEPART_STREAM, (qint64)nAbsoluteOffset, (qint64)stripe.nDataLength, tr("Partition %1 stream %2").arg(i + 1).arg(k + 1),
                           (qint64)nUncompressedSize, handleMethod);
            }
        }
    }

    if ((nFileParts & FILEPART_OVERLAY) && canAppend() && (nKnownEnd < nDeviceSize)) {
        appendPart(FILEPART_OVERLAY, nKnownEnd, nDeviceSize - nKnownEnd, tr("Overlay"));
    }

    if (!sourceIsCurrent() || !XBinary::isPdStructNotCanceled(pPdStruct)) listResult.clear();
    return listResult;
}

quint64 XDMG::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    KOLY_BLOCK kolyBlock = {};
    QList<DMG_PARTITION_INFO> listPartitions;
    const bool bMetadata = guardedThis->_loadPartitionMetadata(&kolyBlock, &listPartitions, pPdStruct);
    if (!guardedThis || !bMetadata) return 0;

    QList<MISH_BLOCK> listMishBlocks;
    const bool bPartitions = guardedThis->_parseAllPartitions(listPartitions, kolyBlock, &listMishBlocks, nullptr, pPdStruct);
    return (guardedThis && bPartitions) ? (quint64)listMishBlocks.size() : 0;
}

QList<XArchive::RECORD> XDMG::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    QList<RECORD> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    KOLY_BLOCK kolyBlock = {};
    QList<DMG_PARTITION_INFO> listPartitions;
    const bool bMetadata = guardedThis->_loadPartitionMetadata(&kolyBlock, &listPartitions, pPdStruct);
    if (!guardedThis || !bMetadata) return listResult;

    QList<MISH_BLOCK> listMishBlocks;
    QList<QList<BLOCK_DATA>> listStripes;
    const bool bPartitions = guardedThis->_parseAllPartitions(listPartitions, kolyBlock, &listMishBlocks, &listStripes, pPdStruct);
    if (!guardedThis || !bPartitions) return listResult;
    for (qint32 i = 0; i < listMishBlocks.size(); i++) {
        const MISH_BLOCK mishBlock = listMishBlocks.at(i);

        if ((nLimit != -1) && (listResult.size() >= nLimit)) continue;

        RECORD record = {};
        QString sName = listPartitions.at(i).sName;
        if (sName.isEmpty()) sName = QString("partition%1").arg(i);

        qint64 nCompressedSize = 0;
        HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;
        if ((i >= listStripes.size()) || !dmgGetPartitionStorageInfo(listStripes.at(i), &nCompressedSize, &compressMethod)) {
            listResult.clear();
            return listResult;
        }

        record.spInfo.sRecordName = sName + ".img";
        record.spInfo.nUncompressedSize = (qint64)(mishBlock.nSectorCount * DMG_SECTOR_SIZE);
        if (dmgChecksumIsCRC32(mishBlock.checksum) && (i < listStripes.size()) && dmgHasFullPartitionCRC(listStripes.at(i))) {
            record.spInfo.nCRC32 = mishBlock.checksum[2];
        }
        record.spInfo.compressMethod = compressMethod;
        record.nDataOffset = 0;
        record.nDataSize = 0;
        record.mapProperties.insert(FPART_PROP_ORIGINALNAME, record.spInfo.sRecordName);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, record.spInfo.nUncompressedSize);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nCompressedSize);
        record.mapProperties.insert(FPART_PROP_EXT, "img");
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, compressMethod);
        if (dmgChecksumIsCRC32(mishBlock.checksum) && dmgHasFullPartitionCRC(listStripes.at(i))) {
            record.mapProperties.insert(FPART_PROP_RESULTCRC, mishBlock.checksum[2]);
            record.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
        }
        listResult.append(record);
    }

    if (!isPdStructNotCanceled(pPdStruct)) listResult.clear();

    return listResult;
}

QList<XBinary::PM_INFO> XDMG::unpackImplemented()
{
    QList<PM_INFO> listResult;
    for (HANDLE_METHOD method : {HANDLE_METHOD_STORE, HANDLE_METHOD_ZLIB, HANDLE_METHOD_BZIP2}) {
        PM_INFO info = {};
        info.hm[0] = method;
        listResult.append(info);
    }
    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XDMG::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XDMG::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapOptions, PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    if (!pState || m_bUnpackOperationInProgress) return false;
    const bool bFinished = guardedThis->finishUnpack(pState, nullptr);
    if (!guardedThis || !bFinished) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    if (!isPdStructNotCanceled(pPdStruct)) return false;
    const bool bBound = guardedThis->bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;
    const auto failSource = [&guardedThis, pState]() -> bool {
        if (!guardedThis) return false;
        guardedThis->releaseUnpackSource(pState);
        return false;
    };

    SOURCE_DEVICE_SNAPSHOT sourceSnapshot = {};
    if (!guardedThis->getBoundUnpackSourceSnapshot(pState, &sourceSnapshot)) return failSource();

    KOLY_BLOCK kolyBlock = {};
    QList<DMG_PARTITION_INFO> listPartitions;
    const bool bMetadata = guardedThis->_loadPartitionMetadata(&kolyBlock, &listPartitions, pPdStruct);
    if (!guardedThis) return false;
    if (!bMetadata) {
        return failSource();
    }

    QList<MISH_BLOCK> listMishBlocks;
    QList<QList<BLOCK_DATA>> listStripes;
    QList<QString> listPartitionNames;
    const bool bPartitions = guardedThis->_parseAllPartitions(listPartitions, kolyBlock, &listMishBlocks, &listStripes, pPdStruct);
    if (!guardedThis) return false;
    if (!bPartitions) {
        return failSource();
    }
    for (qint32 i = 0; i < listPartitions.size(); i++) {
        listPartitionNames.append(listPartitions.at(i).sName);
    }

    if (!isPdStructNotCanceled(pPdStruct) || listMishBlocks.isEmpty()) {
        return failSource();
    }

    const bool bSnapshotCurrent = guardedThis->isSourceDeviceSnapshotCurrent(sourceSnapshot, guardedThis->getDevice(), pPdStruct);
    if (!guardedThis) return false;
    if (!bSnapshotCurrent) return failSource();
    const qint64 nTotalSize = guardedThis->getSize();
    if (!guardedThis) return false;
    if (nTotalSize < 0) return failSource();

    DMG_UNPACK_CONTEXT *pContext = new (std::nothrow) DMG_UNPACK_CONTEXT;
    if (!pContext) {
        return failSource();
    }

    pContext->sourceSnapshot = sourceSnapshot;
    pContext->baXmlData.clear();
    pContext->nDataForkOffset = (qint64)kolyBlock.nDataForkOffset;
    pContext->nDataForkLength = (qint64)kolyBlock.nDataForkLength;
    pContext->listMishBlocks = listMishBlocks;
    pContext->listStripes = listStripes;
    pContext->listPartitionNames = listPartitionNames;
    pContext->nCurrentMishIndex = 0;
    pContext->nCurrentStripeIndex = 0;
    pContext->nCurrentFileIndex = 0;
    pContext->sCurrentFileName.clear();

    pState->mapUnpackProperties = mapOptions;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = nTotalSize;
    pState->nNumberOfRecords = listMishBlocks.size();
    pState->nCurrentIndex = 0;
    pState->pContext = pContext;
    if (!guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) return false;
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        delete pContext;
        pState->nCurrentOffset = 0;
        pState->nTotalSize = 0;
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 0;
        pState->mapUnpackProperties.clear();
        pState->mapArchiveProperties.clear();
        return false;
    }
    return true;
}

XArchive::ARCHIVERECORD XDMG::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    ARCHIVERECORD result = {};
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return result;

    if (isPdStructNotCanceled(pPdStruct) && pState && pState->pContext) {
        const bool bSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
        if (!guardedThis || !bSourceCurrent) return result;
        DMG_UNPACK_CONTEXT *pContext = (DMG_UNPACK_CONTEXT *)pState->pContext;

        const bool bSnapshotCurrent = guardedThis->isSourceDeviceSnapshotCurrent(pContext->sourceSnapshot, guardedThis->getDevice(), pPdStruct);
        if (!guardedThis || !bSnapshotCurrent) {
            return result;
        }

        if ((pState->nCurrentIndex >= 0) && (pState->nCurrentIndex < pContext->listMishBlocks.size())) {
            QString sPartitionName;
            if (pState->nCurrentIndex < pContext->listPartitionNames.size()) {
                sPartitionName = pContext->listPartitionNames.at(pState->nCurrentIndex);
            }

            if (sPartitionName.isEmpty()) {
                sPartitionName = QString("partition%1").arg(pContext->nCurrentFileIndex);
            }

            qint64 nUncompressedSize = (qint64)(pContext->listMishBlocks.at(pState->nCurrentIndex).nSectorCount * DMG_SECTOR_SIZE);

            qint64 nCompressedSize = 0;
            HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;
            const bool bStorageInfoValid = (pState->nCurrentIndex < pContext->listStripes.size()) &&
                                           dmgGetPartitionStorageInfo(pContext->listStripes.at(pState->nCurrentIndex), &nCompressedSize, &compressMethod);

            result.mapProperties.insert(FPART_PROP_ORIGINALNAME, sPartitionName + ".img");
            result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
            if (bStorageInfoValid) {
                result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nCompressedSize);
                result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, compressMethod);
            }
            result.mapProperties.insert(FPART_PROP_EXT, "img");
            const MISH_BLOCK &mishBlock = pContext->listMishBlocks.at(pState->nCurrentIndex);
            if (dmgChecksumIsCRC32(mishBlock.checksum) && (pState->nCurrentIndex < pContext->listStripes.size()) &&
                dmgHasFullPartitionCRC(pContext->listStripes.at(pState->nCurrentIndex))) {
                result.mapProperties.insert(FPART_PROP_RESULTCRC, mishBlock.checksum[2]);
                result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
            }
        }

        result.nStreamOffset = 0;
        result.nStreamSize = 0;
    }

    if (pState && pState->pContext) {
        const bool bFinalSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
        if (!guardedThis || !bFinalSourceCurrent) return ARCHIVERECORD();
    }
    return result;
}

bool XDMG::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState) return false;
    if (!pState->pContext) return false;

    DMG_UNPACK_CONTEXT *pContext = (DMG_UNPACK_CONTEXT *)pState->pContext;
    // The parsed context is bound to one exact source device.  Reject a public
    // setDevice() replacement before resetting state or touching an output.
    const bool bSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return false;
    const bool bSnapshotCurrent = guardedThis->isSourceDeviceSnapshotCurrent(pContext->sourceSnapshot, guardedThis->getDevice(), pPdStruct);
    if (!guardedThis || !bSnapshotCurrent) return false;

    if (!guardedOutput) return false;
    const bool bOutputSupported = guardedThis->isUnpackOutputSupported(guardedOutput.data());
    if (!guardedThis || !guardedOutput || !bOutputSupported || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pContext->listStripes.size()) || (pState->nCurrentIndex >= pContext->listMishBlocks.size())) {
        return false;
    }
    if (!guardedOutput) return false;
    const bool bAliases = dmgDevicesAlias(pContext->sourceSnapshot.pSourceDevice.data(), guardedOutput.data());
    if (!guardedThis || !guardedOutput || bAliases) return false;

    const bool bSecondSnapshotCurrent = guardedThis->isSourceDeviceSnapshotCurrent(pContext->sourceSnapshot, guardedThis->getDevice(), pPdStruct);
    if (!guardedThis || !bSecondSnapshotCurrent) return false;

    const MISH_BLOCK mishBlock = pContext->listMishBlocks.at(pState->nCurrentIndex);
    const QList<BLOCK_DATA> listCurrentStripes = pContext->listStripes.at(pState->nCurrentIndex);
    if (mishBlock.nSectorCount > (quint64)(std::numeric_limits<qint64>::max)() / (quint64)DMG_SECTOR_SIZE) return false;
    const qint64 nExpectedOutput = (qint64)(mishBlock.nSectorCount * (quint64)DMG_SECTOR_SIZE);
    if (!XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nExpectedOutput)) return false;

    // This override decodes the stripes into a private stage and publishes the
    // result, so it bypasses the base decode chain and must charge the budget
    // itself: one entry, nExpectedOutput produced bytes.  The partition is
    // either produced whole (staging.size() == nExpectedOutput is required
    // below) or the extraction fails, so this is the exact produced size.  The
    // publish copy is not a second charge.
    if (pState->spOutputBudget) {
        QString sRecordName;
        if (pState->nCurrentIndex < pContext->listPartitionNames.size()) {
            sRecordName = pContext->listPartitionNames.at(pState->nCurrentIndex);
        }
        if (sRecordName.isEmpty()) {
            sRecordName = QString("partition%1").arg(pContext->nCurrentFileIndex);
        }
        sRecordName += ".img";
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, sRecordName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(nExpectedOutput)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    const bool bCheckCRC = XBinary::isUnpackCRCEnabled(pState->mapUnpackProperties, XBinary::CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    QTemporaryFile staging;
    if (!staging.open()) return false;
    bool bResult = true;

    for (qint32 i = 0; (i < listCurrentStripes.size()) && bResult && isPdStructNotCanceled(pPdStruct); i++) {
        bResult = guardedThis->_decompressStripe(listCurrentStripes.at(i), pContext->nDataForkOffset, pContext->nDataForkLength, (qint64)mishBlock.nDataOffset, &staging,
                                                 pState->mapUnpackProperties, pPdStruct);
        if (!guardedThis) return false;
    }

    bResult = bResult && isPdStructNotCanceled(pPdStruct) && (staging.size() == nExpectedOutput);
    if (bResult && bCheckCRC) {
        bResult = guardedThis->_validatePartitionCRC(&staging, mishBlock, listCurrentStripes, pPdStruct);
        if (!guardedThis) return false;
    }
    bResult = bResult && staging.seek(0);

    if (!bResult) return false;
    const bool bFinalSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !guardedOutput || !bFinalSourceCurrent) return false;
    const bool bFinalSnapshotCurrent = guardedThis->isSourceDeviceSnapshotCurrent(pContext->sourceSnapshot, guardedThis->getDevice(), pPdStruct);
    if (!guardedThis || !guardedOutput || !bFinalSnapshotCurrent) return false;

    const bool bPublished = guardedThis->publishUnpackOutput(&staging, guardedOutput.data(), pState, pPdStruct);
    if (!guardedThis || !bPublished) return false;

    pState->nCurrentOffset = nExpectedOutput;
    return true;
}

bool XDMG::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    bool bResult = false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (isPdStructNotCanceled(pPdStruct) && pState && pState->pContext && (pState->nCurrentIndex >= 0) && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        DMG_UNPACK_CONTEXT *pContext = (DMG_UNPACK_CONTEXT *)pState->pContext;

        const bool bSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
        if (!guardedThis || !bSourceCurrent) return false;
        const bool bSnapshotCurrent = guardedThis->isSourceDeviceSnapshotCurrent(pContext->sourceSnapshot, guardedThis->getDevice(), pPdStruct);
        if (!guardedThis || !bSnapshotCurrent) return false;

        pContext->nCurrentFileIndex++;
        pState->nCurrentIndex++;

        if (pState->nCurrentIndex < pState->nNumberOfRecords) {
            bResult = true;
        }
    }

    return bResult;
}

bool XDMG::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    Q_UNUSED(pPdStruct)

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedThis->ownsUnpackSource(pState)) return false;
    DMG_UNPACK_CONTEXT *pContext = static_cast<DMG_UNPACK_CONTEXT *>(pState->pContext);
    pState->pContext = nullptr;
    guardedThis->releaseUnpackSource(pState);
    if (!guardedThis) return false;
    delete pContext;
    if (!guardedThis) return false;

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    return true;
}

XDMG::KOLY_BLOCK XDMG::readKolyBlock(QIODevice *pDevice, qint64 nOffset)
{
    KOLY_BLOCK result = {};

    // Snapshot through the last field consumed by the legacy reader.  Parsing
    // from memory prevents a self-destructing custom device from being reused
    // by the dozens of individual scalar reads and preserves the final source
    // position at nOffset + 500.
    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice) return result;
    const qint64 nDeviceSize = guardedDevice->size();
    if (!guardedDevice || (nOffset < 0) || (nDeviceSize < 512) || (nOffset > nDeviceSize - 512)) return result;
    QByteArray baData;
    if (!dmgReadExactSnapshot(guardedDevice.data(), nOffset, 500, &baData)) return result;

    result.nMagic = dmgReadBE32(baData, 0);
    result.nVersion = dmgReadBE32(baData, 4);
    result.nHeaderLength = dmgReadBE32(baData, 8);
    result.nFlags = dmgReadBE32(baData, 12);
    result.nRunningOffset = dmgReadBE64(baData, 16);
    result.nDataForkOffset = dmgReadBE64(baData, 24);
    result.nDataForkLength = dmgReadBE64(baData, 32);
    result.nResourceForkOffset = dmgReadBE64(baData, 40);
    result.nResourceForkLength = dmgReadBE64(baData, 48);
    result.nSegment = dmgReadBE32(baData, 56);
    result.nSegmentCount = dmgReadBE32(baData, 60);
    memcpy(result.segmentID, baData.constData() + 64, 16);

    for (qint32 i = 0; i < 34; i++) {
        result.dataChecksum[i] = dmgReadBE32(baData, 80 + i * 4);
        result.masterChecksum[i] = dmgReadBE32(baData, 352 + i * 4);
    }

    result.nXmlOffset = dmgReadBE64(baData, 216);
    result.nXmlLength = dmgReadBE64(baData, 224);
    result.nCodeSignatureOffset = dmgReadBE64(baData, 296);
    result.nCodeSignatureLength = dmgReadBE64(baData, 304);
    result.nImageVariant = dmgReadBE32(baData, 488);
    result.nSectorCount = dmgReadBE64(baData, 492);

    return result;
}

XDMG::MISH_BLOCK XDMG::readMishBlock(QIODevice *pDevice, qint64 nOffset)
{
    MISH_BLOCK result = {};

    QByteArray baData;
    if (!dmgReadExactSnapshot(pDevice, nOffset, 204, &baData)) return result;

    result.nMagic = dmgReadBE32(baData, 0);
    result.nVersion = dmgReadBE32(baData, 4);
    result.nStartSector = dmgReadBE64(baData, 8);
    result.nSectorCount = dmgReadBE64(baData, 16);
    result.nDataOffset = dmgReadBE64(baData, 24);
    result.nBufferCount = dmgReadBE32(baData, 32);
    result.nDescriptorBlocks = dmgReadBE32(baData, 36);
    for (qint32 i = 0; i < 34; i++) {
        result.checksum[i] = dmgReadBE32(baData, 64 + i * 4);
    }
    result.nBlockDataCount = dmgReadBE32(baData, 200);

    return result;
}

XDMG::BLOCK_DATA XDMG::readBlockData(QIODevice *pDevice, qint64 nOffset)
{
    BLOCK_DATA result = {};

    QByteArray baData;
    if (!dmgReadExactSnapshot(pDevice, nOffset, 40, &baData)) return result;

    result.nType = dmgReadBE32(baData, 0);
    result.nReserved = dmgReadBE32(baData, 4);
    result.nStartSector = dmgReadBE64(baData, 8);
    result.nSectorCount = dmgReadBE64(baData, 16);
    result.nDataOffset = dmgReadBE64(baData, 24);
    result.nDataLength = dmgReadBE64(baData, 32);

    return result;
}

bool XDMG::_parsePartition(const DMG_PARTITION_INFO &partitionInfo, const KOLY_BLOCK &kolyBlock, MISH_BLOCK *pMishBlock, QList<BLOCK_DATA> *pStripes, PDSTRUCT *pPdStruct)
{
    if (pStripes) pStripes->clear();
    if (!pMishBlock || !isPdStructNotCanceled(pPdStruct) || (partitionInfo.mishData.size() < 204) || ((quint64)partitionInfo.mishData.size() > DMG_MAX_MISH_SIZE)) {
        return false;
    }

    QByteArray baMishData = partitionInfo.mishData;
    QBuffer buffer(&baMishData);
    if (!buffer.open(QIODevice::ReadOnly)) return false;

    const MISH_BLOCK mishBlock = readMishBlock(&buffer, 0);
    if ((mishBlock.nMagic != 0x6d697368) || (mishBlock.nVersion != 1) || (mishBlock.nBlockDataCount == 0) ||
        (mishBlock.nBlockDataCount > DMG_MAX_STRIPES_PER_PARTITION) || (mishBlock.nSectorCount > ((quint64)(std::numeric_limits<qint64>::max)() / DMG_SECTOR_SIZE)) ||
        (mishBlock.nStartSector > kolyBlock.nSectorCount) || (mishBlock.nSectorCount > (kolyBlock.nSectorCount - mishBlock.nStartSector)) ||
        (mishBlock.nDataOffset > kolyBlock.nDataForkLength)) {
        return false;
    }
    if (!dmgChecksumDescriptorValid(mishBlock.checksum)) return false;

    const quint64 nExpectedMishSize = 204ULL + ((quint64)mishBlock.nBlockDataCount * 40ULL);
    if (nExpectedMishSize != (quint64)partitionInfo.mishData.size()) return false;

    QList<BLOCK_DATA> listStripes;
    if (pStripes) listStripes.reserve((qint32)mishBlock.nBlockDataCount);

    quint64 nCoveredSectors = 0;
    quint64 nTotalPackedSize = 0;
    bool bEndSeen = false;
    for (quint32 i = 0; (i < mishBlock.nBlockDataCount) && isPdStructNotCanceled(pPdStruct); i++) {
        const BLOCK_DATA stripe = readBlockData(&buffer, 204 + ((qint64)i * 40));
        if (stripe.nStartSector != nCoveredSectors) return false;

        const bool bControl = (stripe.nType == DMG_STRIPE_SKIP) || (stripe.nType == DMG_STRIPE_END);
        if (bControl) {
            if ((stripe.nSectorCount != 0) || (stripe.nDataOffset != 0) || (stripe.nDataLength != 0) || bEndSeen) return false;
            if (stripe.nType == DMG_STRIPE_END) {
                if (i != (mishBlock.nBlockDataCount - 1)) return false;
                bEndSeen = true;
            }
        } else {
            if ((stripe.nSectorCount == 0) || (stripe.nSectorCount > ((quint64)(std::numeric_limits<qint64>::max)() / DMG_SECTOR_SIZE)) ||
                (nCoveredSectors > mishBlock.nSectorCount) || (stripe.nSectorCount > (mishBlock.nSectorCount - nCoveredSectors))) {
                return false;
            }

            const quint64 nExpectedOutput = stripe.nSectorCount * DMG_SECTOR_SIZE;
            switch (stripe.nType) {
                case DMG_STRIPE_EMPTY:
                case DMG_STRIPE_ZEROES:
                    if (stripe.nDataLength != 0) return false;
                    break;

                case DMG_STRIPE_STORED:
                    if (stripe.nDataLength != nExpectedOutput) return false;
                    break;

                case DMG_STRIPE_ADC:
                case DMG_STRIPE_DEFLATE:
                case DMG_STRIPE_BZ:
                case DMG_STRIPE_LZFSE:
                case DMG_STRIPE_XZ:
                    if (stripe.nDataLength == 0) return false;
                    break;

                default: return false;
            }

            if ((stripe.nType != DMG_STRIPE_EMPTY) && (stripe.nType != DMG_STRIPE_ZEROES)) {
                if (stripe.nDataLength > ((quint64)(std::numeric_limits<qint64>::max)() - nTotalPackedSize)) {
                    return false;
                }
                nTotalPackedSize += stripe.nDataLength;
                const quint64 nRemainingFork = kolyBlock.nDataForkLength - mishBlock.nDataOffset;
                if (!dmgRangeWithin(stripe.nDataOffset, stripe.nDataLength, nRemainingFork)) return false;
            }

            nCoveredSectors += stripe.nSectorCount;
        }

        if (pStripes) listStripes.append(stripe);
    }

    if (!isPdStructNotCanceled(pPdStruct) || !bEndSeen || (nCoveredSectors != mishBlock.nSectorCount)) {
        return false;
    }

    *pMishBlock = mishBlock;
    if (pStripes) *pStripes = listStripes;
    return true;
}

bool XDMG::_parseAllPartitions(const QList<DMG_PARTITION_INFO> &listPartitions, const KOLY_BLOCK &kolyBlock, QList<MISH_BLOCK> *pMishBlocks,
                               QList<QList<BLOCK_DATA>> *pStripes, PDSTRUCT *pPdStruct)
{
    if (pMishBlocks) pMishBlocks->clear();
    if (pStripes) pStripes->clear();
    if (!pMishBlocks || listPartitions.isEmpty() || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    quint64 nExpectedStartSector = 0;
    quint32 nMasterCRC = 0xFFFFFFFFU;
    bool bCanValidateMasterCRC = true;
    for (qint32 i = 0; (i < listPartitions.size()) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        MISH_BLOCK mishBlock = {};
        QList<BLOCK_DATA> listCurrentStripes;
        if (!_parsePartition(listPartitions.at(i), kolyBlock, &mishBlock, pStripes ? &listCurrentStripes : nullptr, pPdStruct) ||
            (mishBlock.nStartSector != nExpectedStartSector) || (mishBlock.nSectorCount > kolyBlock.nSectorCount - nExpectedStartSector)) {
            pMishBlocks->clear();
            if (pStripes) pStripes->clear();
            return false;
        }

        nExpectedStartSector += mishBlock.nSectorCount;
        if ((mishBlock.checksum[1] & 7U) != 0) bCanValidateMasterCRC = false;
        const qint32 nChecksumBytes = (qint32)(mishBlock.checksum[1] >> 3);
        if ((nChecksumBytes < 0) || (nChecksumBytes > 128)) return false;
        if (nChecksumBytes > 0) {
            QByteArray baChecksumBytes(nChecksumBytes, 0);
            for (qint32 nByte = 0; nByte < nChecksumBytes; nByte++) {
                const quint32 nWord = mishBlock.checksum[2 + (nByte >> 2)];
                baChecksumBytes[nByte] = (char)((nWord >> (24 - ((nByte & 3) * 8))) & 0xFFU);
            }
            nMasterCRC = XBinary::_getCRC32(baChecksumBytes.constData(), baChecksumBytes.size(), nMasterCRC, XBinary::_getCRC32Table_EDB88320());
        }

        pMishBlocks->append(mishBlock);
        if (pStripes) pStripes->append(listCurrentStripes);
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || (nExpectedStartSector != kolyBlock.nSectorCount) ||
        (bCanValidateMasterCRC && dmgChecksumIsCRC32(kolyBlock.masterChecksum) && ((nMasterCRC ^ 0xFFFFFFFFU) != kolyBlock.masterChecksum[2]))) {
        pMishBlocks->clear();
        if (pStripes) pStripes->clear();
        return false;
    }
    return true;
}

bool XDMG::_validatePartitionCRC(QIODevice *pDevice, const MISH_BLOCK &mishBlock, const QList<BLOCK_DATA> &listStripes, PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    if (!dmgChecksumDescriptorValid(mishBlock.checksum)) return false;
    if (!dmgChecksumIsCRC32(mishBlock.checksum)) return true;
    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedThis || !guardedDevice || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const bool bOpen = guardedDevice->isOpen();
    if (!guardedThis || !guardedDevice || !bOpen) return false;
    const bool bReadable = guardedDevice->isReadable();
    if (!guardedThis || !guardedDevice || !bReadable) return false;

    XBinary binary(guardedDevice.data());
    QByteArray baBuffer(0x10000, 0);
    quint32 nCRC = 0xFFFFFFFFU;
    for (qint32 i = 0; (i < listStripes.size()) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        const BLOCK_DATA &stripe = listStripes.at(i);
        if ((stripe.nType == DMG_STRIPE_ZEROES) || (stripe.nType == DMG_STRIPE_SKIP) || (stripe.nType == DMG_STRIPE_END)) continue;
        if ((stripe.nSectorCount > (quint64)(std::numeric_limits<qint64>::max)() / DMG_SECTOR_SIZE) ||
            (stripe.nStartSector > (quint64)(std::numeric_limits<qint64>::max)() / DMG_SECTOR_SIZE)) {
            return false;
        }
        const qint64 nOffset = (qint64)(stripe.nStartSector * DMG_SECTOR_SIZE);
        const qint64 nSize = (qint64)(stripe.nSectorCount * DMG_SECTOR_SIZE);
        qint64 nDone = 0;
        while ((nDone < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            const qint32 nChunk = (qint32)qMin<qint64>(baBuffer.size(), nSize - nDone);
            const qint64 nRead = binary.read_array_process(nOffset + nDone, baBuffer.data(), nChunk, pPdStruct);
            if (!guardedThis || !guardedDevice || (nRead != nChunk)) return false;
            nCRC = XBinary::_getCRC32(baBuffer.constData(), nChunk, nCRC, XBinary::_getCRC32Table_EDB88320());
            nDone += nChunk;
        }
    }

    return guardedThis && guardedDevice && XBinary::isPdStructNotCanceled(pPdStruct) && ((nCRC ^ 0xFFFFFFFFU) == mishBlock.checksum[2]);
}

QList<XDMG::DMG_PARTITION_INFO> XDMG::_parseBlkxPartitions(const QByteArray &baXml, PDSTRUCT *pPdStruct)
{
    QList<DMG_PARTITION_INFO> listResult;

    if (baXml.isEmpty() || ((quint64)baXml.size() > DMG_MAX_XML_SIZE) || !isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    QXmlStreamReader reader(baXml);
    qint32 nDepth = 0;
    qint32 nPlistDepth = -1;
    qint32 nRootDictDepth = -1;
    qint32 nResourceForkDictDepth = -1;
    qint32 nBlkxArrayDepth = -1;
    qint32 nPartitionDictDepth = -1;
    bool bWaitingForRootDict = false;
    bool bDtdSeen = false;
    bool bFoundPlist = false;
    bool bClosedRootDict = false;
    bool bFoundResourceFork = false;
    bool bClosedResourceFork = false;
    bool bFoundBlkx = false;
    bool bClosedBlkx = false;
    bool bMalformed = false;
    bool bRootKeyPending = false;
    bool bResourceKeyPending = false;
    bool bPartitionKeyPending = false;
    QString sRootPendingKey;
    QString sResourcePendingKey;
    QString sPendingKey;
    QString sName;
    QString sCFName;
    QByteArray baMishData;
    bool bNameSeen = false;
    bool bCFNameSeen = false;
    bool bDataSeen = false;

    while (!reader.atEnd() && isPdStructNotCanceled(pPdStruct) && !bMalformed) {
        const QXmlStreamReader::TokenType token = reader.readNext();

        if (token == QXmlStreamReader::DTD) {
            const QString sPublicId = reader.dtdPublicId().toString();
            const QString sSystemId = reader.dtdSystemId().toString();
            const bool bNoExternalId = sPublicId.isEmpty() && sSystemId.isEmpty();
            const bool bApplePlistId =
                ((sPublicId == QLatin1String("-//Apple//DTD PLIST 1.0//EN")) || (sPublicId == QLatin1String("-//Apple Computer//DTD PLIST 1.0//EN"))) &&
                (sSystemId == QLatin1String("http://www.apple.com/DTDs/PropertyList-1.0.dtd"));
            if (bDtdSeen || bFoundPlist || (nDepth != 0) || (reader.dtdName() != QLatin1String("plist")) || !reader.entityDeclarations().isEmpty() ||
                !reader.notationDeclarations().isEmpty() || (!bNoExternalId && !bApplePlistId)) {
                bMalformed = true;
            } else {
                // QXmlStreamReader does not fetch an external subset.  Accept
                // the conventional plist prolog DTD, but never declarations
                // that Qt could expand before the outer token loop sees them.
                bDtdSeen = true;
            }
            continue;
        }
        if ((token == QXmlStreamReader::EntityReference) || (token == QXmlStreamReader::Invalid)) {
            bMalformed = true;
            break;
        }

        if (token == QXmlStreamReader::StartElement) {
            nDepth++;
            if (nDepth > DMG_MAX_XML_DEPTH) {
                bMalformed = true;
                break;
            }
            // Apple plist vocabulary is unnamespaced.  QXmlStreamReader::name()
            // is only the local name, so reject QName aliases explicitly.
            if (!reader.namespaceUri().isEmpty()) {
                bMalformed = true;
                break;
            }

            const auto elementName = reader.name();
            if (bWaitingForRootDict) {
                if (((nDepth - 1) != nPlistDepth) || (elementName != QLatin1String("dict"))) {
                    bMalformed = true;
                    break;
                }
                bWaitingForRootDict = false;
                nRootDictDepth = nDepth;
                continue;
            }

            if (elementName == QLatin1String("plist")) {
                if ((nDepth != 1) || bFoundPlist || (nPlistDepth >= 0)) {
                    bMalformed = true;
                    break;
                }
                bFoundPlist = true;
                nPlistDepth = nDepth;
                bWaitingForRootDict = true;
                continue;
            }

            if (elementName == QLatin1String("key")) {
                const qint32 nParentDepth = nDepth - 1;
                const QString sKey = reader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
                nDepth--;
                if (reader.hasError()) {
                    bMalformed = true;
                    break;
                }

                if (nParentDepth == nRootDictDepth) {
                    if (bRootKeyPending) {
                        bMalformed = true;
                        break;
                    }
                    sRootPendingKey = sKey;
                    bRootKeyPending = true;
                } else if (nParentDepth == nResourceForkDictDepth) {
                    if (bResourceKeyPending) {
                        bMalformed = true;
                        break;
                    }
                    sResourcePendingKey = sKey;
                    bResourceKeyPending = true;
                } else if ((nPartitionDictDepth >= 0) && (nParentDepth == nPartitionDictDepth)) {
                    if (bPartitionKeyPending) {
                        bMalformed = true;
                        break;
                    }
                    sPendingKey = sKey;
                    bPartitionKeyPending = true;
                } else {
                    bMalformed = true;
                    break;
                }
                continue;
            }

            if ((nRootDictDepth >= 0) && (nDepth == (nRootDictDepth + 1))) {
                if (!bRootKeyPending) {
                    bMalformed = true;
                    break;
                }
                if (sRootPendingKey == QLatin1String("resource-fork")) {
                    if ((elementName != QLatin1String("dict")) || bFoundBlkx || bFoundResourceFork || bClosedResourceFork) {
                        bMalformed = true;
                        break;
                    }
                    bFoundResourceFork = true;
                    nResourceForkDictDepth = nDepth;
                } else if (sRootPendingKey == QLatin1String("blkx")) {
                    if ((elementName != QLatin1String("array")) || bFoundBlkx || bClosedBlkx || bFoundResourceFork) {
                        bMalformed = true;
                        break;
                    }
                    bFoundBlkx = true;
                    nBlkxArrayDepth = nDepth;
                } else {
                    if (!dmgSkipCurrentElementBounded(&reader, nDepth, pPdStruct)) bMalformed = true;
                    nDepth--;
                    if (reader.hasError()) bMalformed = true;
                }
                sRootPendingKey.clear();
                bRootKeyPending = false;
                continue;
            }

            if ((nResourceForkDictDepth >= 0) && (nDepth == (nResourceForkDictDepth + 1))) {
                if (!bResourceKeyPending) {
                    bMalformed = true;
                    break;
                }
                if (sResourcePendingKey == QLatin1String("blkx")) {
                    if ((elementName != QLatin1String("array")) || bFoundBlkx || bClosedBlkx) {
                        bMalformed = true;
                        break;
                    }
                    bFoundBlkx = true;
                    nBlkxArrayDepth = nDepth;
                } else {
                    if (!dmgSkipCurrentElementBounded(&reader, nDepth, pPdStruct)) bMalformed = true;
                    nDepth--;
                    if (reader.hasError()) bMalformed = true;
                }
                sResourcePendingKey.clear();
                bResourceKeyPending = false;
                continue;
            }

            if ((nPlistDepth >= 0) && (nDepth == (nPlistDepth + 1)) && (nRootDictDepth < 0)) {
                // A plist contains exactly one root value.  The first one was
                // required to be a dictionary above.
                bMalformed = true;
                break;
            }

            if (nBlkxArrayDepth >= 0) {
                if (nPartitionDictDepth < 0) {
                    if ((nDepth != (nBlkxArrayDepth + 1)) || (elementName != QLatin1String("dict"))) {
                        bMalformed = true;
                        break;
                    }
                    nPartitionDictDepth = nDepth;
                    sPendingKey.clear();
                    bPartitionKeyPending = false;
                    sName.clear();
                    sCFName.clear();
                    baMishData.clear();
                    bNameSeen = false;
                    bCFNameSeen = false;
                    bDataSeen = false;
                    continue;
                }

                if (nDepth == (nPartitionDictDepth + 1)) {
                    if (!bPartitionKeyPending) {
                        bMalformed = true;
                        break;
                    }

                    const bool bStringValue = (elementName == QLatin1String("string"));
                    const bool bDataValue = (elementName == QLatin1String("data"));
                    if (((sPendingKey == QLatin1String("Name")) || (sPendingKey == QLatin1String("CFName"))) && !bStringValue) {
                        bMalformed = true;
                        break;
                    }
                    if ((sPendingKey == QLatin1String("Data")) && !bDataValue) {
                        bMalformed = true;
                        break;
                    }

                    if (bStringValue || bDataValue) {
                        const QString sValue = reader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
                        nDepth--;
                        if (reader.hasError()) {
                            bMalformed = true;
                            break;
                        }

                        if (sPendingKey == QLatin1String("Name")) {
                            if (bNameSeen) bMalformed = true;
                            bNameSeen = true;
                            sName = sValue.trimmed();
                            if (sName.size() > 4096) bMalformed = true;
                        } else if (sPendingKey == QLatin1String("CFName")) {
                            if (bCFNameSeen) bMalformed = true;
                            bCFNameSeen = true;
                            sCFName = sValue.trimmed();
                            if (sCFName.size() > 4096) bMalformed = true;
                        } else if (sPendingKey == QLatin1String("Data")) {
                            if (bDataSeen || !dmgDecodeBase64(sValue, &baMishData)) bMalformed = true;
                            bDataSeen = true;
                        }
                        sPendingKey.clear();
                        bPartitionKeyPending = false;
                        continue;
                    }

                    // Unknown plist values (for example the nested Attributes
                    // dictionary) are ignored as a complete bounded subtree.
                    if (!dmgSkipCurrentElementBounded(&reader, nDepth, pPdStruct)) bMalformed = true;
                    nDepth--;
                    sPendingKey.clear();
                    bPartitionKeyPending = false;
                    if (reader.hasError()) bMalformed = true;
                    continue;
                }
            }
        } else if (token == QXmlStreamReader::Characters) {
            if (!reader.isWhitespace()) bMalformed = true;
        } else if (token == QXmlStreamReader::EndElement) {
            const auto elementName = reader.name();
            if ((nPartitionDictDepth == nDepth) && (elementName == QLatin1String("dict"))) {
                if (bPartitionKeyPending || !bDataSeen || baMishData.isEmpty() || (listResult.size() >= DMG_MAX_PARTITIONS)) {
                    bMalformed = true;
                } else {
                    DMG_PARTITION_INFO info = {};
                    info.sName = !sName.isEmpty() ? sName : sCFName;
                    if (info.sName.isEmpty()) info.sName = QString("partition%1").arg(listResult.size());
                    info.mishData = baMishData;
                    listResult.append(info);
                }
                nPartitionDictDepth = -1;
            } else if ((nBlkxArrayDepth == nDepth) && (elementName == QLatin1String("array"))) {
                if (nPartitionDictDepth >= 0) bMalformed = true;
                nBlkxArrayDepth = -1;
                bClosedBlkx = true;
            } else if ((nResourceForkDictDepth == nDepth) && (elementName == QLatin1String("dict"))) {
                if (bResourceKeyPending || !bFoundBlkx || !bClosedBlkx) bMalformed = true;
                nResourceForkDictDepth = -1;
                bClosedResourceFork = true;
            } else if ((nRootDictDepth == nDepth) && (elementName == QLatin1String("dict"))) {
                if (bRootKeyPending || !bFoundBlkx || (bFoundResourceFork && !bClosedResourceFork)) bMalformed = true;
                nRootDictDepth = -1;
                bClosedRootDict = true;
            } else if ((nPlistDepth == nDepth) && (elementName == QLatin1String("plist"))) {
                if (bWaitingForRootDict || !bClosedRootDict || (nRootDictDepth >= 0)) bMalformed = true;
                nPlistDepth = -1;
            }
            nDepth--;
            if (nDepth < 0) bMalformed = true;
        }
    }

    if (reader.hasError() || bMalformed || !isPdStructNotCanceled(pPdStruct) || bWaitingForRootDict || bRootKeyPending || bResourceKeyPending || bPartitionKeyPending ||
        !bFoundPlist || !bClosedRootDict || !bFoundBlkx || !bClosedBlkx || (bFoundResourceFork && !bClosedResourceFork) || (nPlistDepth >= 0) || (nRootDictDepth >= 0) ||
        (nResourceForkDictDepth >= 0) || (nDepth != 0)) {
        listResult.clear();
    }

    return listResult;
}

QList<XDMG::DMG_PARTITION_INFO> XDMG::_parseResourceForkPartitions(const QByteArray &baResource, PDSTRUCT *pPdStruct)
{
    QList<DMG_PARTITION_INFO> listResult;
    if ((baResource.size() < 0x100) || ((quint64)baResource.size() > DMG_MAX_RESOURCE_SIZE) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    const auto read16 = [&](quint32 nOffset, quint16 *pValue) -> bool {
        if (!pValue || (nOffset > (quint32)baResource.size()) || ((quint32)baResource.size() - nOffset < 2)) return false;
        const uchar *p = reinterpret_cast<const uchar *>(baResource.constData() + nOffset);
        *pValue = ((quint16)p[0] << 8) | p[1];
        return true;
    };
    const auto read32 = [&](quint32 nOffset, quint32 *pValue) -> bool {
        if (!pValue || (nOffset > (quint32)baResource.size()) || ((quint32)baResource.size() - nOffset < 4)) return false;
        const uchar *p = reinterpret_cast<const uchar *>(baResource.constData() + nOffset);
        *pValue = ((quint32)p[0] << 24) | ((quint32)p[1] << 16) | ((quint32)p[2] << 8) | p[3];
        return true;
    };

    quint32 nDataOffset = 0;
    quint32 nMapOffset = 0;
    quint32 nDataLength = 0;
    quint32 nMapLength = 0;
    if (!read32(0, &nDataOffset) || !read32(4, &nMapOffset) || !read32(8, &nDataLength) || !read32(12, &nMapLength) || (nDataOffset != 0x100) ||
        ((quint64)nDataOffset + nDataLength != nMapOffset) || ((quint64)nMapOffset + nMapLength > (quint64)baResource.size()) || (nMapLength < 0x1e)) {
        return listResult;
    }

    const quint64 nMapEnd = (quint64)nMapOffset + nMapLength;
    const quint64 nTrailing = (quint64)baResource.size() - nMapEnd;
    quint32 nTrailingValue = 0;
    if ((nTrailing != 0) && ((nTrailing != 4) || !read32((quint32)nMapEnd, &nTrailingValue) || (nTrailingValue != 0))) {
        return listResult;
    }
    if (memcmp(baResource.constData(), baResource.constData() + nMapOffset, 16) != 0) {
        return listResult;
    }

    quint16 nTypeListOffset = 0;
    quint16 nNameListOffset = 0;
    quint16 nTypesMinusOne = 0;
    if (!read16(nMapOffset + 0x18, &nTypeListOffset) || !read16(nMapOffset + 0x1a, &nNameListOffset) || !read16(nMapOffset + 0x1c, &nTypesMinusOne) ||
        (nTypeListOffset != 0x1c) || (nNameListOffset > nMapLength)) {
        return listResult;
    }

    const quint32 nNumTypes = (quint32)nTypesMinusOne + 1;
    const quint64 nTypeEntriesEnd = 0x1eULL + (quint64)nNumTypes * 8ULL;
    if (nTypeEntriesEnd > nNameListOffset) return listResult;

    const quint32 nNameBlockOffset = nMapOffset + nNameListOffset;
    const quint32 nNameBlockSize = nMapLength - nNameListOffset;
    quint64 nTotalReferences = 0;
    const quint32 nBlkxType = 0x626c6b78;  // blkx
    for (quint32 i = 0; (i < nNumTypes) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        const quint32 nTypeOffset = nMapOffset + 0x1e + i * 8;
        quint32 nType = 0;
        quint16 nResourcesMinusOne = 0;
        quint16 nReferenceListOffset = 0;
        if (!read32(nTypeOffset, &nType) || !read16(nTypeOffset + 4, &nResourcesMinusOne) || !read16(nTypeOffset + 6, &nReferenceListOffset)) {
            listResult.clear();
            return listResult;
        }

        const quint32 nNumResources = (quint32)nResourcesMinusOne + 1;
        nTotalReferences += nNumResources;
        const quint64 nReferenceStart = 0x1cULL + nReferenceListOffset;
        const quint64 nReferenceEnd = nReferenceStart + (quint64)nNumResources * 12ULL;
        if ((nTotalReferences > 1000000ULL) || (nReferenceStart < nTypeEntriesEnd) || (nReferenceEnd > nNameListOffset)) {
            listResult.clear();
            return listResult;
        }

        for (quint32 k = 0; (k < nNumResources) && XBinary::isPdStructNotCanceled(pPdStruct); k++) {
            const quint32 nReferenceOffset = nMapOffset + (quint32)nReferenceStart + k * 12;
            quint16 nNameOffset = 0;
            quint32 nDataField = 0;
            if (!read16(nReferenceOffset + 2, &nNameOffset) || !read32(nReferenceOffset + 4, &nDataField)) {
                listResult.clear();
                return listResult;
            }

            const quint32 nBlockOffset = nDataField & 0x00FFFFFFU;
            quint32 nBlockSize = 0;
            if (nBlockOffset > nDataLength || (nDataLength - nBlockOffset < 4) || !read32(nDataOffset + nBlockOffset, &nBlockSize) ||
                (nBlockSize > (nDataLength - nBlockOffset - 4))) {
                listResult.clear();
                return listResult;
            }

            QString sName;
            if (nNameOffset != 0xFFFFU) {
                if (nNameOffset >= nNameBlockSize) {
                    listResult.clear();
                    return listResult;
                }
                const uchar nNameLength = (uchar)baResource.at(nNameBlockOffset + nNameOffset);
                if ((quint32)nNameLength >= (nNameBlockSize - nNameOffset)) {
                    listResult.clear();
                    return listResult;
                }
                const char *pName = baResource.constData() + nNameBlockOffset + nNameOffset + 1;
                sName = dmgFromMacRoman(pName, nNameLength);
            }

            if (nType == nBlkxType) {
                if ((nBlockSize < 204) || ((quint64)nBlockSize > DMG_MAX_MISH_SIZE) || (listResult.size() >= DMG_MAX_PARTITIONS)) {
                    listResult.clear();
                    return listResult;
                }
                DMG_PARTITION_INFO info = {};
                info.sName = sName;
                info.mishData = baResource.mid((qint32)(nDataOffset + nBlockOffset + 4), (qint32)nBlockSize);
                listResult.append(info);
            }
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) listResult.clear();
    return listResult;
}

bool XDMG::_decompressStripe(const BLOCK_DATA &stripe, qint64 nDataForkOffset, qint64 nDataForkLength, qint64 nMishDataOffset, QIODevice *pDevice,
                             const QMap<UNPACK_PROP, QVariant> &mapUnpackProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!guardedThis || !guardedOutput || !XBinary::isPdStructNotCanceled(pPdStruct) || (nDataForkOffset < 0) || (nDataForkLength < 0) || (nMishDataOffset < 0) ||
        (nMishDataOffset > nDataForkLength) || (stripe.nSectorCount > (quint64)(std::numeric_limits<qint64>::max)() / DMG_SECTOR_SIZE)) {
        return false;
    }

    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedSource) return false;
    const auto allDevicesAreCurrent = [&guardedThis, &guardedSource, &guardedOutput]() -> bool {
        return guardedThis && guardedSource && guardedOutput && (guardedThis->getDevice() == guardedSource.data());
    };
    const bool bOutputOpen = guardedOutput->isOpen();
    if (!allDevicesAreCurrent() || !bOutputOpen) return false;
    const bool bOutputWritable = guardedOutput->isWritable();
    if (!allDevicesAreCurrent() || !bOutputWritable) return false;
    const qint64 nFileSize = guardedSource->size();
    if (!allDevicesAreCurrent() || (nFileSize < 0)) return false;

    const qint64 nExpectedSize = (qint64)(stripe.nSectorCount * DMG_SECTOR_SIZE);
    const auto getDataRange = [&](qint64 *pOffset, qint64 *pSize) -> bool {
        if (!pOffset || !pSize || (stripe.nDataOffset > (quint64)(std::numeric_limits<qint64>::max)()) ||
            (stripe.nDataLength > (quint64)(std::numeric_limits<qint64>::max)())) {
            return false;
        }
        const qint64 nRelativeOffset = (qint64)stripe.nDataOffset;
        const qint64 nLength = (qint64)stripe.nDataLength;
        if ((nRelativeOffset > (nDataForkLength - nMishDataOffset)) || (nLength > (nDataForkLength - nMishDataOffset - nRelativeOffset)) ||
            (nMishDataOffset > ((std::numeric_limits<qint64>::max)() - nDataForkOffset)) ||
            (nRelativeOffset > ((std::numeric_limits<qint64>::max)() - nDataForkOffset - nMishDataOffset))) {
            return false;
        }
        const qint64 nAbsoluteOffset = nDataForkOffset + nMishDataOffset + nRelativeOffset;
        if ((nAbsoluteOffset > nFileSize) || (nLength > (nFileSize - nAbsoluteOffset))) return false;
        *pOffset = nAbsoluteOffset;
        *pSize = nLength;
        return true;
    };

    switch (stripe.nType) {
        case DMG_STRIPE_EMPTY:
        case DMG_STRIPE_ZEROES: {
            const bool bWritten = guardedThis->_writeZeroes(guardedOutput.data(), nExpectedSize, pPdStruct);
            return allDevicesAreCurrent() && bWritten;
        }

        case DMG_STRIPE_STORED: {
            qint64 nInputOffset = 0;
            qint64 nInputSize = 0;
            if (!getDataRange(&nInputOffset, &nInputSize) || (nInputSize != nExpectedSize)) return false;

            QByteArray baBuffer(0x10000, 0);
            qint64 nDone = 0;
            while ((nDone < nInputSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                const qint32 nChunk = (qint32)qMin<qint64>(baBuffer.size(), nInputSize - nDone);
                const qint64 nRead = XBinary::read_array_process(guardedSource.data(), nInputOffset + nDone, baBuffer.data(), nChunk, pPdStruct);
                if (!allDevicesAreCurrent() || (nRead != nChunk)) {
                    return false;
                }
                const bool bWritten = dmgWriteAll(guardedOutput.data(), baBuffer.constData(), nChunk, pPdStruct);
                if (!allDevicesAreCurrent() || !bWritten) {
                    return false;
                }
                nDone += nChunk;
            }
            return allDevicesAreCurrent() && (nDone == nInputSize) && XBinary::isPdStructNotCanceled(pPdStruct);
        }

        case DMG_STRIPE_DEFLATE: {
            qint64 nInputOffset = 0;
            qint64 nInputSize = 0;
            if (!getDataRange(&nInputOffset, &nInputSize) || (nInputSize <= 0)) return false;

            QByteArray baInput(0x10000, 0);
            QByteArray baOutput(0x10000, 0);
            z_stream stream = {};
            if (inflateInit(&stream) != Z_OK) return false;

            bool bResult = true;
            qint64 nUnread = nInputSize;
            qint64 nReadOffset = nInputOffset;
            qint64 nOutputSize = 0;
            int nResult = Z_OK;

            while ((nResult != Z_STREAM_END) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                if ((stream.avail_in == 0) && (nUnread > 0)) {
                    const qint32 nChunk = (qint32)qMin<qint64>(baInput.size(), nUnread);
                    const qint64 nRead = XBinary::read_array_process(guardedSource.data(), nReadOffset, baInput.data(), nChunk, pPdStruct);
                    if (!allDevicesAreCurrent() || (nRead != nChunk)) {
                        bResult = false;
                        break;
                    }
                    nReadOffset += nChunk;
                    nUnread -= nChunk;
                    stream.next_in = reinterpret_cast<Bytef *>(baInput.data());
                    stream.avail_in = (uInt)nChunk;
                }

                stream.next_out = reinterpret_cast<Bytef *>(baOutput.data());
                stream.avail_out = (uInt)baOutput.size();
                nResult = inflate(&stream, Z_NO_FLUSH);
                if ((nResult != Z_OK) && (nResult != Z_STREAM_END)) {
                    bResult = false;
                    break;
                }

                const qint64 nProduced = baOutput.size() - stream.avail_out;
                if ((nOutputSize > nExpectedSize) || (nProduced > (nExpectedSize - nOutputSize))) {
                    bResult = false;
                    break;
                }
                if (nProduced > 0) {
                    const bool bWritten = dmgWriteAll(guardedOutput.data(), baOutput.constData(), nProduced, pPdStruct);
                    if (!allDevicesAreCurrent() || !bWritten) {
                        bResult = false;
                        break;
                    }
                }
                nOutputSize += nProduced;

                if ((nResult == Z_OK) && (nProduced == 0) && (stream.avail_in == 0) && (nUnread == 0)) {
                    bResult = false;
                    break;
                }
            }

            const bool bExactInput = (nUnread == 0) && (stream.avail_in == 0);
            inflateEnd(&stream);
            return allDevicesAreCurrent() && bResult && XBinary::isPdStructNotCanceled(pPdStruct) && (nResult == Z_STREAM_END) && (nOutputSize == nExpectedSize) &&
                   bExactInput;
        }

        case DMG_STRIPE_BZ: {
            qint64 nInputOffset = 0;
            qint64 nInputSize = 0;
            if (!getDataRange(&nInputOffset, &nInputSize) || (nInputSize <= 0)) return false;

            // The shared BZIP2 decoder defines logical output offset zero and
            // seeks there during state preparation.  Isolate each stripe so
            // it cannot overwrite bytes already appended by earlier stripes.
            QTemporaryFile stripeStaging;
            if (!stripeStaging.open()) return false;
            DATAPROCESS_STATE state = {};
            state.mapUnpackProperties = mapUnpackProperties;
            state.pDeviceInput = guardedSource.data();
            state.pDeviceOutput = &stripeStaging;
            state.nInputOffset = nInputOffset;
            state.nInputLimit = nInputSize;
            state.nProcessedOffset = 0;
            state.nProcessedLimit = nExpectedSize;
            state.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nExpectedSize);
            const bool bDecoded = XBZIP2Decoder::decompress(&state, pPdStruct);
            if (!allDevicesAreCurrent() || !bDecoded || (state.nCountInput != nInputSize) || (state.nCountOutput != nExpectedSize) || state.bReadError ||
                state.bWriteError || (stripeStaging.size() != nExpectedSize) || !XBinary::isPdStructNotCanceled(pPdStruct) || !stripeStaging.seek(0)) {
                return false;
            }

            QByteArray baBuffer(0x10000, 0);
            qint64 nRemaining = nExpectedSize;
            while ((nRemaining > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                const qint32 nChunk = (qint32)qMin<qint64>(baBuffer.size(), nRemaining);
                if (stripeStaging.read(baBuffer.data(), nChunk) != nChunk) {
                    return false;
                }
                const bool bWritten = dmgWriteAll(guardedOutput.data(), baBuffer.constData(), nChunk, pPdStruct);
                if (!allDevicesAreCurrent() || !bWritten) {
                    return false;
                }
                nRemaining -= nChunk;
            }
            return allDevicesAreCurrent() && (nRemaining == 0) && XBinary::isPdStructNotCanceled(pPdStruct);
        }

        case DMG_STRIPE_ADC:
        case DMG_STRIPE_LZFSE:
        case DMG_STRIPE_XZ:
            // Unsupported stripes must never be reported as successfully
            // extracted with fabricated zero-filled data.
            return false;

        case DMG_STRIPE_SKIP:
        case DMG_STRIPE_END: return nExpectedSize == 0;

        default: return false;
    }
}

bool XDMG::_writeZeroes(QIODevice *pDevice, qint64 nSize, PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!guardedThis || !guardedOutput || (nSize < 0)) return false;
    QByteArray baZeroes(4096, 0);
    while ((nSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nToWrite = qMin(nSize, (qint64)baZeroes.size());
        const bool bWritten = dmgWriteAll(guardedOutput.data(), baZeroes.constData(), nToWrite, pPdStruct);
        if (!guardedThis || !guardedOutput || !bWritten) return false;
        nSize -= nToWrite;
    }
    return guardedThis && guardedOutput && (nSize == 0) && XBinary::isPdStructNotCanceled(pPdStruct);
}

QList<QString> XDMG::getSearchSignatures()
{
    QList<QString> listResult;

    // UDIF KOLY magic, version 4 and the fixed 512-byte header length.
    listResult.append("'koly'0000000400000200");

    return listResult;
}

XBinary::FFSEARCH_INFO XDMG::searchFFNext(FFSEARCH_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    FFSEARCH_INFO result = {};
    if (!guardedThis || !pState || !XBinary::isPdStructNotCanceled(pPdStruct)) return result;

    const bool bUsesOwnerSource = !pState->pDevice;
    QPointer<QIODevice> guardedSearchDevice(pState->pDevice ? pState->pDevice : guardedThis->getDevice());
    if (!guardedSearchDevice) return result;
    const auto searchIsAlive = [&guardedThis, &guardedSearchDevice, bUsesOwnerSource]() -> bool {
        return guardedThis && guardedSearchDevice && (!bUsesOwnerSource || (guardedThis->getDevice() == guardedSearchDevice.data()));
    };
    const bool bOpen = guardedSearchDevice->isOpen();
    if (!searchIsAlive() || !bOpen) return result;
    const bool bReadable = guardedSearchDevice->isReadable();
    if (!searchIsAlive() || !bReadable) return result;

    const qint64 nDeviceSize = guardedSearchDevice->size();
    if (!searchIsAlive() || (nDeviceSize < 0) || (pState->nStartOffset < 0) || (pState->nStartOffset > nDeviceSize)) {
        return result;
    }

    qint64 nRangeEnd = nDeviceSize;
    if (pState->nSize > 0) {
        nRangeEnd = pState->nStartOffset + qMin(pState->nSize, nDeviceSize - pState->nStartOffset);
    }
    const qint64 nCallStart = qMax(pState->nStartOffset, pState->nCurrentOffset);
    if ((nCallStart < 0) || (nCallStart > nRangeEnd)) return result;

    // No unread bytes remain.  Advancing to the range end makes repeated
    // no-match calls stable without skipping a possible header on a nonempty
    // range.
    if (nCallStart == nRangeEnd) {
        pState->nCurrentOffset = nRangeEnd;
        return result;
    }

    XBinary searchBinary(guardedSearchDevice.data());

    // Build standard prefix CRC checkpoints lazily up to the furthest range
    // queried.  Arbitrary fork CRCs then require at most two small edge reads;
    // coherent false KOLYs cannot rescan every preceding carrier byte.
    static const qint64 N_MIN_CRC_CHECKPOINT_SIZE = 0x1000;
    static const qint64 N_MAX_CRC_CHECKPOINTS = 4LL * 1024 * 1024;
    const qint64 nSearchSpan = nRangeEnd - nCallStart;
    qint64 nCrcCheckpointSize = N_MIN_CRC_CHECKPOINT_SIZE;
    const qint64 nCheckpointDenominator = N_MAX_CRC_CHECKPOINTS - 1;
    const qint64 nRequiredCheckpointSize = (nSearchSpan / nCheckpointDenominator) + ((nSearchSpan % nCheckpointDenominator) ? 1 : 0);
    if (nRequiredCheckpointSize > nCrcCheckpointSize) {
        nCrcCheckpointSize = nRequiredCheckpointSize;
        const qint64 nRemainder = nCrcCheckpointSize % N_MIN_CRC_CHECKPOINT_SIZE;
        if (nRemainder != 0) {
            nCrcCheckpointSize += N_MIN_CRC_CHECKPOINT_SIZE - nRemainder;
        }
    }
    QVector<quint32> listCrcCheckpoints;
    listCrcCheckpoints.append(0);
    qint64 nCrcCheckpointEnd = nCallStart;
    QByteArray baCrcBuffer(0x10000, 0);
    bool bCrcReadFailure = false;
    const auto ensureCrcCheckpointsTo = [&](qint64 nEndOffset) -> bool {
        if (!searchIsAlive() || bCrcReadFailure || (nEndOffset < nCallStart) || (nEndOffset > nRangeEnd) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        const qint64 nRelativeEnd = nEndOffset - nCallStart;
        const qint64 nCheckpointRelative = (nRelativeEnd / nCrcCheckpointSize) * nCrcCheckpointSize;
        const qint64 nCheckpointAbsolute = nCallStart + nCheckpointRelative;
        while ((nCrcCheckpointEnd < nCheckpointAbsolute) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            const qint64 nCheckpointAdvance = qMin(nCrcCheckpointSize, nCheckpointAbsolute - nCrcCheckpointEnd);
            const qint64 nNextCheckpoint = nCrcCheckpointEnd + nCheckpointAdvance;
            qint64 nReadOffset = nCrcCheckpointEnd;
            quint32 nNextCRC = listCrcCheckpoints.constLast();
            while ((nReadOffset < nNextCheckpoint) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                const qint32 nChunk = (qint32)qMin<qint64>(baCrcBuffer.size(), nNextCheckpoint - nReadOffset);
                if (searchBinary.read_array_process(nReadOffset, baCrcBuffer.data(), nChunk, pPdStruct) != nChunk) {
                    bCrcReadFailure = true;
                    return false;
                }
                if (!searchIsAlive()) {
                    bCrcReadFailure = true;
                    return false;
                }
                nNextCRC = dmgUpdateCRC32(nNextCRC, baCrcBuffer.constData(), nChunk);
                nReadOffset += nChunk;
            }
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            listCrcCheckpoints.append(nNextCRC);
            nCrcCheckpointEnd = nNextCheckpoint;
        }
        return XBinary::isPdStructNotCanceled(pPdStruct);
    };

    const auto crcPrefixesAt = [&](const QList<qint64> &listOffsets, QMap<qint64, quint32> *pPrefixes) -> bool {
        if (pPrefixes) pPrefixes->clear();
        if (!searchIsAlive() || !pPrefixes || bCrcReadFailure || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        // Sorting in a map lets every requested tail in one checkpoint be
        // evaluated by a single forward read.  In particular, thousands of
        // XML-derived bases cannot exhaust a shared read allowance before the
        // genuine base is reached.
        QMap<qint64, quint32> mapRequested;
        for (qint64 nOffset : listOffsets) {
            if ((nOffset < nCallStart) || (nOffset > nRangeEnd)) return false;
            mapRequested.insert(nOffset, 0);
        }
        if (mapRequested.isEmpty()) return true;
        auto itLastRequested = mapRequested.constEnd();
        --itLastRequested;
        if (!ensureCrcCheckpointsTo(itLastRequested.key())) return false;

        qint64 nActiveCheckpoint = -1;
        qint64 nReadOffset = -1;
        quint32 nCRC = 0;
        for (auto it = mapRequested.constBegin(); it != mapRequested.constEnd(); ++it) {
            const qint64 nRelativeEnd = it.key() - nCallStart;
            const qint64 nCheckpointRelative = (nRelativeEnd / nCrcCheckpointSize) * nCrcCheckpointSize;
            const qint64 nCheckpointAbsolute = nCallStart + nCheckpointRelative;
            if (nCheckpointAbsolute != nActiveCheckpoint) {
                const qint64 nCheckpointIndex = nCheckpointRelative / nCrcCheckpointSize;
                if ((nCheckpointIndex < 0) || (nCheckpointIndex >= (qint64)listCrcCheckpoints.size())) {
                    bCrcReadFailure = true;
                    return false;
                }
                nActiveCheckpoint = nCheckpointAbsolute;
                nReadOffset = nCheckpointAbsolute;
                nCRC = listCrcCheckpoints.at((qint32)nCheckpointIndex);
            }

            while ((nReadOffset < it.key()) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                const qint32 nTailSize = (qint32)qMin<qint64>(baCrcBuffer.size(), it.key() - nReadOffset);
                if (searchBinary.read_array_process(nReadOffset, baCrcBuffer.data(), nTailSize, pPdStruct) != nTailSize) {
                    bCrcReadFailure = true;
                    return false;
                }
                if (!searchIsAlive()) {
                    bCrcReadFailure = true;
                    return false;
                }
                nCRC = dmgUpdateCRC32(nCRC, baCrcBuffer.constData(), nTailSize);
                nReadOffset += nTailSize;
            }
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            pPrefixes->insert(it.key(), nCRC);
        }
        return true;
    };

    const auto crcRange = [&](qint64 nOffset, qint64 nLength, quint32 *pCRC) -> bool {
        if (pCRC) *pCRC = 0;
        if (!searchIsAlive() || !pCRC || (nOffset < nCallStart) || (nLength < 0) || (nOffset > nRangeEnd) || (nLength > nRangeEnd - nOffset)) {
            return false;
        }
        if (nLength == 0) {
            *pCRC = 0;
            return true;
        }

        QMap<qint64, quint32> mapPrefixes;
        if (!crcPrefixesAt({nOffset, nOffset + nLength}, &mapPrefixes) || !mapPrefixes.contains(nOffset) || !mapPrefixes.contains(nOffset + nLength)) {
            return false;
        }

        const quint32 nShiftedPrefix = dmgCombineCRC32(mapPrefixes.value(nOffset), 0, (quint64)nLength);
        *pCRC = mapPrefixes.value(nOffset + nLength) ^ nShiftedPrefix;
        return true;
    };

    const auto crcRangesForBases = [&](const QList<qint64> &listBases, quint64 nRelativeOffset, quint64 nLength, QMap<qint64, quint32> *pCRCs) -> bool {
        if (pCRCs) pCRCs->clear();
        if (!searchIsAlive() || !pCRCs || (nRelativeOffset > (quint64)(std::numeric_limits<qint64>::max)()) ||
            (nLength > (quint64)(std::numeric_limits<qint64>::max)())) {
            return false;
        }

        QList<qint64> listEndpoints;
        listEndpoints.reserve(listBases.size() * 2);
        const qint64 nRelative = (qint64)nRelativeOffset;
        const qint64 nSignedLength = (qint64)nLength;
        for (qint64 nBase : listBases) {
            if ((nBase < nCallStart) || (nRelative > nRangeEnd - nBase)) {
                return false;
            }
            const qint64 nStart = nBase + nRelative;
            if (nSignedLength > nRangeEnd - nStart) return false;
            listEndpoints.append(nStart);
            listEndpoints.append(nStart + nSignedLength);
        }

        QMap<qint64, quint32> mapPrefixes;
        if (!crcPrefixesAt(listEndpoints, &mapPrefixes)) return false;
        for (qint64 nBase : listBases) {
            const qint64 nStart = nBase + nRelative;
            const qint64 nEnd = nStart + nSignedLength;
            if (!mapPrefixes.contains(nStart) || !mapPrefixes.contains(nEnd)) {
                return false;
            }
            const quint32 nShiftedPrefix = dmgCombineCRC32(mapPrefixes.value(nStart), 0, nLength);
            pCRCs->insert(nBase, mapPrefixes.value(nEnd) ^ nShiftedPrefix);
        }
        return true;
    };

    const auto tryExactImage = [&](qint64 nOffset, qint64 nLength, const KOLY_BLOCK &rawKoly, bool bFrontKoly, const QMap<qint64, quint32> *pDataCRCs,
                                   FFSEARCH_INFO *pInfo) -> bool {
        if (!searchIsAlive() || !pInfo || (nOffset < nCallStart) || (nLength <= 0) || (nOffset > nRangeEnd) || (nLength > nRangeEnd - nOffset)) {
            return false;
        }

        const qint64 nPayloadSize = bFrontKoly ? nLength : nLength - 512;
        if ((nPayloadSize < 0) || !dmgRangeWithin(rawKoly.nDataForkOffset, rawKoly.nDataForkLength, (quint64)nPayloadSize)) {
            return false;
        }

        if (dmgChecksumIsCRC32(rawKoly.dataChecksum)) {
            if ((rawKoly.nDataForkOffset > (quint64)(std::numeric_limits<qint64>::max)()) || (rawKoly.nDataForkLength > (quint64)(std::numeric_limits<qint64>::max)())) {
                return false;
            }
            const qint64 nDataOffset = nOffset + (qint64)rawKoly.nDataForkOffset;
            quint32 nDataCRC = 0;
            bool bHaveCRC = false;
            if (pDataCRCs) {
                const auto it = pDataCRCs->constFind(nOffset);
                if (it != pDataCRCs->constEnd()) {
                    nDataCRC = it.value();
                    bHaveCRC = true;
                }
            } else {
                bHaveCRC = crcRange(nDataOffset, (qint64)rawKoly.nDataForkLength, &nDataCRC);
                if (!searchIsAlive()) return false;
            }
            if (!bHaveCRC || (nDataCRC != rawKoly.dataChecksum[2])) {
                return false;
            }
        }

        SubDevice subdevice(guardedSearchDevice.data(), nOffset, nLength);
        if (!searchIsAlive()) return false;
        const bool bSubdeviceOpen = subdevice.open(QIODevice::ReadOnly);
        if (!searchIsAlive() || !bSubdeviceOpen) return false;
        XDMG image(&subdevice);
        KOLY_BLOCK exactKoly = {};
        qint64 nSelectedKolyOffset = -1;
        const bool bValid = image._loadKolyAndXml(&exactKoly, nullptr, false, pPdStruct, &nSelectedKolyOffset, nullptr, false, false);
        if (!searchIsAlive()) return false;
        const qint64 nExpectedKolyOffset = bFrontKoly ? 0 : (nLength - 512);
        const bool bExactValid = bValid && (nSelectedKolyOffset == nExpectedKolyOffset) && XBinary::isPdStructNotCanceled(pPdStruct);
        if (!bExactValid) {
            return false;
        }

        subdevice.close();
        if (!searchIsAlive()) return false;

        FILEFORMATINFO formatInfo = {};
        formatInfo.bIsValid = true;
        formatInfo.fileType = FT_DMG;
        formatInfo.sExt = QStringLiteral("dmg");
        formatInfo.sVersion = QString::number(exactKoly.nVersion);

        pInfo->bIsValid = true;
        pInfo->fileTYPE = FT_DMG;
        pInfo->nOffset = nOffset;
        pInfo->nSize = nLength;
        pInfo->sExt = QStringLiteral("dmg");
        pInfo->sString = XBinary::getFileFormatString(&formatInfo);
        return true;
    };

    static const QByteArray baKolySignature("koly\x00\x00\x00\x04\x00\x00\x02\x00", 12);
    static const QByteArray baXmlPrefix("<?xml version");
    QList<qint64> listXmlDeclarations;
    qint64 nXmlScanCursor = nCallStart;
    qint64 nLastXmlDeclaration = nCallStart - 1;
    QByteArray baXmlScanCarry;
    const auto scanXmlDeclarationsTo = [&](qint64 nEndOffset) -> bool {
        if (!searchIsAlive()) return false;
        if (nEndOffset <= nXmlScanCursor) return true;

        // Index raw declaration offsets in a single forward pass.  Fixed-size
        // head probes both amplified dense marker noise and imposed a semantic
        // prolog-length limit.  Each KOLY below supplies the exact XML extent;
        // indexed CRC validation and the authoritative parser decide which
        // declaration is genuine.  The explicit candidate cap still bounds
        // retained adversarial state.
        const qint32 nTokenOverlap = baXmlPrefix.size() - 1;
        QByteArray baChunk(0x10000, 0);
        while ((nXmlScanCursor < nEndOffset) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            const qint32 nReadSize = (qint32)qMin<qint64>(baChunk.size(), nEndOffset - nXmlScanCursor);
            if (searchBinary.read_array_process(nXmlScanCursor, baChunk.data(), nReadSize, pPdStruct) != nReadSize) {
                return false;
            }
            if (!searchIsAlive()) return false;

            QByteArray baTokens = baXmlScanCarry;
            baTokens.append(baChunk.constData(), nReadSize);
            const qint64 nTokensOffset = nXmlScanCursor - baXmlScanCarry.size();
            for (qint32 nFound = baTokens.indexOf(baXmlPrefix); nFound >= 0;) {
                const qint64 nDeclaration = nTokensOffset + nFound;
                if (nDeclaration > nLastXmlDeclaration) {
                    if (listXmlDeclarations.size() >= DMG_MAX_XML_BASE_CANDIDATES) {
                        return false;
                    }
                    listXmlDeclarations.append(nDeclaration);
                    nLastXmlDeclaration = nDeclaration;
                }
                if (nFound == (std::numeric_limits<qint32>::max)()) return false;
                nFound = baTokens.indexOf(baXmlPrefix, nFound + 1);
            }

            if (baTokens.size() > nTokenOverlap) {
                baXmlScanCarry = baTokens.right(nTokenOverlap);
            } else {
                baXmlScanCarry = baTokens;
            }
            nXmlScanCursor += nReadSize;
        }

        return XBinary::isPdStructNotCanceled(pPdStruct);
    };

    qint64 nSignatureCursor = nCallStart;
    while (searchIsAlive() && XBinary::isPdStructNotCanceled(pPdStruct) && (nSignatureCursor <= nRangeEnd - 12)) {
        const qint64 nHeaderOffset = searchBinary.find_byteArray(nSignatureCursor, nRangeEnd - nSignatureCursor, baKolySignature, pPdStruct);
        if (!searchIsAlive()) return FFSEARCH_INFO{};
        if (nHeaderOffset < 0) break;

        // Twelve signature bytes are not enough to admit a candidate: every
        // subsequent structural field must also lie inside the search range.
        if (nHeaderOffset > nRangeEnd - 512) break;

        const KOLY_BLOCK rawKoly = readKolyBlock(guardedSearchDevice.data(), nHeaderOffset);
        if (!searchIsAlive()) return FFSEARCH_INFO{};
        if ((rawKoly.nMagic != 0x6b6f6c79) || (rawKoly.nVersion != 4) || (rawKoly.nHeaderLength != 512) || (rawKoly.nSegment > 1) || (rawKoly.nSegmentCount > 1) ||
            (rawKoly.nXmlLength > DMG_MAX_XML_SIZE) || (rawKoly.nSectorCount > ((quint64)(std::numeric_limits<qint64>::max)() / DMG_SECTOR_SIZE)) ||
            !dmgChecksumDescriptorValid(rawKoly.dataChecksum) || !dmgChecksumDescriptorValid(rawKoly.masterChecksum)) {
            if (nHeaderOffset == (std::numeric_limits<qint64>::max)()) break;
            nSignatureCursor = nHeaderOffset + 1;
            continue;
        }

        // A front-header image has no terminal KOLY.  Its declared fork top is
        // the exact searchable size, so carrier bytes after it are not swallowed.
        if (rawKoly.nDataForkOffset == 512) {
            const quint64 nAvailable = (quint64)(nRangeEnd - nHeaderOffset);
            quint64 nRequiredTop = 512;
            bool bFrontRangesValid = true;
            const auto accountRequired = [&](quint64 nOffset, quint64 nLength) {
                if (nLength == 0) return;
                if (!dmgRangeWithin(nOffset, nLength, nAvailable)) {
                    bFrontRangesValid = false;
                } else {
                    nRequiredTop = qMax(nRequiredTop, nOffset + nLength);
                }
            };
            accountRequired(rawKoly.nDataForkOffset, rawKoly.nDataForkLength);
            accountRequired(rawKoly.nResourceForkOffset, rawKoly.nResourceForkLength);
            accountRequired(rawKoly.nXmlOffset, rawKoly.nXmlLength);

            if (bFrontRangesValid) {
                // A declared code-signature extent that starts with another
                // structural KOLY makes the front candidate ambiguous.  Do not
                // accept it as a shortened front-header image; the embedded
                // terminal candidate must validate independently below.
                bool bFrontMetadataAmbiguous = false;
                if ((rawKoly.nCodeSignatureLength == 512) && dmgRangeWithin(rawKoly.nCodeSignatureOffset, 512, nAvailable) &&
                    (rawKoly.nCodeSignatureOffset <= (quint64)(std::numeric_limits<qint64>::max)())) {
                    const qint64 nNestedKolyOffset = nHeaderOffset + (qint64)rawKoly.nCodeSignatureOffset;
                    const KOLY_BLOCK nestedKoly = readKolyBlock(guardedSearchDevice.data(), nNestedKolyOffset);
                    if (!searchIsAlive()) return FFSEARCH_INFO{};
                    bFrontMetadataAmbiguous = (nestedKoly.nMagic == 0x6b6f6c79) && (nestedKoly.nVersion == 4) && (nestedKoly.nHeaderLength == 512);
                }
                QList<quint64> listTops;
                if ((rawKoly.nCodeSignatureLength != 0) && dmgRangeWithin(rawKoly.nCodeSignatureOffset, rawKoly.nCodeSignatureLength, nAvailable)) {
                    listTops.append(qMax(nRequiredTop, rawKoly.nCodeSignatureOffset + rawKoly.nCodeSignatureLength));
                }
                if (!listTops.contains(nRequiredTop)) listTops.append(nRequiredTop);

                for (quint64 nTop : listTops) {
                    if (bFrontMetadataAmbiguous) break;
                    if ((nTop > 512) && (nTop <= (quint64)(std::numeric_limits<qint64>::max)()) &&
                        tryExactImage(nHeaderOffset, (qint64)nTop, rawKoly, true, nullptr, &result)) {
                        if (!searchIsAlive()) return FFSEARCH_INFO{};
                        pState->nCurrentOffset = nHeaderOffset + (qint64)nTop;
                        return result;
                    }
                    if (!searchIsAlive()) return FFSEARCH_INFO{};
                    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return FFSEARCH_INFO{};
                }
            }
        }

        // A terminal KOLY identifies the end, not the beginning, of an image.
        // Declaration discovery is one-pass and the exact data CRC uses the
        // shared prefix index above before authoritative metadata parsing.
        if (nHeaderOffset <= nRangeEnd - 512) {
            const qint64 nImageEnd = nHeaderOffset + 512;
            if (!scanXmlDeclarationsTo(nHeaderOffset)) return FFSEARCH_INFO{};

            bool bRequiredTopValid = true;
            quint64 nRequiredTop = 0;
            const auto accountRelativeRange = [&](quint64 nOffset, quint64 nLength) {
                if (nLength == 0) return;
                if (nOffset > (std::numeric_limits<quint64>::max)() - nLength) {
                    bRequiredTopValid = false;
                } else {
                    nRequiredTop = qMax(nRequiredTop, nOffset + nLength);
                }
            };
            accountRelativeRange(rawKoly.nDataForkOffset, rawKoly.nDataForkLength);
            accountRelativeRange(rawKoly.nResourceForkOffset, rawKoly.nResourceForkLength);
            accountRelativeRange(rawKoly.nXmlOffset, rawKoly.nXmlLength);

            QList<qint64> listBases;
            QSet<qint64> setBases;
            const auto addBase = [&](qint64 nBase) {
                if ((nBase < nCallStart) || (nBase > nHeaderOffset) || setBases.contains(nBase)) {
                    return;
                }
                const quint64 nRelativeSize = (quint64)(nHeaderOffset - nBase);
                const auto pairFits = [&](quint64 nOffset, quint64 nLength) { return (nLength == 0) || dmgRangeWithin(nOffset, nLength, nRelativeSize); };
                if (pairFits(rawKoly.nDataForkOffset, rawKoly.nDataForkLength) && pairFits(rawKoly.nResourceForkOffset, rawKoly.nResourceForkLength) &&
                    pairFits(rawKoly.nXmlOffset, rawKoly.nXmlLength)) {
                    setBases.insert(nBase);
                    listBases.append(nBase);
                }
            };

            if (bRequiredTopValid) {
                const auto addXmlMarkerBase = [&](qint64 nXmlDeclaration) {
                    if ((nXmlDeclaration >= nCallStart) && (nXmlDeclaration <= nHeaderOffset) && (rawKoly.nXmlLength >= (quint64)baXmlPrefix.size()) &&
                        (rawKoly.nXmlLength <= (quint64)(nHeaderOffset - nXmlDeclaration)) && ((quint64)(nXmlDeclaration - nCallStart) >= rawKoly.nXmlOffset)) {
                        const quint64 nBaseRelative = (quint64)(nXmlDeclaration - nCallStart) - rawKoly.nXmlOffset;
                        if (nBaseRelative <= (quint64)(nHeaderOffset - nCallStart)) {
                            addBase(nCallStart + (qint64)nBaseRelative);
                        }
                    }
                };

                // Nearest declarations are most likely to describe a terminal
                // image, but every exact-range candidate is retained up to the
                // explicit fail-closed resource cap.
                for (auto it = listXmlDeclarations.crbegin(); it != listXmlDeclarations.crend(); ++it) {
                    addXmlMarkerBase(*it);
                    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                        return FFSEARCH_INFO{};
                    }
                }

                const quint64 nAvailable = (quint64)(nHeaderOffset - nCallStart);
                if ((rawKoly.nCodeSignatureLength != 0) && (rawKoly.nCodeSignatureOffset <= (std::numeric_limits<quint64>::max)() - rawKoly.nCodeSignatureLength)) {
                    const quint64 nOptionalTop = qMax(nRequiredTop, rawKoly.nCodeSignatureOffset + rawKoly.nCodeSignatureLength);
                    if (nOptionalTop <= nAvailable) {
                        addBase(nHeaderOffset - (qint64)nOptionalTop);
                    }
                }
                if (nRequiredTop <= nAvailable) {
                    addBase(nHeaderOffset - (qint64)nRequiredTop);
                }
                addBase(nCallStart);
            }

            QMap<qint64, quint32> mapCandidateDataCRCs;
            const QMap<qint64, quint32> *pCandidateDataCRCs = nullptr;
            if (dmgChecksumIsCRC32(rawKoly.dataChecksum) && !listBases.isEmpty()) {
                if (!crcRangesForBases(listBases, rawKoly.nDataForkOffset, rawKoly.nDataForkLength, &mapCandidateDataCRCs)) {
                    if (!searchIsAlive()) return FFSEARCH_INFO{};
                    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                        return FFSEARCH_INFO{};
                    }
                    mapCandidateDataCRCs.clear();
                }
                pCandidateDataCRCs = &mapCandidateDataCRCs;
            }

            for (qint64 nImageOffset : listBases) {
                if (tryExactImage(nImageOffset, nImageEnd - nImageOffset, rawKoly, false, pCandidateDataCRCs, &result)) {
                    if (!searchIsAlive()) return FFSEARCH_INFO{};
                    pState->nCurrentOffset = nImageEnd;
                    return result;
                }
                if (!searchIsAlive()) return FFSEARCH_INFO{};
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                    return FFSEARCH_INFO{};
                }
            }
        }

        if (nHeaderOffset == (std::numeric_limits<qint64>::max)()) break;
        nSignatureCursor = nHeaderOffset + 1;
    }

    if (searchIsAlive() && XBinary::isPdStructNotCanceled(pPdStruct)) {
        pState->nCurrentOffset = nRangeEnd;
    }
    return FFSEARCH_INFO{};
}

XBinary *XDMG::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XDMG(pDevice);
}

bool XDMG::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
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

void *XDMG::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XDMG> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XDMG::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
