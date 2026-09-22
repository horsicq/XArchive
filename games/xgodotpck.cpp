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
#include "xgodotpck.h"

#include <QtEndian>

#include <cstring>
#include <limits>

namespace {
constexpr qint64 GODOT_PCK_V1_HEADER_SIZE = 84;
constexpr qint64 GODOT_PCK_V2_HEADER_SIZE = 96;
constexpr qint64 GODOT_PCK_FILE_COUNT_SIZE = 4;
constexpr qint64 GODOT_PCK_V1_ENTRY_TAIL_SIZE = 32;
constexpr qint64 GODOT_PCK_V2_ENTRY_TAIL_SIZE = 36;
constexpr qint64 GODOT_PCK_MAX_NAME_SIZE = 1024 * 1024;

constexpr quint32 GODOT_PCK_DIRECTORY_ENCRYPTED = 0x00000001U;
constexpr quint32 GODOT_PCK_RELATIVE_FILE_BASE = 0x00000002U;
constexpr quint32 GODOT_PCK_SUPPORTED_HEADER_FLAGS =
    GODOT_PCK_DIRECTORY_ENCRYPTED | GODOT_PCK_RELATIVE_FILE_BASE;

quint64 readLE64(const uchar *pData)
{
    return qFromLittleEndian<quint64>(pData);
}

bool toSignedValue(quint64 nValue, qint64 *pResult)
{
    if (!pResult ||
        (nValue > static_cast<quint64>((std::numeric_limits<qint64>::max)()))) {
        return false;
    }
    *pResult = static_cast<qint64>(nValue);
    return true;
}

bool addOffsets(qint64 nOffset1, qint64 nOffset2, qint64 *pResult)
{
    if (!pResult || (nOffset1 < 0) || (nOffset2 < 0) ||
        (nOffset1 > ((std::numeric_limits<qint64>::max)() - nOffset2))) {
        return false;
    }
    *pResult = nOffset1 + nOffset2;
    return true;
}

bool decodePCKPath(const uchar *pData, qint32 nSize, QString *pResult)
{
    if (!pData || !pResult || (nSize <= 0)) return false;

    QByteArray baName(reinterpret_cast<const char *>(pData), nSize);
    // Godot 3 pads each Pascal-style UTF-8 name to a four-byte boundary.  The
    // length includes that padding, so only trailing NUL bytes are ignored.
    while (!baName.isEmpty() && baName.endsWith('\0')) baName.chop(1);
    if (baName.isEmpty()) return false;

    QString sName = QString::fromUtf8(baName.constData(), baName.size());
    if (sName.toUtf8() != baName) return false;

    for (qint32 i = 0; i < sName.size(); ++i) {
        const ushort nCharacter = sName.at(i).unicode();
        if ((nCharacter < 0x20) ||
            ((nCharacter >= 0x7f) && (nCharacter <= 0x9f))) {
            return false;
        }
    }

    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (sName.startsWith(QLatin1String("res://"))) sName.remove(0, 6);
    if (sName.isEmpty() || sName.startsWith(QLatin1Char('/'))) return false;

    const QStringList listParts =
        sName.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) ||
            (sPart == QLatin1String(".."))) {
            return false;
        }
    }

    sName = sName.normalized(QString::NormalizationForm_C);
    if (XBinary::fixFileName(sName) != sName) return false;

    *pResult = sName;
    return true;
}
}  // namespace

XGodotPCK::XGodotPCK(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_GODOT_PCK)
{
}

bool XGodotPCK::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XGodotPCK archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XGodotPCK::createInstance(QIODevice *pDevice, bool bIsImage,
                                   XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XGodotPCK(pDevice);
}

bool XGodotPCK::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                           PDSTRUCT *pPdStruct)
{
    const qint64 nTotalSize = getSize();
    if ((nTotalSize < 8) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const QByteArray baPrefix = read_array_process(0, 8, pPdStruct);
    if ((baPrefix.size() != 8) ||
        (std::memcmp(baPrefix.constData(), "GDPC", 4) != 0)) {
        return false;
    }

    const uchar *pPrefix = reinterpret_cast<const uchar *>(
        baPrefix.constData());
    const quint32 nFormatVersion = readLE32(pPrefix + 4);
    if ((nFormatVersion != 1U) && (nFormatVersion != 2U)) return false;

    const qint64 nHeaderSize = (nFormatVersion == 1U)
        ? GODOT_PCK_V1_HEADER_SIZE : GODOT_PCK_V2_HEADER_SIZE;
    const qint64 nDirectoryStart = nHeaderSize + GODOT_PCK_FILE_COUNT_SIZE;
    if (nTotalSize < nDirectoryStart) return false;

    const QByteArray baHeader = read_array_process(
        0, nDirectoryStart, pPdStruct);
    if ((baHeader.size() != nDirectoryStart) ||
        (std::memcmp(baHeader.constData(), "GDPC", 4) != 0)) {
        return false;
    }
    const uchar *pHeader = reinterpret_cast<const uchar *>(
        baHeader.constData());
    if (readLE32(pHeader + 8) == 0) return false;

    const qint64 nReservedOffset = (nFormatVersion == 1U) ? 20 : 32;
    for (qint32 i = 0; i < 16; ++i) {
        if (readLE32(pHeader + nReservedOffset + ((qint64)i * 4)) != 0) {
            return false;
        }
    }

    qint64 nFileBase = 0;
    if (nFormatVersion == 2U) {
        const quint32 nHeaderFlags = readLE32(pHeader + 20);
        if ((nHeaderFlags & ~GODOT_PCK_SUPPORTED_HEADER_FLAGS) ||
            (nHeaderFlags & GODOT_PCK_DIRECTORY_ENCRYPTED) ||
            !toSignedValue(readLE64(pHeader + 24), &nFileBase) ||
            !rangeWithin(nTotalSize, nFileBase, 0)) {
            return false;
        }
    }

    const quint32 nRecordCountValue = readLE32(pHeader + nHeaderSize);
    if ((nRecordCountValue == 0) ||
        (nRecordCountValue > static_cast<quint32>(MAX_RECORDS))) {
        return false;
    }
    const qint32 nRecordCount = static_cast<qint32>(nRecordCountValue);
    const qint64 nEntryTailSize = (nFormatVersion == 1U)
        ? GODOT_PCK_V1_ENTRY_TAIL_SIZE : GODOT_PCK_V2_ENTRY_TAIL_SIZE;
    const qint64 nMinimumEntrySize = 4 + nEntryTailSize;
    if (nRecordCount > ((nTotalSize - nDirectoryStart) / nMinimumEntrySize)) {
        return false;
    }

    QSet<QString> stUsedFiles;
    QSet<QString> stUsedDirectories;
    QHash<QString, qint32> mapNextSuffixes;
    QHash<QString, QString> mapResolvedDirectories;
    qint64 nDirectoryOffset = nDirectoryStart;
    qint64 nMinimumDataOffset = nTotalSize;
    qint64 nArchiveEnd = nDirectoryStart;

    for (qint32 i = 0; i < nRecordCount; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) ||
            !rangeWithin(nTotalSize, nDirectoryOffset, 4)) {
            return false;
        }

        const QByteArray baNameLength = read_array_process(
            nDirectoryOffset, 4, pPdStruct);
        if ((baNameLength.size() != 4)) return false;
        const quint32 nNameLengthValue = readLE32(
            reinterpret_cast<const uchar *>(baNameLength.constData()));
        if ((nNameLengthValue == 0) ||
            (nNameLengthValue > static_cast<quint32>(GODOT_PCK_MAX_NAME_SIZE))) {
            return false;
        }
        const qint32 nNameLength = static_cast<qint32>(nNameLengthValue);
        const qint64 nEntrySize = 4 + nNameLength + nEntryTailSize;
        if (!rangeWithin(nTotalSize, nDirectoryOffset, nEntrySize)) {
            return false;
        }

        const QByteArray baEntry = read_array_process(
            nDirectoryOffset + 4, nEntrySize - 4, pPdStruct);
        if ((baEntry.size() != (nEntrySize - 4))) {
            return false;
        }
        const uchar *pEntry = reinterpret_cast<const uchar *>(
            baEntry.constData());
        const uchar *pTail = pEntry + nNameLength;

        QString sName;
        QString sUniqueName;
        if (!decodePCKPath(pEntry, nNameLength, &sName) ||
            !makeUniquePath(sName, &stUsedFiles, &stUsedDirectories,
                            &mapNextSuffixes, &mapResolvedDirectories,
                            &sUniqueName)) {
            return false;
        }

        qint64 nEntryOffset = 0;
        qint64 nDataSize = 0;
        if (!toSignedValue(readLE64(pTail), &nEntryOffset) ||
            !toSignedValue(readLE64(pTail + 8), &nDataSize)) {
            return false;
        }
        qint64 nDataOffset = nEntryOffset;
        // Godot v2 always adds file_base to each member offset.  The relative
        // file-base flag controls whether file_base itself is relative to the
        // enclosing PCK start (which is zero for this standalone stream).
        if ((nFormatVersion == 2U) &&
            !addOffsets(nFileBase, nEntryOffset, &nDataOffset)) {
            return false;
        }
        // In version 2, an encrypted member has a larger stored byte stream
        // than its directory size.  This store-only reader cannot represent
        // that stream faithfully, so recognize only directly stored members.
        if ((nFormatVersion == 2U) && (readLE32(pTail + 32) != 0)) {
            return false;
        }
        if (!rangeWithin(nTotalSize, nDataOffset, nDataSize)) return false;

        nMinimumDataOffset = qMin(nMinimumDataOffset, nDataOffset);
        nArchiveEnd = qMax(nArchiveEnd, nDataOffset + nDataSize);
        if (pEntries) {
            ENTRY entry = {};
            entry.nHeaderOffset = nDirectoryOffset;
            entry.nHeaderSize = nEntrySize;
            entry.nDataOffset = nDataOffset;
            entry.nDataSize = nDataSize;
            entry.sChecksum = QString::fromLatin1(
                QByteArray(reinterpret_cast<const char *>(pTail + 16), 16)
                    .toHex());
            entry.sChecksumType = QStringLiteral("MD5");
            entry.sFileName = sUniqueName;
            pEntries->append(entry);
        }
        nDirectoryOffset += nEntrySize;
    }

    // The variable-length directory is always stored before the raw data.
    // Checking it after the final record catches offsets into later directory
    // records without assuming a particular file ordering.
    if ((nMinimumDataOffset < nDirectoryOffset) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    nArchiveEnd = qMax(nArchiveEnd, nDirectoryOffset);
    if (pArchiveEnd) *pArchiveEnd = nArchiveEnd;
    return true;
}
