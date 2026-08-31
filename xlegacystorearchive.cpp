/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xlegacystorearchive.h"
#include "xdecompress.h"
#include "Algos/xdcldecoder.h"
#include "Algos/xpaxdecoder.h"
#include "Algos/xvisedeflatedecoder.h"

#include <QtEndian>
#include <QHash>
#include <QPointer>
#include <QSet>
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
#include <QTextCodec>  // Qt5 Compat; removed from Qt6 core
#endif
#include <QVector>

#include <algorithm>
#include <limits>
#include <cstring>

namespace {
const qint64 MAX_LEGACY_STORE_SIZE = Q_INT64_C(512) * 1024 * 1024;
const qint32 MAX_GPFPACK_BLOCKS = 65536;
constexpr quint8 IS_SKIN_XOR_KEY[] = {
    0x2a, 0x58, 0x95, 0xcb, 0x3a, 0xf9, 0xb3, 0xca
};

quint8 decodeIsSkinByte(quint8 value, qint64 absoluteOffset)
{
    const quint8 decoded = value ^
        IS_SKIN_XOR_KEY[quint64(absoluteOffset) & 7U];
    return quint8((decoded >> 4U) | (decoded << 4U));
}

bool looksLikeIsSkin(const QByteArray &header)
{
    // qint32(): QByteArray::size() is qsizetype on Qt6, so qMin() cannot deduce
    // a single T from it and the int literal 80.
    const qint32 nameLimit = qMin(qint32(header.size()), 80);
    qint32 pos = 0;
    while (pos < nameLimit) {
        const quint8 value = decodeIsSkinByte(quint8(header.at(pos)), pos);
        if (!value) break;
        if (value < 0x20U || value > 0x7eU) return false;
        ++pos;
    }
    if (pos < 1 || pos >= nameLimit) return false;
    ++pos;

    qint32 digits = 0;
    while (pos < header.size()) {
        const quint8 value = decodeIsSkinByte(quint8(header.at(pos)), pos);
        if (!value) return digits >= 1;
        if (digits >= 10 || value < '0' || value > '9') return false;
        ++digits;
        ++pos;
    }
    return false;
}

bool isLegacyStoreType(XBinary::FT ft)
{
    return ft == XBinary::FT_LEGACY_CAT ||
           ft == XBinary::FT_KA_ARCHIVE ||
           ft == XBinary::FT_MLB_ARCHIVE ||
           ft == XBinary::FT_LEGACY_RES ||
           ft == XBinary::FT_LEGACY_RSC ||
           ft == XBinary::FT_SHRINKWRAP_IMAGE ||
           ft == XBinary::FT_LPAK ||
           ft == XBinary::FT_INSTALLSHIELD_BOOT ||
           ft == XBinary::FT_SABDU_IMAGE ||
           ft == XBinary::FT_COMPAQ_LZH ||
           ft == XBinary::FT_INSA ||
           ft == XBinary::FT_EPFS_ARCHIVE ||
           ft == XBinary::FT_STUNTS_DSI ||
           ft == XBinary::FT_FINSTALL_ARCHIVE ||
           ft == XBinary::FT_IS_STORED ||
           ft == XBinary::FT_INSTALLSHIELD3_ARCHIVE ||
           ft == XBinary::FT_EMT_IMAGE ||
           ft == XBinary::FT_GPFPACK ||
           ft == XBinary::FT_PAX ||
           ft == XBinary::FT_SCF ||
           ft == XBinary::FT_SOLITAIRE_DELUXE ||
           ft == XBinary::FT_INSTALIT_DATA ||
           ft == XBinary::FT_ARCV ||
           ft == XBinary::FT_PIMP_SFX ||
           ft == XBinary::FT_VISE_SFX ||
           ft == XBinary::FT_FTCOMP ||
           ft == XBinary::FT_DN_ARCHIVE ||
           ft == XBinary::FT_FPAK ||
           ft == XBinary::FT_SOFTPAQ1_SFX ||
           ft == XBinary::FT_INSTALIT_SFX ||
           ft == XBinary::FT_LIF_COMPRESSED ||
           ft == XBinary::FT_JASC_ARCHIVE ||
           ft == XBinary::FT_SSM_MODULE ||
           ft == XBinary::FT_SSBOB ||
           ft == XBinary::FT_IS_SKIN ||
           ft == XBinary::FT_GPINSTALL_SFX ||
           ft == XBinary::FT_INSTALLSHIELD_LAUNCHER;
}

QString fixedName(const uchar *p, qint32 n)
{
    if (!p || n <= 0) return QString();
    qint32 len = 0;
    while (len < n && p[len]) ++len;
    if (!len) return QString();
    for (qint32 i = 0; i < len; ++i) {
        if (p[i] < 0x20 || p[i] == 0x7f) return QString();
    }
    QString s = QString::fromLatin1(reinterpret_cast<const char *>(p), len);
    s.replace(QLatin1Char('\\'), QLatin1Char('/'));
    s.replace(QLatin1Char(':'), QLatin1Char('_'));
    s = XBinary::fixFileName(s.normalized(QString::NormalizationForm_C));
    if (s.isEmpty() || s.startsWith(QLatin1Char('/')) ||
        s.contains(QStringLiteral("../")) || s == QLatin1String(".."))
        return QString();
    return s;
}

bool isJascRecordHeader(const uchar *pData, qint64 nAvailable,
                        qint32 nMaximumNameSize)
{
    if (!pData || nAvailable < 18 || nMaximumNameSize < 1) return false;

    const qint32 nNameSize = pData[16];
    const qint32 nHeaderSize = 17 + nNameSize;
    if (nNameSize < 1 || nNameSize > nMaximumNameSize ||
        nHeaderSize > nAvailable ||
        pData[0] != static_cast<quint8>(0x0f + nNameSize)) {
        return false;
    }

    quint32 nChecksum = 0;
    bool bHasExtensionSeparator = false;
    for (qint32 i = 2; i < nHeaderSize; ++i) {
        nChecksum = (nChecksum + pData[i]) & 0xffU;
        if (i >= 17) {
            if (pData[i] < 0x20 || pData[i] > 0x7e) return false;
            bHasExtensionSeparator = bHasExtensionSeparator ||
                                     (pData[i] == '.');
        }
    }

    return (pData[1] == nChecksum) && bHasExtensionSeparator;
}

QString macName(const uchar *p, qint32 n)
{
    if (!p || n <= 0) return QString();
    // Qt6 dropped QTextCodec from QtCore, and this build does not link
    // Core5Compat. Fall back to the same Latin-1 decoding the Qt5 path already
    // uses when the "macintosh" codec is unavailable.
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
    QTextCodec *codec = QTextCodec::codecForName("macintosh");
    QString s = codec ? codec->toUnicode(
                            reinterpret_cast<const char *>(p), n)
                      : QString::fromLatin1(
                            reinterpret_cast<const char *>(p), n);
#else
    QString s = QString::fromLatin1(reinterpret_cast<const char *>(p), n);
#endif
    s.replace(QLatin1Char('/'), QLatin1Char('_'));
    s.replace(QLatin1Char(':'), QLatin1Char('_'));
    return XBinary::fixFileName(
        s.normalized(QString::NormalizationForm_C).trimmed());
}

bool decodeEmtRecord(const QByteArray &data, qint64 *position,
                     qint32 outputSize, QByteArray *output)
{
    if (!position || outputSize < 0 || *position < 0 ||
        *position >= data.size()) return false;
    QByteArray result;
    result.reserve(outputSize);
    qint64 pos = *position;
    while (result.size() < outputSize) {
        if (pos >= data.size()) return false;
        const quint8 value = quint8(data.at(pos++));
        if (value != 0xf1U) {
            result.append(char(value));
            continue;
        }
        if (pos > data.size() - 2) return false;
        const quint8 repeated = quint8(data.at(pos++));
        const quint8 count = quint8(data.at(pos++));
        if (count > outputSize - result.size()) return false;
        if (count) result.append(QByteArray(count, char(repeated)));
    }
    *position = pos;
    if (output) *output = result;
    return true;
}

bool readGpfPackCode(const uchar *data, quint32 bitCount,
                     quint32 codeBits, quint32 *pBitPosition,
                     quint32 *pCode)
{
    if (!data || !pBitPosition || !pCode || !codeBits ||
        (*pBitPosition > bitCount) ||
        (codeBits > bitCount - *pBitPosition)) {
        return false;
    }
    quint32 value = 0;
    for (quint32 i = 0; i < codeBits; ++i) {
        const quint32 bit = *pBitPosition + i;
        value = (value << 1) |
                ((data[bit >> 3] >> (7U - (bit & 7U))) & 1U);
    }
    *pBitPosition += codeBits;
    *pCode = value;
    return true;
}

bool readPaxBits(const QByteArray &data, const uchar *pData,
                 qint64 streamOffset, quint32 count,
                 quint32 *pBitPosition, quint32 *pValue)
{
    if (!pData || !pBitPosition || !pValue || (streamOffset < 0) ||
        (count > 24) ||
        (streamOffset > data.size()) ||
        (qint64(*pBitPosition) >
         qint64(data.size() - streamOffset) * 8 - count)) {
        return false;
    }
    quint32 result = 0;
    for (quint32 i = 0; i < count; ++i) {
        const quint32 bit = (*pBitPosition)++;
        result = (result << 1) |
                 ((pData[streamOffset + (bit >> 3)] >>
                   (7U - (bit & 7U))) & 1U);
    }
    *pValue = result;
    return true;
}

bool readZooLzdCode(const uchar *data, qint64 size, qint32 width,
                    qint64 *pBitPosition, quint32 *pValue)
{
    if (!data || !pBitPosition || !pValue || (size < 0) ||
        (width <= 0) || (*pBitPosition < 0) ||
        (*pBitPosition > size * 8 - width)) {
        return false;
    }
    quint32 result = 0;
    for (qint32 i = 0; i < width; ++i) {
        const qint64 bit = *pBitPosition + i;
        result |= quint32((data[bit >> 3] >> (bit & 7)) & 1U) << i;
    }
    *pBitPosition += width;
    *pValue = result;
    return true;
}

bool decodeGpfPackBlock(const uchar *data, qint64 byteCount,
                        quint32 bitCount, QByteArray *output)
{
    if (!data || !output || !bitCount || byteCount < 1 ||
        qint64(bitCount) > byteCount * 8) return false;
    QVector<quint16> prefix(4096, 0);
    QByteArray suffix(4096, 0);
    QByteArray stack(8192, 0);
    QByteArray result;
    result.reserve(8192);
    quint32 bitPosition = 0;
    quint32 codeBits = 9;
    quint32 nextCode = 258;
    quint32 previousCode = 0;
    quint8 previousFirst = 0;
    bool havePrevious = false;

    while (bitPosition < bitCount) {
        quint32 code = 0;
        if (!readGpfPackCode(data, bitCount, codeBits, &bitPosition,
                             &code)) {
            return false;
        }
        if (code == 256) {
            codeBits = 9;
            nextCode = 258;
            havePrevious = false;
            continue;
        }
        // Code 257 is reserved; exact compressed bit counts delimit blocks.
        if (code == 257 || (!havePrevious && code > 0xffU)) return false;

        quint32 stackSize = 0;
        quint32 current = code;
        if (havePrevious && current == nextCode) {
            stack[stackSize++] = char(previousFirst);
            current = previousCode;
        } else if (current >= nextCode) {
            return false;
        }
        while (current > 0xffU) {
            if (current >= nextCode || current >= 4096 ||
                stackSize >= quint32(stack.size())) return false;
            stack[qint32(stackSize++)] = suffix.at(qint32(current));
            current = prefix.at(qint32(current));
        }
        if (stackSize >= quint32(stack.size())) return false;
        stack[qint32(stackSize++)] = char(current);
        const quint8 firstCharacter = quint8(current);
        if (stackSize > quint32(8192 - result.size())) return false;
        while (stackSize) result.append(stack.at(qint32(--stackSize)));

        if (havePrevious) {
            if (nextCode >= 4092) return false;
            prefix[qint32(nextCode)] = quint16(previousCode);
            suffix[qint32(nextCode)] = char(firstCharacter);
            ++nextCode;
            // GPFPACK changes width one code earlier than conventional LZW.
            if (codeBits < 12 && nextCode >= ((1U << codeBits) - 1U))
                ++codeBits;
        }
        previousCode = code;
        previousFirst = firstCharacter;
        havePrevious = true;
    }
    if (bitPosition != bitCount || result.isEmpty()) return false;
    *output = result;
    return true;
}

bool scanGpfPack(const QByteArray &data, qint64 *rawSize, QString *fileName)
{
    if (!rawSize || !fileName || data.size() < 33) return false;
    const uchar *p = reinterpret_cast<const uchar *>(data.constData());
    if (qFromLittleEndian<quint32>(p) != 192 ||
        std::memcmp(p + 4, "GPFPACK\0", 8) != 0 ||
        qFromLittleEndian<quint16>(p + 12) != 1) return false;
    const QString name = fixedName(p + 14, 14);
    if (name.isEmpty()) return false;

    qint64 offset = 32;
    quint32 compressedBits = qFromLittleEndian<quint32>(p + 28);
    qint64 totalRaw = 0;
    qint32 blockCount = 0;
    while (offset < data.size()) {
        if (!compressedBits) return false;
        const qint64 compressedBytes = (qint64(compressedBits) + 7) / 8;
        if (compressedBytes < 1 || compressedBytes > data.size() - offset)
            return false;
        QByteArray block;
        if (!decodeGpfPackBlock(p + offset, compressedBytes,
                                compressedBits, &block)) return false;
        offset += compressedBytes;
        if (offset < data.size() && block.size() != 8192) return false;
        if (totalRaw > MAX_LEGACY_STORE_SIZE - block.size()) return false;
        totalRaw += block.size();
        if (++blockCount > MAX_GPFPACK_BLOCKS) return false;
        if (offset == data.size()) break;
        if (data.size() - offset < 4) return false;
        compressedBits = qFromLittleEndian<quint32>(p + offset);
        offset += 4;
    }
    if (offset != data.size() || totalRaw < 1) return false;
    *rawSize = totalRaw;
    *fileName = name;
    return true;
}

bool scanPaxRecord(const QByteArray &data, qint64 offset,
                   qint64 *headerEnd, qint64 *rawSize, QString *fileName)
{
    if (!headerEnd || !rawSize || !fileName || offset < 0 ||
        offset > data.size() - 38 ||
        std::memcmp(data.constData() + offset, "LZF0", 4) != 0)
        return false;
    const uchar *p = reinterpret_cast<const uchar *>(data.constData());
    const qint64 size = qFromBigEndian<quint32>(p + offset + 8);
    const qint32 nameSize = qFromBigEndian<quint16>(p + offset + 28);
    if (size < 1 || size > MAX_LEGACY_STORE_SIZE || nameSize < 2 ||
        nameSize > 1024 || offset > data.size() - 30 - nameSize - 8)
        return false;
    const qint64 nameOffset = offset + 30;
    if (p[nameOffset + nameSize - 1] != 0 ||
        std::memchr(p + nameOffset, 0, size_t(nameSize - 1)))
        return false;
    for (qint32 i = 0; i < nameSize - 1; ++i)
        if (p[nameOffset + i] < 0x20 || p[nameOffset + i] > 0x7e)
            return false;
    const QString name = fixedName(p + nameOffset, nameSize - 1);
    if (name.isEmpty()) return false;

    const qint64 streamOffset = nameOffset + nameSize;
    quint32 bitPosition = 0;
    quint32 windowBits = 0, lowBits = 0, subtractor = 0;
    quint32 lengthSymbols = 0, minimumLength = 0;
    quint32 mainMaximum = 0, lengthMaximum = 0;
    if (!readPaxBits(data, p, streamOffset, 4, &bitPosition, &windowBits) ||
        !readPaxBits(data, p, streamOffset, 4, &bitPosition, &lowBits) ||
        !readPaxBits(data, p, streamOffset, 5, &bitPosition, &subtractor) ||
        !readPaxBits(data, p, streamOffset, 10, &bitPosition,
                     &lengthSymbols) ||
        !readPaxBits(data, p, streamOffset, 3, &bitPosition,
                     &minimumLength) ||
        !readPaxBits(data, p, streamOffset, 16, &bitPosition,
                     &mainMaximum) ||
        !readPaxBits(data, p, streamOffset, 16, &bitPosition,
                     &lengthMaximum) || windowBits < 9 || windowBits > 15 ||
        lowBits > windowBits || (windowBits - lowBits) > 10 ||
        !subtractor || subtractor >= (1U << windowBits) ||
        lengthSymbols < 2 || lengthSymbols > 2048 || minimumLength < 2 ||
        mainMaximum < 1 || lengthMaximum < 1)
        return false;

    *headerEnd = streamOffset;
    *rawSize = size;
    *fileName = name;
    return true;
}

bool scanDclStream(const uchar *data, qint64 size, qint64 *consumed,
                   qint64 *rawSize)
{
    return XDclDecoder::scan(data, size, MAX_LEGACY_STORE_SIZE, consumed,
                             rawSize);
}

bool instalitRangeWithin(qint64 total, qint64 offset, qint64 size)
{
    return total >= 0 && offset >= 0 && size >= 0 &&
           offset <= total && size <= total - offset;
}

bool instalitNeResourceTable(const uchar *p, qint64 total,
                             qint64 *tableOffset, quint16 *shift)
{
    if (!p || !tableOffset || !shift ||
        !instalitRangeWithin(total, 0x3c, 4)) return false;
    const qint64 neOffset = qFromLittleEndian<quint32>(p + 0x3c);
    if (!instalitRangeWithin(total, neOffset, 0x26) ||
        p[neOffset] != 'N' || p[neOffset + 1] != 'E') return false;
    const qint64 relativeOffset =
        qFromLittleEndian<quint16>(p + neOffset + 0x24);
    if (relativeOffset < 1 || neOffset > total - relativeOffset) return false;
    const qint64 resultOffset = neOffset + relativeOffset;
    if (!instalitRangeWithin(total, resultOffset, 2)) return false;
    const quint16 resultShift = qFromLittleEndian<quint16>(p + resultOffset);
    if (resultShift > 15) return false;
    *tableOffset = resultOffset;
    *shift = resultShift;
    return true;
}

QString instalitPascalName(const uchar *p, qint64 total, qint64 tableOffset,
                           quint16 token)
{
    if (!p || (token & 0x8000) || tableOffset > total - token) return QString();
    const qint64 offset = tableOffset + token;
    if (!instalitRangeWithin(total, offset, 1)) return QString();
    const qint32 length = p[offset];
    if (length < 1 || !instalitRangeWithin(total, offset + 1, length))
        return QString();
    return fixedName(p + offset + 1, length);
}

bool instalitReadCString(const uchar *p, qint64 offset, qint64 end,
                         QString *value, qint64 *nextOffset)
{
    if (!p || !value || !nextOffset || offset < 0 || end <= offset ||
        end - offset > (std::numeric_limits<qint32>::max)()) return false;
    const void *terminator = std::memchr(p + offset, 0, size_t(end - offset));
    if (!terminator) return false;
    const qint64 length = static_cast<const uchar *>(terminator) - (p + offset);
    if (length < 1 || length > 1024) return false;
    const QString result = fixedName(p + offset, qint32(length));
    if (result.isEmpty()) return false;
    *value = result;
    *nextOffset = offset + length + 1;
    return true;
}

quint32 instalitNameKey(quint16 typeId, quint16 resourceId)
{
    return (quint32(typeId) << 16) | resourceId;
}

bool instalitNeNameTable(const uchar *p, qint64 total, qint64 tableOffset,
                         quint16 shift, QHash<quint32, QString> *typeNames,
                         QHash<quint32, QString> *resourceNames)
{
    if (!p || !typeNames || !resourceNames) return false;
    typeNames->clear();
    resourceNames->clear();

    qint64 typeOffset = tableOffset + 2;
    qint32 typeGuard = 0;
    while (instalitRangeWithin(total, typeOffset, 8) &&
           typeGuard++ < 0x4000) {
        const quint16 typeId = qFromLittleEndian<quint16>(p + typeOffset);
        if (!typeId) return true;
        const quint16 count = qFromLittleEndian<quint16>(p + typeOffset + 2);
        const qint64 rowsOffset = typeOffset + 8;
        if (count > 0x4000 ||
            !instalitRangeWithin(total, rowsOffset, qint64(count) * 12))
            return false;

        if (typeId == 0x800f) {
            for (quint16 i = 0; i < count; ++i) {
                const qint64 row = rowsOffset + qint64(i) * 12;
                const qint64 dataOffset =
                    qint64(qFromLittleEndian<quint16>(p + row)) << shift;
                const qint64 dataSize =
                    qint64(qFromLittleEndian<quint16>(p + row + 2)) << shift;
                if (!instalitRangeWithin(total, dataOffset, dataSize))
                    continue;

                qint64 recordOffset = dataOffset;
                const qint64 dataEnd = dataOffset + dataSize;
                while (instalitRangeWithin(dataEnd, recordOffset, 2)) {
                    const quint16 recordSize =
                        qFromLittleEndian<quint16>(p + recordOffset);
                    if (!recordSize) break;
                    if (recordSize < 8 ||
                        !instalitRangeWithin(dataEnd, recordOffset,
                                             recordSize)) break;
                    const quint16 mappedType =
                        qFromLittleEndian<quint16>(p + recordOffset + 2);
                    const quint16 mappedResource =
                        qFromLittleEndian<quint16>(p + recordOffset + 4);
                    const qint64 recordEnd = recordOffset + recordSize;
                    QString typeName;
                    QString resourceName;
                    qint64 stringOffset = recordOffset + 6;
                    if (!instalitReadCString(p, stringOffset, recordEnd,
                                             &typeName, &stringOffset) ||
                        !instalitReadCString(p, stringOffset, recordEnd,
                                             &resourceName, &stringOffset)) {
                        break;
                    }
                    typeNames->insert(mappedType, typeName);
                    resourceNames->insert(
                        instalitNameKey(mappedType, mappedResource),
                        resourceName);
                    recordOffset = recordEnd;
                }
            }
        }
        typeOffset = rowsOffset + qint64(count) * 12;
    }
    return false;
}

QString instalitNeResourceName(
    const uchar *p, qint64 total, qint64 tableOffset, quint16 typeId,
    quint16 resourceId, bool typeName,
    const QHash<quint32, QString> &typeNames,
    const QHash<quint32, QString> &resourceNames)
{
    const quint16 token = typeName ? typeId : resourceId;
    if (!(token & 0x8000))
        return instalitPascalName(p, total, tableOffset, token);
    if (typeName) return typeNames.value(typeId);
    return resourceNames.value(instalitNameKey(typeId, resourceId));
}

bool instalitResource(const uchar *p, qint64 total, qint64 resourceOffset,
                      qint64 resourceLength, quint16 shift,
                      qint64 *dataOffset, qint64 *packedSize,
                      qint64 *rawSize, quint32 *crc32)
{
    if (!p || !dataOffset || !packedSize || !rawSize || !crc32 ||
        resourceLength < 9 ||
        !instalitRangeWithin(total, resourceOffset, resourceLength))
        return false;
    const qint64 packed =
        qFromLittleEndian<quint32>(p + resourceOffset + 4);
    if (packed < 1 || packed > resourceLength - 8 ||
        resourceLength - (8 + packed) >= (Q_INT64_C(1) << shift) ||
        !instalitRangeWithin(total, resourceOffset + 8, packed))
        return false;

    qint64 consumed = 0;
    qint64 raw = 0;
    if (!scanDclStream(p + resourceOffset + 8, packed, &consumed, &raw) ||
        consumed != packed || raw < 1) return false;

    *dataOffset = resourceOffset + 8;
    *packedSize = packed;
    *rawSize = raw;
    *crc32 = qFromLittleEndian<quint32>(p + resourceOffset);
    return true;
}

bool scanZooLzdStream(const uchar *data, qint64 size, qint64 *consumed,
                       qint64 *rawSize)
{
    if (!data || !consumed || !rawSize || size < 3 ||
        size > MAX_LEGACY_STORE_SIZE)
        return false;
    QVector<quint32> lengths(8192, 0);
    for (qint32 i = 0; i < 256; ++i) lengths[i] = 1;
    qint64 bitPosition = 0;
    qint32 width = 9;
    quint32 nextCode = 258;
    quint32 widthLimit = 512;
    qint32 previousCode = -1;
    qint64 outputSize = 0;
    bool initialClear = false;

    while (true) {
        quint32 code = 0;
        if (!readZooLzdCode(data, size, width, &bitPosition, &code)) {
            return false;
        }
        if (!initialClear) {
            if (code != 256) return false;
            initialClear = true;
        }
        if (code == 257) break;
        if (code == 256) {
            width = 9;
            nextCode = 258;
            widthLimit = 512;
            previousCode = -1;
            continue;
        }

        quint32 length = 0;
        if (code < nextCode && lengths[qint32(code)]) {
            length = lengths[qint32(code)];
        } else if (previousCode >= 0 && code == nextCode) {
            length = lengths[previousCode] + 1;
        } else {
            return false;
        }
        if (outputSize > MAX_LEGACY_STORE_SIZE - length) return false;
        outputSize += length;

        if (previousCode >= 0 && nextCode < 8192) {
            const quint32 previousLength = lengths[previousCode];
            if (!previousLength || previousLength ==
                    (std::numeric_limits<quint32>::max)())
                return false;
            lengths[qint32(nextCode++)] = previousLength + 1;
            if (width < 13 && nextCode >= widthLimit) {
                ++width;
                widthLimit <<= 1;
            }
        }
        previousCode = qint32(code);
    }
    if (!outputSize) return false;
    *consumed = (bitPosition + 7) / 8;
    *rawSize = outputSize;
    return true;
}

struct ViseRecord {
    qint64 row = 0;
    qint64 dataOffset = 0;
    qint64 packedSize = 0;
    qint64 rawSize = 0;
    QString name;
};

bool scanViseServiceTable(const QByteArray &data, const uchar *pData,
                          qint64 footerOffset, qint64 tableOffset,
                          qint64 *pTableEnd,
                          QList<ViseRecord> *pRecords,
                          XBinary::PDSTRUCT *pPdStruct)
{
    if (!pData || !pTableEnd || !pRecords ||
        !instalitRangeWithin(footerOffset, tableOffset, 2)) {
        return false;
    }
    const qint32 count = qFromLittleEndian<qint16>(pData + tableOffset);
    if ((count < 1) || (count > 32)) return false;
    qint64 position = tableOffset + 2;
    QList<ViseRecord> found;
    for (qint32 i = 0; i < count; ++i) {
        if (!instalitRangeWithin(footerOffset, position, 1)) return false;
        const qint32 nameSize = pData[position++];
        if ((nameSize < 1) || (nameSize > 64) ||
            !instalitRangeWithin(footerOffset, position, nameSize + 16)) {
            return false;
        }
        const QString name = fixedName(pData + position, nameSize);
        if (name.isEmpty()) return false;
        const qint64 row = position - 1;
        position += nameSize + 12;
        const qint64 packedSize =
            qFromLittleEndian<quint32>(pData + position);
        position += 4;
        if ((packedSize < 2) || (packedSize & 1) ||
            !instalitRangeWithin(footerOffset, position, packedSize)) {
            return false;
        }
        qint64 rawSize = 0;
        const QByteArray packed = data.mid(qint32(position),
                                           qint32(packedSize));
        if (!XViseDeflateDecoder::decode(packed, -1, nullptr, &rawSize,
                                         pPdStruct) ||
            (rawSize < 1) || (rawSize > MAX_LEGACY_STORE_SIZE)) {
            return false;
        }
        ViseRecord record;
        record.row = row;
        record.dataOffset = position;
        record.packedSize = packedSize;
        record.rawSize = rawSize;
        record.name = name;
        found.append(record);
        position += packedSize;
    }
    *pRecords = found;
    *pTableEnd = position;
    return true;
}

bool isDosNameCharacter(uchar value)
{
    return ((value >= 'A') && (value <= 'Z')) ||
           ((value >= 'a') && (value <= 'z')) ||
           ((value >= '0') && (value <= '9')) ||
           std::strchr("_$~!#%&(){}@'`^-", int(value));
}

bool readLegacyCString(const QByteArray &data, const uchar *pData,
                       qint64 total, qint64 *pPosition, QString *pResult)
{
    if (!pData || !pPosition || !pResult || (*pPosition < 0) ||
        (*pPosition >= total) || (total > data.size())) {
        return false;
    }
    const qint64 nul = data.indexOf('\0', qint32(*pPosition));
    if ((nul < *pPosition) || (nul - *pPosition > 260)) return false;
    *pResult = fixedName(pData + *pPosition, qint32(nul - *pPosition));
    *pPosition = nul + 1;
    return !pResult->isEmpty();
}
}  // namespace

struct XLegacyStoreArchive::EntryBuilder
{
    EntryBuilder(XLegacyStoreArchive *pArchive, qint64 total,
                 QList<ENTRY> *pEntries, QSet<QString> *pUsedFiles,
                 QSet<QString> *pUsedDirs,
                 QHash<QString, qint32> *pNextSuffixes,
                 QHash<QString, QString> *pResolvedDirs)
        : m_pArchive(pArchive),
          m_nTotal(total),
          m_pEntries(pEntries),
          m_pUsedFiles(pUsedFiles),
          m_pUsedDirs(pUsedDirs),
          m_pNextSuffixes(pNextSuffixes),
          m_pResolvedDirs(pResolvedDirs)
    {
    }

    bool operator()(qint64 headerOffset, qint64 headerSize,
                    qint64 dataOffset, qint64 dataSize, qint64 rawSize,
                    HANDLE_METHOD method, const QString &name,
                    bool haveCrc32 = false, quint32 crc32 = 0)
    {
        if (!m_pArchive || !m_pEntries || !m_pUsedFiles || !m_pUsedDirs ||
            !m_pNextSuffixes || !m_pResolvedDirs || name.isEmpty() ||
            !m_pArchive->rangeWithin(m_nTotal, headerOffset, headerSize) ||
            !m_pArchive->rangeWithin(m_nTotal, dataOffset, dataSize)) {
            return false;
        }
        QString unique;
        if (!m_pArchive->makeUniquePath(name, m_pUsedFiles, m_pUsedDirs,
                                        m_pNextSuffixes, m_pResolvedDirs,
                                        &unique)) {
            return false;
        }
        ENTRY entry = {};
        entry.nHeaderOffset = headerOffset;
        entry.nHeaderSize = headerSize;
        entry.nDataOffset = dataOffset;
        entry.nDataSize = dataSize;
        entry.nUncompressedSize = rawSize;
        entry.handleMethod = method;
        entry.sFileName = unique;
        entry.bCRC32Defined = haveCrc32;
        entry.nCRC32 = crc32;
        m_pEntries->append(entry);
        return m_pEntries->size() <= MAX_RECORDS;
    }

    bool addViseRecords(const QList<ViseRecord> &records,
                        QSet<qint64> *pUsedStreams)
    {
        if (!pUsedStreams) return false;
        for (const ViseRecord &record : records) {
            if (pUsedStreams->contains(record.dataOffset)) continue;
            if (!operator()(record.row,
                            record.dataOffset - record.row,
                            record.dataOffset, record.packedSize,
                            record.rawSize, HANDLE_METHOD_VISE_DEFLATE,
                            record.name)) {
                return false;
            }
            pUsedStreams->insert(record.dataOffset);
        }
        return true;
    }

    static bool headerOffsetLess(const ENTRY &left, const ENTRY &right)
    {
        return left.nHeaderOffset < right.nHeaderOffset;
    }

private:
    XLegacyStoreArchive *m_pArchive = nullptr;
    qint64 m_nTotal = 0;
    QList<ENTRY> *m_pEntries = nullptr;
    QSet<QString> *m_pUsedFiles = nullptr;
    QSet<QString> *m_pUsedDirs = nullptr;
    QHash<QString, qint32> *m_pNextSuffixes = nullptr;
    QHash<QString, QString> *m_pResolvedDirs = nullptr;
};

XLegacyStoreArchive::XLegacyStoreArchive(QIODevice *pDevice, FT fileType)
    : XGameStoreArchiveBase(pDevice, fileType)
{
}

bool XLegacyStoreArchive::isValid(QIODevice *pDevice, FT fileType,
                                  PDSTRUCT *pPdStruct)
{
    if (!isLegacyStoreType(fileType)) return false;
    XLegacyStoreArchive archive(pDevice, fileType);
    return archive.isValid(pPdStruct);
}

XBinary::FT XLegacyStoreArchive::detectFileType(QIODevice *pDevice,
                                                 PDSTRUCT *pPdStruct)
{
    if (!pDevice || pDevice->isSequential() || pDevice->size() < 8 ||
        pDevice->size() > MAX_LEGACY_STORE_SIZE ||
        !isPdStructNotCanceled(pPdStruct)) return FT_UNKNOWN;
    const qint64 oldPos = pDevice->pos();
    if (!pDevice->seek(0)) return FT_UNKNOWN;
    const QByteArray h = pDevice->read(128);
    pDevice->seek(oldPos);
    if (h.size() < 8) return FT_UNKNOWN;

    QList<FT> candidates;
    if (h.startsWith("MZ")) {
        // PiMP is a PE SFX.  Its authenticated archive starts at the PE
        // overlay, so the full structural validator below determines whether
        // an otherwise ordinary executable is a PiMP package.
        candidates << FT_INSTALLSHIELD_LAUNCHER << FT_GPINSTALL_SFX << FT_PIMP_SFX << FT_VISE_SFX << FT_SOFTPAQ1_SFX
                   << FT_INSTALIT_SFX;
    } else if (h.size() >= 31 &&
               h.startsWith(QByteArray("\xa5\x96\xfd\xff", 4)) &&
               h.mid(24, 7) == QByteArray("FTCOMP\0", 7)) {
        candidates << FT_FTCOMP;
    } else if (h.startsWith(QByteArray("\x84\x8d\x01\x02", 4))) {
        candidates << FT_DN_ARCHIVE;
    } else if (h.startsWith("FPAK") || h.startsWith("FPAC")) {
        candidates << FT_FPAK;
    } else if (h.size() >= 41 &&
               h.startsWith(QByteArray("DC\x02\0\x01\0", 6))) {
        candidates << FT_LIF_COMPRESSED;
    } else if (isJascRecordHeader(
                   reinterpret_cast<const uchar *>(h.constData()),
                   h.size(), 12)) {
        candidates << FT_JASC_ARCHIVE;
    } else if (h.size() >= 44 && h.startsWith(QByteArray("SSM\0", 4))) {
        candidates << FT_SSM_MODULE;
    } else if (h.startsWith("SSBOB")) {
        candidates << FT_SSBOB;
    } else if (h.size() >= 0x69 && quint8(h.at(0x64)) == 0x1a &&
        qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar *>(h.constData()) + 0x65) ==
            0x12345678U) {
        candidates << FT_SOLITAIRE_DELUXE;
    } else if (h.size() >= 16 && h.startsWith("ARCV") &&
               qFromLittleEndian<quint16>(
                   reinterpret_cast<const uchar *>(h.constData()) + 4) ==
                   0x0110) {
        candidates << FT_ARCV;
    } else if (h.size() >= 8 && quint8(h.at(0)) == 0x01 &&
               quint8(h.at(1)) == 0x00 &&
               qFromLittleEndian<quint32>(
                   reinterpret_cast<const uchar *>(h.constData()) + 2) != 0 &&
               qFromLittleEndian<quint32>(
                   reinterpret_cast<const uchar *>(h.constData()) + 2) <=
                   0x0fffffffU) {
        candidates << FT_INSA;
    } else if (h.size() >= 3 && quint8(h.at(0)) <= 1 &&
        quint8(h.at(1)) >= 4 && quint8(h.at(1)) <= 6) {
        candidates << FT_INSTALIT_DATA;
    } else if (h.size() >= 32 && h.startsWith(QByteArray("\x04\0\0\0", 4)) &&
        quint8(h.at(30)) <= 1 && quint8(h.at(31)) >= 4 &&
        quint8(h.at(31)) <= 6) {
        candidates << FT_SCF;
    } else if (h.startsWith("LZF0")) {
        candidates << FT_PAX;
    } else if (h.size() >= 32 &&
        qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(h.constData())) == 192 &&
        std::memcmp(h.constData() + 4, "GPFPACK\0", 8) == 0 &&
        qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(h.constData()) + 12) == 1) {
        candidates << FT_GPFPACK;
    } else if (h.startsWith(QByteArray("\\\\z\xc5\xd4\xe3\x40\xf0\xf0\xf1\xf0\xf0\xf1", 13))) {
        candidates << FT_EMT_IMAGE;
    } else if (h.size() >= 5 &&
        quint8(h.at(0)) == 0x13 && quint8(h.at(1)) == 0x5d &&
        quint8(h.at(2)) == 0x65 && quint8(h.at(3)) == 0x8c &&
        quint8(h.at(4)) == 0x3a) {
        candidates << FT_INSTALLSHIELD3_ARCHIVE;
    } else if (h.startsWith(QByteArray("\x01" "F Install 2", 12))) {
        candidates << FT_FINSTALL_ARCHIVE;
    } else if (h.size() >= 16 &&
               quint8(h.at(0)) == 0xe7 && quint8(h.at(1)) == 0x50 &&
               quint8(h.at(2)) == 0xa9 && quint8(h.at(3)) == 0xae &&
               quint8(h.at(6)) == 0xe7 && quint8(h.at(7)) == 0x50 &&
               quint8(h.at(8)) == 0xa9 && quint8(h.at(9)) == 0xae &&
               quint8(h.at(12)) == 0xdd && quint8(h.at(13)) == 0xc1 &&
               quint8(h.at(14)) == 0xc0) {
        candidates << FT_IS_STORED;
    } else if (h.startsWith("EPFS")) {
        candidates << FT_EPFS_ARCHIVE;
    } else if (h.size() >= 12 &&
               ((quint8(h.at(0)) & 0x80U) != 0) &&
               (quint8(h.at(0)) & 0x7fU) >= 1 &&
               (quint8(h.at(0)) & 0x7fU) <= 8 &&
               (quint8(h.at(4)) == 1 || quint8(h.at(4)) == 2)) {
        candidates << FT_STUNTS_DSI;
    } else if (h.startsWith("KA Archive\0")) {
        candidates << FT_KA_ARCHIVE;
    } else if (h.startsWith("LPAK")) {
        candidates << FT_LPAK;
    } else if (h.startsWith("CPQ_LZH")) {
        candidates << FT_COMPAQ_LZH;
    } else if (h.startsWith("SAB Diskette Utility\0")) {
        candidates << FT_SABDU_IMAGE;
    } else if (h.size() >= 48 && h.indexOf(
                   QByteArray("SZDD\x88\xf0\x27\x33", 8)) >= 20) {
        candidates << FT_INSTALLSHIELD_BOOT;
    } else {
        if (h.size() >= 84 && quint8(h.at(0)) >= 1 &&
            quint8(h.at(0)) <= 63)
            candidates << FT_SHRINKWRAP_IMAGE;
        if (h.size() >= 4 && qFromLittleEndian<quint16>(
                reinterpret_cast<const uchar *>(h.constData()) + 2) == 6)
            candidates << FT_MLB_ARCHIVE;
        candidates << FT_LEGACY_CAT << FT_LEGACY_RES << FT_LEGACY_RSC;
    }

    // IS skin archives have no magic. Add this candidate independently so a
    // weak magicless legacy gate cannot prevent the strict full-file parser
    // below from seeing a plausible decoded name/size prefix.
    if (looksLikeIsSkin(h) && !candidates.contains(FT_IS_SKIN))
        candidates << FT_IS_SKIN;

    for (FT ft : candidates) {
        if (!isPdStructNotCanceled(pPdStruct)) return FT_UNKNOWN;
        if (isValid(pDevice, ft, pPdStruct)) return ft;
    }
    return FT_UNKNOWN;
}

XBinary *XLegacyStoreArchive::createInstance(QIODevice *pDevice,
                                              bool bIsImage,
                                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XLegacyStoreArchive(pDevice, getFileType());
}

bool XLegacyStoreArchive::scanFormat(QList<ENTRY> *pEntries,
                                     qint64 *pArchiveEnd,
                                     PDSTRUCT *pPdStruct)
{
    QPointer<XLegacyStoreArchive> guardedThis(this);
    const FT ft = getFileType();
    const qint64 total = getSize();
    if (!guardedThis || !isLegacyStoreType(ft) || total < 8 ||
        total > MAX_LEGACY_STORE_SIZE ||
        total > (std::numeric_limits<int>::max)() ||
        !isPdStructNotCanceled(pPdStruct)) return false;
    const QByteArray data = read_array_process(0, total, pPdStruct);
    if (!guardedThis || data.size() != total) return false;
    const uchar *p = reinterpret_cast<const uchar *>(data.constData());

    QList<ENTRY> entries;
    qint64 archiveEnd = total;
    QSet<QString> usedFiles;
    QSet<QString> usedDirs;
    QHash<QString, qint32> nextSuffixes;
    QHash<QString, QString> resolvedDirs;
    EntryBuilder addEntry(this, total, &entries, &usedFiles, &usedDirs,
                          &nextSuffixes, &resolvedDirs);

    if (ft == FT_SSBOB) {
        // SSBOB slideshow packages carry a fixed playback-script area, a
        // 74-byte record table, and raw embedded image payloads.
        if ((total < 12) || (std::memcmp(p, "SSBOB", 5) != 0)) return false;
        const qint64 nScriptCount = qFromLittleEndian<quint16>(p + 8);
        const qint64 nRecordCount = qFromLittleEndian<quint16>(p + 10);
        if ((nRecordCount < 1) || (nRecordCount > MAX_RECORDS)) return false;
        const qint64 nTableOffset = 12 + nScriptCount * 12;
        const qint64 nDataOffset = nTableOffset + nRecordCount * 74;
        if ((nTableOffset < 12) || (nDataOffset < nTableOffset) || (nDataOffset > total)) return false;

        for (qint64 i = 0; i < nRecordCount; ++i) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            const qint64 nRecordOffset = nTableOffset + i * 74;
            if (!rangeWithin(total, nRecordOffset, 74)) return false;
            const quint8 nType = p[nRecordOffset];
            if (nType != 0x10) continue;
            const QString sName = fixedName(p + nRecordOffset + 1, 65);
            const qint64 nMemberOffset = qFromLittleEndian<quint32>(p + nRecordOffset + 66);
            const qint64 nMemberSize = qFromLittleEndian<quint32>(p + nRecordOffset + 70);
            if (sName.isEmpty() || (nMemberOffset < nDataOffset) || !rangeWithin(total, nMemberOffset, nMemberSize) ||
                !addEntry(nRecordOffset, 74, nMemberOffset, nMemberSize, nMemberSize, HANDLE_METHOD_STORE, sName)) {
                return false;
            }
        }
    } else if (ft == FT_SSM_MODULE) {
        // Pegasus/AccuSoft PICTools distributes opcode modules as tiny DOS
        // self-expanders.  Their fixed header identifies the target DLL and
        // exact expanded size and codec variant.
        if (total < 64 || std::memcmp(p, "SSM\0", 4) != 0 ||
            qFromLittleEndian<quint32>(p + 24) != 0x2000 ||
            std::memcmp(p + 36, "\x0d\xc0\x03\x01\x01\0\0\x3d", 8) != 0)
            return false;
        const QString name = fixedName(p + 4, 12);
        const qint64 rawSize = qFromLittleEndian<quint32>(p + 20);
        const quint16 dosDate = qFromLittleEndian<quint16>(p + 16);
        const quint16 dosTime = qFromLittleEndian<quint16>(p + 18);
        const quint32 codecId = qFromLittleEndian<quint32>(p + 28);
        const quint32 method = qFromLittleEndian<quint32>(p + 32);
        const bool method3 = (method == 3) && (codecId == 0x00068c7c);
        const bool method5 = (method == 5) && (codecId == 0x001b8c7c);
        if (name.isEmpty() || !name.endsWith(QStringLiteral(".dll"),
                                             Qt::CaseInsensitive) ||
            rawSize < 8 || rawSize > MAX_LEGACY_STORE_SIZE ||
            (!method3 && !method5) ||
            !XBinary::isValidDosDateTime(dosDate, dosTime) ||
            !addEntry(0, 36, 36, total - 36, rawSize,
                      method3 ? HANDLE_METHOD_SSM_PICTOOLS
                              : HANDLE_METHOD_SSM_PICTOOLS5,
                      name))
            return false;
        entries.last().mtDateTime =
            XBinary::dosDateTimeToQDateTime(dosDate, dosTime);
    } else if (ft == FT_JASC_ARCHIVE) {
        // JASC's early installer archives concatenate exact records:
        // 17 bytes of metadata, a counted DOS filename, and a raw LHA -lh5-
        // stream.  The u32 at +2 gives the packed size, +10 is a Unix time_t,
        // and +14 is the uncompressed data's CRC-16/ARC.
        qint64 row = 0;
        while (row < total) {
            // Corpus archives carry one zero terminator.  Also accept an exact
            // end immediately after the final stream for complete files whose
            // optional terminator was truncated.
            if ((total - row == 1) && (p[row] == 0)) {
                ++row;
                break;
            }

            if (!isJascRecordHeader(p + row, total - row, 16)) return false;

            const qint32 nameSize = p[row + 16];
            const qint64 headerSize = 17 + nameSize;
            const qint64 packedSize =
                qFromLittleEndian<quint32>(p + row + 2);
            const qint64 rawSize =
                qFromLittleEndian<quint32>(p + row + 6);
            const qint64 dataOffset = row + headerSize;
            const QString name = fixedName(p + row + 17, nameSize);

            if (rawSize < 1 || rawSize > MAX_LEGACY_STORE_SIZE ||
                name.isEmpty() || !name.contains(QLatin1Char('.')) ||
                !rangeWithin(total, dataOffset, packedSize) ||
                !addEntry(row, headerSize, dataOffset, packedSize, rawSize,
                          HANDLE_METHOD_JASC_COMPRESSED, name)) {
                return false;
            }

            row = dataOffset + packedSize;
        }
        if (entries.isEmpty() || row != total) return false;
    } else if (ft == FT_LIF_COMPRESSED) {
        // Binary LIF/DC files contain one Zoo-style 13-bit LZW stream.  The
        // wrapper gives its original name and packed size but not its raw
        // size, so validate the explicit EOF while counting decoded bytes.
        if (total < 44 || std::memcmp(p, "DC\x02\0\x01\0", 6) != 0 ||
            qFromLittleEndian<quint32>(p + 21) != quint32(total) ||
            qFromLittleEndian<quint32>(p + 25) != 6)
            return false;
        const qint64 packedSize = qFromLittleEndian<quint32>(p + 29);
        const QString name = fixedName(p + 6, 15);
        if (name.isEmpty() || packedSize != total - 41) return false;
        qint64 consumed = 0;
        qint64 outputSize = 0;
        if (!scanZooLzdStream(p + 41, packedSize, &consumed, &outputSize) ||
            consumed > packedSize || packedSize - consumed > 1 ||
            (consumed < packedSize && p[41 + consumed] != 0))
            return false;
        // The wrapper's packed-size field includes its optional one-byte
        // alignment pad.  Publish only the bytes through the LZD EOF marker;
        // the generic codec deliberately rejects whole trailing bytes.
        if (!addEntry(0, 41, 41, consumed, outputSize,
                      HANDLE_METHOD_ZOO_LZD, name))
            return false;
    } else if (ft == FT_SOFTPAQ1_SFX) {
        // First-generation Compaq SoftPaqs finish with 25-byte table rows:
        // a fixed DOS name, size, absolute data offset, and timestamp.  The
        // FIT002 sentinel points back to the first row.  Payloads themselves
        // are stored byte-for-byte (and may in turn be ZIP executables).
        if (total < 50 || p[0] != 'M' || p[1] != 'Z') return false;
        const qint64 sentinel = total - 25;
        if (fixedName(p + sentinel, 13).trimmed() != QLatin1String("FIT002") ||
            qFromLittleEndian<quint32>(p + sentinel + 13) != 0)
            return false;
        const qint64 tableOffset = qFromLittleEndian<quint32>(p + sentinel + 17);
        if (tableOffset < 1 || tableOffset >= sentinel ||
            (sentinel - tableOffset) % 25 != 0 ||
            (sentinel - tableOffset) / 25 > MAX_RECORDS)
            return false;
        qint64 previousEnd = -1;
        for (qint64 row = tableOffset; row < sentinel; row += 25) {
            QString name = fixedName(p + row, 13).trimmed();
            const qint64 size = qFromLittleEndian<quint32>(p + row + 13);
            const qint64 offset = qFromLittleEndian<quint32>(p + row + 17);
            if (name.isEmpty() || size < 1 ||
                !rangeWithin(tableOffset, offset, size) ||
                (previousEnd >= 0 && offset <= previousEnd))
                return false;
            if (!addEntry(row, 25, offset, size, size,
                          HANDLE_METHOD_STORE, name))
                return false;
            previousEnd = offset + size - 1;
        }
        if (entries.isEmpty() || previousEnd + 2 != tableOffset) return false;
    } else if (ft == FT_INSTALIT_SFX) {
        // Instalit/Shadow keeps its executable engine components in a custom
        // EXEFILE resource type. Stub-only releases contain only this resource
        // set; full releases optionally append the older [PVL]/[PVM] payload.
        if (total < 64 || p[0] != 'M' || p[1] != 'Z') return false;
        qint64 tableOffset = 0;
        quint16 shift = 0;
        if (!instalitNeResourceTable(p, total, &tableOffset, &shift))
            return false;

        QHash<quint32, QString> typeNames;
        QHash<quint32, QString> resourceNames;
        if (!instalitNeNameTable(p, total, tableOffset, shift, &typeNames,
                                 &resourceNames)) return false;

        qint64 typeOffset = tableOffset + 2;
        qint32 typeGuard = 0;
        qint32 resourceNumber = 0;
        qint32 resourceCount = 0;
        qint64 resourceEnd = 0;
        bool tableTerminated = false;
        while (instalitRangeWithin(total, typeOffset, 8) &&
               typeGuard++ < 0x4000) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            const quint16 typeId =
                qFromLittleEndian<quint16>(p + typeOffset);
            if (!typeId) {
                tableTerminated = true;
                break;
            }
            const quint16 count =
                qFromLittleEndian<quint16>(p + typeOffset + 2);
            const qint64 rowsOffset = typeOffset + 8;
            if (count > 0x4000 ||
                !instalitRangeWithin(total, rowsOffset,
                                     qint64(count) * 12)) return false;
            const QString typeName = instalitNeResourceName(
                p, total, tableOffset, typeId, 0, true, typeNames,
                resourceNames);
            const bool isExeFile =
                typeName.compare(QLatin1String("EXEFILE"),
                                 Qt::CaseInsensitive) == 0;

            for (quint16 i = 0; i < count; ++i) {
                const qint64 row = rowsOffset + qint64(i) * 12;
                if (!isExeFile) continue;
                const quint16 resourceId =
                    qFromLittleEndian<quint16>(p + row + 6);
                const qint64 resourceOffset =
                    qint64(qFromLittleEndian<quint16>(p + row)) << shift;
                const qint64 resourceLength =
                    qint64(qFromLittleEndian<quint16>(p + row + 2)) << shift;
                qint64 dataOffset = 0;
                qint64 packedSize = 0;
                qint64 rawSize = 0;
                quint32 crc32 = 0;
                if (!instalitResource(p, total, resourceOffset,
                                      resourceLength, shift, &dataOffset,
                                      &packedSize, &rawSize, &crc32))
                    return false;

                ++resourceNumber;
                QString name = instalitNeResourceName(
                    p, total, tableOffset, typeId, resourceId, false,
                    typeNames, resourceNames);
                if (name.isEmpty())
                    name = QStringLiteral("Resource_%1").arg(resourceNumber);
                if (!addEntry(resourceOffset, 8, dataOffset, packedSize,
                              rawSize, HANDLE_METHOD_PKWARE_DCL_IMPLODE,
                              name, true, crc32)) return false;
                ++resourceCount;
                resourceEnd = qMax(resourceEnd,
                                   resourceOffset + resourceLength);
            }
            typeOffset = rowsOffset + qint64(count) * 12;
        }
        if (!tableTerminated || resourceCount < 1) return false;
        archiveEnd = resourceEnd;

        // The serial footer and its payload pointers are optional. Parse the
        // complete suffix into temporary vectors first, so a damaged suffix
        // cannot leave a partial member set after the valid EXEFILE resources.
        const qint64 marker = data.lastIndexOf("934730434875");
        if (marker >= 1 && p[marker - 1] == 12 &&
            instalitRangeWithin(total, marker + 12, 16)) {
            archiveEnd = qMax(archiveEnd, marker + 28);
            const qint64 pvlOffset =
                qFromLittleEndian<quint32>(p + marker + 20);
            const qint64 pvmOffset =
                qFromLittleEndian<quint32>(p + marker + 24);
            bool suffixValid = pvlOffset > 0 && pvmOffset > 0 &&
                instalitRangeWithin(total, pvlOffset, 6) &&
                instalitRangeWithin(total, pvmOffset, 11) &&
                pvlOffset < pvmOffset &&
                std::memcmp(p + pvlOffset, "\x05[PVL]", 6) == 0 &&
                std::memcmp(p + pvmOffset, "\x05[PVM]", 6) == 0 &&
                qFromLittleEndian<quint32>(p + pvmOffset + 6) == 28;

            QVector<qint64> streamOffsets;
            QVector<qint64> streamPackedSizes;
            QVector<qint64> streamRawSizes;
            if (suffixValid) {
                qint64 stream = pvlOffset + 6;
                while (stream < pvmOffset) {
                    if (!isPdStructNotCanceled(pPdStruct)) return false;
                    qint64 packedSize = 0;
                    qint64 rawSize = 0;
                    if (!scanDclStream(p + stream, pvmOffset - stream,
                                       &packedSize, &rawSize) ||
                        packedSize < 1) {
                        suffixValid = false;
                        break;
                    }
                    streamOffsets.append(stream);
                    streamPackedSizes.append(packedSize);
                    streamRawSizes.append(rawSize);
                    if (streamOffsets.size() > MAX_RECORDS - entries.size()) {
                        suffixValid = false;
                        break;
                    }
                    stream += packedSize;
                }
                suffixValid = suffixValid && stream == pvmOffset &&
                              !streamOffsets.isEmpty();
            }

            QVector<qint64> manifestOffsets;
            QVector<QString> manifestNames;
            qint64 manifest = pvmOffset + 11;
            if (suffixValid) {
                for (qint32 i = 0; i < streamOffsets.size(); ++i) {
                    if (!instalitRangeWithin(marker, manifest, 55) ||
                        p[manifest] != 0xf0) {
                        suffixValid = false;
                        break;
                    }
                    qint32 nameSize = 0;
                    while (nameSize < 12 &&
                           p[manifest + 1 + nameSize]) ++nameSize;
                    if (nameSize < 1 || nameSize > 12 ||
                        p[manifest + 1 + nameSize] != 0) {
                        suffixValid = false;
                        break;
                    }
                    const QString name =
                        fixedName(p + manifest + 1, nameSize);
                    if (name.isEmpty()) {
                        suffixValid = false;
                        break;
                    }
                    manifestOffsets.append(manifest);
                    manifestNames.append(name);
                    manifest += 55;
                }
            }

            if (suffixValid) {
                for (qint32 i = 0; i < streamOffsets.size(); ++i) {
                    if (!addEntry(manifestOffsets.at(i), 55,
                                  streamOffsets.at(i),
                                  streamPackedSizes.at(i),
                                  streamRawSizes.at(i),
                                  HANDLE_METHOD_PKWARE_DCL_IMPLODE,
                                  manifestNames.at(i))) return false;
                }
                archiveEnd = qMax(archiveEnd, qMax(manifest, pvmOffset + 11));
            }
        }

        std::stable_sort(entries.begin(), entries.end(),
                         EntryBuilder::headerOffsetLess);
    } else if (ft == FT_DN_ARCHIVE) {
        // DOS Navigator 1.x places all data records first, repeats their
        // names and absolute source offsets in a directory, then terminates
        // with a fixed-size 0506 footer.  Cross-checking both tables avoids a
        // false positive on the short record magic.
        struct DnRecord {
            qint64 row;
            qint64 dataOffset;
            qint64 packedSize;
            qint64 rawSize;
            QString name;
        };
        QList<DnRecord> records;
        qint64 pos = 0;
        while (rangeWithin(total, pos, 30) &&
               std::memcmp(p + pos, "\x84\x8d\x01\x02", 4) == 0) {
            const qint64 packedSize = qFromLittleEndian<quint32>(p + pos + 18);
            const qint64 rawSize = qFromLittleEndian<quint32>(p + pos + 22);
            const qint32 nameSize = qFromLittleEndian<quint16>(p + pos + 28);
            if (packedSize < 1 || rawSize < 1 ||
                rawSize > MAX_LEGACY_STORE_SIZE || nameSize < 1 ||
                nameSize > 512 || !rangeWithin(total, pos + 30,
                                                qint64(nameSize) + packedSize))
                return false;
            const QString name = fixedName(p + pos + 30, nameSize);
            if (name.isEmpty()) return false;
            const qint64 dataOffset = pos + 30 + nameSize;
            records.append({pos, dataOffset, packedSize, rawSize, name});
            if (records.size() > MAX_RECORDS) return false;
            pos = dataOffset + packedSize;
        }
        if (records.isEmpty()) return false;
        for (const DnRecord &record : records) {
            if (!rangeWithin(total, pos, 46) ||
                std::memcmp(p + pos, "\x84\x8d\x03\x04", 4) != 0)
                return false;
            const qint32 nameSize = qFromLittleEndian<quint16>(p + pos + 30);
            const qint64 sourceOffset = qFromLittleEndian<quint32>(p + pos + 42);
            if (nameSize < 1 || nameSize > 512 ||
                !rangeWithin(total, pos + 46, nameSize) ||
                sourceOffset != record.row ||
                fixedName(p + pos + 46, nameSize) != record.name)
                return false;
            pos += 46 + nameSize;
        }
        if (!rangeWithin(total, pos, 46) ||
            std::memcmp(p + pos, "\x84\x8d\x05\x06", 4) != 0 ||
            pos + 46 != total)
            return false;
        for (const DnRecord &record : records) {
            const HANDLE_METHOD method =
                record.packedSize == record.rawSize
                    ? HANDLE_METHOD_STORE
                    : HANDLE_METHOD_DN_COMPRESSED;
            if (!addEntry(record.row, record.dataOffset - record.row,
                          record.dataOffset, record.packedSize, record.rawSize,
                          method, record.name))
                return false;
        }
    } else if (ft == FT_FPAK) {
        // FoxPro's distribution volumes use an FPAK lead volume or FPAC
        // continuation and one or more completely bounded FPPF segments.
        qint64 pos = 4;
        qint64 declaredPacked = -1;
        qint64 declaredRaw = -1;
        qint64 completePacked = 0;
        qint64 completeRaw = 0;
        bool havePartial = false;
        if (std::memcmp(p, "FPAK", 4) == 0) {
            if (!rangeWithin(total, 0, 16)) return false;
            const quint16 version = qFromLittleEndian<quint16>(p + 4);
            declaredPacked = qFromLittleEndian<quint32>(p + 6);
            declaredRaw = qFromLittleEndian<quint32>(p + 10);
            const qint32 descriptionSize = qFromLittleEndian<quint16>(p + 14);
            if ((version != 1 && version != 2) || declaredPacked < 1 ||
                declaredPacked > MAX_LEGACY_STORE_SIZE || declaredRaw < 1 ||
                declaredRaw > MAX_LEGACY_STORE_SIZE || descriptionSize < 1 ||
                descriptionSize > 1024 ||
                !rangeWithin(total, 16, descriptionSize))
                return false;
            for (qint32 i = 0; i < descriptionSize; ++i) {
                const quint8 value = p[16 + i];
                if (value < 0x20U || value > 0x7eU) return false;
            }
            pos = 16 + descriptionSize;
        } else if (std::memcmp(p, "FPAC", 4) != 0) {
            return false;
        }
        qint32 segmentIndex = 0;
        while (pos < total) {
            if (!rangeWithin(total, pos, 30) ||
                std::memcmp(p + pos, "FPPF", 4) != 0 ||
                qFromLittleEndian<quint16>(p + pos + 4) != 0 ||
                qFromLittleEndian<quint16>(p + pos + 6) != 6)
                return false;
            const quint16 dosTime = qFromLittleEndian<quint16>(p + pos + 8);
            const quint16 dosDate = qFromLittleEndian<quint16>(p + pos + 10);
            const quint32 crc32 = qFromLittleEndian<quint32>(p + pos + 12);
            const qint64 packedSize = qFromLittleEndian<quint32>(p + pos + 16);
            const qint64 rawSize = qFromLittleEndian<quint32>(p + pos + 20);
            const qint64 segmentSize = qFromLittleEndian<quint32>(p + pos + 24);
            const qint32 nameSize = qFromLittleEndian<quint16>(p + pos + 28);
            if (packedSize < 1 || rawSize < 1 || segmentSize < 1 ||
                packedSize > MAX_LEGACY_STORE_SIZE ||
                rawSize > MAX_LEGACY_STORE_SIZE || segmentSize > packedSize ||
                nameSize < 1 || nameSize > 512 ||
                !rangeWithin(total, pos + 30,
                             qint64(nameSize) + segmentSize) ||
                ((dosTime || dosDate) &&
                 !XBinary::isValidDosDateTime(dosDate, dosTime)))
                return false;
            QString name = fixedName(p + pos + 30, nameSize);
            if (name.isEmpty()) return false;
            const qint64 dataOffset = pos + 30 + nameSize;
            // A continuation volume is not independently decodable.  Keep
            // its real segment length visible while preserving the logical
            // output size only for a complete member.
            const bool complete = segmentSize == packedSize;
            if (complete) {
                if ((declaredPacked >= 0 &&
                     completePacked > declaredPacked - packedSize) ||
                    (declaredRaw >= 0 &&
                     completeRaw > declaredRaw - rawSize))
                    return false;
                completePacked += packedSize;
                completeRaw += rawSize;
            } else {
                havePartial = true;
                name += QStringLiteral(".part%1").arg(++segmentIndex,
                                                       2, 10, QLatin1Char('0'));
            }
            if (!addEntry(pos, dataOffset - pos, dataOffset, segmentSize,
                          complete ? rawSize : segmentSize,
                          complete ? HANDLE_METHOD_FPAK_COMPRESSED
                                   : HANDLE_METHOD_UNKNOWN,
                          name, complete, crc32))
                return false;
            if (complete && (dosTime || dosDate))
                entries.last().mtDateTime =
                    XBinary::dosDateTimeToQDateTime(dosDate, dosTime);
            pos = dataOffset + segmentSize;
        }
        if (!segmentIndex && entries.isEmpty()) return false;
        if ((declaredPacked >= 0) && !havePartial &&
            ((completePacked != declaredPacked) ||
             (completeRaw != declaredRaw)))
            return false;
    } else if (ft == FT_FTCOMP) {
        // OS/2 PACK2 (FTCOMP) can concatenate members.  Every member provides
        // absolute offsets for the next member and optional extended
        // attributes, making the directory independently verifiable before
        // the bounded fT19 decoder is invoked.
        qint64 pos = 0;
        while (true) {
            if (!rangeWithin(total, pos, 41) ||
                std::memcmp(p + pos, "\xa5\x96\xfd\xff", 4) != 0 ||
                std::memcmp(p + pos + 24, "FTCOMP\0", 7) != 0)
                return false;
            const qint64 eaOffset = qFromLittleEndian<quint32>(p + pos + 12);
            const qint64 rawSize = qFromLittleEndian<quint32>(p + pos + 16);
            const qint64 nextOffset = qFromLittleEndian<quint32>(p + pos + 20);
            const qint32 nameSize = qFromLittleEndian<quint16>(p + pos + 39);
            if (rawSize < 1 || rawSize > MAX_LEGACY_STORE_SIZE ||
                nameSize < 2 || nameSize > 512 ||
                !rangeWithin(total, pos + 41, nameSize))
                return false;
            const QString name = fixedName(p + pos + 41, nameSize);
            if (name.isEmpty()) return false;
            const qint64 dataOffset = pos + 41 + nameSize;
            const qint64 memberEnd = nextOffset ? nextOffset : total;
            const qint64 dataEnd = eaOffset ? eaOffset : memberEnd;
            if (memberEnd <= pos || memberEnd > total || dataEnd <= dataOffset ||
                dataEnd > memberEnd || !rangeWithin(dataEnd, dataOffset, 8) ||
                std::memcmp(p + dataOffset + 4, "fT19", 4) != 0)
                return false;
            if (!addEntry(pos, dataOffset - pos, dataOffset,
                          dataEnd - dataOffset, rawSize,
                          HANDLE_METHOD_FTCOMP_FT19, name))
                return false;
            if (!nextOffset) break;
            pos = nextOffset;
        }
    } else if (ft == FT_VISE_SFX) {
        // Windows Installer VISE locates its ESIV data through a footer.  A
        // signed executable can append an Authenticode certificate after that
        // footer, so search backwards for a footer whose pointer resolves to a
        // second ESIV header (some releases put an eight-byte SIVM prefix in
        // front of it).
        qint64 headerOffset = -1;
        qint64 footerOffset = data.lastIndexOf("ESIV");
        while (footerOffset >= 0) {
            if (rangeWithin(total, footerOffset, 8)) {
                const qint64 pointed = qFromLittleEndian<quint32>(p + footerOffset + 4);
                if (rangeWithin(total, pointed, 4) &&
                    std::memcmp(p + pointed, "ESIV", 4) == 0) {
                    headerOffset = pointed;
                    break;
                }
                if (rangeWithin(total, pointed, 12) &&
                    std::memcmp(p + pointed, "SIVM", 4) == 0 &&
                    std::memcmp(p + pointed + 8, "ESIV", 4) == 0) {
                    headerOffset = pointed + 8;
                    break;
                }
            }
            footerOffset = data.lastIndexOf("ESIV", footerOffset - 1);
        }
        if (headerOffset < 0 || footerOffset <= headerOffset ||
            !rangeWithin(total, headerOffset, 17))
            return false;

        qint64 initialTable = headerOffset + 16;
        while (initialTable < footerOffset && p[initialTable]) {
            const qint64 skip = p[initialTable] & 0x7fU;
            if (!skip || !rangeWithin(footerOffset, initialTable + 1, skip))
                return false;
            initialTable += 1 + skip;
        }
        if (initialTable >= footerOffset) return false;
        ++initialTable;

        QSet<qint64> usedStreams;
        qint64 initialEnd = 0;
        QList<ViseRecord> initialRecords;
        if (!scanViseServiceTable(data, p, footerOffset, initialTable,
                                  &initialEnd, &initialRecords, pPdStruct))
            return false;
        if (!addEntry.addViseRecords(initialRecords, &usedStreams)) {
            return false;
        }

        // A second service table follows variable installer metadata.  Locate
        // it structurally; complete byte-swapped Deflate records make the
        // validator substantially stronger than a version-specific offset.
        qint64 serviceCandidate = initialEnd;
        while (serviceCandidate < footerOffset - 20) {
            qint64 tableEnd = 0;
            QList<ViseRecord> records;
            if (scanViseServiceTable(data, p, footerOffset,
                                     serviceCandidate, &tableEnd, &records,
                                     pPdStruct)) {
                if (!addEntry.addViseRecords(records, &usedStreams)) {
                    return false;
                }
                serviceCandidate = tableEnd;
            } else {
                ++serviceCandidate;
            }
        }

        // File-object layouts differ slightly between VISE releases.  The
        // stable portion is a type-2 marker, a nearby counted filename, size
        // fields, flags, and an ESIV-relative payload offset.  Accept an object
        // only after its entire payload inflates to the declared size.
        for (qint64 object = initialEnd; object < footerOffset - 160; ++object) {
            if (qFromLittleEndian<quint32>(p + object) != 2) continue;
            bool accepted = false;
            for (qint32 skip = 90; skip <= 114 && !accepted; ++skip) {
                const qint64 stringOffset = object + 4 + skip;
                if (!rangeWithin(footerOffset, stringOffset, 2)) continue;
                const qint32 nameSize = qFromLittleEndian<qint16>(p + stringOffset);
                if (nameSize < 1 || nameSize > 255 ||
                    !rangeWithin(footerOffset, stringOffset + 2,
                                 qint64(nameSize) + 22))
                    continue;
                const QString name = fixedName(p + stringOffset + 2, nameSize);
                if (name.isEmpty()) continue;
                for (qint32 reserved : {4, 6}) {
                    const qint64 fields = stringOffset + 2 + nameSize + reserved;
                    if (!rangeWithin(footerOffset, fields, 16)) continue;
                    const qint64 rawSize = qFromLittleEndian<quint32>(p + fields);
                    const qint64 packedSize = qFromLittleEndian<quint32>(p + fields + 4);
                    const quint32 flags = qFromLittleEndian<quint32>(p + fields + 8);
                    const qint64 relativeOffset = qFromLittleEndian<quint32>(p + fields + 12);
                    if (rawSize < 1 || rawSize > MAX_LEGACY_STORE_SIZE ||
                        packedSize < 2 || (packedSize & 1) || flags != 1 ||
                        relativeOffset > footerOffset - headerOffset)
                        continue;
                    const qint64 dataOffset = headerOffset + relativeOffset;
                    if (usedStreams.contains(dataOffset) ||
                        !rangeWithin(footerOffset, dataOffset, packedSize))
                        continue;
                    const QByteArray packed = data.mid(qint32(dataOffset),
                                                       qint32(packedSize));
                    if (!XViseDeflateDecoder::decode(packed, rawSize, nullptr,
                                                     nullptr, pPdStruct))
                        continue;
                    if (!addEntry(object, fields + 16 - object, dataOffset,
                                  packedSize, rawSize,
                                  HANDLE_METHOD_VISE_DEFLATE, name))
                        return false;
                    usedStreams.insert(dataOffset);
                    accepted = true;
                    break;
                }
            }
        }
    } else if (ft == FT_INSTALLSHIELD_LAUNCHER) {
        // InstallShield's InstallScript-MSI Setup Launcher appends a stored
        // support-file table at the exact PE overlay boundary. The 0x2e-byte
        // header is followed by fixed 0x138-byte records and raw payloads.
        if ((total < 0x200) || (p[0] != 'M') || (p[1] != 'Z')) return false;
        const qint64 peOffset = qFromLittleEndian<quint32>(p + 0x3c);
        if (!rangeWithin(total, peOffset, 24) ||
            (std::memcmp(p + peOffset, "PE\0\0", 4) != 0)) return false;
        const quint16 sectionCount = qFromLittleEndian<quint16>(p + peOffset + 6);
        const quint16 optionalSize = qFromLittleEndian<quint16>(p + peOffset + 20);
        const qint64 sectionTable = peOffset + 24 + optionalSize;
        if (!sectionCount || (sectionCount > 96) ||
            !rangeWithin(total, sectionTable, qint64(sectionCount) * 40)) return false;

        qint64 overlayOffset = 0;
        for (quint32 i = 0; i < sectionCount; ++i) {
            const qint64 row = sectionTable + qint64(i) * 40;
            const qint64 rawSize = qFromLittleEndian<quint32>(p + row + 16);
            const qint64 rawOffset = qFromLittleEndian<quint32>(p + row + 20);
            if ((rawOffset > total) || (rawSize > total - rawOffset)) return false;
            overlayOffset = qMax(overlayOffset, rawOffset + rawSize);
        }
        if (!rangeWithin(total, overlayOffset, 0x2e + 0x138) ||
            (std::memcmp(p + overlayOffset, "InstallShield\0", 14) != 0)) return false;
        const quint32 fileCount = qFromLittleEndian<quint32>(p + overlayOffset + 0x0e);
        if (!fileCount || (fileCount > quint32(MAX_RECORDS))) return false;
        for (qint64 i = overlayOffset + 0x12; i < overlayOffset + 0x2e; ++i) {
            if (p[i] != 0) return false;
        }

        qint64 memberOffset = overlayOffset + 0x2e;
        for (quint32 i = 0; i < fileCount; ++i) {
            if (!rangeWithin(total, memberOffset, 0x138)) return false;
            const uchar *pNameEnd = static_cast<const uchar *>(
                std::memchr(p + memberOffset, 0, 0x10c));
            if (!pNameEnd || (pNameEnd == p + memberOffset)) return false;
            const qint32 nameSize = qint32(pNameEnd - (p + memberOffset));
            for (qint64 j = nameSize; j < 0x10c; ++j) {
                if (p[memberOffset + j] != 0) return false;
            }
            for (qint64 j = 0x110; j < 0x138; ++j) {
                if (p[memberOffset + j] != 0) return false;
            }
            const QString name = fixedName(p + memberOffset, nameSize);
            const qint64 memberSize = qFromLittleEndian<quint32>(p + memberOffset + 0x10c);
            const qint64 dataOffset = memberOffset + 0x138;
            if (name.isEmpty() || (memberSize > MAX_LEGACY_STORE_SIZE) ||
                !rangeWithin(total, dataOffset, memberSize) ||
                !addEntry(memberOffset, 0x138, dataOffset, memberSize,
                          memberSize, HANDLE_METHOD_STORE, name)) return false;
            memberOffset = dataOffset + memberSize;
        }
        if (memberOffset != total) return false;
    } else if (ft == FT_GPINSTALL_SFX) {
        // GP-Install appends one or more independently framed SPIS/LH5 blocks
        // at the exact end of the PE image. Each member has a fixed 25-byte
        // header followed by its ANSI path and a bounded raw LHA -lh5- stream.
        if ((total < 0x100) || (p[0] != 'M') || (p[1] != 'Z')) return false;
        const qint64 peOffset = qFromLittleEndian<quint32>(p + 0x3c);
        if (!rangeWithin(total, peOffset, 24) ||
            (std::memcmp(p + peOffset, "PE\0\0", 4) != 0)) return false;
        const quint16 sectionCount = qFromLittleEndian<quint16>(p + peOffset + 6);
        const quint16 optionalSize = qFromLittleEndian<quint16>(p + peOffset + 20);
        const qint64 sectionTable = peOffset + 24 + optionalSize;
        if (!sectionCount || (sectionCount > 96) ||
            !rangeWithin(total, sectionTable, qint64(sectionCount) * 40)) return false;

        qint64 overlayOffset = 0;
        for (quint32 i = 0; i < sectionCount; ++i) {
            const qint64 row = sectionTable + qint64(i) * 40;
            const qint64 rawSize = qFromLittleEndian<quint32>(p + row + 16);
            const qint64 rawOffset = qFromLittleEndian<quint32>(p + row + 20);
            if ((rawOffset > total) || (rawSize > total - rawOffset)) return false;
            overlayOffset = qMax(overlayOffset, rawOffset + rawSize);
        }
        if (!rangeWithin(total, overlayOffset, 50)) return false;

        qint64 blockOffset = overlayOffset;
        while (blockOffset < total) {
            if (!isPdStructNotCanceled(pPdStruct) ||
                !rangeWithin(total, blockOffset, 25)) return false;
            const qint64 blockSize = qFromLittleEndian<quint32>(p + blockOffset);
            if ((blockSize < 21) || !rangeWithin(total, blockOffset + 4, blockSize) ||
                (std::memcmp(p + blockOffset + 4, "SPIS\x1a", 5) != 0) ||
                (std::memcmp(p + blockOffset + 9, "LH5", 3) != 0)) return false;
            const qint64 blockEnd = blockOffset + 4 + blockSize;
            const quint64 declaredRawSize = qFromLittleEndian<quint32>(p + blockOffset + 12);
            quint64 blockRawSize = 0;
            qint64 memberOffset = blockOffset + 25;
            qint32 blockMembers = 0;

            while (memberOffset < blockEnd) {
                if (!rangeWithin(blockEnd, memberOffset, 25)) return false;
                const qint64 nameSize = qFromLittleEndian<quint16>(p + memberOffset);
                const qint64 rawSize = qFromLittleEndian<quint32>(p + memberOffset + 8);
                const qint64 packedSize = qFromLittleEndian<quint32>(p + memberOffset + 12);
                if ((nameSize < 1) || (nameSize > 1024) ||
                    (p[memberOffset + 16] != 4) || (rawSize < 1) ||
                    (rawSize > MAX_LEGACY_STORE_SIZE) || (packedSize < 1) ||
                    !rangeWithin(blockEnd, memberOffset + 25, nameSize)) return false;
                const qint64 dataOffset = memberOffset + 25 + nameSize;
                if (!rangeWithin(blockEnd, dataOffset, packedSize)) return false;

                bool validName = true;
                for (qint64 i = 0; i < nameSize; ++i) {
                    const quint8 c = p[memberOffset + 25 + i];
                    if ((c == 0) || (c < 0x20) || (c == 0x7f)) {
                        validName = false;
                        break;
                    }
                }
                if (!validName) return false;
                QString name = QString::fromLatin1(
                    reinterpret_cast<const char *>(p + memberOffset + 25),
                    qint32(nameSize));
                name.replace(QLatin1Char('\\'), QLatin1Char('/'));
                if (!addEntry(memberOffset, 25 + nameSize, dataOffset,
                              packedSize, rawSize, HANDLE_METHOD_LZH5, name)) return false;
                blockRawSize += quint64(rawSize);
                if (blockRawSize > 0xffffffffULL) return false;
                memberOffset = dataOffset + packedSize;
                ++blockMembers;
            }
            if (!blockMembers || (memberOffset != blockEnd) ||
                (blockRawSize != declaredRawSize)) return false;
            blockOffset = blockEnd;
        }
        if ((blockOffset != total) || entries.isEmpty()) return false;
    } else if (ft == FT_PIMP_SFX) {
        // Nullsoft PiMP stores a small metadata block and a sequence of zlib
        // members at the exact PE overlay boundary.  Older scripts omit empty
        // metadata fields, so locate the directory by its fully bounded record
        // chain instead of assuming one fixed metadata size.
        if (total < 0x100 || p[0] != 'M' || p[1] != 'Z') return false;
        const qint64 peOffset = qFromLittleEndian<quint32>(p + 0x3c);
        if (!rangeWithin(total, peOffset, 24) ||
            std::memcmp(p + peOffset, "PE\0\0", 4) != 0)
            return false;
        const quint16 sectionCount = qFromLittleEndian<quint16>(p + peOffset + 6);
        const quint16 optionalSize = qFromLittleEndian<quint16>(p + peOffset + 20);
        const qint64 sectionTable = peOffset + 24 + optionalSize;
        if (!sectionCount || sectionCount > 96 ||
            !rangeWithin(total, sectionTable, qint64(sectionCount) * 40))
            return false;
        qint64 overlayOffset = 0;
        for (quint32 i = 0; i < sectionCount; ++i) {
            const qint64 row = sectionTable + qint64(i) * 40;
            const qint64 rawSize = qFromLittleEndian<quint32>(p + row + 16);
            const qint64 rawOffset = qFromLittleEndian<quint32>(p + row + 20);
            if (rawOffset > total || rawSize > total - rawOffset) return false;
            overlayOffset = qMax(overlayOffset, rawOffset + rawSize);
        }
        if (!rangeWithin(total, overlayOffset, 16) ||
            std::memcmp(p + overlayOffset, "PIMPFILE", 8) != 0)
            return false;

        const qint64 searchEnd = qMin(total - 16, overlayOffset + 0x10000);
        bool foundDirectory = false;
        for (qint64 candidate = overlayOffset + 8;
             candidate <= searchEnd && !foundDirectory; ++candidate) {
            const quint32 count = qFromLittleEndian<quint32>(p + candidate);
            if (!count || count > quint32(MAX_RECORDS)) continue;
            qint64 pos = candidate + 4;
            QList<ENTRY> candidateEntries;
            QSet<QString> candidateFiles;
            QSet<QString> candidateDirs;
            QHash<QString, qint32> candidateSuffixes;
            QHash<QString, QString> candidateResolvedDirs;
            bool valid = true;
            for (quint32 i = 0; i < count && valid; ++i) {
                if (!rangeWithin(total, pos, 4)) { valid = false; break; }
                const quint32 nameSize = qFromLittleEndian<quint32>(p + pos);
                const qint64 row = pos;
                pos += 4;
                if (nameSize < 2 || nameSize > 1024 ||
                    !rangeWithin(total, pos, qint64(nameSize) + 8) ||
                    p[pos + nameSize - 1] != 0 ||
                    std::memchr(p + pos, 0, size_t(nameSize - 1))) {
                    valid = false;
                    break;
                }
                QString name = fixedName(p + pos, qint32(nameSize - 1));
                pos += nameSize;
                const qint64 packedSize = qFromLittleEndian<quint32>(p + pos);
                const qint64 rawSize = qFromLittleEndian<quint32>(p + pos + 4);
                pos += 8;
                if (name.isEmpty() || packedSize < 8 || rawSize < 1 ||
                    rawSize > MAX_LEGACY_STORE_SIZE ||
                    !rangeWithin(total, pos, packedSize) || p[pos] != 0x78 ||
                    (((quint16(p[pos]) << 8) | p[pos + 1]) % 31) != 0) {
                    valid = false;
                    break;
                }
                QString unique;
                if (!makeUniquePath(name, &candidateFiles, &candidateDirs,
                                    &candidateSuffixes, &candidateResolvedDirs,
                                    &unique)) {
                    valid = false;
                    break;
                }
                ENTRY e = {};
                e.nHeaderOffset = row;
                e.nHeaderSize = pos - row;
                e.nDataOffset = pos;
                e.nDataSize = packedSize;
                e.nUncompressedSize = rawSize;
                e.handleMethod = HANDLE_METHOD_ZLIB;
                e.sFileName = unique;
                candidateEntries.append(e);
                pos += packedSize;
            }
            if (!valid || candidateEntries.size() != qint32(count) ||
                !rangeWithin(total, pos, 4))
                continue;
            const qint64 commandSize = qFromLittleEndian<quint32>(p + pos);
            if (commandSize < 1 || commandSize > total - pos - 4 ||
                pos + 4 + commandSize != total ||
                p[total - 1] != 0)
                continue;
            entries = candidateEntries;
            foundDirectory = true;
        }
        if (!foundDirectory) return false;
    } else if (ft == FT_ARCV) {
        // ARCV 1.10 contains one file descriptor followed by one CHNK
        // segment.  The header checksum is JAMCRC of the compressed bytes;
        // validating it here prevents damaged partial samples from being
        // presented as extractable members.
        if (total < 64 || std::memcmp(p, "ARCV", 4) != 0 ||
            qFromLittleEndian<quint16>(p + 4) != 0x0110)
            return false;
        const qint64 archiveHeaderSize = qFromLittleEndian<quint16>(p + 6);
        const quint8 nameSize = p[12];
        const qint64 fieldsOffset = 13 + nameSize;
        if (!nameSize || nameSize > 240 ||
            archiveHeaderSize != fieldsOffset + 34 ||
            !rangeWithin(total, 13, nameSize) ||
            !rangeWithin(total, fieldsOffset, 28) ||
            !rangeWithin(total, archiveHeaderSize, 16) ||
            std::memcmp(p + archiveHeaderSize, "CHNK", 4) != 0)
            return false;
        const QString name = fixedName(p + 13, nameSize);
        const qint64 rawSize = qFromLittleEndian<quint32>(p + fieldsOffset);
        const qint64 packedSize =
            qFromLittleEndian<quint32>(p + fieldsOffset + 4);
        const quint32 reportedJamCrc =
            qFromLittleEndian<quint32>(p + fieldsOffset + 24);
        const qint64 chunkHeaderSize =
            qFromLittleEndian<quint16>(p + archiveHeaderSize + 6);
        const qint64 chunkDataSize =
            qFromLittleEndian<quint32>(p + archiveHeaderSize + 12);
        const qint64 dataOffset = archiveHeaderSize + chunkHeaderSize;
        if (name.isEmpty() || rawSize < 1 ||
            rawSize > MAX_LEGACY_STORE_SIZE || packedSize < 1 ||
            packedSize > MAX_LEGACY_STORE_SIZE || chunkHeaderSize < 16 ||
            chunkHeaderSize > 4096 || chunkDataSize != packedSize ||
            !rangeWithin(total, dataOffset, packedSize))
            return false;
        const quint32 calculatedJamCrc = XBinary::_getCRC32(
            reinterpret_cast<const char *>(p + dataOffset),
            qint32(packedSize), 0xffffffffU,
            XBinary::_getCRC32Table_EDB88320());
        if (calculatedJamCrc == reportedJamCrc) {
            // Compact F=32 sub-variant: the header JAMCRC covers the
            // compressed member (matches the reader's original behavior).
            if (!addEntry(0, dataOffset, dataOffset, packedSize, rawSize,
                          HANDLE_METHOD_ARCV_LZHUF, name))
                return false;
        } else {
            // Stock F=60 sub-variant: the header JAMCRC covers the
            // decompressed member instead. Trial-decode and accept only when
            // the output size and JAMCRC both match the header.
            QByteArray baTrial;
            const QByteArray baPacked(reinterpret_cast<const char *>(p + dataOffset), qint32(packedSize));
            if (!XDecompress::decompressArcvLzhuf(baPacked, qint32(rawSize), true, &baTrial, pPdStruct) || (baTrial.size() != rawSize)) return false;
            const quint32 nRawJamCrc =
                XBinary::_getCRC32(baTrial.constData(), qint32(rawSize), 0xffffffffU, XBinary::_getCRC32Table_EDB88320());
            if ((nRawJamCrc != reportedJamCrc) ||
                !addEntry(0, dataOffset, dataOffset, packedSize, rawSize, HANDLE_METHOD_ARCV_LZHUF60, name))
                return false;
        }
    } else if (ft == FT_INSTALIT_DATA) {
        // Instalit .001 media concatenate independent PKWARE DCL streams
        // without a filename catalog. Decode each stream once to recover its
        // exact raw size and bitstream boundary, and expose stable numbered
        // names rather than inventing source filenames that are not present.
        qint64 offset = 0;
        QVector<qint64> streamOffsets;
        QVector<qint64> streamPackedSizes;
        QVector<qint64> streamRawSizes;
        while (offset < total) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            qint64 packedSize = 0;
            qint64 rawSize = 0;
            if (!scanDclStream(p + offset, total - offset, &packedSize,
                               &rawSize)) {
                // Numbered Instalit media can end partway through the next
                // member or with the raw installation manifest. Complete
                // independent prefix streams remain safe.
                if (!streamOffsets.isEmpty()) break;
                return false;
            }
            streamOffsets.append(offset);
            streamPackedSizes.append(packedSize);
            streamRawSizes.append(rawSize);
            offset += packedSize;
        }
        if (streamOffsets.isEmpty()) return false;

        // The raw manifest follows the media data and stores one fixed-width
        // record per original DOS filename. Releases in the corpus use either
        // a 55- or 59-byte record. Select the longest regularly spaced run to
        // avoid treating coincidental 0xf0 bytes in a truncated DCL tail as
        // names, then restore names only when the run covers every stream.
        QHash<qint64, QString> manifestCandidates;
        for (qint64 pos = offset; pos < total - 2; ++pos) {
            if (p[pos] != 0xf0) continue;
            qint32 length = 0;
            while (length < 12 && pos + 1 + length < total &&
                   p[pos + 1 + length])
                ++length;
            if (!length || length > 12 || pos + 1 + length >= total ||
                p[pos + 1 + length])
                continue;
            qint32 dot = -1;
            bool validName = true;
            for (qint32 j = 0; j < length; ++j) {
                const uchar ch = p[pos + 1 + j];
                if (ch == '.') {
                    if (dot >= 0 || j == 0 || j + 1 == length) {
                        validName = false;
                        break;
                    }
                    dot = j;
                } else if (!isDosNameCharacter(ch)) {
                    validName = false;
                    break;
                }
            }
            if (validName && ((dot < 0 && length <= 8) ||
                              (dot > 0 && dot <= 8 &&
                               length - dot - 1 <= 3))) {
                const QString name = fixedName(p + pos + 1, length);
                if (!name.isEmpty()) manifestCandidates.insert(pos, name);
            }
        }

        QList<QString> manifestNames;
        for (QHash<qint64, QString>::const_iterator it =
                 manifestCandidates.constBegin();
             it != manifestCandidates.constEnd(); ++it) {
            for (qint64 stride : {qint64(55), qint64(59)}) {
                QList<QString> run;
                qint64 record = it.key();
                while (manifestCandidates.contains(record)) {
                    run.append(manifestCandidates.value(record));
                    record += stride;
                }
                if (run.size() > manifestNames.size()) manifestNames = run;
            }
        }
        const bool haveNames = manifestNames.size() >= streamOffsets.size();
        for (qint32 i = 0; i < streamOffsets.size(); ++i) {
            const QString name = haveNames
                ? manifestNames.at(i)
                : QStringLiteral("stream_%1.bin")
                      .arg(i + 1, 4, 10, QLatin1Char('0'));
            if (!addEntry(streamOffsets.at(i), 0, streamOffsets.at(i),
                          streamPackedSizes.at(i), streamRawSizes.at(i),
                          HANDLE_METHOD_PKWARE_DCL_IMPLODE, name))
                return false;
        }
    } else if (ft == FT_SOLITAIRE_DELUXE) {
        // Publishing International's installer keeps a fixed product header,
        // 25-byte file rows, and 22-byte component rows on DISK.1.  Member
        // payloads are consecutive PKWARE Data Compression Library streams.
        // Later members may span DISK.1/DISK.2; expose only the complete prefix
        // resident in the supplied volume, as XArchive intentionally never
        // reads an undeclared sibling device.
        if (total < 0x77 || p[0x64] != 0x1a ||
            qFromLittleEndian<quint32>(p + 0x65) != 0x12345678U)
            return false;
        const quint16 componentCount = qFromLittleEndian<quint16>(p + 0x73);
        const quint16 fileCount = qFromLittleEndian<quint16>(p + 0x75);
        const qint64 tableSize = qint64(fileCount) * 25;
        const qint64 componentSize = qint64(componentCount) * 22;
        const qint64 dataOffset = 0x77 + tableSize + componentSize;
        if (!componentCount || componentCount > 256 || !fileCount ||
            fileCount > MAX_RECORDS ||
            !rangeWithin(total, 0x77, tableSize) ||
            !rangeWithin(total, 0x77 + tableSize, componentSize) ||
            dataOffset >= total)
            return false;

        for (quint32 i = 0; i < componentCount; ++i) {
            const qint64 row = 0x77 + tableSize + qint64(i) * 22;
            if (fixedName(p + row, 20).isEmpty()) return false;
        }

        qint64 packedOffset = dataOffset;
        bool encounteredSpanningMember = false;
        for (quint32 i = 0; i < fileCount; ++i) {
            const qint64 row = 0x77 + qint64(i) * 25;
            const QString name = fixedName(p + row, 13);
            const qint64 rawSize = qFromLittleEndian<quint32>(p + row + 13);
            const qint64 packedSize = qFromLittleEndian<quint32>(p + row + 17);
            // The low byte selects the component; the high byte carries
            // installer flags (mandatory/start-menu metadata in this set).
            const quint8 component = p[row + 21];
            if (name.isEmpty() || rawSize < 1 ||
                rawSize > MAX_LEGACY_STORE_SIZE || packedSize < 2 ||
                packedSize > MAX_LEGACY_STORE_SIZE ||
                component >= componentCount ||
                packedOffset > (std::numeric_limits<qint64>::max)() - packedSize)
                return false;

            const bool resident = rangeWithin(total, packedOffset, packedSize);
            if (resident && !encounteredSpanningMember) {
                if (p[packedOffset] > 1 || p[packedOffset + 1] < 4 ||
                    p[packedOffset + 1] > 6 ||
                    !addEntry(row, 25, packedOffset, packedSize, rawSize,
                              HANDLE_METHOD_PKWARE_DCL_IMPLODE, name))
                    return false;
            } else {
                encounteredSpanningMember = true;
            }
            packedOffset += packedSize;
        }
        if (entries.isEmpty()) return false;
    } else if (ft == FT_SCF) {
        if (total < 32 || std::memcmp(p, "\x04\0\0\0", 4) != 0)
            return false;
        qint64 pos = 4;
        while (pos < total) {
            if (!rangeWithin(total, pos, 26)) return false;
            const void *pTerminator = std::memchr(p + pos, 0, 14);
            if (!pTerminator) return false;
            const qint32 nameSize = qint32(
                static_cast<const uchar *>(pTerminator) - (p + pos));
            const QString name = fixedName(p + pos, nameSize);
            const qint64 packedSize = qFromLittleEndian<quint32>(p + pos + 14);
            const qint64 rawSize = qFromLittleEndian<quint32>(p + pos + 18);
            const qint64 dataOffset = pos + 26;
            if (name.isEmpty() || packedSize < 2 || rawSize < 1 ||
                rawSize > MAX_LEGACY_STORE_SIZE ||
                !rangeWithin(total, dataOffset, packedSize) ||
                p[dataOffset] > 1 || p[dataOffset + 1] < 4 ||
                p[dataOffset + 1] > 6 ||
                !addEntry(pos, 26, dataOffset, packedSize, rawSize,
                          HANDLE_METHOD_PKWARE_DCL_IMPLODE, name))
                return false;
            pos = dataOffset + packedSize;
        }
        if (pos != total) return false;
    } else if (ft == FT_PAX) {
        qint64 recordOffset = 0;
        while (recordOffset < total) {
            qint64 dataOffset = 0;
            qint64 rawSize = 0;
            QString name;
            if (!scanPaxRecord(data, recordOffset, &dataOffset, &rawSize,
                               &name)) return false;
            qint64 nextOffset = data.indexOf("LZF0", dataOffset + 1);
            while (nextOffset >= 0) {
                qint64 ignoredHeader = 0;
                qint64 ignoredSize = 0;
                QString ignoredName;
                if (scanPaxRecord(data, nextOffset, &ignoredHeader,
                                  &ignoredSize, &ignoredName)) break;
                nextOffset = data.indexOf("LZF0", nextOffset + 1);
            }
            const qint64 dataEnd = nextOffset >= 0 ? nextOffset : total;
            if (dataEnd <= dataOffset || rawSize >
                (std::numeric_limits<qint32>::max)()) return false;
            QByteArray unpacked;
            qint64 consumedSize = 0;
            const QByteArray packed = data.mid(qint32(dataOffset),
                                               qint32(dataEnd - dataOffset));
            const bool decoded = XPaxDecoder::decode(
                packed, qint32(rawSize), &unpacked, &consumedSize,
                pPdStruct);
            const bool prefixMatches = decoded && unpacked.size() >= 4 &&
                std::memcmp(unpacked.constData(), p + recordOffset + 4, 4) == 0;
            // PAX normally uses only byte-alignment padding, but some sets
            // carry additional per-member bytes before the next LZF0 marker.
            // The decoder's exact output size safely delimits the bitstream.
            const bool paddingValid = decoded && consumedSize > 0 &&
                consumedSize <= packed.size();
            if (!prefixMatches || !paddingValid) {
                // Some distributed PAX sets end in a physically truncated
                // final record.  Earlier independent records remain safe to
                // expose; corruption in the middle is not accepted.
                if (nextOffset < 0 && !entries.isEmpty()) break;
                return false;
            }
            if (!addEntry(recordOffset, dataOffset - recordOffset,
                          dataOffset, dataEnd - dataOffset, rawSize,
                          HANDLE_METHOD_PAX_LZF, name)) return false;
            if (nextOffset < 0) break;
            recordOffset = nextOffset;
        }
    } else if (ft == FT_GPFPACK) {
        qint64 rawSize = 0;
        QString name;
        if (!scanGpfPack(data, &rawSize, &name) ||
            !addEntry(0, 28, 28, total - 28, rawSize,
                      HANDLE_METHOD_GPFPACK_LZW, name)) return false;
    } else if (ft == FT_EMT_IMAGE) {
        // EMT's compressed LOADDSKF derivative stores one RLE record per
        // physical track.  Each regular record expands to 0x2480 bytes:
        // a 126-byte track descriptor, 9216 disk bytes, then the following
        // record's two-byte CRC. The final record omits that trailing CRC.
        static const QByteArray signature("\xf1\x00\x03\x24\x80\x00\x31", 7);
        if (total < 1024 ||
            !data.startsWith(QByteArray("\\\\z\xc5\xd4\xe3\x40\xf0\xf0\xf1\xf0\xf0\xf1", 13)))
            return false;
        qint64 pos = data.indexOf(signature);
        if (pos < 0 || pos > 4096) return false;
        QByteArray firstRecord;
        if (!decodeEmtRecord(data, &pos, 0x2480, &firstRecord) ||
            firstRecord.size() != 0x2480 ||
            quint8(firstRecord.at(1)) != 0 ||
            quint8(firstRecord.at(2)) != 0 ||
            quint8(firstRecord.at(3)) != 0x24 ||
            quint8(firstRecord.at(4)) != 0x80 ||
            quint8(firstRecord.at(5)) != 0 ||
            quint8(firstRecord.at(6)) != 0x31 ||
            quint8(firstRecord.at(7)) != 0 ||
            quint8(firstRecord.at(8)) != 0) return false;
        const uchar *boot = reinterpret_cast<const uchar *>(
            firstRecord.constData()) + 126;
        const quint32 bytesPerSector = qFromLittleEndian<quint16>(boot + 11);
        const quint32 sectorsPerTrack = qFromLittleEndian<quint16>(boot + 24);
        quint32 totalSectors = qFromLittleEndian<quint16>(boot + 19);
        if (!totalSectors)
            totalSectors = qFromLittleEndian<quint32>(boot + 32);
        const qint64 trackSize = qint64(bytesPerSector) * sectorsPerTrack;
        const qint64 rawSize = qint64(bytesPerSector) * totalSectors;
        if (bytesPerSector < 128 || bytesPerSector > 4096 ||
            (bytesPerSector & (bytesPerSector - 1U)) ||
            sectorsPerTrack < 1 || sectorsPerTrack > 63 ||
            trackSize != 9216 || rawSize < trackSize ||
            rawSize > MAX_LEGACY_STORE_SIZE || rawSize % trackSize)
            return false;
        const qint64 trackCount = rawSize / trackSize;
        for (qint64 track = 1; track < trackCount; ++track) {
            QByteArray record;
            if (!isPdStructNotCanceled(pPdStruct) ||
                !decodeEmtRecord(data, &pos,
                                 track + 1 == trackCount ? 0x247e : 0x2480,
                                 &record) || record.size() < 9 ||
                quint8(record.at(1)) != 0 ||
                quint8(record.at(2)) != 0 ||
                quint8(record.at(3)) != 0x24 ||
                quint8(record.at(4)) != 0x80 ||
                quint8(record.at(5)) != 0 ||
                quint8(record.at(6)) != 0x31 ||
                quint8(record.at(7)) != quint8(track / 2) ||
                quint8(record.at(8)) != quint8(track & 1)) return false;
        }
        QString name = XBinary::getDeviceFileBaseName(getDevice());
        if (name.isEmpty()) name = QStringLiteral("disk");
        if (!addEntry(0, 0, 0, total, rawSize, HANDLE_METHOD_EMT_RLE,
                      XBinary::fixFileName(name) + QStringLiteral(".img")))
            return false;
    } else if (ft == FT_INSTALLSHIELD3_ARCHIVE) {
        // InstallShield 3 / ICOMP archives begin with a fixed 58-byte media
        // header followed by independent directory and file tables.  Numbered
        // volumes repeat the complete catalog.  A single XArchive input cannot
        // safely borrow bytes from a missing sibling volume, so expose every
        // member wholly resident in this volume and omit spanning records.
        if (total < 63 || p[0] != 0x13 || p[1] != 0x5d ||
            p[2] != 0x65 || p[3] != 0x8c || p[4] != 0x3a ||
            p[5] != 1 || p[6] != 2) return false;
        const quint16 volumeFlags = qFromLittleEndian<quint16>(p + 10);
        const quint16 fileCount = qFromLittleEndian<quint16>(p + 12);
        const quint8 partId = p[31];
        const qint64 dirsOffset = qFromLittleEndian<quint32>(p + 41);
        const qint64 dirsSize = qFromLittleEndian<quint32>(p + 45);
        const quint16 dirCount = qFromLittleEndian<quint16>(p + 49);
        const qint64 filesOffset = qFromLittleEndian<quint32>(p + 51);
        const qint64 filesSize = qFromLittleEndian<quint32>(p + 55);
        if (!fileCount || fileCount > MAX_RECORDS || !dirCount ||
            !rangeWithin(total, dirsOffset, dirsSize) ||
            !rangeWithin(total, filesOffset, filesSize) ||
            rangesOverlap(dirsOffset, dirsSize, filesOffset, filesSize))
            return false;

        QList<QString> directories;
        qint64 pos = dirsOffset;
        for (quint32 i = 0; i < dirCount; ++i) {
            if (!rangeWithin(total, pos, 7)) return false;
            const quint16 entrySize = qFromLittleEndian<quint16>(p + pos + 2);
            const quint16 nameSize = qFromLittleEndian<quint16>(p + pos + 4);
            if (entrySize != 11 + nameSize ||
                !rangeWithin(total, pos, entrySize) ||
                p[pos + 6 + nameSize] != 0) return false;
            QString directory;
            if (nameSize) {
                directory = fixedName(p + pos + 6, nameSize);
                if (directory.isEmpty()) return false;
            }
            directories.append(directory);
            pos += entrySize;
        }
        if (pos != dirsOffset + dirsSize) return false;

        pos = filesOffset;
        for (quint32 i = 0; i < fileCount; ++i) {
            if (!rangeWithin(total, pos, 31)) return false;
            const quint8 endPart = p[pos];
            const quint16 dirId = qFromLittleEndian<quint16>(p + pos + 1);
            const qint64 rawSize = qFromLittleEndian<quint32>(p + pos + 3);
            const qint64 packedSize = qFromLittleEndian<quint32>(p + pos + 7);
            const qint64 dataOffset = qFromLittleEndian<quint32>(p + pos + 11);
            const quint16 entrySize = qFromLittleEndian<quint16>(p + pos + 23);
            const quint16 flags = qFromLittleEndian<quint16>(p + pos + 25);
            const quint8 startPart = p[pos + 28];
            const quint8 nameSize = p[pos + 29];
            if (dirId >= directories.size() || !nameSize ||
                entrySize != 43 + nameSize ||
                !rangeWithin(total, pos, entrySize) ||
                p[pos + 30 + nameSize] != 0) return false;
            const QString leaf = fixedName(p + pos + 30, nameSize);
            if (leaf.isEmpty()) return false;

            bool local = true;
            if (volumeFlags & 3U)
                local = startPart == partId && endPart == partId;
            if (local && !(flags & 0x20U)) {
                const bool stored = flags & 0x10U;
                if (!rangeWithin(total, dataOffset, packedSize) ||
                    (stored && packedSize != rawSize) ||
                    (!stored && (packedSize < 4 ||
                     (p[dataOffset] != 0 && p[dataOffset] != 1) ||
                     p[dataOffset + 1] < 4 || p[dataOffset + 1] > 6)) ||
                    rawSize > MAX_LEGACY_STORE_SIZE) return false;
                QString name = leaf;
                if (!directories.at(dirId).isEmpty())
                    name = directories.at(dirId) + QLatin1Char('/') + leaf;
                if (!addEntry(pos, entrySize, dataOffset, packedSize, rawSize,
                              stored ? HANDLE_METHOD_STORE
                                     : HANDLE_METHOD_PKWARE_DCL_IMPLODE,
                              name)) return false;
            }
            pos += entrySize;
        }
        if (pos != filesOffset + filesSize || entries.isEmpty()) return false;
    } else if (ft == FT_FINSTALL_ARCHIVE) {
        // F Install 2 places a compact table immediately before its stored
        // members. Each row is absolute offset, length, and a fixed DOS name.
        if (total < 42 || std::memcmp(p, "\x01" "F Install 2", 12) != 0)
            return false;
        const qint64 count = qFromLittleEndian<quint16>(p + 16);
        const qint64 tableEnd = 18 + count * 24;
        if (count < 1 || count > MAX_RECORDS || tableEnd > total) return false;
        qint64 expectedOffset = tableEnd;
        for (qint64 i = 0; i < count; ++i) {
            const qint64 row = 18 + i * 24;
            const qint64 dataOffset = qFromLittleEndian<quint32>(p + row);
            const qint64 dataSize = qFromLittleEndian<quint32>(p + row + 4);
            const QString name = fixedName(p + row + 8, 16);
            if (dataOffset != expectedOffset || dataSize < 0 ||
                !rangeWithin(total, dataOffset, dataSize) ||
                !addEntry(row, 24, dataOffset, dataSize, dataSize,
                          HANDLE_METHOD_STORE, name)) return false;
            expectedOffset += dataSize;
        }
        if (expectedOffset != total) return false;
    } else if (ft == FT_IS_SKIN) {
        // InstallShield setup skins concatenate obfuscated
        // name-ASCIIZ/decimal-size-ASCIIZ/stored-data records. The XOR key
        // phase is the absolute file offset, including member data.
        qint64 pos = 0;
        while (pos < total) {
            if (entries.size() >= MAX_RECORDS) return false;
            const qint64 nameOffset = pos;
            QByteArray decodedName;
            decodedName.reserve(260);
            bool nameTerminated = false;
            while (pos < total) {
                const quint8 value = decodeIsSkinByte(p[pos], pos);
                ++pos;
                if (!value) {
                    nameTerminated = true;
                    break;
                }
                if (decodedName.size() >= 260 ||
                    value < 0x20U || value > 0x7eU) return false;
                decodedName.append(char(value));
            }
            if (!nameTerminated || decodedName.isEmpty()) return false;
            const QString name = fixedName(
                reinterpret_cast<const uchar *>(decodedName.constData()),
                decodedName.size());
            if (name.isEmpty()) return false;

            qint64 dataSize = 0;
            qint32 digits = 0;
            bool sizeTerminated = false;
            while (pos < total) {
                const quint8 value = decodeIsSkinByte(p[pos], pos);
                ++pos;
                if (!value) {
                    sizeTerminated = true;
                    break;
                }
                if (digits >= 10 || value < '0' || value > '9') return false;
                dataSize = dataSize * 10 + (value - '0');
                ++digits;
            }
            if (!sizeTerminated || !digits) return false;

            const qint64 dataOffset = pos;
            if (dataSize > total - dataOffset ||
                !addEntry(nameOffset, dataOffset - nameOffset,
                          dataOffset, dataSize, dataSize,
                          HANDLE_METHOD_IS_SKIN_XOR, name)) return false;
            pos += dataSize;
        }
        if (pos != total || entries.isEmpty()) return false;
    } else if (ft == FT_IS_STORED) {
        // A small Atari distribution wrapper obfuscates an otherwise stored
        // 68000 data stream with a fixed A9 XOR byte.
        if (total < 16 || p[0] != 0xe7 || p[1] != 0x50 || p[2] != 0xa9 ||
            p[3] != 0xae || p[6] != 0xe7 || p[7] != 0x50 ||
            p[8] != 0xa9 || p[9] != 0xae || p[12] != 0xdd ||
            p[13] != 0xc1 || p[14] != 0xc0) return false;
        QString name = XBinary::getDeviceFileBaseName(getDevice());
        if (name.isEmpty()) name = QStringLiteral("decoded");
        if (!addEntry(0, 0, 0, total, total, HANDLE_METHOD_XOR_A9,
                      XBinary::fixFileName(name) + QStringLiteral(".bin")))
            return false;
    } else if (ft == FT_STUNTS_DSI) {
        // DSI resources normally use a multi-pass header: high bit, pass
        // count, final 24-bit size, then the first pass header.  The format
        // has no textual magic, so validate the first codec table as well as
        // all declared limits before accepting it.
        if (total < 9) return false;
        qint64 pos = 0;
        const quint8 first = p[pos];
        qint32 passes = 1;
        qint64 finalSize = 0;
        if (first & 0x80U) {
            passes = first & 0x7fU;
            if (passes < 1 || passes > 8) return false;
            finalSize = qint64(p[1]) | (qint64(p[2]) << 8) |
                        (qint64(p[3]) << 16);
            pos = 4;
        }
        if (!rangeWithin(total, pos, 4)) return false;
        const quint8 type = p[pos++];
        const qint64 passSize = qint64(p[pos]) |
                                (qint64(p[pos + 1]) << 8) |
                                (qint64(p[pos + 2]) << 16);
        pos += 3;
        if ((type != 1 && type != 2) || passSize < 1 ||
            passSize > MAX_LEGACY_STORE_SIZE ||
            (finalSize && (finalSize < 1 ||
                           finalSize > MAX_LEGACY_STORE_SIZE))) return false;
        if (!finalSize) finalSize = passSize;

        if (type == 2) {
            if (!rangeWithin(total, pos, 1)) return false;
            const quint8 widthsHeader = p[pos++];
            const quint8 widthsLen = widthsHeader & 0x7fU;
            if (!widthsLen || widthsLen > 15 ||
                !rangeWithin(total, pos, widthsLen)) return false;
            qint64 alphabetSize = 0;
            qint64 codeSpace = 0;
            for (qint32 i = 0; i < widthsLen; ++i) {
                codeSpace = codeSpace * 2 + p[pos + i];
                alphabetSize += p[pos + i];
                if (alphabetSize > 256 || codeSpace > 0xffff) return false;
            }
            pos += widthsLen;
            if (alphabetSize < 2 || !rangeWithin(total, pos, alphabetSize + 2))
                return false;
        } else {
            if (!rangeWithin(total, pos, 5)) return false;
            const quint8 escapeCount = p[pos + 4] & 0x7fU;
            if (escapeCount < 1 || escapeCount > 10 ||
                !rangeWithin(total, pos, 5 + escapeCount)) return false;
        }

        QString name = XBinary::getDeviceFileBaseName(getDevice());
        if (name.isEmpty()) name = QStringLiteral("resource");
        const QString sourceName =
            XBinary::getDeviceFileName(getDevice()).toLower();
        QString suffix = QStringLiteral(".bin");
        if (sourceName.endsWith(QStringLiteral(".pes")))
            suffix = QStringLiteral(".esh");
        else if (sourceName.endsWith(QStringLiteral(".pvs")))
            suffix = QStringLiteral(".vsh");
        else if (sourceName.endsWith(QStringLiteral(".p3s")))
            suffix = QStringLiteral(".3sh");
        else if (sourceName.endsWith(QStringLiteral(".pre")))
            suffix = QStringLiteral(".res");
        if (!addEntry(0, pos, 0, total, finalSize,
                      HANDLE_METHOD_STUNTS_DSI,
                      XBinary::fixFileName(name) + suffix)) return false;
    } else if (ft == FT_EPFS_ARCHIVE) {
        // EPFS keeps its member data consecutively after the 11-byte header;
        // its table is at a header-provided offset and has fixed 22-byte rows.
        if (total < 33 || std::memcmp(p, "EPFS", 4) != 0) return false;
        const qint64 tableOff = qFromLittleEndian<quint32>(p + 4);
        const qint64 count = qFromLittleEndian<quint16>(p + 9);
        if (count < 1 || count > MAX_RECORDS || tableOff < 11 ||
            !rangeWithin(total, tableOff, count * 22)) return false;
        qint64 dataOff = 11;
        for (qint64 i = 0; i < count; ++i) {
            const qint64 pos = tableOff + i * 22;
            const QString name = fixedName(p + pos, 13);
            const quint8 flags = p[pos + 13];
            const qint64 packedSize = qFromLittleEndian<quint32>(p + pos + 14);
            const qint64 rawSize = qFromLittleEndian<quint32>(p + pos + 18);
            if ((flags & ~quint8(1)) || packedSize < 0 ||
                (flags && rawSize <= 0) ||
                !rangeWithin(total, dataOff, packedSize) ||
                dataOff + packedSize > tableOff ||
                !addEntry(pos, 22, dataOff, packedSize,
                          flags ? rawSize : packedSize,
                          flags ? HANDLE_METHOD_EPFS_LZW : HANDLE_METHOD_STORE,
                          name)) return false;
            dataOff += packedSize;
        }
        if (dataOff > tableOff) return false;
    } else if (ft == FT_COMPAQ_LZH) {
        // Compaq's driver/software distribution wrapper contains a fixed
        // 29-byte header followed by a raw LHA -lh1- (LZHUF) member.  Bytes
        // 7..19 hold the original DOS name and bytes 25..28 its exact size.
        if (total <= 29 || std::memcmp(p, "CPQ_LZH", 7) != 0) return false;
        const QString name = fixedName(p + 7, 13);
        const qint64 rawSize = qFromLittleEndian<quint32>(p + 25);
        if (name.isEmpty() || rawSize <= 0 ||
            rawSize > MAX_LEGACY_STORE_SIZE ||
            !addEntry(0, 29, 29, total - 29, rawSize,
                      HANDLE_METHOD_LZH1, name)) return false;
    } else if (ft == FT_INSA) {
        // INSA installer data starts with a u16 version and then concatenates
        // size-delimited LZHUF streams. Compressed sizes are implicit: decode
        // each record to its declared output count and byte-align the exact
        // number of consumed input bits to locate the following record.
        if (total < 7 || qFromLittleEndian<quint16>(p) != 1) return false;
        qint64 pos = 2;
        qint32 recordIndex = 0;
        while (pos < total) {
            if (!isPdStructNotCanceled(pPdStruct) ||
                !rangeWithin(total, pos, 4)) return false;
            const qint64 recordStart = pos;
            const qint64 rawSize = qFromLittleEndian<quint32>(p + pos);
            pos += 4;
            if (rawSize < 1 || rawSize > MAX_LEGACY_STORE_SIZE) return false;

            const qint64 consumed = XLZHDecoder::lh1MeasureStream(
                p + pos, total - pos, rawSize, pPdStruct);
            if (consumed < 1 || !rangeWithin(total, pos, consumed) ||
                !addEntry(recordIndex ? recordStart : 0,
                          recordIndex ? 4 : 6, pos, consumed, rawSize,
                          HANDLE_METHOD_LZH1,
                          QStringLiteral("File_%1.bin").arg(recordIndex + 1))) {
                return false;
            }
            pos += consumed;
            if (++recordIndex > MAX_RECORDS) return false;
        }
        if (recordIndex < 1 || pos != total) return false;
    } else if (ft == FT_INSTALLSHIELD_BOOT) {
        qint64 pos = 0;
        qint32 recordIndex = 0;
        while (pos < total) {
            const qint64 recordStart = pos;
            QString packedName;
            QString outputName;
            QString version;
            QString packedSizeText;
            if (!readLegacyCString(data, p, total, &pos, &packedName) ||
                !readLegacyCString(data, p, total, &pos, &outputName) ||
                !readLegacyCString(data, p, total, &pos, &version) ||
                !readLegacyCString(data, p, total, &pos,
                                   &packedSizeText))
                return false;
            bool ok = false;
            const qint64 packedSize = packedSizeText.toLongLong(&ok, 10);
            if (!ok || packedSize < 15 || !rangeWithin(total, pos, packedSize) ||
                std::memcmp(p + pos, "SZDD\x88\xf0\x27\x33", 8) != 0)
                return false;
            const qint64 rawSize = qFromLittleEndian<quint32>(p + pos + 10);
            if (rawSize <= 0 || rawSize > MAX_LEGACY_STORE_SIZE ||
                !addEntry(recordStart, pos + 14 - recordStart, pos + 14,
                          packedSize - 14, rawSize,
                          HANDLE_METHOD_LZSS_SZDD, outputName)) return false;
            pos += packedSize;
            if (++recordIndex > MAX_RECORDS) return false;
        }
        if (recordIndex < 1 || pos != total) return false;
    } else if (ft == FT_SABDU_IMAGE) {
        if (total <= 46 || std::memcmp(p, "SAB Diskette Utility\0", 21) != 0 ||
            p[25] != 0 || p[22] != '.' ||
            p[21] < '0' || p[21] > '9' ||
            p[23] < '0' || p[23] > '9' ||
            p[24] < '0' || p[24] > '9') return false;
        const qint64 cylinders = qFromLittleEndian<quint16>(p + 30);
        const qint64 heads = qFromLittleEndian<quint16>(p + 32);
        const qint64 sectors = qFromLittleEndian<quint16>(p + 34);
        const qint64 sectorSize = qFromLittleEndian<quint16>(p + 42);
        const qint64 trackSize = qFromLittleEndian<quint16>(p + 44);
        const qint64 payloadSize = total - 46;
        if (cylinders < 1 || cylinders > 255 || heads < 1 || heads > 2 ||
            sectors < 1 || sectors > 64 || sectorSize < 128 ||
            sectorSize > 4096 || (sectorSize & (sectorSize - 1)) ||
            trackSize != sectors * sectorSize ||
            (payloadSize % sectorSize) != 0 ||
            payloadSize > cylinders * heads * trackSize) return false;
        QString name = XBinary::getDeviceFileBaseName(getDevice());
        if (name.isEmpty()) name = QStringLiteral("disk");
        if (!addEntry(0, 46, 46, payloadSize, payloadSize,
                      HANDLE_METHOD_STORE,
                      XBinary::fixFileName(name) + QStringLiteral(".img")))
            return false;
    } else if (ft == FT_LPAK) {
        if (total <= 8 || std::memcmp(p, "LPAK", 4) != 0) return false;
        const qint64 rawSize = qFromBigEndian<quint32>(p + 4);
        if (rawSize <= 0 || rawSize > MAX_LEGACY_STORE_SIZE) return false;
        QString name = XBinary::getDeviceFileBaseName(getDevice());
        if (name.isEmpty()) name = QStringLiteral("unpacked");
        if (!addEntry(0, 8, 8, total - 8, rawSize,
                      HANDLE_METHOD_LPAK_LZSS, XBinary::fixFileName(name)))
            return false;
    } else if (ft == FT_SHRINKWRAP_IMAGE) {
        if (total < 84) return false;
        const qint32 nameLen = p[0];
        if (nameLen < 1 || nameLen > 63) return false;
        const qint64 dataSize = qFromBigEndian<quint32>(p + 64);
        const qint64 resourceSize = qFromBigEndian<quint32>(p + 68);
        if (dataSize <= 0 || dataSize > total - 84 ||
            resourceSize > total - 84 - dataSize ||
            total - (84 + dataSize + resourceSize) > 65536) return false;
        QString name = macName(p + 1, nameLen);
        if (name.isEmpty()) return false;
        if (!addEntry(0, 84, 84, dataSize, dataSize,
                      HANDLE_METHOD_STORE, name + QStringLiteral(".img")))
            return false;
        if (resourceSize &&
            !addEntry(0, 84, 84 + dataSize, resourceSize, resourceSize,
                      HANDLE_METHOD_STORE, name + QStringLiteral(".rsrc")))
            return false;
        const qint64 trailing = total - (84 + dataSize + resourceSize);
        if (trailing &&
            !addEntry(0, 84, 84 + dataSize + resourceSize, trailing,
                      trailing, HANDLE_METHOD_STORE,
                      name + QStringLiteral(".trailer"))) return false;
    } else if (ft == FT_KA_ARCHIVE) {
        if (total < 22 || std::memcmp(p, "KA Archive\0", 11) != 0)
            return false;
        for (qint32 i = 11; i < 20; ++i) if (p[i]) return false;
        const qint64 count = qFromLittleEndian<quint16>(p + 20);
        const qint64 tableEnd = 22 + count * 21;
        if (count < 1 || count > MAX_RECORDS || tableEnd > total)
            return false;
        qint64 dataOff = tableEnd;
        for (qint64 i = 0; i < count; ++i) {
            const qint64 pos = 22 + i * 21;
            const QString name = fixedName(p + pos, 17);
            const qint64 size = qFromLittleEndian<quint32>(p + pos + 17);
            if (!rangeWithin(total, dataOff, size) ||
                !addEntry(pos, 21, dataOff, size, size,
                          HANDLE_METHOD_STORE, name)) return false;
            dataOff += size;
        }
        if (dataOff != total) return false;
    } else if (ft == FT_LEGACY_CAT) {
        const qint64 count = qFromLittleEndian<quint16>(p);
        const qint64 tableEnd = 2 + count * 24;
        if (count < 1 || count > MAX_RECORDS || tableEnd > total)
            return false;
        qint64 expected = tableEnd;
        for (qint64 i = 0; i < count; ++i) {
            const qint64 pos = 2 + i * 24;
            const QString name = fixedName(p + pos, 12);
            const qint64 size = qFromLittleEndian<quint32>(p + pos + 16);
            const qint64 off = qFromLittleEndian<quint32>(p + pos + 20);
            if (off != expected || !rangeWithin(total, off, size) ||
                !addEntry(pos, 24, off, size, size,
                          HANDLE_METHOD_STORE, name)) return false;
            expected += size;
        }
        if (expected != total) return false;
    } else if (ft == FT_MLB_ARCHIVE) {
        const qint64 count = qFromLittleEndian<quint16>(p);
        const quint16 version = qFromLittleEndian<quint16>(p + 2);
        const qint64 tableEnd = 4 + count * 21;
        if (version != 6 || count < 1 || count > MAX_RECORDS ||
            tableEnd > total) return false;
        qint64 previousEnd = tableEnd;
        for (qint64 i = 0; i < count; ++i) {
            const qint64 pos = 4 + i * 21;
            const qint64 off = qFromLittleEndian<quint32>(p + pos);
            const qint64 size = qFromLittleEndian<quint32>(p + pos + 4);
            const QString name = fixedName(p + pos + 8, 13);
            if (off < previousEnd || !rangeWithin(total, off, size) ||
                !addEntry(pos, 21, off, size, size,
                          HANDLE_METHOD_STORE, name)) return false;
            previousEnd = off + size;
        }
        if (previousEnd != total) return false;
    } else if (ft == FT_LEGACY_RES) {
        const qint64 count = qFromLittleEndian<quint16>(p);
        const qint64 tableEnd = 2 + count * 22;
        if (count < 1 || count > MAX_RECORDS || tableEnd > total)
            return false;
        qint64 expected = tableEnd;
        for (qint64 i = 0; i < count; ++i) {
            const qint64 pos = 2 + i * 22;
            const qint32 nameLen = p[pos];
            if (nameLen < 1 || nameLen > 12) return false;
            for (qint32 j = nameLen; j < 12; ++j)
                if (p[pos + 1 + j]) return false;
            const QString name = fixedName(p + pos + 1, nameLen);
            const qint64 off = qFromLittleEndian<quint32>(p + pos + 13);
            const qint64 size = qFromLittleEndian<quint32>(p + pos + 17);
            if (off != expected || !rangeWithin(total, off, size) ||
                !addEntry(pos, 22, off, size, size,
                          HANDLE_METHOD_STORE, name)) return false;
            expected += size;
        }
        if (expected != total) return false;
    } else if (ft == FT_LEGACY_RSC) {
        const qint64 count = qFromLittleEndian<quint16>(p);
        const qint64 tableEnd = 2 + count * 25;
        if (count < 1 || count > MAX_RECORDS || tableEnd > total)
            return false;
        qint64 expected = tableEnd;
        for (qint64 i = 0; i < count; ++i) {
            const qint64 pos = 2 + i * 25;
            const qint64 size = qFromLittleEndian<quint32>(p + pos);
            const qint64 off = qFromLittleEndian<quint32>(p + pos + 8);
            const QString name = fixedName(p + pos + 12, 13);
            if (off != expected || !rangeWithin(total, off, size) ||
                !addEntry(pos, 25, off, size, size,
                          HANDLE_METHOD_STORE, name)) return false;
            expected += size;
        }
        if (expected != total) return false;
    } else {
        return false;
    }

    if (entries.isEmpty()) return false;
    if (pEntries) *pEntries = entries;
    if (pArchiveEnd) *pArchiveEnd = archiveEnd;
    return true;
}
