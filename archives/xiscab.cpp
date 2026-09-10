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
#include "xiscab.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QPointer>
#include <QSet>
#include <QTemporaryFile>
#include <QtEndian>

#include <limits>
#include <new>

#include "zlib.h"

namespace {
const quint32 IS_CAB_SIGNATURE = 0x28635349U;  // "ISc(" as little endian.
const qint64 IS_COMMON_HEADER_SIZE = 20;
const qint64 IS_VOLUME_HEADER_V5_SIZE = 40;
const qint64 IS_VOLUME_HEADER_V6_SIZE = 64;
const qint64 IS_MAX_CATALOG_SIZE = 256LL * 1024 * 1024;
const quint32 IS_MAX_RECORDS = 1000000U;
const qint32 IS_MAX_STRING_BYTES = 0x10000;
const quint16 IS_FILE_SPLIT = 0x0001;
const quint16 IS_FILE_OBFUSCATED = 0x0002;
const quint16 IS_FILE_COMPRESSED = 0x0004;
const quint16 IS_FILE_INVALID = 0x0008;
const quint8 IS_LINK_PREV = 0x01;

bool isRangeWithin(qint64 nSize, quint64 nOffset, quint64 nLength)
{
    if (nSize < 0) return false;
    const quint64 nTotal = static_cast<quint64>(nSize);
    return (nOffset <= nTotal) && (nLength <= (nTotal - nOffset));
}

bool isRangeWithin(const QByteArray &baData, quint64 nOffset, quint64 nLength)
{
    return isRangeWithin(baData.size(), nOffset, nLength);
}

quint16 readLE16(const QByteArray &baData, quint64 nOffset)
{
    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baData.constData() + nOffset));
}

quint32 readLE32(const QByteArray &baData, quint64 nOffset)
{
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData() + nOffset));
}

quint64 readLE64(const QByteArray &baData, quint64 nOffset)
{
    return qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(baData.constData() + nOffset));
}

qint32 majorVersion(quint32 nVersion)
{
    const quint8 nFamily = static_cast<quint8>(nVersion >> 24);
    if (nFamily == 1) return static_cast<qint32>((nVersion >> 12) & 0x0f);
    if ((nFamily == 2) || (nFamily == 4)) {
        const quint32 nValue = nVersion & 0xffffU;
        return nValue ? static_cast<qint32>(nValue / 100U) : 0;
    }
    return 0;
}

QString mediaPrefixFromPath(const QString &sPath)
{
    if (sPath.isEmpty()) return QString();
    const QFileInfo fileInfo(sPath);
    const QString sBaseName = fileInfo.completeBaseName();
    qint32 nCut = sBaseName.size();
    while ((nCut > 0) && sBaseName.at(nCut - 1).isDigit()) --nCut;
    if (nCut <= 0) return QString();
    return QDir(fileInfo.absolutePath()).filePath(sBaseName.left(nCut));
}

QString resolveCaseInsensitivePath(const QString &sCandidate)
{
    const QFileInfo exactInfo(sCandidate);
    if (exactInfo.exists() && exactInfo.isFile()) return exactInfo.absoluteFilePath();

    QDir directory(exactInfo.absolutePath());
    const QString sWanted = exactInfo.fileName().toCaseFolded();
    const QFileInfoList listFiles = directory.entryInfoList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &fileInfo : listFiles) {
        if (fileInfo.fileName().toCaseFolded() == sWanted) return fileInfo.absoluteFilePath();
    }
    return QString();
}

QString mediaPath(const QString &sPrefix, quint32 nVolume, const QString &sSuffix, QMap<QString, QString> *pResolvedPaths = nullptr)
{
    if (sPrefix.isEmpty() || !nVolume) return QString();
    const QString sCandidate = QStringLiteral("%1%2.%3").arg(sPrefix).arg(nVolume).arg(sSuffix);
    if (pResolvedPaths) {
        const QMap<QString, QString>::const_iterator it =
            pResolvedPaths->constFind(sCandidate);
        if (it != pResolvedPaths->constEnd()) return it.value();
    }

    const QString sResult = resolveCaseInsensitivePath(sCandidate);
    if (pResolvedPaths) pResolvedPaths->insert(sCandidate, sResult);
    return sResult;
}

bool readFileBounded(const QString &sFileName, QByteArray *pData)
{
    if (!pData || sFileName.isEmpty()) return false;
    pData->clear();
    QFile file(sFileName);
    if (!file.open(QIODevice::ReadOnly) || file.isSequential()) return false;
    const qint64 nSize = file.size();
    if ((nSize < IS_COMMON_HEADER_SIZE) || (nSize > IS_MAX_CATALOG_SIZE) || (nSize > (std::numeric_limits<qint32>::max)())) {
        return false;
    }
    *pData = file.read(nSize);
    return pData->size() == nSize;
}

QString readCabString(const QByteArray &baData, quint64 nOffset, qint32 nMajorVersion)
{
    if (!isRangeWithin(baData, nOffset, 1)) return QString();
    if (nMajorVersion >= 17) {
        QString sResult;
        for (qint32 i = 0; i < (IS_MAX_STRING_BYTES / 2); ++i) {
            const quint64 nCharacterOffset = nOffset + (quint64)i * 2U;
            if (!isRangeWithin(baData, nCharacterOffset, 2)) return QString();
            const quint16 nCharacter = readLE16(baData, nCharacterOffset);
            if (!nCharacter) return sResult;
            sResult.append(QChar(nCharacter));
        }
        return QString();
    }

    qint32 nLength = 0;
    while (nLength < IS_MAX_STRING_BYTES) {
        if (!isRangeWithin(baData, nOffset + (quint64)nLength, 1)) return QString();
        if (!baData.at(static_cast<qint32>(nOffset) + nLength)) return QString::fromLatin1(baData.constData() + nOffset, nLength);
        ++nLength;
    }
    return QString();
}

// Returns the sanitized "directory/name" relative path, or an empty string
// when the raw catalog strings are unsafe for filesystem use.
QString safeCombinedName(QString sDirectory, QString sFileName)
{
    sDirectory.replace(QLatin1Char('\\'), QLatin1Char('/'));
    sFileName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    QString sCombined = sDirectory.isEmpty() ? sFileName : (sDirectory + QLatin1Char('/') + sFileName);
    sCombined = QDir::cleanPath(sCombined);

    bool bSafe = !sCombined.isEmpty() && !sCombined.startsWith(QLatin1Char('/')) && !sCombined.startsWith(QLatin1String("../")) && (sCombined != QLatin1String("..")) &&
                 !sCombined.contains(QLatin1String("/../")) && !sCombined.contains(QLatin1Char(':'));
    const QStringList listParts = sCombined.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) || (sPart == QLatin1String(".."))) {
            bSafe = false;
            break;
        }
    }
    if (bSafe) {
        const QString sFixed = XBinary::fixFileName(sCombined);
        if (!sFixed.isEmpty()) return sFixed;
    }
    return QString();
}

QString safeArchiveName(const QString &sDirectory, const QString &sFileName, qint32 nIndex)
{
    const QString sCombined = safeCombinedName(sDirectory, sFileName);
    if (!sCombined.isEmpty()) return sCombined;
    return QStringLiteral("file_%1.bin").arg(nIndex, 6, 10, QLatin1Char('0'));
}

QString uniqueArchiveName(const QString &sName, QSet<QString> *pNames)
{
    if (!pNames) return sName;
    QString sResult = sName;
    QString sKey = sResult.toCaseFolded();
    qint32 nSuffix = 2;
    while (pNames->contains(sKey) && (nSuffix <= 1000000)) {
        const qint32 nSlash = sName.lastIndexOf(QLatin1Char('/'));
        const qint32 nDot = sName.lastIndexOf(QLatin1Char('.'));
        const bool bHasExtension = nDot > nSlash + 1;
        const QString sTag = QStringLiteral("_%1").arg(nSuffix++);
        sResult = bHasExtension ? sName.left(nDot) + sTag + sName.mid(nDot) : sName + sTag;
        sKey = sResult.toCaseFolded();
    }
    pNames->insert(sKey);
    return sResult;
}

struct VOLUME_HEADER {
    quint64 nDataOffset = 0;
    quint32 nFirstFileIndex = 0;
    quint32 nLastFileIndex = 0;
    quint64 nFirstFileOffset = 0;
    quint64 nFirstExpandedSize = 0;
    quint64 nFirstCompressedSize = 0;
    quint64 nLastFileOffset = 0;
    quint64 nLastExpandedSize = 0;
    quint64 nLastCompressedSize = 0;
};

bool parseVolumeFile(const QString &sPath, qint32 nMajorVersion, VOLUME_HEADER *pHeader, qint64 *pnFileSize, quint64 nBaseOffset = 0, quint64 nWindowSize = 0,
                     const QByteArray *pPinnedHeader = nullptr)
{
    if (!pHeader || !pnFileSize || sPath.isEmpty()) return false;
    *pHeader = VOLUME_HEADER();
    *pnFileSize = -1;
    QFile file(sPath);
    if (!file.open(QIODevice::ReadOnly) || file.isSequential()) return false;
    // An embedded volume is a window inside the container file; its logical
    // size is the window size, and all header reads shift by the base offset.
    const qint64 nPhysicalSize = file.size();
    if ((nPhysicalSize < 0) || !isRangeWithin(nPhysicalSize, nBaseOffset, nWindowSize)) return false;
    *pnFileSize = nWindowSize ? static_cast<qint64>(nWindowSize) : (nPhysicalSize - static_cast<qint64>(nBaseOffset));
    const qint64 nHeaderSize = IS_COMMON_HEADER_SIZE + ((nMajorVersion <= 5) ? IS_VOLUME_HEADER_V5_SIZE : IS_VOLUME_HEADER_V6_SIZE);
    if (*pnFileSize < nHeaderSize) return false;
    if (!file.seek(static_cast<qint64>(nBaseOffset))) return false;
    const QByteArray baHeader = file.read(nHeaderSize);
    if ((baHeader.size() != nHeaderSize) || (readLE32(baHeader, 0) != IS_CAB_SIGNATURE)) {
        return false;
    }
    if (pPinnedHeader && !pPinnedHeader->isEmpty()) {
        const qint32 nCompareSize = qMin<qint32>(pPinnedHeader->size(), baHeader.size());
        if ((nCompareSize <= 0) || (memcmp(pPinnedHeader->constData(), baHeader.constData(), nCompareSize) != 0)) {
            return false;
        }
    }
    if (majorVersion(readLE32(baHeader, 4)) != nMajorVersion) return false;

    quint64 nOffset = IS_COMMON_HEADER_SIZE;
    if (nMajorVersion <= 5) {
        pHeader->nDataOffset = readLE32(baHeader, nOffset);
        nOffset += 8;
        pHeader->nFirstFileIndex = readLE32(baHeader, nOffset);
        nOffset += 4;
        pHeader->nLastFileIndex = readLE32(baHeader, nOffset);
        nOffset += 4;
        pHeader->nFirstFileOffset = readLE32(baHeader, nOffset);
        nOffset += 4;
        pHeader->nFirstExpandedSize = readLE32(baHeader, nOffset);
        nOffset += 4;
        pHeader->nFirstCompressedSize = readLE32(baHeader, nOffset);
        nOffset += 4;
        pHeader->nLastFileOffset = readLE32(baHeader, nOffset);
        nOffset += 4;
        pHeader->nLastExpandedSize = readLE32(baHeader, nOffset);
        nOffset += 4;
        pHeader->nLastCompressedSize = readLE32(baHeader, nOffset);
        if (!pHeader->nLastFileOffset) pHeader->nLastFileOffset = 0x7fffffffU;
    } else {
        pHeader->nDataOffset = readLE64(baHeader, nOffset);
        nOffset += 8;
        pHeader->nFirstFileIndex = readLE32(baHeader, nOffset);
        nOffset += 4;
        pHeader->nLastFileIndex = readLE32(baHeader, nOffset);
        nOffset += 4;
        pHeader->nFirstFileOffset = readLE64(baHeader, nOffset);
        nOffset += 8;
        pHeader->nFirstExpandedSize = readLE64(baHeader, nOffset);
        nOffset += 8;
        pHeader->nFirstCompressedSize = readLE64(baHeader, nOffset);
        nOffset += 8;
        pHeader->nLastFileOffset = readLE64(baHeader, nOffset);
        nOffset += 8;
        pHeader->nLastExpandedSize = readLE64(baHeader, nOffset);
        nOffset += 8;
        pHeader->nLastCompressedSize = readLE64(baHeader, nOffset);
    }
    return (pHeader->nDataOffset <= static_cast<quint64>(*pnFileSize)) && (pHeader->nFirstFileIndex <= pHeader->nLastFileIndex);
}

struct INPUT_SEGMENT {
    QString sPath;
    quint64 nOffset = 0;
    quint64 nSize = 0;
};

bool buildSegments(const QString &sPrefix, const QString &sContainerPath, const QMap<quint32, XISCab::EMBEDDED_VOLUME> &mapEmbedded,
                   QMap<QString, QString> *pResolvedMediaPaths, qint32 nMajorVersion,
                   quint16 nFlags, quint64 nExpandedSize, quint64 nCompressedSize, quint64 nDataOffset, quint16 nEntryVolume, qint32 nEntryIndex, qint32 nEntryCount,
                   QList<INPUT_SEGMENT> *pSegments)
{
    const bool bEmbedded = !mapEmbedded.isEmpty();
    if (!pSegments || (bEmbedded ? sContainerPath.isEmpty() : sPrefix.isEmpty()) || (nEntryIndex < 0) || (nEntryCount <= nEntryIndex)) {
        return false;
    }
    pSegments->clear();
    const bool bCompressed = nFlags & IS_FILE_COMPRESSED;
    bool bSplit = nFlags & IS_FILE_SPLIT;
    quint64 nRemaining = bCompressed ? nCompressedSize : nExpandedSize;
    quint32 nVolume = (nMajorVersion <= 5) ? 1U : qMax<quint32>(1U, nEntryVolume);

    for (quint32 nAttempt = 0; nAttempt < 10000U; ++nAttempt, ++nVolume) {
        quint64 nBaseOffset = 0;
        quint64 nWindowSize = 0;
        QByteArray baPinnedHeader;
        QString sPath;
        if (bEmbedded) {
            if (!mapEmbedded.contains(nVolume)) return false;
            const XISCab::EMBEDDED_VOLUME window = mapEmbedded.value(nVolume);
            if (!window.nSize) return false;
            sPath = sContainerPath;
            nBaseOffset = window.nOffset;
            nWindowSize = window.nSize;
            baPinnedHeader = window.baPinnedHeader;
        } else {
            sPath = mediaPath(sPrefix, nVolume, QStringLiteral("cab"), pResolvedMediaPaths);
        }
        if (sPath.isEmpty()) return false;
        VOLUME_HEADER volume = {};
        qint64 nFileSize = -1;
        if (!parseVolumeFile(sPath, nMajorVersion, &volume, &nFileSize, nBaseOffset, nWindowSize, baPinnedHeader.isEmpty() ? nullptr : &baPinnedHeader)) return false;
        // Empty members still require a real, structurally valid companion
        // volume.  Otherwise a detached catalog could appear extractable just
        // because no payload bytes need copying.
        if (!nRemaining) return true;

        if ((nMajorVersion <= 5) && (static_cast<quint32>(nEntryIndex) > volume.nLastFileIndex)) {
            continue;
        }

        // InstallShield 5 does not reliably publish FILE_SPLIT.  Unshield
        // infers it from the boundary member sizes in each volume.
        if (!bSplit && (nMajorVersion == 5)) {
            if ((nEntryIndex + 1 < nEntryCount) && (static_cast<quint32>(nEntryIndex) == volume.nLastFileIndex) && (volume.nLastCompressedSize != nCompressedSize)) {
                bSplit = true;
            } else if ((nEntryIndex > 0) && (static_cast<quint32>(nEntryIndex) == volume.nFirstFileIndex) && (volume.nFirstCompressedSize != nCompressedSize)) {
                bSplit = true;
            }
        }

        INPUT_SEGMENT segment;
        segment.sPath = sPath;
        if (!bSplit) {
            segment.nOffset = nDataOffset;
            segment.nSize = nRemaining;
        } else if ((static_cast<quint32>(nEntryIndex) == volume.nLastFileIndex) && (volume.nLastFileOffset != 0x7fffffffU)) {
            segment.nOffset = volume.nLastFileOffset;
            segment.nSize = bCompressed ? volume.nLastCompressedSize : volume.nLastExpandedSize;
        } else if (static_cast<quint32>(nEntryIndex) == volume.nFirstFileIndex) {
            segment.nOffset = volume.nFirstFileOffset;
            segment.nSize = bCompressed ? volume.nFirstCompressedSize : volume.nFirstExpandedSize;
        } else {
            return false;
        }

        if (segment.nSize > nRemaining) segment.nSize = nRemaining;
        if ((nRemaining && !segment.nSize) || (segment.nOffset < volume.nDataOffset) || !isRangeWithin(nFileSize, segment.nOffset, segment.nSize)) {
            return false;
        }
        segment.nOffset += nBaseOffset;  // no-op for sibling files; shifts embedded windows into the container
        pSegments->append(segment);
        nRemaining -= segment.nSize;
        if (!nRemaining) return true;
        if (!bSplit) return false;
    }
    return false;
}

class SegmentReader {
public:
    explicit SegmentReader(const QList<INPUT_SEGMENT> &listSegments) : m_listSegments(listSegments), m_nIndex(0), m_nOpenedIndex(-1), m_nPosition(0)
    {
    }

    bool readExact(char *pData, qint64 nSize)
    {
        if (!pData || (nSize < 0)) return false;
        qint64 nDone = 0;
        while (nDone < nSize) {
            if (m_nIndex >= m_listSegments.count()) return false;
            const INPUT_SEGMENT &segment = m_listSegments.at(m_nIndex);
            // Segment identity is the index, not the path: embedded volumes
            // share one container path but live at different offsets.
            if (m_nOpenedIndex != m_nIndex) {
                m_file.close();
                m_file.setFileName(segment.sPath);
                if (!m_file.open(QIODevice::ReadOnly) || !m_file.seek(static_cast<qint64>(segment.nOffset))) {
                    return false;
                }
                m_nOpenedIndex = m_nIndex;
                m_nPosition = 0;
            }
            const quint64 nAvailable = segment.nSize - m_nPosition;
            if (!nAvailable) {
                m_file.close();
                ++m_nIndex;
                m_nPosition = 0;
                continue;
            }
            const qint64 nRequest =
                qMin<qint64>(nSize - nDone, static_cast<qint64>(qMin<quint64>(nAvailable, static_cast<quint64>((std::numeric_limits<qint64>::max)()))));
            const qint64 nRead = m_file.read(pData + nDone, nRequest);
            if (nRead != nRequest) return false;
            nDone += nRead;
            m_nPosition += static_cast<quint64>(nRead);
        }
        return true;
    }

private:
    QList<INPUT_SEGMENT> m_listSegments;
    qint32 m_nIndex;
    qint32 m_nOpenedIndex;
    quint64 m_nPosition;
    QFile m_file;
};

quint8 rotateRight2(quint8 nValue)
{
    return static_cast<quint8>((nValue >> 2) | (nValue << 6));
}

void deobfuscate(char *pData, qint64 nSize, quint64 *pnSeed)
{
    if (!pData || !pnSeed || (nSize <= 0)) return;
    quint64 nSeed = *pnSeed;
    for (qint64 i = 0; i < nSize; ++i, ++nSeed) {
        const quint8 nValue = static_cast<quint8>(pData[i]);
        pData[i] = static_cast<char>(rotateRight2(static_cast<quint8>(nValue ^ 0xd5U)) - static_cast<quint8>(nSeed % 0x47U));
    }
    *pnSeed = nSeed;
}

bool writeAll(QIODevice *pDevice, const char *pData, qint64 nSize)
{
    if (!pDevice || !pData || (nSize < 0)) return false;
    qint64 nDone = 0;
    while (nDone < nSize) {
        const qint64 nWritten = pDevice->write(pData + nDone, nSize - nDone);
        if (nWritten <= 0) return false;
        nDone += nWritten;
    }
    return true;
}

class ISCabChunkPublisher {
public:
    ISCabChunkPublisher(QIODevice *pStageDevice, XBinary::UNPACK_STATE *pState, XBinary::PDSTRUCT *pPdStruct, quint64 nExpandedSize,
                        QCryptographicHash *pHash, quint64 *pnWritten, const QString &sBudgetError)
        : m_pStageDevice(pStageDevice),
          m_pState(pState),
          m_pPdStruct(pPdStruct),
          m_nExpandedSize(nExpandedSize),
          m_pHash(pHash),
          m_pnWritten(pnWritten),
          m_sBudgetError(sBudgetError)
    {
    }

    bool publish(QByteArray *pChunk) const
    {
        if (!pChunk || !m_pState || !m_pHash || !m_pnWritten || !XBinary::isPdStructNotCanceled(m_pPdStruct) || (*m_pnWritten > m_nExpandedSize) ||
            (static_cast<quint64>(pChunk->size()) > (m_nExpandedSize - *m_pnWritten))) {
            return false;
        }
        if (m_pState->spOutputBudget && !m_pState->spOutputBudget->debit(pChunk->size())) {
            if (m_pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(m_pPdStruct, m_sBudgetError);
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(m_pState->spOutputBudget.data());
        }
        if (!writeAll(m_pStageDevice, pChunk->constData(), pChunk->size())) return false;
        m_pHash->addData(*pChunk);
        *m_pnWritten += static_cast<quint64>(pChunk->size());
        return true;
    }

private:
    QIODevice *m_pStageDevice;
    XBinary::UNPACK_STATE *m_pState;
    XBinary::PDSTRUCT *m_pPdStruct;
    quint64 m_nExpandedSize;
    QCryptographicHash *m_pHash;
    quint64 *m_pnWritten;
    QString m_sBudgetError;
};

bool inflateInstallShieldBlock(const QByteArray &baInput, QByteArray *pOutput)
{
    if (!pOutput || baInput.isEmpty()) return false;
    QByteArray baPadded = baInput;
    baPadded.append('\0');
    QByteArray baResult(64 * 1024, '\0');

    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    stream.next_in = reinterpret_cast<Bytef *>(baPadded.data());
    stream.avail_in = static_cast<uInt>(baPadded.size());
    stream.next_out = reinterpret_cast<Bytef *>(baResult.data());
    stream.avail_out = static_cast<uInt>(baResult.size());
    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) return false;
    const int nResult = inflate(&stream, Z_FINISH);
    const bool bResult = (nResult == Z_STREAM_END) && (stream.total_out <= (uLong)baResult.size());
    const qint32 nOutputSize = static_cast<qint32>(stream.total_out);
    inflateEnd(&stream);
    if (!bResult) return false;
    baResult.resize(nOutputSize);
    *pOutput = baResult;
    return true;
}

// Old-style InstallShield chunk: a raw-deflate fragment terminated by a
// 00 00 ff ff sync-flush marker instead of a u16 length prefix.  The stream
// is never finalized (no BFINAL block), so unshield inflates with Z_BLOCK
// until at most one input byte remains (unshield_uncompress_old).
bool inflateInstallShieldOldChunk(const char *pData, qint32 nSize, QByteArray *pOutput)
{
    if (!pOutput || !pData || (nSize <= 0)) return false;
    QByteArray baResult(64 * 1024, '\0');

    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(pData));
    stream.avail_in = static_cast<uInt>(nSize);
    stream.next_out = reinterpret_cast<Bytef *>(baResult.data());
    stream.avail_out = static_cast<uInt>(baResult.size());
    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) return false;
    bool bResult = true;
    while (stream.avail_in > 1) {
        const int nStatus = inflate(&stream, Z_BLOCK);
        if (nStatus == Z_STREAM_END) {
            bResult = stream.avail_in <= 1;
            break;
        }
        if (nStatus != Z_OK) {
            bResult = false;
            break;
        }
        if (!stream.avail_out && (stream.avail_in > 1)) {
            // Output window exhausted with input still pending: the reference
            // caps old-style chunks at 64 KiB of output, so this is corrupt.
            bResult = false;
            break;
        }
    }
    const qint32 nOutputSize = static_cast<qint32>(stream.total_out);
    inflateEnd(&stream);
    if (!bResult) return false;
    baResult.resize(nOutputSize);
    *pOutput = baResult;
    return true;
}

// NEW-style framing walks a u16-length block chain over the whole compressed
// stream.  A first block size can look plausible by accident (seen in the
// corpus), so the framing is only trusted when the chain lands exactly on the
// stream end with every block size nonzero.
bool isNewStyleBlockChain(const QByteArray &baCompressed)
{
    quint64 nOffset = 0;
    const quint64 nSize = static_cast<quint64>(baCompressed.size());
    while (nOffset < nSize) {
        if ((nSize - nOffset) < 2) return false;
        const quint16 nBlockSize = readLE16(baCompressed, nOffset);
        if (!nBlockSize) return false;
        nOffset += 2;
        if (nBlockSize > (nSize - nOffset)) return false;
        nOffset += nBlockSize;
    }
    return nOffset == nSize;
}
}  // namespace

XISCab::XISCab(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XISCab::_readCommonHeader(QIODevice *pDevice, COMMON_HEADER *pHeader, PDSTRUCT *pPdStruct) const
{
    if (pHeader) *pHeader = COMMON_HEADER();
    if (!pDevice || !pHeader || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XISCab> guardedThis(const_cast<XISCab *>(this));
    QPointer<QIODevice> guardedDevice(pDevice);
    const bool bOpen = guardedDevice->isOpen();
    if (!guardedThis || !guardedDevice || !bOpen) return false;
    const bool bReadable = guardedDevice->isReadable();
    if (!guardedThis || !guardedDevice || !bReadable) return false;
    const bool bSequential = guardedDevice->isSequential();
    if (!guardedThis || !guardedDevice || bSequential) return false;

    const qint64 nSavedPosition = guardedDevice->pos();
    if (!guardedThis || !guardedDevice || (nSavedPosition < 0)) return false;
    const qint64 nSize = guardedDevice->size();
    if (!guardedThis || !guardedDevice || (nSize < IS_COMMON_HEADER_SIZE)) return false;
    const qint64 nReadSize = qMin<qint64>(nSize, IS_COMMON_HEADER_SIZE + IS_VOLUME_HEADER_V6_SIZE);
    const bool bSeeked = guardedDevice->seek(0);
    if (!guardedThis || !guardedDevice || !bSeeked) return false;
    const QByteArray baHeader = guardedDevice->read(nReadSize);
    if (!guardedThis || !guardedDevice) return false;
    const bool bRestored = guardedDevice->seek(nSavedPosition);
    if (!guardedThis || !guardedDevice || !bRestored || (baHeader.size() != nReadSize) || (readLE32(baHeader, 0) != IS_CAB_SIGNATURE)) {
        return false;
    }

    pHeader->nVersion = readLE32(baHeader, 4);
    pHeader->nVolumeInfo = readLE32(baHeader, 8);
    pHeader->nDescriptorOffset = readLE32(baHeader, 12);
    pHeader->nDescriptorSize = readLE32(baHeader, 16);
    pHeader->nMajorVersion = majorVersion(pHeader->nVersion);
    if (pHeader->nMajorVersion > 32) return false;
    if (pHeader->nDescriptorSize) {
        return (pHeader->nDescriptorOffset >= IS_COMMON_HEADER_SIZE) && isRangeWithin(nSize, pHeader->nDescriptorOffset, 0x30);
    }

    const qint64 nVolumeHeaderSize = (pHeader->nMajorVersion <= 5) ? IS_VOLUME_HEADER_V5_SIZE : IS_VOLUME_HEADER_V6_SIZE;
    if ((nSize < (IS_COMMON_HEADER_SIZE + nVolumeHeaderSize)) || (baHeader.size() < (IS_COMMON_HEADER_SIZE + nVolumeHeaderSize))) {
        return false;
    }
    const quint64 nDataOffset = (pHeader->nMajorVersion <= 5) ? readLE32(baHeader, IS_COMMON_HEADER_SIZE) : readLE64(baHeader, IS_COMMON_HEADER_SIZE);
    return nDataOffset <= static_cast<quint64>(nSize);
}

bool XISCab::isValid(PDSTRUCT *pPdStruct)
{
    COMMON_HEADER common;
    return _readCommonHeader(getDevice(), &common, pPdStruct);
}

bool XISCab::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XISCab archive(pDevice);
    return archive.isValid(pPdStruct);
}

XISCab::INTERNAL_INFO XISCab::_getInternalInfo(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO result;
    COMMON_HEADER common;
    result.bIsValid = _readCommonHeader(getDevice(), &common, pPdStruct);
    if (result.bIsValid) {
        result.nMajorVersion = common.nMajorVersion;
        result.bHasCabDescriptor = common.nDescriptorSize != 0;
        result.nVolumeNumber = common.nVolumeInfo;
    }
    return result;
}

bool XISCab::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XISCab> guardedThis(this);
    if (!isInternalInfoHandled()) {
        if (!XArchive::handleInternalInfo(pPdStruct) || !guardedThis) return false;
        XArchive::INTERNAL_INFO *pBase = static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pBase) return false;
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) = *pBase;
        const INTERNAL_INFO detected = _getInternalInfo(pPdStruct);
        if (!guardedThis) return false;
        m_internalInfo.bIsValid = detected.bIsValid;
        m_internalInfo.nMajorVersion = detected.nMajorVersion;
        m_internalInfo.bHasCabDescriptor = detected.bHasCabDescriptor;
        m_internalInfo.nVolumeNumber = detected.nVolumeNumber;
    }
    return true;
}

void *XISCab::getInternalInfo(PDSTRUCT *pPdStruct)
{
    return handleInternalInfo(pPdStruct) ? &m_internalInfo : nullptr;
}

void XISCab::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}

XBinary::FT XISCab::getFileType()
{
    return FT_ISCAB;
}

XBinary::MODE XISCab::getMode()
{
    return MODE_32;
}

qint32 XISCab::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XISCab::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XISCab::getArch()
{
    return QString();
}

QString XISCab::getFileFormatExt()
{
    return QStringLiteral("cab");
}

QString XISCab::getFileFormatExtsString()
{
    return QStringLiteral("InstallShield Cabinet (*.cab *.hdr)");
}

QString XISCab::getMIMEString()
{
    return QStringLiteral("application/x-installshield-cabinet");
}

qint64 XISCab::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return isValid(pPdStruct) ? getSize() : 0;
}

XBinary::OSNAME XISCab::getOsName()
{
    return OSNAME_WINDOWS;
}

QString XISCab::getVersion()
{
    INTERNAL_INFO *pInfo = static_cast<INTERNAL_INFO *>(getInternalInfo(nullptr));
    return (pInfo && pInfo->bIsValid && pInfo->nMajorVersion) ? QString::number(pInfo->nMajorVersion) : QString();
}

QList<QString> XISCab::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'ISc('");
}

XBinary *XISCab::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XISCab(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XISCab::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XISCab::_loadCatalog(UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct) const
{
    if (!pContext) return false;
    QByteArray *pCatalog = &pContext->baCatalog;
    QString *pMediaPrefix = &pContext->sMediaPrefix;
    QString *pSourcePath = &pContext->sSourcePath;
    COMMON_HEADER *pHeader = &pContext->common;
    pCatalog->clear();
    pMediaPrefix->clear();
    pSourcePath->clear();
    pContext->sContainerPath.clear();
    pContext->mapEmbeddedVolumes.clear();
    pContext->mapResolvedMediaPaths.clear();
    *pHeader = COMMON_HEADER();

    QPointer<XISCab> guardedThis(const_cast<XISCab *>(this));
    QPointer<QIODevice> guardedDevice(guardedThis ? guardedThis->getDevice() : nullptr);
    if (!guardedThis || !guardedDevice) return false;
    COMMON_HEADER sourceHeader;
    const bool bHeaderRead = _readCommonHeader(guardedDevice.data(), &sourceHeader, pPdStruct);
    if (!guardedThis || !guardedDevice || !bHeaderRead) return false;

    QFile *pSourceFile = dynamic_cast<QFile *>(guardedDevice.data());
    const QString sSourcePath = pSourceFile ? QFileInfo(pSourceFile->fileName()).absoluteFilePath() : QString();
    const QString sPrefix = mediaPrefixFromPath(sSourcePath);

    if (sourceHeader.nDescriptorSize) {
        const qint64 nSize = guardedDevice->size();
        if (!guardedThis || !guardedDevice || (nSize < IS_COMMON_HEADER_SIZE) || (nSize > IS_MAX_CATALOG_SIZE) || (nSize > (std::numeric_limits<qint32>::max)())) {
            return false;
        }
        const qint64 nSavedPosition = guardedDevice->pos();
        if (!guardedThis || !guardedDevice || (nSavedPosition < 0)) return false;
        const bool bSeeked = guardedDevice->seek(0);
        if (!guardedThis || !guardedDevice || !bSeeked) return false;
        *pCatalog = guardedDevice->read(nSize);
        if (!guardedThis || !guardedDevice) {
            pCatalog->clear();
            return false;
        }
        const bool bRestored = guardedDevice->seek(nSavedPosition);
        if (!guardedThis || !guardedDevice || !bRestored || (pCatalog->size() != nSize)) {
            pCatalog->clear();
            return false;
        }
        *pHeader = sourceHeader;
        *pMediaPrefix = sPrefix;
        *pSourcePath = sSourcePath;
        return true;
    }

    if (sPrefix.isEmpty()) {
        XBinary::setPdStructErrorString(pPdStruct, tr("InstallShield cabinet catalog is not embedded; DATA1.HDR is required"));
        return false;
    }

    const QStringList listCandidates = QStringList() << mediaPath(sPrefix, 1, QStringLiteral("hdr"), &pContext->mapResolvedMediaPaths)
                                                     << mediaPath(sPrefix, 1, QStringLiteral("cab"), &pContext->mapResolvedMediaPaths);
    for (const QString &sCandidate : listCandidates) {
        if (sCandidate.isEmpty()) continue;
        QByteArray baCandidate;
        if (!readFileBounded(sCandidate, &baCandidate)) continue;
        if (!isRangeWithin(baCandidate, 0, IS_COMMON_HEADER_SIZE) || (readLE32(baCandidate, 0) != IS_CAB_SIGNATURE)) {
            continue;
        }
        COMMON_HEADER candidate;
        candidate.nVersion = readLE32(baCandidate, 4);
        candidate.nVolumeInfo = readLE32(baCandidate, 8);
        candidate.nDescriptorOffset = readLE32(baCandidate, 12);
        candidate.nDescriptorSize = readLE32(baCandidate, 16);
        candidate.nMajorVersion = majorVersion(candidate.nVersion);
        if (!candidate.nDescriptorSize || (candidate.nMajorVersion != sourceHeader.nMajorVersion) || (candidate.nMajorVersion > 32) ||
            (candidate.nDescriptorOffset < IS_COMMON_HEADER_SIZE) || !isRangeWithin(baCandidate, candidate.nDescriptorOffset, 0x30)) {
            continue;
        }
        *pCatalog = baCandidate;
        *pHeader = candidate;
        *pMediaPrefix = sPrefix;
        *pSourcePath = sSourcePath;
        return true;
    }

    XBinary::setPdStructErrorString(pPdStruct, tr("InstallShield cabinet catalog was not found; DATA1.HDR is required"));
    return false;
}

bool XISCab::_parseCatalog(const QByteArray &baCatalog, const COMMON_HEADER &common, QList<FILE_ENTRY> *pEntries, QList<qint32> *pVisibleIndices,
                           PDSTRUCT *pPdStruct) const
{
    if (pEntries) pEntries->clear();
    if (pVisibleIndices) pVisibleIndices->clear();
    if (!pEntries || !pVisibleIndices || !common.nDescriptorSize || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !isRangeWithin(baCatalog, common.nDescriptorOffset, 0x30)) {
        return false;
    }

    const quint64 nDescriptor = common.nDescriptorOffset;
    const quint32 nFileTableOffset = readLE32(baCatalog, nDescriptor + 0x0c);
    const quint32 nFileTableSize = readLE32(baCatalog, nDescriptor + 0x14);
    const quint32 nFileTableSize2 = readLE32(baCatalog, nDescriptor + 0x18);
    const quint32 nDirectoryCount = readLE32(baCatalog, nDescriptor + 0x1c);
    const quint32 nFileCount = readLE32(baCatalog, nDescriptor + 0x28);
    const quint32 nFileTableOffset2 = readLE32(baCatalog, nDescriptor + 0x2c);
    if ((nDirectoryCount > IS_MAX_RECORDS) || (nFileCount > IS_MAX_RECORDS) || (nDirectoryCount > IS_MAX_RECORDS - nFileCount) || (nFileTableSize != nFileTableSize2)) {
        return false;
    }

    const quint64 nTable = nDescriptor + nFileTableOffset;
    const quint64 nOffsetCount = static_cast<quint64>(nDirectoryCount) + nFileCount;
    if (!isRangeWithin(baCatalog, nTable, nOffsetCount * 4U) || (nFileTableSize && !isRangeWithin(baCatalog, nTable, nFileTableSize))) {
        return false;
    }

    QList<QString> listDirectories;
    listDirectories.reserve(static_cast<qint32>(nDirectoryCount));
    for (quint32 i = 0; i < nDirectoryCount; ++i) {
        const quint32 nStringOffset = readLE32(baCatalog, nTable + i * 4U);
        if (!isRangeWithin(baCatalog, nTable + nStringOffset, 1)) return false;
        listDirectories.append(readCabString(baCatalog, nTable + nStringOffset, common.nMajorVersion));
    }

    pEntries->reserve(static_cast<qint32>(nFileCount));
    QSet<QString> setNames;
    for (quint32 i = 0; i < nFileCount; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        FILE_ENTRY entry;
        quint64 nEntryOffset = 0;
        quint64 nRequiredSize = 0;
        if (common.nMajorVersion <= 5) {
            const quint32 nRelative = readLE32(baCatalog, nTable + static_cast<quint64>(nDirectoryCount + i) * 4U);
            nEntryOffset = nTable + nRelative;
            nRequiredSize = (common.nMajorVersion == 5) ? 0x3a : 0x2a;
            if (!isRangeWithin(baCatalog, nEntryOffset, nRequiredSize)) return false;
            entry.nNameOffset = readLE32(baCatalog, nEntryOffset);
            entry.nDirectoryIndex = readLE16(baCatalog, nEntryOffset + 4);
            entry.nFlags = readLE16(baCatalog, nEntryOffset + 8);
            entry.nExpandedSize = readLE32(baCatalog, nEntryOffset + 10);
            entry.nCompressedSize = readLE32(baCatalog, nEntryOffset + 14);
            entry.nDataOffset = readLE32(baCatalog, nEntryOffset + 38);
            if (common.nMajorVersion == 5) entry.baMD5 = baCatalog.mid(static_cast<qint32>(nEntryOffset + 42), 16);
            entry.nVolume = 1;
        } else {
            nEntryOffset = nTable + nFileTableOffset2 + static_cast<quint64>(i) * 0x57U;
            nRequiredSize = 0x57;
            if (!isRangeWithin(baCatalog, nEntryOffset, nRequiredSize)) return false;
            entry.nFlags = readLE16(baCatalog, nEntryOffset);
            entry.nExpandedSize = readLE64(baCatalog, nEntryOffset + 2);
            entry.nCompressedSize = readLE64(baCatalog, nEntryOffset + 10);
            entry.nDataOffset = readLE64(baCatalog, nEntryOffset + 18);
            entry.baMD5 = baCatalog.mid(static_cast<qint32>(nEntryOffset + 26), 16);
            entry.nNameOffset = readLE32(baCatalog, nEntryOffset + 58);
            entry.nDirectoryIndex = readLE16(baCatalog, nEntryOffset + 62);
            entry.nLinkPrevious = readLE32(baCatalog, nEntryOffset + 76);
            entry.nLinkNext = readLE32(baCatalog, nEntryOffset + 80);
            entry.nLinkFlags = static_cast<quint8>(baCatalog.at(static_cast<qint32>(nEntryOffset + 84)));
            entry.nVolume = readLE16(baCatalog, nEntryOffset + 85);
        }

        const bool bSizeValid = (entry.nExpandedSize <= static_cast<quint64>((std::numeric_limits<qint64>::max)())) &&
                                (entry.nCompressedSize <= static_cast<quint64>((std::numeric_limits<qint64>::max)())) &&
                                (entry.nDataOffset <= static_cast<quint64>((std::numeric_limits<qint64>::max)()));
        const QString sFile = entry.nNameOffset ? readCabString(baCatalog, nTable + entry.nNameOffset, common.nMajorVersion) : QString();
        const bool bDirectoryValid = entry.nDirectoryIndex < listDirectories.count();
        const QString sDirectory = bDirectoryValid ? listDirectories.at(entry.nDirectoryIndex) : QString();
        entry.sFileName = uniqueArchiveName(safeArchiveName(sDirectory, sFile, static_cast<qint32>(i)), &setNames);
        entry.sRawDirectory = sDirectory;
        entry.sRawName = sFile;
        entry.bVisible = bSizeValid && !(entry.nFlags & IS_FILE_INVALID) && entry.nNameOffset && !sFile.isEmpty() && bDirectoryValid && entry.nDataOffset;
        pEntries->append(entry);
        if (entry.bVisible) pVisibleIndices->append(static_cast<qint32>(i));
    }
    return true;
}

bool XISCab::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XISCab> guardedThis(this);
    if (m_bUnpackOperationInProgress) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }

    UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (!bindUnpackSource(pState, pPdStruct) || !guardedThis) return false;

    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    bool bResult = _loadCatalog(pContext, pPdStruct);
    bResult = bResult && guardedThis && _parseCatalog(pContext->baCatalog, pContext->common, &pContext->listEntries, &pContext->listVisibleIndices, pPdStruct);
    const bool bMetadataOnly = mapProperties.value(UNPACK_PROP_METADATAONLY, false).toBool();
    if (bResult && !bMetadataOnly && pContext->mapEmbeddedVolumes.isEmpty()) {
        const QString sVolumePath = mediaPath(pContext->sMediaPrefix, 1, QStringLiteral("cab"), &pContext->mapResolvedMediaPaths);
        VOLUME_HEADER volume = {};
        qint64 nVolumeSize = -1;
        if (sVolumePath.isEmpty() || !parseVolumeFile(sVolumePath, pContext->common.nMajorVersion, &volume, &nVolumeSize)) {
            XBinary::setPdStructErrorString(pPdStruct, tr("InstallShield companion volume is missing; only the cabinet catalog is present"));
            bResult = false;
        }
    }
    if (!guardedThis) {
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    if (!bResult || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        delete pContext;
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < 0)) {
        delete pContext;
        if (guardedThis) releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listVisibleIndices.count();
    pState->nCurrentOffset = 0;
    pState->nTotalSize = nTotalSize;
    pState->mapUnpackProperties = mapProperties;
    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XISCab::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XISCab> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext) {
        return ARCHIVERECORD();
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return ARCHIVERECORD();
    }
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->listVisibleIndices.count()) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    const qint32 nEntryIndex = pContext->listVisibleIndices.at(pState->nCurrentIndex);
    if ((nEntryIndex < 0) || (nEntryIndex >= pContext->listEntries.count())) return ARCHIVERECORD();
    const FILE_ENTRY entry = pContext->listEntries.at(nEntryIndex);

    ARCHIVERECORD result = {};
    result.nStreamOffset = static_cast<qint64>(entry.nDataOffset);
    result.nStreamSize = static_cast<qint64>(entry.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, static_cast<qint64>(entry.nExpandedSize));
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, static_cast<qint64>(entry.nCompressedSize));
    result.mapProperties.insert(FPART_PROP_ENCRYPTED, false);
    result.mapProperties.insert(FPART_PROP_FILEMODE, (quint32)0644);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (entry.baMD5.size() == 16) result.mapProperties.insert(FPART_PROP_FILEMD5, QString::fromLatin1(entry.baMD5.toHex()));
    if (!XBinary::markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();
    return result;
}

bool XISCab::_extractEntry(const UNPACK_CONTEXT *pContext, qint32 nEntryIndex, QIODevice *pStageDevice, UNPACK_STATE *pState, PDSTRUCT *pPdStruct) const
{
    if (!pContext || !pStageDevice || !pState || (nEntryIndex < 0) || (nEntryIndex >= pContext->listEntries.count()) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    qint32 nSourceIndex = nEntryIndex;
    QSet<qint32> setVisited;
    while ((nSourceIndex >= 0) && (nSourceIndex < pContext->listEntries.count())) {
        if (setVisited.contains(nSourceIndex)) return false;
        setVisited.insert(nSourceIndex);
        const FILE_ENTRY &candidate = pContext->listEntries.at(nSourceIndex);
        if (!(candidate.nLinkFlags & IS_LINK_PREV)) break;
        if (candidate.nLinkPrevious >= static_cast<quint32>(pContext->listEntries.count())) {
            return false;
        }
        nSourceIndex = static_cast<qint32>(candidate.nLinkPrevious);
    }
    if ((nSourceIndex < 0) || (nSourceIndex >= pContext->listEntries.count())) return false;
    const FILE_ENTRY &entry = pContext->listEntries.at(nSourceIndex);

    QList<INPUT_SEGMENT> listSegments;
    QByteArray baExternal;
    bool bExternal = false;
    if (!buildSegments(pContext->sMediaPrefix, pContext->sContainerPath, pContext->mapEmbeddedVolumes, &pContext->mapResolvedMediaPaths,
                       pContext->common.nMajorVersion, entry.nFlags,
                       entry.nExpandedSize, entry.nCompressedSize, entry.nDataOffset, entry.nVolume, nSourceIndex, pContext->listEntries.count(), &listSegments)) {
        // Distinguish the two common non-decodable layouts so a missing
        // companion file does not read like a parser defect: a detached
        // catalog (.hdr) references dataN.cab volumes that are not present,
        // and "external" media store the payload as loose files, marking
        // entries with a data offset at (or past) the volume end.
        QString sError = tr("InstallShield cabinet volume is missing or invalid");
        if (pContext->mapEmbeddedVolumes.isEmpty()) {
            const quint32 nStartVolume = (pContext->common.nMajorVersion <= 5) ? 1U : qMax<quint32>(1U, entry.nVolume);
            const QString sVolumePath = mediaPath(pContext->sMediaPrefix, nStartVolume, QStringLiteral("cab"), &pContext->mapResolvedMediaPaths);
            if (sVolumePath.isEmpty()) {
                sError = tr("InstallShield companion volume is missing; only the cabinet catalog is present");
            } else {
                const qint64 nVolumeSize = QFileInfo(sVolumePath).size();
                if ((nVolumeSize > 0) && (entry.nDataOffset >= static_cast<quint64>(nVolumeSize))) {
                    // unshield external-media parity: the sentinel
                    // data_offset == volume size means the payload is a loose
                    // file on the media, at <media dir>/<directory>/<name>.
                    // The compressed variant may lack its final chunk marker;
                    // the reference completes it with END_OF_CHUNK tail bytes.
                    const bool bCompressedEntry = entry.nFlags & IS_FILE_COMPRESSED;
                    const quint64 nBudget = bCompressedEntry ? entry.nCompressedSize : entry.nExpandedSize;
                    if ((entry.nDataOffset == static_cast<quint64>(nVolumeSize)) && nBudget && (nBudget <= static_cast<quint64>(IS_MAX_CATALOG_SIZE))) {
                        const QString sMediaDirectory = QFileInfo(sVolumePath).absolutePath();
                        // The loose path uses the RAW catalog directory/name:
                        // duplicate catalog entries all reference the same
                        // loose file, while sFileName is uniquified per entry.
                        const QString sLooseRelative = safeCombinedName(entry.sRawDirectory, entry.sRawName);
                        const QString sLoosePath = sLooseRelative.isEmpty() ? QString() : resolveCaseInsensitivePath(QDir(sMediaDirectory).filePath(sLooseRelative));
                        if (!sLoosePath.isEmpty()) {
                            QFile looseFile(sLoosePath);
                            const qint64 nLooseSize = looseFile.size();
                            // A larger loose file than the recorded size is
                            // legal (the reference reads only the recorded
                            // byte count); truncate instead of rejecting.
                            const qint64 nReadSize = qMin<qint64>(nLooseSize, static_cast<qint64>(nBudget));
                            if ((nReadSize > 0) && looseFile.open(QIODevice::ReadOnly)) {
                                baExternal = looseFile.read(nReadSize);
                                looseFile.close();
                                if (baExternal.size() == nReadSize) {
                                    if (bCompressedEntry && (static_cast<quint64>(baExternal.size()) < entry.nCompressedSize)) {
                                        const qint64 nMissing = qMin<qint64>(4, static_cast<qint64>(entry.nCompressedSize) - baExternal.size());
                                        baExternal.append(QByteArray("\x00\x00\xff\xff", 4).right(static_cast<qint32>(nMissing)));
                                    }
                                    bExternal = true;
                                } else {
                                    baExternal.clear();
                                }
                            }
                        }
                    }
                    if (!bExternal) sError = tr("InstallShield member data is stored outside the cabinet (external media file)");
                }
            }
        }
        if (!bExternal) {
            XBinary::setPdStructErrorString(pPdStruct, sError);
            return false;
        }
    }
    SegmentReader reader(listSegments);
    const bool bObfuscated = entry.nFlags & IS_FILE_OBFUSCATED;
    const bool bCompressed = entry.nFlags & IS_FILE_COMPRESSED;
    quint64 nSeed = 0;
    quint64 nWritten = 0;
    QCryptographicHash hash(QCryptographicHash::Md5);
    const ISCabChunkPublisher chunkPublisher(pStageDevice, pState, pPdStruct, entry.nExpandedSize, &hash, &nWritten,
                                             tr("Unpacked output exceeds the configured limit"));

    // The version-5 descriptor flag 0x2 is not the byte cipher: corpus media
    // with that bit set decode as plain streams, and the cipher was introduced
    // with the version-6 "obfuscated media" option.  Applying it to v5 data
    // corrupts the stream, so it is gated on the catalog generation.
    const bool bApplyCipher = bObfuscated && (pContext->common.nMajorVersion >= 6);

    if (!bCompressed) {
        if (bExternal) {
            // Loose media file: the bytes are the member's plain content.
            if (static_cast<quint64>(baExternal.size()) != entry.nExpandedSize) return false;
            if (bApplyCipher) deobfuscate(baExternal.data(), baExternal.size(), &nSeed);
            qint32 nOffset = 0;
            while (nOffset < baExternal.size()) {
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
                const qint32 nChunk = qMin<qint32>(64 * 1024, baExternal.size() - nOffset);
                QByteArray baOutput = baExternal.mid(nOffset, nChunk);
                if (!chunkPublisher.publish(&baOutput)) return false;
                nOffset += nChunk;
            }
        } else {
            quint64 nRemaining = entry.nExpandedSize;
            QByteArray baBuffer(64 * 1024, '\0');
            while (nRemaining) {
                const qint64 nChunk = static_cast<qint64>(qMin<quint64>(nRemaining, (quint64)baBuffer.size()));
                if (!reader.readExact(baBuffer.data(), nChunk)) return false;
                if (bApplyCipher) deobfuscate(baBuffer.data(), nChunk, &nSeed);
                QByteArray baOutput = baBuffer.left(static_cast<qint32>(nChunk));
                if (!chunkPublisher.publish(&baOutput)) return false;
                nRemaining -= static_cast<quint64>(nChunk);
            }
        }
    } else {
        // InstallShield media use one of two compressed framings that share a
        // version dword and can only be told apart by walking the data: NEW
        // (u16 length + raw-deflate blocks) and OLD (raw-deflate chunks
        // separated by 00 00 ff ff sync-flush markers).  Buffer the stream
        // once, then decode with whichever framing validates.
        if (entry.nCompressedSize > static_cast<quint64>(IS_MAX_CATALOG_SIZE)) {
            // Members beyond the buffering cap stream with NEW framing (the
            // reference streams in 64 KiB windows and never whole-buffers);
            // old-style framing needs the buffered marker scan and is not
            // attempted at this scale.
            quint64 nRemaining = entry.nCompressedSize;
            while (nRemaining) {
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
                if (nRemaining < 2) return false;
                char szLength[2];
                if (!reader.readExact(szLength, 2)) return false;
                if (bApplyCipher) deobfuscate(szLength, 2, &nSeed);
                nRemaining -= 2;
                const quint16 nBlockSize = static_cast<quint8>(szLength[0]) | (static_cast<quint16>(static_cast<quint8>(szLength[1])) << 8);
                if (!nBlockSize || (nBlockSize > nRemaining)) {
                    XBinary::setPdStructErrorString(pPdStruct, tr("InstallShield cabinet deflate block is invalid"));
                    return false;
                }
                QByteArray baBlock(nBlockSize, '\0');
                if (!reader.readExact(baBlock.data(), nBlockSize)) return false;
                if (bApplyCipher) deobfuscate(baBlock.data(), nBlockSize, &nSeed);
                nRemaining -= nBlockSize;
                QByteArray baOutput;
                if (!inflateInstallShieldBlock(baBlock, &baOutput) || !chunkPublisher.publish(&baOutput)) {
                    XBinary::setPdStructErrorString(pPdStruct, tr("InstallShield cabinet deflate block is invalid"));
                    return false;
                }
            }
            if (nWritten != entry.nExpandedSize) return false;
            if ((pContext->common.nMajorVersion >= 6) && (entry.baMD5.size() == 16) && (hash.result() != entry.baMD5)) {
                XBinary::setPdStructErrorString(pPdStruct, tr("InstallShield cabinet member checksum mismatch"));
                return false;
            }
            return true;
        }
        QByteArray baCompressed;
        if (bExternal) {
            // Loose media file carrying the compressed stream (possibly
            // completed with END_OF_CHUNK tail bytes above); treat it as one
            // volume window for the old-style chunk scan.
            baCompressed = baExternal;
            INPUT_SEGMENT externalSegment;
            externalSegment.nOffset = 0;
            externalSegment.nSize = static_cast<quint64>(baCompressed.size());
            listSegments = QList<INPUT_SEGMENT>() << externalSegment;
        } else {
            baCompressed.resize(static_cast<qint32>(entry.nCompressedSize));
            if (!reader.readExact(baCompressed.data(), baCompressed.size())) return false;
        }
        if (bApplyCipher) deobfuscate(baCompressed.data(), baCompressed.size(), &nSeed);

        if (isNewStyleBlockChain(baCompressed)) {
            quint64 nOffset = 0;
            const quint64 nSize = static_cast<quint64>(baCompressed.size());
            while (nOffset < nSize) {
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
                const quint16 nBlockSize = readLE16(baCompressed, nOffset);
                nOffset += 2;
                const QByteArray baBlock = baCompressed.mid(static_cast<qint32>(nOffset), nBlockSize);
                nOffset += nBlockSize;
                QByteArray baOutput;
                if (!inflateInstallShieldBlock(baBlock, &baOutput) || !chunkPublisher.publish(&baOutput)) {
                    XBinary::setPdStructErrorString(pPdStruct, tr("InstallShield cabinet deflate block is invalid"));
                    return false;
                }
            }
        } else {
            // Old-style framing: chunks never span volumes, so scan each
            // volume's portion of the stream separately (the reference reads
            // one volume at a time).
            quint64 nSegmentBase = 0;
            for (qint32 nSegmentIndex = 0; nSegmentIndex < listSegments.count(); ++nSegmentIndex) {
                const quint64 nSegmentSize = listSegments.at(nSegmentIndex).nSize;
                quint64 nChunkStart = nSegmentBase;
                const quint64 nSegmentEnd = nSegmentBase + nSegmentSize;
                while ((nChunkStart < nSegmentEnd) && (nWritten < entry.nExpandedSize)) {
                    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
                    // Locate the chunk's 00 00 ff ff terminator, skipping
                    // false markers whose following byte starts a block with
                    // the reserved first bit set (reference workaround).
                    qint64 nMarker = -1;
                    quint64 nSearchFrom = nChunkStart;
                    while (true) {
                        const int nFound = baCompressed.indexOf(QByteArray("\x00\x00\xff\xff", 4), static_cast<qint32>(nSearchFrom));
                        if ((nFound < 0) || ((static_cast<quint64>(nFound) + 4) > nSegmentEnd)) break;
                        if (((static_cast<quint64>(nFound) + 4) < nSegmentEnd) && (baCompressed.at(nFound + 4) & 1)) {
                            nSearchFrom = static_cast<quint64>(nFound) + 4;
                            continue;
                        }
                        nMarker = nFound;
                        break;
                    }
                    if ((nMarker < 0) || (static_cast<quint64>(nMarker) < nChunkStart)) {
                        XBinary::setPdStructErrorString(pPdStruct, tr("InstallShield cabinet chunk terminator was not found"));
                        return false;
                    }
                    const qint32 nChunkSize = static_cast<qint32>(static_cast<quint64>(nMarker) - nChunkStart);
                    if (!nChunkSize) {
                        // Back-to-back markers: an empty sync-flush fragment.
                        // The reference inflates zero bytes to zero output and
                        // moves on; skip the marker and continue.
                        nChunkStart = static_cast<quint64>(nMarker) + 4;
                        continue;
                    }
                    QByteArray baOutput;
                    if (!inflateInstallShieldOldChunk(baCompressed.constData() + nChunkStart, nChunkSize, &baOutput) || !chunkPublisher.publish(&baOutput)) {
                        XBinary::setPdStructErrorString(pPdStruct, tr("InstallShield cabinet deflate chunk is invalid"));
                        return false;
                    }
                    nChunkStart = static_cast<quint64>(nMarker) + 4;
                }
                nSegmentBase = nSegmentEnd;
            }
        }
    }

    if (nWritten != entry.nExpandedSize) return false;
    if ((pContext->common.nMajorVersion >= 6) && (entry.baMD5.size() == 16) && (hash.result() != entry.baMD5)) {
        XBinary::setPdStructErrorString(pPdStruct, tr("InstallShield cabinet member checksum mismatch"));
        return false;
    }
    return true;
}

bool XISCab::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XISCab> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !guardedOutput) {
        return false;
    }
    const bool bOutputSupported = isUnpackOutputSupported(guardedOutput.data());
    if (!guardedThis || !guardedOutput || !bOutputSupported) return false;
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !guardedOutput || !bSourceCurrent || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (pState->nNumberOfRecords != pContext->listVisibleIndices.count()) return false;
    const qint32 nEntryIndex = pContext->listVisibleIndices.at(pState->nCurrentIndex);
    const FILE_ENTRY entry = pContext->listEntries.at(nEntryIndex);

    qint32 nSourceIndex = nEntryIndex;
    QSet<qint32> setVisited;
    while ((nSourceIndex >= 0) && (nSourceIndex < pContext->listEntries.count())) {
        if (setVisited.contains(nSourceIndex)) return false;
        setVisited.insert(nSourceIndex);
        const FILE_ENTRY &candidate = pContext->listEntries.at(nSourceIndex);
        if (!(candidate.nLinkFlags & IS_LINK_PREV)) break;
        if (candidate.nLinkPrevious >= static_cast<quint32>(pContext->listEntries.count())) {
            return false;
        }
        nSourceIndex = static_cast<qint32>(candidate.nLinkPrevious);
    }
    if ((nSourceIndex < 0) || (nSourceIndex >= pContext->listEntries.count())) return false;
    const quint64 nOutputSize = pContext->listEntries.at(nSourceIndex).nExpandedSize;

    qint64 nOutputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(pState->mapUnpackProperties, &nOutputLimit) || ((nOutputLimit >= 0) && (nOutputSize > static_cast<quint64>(nOutputLimit)))) {
        XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
        return false;
    }
    if (pState->spOutputBudget && !pState->spOutputBudget->beginEntry(pState->nCurrentIndex, entry.sFileName)) {
        if (pState->spOutputBudget->isEnforcing()) {
            XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
            return false;
        }
        XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
    }

    QTemporaryFile stage;
    if (!stage.open()) return false;
    bool bResult = _extractEntry(pContext, nEntryIndex, &stage, pState, pPdStruct);
    bResult = bResult && guardedThis && guardedOutput && isUnpackSourceCurrent(pState, pPdStruct) && guardedThis;
    if (bResult) bResult = publishUnpackOutput(&stage, guardedOutput.data(), pState, pPdStruct);
    return bResult;
}

bool XISCab::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XISCab> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext) {
        return false;
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->listVisibleIndices.count()) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    ++pState->nCurrentIndex;
    pState->nCurrentOffset = pState->nCurrentIndex;
    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool XISCab::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
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

QList<XBinary::FPART_PROP> XISCab::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_FILEMD5 << FPART_PROP_ENCRYPTED
                               << FPART_PROP_FILEMODE;
}
