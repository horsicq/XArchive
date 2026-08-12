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

QString dmgFromMacRoman(const char *pData, qint32 nSize)
{
    static const ushort anMacRomanHigh[128] = {
        0x00C4, 0x00C5, 0x00C7, 0x00C9, 0x00D1, 0x00D6, 0x00DC, 0x00E1,
        0x00E0, 0x00E2, 0x00E4, 0x00E3, 0x00E5, 0x00E7, 0x00E9, 0x00E8,
        0x00EA, 0x00EB, 0x00ED, 0x00EC, 0x00EE, 0x00EF, 0x00F1, 0x00F3,
        0x00F2, 0x00F4, 0x00F6, 0x00F5, 0x00FA, 0x00F9, 0x00FB, 0x00FC,
        0x2020, 0x00B0, 0x00A2, 0x00A3, 0x00A7, 0x2022, 0x00B6, 0x00DF,
        0x00AE, 0x00A9, 0x2122, 0x00B4, 0x00A8, 0x2260, 0x00C6, 0x00D8,
        0x221E, 0x00B1, 0x2264, 0x2265, 0x00A5, 0x00B5, 0x2202, 0x2211,
        0x220F, 0x03C0, 0x222B, 0x00AA, 0x00BA, 0x03A9, 0x00E6, 0x00F8,
        0x00BF, 0x00A1, 0x00AC, 0x221A, 0x0192, 0x2248, 0x2206, 0x00AB,
        0x00BB, 0x2026, 0x00A0, 0x00C0, 0x00C3, 0x00D5, 0x0152, 0x0153,
        0x2013, 0x2014, 0x201C, 0x201D, 0x2018, 0x2019, 0x00F7, 0x25CA,
        0x00FF, 0x0178, 0x2044, 0x20AC, 0x2039, 0x203A, 0xFB01, 0xFB02,
        0x2021, 0x00B7, 0x201A, 0x201E, 0x2030, 0x00C2, 0x00CA, 0x00C1,
        0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF, 0x00CC, 0x00D3, 0x00D4,
        0xF8FF, 0x00D2, 0x00DA, 0x00DB, 0x00D9, 0x0131, 0x02C6, 0x02DC,
        0x00AF, 0x02D8, 0x02D9, 0x02DA, 0x00B8, 0x02DD, 0x02DB, 0x02C7
    };

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

bool dmgChecksumDescriptorValid(const quint32 *pChecksum)
{
    return pChecksum && (pChecksum[1] <= 1024);
}

bool dmgChecksumIsCRC32(const quint32 *pChecksum)
{
    return dmgChecksumDescriptorValid(pChecksum) && (pChecksum[0] == 2) && (pChecksum[1] == 32);
}

bool dmgHasFullPartitionCRC(const QList<XDMG::BLOCK_DATA> &listStripes)
{
    for (qint32 i = 0; i < listStripes.size(); i++) {
        if (listStripes.at(i).nType == XDMG::DMG_STRIPE_ZEROES) return false;
    }
    return true;
}

QString dmgSanitizePartitionName(QString sName, qint32 nIndex, QSet<QString> *pUsedNames)
{
    const auto truncatePortable = [](QString sValue, qint32 nMaxUnits,
                                     qint32 nMaxUtf8Bytes) -> QString {
        while ((sValue.size() > nMaxUnits) ||
               (sValue.toUtf8().size() > nMaxUtf8Bytes)) {
            if (sValue.isEmpty()) break;
            const qint32 nLast = sValue.size() - 1;
            if (sValue.at(nLast).isLowSurrogate() && (nLast > 0) &&
                sValue.at(nLast - 1).isHighSurrogate()) {
                sValue.chop(2);
            } else {
                sValue.chop(1);
            }
        }
        return sValue;
    };

    sName = sName.normalized(QString::NormalizationForm_C).trimmed();
    for (qint32 i = 0; i < sName.size(); i++) {
        const QChar cValue = sName.at(i);
        if ((cValue.unicode() < 0x20) || (cValue.unicode() == 0x7F) ||
            QStringLiteral("<>:\"/\\|?*").contains(cValue)) {
            sName[i] = QLatin1Char('_');
        }
    }
    while (sName.endsWith(QLatin1Char(' ')) || sName.endsWith(QLatin1Char('.'))) sName.chop(1);

    if ((sName == QLatin1String(".")) || (sName == QLatin1String(".."))) sName.clear();
    if (sName.isEmpty()) sName = QStringLiteral("partition%1").arg(nIndex);

    const qint32 nDot = sName.indexOf(QLatin1Char('.'));
    const QString sStem = (nDot < 0) ? sName : sName.left(nDot);
    const QString sUpperStem = sStem.toUpper();
    bool bReservedDevice = (sUpperStem == QLatin1String("CON")) ||
                                  (sUpperStem == QLatin1String("PRN")) ||
                                  (sUpperStem == QLatin1String("AUX")) ||
                                  (sUpperStem == QLatin1String("NUL")) ||
                                  (sUpperStem == QLatin1String("CLOCK$")) ||
                                  (sUpperStem == QLatin1String("CONIN$")) ||
                                  (sUpperStem == QLatin1String("CONOUT$"));
    if (!bReservedDevice && (sUpperStem.size() == 4) &&
        (sUpperStem.startsWith(QLatin1String("COM")) ||
         sUpperStem.startsWith(QLatin1String("LPT")))) {
        const QChar cIndex = sUpperStem.at(3);
        bReservedDevice = ((cIndex >= QLatin1Char('1')) && (cIndex <= QLatin1Char('9'))) ||
                          (cIndex.unicode() == 0x00B9) ||
                          (cIndex.unicode() == 0x00B2) ||
                          (cIndex.unicode() == 0x00B3);
    }
    if (bReservedDevice) sName.prepend(QLatin1Char('_'));

    // Reserve four units/bytes for the public ".img" suffix and keep every
    // filesystem component within both UTF-16 and UTF-8 portable limits.
    const qint32 nMaxNameLength = 240;
    const qint32 nMaxNameUtf8Bytes = 251;
    sName = truncatePortable(sName, nMaxNameLength, nMaxNameUtf8Bytes);
    while (sName.endsWith(QLatin1Char(' ')) || sName.endsWith(QLatin1Char('.'))) sName.chop(1);
    if (sName.isEmpty()) sName = QStringLiteral("partition%1").arg(nIndex);

    if (pUsedNames) {
        const QString sBaseName = sName;
        qint32 nSuffix = 2;
        while (pUsedNames->contains(sName.toCaseFolded())) {
            const QString sSuffix = QStringLiteral("_%1").arg(nSuffix++);
            qint32 nBaseLength = nMaxNameLength - sSuffix.size();
            if (nBaseLength < 1) nBaseLength = 1;
            const qint32 nBaseBytes = nMaxNameUtf8Bytes - sSuffix.toUtf8().size();
            QString sCandidateBase = truncatePortable(sBaseName, nBaseLength,
                                                       qMax(1, nBaseBytes));
            sName = sCandidateBase + sSuffix;
        }
        pUsedNames->insert(sName.toCaseFolded());
    }

    return sName;
}

bool dmgCalculateCRC32(XBinary *pBinary, qint64 nOffset, qint64 nSize,
                       XBinary::PDSTRUCT *pPdStruct, quint32 *pCRC32)
{
    if (pCRC32) *pCRC32 = 0;
    if (!pBinary || !pCRC32 || (nOffset < 0) || (nSize < 0) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QByteArray baBuffer(0x10000, 0);
    quint32 nCRC = 0xFFFFFFFFU;
    qint64 nDone = 0;
    while ((nDone < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nChunk = (qint32)qMin<qint64>(baBuffer.size(), nSize - nDone);
        if (pBinary->read_array_process(nOffset + nDone, baBuffer.data(), nChunk, pPdStruct) != nChunk) {
            return false;
        }
        nCRC = XBinary::_getCRC32(baBuffer.constData(), nChunk, nCRC,
                                  XBinary::_getCRC32Table_EDB88320());
        nDone += nChunk;
    }

    if ((nDone != nSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    *pCRC32 = nCRC ^ 0xFFFFFFFFU;
    return true;
}

bool dmgWriteAll(QIODevice *pDevice, const char *pData, qint64 nSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDevice || (nSize < 0) || ((nSize > 0) && !pData)) return false;

    qint64 nDone = 0;
    while ((nDone < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nWritten = pDevice->write(pData + nDone, nSize - nDone);
        if ((nWritten <= 0) || (nWritten > (nSize - nDone))) return false;
        nDone += nWritten;
    }
    return (nDone == nSize) && XBinary::isPdStructNotCanceled(pPdStruct);
}

void dmgRollbackWrite(QIODevice *pDevice, qint64 nStartPosition)
{
    if (pDevice && !pDevice->isSequential() && (nStartPosition >= 0) &&
        XBinary::isResizeEnable(pDevice)) {
        XBinary::resize(pDevice, nStartPosition);
        pDevice->seek(nStartPosition);
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
    if (pSource == pDestination) return true;

    QBuffer *pSourceBuffer = dynamic_cast<QBuffer *>(pSource);
    QBuffer *pDestinationBuffer = dynamic_cast<QBuffer *>(pDestination);
    if (pSourceBuffer && pDestinationBuffer &&
        (&pSourceBuffer->buffer() == &pDestinationBuffer->buffer())) {
        return true;
    }

    QFile *pSourceFile = dynamic_cast<QFile *>(pSource);
    QFile *pDestinationFile = dynamic_cast<QFile *>(pDestination);
    if (!pSourceFile || !pDestinationFile) return false;

    const QFileInfo sourceInfo(pSourceFile->fileName());
    const QFileInfo destinationInfo(pDestinationFile->fileName());
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
        (QString::compare(QDir::fromNativeSeparators(sSourcePath),
                          QDir::fromNativeSeparators(sDestinationPath), caseSensitivity) == 0)) {
        return true;
    }

    if ((pSourceFile->handle() < 0) || (pDestinationFile->handle() < 0)) return false;
#ifdef Q_OS_WIN
    const intptr_t nSourceHandle = _get_osfhandle(pSourceFile->handle());
    const intptr_t nDestinationHandle = _get_osfhandle(pDestinationFile->handle());
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
    return (fstat(pSourceFile->handle(), &sourceStatus) == 0) &&
           (fstat(pDestinationFile->handle(), &destinationStatus) == 0) &&
           (sourceStatus.st_dev == destinationStatus.st_dev) &&
           (sourceStatus.st_ino == destinationStatus.st_ino);
#else
    return false;
#endif
}

bool dmgGetPartitionStorageInfo(const QList<XDMG::BLOCK_DATA> &listStripes,
                                qint64 *pCompressedSize,
                                XBinary::HANDLE_METHOD *pMethod)
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
            case XDMG::DMG_STRIPE_END:
                break;

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

            default:
                return false;
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
    *pMethod = !bPhysicalDataSeen
                   ? XBinary::HANDLE_METHOD_STORE
                   : ((bMethodIsKnown && bDataMethodSeen) ? method : XBinary::HANDLE_METHOD_UNKNOWN);
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
        const bool bAlphabet = ((cValue >= 'A') && (cValue <= 'Z')) ||
                               ((cValue >= 'a') && (cValue <= 'z')) ||
                               ((cValue >= '0') && (cValue <= '9')) || (cValue == '+') || (cValue == '/');
        if (cValue == '=') {
            nPadding++;
            if ((nPadding > 2) || (i < (baEncoded.size() - 2))) return false;
        } else if (!bAlphabet || (nPadding != 0)) {
            return false;
        }
    }

    const quint64 nDecodedUpperBound = ((quint64)baEncoded.size() / 4ULL) * 3ULL;
    if ((nDecodedUpperBound < (quint64)nPadding) ||
        ((nDecodedUpperBound - (quint64)nPadding) > DMG_MAX_MISH_SIZE)) {
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

bool dmgSkipCurrentElementBounded(QXmlStreamReader *pReader, qint32 nCurrentDepth,
                                  XBinary::PDSTRUCT *pPdStruct)
{
    if (!pReader || (pReader->tokenType() != QXmlStreamReader::StartElement) ||
        (nCurrentDepth <= 0) || (nCurrentDepth > DMG_MAX_XML_DEPTH)) {
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
        } else if ((token == QXmlStreamReader::DTD) ||
                   (token == QXmlStreamReader::EntityReference) ||
                   (token == QXmlStreamReader::Invalid)) {
            return false;
        }
    }

    return false;
}
}

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
    KOLY_BLOCK kolyBlock = {};
    QList<DMG_PARTITION_INFO> listPartitions;
    QList<MISH_BLOCK> listMishBlocks;
    return _loadPartitionMetadata(&kolyBlock, &listPartitions, pPdStruct) &&
           _parseAllPartitions(listPartitions, kolyBlock, &listMishBlocks, nullptr, pPdStruct) &&
           !listMishBlocks.isEmpty();
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
    return isValid(pPdStruct) ? getSize() : 0;
}

QString XDMG::getVersion()
{
    QString sResult;

    KOLY_BLOCK kolyBlock = {};
    if (_loadKolyAndXml(&kolyBlock, nullptr, false, nullptr)) {
        sResult = QString::number(kolyBlock.nVersion);
    }

    return sResult;
}

bool XDMG::_loadKolyAndXml(KOLY_BLOCK *pKolyBlock, QByteArray *pXmlData, bool bRequireXml,
                           PDSTRUCT *pPdStruct, qint64 *pKolyOffset)
{
    if (pXmlData) pXmlData->clear();
    if (pKolyOffset) *pKolyOffset = -1;
    if (!pKolyBlock || !getDevice() || !getDevice()->isOpen() || !getDevice()->isReadable() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint64 nSize = getSize();
    if (nSize < 512) return false;

    const auto tryCandidate = [&](qint64 nKolyOffset, bool bFrontKoly,
                                  KOLY_BLOCK *pCandidate, QByteArray *pCandidateXml) -> bool {
        const KOLY_BLOCK kolyBlock = readKolyBlock(getDevice(), nKolyOffset);
        if (bFrontKoly && ((nSize <= 512) || (kolyBlock.nDataForkOffset != 512))) return false;

        const quint64 nPayloadLimit = bFrontKoly ? (quint64)nSize : (quint64)nKolyOffset;
        if ((kolyBlock.nMagic != 0x6b6f6c79) || (kolyBlock.nVersion != 4) ||
            (kolyBlock.nHeaderLength != 512) ||
            (kolyBlock.nSegment > 1) || (kolyBlock.nSegmentCount > 1) ||
            !dmgRangeWithin(kolyBlock.nDataForkOffset, kolyBlock.nDataForkLength, nPayloadLimit) ||
            !dmgRangeWithin(kolyBlock.nResourceForkOffset, kolyBlock.nResourceForkLength, nPayloadLimit) ||
            !dmgRangeWithin(kolyBlock.nXmlOffset, kolyBlock.nXmlLength, nPayloadLimit) ||
            (kolyBlock.nXmlLength > DMG_MAX_XML_SIZE) ||
            (kolyBlock.nSectorCount > ((quint64)(std::numeric_limits<qint64>::max)() / DMG_SECTOR_SIZE)) ||
            (bRequireXml && (kolyBlock.nXmlLength == 0)) ||
            !dmgChecksumDescriptorValid(kolyBlock.dataChecksum) ||
            !dmgChecksumDescriptorValid(kolyBlock.masterChecksum)) {
            return false;
        }

        if (dmgChecksumIsCRC32(kolyBlock.dataChecksum)) {
            quint32 nDataForkCRC = 0;
            if (!dmgCalculateCRC32(this, (qint64)kolyBlock.nDataForkOffset,
                                   (qint64)kolyBlock.nDataForkLength, pPdStruct, &nDataForkCRC) ||
                (nDataForkCRC != kolyBlock.dataChecksum[2])) return false;
        }

        QByteArray baXml;
        if (pXmlData && (kolyBlock.nXmlLength > 0)) {
            if (kolyBlock.nXmlLength > (quint64)(std::numeric_limits<qint32>::max)()) return false;
            baXml.resize((qint32)kolyBlock.nXmlLength);
            if (read_array_process((qint64)kolyBlock.nXmlOffset, baXml.data(), baXml.size(), pPdStruct) !=
                    baXml.size() ||
                !XBinary::isPdStructNotCanceled(pPdStruct)) {
                return false;
            }
        }

        *pCandidate = kolyBlock;
        if (pCandidateXml) *pCandidateXml = baXml;
        return XBinary::isPdStructNotCanceled(pPdStruct);
    };

    const qint64 nTrailerOffset = nSize - 512;
    KOLY_BLOCK kolyBlock = {};
    QByteArray baXml;
    qint64 nSelectedOffset = nTrailerOffset;

    // Once the terminal bytes identify a structural KOLY trailer, it is
    // authoritative.  In particular, an integrity/range failure must not be
    // downgraded to a separately crafted front header.
    const KOLY_BLOCK terminalKolyBlock = readKolyBlock(getDevice(), nTrailerOffset);
    const bool bTerminalKolyPresent = (terminalKolyBlock.nMagic == 0x6b6f6c79) &&
                                      (terminalKolyBlock.nVersion == 4) &&
                                      (terminalKolyBlock.nHeaderLength == 512);
    if (bTerminalKolyPresent) {
        if (!tryCandidate(nTrailerOffset, false, &kolyBlock, &baXml)) return false;
    } else {
        nSelectedOffset = 0;
        if ((nTrailerOffset == 0) || !tryCandidate(0, true, &kolyBlock, &baXml)) return false;
    }

    *pKolyBlock = kolyBlock;
    if (pXmlData) *pXmlData = baXml;
    if (pKolyOffset) *pKolyOffset = nSelectedOffset;
    return true;
}

bool XDMG::_loadPartitionMetadata(KOLY_BLOCK *pKolyBlock,
                                  QList<DMG_PARTITION_INFO> *pPartitions,
                                  PDSTRUCT *pPdStruct)
{
    if (pPartitions) pPartitions->clear();
    if (!pKolyBlock || !pPartitions || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QByteArray baXml;
    if (!_loadKolyAndXml(pKolyBlock, &baXml, false, pPdStruct)) return false;

    if (pKolyBlock->nXmlLength != 0) {
        *pPartitions = _parseBlkxPartitions(baXml, pPdStruct);
    } else {
        if ((pKolyBlock->nResourceForkLength < 0x100) ||
            (pKolyBlock->nResourceForkLength > DMG_MAX_RESOURCE_SIZE) ||
            (pKolyBlock->nResourceForkLength > (quint64)(std::numeric_limits<qint32>::max)())) {
            return false;
        }
        QByteArray baResource((qint32)pKolyBlock->nResourceForkLength, 0);
        if (read_array_process((qint64)pKolyBlock->nResourceForkOffset, baResource.data(),
                               baResource.size(), pPdStruct) != baResource.size() ||
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        *pPartitions = _parseResourceForkPartitions(baResource, pPdStruct);
    }

    if (pPartitions->isEmpty() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        pPartitions->clear();
        return false;
    }

    QSet<QString> stUsedNames;
    for (qint32 i = 0; i < pPartitions->size(); i++) {
        (*pPartitions)[i].sName = dmgSanitizePartitionName((*pPartitions)[i].sName, i,
                                                          &stUsedNames);
    }
    return true;
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
    QList<XBinary::XFHEADER> listResult;
    const auto getKolyOffset = [&]() -> qint64 {
        KOLY_BLOCK kolyBlock = {};
        qint64 nKolyOffset = -1;
        if (!_loadKolyAndXml(&kolyBlock, nullptr, false, pPdStruct, &nKolyOffset)) return -1;
        return nKolyOffset;
    };

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        qint64 nKolyOffset = getKolyOffset();

        if (nKolyOffset >= 0) {
            XFSTRUCT _xfStruct = xfStruct;
            _xfStruct.nStructID = STRUCTID_KOLY_BLOCK;
            _xfStruct.xLoc = offsetToLoc(nKolyOffset);
            listResult.append(getXFHeaders(_xfStruct, pPdStruct));
        }
    } else if (nStructID == STRUCTID_KOLY_BLOCK) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            qint64 nKolyOffset = getKolyOffset();

            if (nKolyOffset < 0) {
                return listResult;
            }

            headerLoc = offsetToLoc(nKolyOffset);
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

QList<XBinary::FPART> XDMG::getFileParts(quint32 nFileParts, qint32 nLimit,
                                         PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    if ((nLimit < -1) || (nLimit == 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    const auto canAppend = [&]() -> bool {
        return XBinary::isPdStructNotCanceled(pPdStruct) &&
               ((nLimit == -1) || (listResult.size() < nLimit));
    };
    const auto appendPart = [&](FILEPART filePart, qint64 nOffset, qint64 nSize,
                                const QString &sName, qint64 nUncompressedSize = -1,
                                HANDLE_METHOD handleMethod = HANDLE_METHOD_UNKNOWN) {
        if (!canAppend() || (nOffset < 0) || (nSize <= 0) ||
            (nOffset > getSize() - nSize)) return;
        FPART part = getFPART(filePart, sName, nOffset, nSize, -1, 0);
        if (nUncompressedSize >= 0) {
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, handleMethod);
        }
        listResult.append(part);
    };

    KOLY_BLOCK kolyBlock = {};
    qint64 nKolyOffset = -1;
    if (!_loadKolyAndXml(&kolyBlock, nullptr, false, pPdStruct, &nKolyOffset)) {
        return listResult;
    }

    qint64 nKnownEnd = 0;
    const auto accountRange = [&](quint64 nOffset, quint64 nSize) {
        const quint64 nMaximum = (quint64)(std::numeric_limits<qint64>::max)();
        if ((nSize != 0) && (nOffset <= nMaximum) &&
            (nSize <= (nMaximum - nOffset))) {
            nKnownEnd = qMax(nKnownEnd, (qint64)(nOffset + nSize));
        }
    };
    accountRange((quint64)nKolyOffset, 512);
    accountRange(kolyBlock.nDataForkOffset, kolyBlock.nDataForkLength);
    accountRange(kolyBlock.nResourceForkOffset, kolyBlock.nResourceForkLength);
    accountRange(kolyBlock.nXmlOffset, kolyBlock.nXmlLength);

    if ((nFileParts & FILEPART_HEADER) && canAppend()) {
        appendPart(FILEPART_HEADER, nKolyOffset, 512, tr("KOLY header"));
    }
    if ((nFileParts & FILEPART_DATA) && canAppend()) {
        if (kolyBlock.nDataForkLength != 0) {
            appendPart(FILEPART_DATA, (qint64)kolyBlock.nDataForkOffset,
                       (qint64)kolyBlock.nDataForkLength, tr("Data fork"));
        }
        if ((kolyBlock.nResourceForkLength != 0) && canAppend()) {
            appendPart(FILEPART_DATA, (qint64)kolyBlock.nResourceForkOffset,
                       (qint64)kolyBlock.nResourceForkLength, tr("Resource fork"));
        }
        if ((kolyBlock.nXmlLength != 0) && canAppend()) {
            appendPart(FILEPART_DATA, (qint64)kolyBlock.nXmlOffset,
                       (qint64)kolyBlock.nXmlLength, tr("XML metadata"));
        }
    }

    if ((nFileParts & FILEPART_STREAM) && canAppend()) {
        QList<DMG_PARTITION_INFO> listPartitions;
        QList<MISH_BLOCK> listMishBlocks;
        QList<QList<BLOCK_DATA> > listStripes;
        if (!_loadPartitionMetadata(&kolyBlock, &listPartitions, pPdStruct) ||
            !_parseAllPartitions(listPartitions, kolyBlock, &listMishBlocks,
                                 &listStripes, pPdStruct)) {
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
                    case DMG_STRIPE_XZ:
                        break;

                    case DMG_STRIPE_EMPTY:
                    case DMG_STRIPE_ZEROES:
                    case DMG_STRIPE_SKIP:
                    case DMG_STRIPE_END:
                        bPhysicalStream = false;
                        break;

                    default:
                        listResult.clear();
                        return listResult;
                }
                if (!bPhysicalStream) continue;

                const quint64 nMaximum = (quint64)(std::numeric_limits<qint64>::max)();
                if ((kolyBlock.nDataForkOffset > nMaximum) ||
                    (mishBlock.nDataOffset > (nMaximum - kolyBlock.nDataForkOffset))) {
                    listResult.clear();
                    return listResult;
                }
                const quint64 nPartitionDataOffset = kolyBlock.nDataForkOffset + mishBlock.nDataOffset;
                if ((stripe.nDataOffset > (nMaximum - nPartitionDataOffset)) ||
                    (stripe.nDataLength > nMaximum) ||
                    (stripe.nSectorCount > (nMaximum / DMG_SECTOR_SIZE))) {
                    listResult.clear();
                    return listResult;
                }
                const quint64 nAbsoluteOffset = nPartitionDataOffset + stripe.nDataOffset;
                const quint64 nUncompressedSize = stripe.nSectorCount * DMG_SECTOR_SIZE;
                appendPart(FILEPART_STREAM, (qint64)nAbsoluteOffset,
                           (qint64)stripe.nDataLength,
                           tr("Partition %1 stream %2").arg(i + 1).arg(k + 1),
                           (qint64)nUncompressedSize, handleMethod);
            }
        }
    }

    if ((nFileParts & FILEPART_OVERLAY) && canAppend() && (nKnownEnd < getSize())) {
        appendPart(FILEPART_OVERLAY, nKnownEnd, getSize() - nKnownEnd, tr("Overlay"));
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) listResult.clear();
    return listResult;
}

quint64 XDMG::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    KOLY_BLOCK kolyBlock = {};
    QList<DMG_PARTITION_INFO> listPartitions;
    if (!_loadPartitionMetadata(&kolyBlock, &listPartitions, pPdStruct)) return 0;

    QList<MISH_BLOCK> listMishBlocks;
    if (!_parseAllPartitions(listPartitions, kolyBlock, &listMishBlocks, nullptr, pPdStruct)) return 0;
    return (quint64)listMishBlocks.size();
}

QList<XArchive::RECORD> XDMG::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<RECORD> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    KOLY_BLOCK kolyBlock = {};
    QList<DMG_PARTITION_INFO> listPartitions;
    if (!_loadPartitionMetadata(&kolyBlock, &listPartitions, pPdStruct)) return listResult;

    QList<MISH_BLOCK> listMishBlocks;
    QList<QList<BLOCK_DATA>> listStripes;
    if (!_parseAllPartitions(listPartitions, kolyBlock, &listMishBlocks, &listStripes, pPdStruct)) return listResult;
    for (qint32 i = 0; i < listMishBlocks.size(); i++) {
        const MISH_BLOCK mishBlock = listMishBlocks.at(i);

        if ((nLimit != -1) && (listResult.size() >= nLimit)) continue;

        RECORD record = {};
        QString sName = listPartitions.at(i).sName;
        if (sName.isEmpty()) sName = QString("partition%1").arg(i);

        qint64 nCompressedSize = 0;
        HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;
        if ((i >= listStripes.size()) ||
            !dmgGetPartitionStorageInfo(listStripes.at(i), &nCompressedSize, &compressMethod)) {
            listResult.clear();
            return listResult;
        }

        record.spInfo.sRecordName = sName + ".img";
        record.spInfo.nUncompressedSize = (qint64)(mishBlock.nSectorCount * DMG_SECTOR_SIZE);
        if (dmgChecksumIsCRC32(mishBlock.checksum) &&
            (i < listStripes.size()) && dmgHasFullPartitionCRC(listStripes.at(i))) {
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
            record.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                        CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
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
    if (!pState) return false;
    finishUnpack(pState, nullptr);
    if (!isPdStructNotCanceled(pPdStruct)) return false;

    pState->mapUnpackProperties = mapOptions;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = getSize();

    KOLY_BLOCK kolyBlock = {};
    QList<DMG_PARTITION_INFO> listPartitions;
    if (!_loadPartitionMetadata(&kolyBlock, &listPartitions, pPdStruct)) {
        finishUnpack(pState, nullptr);
        return false;
    }

    QList<MISH_BLOCK> listMishBlocks;
    QList<QList<BLOCK_DATA>> listStripes;
    QList<QString> listPartitionNames;
    if (!_parseAllPartitions(listPartitions, kolyBlock, &listMishBlocks, &listStripes, pPdStruct)) {
        finishUnpack(pState, nullptr);
        return false;
    }
    for (qint32 i = 0; i < listPartitions.size(); i++) {
        listPartitionNames.append(listPartitions.at(i).sName);
    }

    if (!isPdStructNotCanceled(pPdStruct) || listMishBlocks.isEmpty()) {
        finishUnpack(pState, nullptr);
        return false;
    }

    DMG_UNPACK_CONTEXT *pContext = new (std::nothrow) DMG_UNPACK_CONTEXT;
    if (!pContext) {
        finishUnpack(pState, nullptr);
        return false;
    }

    pContext->pSourceDevice = getDevice();
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

    pState->nNumberOfRecords = listMishBlocks.size();
    pState->nCurrentIndex = 0;
    pState->pContext = pContext;
    return true;
}

XArchive::ARCHIVERECORD XDMG::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};

    if (isPdStructNotCanceled(pPdStruct) && pState && pState->pContext) {
        DMG_UNPACK_CONTEXT *pContext = (DMG_UNPACK_CONTEXT *)pState->pContext;

        if ((pState->nCurrentIndex >= 0) && (pState->nCurrentIndex < pContext->listMishBlocks.size())) {
            QString sPartitionName;
            if (pState->nCurrentIndex < pContext->listPartitionNames.size()) {
                sPartitionName = pContext->listPartitionNames.at(pState->nCurrentIndex);
            }

            if (sPartitionName.isEmpty()) {
                sPartitionName = QString("partition%1").arg(pContext->nCurrentFileIndex);
            }

            qint64 nUncompressedSize =
                (qint64)(pContext->listMishBlocks.at(pState->nCurrentIndex).nSectorCount * DMG_SECTOR_SIZE);

            qint64 nCompressedSize = 0;
            HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;
            const bool bStorageInfoValid =
                (pState->nCurrentIndex < pContext->listStripes.size()) &&
                dmgGetPartitionStorageInfo(pContext->listStripes.at(pState->nCurrentIndex),
                                           &nCompressedSize, &compressMethod);

            result.mapProperties.insert(FPART_PROP_ORIGINALNAME, sPartitionName + ".img");
            result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
            if (bStorageInfoValid) {
                result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nCompressedSize);
                result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, compressMethod);
            }
            result.mapProperties.insert(FPART_PROP_EXT, "img");
            const MISH_BLOCK &mishBlock = pContext->listMishBlocks.at(pState->nCurrentIndex);
            if (dmgChecksumIsCRC32(mishBlock.checksum) &&
                (pState->nCurrentIndex < pContext->listStripes.size()) &&
                dmgHasFullPartitionCRC(pContext->listStripes.at(pState->nCurrentIndex))) {
                result.mapProperties.insert(FPART_PROP_RESULTCRC, mishBlock.checksum[2]);
                result.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                            CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
            }
        }

        result.nStreamOffset = 0;
        result.nStreamSize = 0;
    }

    return result;
}

bool XDMG::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    if (!pState->pContext) {
        pState->nCurrentOffset = 0;
        return false;
    }

    DMG_UNPACK_CONTEXT *pContext = (DMG_UNPACK_CONTEXT *)pState->pContext;
    // The parsed context is bound to one exact source device.  Reject a public
    // setDevice() replacement before resetting state or touching an output.
    if (getDevice() != pContext->pSourceDevice) return false;

    pState->nCurrentOffset = 0;
    if (!pDevice || !pDevice->isOpen() || !pDevice->isWritable() ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pContext->listStripes.size()) ||
        (pState->nCurrentIndex >= pContext->listMishBlocks.size())) {
        return false;
    }
    if (dmgDevicesAlias(pContext->pSourceDevice, pDevice)) return false;

    // A valid extraction attempt owns the destination from this point on.
    // Clear random-access output before staging so every later failure leaves
    // the same exact empty rollback state, even when no staged byte was ever
    // eligible for publication.
    qint64 nStartPosition = -1;
    if (!pDevice->isSequential()) {
        if (!XBinary::isResizeEnable(pDevice) || !XBinary::resize(pDevice, 0) || !pDevice->seek(0)) {
            return false;
        }
        nStartPosition = 0;
    }

    const MISH_BLOCK mishBlock = pContext->listMishBlocks.at(pState->nCurrentIndex);
    const QList<BLOCK_DATA> listCurrentStripes = pContext->listStripes.at(pState->nCurrentIndex);
    const qint64 nExpectedOutput = (qint64)(mishBlock.nSectorCount * DMG_SECTOR_SIZE);
    const bool bCheckCRC = XBinary::isUnpackCRCEnabled(
        pState->mapUnpackProperties, XBinary::CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    QTemporaryFile staging;
    if (!staging.open()) return false;
    bool bResult = true;

    for (qint32 i = 0; (i < listCurrentStripes.size()) && bResult && isPdStructNotCanceled(pPdStruct); i++) {
        bResult = _decompressStripe(listCurrentStripes.at(i), pContext->nDataForkOffset,
                                    pContext->nDataForkLength, (qint64)mishBlock.nDataOffset,
                                    &staging, pPdStruct);
    }

    bResult = bResult && isPdStructNotCanceled(pPdStruct) &&
              (staging.size() == nExpectedOutput) &&
              (!bCheckCRC || _validatePartitionCRC(&staging, mishBlock, listCurrentStripes, pPdStruct)) &&
              staging.seek(0);

    if (bResult && !pDevice->isSequential()) {
        bResult = (pDevice->size() == 0) && pDevice->seek(0);
    }

    QByteArray baBuffer(0x10000, 0);
    qint64 nRemaining = nExpectedOutput;
    while (bResult && (nRemaining > 0) && isPdStructNotCanceled(pPdStruct)) {
        const qint64 nRequest = qMin<qint64>(nRemaining, baBuffer.size());
        const qint64 nRead = staging.read(baBuffer.data(), nRequest);
        if ((nRead <= 0) || (nRead > nRequest) ||
            !dmgWriteAll(pDevice, baBuffer.constData(), nRead, pPdStruct)) {
            bResult = false;
            break;
        }
        nRemaining -= nRead;
    }
    bResult = bResult && (nRemaining == 0) && isPdStructNotCanceled(pPdStruct);

    if (!bResult) {
        dmgRollbackWrite(pDevice, nStartPosition);
        return false;
    }

    pState->nCurrentOffset = nExpectedOutput;
    return true;
}

bool XDMG::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (isPdStructNotCanceled(pPdStruct) && pState && pState->pContext && (pState->nCurrentIndex >= 0) &&
        (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        DMG_UNPACK_CONTEXT *pContext = (DMG_UNPACK_CONTEXT *)pState->pContext;

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
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if (pState->pContext) {
        DMG_UNPACK_CONTEXT *pContext = (DMG_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
    }

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

    if (pDevice) {
        XBinary binary(pDevice);
        const qint64 nSize = binary.getSize();
        if ((nOffset < 0) || (nSize < 512) || (nOffset > (nSize - 512))) return result;

        result.nMagic = binary.read_uint32(nOffset, true);
        result.nVersion = binary.read_uint32(nOffset + 4, true);
        result.nHeaderLength = binary.read_uint32(nOffset + 8, true);
        result.nFlags = binary.read_uint32(nOffset + 12, true);
        result.nRunningOffset = binary.read_uint64(nOffset + 16, true);
        result.nDataForkOffset = binary.read_uint64(nOffset + 24, true);
        result.nDataForkLength = binary.read_uint64(nOffset + 32, true);
        result.nResourceForkOffset = binary.read_uint64(nOffset + 40, true);
        result.nResourceForkLength = binary.read_uint64(nOffset + 48, true);
        result.nSegment = binary.read_uint32(nOffset + 56, true);
        result.nSegmentCount = binary.read_uint32(nOffset + 60, true);
        for (qint32 i = 0; i < 16; i++) result.segmentID[i] = binary.read_uint8(nOffset + 64 + i);

        for (qint32 i = 0; i < 34; i++) {
            result.dataChecksum[i] = binary.read_uint32(nOffset + 80 + ((qint64)i * 4), true);
            result.masterChecksum[i] = binary.read_uint32(nOffset + 352 + ((qint64)i * 4), true);
        }

        // Skip dataChecksum (136 bytes)
        result.nXmlOffset = binary.read_uint64(nOffset + 216, true);
        result.nXmlLength = binary.read_uint64(nOffset + 224, true);

        // Skip padding and masterChecksum
        result.nImageVariant = binary.read_uint32(nOffset + 488, true);
        result.nSectorCount = binary.read_uint64(nOffset + 492, true);
    }

    return result;
}

XDMG::MISH_BLOCK XDMG::readMishBlock(QIODevice *pDevice, qint64 nOffset)
{
    MISH_BLOCK result = {};

    if (pDevice) {
        XBinary binary(pDevice);
        const qint64 nSize = binary.getSize();
        if ((nOffset < 0) || (nSize < 204) || (nOffset > (nSize - 204))) return result;

        result.nMagic = binary.read_uint32(nOffset, true);
        result.nVersion = binary.read_uint32(nOffset + 4, true);
        result.nStartSector = binary.read_uint64(nOffset + 8, true);
        result.nSectorCount = binary.read_uint64(nOffset + 16, true);
        result.nDataOffset = binary.read_uint64(nOffset + 24, true);
        result.nBufferCount = binary.read_uint32(nOffset + 32, true);
        result.nDescriptorBlocks = binary.read_uint32(nOffset + 36, true);
        for (qint32 i = 0; i < 34; i++) {
            result.checksum[i] = binary.read_uint32(nOffset + 64 + ((qint64)i * 4), true);
        }
        // Skip reserved (24 bytes) and checksum (136 bytes)
        result.nBlockDataCount = binary.read_uint32(nOffset + 200, true);
    }

    return result;
}

XDMG::BLOCK_DATA XDMG::readBlockData(QIODevice *pDevice, qint64 nOffset)
{
    BLOCK_DATA result = {};

    if (pDevice) {
        XBinary binary(pDevice);
        const qint64 nSize = binary.getSize();
        if ((nOffset < 0) || (nSize < 40) || (nOffset > (nSize - 40))) return result;

        result.nType = binary.read_uint32(nOffset, true);
        result.nReserved = binary.read_uint32(nOffset + 4, true);
        result.nStartSector = binary.read_uint64(nOffset + 8, true);
        result.nSectorCount = binary.read_uint64(nOffset + 16, true);
        result.nDataOffset = binary.read_uint64(nOffset + 24, true);
        result.nDataLength = binary.read_uint64(nOffset + 32, true);
    }

    return result;
}

bool XDMG::_parsePartition(const DMG_PARTITION_INFO &partitionInfo, const KOLY_BLOCK &kolyBlock,
                           MISH_BLOCK *pMishBlock, QList<BLOCK_DATA> *pStripes, PDSTRUCT *pPdStruct)
{
    if (pStripes) pStripes->clear();
    if (!pMishBlock || !isPdStructNotCanceled(pPdStruct) ||
        (partitionInfo.mishData.size() < 204) ||
        ((quint64)partitionInfo.mishData.size() > DMG_MAX_MISH_SIZE)) {
        return false;
    }

    QByteArray baMishData = partitionInfo.mishData;
    QBuffer buffer(&baMishData);
    if (!buffer.open(QIODevice::ReadOnly)) return false;

    const MISH_BLOCK mishBlock = readMishBlock(&buffer, 0);
    if ((mishBlock.nMagic != 0x6d697368) || (mishBlock.nVersion != 1) ||
        (mishBlock.nBlockDataCount == 0) ||
        (mishBlock.nBlockDataCount > DMG_MAX_STRIPES_PER_PARTITION) ||
        (mishBlock.nSectorCount > ((quint64)(std::numeric_limits<qint64>::max)() / DMG_SECTOR_SIZE)) ||
        (mishBlock.nStartSector > kolyBlock.nSectorCount) ||
        (mishBlock.nSectorCount > (kolyBlock.nSectorCount - mishBlock.nStartSector)) ||
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
            if ((stripe.nSectorCount != 0) || (stripe.nDataOffset != 0) ||
                (stripe.nDataLength != 0) || bEndSeen) return false;
            if (stripe.nType == DMG_STRIPE_END) {
                if (i != (mishBlock.nBlockDataCount - 1)) return false;
                bEndSeen = true;
            }
        } else {
            if ((stripe.nSectorCount == 0) ||
                (stripe.nSectorCount > ((quint64)(std::numeric_limits<qint64>::max)() / DMG_SECTOR_SIZE)) ||
                (nCoveredSectors > mishBlock.nSectorCount) ||
                (stripe.nSectorCount > (mishBlock.nSectorCount - nCoveredSectors))) {
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

                default:
                    return false;
            }

            if ((stripe.nType != DMG_STRIPE_EMPTY) && (stripe.nType != DMG_STRIPE_ZEROES)) {
                if (stripe.nDataLength >
                    ((quint64)(std::numeric_limits<qint64>::max)() - nTotalPackedSize)) {
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

    if (!isPdStructNotCanceled(pPdStruct) || !bEndSeen ||
        (nCoveredSectors != mishBlock.nSectorCount)) {
        return false;
    }

    *pMishBlock = mishBlock;
    if (pStripes) *pStripes = listStripes;
    return true;
}

bool XDMG::_parseAllPartitions(const QList<DMG_PARTITION_INFO> &listPartitions,
                               const KOLY_BLOCK &kolyBlock, QList<MISH_BLOCK> *pMishBlocks,
                               QList<QList<BLOCK_DATA>> *pStripes, PDSTRUCT *pPdStruct)
{
    if (pMishBlocks) pMishBlocks->clear();
    if (pStripes) pStripes->clear();
    if (!pMishBlocks || listPartitions.isEmpty() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    quint64 nExpectedStartSector = 0;
    quint32 nMasterCRC = 0xFFFFFFFFU;
    bool bCanValidateMasterCRC = true;
    for (qint32 i = 0; (i < listPartitions.size()) &&
                         XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        MISH_BLOCK mishBlock = {};
        QList<BLOCK_DATA> listCurrentStripes;
        if (!_parsePartition(listPartitions.at(i), kolyBlock, &mishBlock,
                             pStripes ? &listCurrentStripes : nullptr, pPdStruct) ||
            (mishBlock.nStartSector != nExpectedStartSector) ||
            (mishBlock.nSectorCount > kolyBlock.nSectorCount - nExpectedStartSector)) {
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
            nMasterCRC = XBinary::_getCRC32(baChecksumBytes.constData(), baChecksumBytes.size(),
                                            nMasterCRC, XBinary::_getCRC32Table_EDB88320());
        }

        pMishBlocks->append(mishBlock);
        if (pStripes) pStripes->append(listCurrentStripes);
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct) ||
        (nExpectedStartSector != kolyBlock.nSectorCount) ||
        (bCanValidateMasterCRC && dmgChecksumIsCRC32(kolyBlock.masterChecksum) &&
         ((nMasterCRC ^ 0xFFFFFFFFU) != kolyBlock.masterChecksum[2]))) {
        pMishBlocks->clear();
        if (pStripes) pStripes->clear();
        return false;
    }
    return true;
}

bool XDMG::_validatePartitionCRC(QIODevice *pDevice, const MISH_BLOCK &mishBlock,
                                 const QList<BLOCK_DATA> &listStripes,
                                 PDSTRUCT *pPdStruct)
{
    if (!dmgChecksumDescriptorValid(mishBlock.checksum)) return false;
    if (!dmgChecksumIsCRC32(mishBlock.checksum)) return true;
    if (!pDevice || !pDevice->isOpen() || !pDevice->isReadable() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    XBinary binary(pDevice);
    QByteArray baBuffer(0x10000, 0);
    quint32 nCRC = 0xFFFFFFFFU;
    for (qint32 i = 0; (i < listStripes.size()) &&
                         XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        const BLOCK_DATA &stripe = listStripes.at(i);
        if ((stripe.nType == DMG_STRIPE_ZEROES) || (stripe.nType == DMG_STRIPE_SKIP) ||
            (stripe.nType == DMG_STRIPE_END)) continue;
        if ((stripe.nSectorCount > (quint64)(std::numeric_limits<qint64>::max)() / DMG_SECTOR_SIZE) ||
            (stripe.nStartSector > (quint64)(std::numeric_limits<qint64>::max)() / DMG_SECTOR_SIZE)) {
            return false;
        }
        const qint64 nOffset = (qint64)(stripe.nStartSector * DMG_SECTOR_SIZE);
        const qint64 nSize = (qint64)(stripe.nSectorCount * DMG_SECTOR_SIZE);
        qint64 nDone = 0;
        while ((nDone < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            const qint32 nChunk = (qint32)qMin<qint64>(baBuffer.size(), nSize - nDone);
            if (binary.read_array_process(nOffset + nDone, baBuffer.data(), nChunk, pPdStruct) != nChunk) return false;
            nCRC = XBinary::_getCRC32(baBuffer.constData(), nChunk, nCRC,
                                      XBinary::_getCRC32Table_EDB88320());
            nDone += nChunk;
        }
    }

    return XBinary::isPdStructNotCanceled(pPdStruct) &&
           ((nCRC ^ 0xFFFFFFFFU) == mishBlock.checksum[2]);
}

QList<XDMG::DMG_PARTITION_INFO> XDMG::_parseBlkxPartitions(const QByteArray &baXml, PDSTRUCT *pPdStruct)
{
    QList<DMG_PARTITION_INFO> listResult;

    if (baXml.isEmpty() || ((quint64)baXml.size() > DMG_MAX_XML_SIZE) ||
        !isPdStructNotCanceled(pPdStruct)) {
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
                ((sPublicId == QLatin1String("-//Apple//DTD PLIST 1.0//EN")) ||
                 (sPublicId == QLatin1String("-//Apple Computer//DTD PLIST 1.0//EN"))) &&
                (sSystemId == QLatin1String("http://www.apple.com/DTDs/PropertyList-1.0.dtd"));
            if (bDtdSeen || bFoundPlist || (nDepth != 0) ||
                (reader.dtdName() != QLatin1String("plist")) ||
                !reader.entityDeclarations().isEmpty() ||
                !reader.notationDeclarations().isEmpty() ||
                (!bNoExternalId && !bApplePlistId)) {
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

            const QStringRef elementName = reader.name();
            if (bWaitingForRootDict) {
                if (((nDepth - 1) != nPlistDepth) ||
                    (elementName != QLatin1String("dict"))) {
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
                    if ((elementName != QLatin1String("dict")) || bFoundBlkx ||
                        bFoundResourceFork || bClosedResourceFork) {
                        bMalformed = true;
                        break;
                    }
                    bFoundResourceFork = true;
                    nResourceForkDictDepth = nDepth;
                } else if (sRootPendingKey == QLatin1String("blkx")) {
                    if ((elementName != QLatin1String("array")) || bFoundBlkx || bClosedBlkx ||
                        bFoundResourceFork) {
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

            if ((nPlistDepth >= 0) && (nDepth == (nPlistDepth + 1)) &&
                (nRootDictDepth < 0)) {
                // A plist contains exactly one root value.  The first one was
                // required to be a dictionary above.
                bMalformed = true;
                break;
            }

            if (nBlkxArrayDepth >= 0) {
                if (nPartitionDictDepth < 0) {
                    if ((nDepth != (nBlkxArrayDepth + 1)) ||
                        (elementName != QLatin1String("dict"))) {
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
                    if (((sPendingKey == QLatin1String("Name")) ||
                         (sPendingKey == QLatin1String("CFName"))) && !bStringValue) {
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
            const QStringRef elementName = reader.name();
            if ((nPartitionDictDepth == nDepth) && (elementName == QLatin1String("dict"))) {
                if (bPartitionKeyPending || !bDataSeen || baMishData.isEmpty() ||
                    (listResult.size() >= DMG_MAX_PARTITIONS)) {
                    bMalformed = true;
                } else {
                    DMG_PARTITION_INFO info = {};
                    info.sName = !sName.isEmpty() ? sName : sCFName;
                    if (info.sName.isEmpty()) info.sName = QString("partition%1").arg(listResult.size());
                    info.mishData = baMishData;
                    listResult.append(info);
                }
                nPartitionDictDepth = -1;
            } else if ((nBlkxArrayDepth == nDepth) &&
                       (elementName == QLatin1String("array"))) {
                if (nPartitionDictDepth >= 0) bMalformed = true;
                nBlkxArrayDepth = -1;
                bClosedBlkx = true;
            } else if ((nResourceForkDictDepth == nDepth) &&
                       (elementName == QLatin1String("dict"))) {
                if (bResourceKeyPending || !bFoundBlkx || !bClosedBlkx) bMalformed = true;
                nResourceForkDictDepth = -1;
                bClosedResourceFork = true;
            } else if ((nRootDictDepth == nDepth) &&
                       (elementName == QLatin1String("dict"))) {
                if (bRootKeyPending || !bFoundBlkx ||
                    (bFoundResourceFork && !bClosedResourceFork)) bMalformed = true;
                nRootDictDepth = -1;
                bClosedRootDict = true;
            } else if ((nPlistDepth == nDepth) &&
                       (elementName == QLatin1String("plist"))) {
                if (bWaitingForRootDict || !bClosedRootDict || (nRootDictDepth >= 0)) bMalformed = true;
                nPlistDepth = -1;
            }
            nDepth--;
            if (nDepth < 0) bMalformed = true;
        }
    }

    if (reader.hasError() || bMalformed || !isPdStructNotCanceled(pPdStruct) ||
        bWaitingForRootDict || bRootKeyPending || bResourceKeyPending || bPartitionKeyPending ||
        !bFoundPlist || !bClosedRootDict ||
        !bFoundBlkx || !bClosedBlkx ||
        (bFoundResourceFork && !bClosedResourceFork) ||
        (nPlistDepth >= 0) || (nRootDictDepth >= 0) || (nResourceForkDictDepth >= 0) ||
        (nDepth != 0)) {
        listResult.clear();
    }

    return listResult;
}

QList<XDMG::DMG_PARTITION_INFO> XDMG::_parseResourceForkPartitions(
    const QByteArray &baResource, PDSTRUCT *pPdStruct)
{
    QList<DMG_PARTITION_INFO> listResult;
    if ((baResource.size() < 0x100) || ((quint64)baResource.size() > DMG_MAX_RESOURCE_SIZE) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    const auto read16 = [&](quint32 nOffset, quint16 *pValue) -> bool {
        if (!pValue || (nOffset > (quint32)baResource.size()) ||
            ((quint32)baResource.size() - nOffset < 2)) return false;
        const uchar *p = reinterpret_cast<const uchar *>(baResource.constData() + nOffset);
        *pValue = ((quint16)p[0] << 8) | p[1];
        return true;
    };
    const auto read32 = [&](quint32 nOffset, quint32 *pValue) -> bool {
        if (!pValue || (nOffset > (quint32)baResource.size()) ||
            ((quint32)baResource.size() - nOffset < 4)) return false;
        const uchar *p = reinterpret_cast<const uchar *>(baResource.constData() + nOffset);
        *pValue = ((quint32)p[0] << 24) | ((quint32)p[1] << 16) |
                  ((quint32)p[2] << 8) | p[3];
        return true;
    };

    quint32 nDataOffset = 0;
    quint32 nMapOffset = 0;
    quint32 nDataLength = 0;
    quint32 nMapLength = 0;
    if (!read32(0, &nDataOffset) || !read32(4, &nMapOffset) ||
        !read32(8, &nDataLength) || !read32(12, &nMapLength) ||
        (nDataOffset != 0x100) ||
        ((quint64)nDataOffset + nDataLength != nMapOffset) ||
        ((quint64)nMapOffset + nMapLength > (quint64)baResource.size()) ||
        (nMapLength < 0x1e)) {
        return listResult;
    }

    const quint64 nMapEnd = (quint64)nMapOffset + nMapLength;
    const quint64 nTrailing = (quint64)baResource.size() - nMapEnd;
    quint32 nTrailingValue = 0;
    if ((nTrailing != 0) &&
        ((nTrailing != 4) || !read32((quint32)nMapEnd, &nTrailingValue) || (nTrailingValue != 0))) {
        return listResult;
    }
    if (memcmp(baResource.constData(), baResource.constData() + nMapOffset, 16) != 0) {
        return listResult;
    }

    quint16 nTypeListOffset = 0;
    quint16 nNameListOffset = 0;
    quint16 nTypesMinusOne = 0;
    if (!read16(nMapOffset + 0x18, &nTypeListOffset) ||
        !read16(nMapOffset + 0x1a, &nNameListOffset) ||
        !read16(nMapOffset + 0x1c, &nTypesMinusOne) ||
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
        if (!read32(nTypeOffset, &nType) || !read16(nTypeOffset + 4, &nResourcesMinusOne) ||
            !read16(nTypeOffset + 6, &nReferenceListOffset)) {
            listResult.clear();
            return listResult;
        }

        const quint32 nNumResources = (quint32)nResourcesMinusOne + 1;
        nTotalReferences += nNumResources;
        const quint64 nReferenceStart = 0x1cULL + nReferenceListOffset;
        const quint64 nReferenceEnd = nReferenceStart + (quint64)nNumResources * 12ULL;
        if ((nTotalReferences > 1000000ULL) || (nReferenceStart < nTypeEntriesEnd) ||
            (nReferenceEnd > nNameListOffset)) {
            listResult.clear();
            return listResult;
        }

        for (quint32 k = 0; (k < nNumResources) && XBinary::isPdStructNotCanceled(pPdStruct); k++) {
            const quint32 nReferenceOffset = nMapOffset + (quint32)nReferenceStart + k * 12;
            quint16 nNameOffset = 0;
            quint32 nDataField = 0;
            if (!read16(nReferenceOffset + 2, &nNameOffset) ||
                !read32(nReferenceOffset + 4, &nDataField)) {
                listResult.clear();
                return listResult;
            }

            const quint32 nBlockOffset = nDataField & 0x00FFFFFFU;
            quint32 nBlockSize = 0;
            if (nBlockOffset > nDataLength || (nDataLength - nBlockOffset < 4) ||
                !read32(nDataOffset + nBlockOffset, &nBlockSize) ||
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
                if ((nBlockSize < 204) || ((quint64)nBlockSize > DMG_MAX_MISH_SIZE) ||
                    (listResult.size() >= DMG_MAX_PARTITIONS)) {
                    listResult.clear();
                    return listResult;
                }
                DMG_PARTITION_INFO info = {};
                info.sName = sName;
                info.mishData = baResource.mid((qint32)(nDataOffset + nBlockOffset + 4),
                                               (qint32)nBlockSize);
                listResult.append(info);
            }
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) listResult.clear();
    return listResult;
}

bool XDMG::_decompressStripe(const BLOCK_DATA &stripe, qint64 nDataForkOffset, qint64 nDataForkLength,
                             qint64 nMishDataOffset, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pDevice || !pDevice->isOpen() || !pDevice->isWritable() || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (nDataForkOffset < 0) || (nDataForkLength < 0) || (nMishDataOffset < 0) ||
        (nMishDataOffset > nDataForkLength) ||
        (stripe.nSectorCount > (quint64)(std::numeric_limits<qint64>::max)() / DMG_SECTOR_SIZE)) {
        return false;
    }

    const qint64 nExpectedSize = (qint64)(stripe.nSectorCount * DMG_SECTOR_SIZE);
    const auto getDataRange = [&](qint64 *pOffset, qint64 *pSize) -> bool {
        if (!pOffset || !pSize || (stripe.nDataOffset > (quint64)(std::numeric_limits<qint64>::max)()) ||
            (stripe.nDataLength > (quint64)(std::numeric_limits<qint64>::max)())) {
            return false;
        }
        const qint64 nRelativeOffset = (qint64)stripe.nDataOffset;
        const qint64 nLength = (qint64)stripe.nDataLength;
        const qint64 nFileSize = getSize();
        if ((nRelativeOffset > (nDataForkLength - nMishDataOffset)) ||
            (nLength > (nDataForkLength - nMishDataOffset - nRelativeOffset)) ||
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
        case DMG_STRIPE_ZEROES:
            return _writeZeroes(pDevice, nExpectedSize, pPdStruct);

        case DMG_STRIPE_STORED: {
            qint64 nInputOffset = 0;
            qint64 nInputSize = 0;
            if (!getDataRange(&nInputOffset, &nInputSize) || (nInputSize != nExpectedSize)) return false;

            QByteArray baBuffer(0x10000, 0);
            qint64 nDone = 0;
            while ((nDone < nInputSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                const qint32 nChunk = (qint32)qMin<qint64>(baBuffer.size(), nInputSize - nDone);
                if ((read_array_process(nInputOffset + nDone, baBuffer.data(), nChunk, pPdStruct) != nChunk) ||
                    !dmgWriteAll(pDevice, baBuffer.constData(), nChunk, pPdStruct)) {
                    return false;
                }
                nDone += nChunk;
            }
            return (nDone == nInputSize) && XBinary::isPdStructNotCanceled(pPdStruct);
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
                    if (read_array_process(nReadOffset, baInput.data(), nChunk, pPdStruct) != nChunk) {
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
                if ((nOutputSize > nExpectedSize) || (nProduced > (nExpectedSize - nOutputSize)) ||
                    ((nProduced > 0) && !dmgWriteAll(pDevice, baOutput.constData(), nProduced, pPdStruct))) {
                    bResult = false;
                    break;
                }
                nOutputSize += nProduced;

                if ((nResult == Z_OK) && (nProduced == 0) && (stream.avail_in == 0) && (nUnread == 0)) {
                    bResult = false;
                    break;
                }
            }

            const bool bExactInput = (nUnread == 0) && (stream.avail_in == 0);
            inflateEnd(&stream);
            return bResult && XBinary::isPdStructNotCanceled(pPdStruct) && (nResult == Z_STREAM_END) &&
                   (nOutputSize == nExpectedSize) && bExactInput;
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
            state.pDeviceInput = getDevice();
            state.pDeviceOutput = &stripeStaging;
            state.nInputOffset = nInputOffset;
            state.nInputLimit = nInputSize;
            state.nProcessedOffset = 0;
            state.nProcessedLimit = nExpectedSize;
            state.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nExpectedSize);
            if (!XBZIP2Decoder::decompress(&state, pPdStruct) ||
                (state.nCountInput != nInputSize) || (state.nCountOutput != nExpectedSize) ||
                state.bReadError || state.bWriteError || (stripeStaging.size() != nExpectedSize) ||
                !XBinary::isPdStructNotCanceled(pPdStruct) || !stripeStaging.seek(0)) {
                return false;
            }

            QByteArray baBuffer(0x10000, 0);
            qint64 nRemaining = nExpectedSize;
            while ((nRemaining > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                const qint32 nChunk = (qint32)qMin<qint64>(baBuffer.size(), nRemaining);
                if ((stripeStaging.read(baBuffer.data(), nChunk) != nChunk) ||
                    !dmgWriteAll(pDevice, baBuffer.constData(), nChunk, pPdStruct)) {
                    return false;
                }
                nRemaining -= nChunk;
            }
            return (nRemaining == 0) && XBinary::isPdStructNotCanceled(pPdStruct);
        }

        case DMG_STRIPE_ADC:
        case DMG_STRIPE_LZFSE:
        case DMG_STRIPE_XZ:
            // Unsupported stripes must never be reported as successfully
            // extracted with fabricated zero-filled data.
            return false;

        case DMG_STRIPE_SKIP:
        case DMG_STRIPE_END:
            return nExpectedSize == 0;

        default:
            return false;
    }
}

bool XDMG::_writeZeroes(QIODevice *pDevice, qint64 nSize, PDSTRUCT *pPdStruct)
{
    if (!pDevice || (nSize < 0)) return false;
    QByteArray baZeroes(4096, 0);
    while ((nSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nToWrite = qMin(nSize, (qint64)baZeroes.size());
        if (!dmgWriteAll(pDevice, baZeroes.constData(), nToWrite, pPdStruct)) return false;
        nSize -= nToWrite;
    }
    return (nSize == 0) && XBinary::isPdStructNotCanceled(pPdStruct);
}

QList<QString> XDMG::getSearchSignatures()
{
    QList<QString> listResult;

    // UDIF DMG trailer signature.
    listResult.append("'koly'");

    return listResult;
}

XBinary *XDMG::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XDMG(pDevice);
}

bool XDMG::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XArchive::handleInternalInfo(pPdStruct);
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) =
            *static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
    }

    return bResult;
}

void *XDMG::getInternalInfo(PDSTRUCT *pPdStruct)
{
    handleInternalInfo(pPdStruct);

    return &m_internalInfo;
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
