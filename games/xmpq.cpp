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
#include "xmpq.h"

#include "../xdecompress.h"

#include <QBuffer>
#include <QHash>
#include <QPointer>
#include <QSet>
#include <QtEndian>
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace {
const quint32 MPQ_HASH_ENTRY_DELETED = 0xFFFFFFFEU;
const quint32 MPQ_HASH_ENTRY_FREE = 0xFFFFFFFFU;
const quint32 MPQ_FILE_IMPLODE = 0x00000100U;
const quint32 MPQ_FILE_COMPRESS = 0x00000200U;
const quint32 MPQ_FILE_ENCRYPTED = 0x00010000U;
const quint32 MPQ_FILE_FIX_KEY = 0x00020000U;
const quint32 MPQ_FILE_PATCH_FILE = 0x00100000U;
const quint32 MPQ_FILE_SINGLE_UNIT = 0x01000000U;
const quint32 MPQ_FILE_DELETE_MARKER = 0x02000000U;
const quint32 MPQ_FILE_SECTOR_CRC = 0x04000000U;
const quint32 MPQ_FILE_EXISTS = 0x80000000U;
const quint32 MPQ_FILE_COMPRESS_MASK =
    MPQ_FILE_IMPLODE | MPQ_FILE_COMPRESS;

const quint32 MPQ_HASH_TABLE_KEY = 0xC3AF3770U;
const quint32 MPQ_BLOCK_TABLE_KEY = 0xEC83B3A3U;
const qint64 MPQ_HEADER_SEARCH_LIMIT = 0x01000000;
const quint32 MPQ_MAX_TABLE_ENTRIES = 0x00080000U;
const qint64 MPQ_MAX_LISTFILE_SIZE = 0x04000000;

class MpqDevicePositionGuard {
public:
    explicit MpqDevicePositionGuard(QIODevice *pDevice)
        : m_pDevice(pDevice), m_nPosition(-1), m_bRestored(false)
    {
        if (m_pDevice && !m_pDevice->isSequential())
            m_nPosition = m_pDevice->pos();
    }

    ~MpqDevicePositionGuard() { restore(); }

    bool isValid() const
    {
        return m_pDevice && (m_nPosition >= 0);
    }

    bool restore()
    {
        if (m_bRestored) return true;
        m_bRestored = true;
        if (!m_pDevice || (m_nPosition < 0)) return false;
        return m_pDevice->seek(m_nPosition) &&
               (m_pDevice->pos() == m_nPosition);
    }

private:
    QPointer<QIODevice> m_pDevice;
    qint64 m_nPosition;
    bool m_bRestored;
};

quint16 mpqReadLE16(const uchar *pData)
{
    return qFromLittleEndian<quint16>(pData);
}

quint32 mpqReadLE32(const uchar *pData)
{
    return qFromLittleEndian<quint32>(pData);
}

quint64 mpqReadLE64(const uchar *pData)
{
    return qFromLittleEndian<quint64>(pData);
}

void mpqWriteLE32(uchar *pData, quint32 nValue)
{
    qToLittleEndian<quint32>(nValue, pData);
}

bool mpqRangeWithin(quint64 nTotalSize, quint64 nOffset, quint64 nSize)
{
    return (nOffset <= nTotalSize) &&
           (nSize <= (nTotalSize - nOffset));
}

bool mpqToQint64(quint64 nValue, qint64 *pResult)
{
    if (!pResult ||
        (nValue > static_cast<quint64>(
             (std::numeric_limits<qint64>::max)()))) {
        return false;
    }
    *pResult = static_cast<qint64>(nValue);
    return true;
}

const std::array<quint32, 0x500> &mpqCryptTable()
{
    static const std::array<quint32, 0x500> table = []() {
        std::array<quint32, 0x500> result = {};
        quint32 nSeed = 0x00100001U;
        for (quint32 i = 0; i < 0x100; ++i) {
            quint32 nIndex = i;
            for (quint32 j = 0; j < 5; ++j, nIndex += 0x100) {
                nSeed = (nSeed * 125U + 3U) % 0x002AAAABU;
                const quint32 nHigh = (nSeed & 0xFFFFU) << 16;
                nSeed = (nSeed * 125U + 3U) % 0x002AAAABU;
                result[nIndex] = nHigh | (nSeed & 0xFFFFU);
            }
        }
        return result;
    }();
    return table;
}

quint8 mpqNormalizeHashCharacter(quint8 nCharacter)
{
    if (nCharacter == '/') return '\\';
    if ((nCharacter >= 'a') && (nCharacter <= 'z'))
        return nCharacter - ('a' - 'A');
    return nCharacter;
}

quint32 mpqHashString(const QByteArray &baString, quint32 nHashType)
{
    const std::array<quint32, 0x500> &table = mpqCryptTable();
    quint32 nSeed1 = 0x7FED7FEDU;
    quint32 nSeed2 = 0xEEEEEEEEU;
    for (int i = 0; i < baString.size(); ++i) {
        const quint8 nCharacter = mpqNormalizeHashCharacter(
            static_cast<quint8>(baString.at(i)));
        nSeed1 = table[(nHashType << 8) + nCharacter] ^
                 (nSeed1 + nSeed2);
        nSeed2 = static_cast<quint32>(nCharacter) + nSeed1 + nSeed2 +
                 (nSeed2 << 5) + 3U;
    }
    return nSeed1;
}

QByteArray mpqPlainFileName(const QByteArray &baFileName)
{
    const int nSlash = qMax(baFileName.lastIndexOf('/'),
                            baFileName.lastIndexOf('\\'));
    return (nSlash >= 0) ? baFileName.mid(nSlash + 1) : baFileName;
}

quint32 mpqFileKey(const QByteArray &baFileName, quint64 nFileOffset,
                   quint32 nFileSize, quint32 nFlags)
{
    quint32 nKey = mpqHashString(mpqPlainFileName(baFileName), 3);
    if (nFlags & MPQ_FILE_FIX_KEY)
        nKey = (nKey + static_cast<quint32>(nFileOffset)) ^ nFileSize;
    return nKey;
}

bool mpqDecryptBlock(QByteArray *pData, quint32 nKey)
{
    if (!pData) return false;
    const std::array<quint32, 0x500> &table = mpqCryptTable();
    quint32 nSeed2 = 0xEEEEEEEEU;
    uchar *pBytes = reinterpret_cast<uchar *>(pData->data());
    // Storm encryption operates on complete DWORDs.  A member sector may
    // have one to three trailing bytes; those bytes are stored verbatim.
    for (int i = 0; i + 3 < pData->size(); i += 4) {
        nSeed2 += table[0x400U + (nKey & 0xFFU)];
        const quint32 nEncrypted = mpqReadLE32(pBytes + i);
        const quint32 nDecrypted = nEncrypted ^ (nKey + nSeed2);
        mpqWriteLE32(pBytes + i, nDecrypted);
        nKey = ((~nKey << 21) + 0x11111111U) | (nKey >> 11);
        nSeed2 = nDecrypted + nSeed2 + (nSeed2 << 5) + 3U;
    }
    return true;
}

bool mpqDetectFileKeyBySectorTable(const QByteArray &baEncrypted,
                                   quint32 nSectorSize,
                                   quint32 nExpectedFirstOffset,
                                   quint32 *pFileKey)
{
    if (pFileKey) *pFileKey = 0;
    if (!pFileKey || (baEncrypted.size() < 8)) return false;

    const uchar *pData = reinterpret_cast<const uchar *>(
        baEncrypted.constData());
    const quint32 nEncrypted0 = mpqReadLE32(pData);
    const quint32 nEncrypted1 = mpqReadLE32(pData + 4);
    const std::array<quint32, 0x500> &table = mpqCryptTable();

    for (quint32 nDelta = 0; nDelta < 4; ++nDelta) {
        const quint32 nPlain0 = nExpectedFirstOffset + nDelta;
        const quint32 nPlain1Maximum = nSectorSize + nPlain0;
        const quint32 nKeyPlusSeed =
            (nEncrypted0 ^ nPlain0) - 0xEEEEEEEEU;
        for (quint32 i = 0; i < 0x100; ++i) {
            quint32 nKey = nKeyPlusSeed - table[0x400U + i];
            quint32 nSeed2 = 0xEEEEEEEEU;
            nSeed2 += table[0x400U + (nKey & 0xFFU)];
            const quint32 nDecoded0 =
                nEncrypted0 ^ (nKey + nSeed2);
            if (nDecoded0 != nPlain0) continue;

            const quint32 nSavedKey = nKey + 1U;
            nKey = ((~nKey << 21) + 0x11111111U) | (nKey >> 11);
            nSeed2 = nDecoded0 + nSeed2 + (nSeed2 << 5) + 3U;
            nSeed2 += table[0x400U + (nKey & 0xFFU)];
            const quint32 nDecoded1 =
                nEncrypted1 ^ (nKey + nSeed2);
            if ((nDecoded1 >= nDecoded0) &&
                (nDecoded1 <= nPlain1Maximum)) {
                *pFileKey = nSavedKey;
                return true;
            }
        }
    }
    return false;
}

bool mpqDetectFileKeyByKnownContent(const QByteArray &baEncrypted,
                                    quint32 nPlain0, quint32 nPlain1,
                                    quint32 *pFileKey)
{
    if (pFileKey) *pFileKey = 0;
    if (!pFileKey || (baEncrypted.size() < 8)) return false;
    const uchar *pData = reinterpret_cast<const uchar *>(
        baEncrypted.constData());
    const quint32 nEncrypted0 = mpqReadLE32(pData);
    const quint32 nEncrypted1 = mpqReadLE32(pData + 4);
    const quint32 nKeyPlusSeed =
        (nEncrypted0 ^ nPlain0) - 0xEEEEEEEEU;
    const std::array<quint32, 0x500> &table = mpqCryptTable();

    for (quint32 i = 0; i < 0x100; ++i) {
        quint32 nKey = nKeyPlusSeed - table[0x400U + i];
        quint32 nSeed2 = 0xEEEEEEEEU;
        nSeed2 += table[0x400U + (nKey & 0xFFU)];
        const quint32 nDecoded0 = nEncrypted0 ^ (nKey + nSeed2);
        if (nDecoded0 != nPlain0) continue;
        const quint32 nSavedKey = nKey;
        nKey = ((~nKey << 21) + 0x11111111U) | (nKey >> 11);
        nSeed2 = nDecoded0 + nSeed2 + (nSeed2 << 5) + 3U;
        nSeed2 += table[0x400U + (nKey & 0xFFU)];
        if ((nEncrypted1 ^ (nKey + nSeed2)) == nPlain1) {
            *pFileKey = nSavedKey;
            return true;
        }
    }
    return false;
}

bool mpqDetectFileKeyByMagic(const QByteArray &baEncrypted,
                             quint32 nFileSize, quint32 *pFileKey)
{
    // A stored, encrypted MPQ can itself contain another MPQ.  The classic
    // header gives us two exact plaintext DWORDs, and the remaining header
    // fields provide strong validation before the recovered key is accepted.
    // This is intentionally format-derived; no archive-specific filenames or
    // key values are needed.
    quint32 nNestedMpqKey = 0;
    if (mpqDetectFileKeyByKnownContent(
            baEncrypted, 0x1A51504DU, 0x00000020U,
            &nNestedMpqKey)) {
        QByteArray baHeader = baEncrypted.left(32);
        if ((baHeader.size() == 32) &&
            mpqDecryptBlock(&baHeader, nNestedMpqKey)) {
            const uchar *pHeader = reinterpret_cast<const uchar *>(
                baHeader.constData());
            const quint32 nHeaderSize = mpqReadLE32(pHeader + 4);
            const quint32 nArchiveSize = mpqReadLE32(pHeader + 8);
            const quint16 nFormatVersion = mpqReadLE16(pHeader + 12);
            const quint16 nSectorShift = mpqReadLE16(pHeader + 14);
            const quint32 nHashTableOffset = mpqReadLE32(pHeader + 16);
            const quint32 nBlockTableOffset = mpqReadLE32(pHeader + 20);
            const quint32 nHashTableEntries = mpqReadLE32(pHeader + 24);
            const quint32 nBlockTableEntries = mpqReadLE32(pHeader + 28);
            const quint64 nHashTableSize =
                static_cast<quint64>(nHashTableEntries) * 16U;
            const quint64 nBlockTableSize =
                static_cast<quint64>(nBlockTableEntries) * 16U;

            if ((mpqReadLE32(pHeader) == 0x1A51504DU) &&
                (nHeaderSize >= 32U) && (nHeaderSize <= nFileSize) &&
                (nArchiveSize >= nHeaderSize) &&
                (nArchiveSize <= nFileSize) &&
                (nFormatVersion <= 1U) && (nSectorShift <= 15U) &&
                (nHashTableEntries > 0U) &&
                (nHashTableEntries <= MPQ_MAX_TABLE_ENTRIES) &&
                (nBlockTableEntries > 0U) &&
                (nBlockTableEntries <= MPQ_MAX_TABLE_ENTRIES) &&
                mpqRangeWithin(nArchiveSize, nHashTableOffset,
                               nHashTableSize) &&
                mpqRangeWithin(nArchiveSize, nBlockTableOffset,
                               nBlockTableSize)) {
                *pFileKey = nNestedMpqKey;
                return true;
            }
        }
    }

    struct KNOWN_PAIR {
        quint32 nFirst;
        quint32 nSecond;
    };
    QList<KNOWN_PAIR> listPairs;
    if (nFileSize >= 8) {
        listPairs.append({0x46464952U, nFileSize - 8U});  // RIFF
        const quint32 nBmpFirst =
            0x00004D42U | ((nFileSize & 0xFFFFU) << 16);
        listPairs.append({nBmpFirst, nFileSize >> 16});  // BMP
        quint32 nFormSize = nFileSize - 8U;
        nFormSize = qToBigEndian<quint32>(nFormSize);
        listPairs.append({0x4D524F46U, nFormSize});      // IFF FORM
    }
    listPairs.append({0x474E5089U, 0x0A1A0A0DU});      // PNG
    listPairs.append({0x00905A4DU, 0x00000003U});      // common DOS EXE
    listPairs.append({0x6D783F3CU, 0x6576206CU});      // <?xml ve
    listPairs.append({0x0801050AU, 0x00000000U});      // common 8-bit PCX
    listPairs.append({0x6468544DU, 0x06000000U});      // MIDI MThd

    for (const KNOWN_PAIR &pair : listPairs) {
        if (mpqDetectFileKeyByKnownContent(
                baEncrypted, pair.nFirst, pair.nSecond, pFileKey)) {
            return true;
        }
    }
    return false;
}

// The following small PKWARE DCL decoder is an adapted, bounds-oriented C++
// implementation of Mark Adler's blast 1.3 algorithm.  The original zlib
// license notice is preserved here:
//
// Copyright (C) 2003, 2012, 2013 Mark Adler.
// This software is provided 'as-is', without any express or implied warranty.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, provided that the origin is not misrepresented, altered versions are
// marked as such, and this notice is not removed or altered.
const int MPQ_BLAST_MAX_BITS = 13;

struct MpqBlastHuffman {
    std::array<short, MPQ_BLAST_MAX_BITS + 1> count;
    std::array<short, 256> symbol;
};

bool mpqBlastConstruct(MpqBlastHuffman *pTable,
                       const uchar *pRepeat, int nRepeatCount)
{
    if (!pTable || !pRepeat || (nRepeatCount <= 0)) return false;
    std::array<short, 256> lengths = {};
    int nSymbols = 0;
    for (int i = 0; i < nRepeatCount; ++i) {
        const int nDescriptor = pRepeat[i];
        const int nCount = (nDescriptor >> 4) + 1;
        const int nLength = nDescriptor & 15;
        if ((nLength > MPQ_BLAST_MAX_BITS) ||
            (nSymbols > static_cast<int>(lengths.size()) - nCount)) {
            return false;
        }
        for (int j = 0; j < nCount; ++j)
            lengths[nSymbols++] = static_cast<short>(nLength);
    }

    pTable->count.fill(0);
    pTable->symbol.fill(0);
    for (int i = 0; i < nSymbols; ++i)
        pTable->count[lengths[i]]++;
    if (pTable->count[0] == nSymbols) return false;

    int nLeft = 1;
    for (int nLength = 1; nLength <= MPQ_BLAST_MAX_BITS; ++nLength) {
        nLeft = (nLeft << 1) - pTable->count[nLength];
        if (nLeft < 0) return false;
    }

    std::array<short, MPQ_BLAST_MAX_BITS + 1> offsets = {};
    offsets[1] = 0;
    for (int nLength = 1; nLength < MPQ_BLAST_MAX_BITS; ++nLength)
        offsets[nLength + 1] =
            offsets[nLength] + pTable->count[nLength];
    for (int i = 0; i < nSymbols; ++i) {
        const int nLength = lengths[i];
        if (nLength != 0)
            pTable->symbol[offsets[nLength]++] = static_cast<short>(i);
    }
    return true;
}

struct MpqBlastTables {
    MpqBlastHuffman literal;
    MpqBlastHuffman length;
    MpqBlastHuffman distance;
    bool bValid;

    MpqBlastTables() : bValid(false)
    {
        static const uchar literalLengths[] = {
            11, 124, 8, 7, 28, 7, 188, 13, 76, 4, 10, 8, 12, 10,
            12, 10, 8, 23, 8, 9, 7, 6, 7, 8, 7, 6, 55, 8, 23, 24,
            12, 11, 7, 9, 11, 12, 6, 7, 22, 5, 7, 24, 6, 11, 9,
            6, 7, 22, 7, 11, 38, 7, 9, 8, 25, 11, 8, 11, 9, 12,
            8, 12, 5, 38, 5, 38, 5, 11, 7, 5, 6, 21, 6, 10, 53,
            8, 7, 24, 10, 27, 44, 253, 253, 253, 252, 252, 252,
            13, 12, 45, 12, 45, 12, 61, 12, 45, 44, 173};
        static const uchar lengthLengths[] = {2, 35, 36, 53, 38, 23};
        static const uchar distanceLengths[] =
            {2, 20, 53, 230, 247, 151, 248};
        bValid = mpqBlastConstruct(
                     &literal, literalLengths,
                     static_cast<int>(sizeof(literalLengths))) &&
                 mpqBlastConstruct(
                     &length, lengthLengths,
                     static_cast<int>(sizeof(lengthLengths))) &&
                 mpqBlastConstruct(
                     &distance, distanceLengths,
                     static_cast<int>(sizeof(distanceLengths)));
    }
};

const MpqBlastTables &mpqBlastTables()
{
    static const MpqBlastTables tables;
    return tables;
}

class MpqBlastBits {
public:
    explicit MpqBlastBits(const QByteArray &baData)
        : m_pData(reinterpret_cast<const uchar *>(baData.constData())),
          m_nSize(baData.size()), m_nPosition(0), m_nBuffer(0),
          m_nCount(0)
    {
    }

    bool read(int nCount, int *pValue)
    {
        if (!pValue || (nCount < 0) || (nCount > 16)) return false;
        while (m_nCount < nCount) {
            if (m_nPosition >= m_nSize) return false;
            m_nBuffer |= static_cast<quint32>(m_pData[m_nPosition++])
                         << m_nCount;
            m_nCount += 8;
        }
        const quint32 nMask = nCount ? ((1U << nCount) - 1U) : 0U;
        *pValue = static_cast<int>(m_nBuffer & nMask);
        m_nBuffer >>= nCount;
        m_nCount -= nCount;
        return true;
    }

private:
    const uchar *m_pData;
    int m_nSize;
    int m_nPosition;
    quint32 m_nBuffer;
    int m_nCount;
};

bool mpqBlastDecodeSymbol(MpqBlastBits *pBits,
                          const MpqBlastHuffman &table, int *pSymbol)
{
    if (!pBits || !pSymbol) return false;
    int nCode = 0;
    int nFirst = 0;
    int nIndex = 0;
    for (int nLength = 1; nLength <= MPQ_BLAST_MAX_BITS; ++nLength) {
        int nBit = 0;
        if (!pBits->read(1, &nBit)) return false;
        nCode |= nBit ^ 1;
        const int nCount = table.count[nLength];
        if (nCode < nFirst + nCount) {
            const int nSymbolIndex = nIndex + (nCode - nFirst);
            if ((nSymbolIndex < 0) ||
                (nSymbolIndex >= static_cast<int>(table.symbol.size()))) {
                return false;
            }
            *pSymbol = table.symbol[nSymbolIndex];
            return true;
        }
        nIndex += nCount;
        nFirst = (nFirst + nCount) << 1;
        nCode <<= 1;
    }
    return false;
}

bool mpqBlast(const QByteArray &baInput, qint32 nExpectedSize,
              QByteArray *pOutput)
{
    if (pOutput) pOutput->clear();
    if (!pOutput || (nExpectedSize < 0) || (baInput.size() < 3))
        return false;
    const MpqBlastTables &tables = mpqBlastTables();
    if (!tables.bValid) return false;

    MpqBlastBits bits(baInput);
    int nLiteralMode = 0;
    int nDictionaryBits = 0;
    if (!bits.read(8, &nLiteralMode) ||
        !bits.read(8, &nDictionaryBits) ||
        (nLiteralMode < 0) || (nLiteralMode > 1) ||
        (nDictionaryBits < 4) || (nDictionaryBits > 6)) {
        return false;
    }

    static const int baseLength[16] =
        {3, 2, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 40, 72, 136, 264};
    static const int extraLength[16] =
        {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8};
    pOutput->reserve(nExpectedSize);

    for (;;) {
        int nIsMatch = 0;
        if (!bits.read(1, &nIsMatch)) return false;
        if (!nIsMatch) {
            int nLiteral = 0;
            if (nLiteralMode) {
                if (!mpqBlastDecodeSymbol(
                        &bits, tables.literal, &nLiteral)) return false;
            } else if (!bits.read(8, &nLiteral)) {
                return false;
            }
            if ((nLiteral < 0) || (nLiteral > 0xFF) ||
                (pOutput->size() >= nExpectedSize)) return false;
            pOutput->append(static_cast<char>(nLiteral));
            continue;
        }

        int nLengthSymbol = 0;
        if (!mpqBlastDecodeSymbol(
                &bits, tables.length, &nLengthSymbol) ||
            (nLengthSymbol < 0) || (nLengthSymbol >= 16)) {
            return false;
        }
        int nExtra = 0;
        if (!bits.read(extraLength[nLengthSymbol], &nExtra)) return false;
        int nLength = baseLength[nLengthSymbol] + nExtra;
        if (nLength == 519) break;

        int nDistanceSymbol = 0;
        if (!mpqBlastDecodeSymbol(
                &bits, tables.distance, &nDistanceSymbol) ||
            (nDistanceSymbol < 0) || (nDistanceSymbol >= 64)) {
            return false;
        }
        const int nLowBits = (nLength == 2) ? 2 : nDictionaryBits;
        int nDistanceLow = 0;
        if (!bits.read(nLowBits, &nDistanceLow)) return false;
        const int nDistance =
            (nDistanceSymbol << nLowBits) + nDistanceLow + 1;
        if ((nDistance <= 0) || (nDistance > pOutput->size()) ||
            (nLength > nExpectedSize - pOutput->size())) {
            return false;
        }
        while (nLength-- > 0) {
            pOutput->append(pOutput->at(pOutput->size() - nDistance));
        }
    }
    return pOutput->size() == nExpectedSize;
}

bool mpqDecompressWithXArchive(
    XBinary::HANDLE_METHOD method, const QByteArray &baInput,
    qint32 nExpectedSize,
    const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties,
    XBinary::PDSTRUCT *pPdStruct, QByteArray *pOutput)
{
    if (pOutput) pOutput->clear();
    if (!pOutput || (nExpectedSize < 0)) return false;
    QByteArray baSource = baInput;
    QBuffer sourceBuffer(&baSource);
    QBuffer outputBuffer(pOutput);
    if (!sourceBuffer.open(QIODevice::ReadOnly) ||
        !outputBuffer.open(QIODevice::ReadWrite)) return false;

    XBinary::FPART part = {};
    part.filePart = XBinary::FILEPART_STREAM;
    part.nFileOffset = 0;
    part.nFileSize = baSource.size();
    part.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, method);
    part.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE,
                              baSource.size());
    part.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE,
                              nExpectedSize);
    QMap<XBinary::UNPACK_PROP, QVariant> mapLimits = mapProperties;
    mapLimits.insert(XBinary::UNPACK_PROP_MAX_OUTPUT_SIZE,
                     static_cast<qint64>(nExpectedSize));
    XDecompress decompressor;
    const bool bResult = decompressor.decompressFPART(
        part, &sourceBuffer, &outputBuffer, mapLimits, pPdStruct);
    return bResult && (pOutput->size() == nExpectedSize);
}

bool mpqDecodeCompressedSector(
    const QByteArray &baInput, qint32 nExpectedSize, quint32 nFlags,
    const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties,
    XBinary::PDSTRUCT *pPdStruct, QByteArray *pOutput)
{
    if (pOutput) pOutput->clear();
    if (!pOutput || (nExpectedSize < 0) || baInput.isEmpty()) return false;
    if (baInput.size() == nExpectedSize) {
        *pOutput = baInput;
        return true;
    }
    if (baInput.size() > nExpectedSize) return false;

    if ((nFlags & MPQ_FILE_IMPLODE) &&
        !(nFlags & MPQ_FILE_COMPRESS)) {
        return mpqBlast(baInput, nExpectedSize, pOutput);
    }
    if (!(nFlags & MPQ_FILE_COMPRESS) || (baInput.size() < 2))
        return false;

    const quint8 nMethod = static_cast<quint8>(baInput.at(0));
    const QByteArray baPayload = baInput.mid(1);
    if (nMethod == 0x08)
        return mpqBlast(baPayload, nExpectedSize, pOutput);
    if (nMethod == 0x02) {
        return mpqDecompressWithXArchive(
            XBinary::HANDLE_METHOD_ZLIB, baPayload, nExpectedSize,
            mapProperties, pPdStruct, pOutput);
    }
    if (nMethod == 0x10) {
        return mpqDecompressWithXArchive(
            XBinary::HANDLE_METHOD_BZIP2, baPayload, nExpectedSize,
            mapProperties, pPdStruct, pOutput);
    }
    // Combined Blizzard compression masks require ordered filter chains
    // (Huffman/ADPCM/sparse) that XArchive does not currently expose.
    return false;
}

bool mpqIsSafeName(const QByteArray &baRawName, QString *pName)
{
    if (pName) pName->clear();
    if (!pName || baRawName.isEmpty() || (baRawName.size() > 4096) ||
        baRawName.contains('\0')) return false;
    QString sName = QString::fromLatin1(baRawName);
    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (sName.isEmpty() || sName.startsWith(QLatin1Char('/')) ||
        sName.contains(QLatin1Char(':'))) return false;
    const QStringList listParts =
        sName.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) ||
            (sPart == QLatin1String(".."))) return false;
    }
    sName = sName.normalized(QString::NormalizationForm_C);
    if (XBinary::fixFileName(sName) != sName) return false;
    *pName = sName;
    return true;
}

QString mpqUniqueName(const QString &sName, QSet<QString> *pUsedNames)
{
    if (!pUsedNames || sName.isEmpty()) return QString();
    QString sResult = sName;
    QString sKey = sResult.toCaseFolded();
    for (qint32 nSuffix = 2; pUsedNames->contains(sKey); ++nSuffix) {
        if (nSuffix > 1000000) return QString();
        const int nSlash = sName.lastIndexOf(QLatin1Char('/'));
        const int nDot = sName.lastIndexOf(QLatin1Char('.'));
        const bool bHasExtension = nDot > nSlash + 1;
        const QString sTail = QStringLiteral("_%1").arg(nSuffix);
        sResult = bHasExtension
            ? sName.left(nDot) + sTail + sName.mid(nDot)
            : sName + sTail;
        sKey = sResult.toCaseFolded();
    }
    pUsedNames->insert(sKey);
    return sResult;
}
}  // namespace

XMPQ::XMPQ(QIODevice *pDevice) : XArchive(pDevice)
{
    setFileType(FT_MPQ);
}

bool XMPQ::isValid(PDSTRUCT *pPdStruct)
{
    return scanArchive(nullptr, nullptr, pPdStruct);
}

bool XMPQ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMPQ archive(pDevice);
    return archive.isValid(pPdStruct);
}

bool XMPQ::scanArchive(MPQ_HEADER *pHeader, QList<MPQ_ENTRY> *pEntries,
                       PDSTRUCT *pPdStruct)
{
    if (pHeader) *pHeader = MPQ_HEADER();
    if (pEntries) pEntries->clear();
    QPointer<XMPQ> guardedThis(this);
    MpqDevicePositionGuard positionGuard(getDevice());
    if (!positionGuard.isValid() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const qint64 nDeviceSize = getSize();
    if (!guardedThis || (nDeviceSize < 32)) return false;
    const qint64 nSearchSize = qMin<qint64>(
        nDeviceSize, MPQ_HEADER_SEARCH_LIMIT + 4);
    const QByteArray baSearch = read_array_process(
        0, nSearchSize, pPdStruct);
    if (!guardedThis || (baSearch.size() != nSearchSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QList<qint64> listCandidates;
    auto addCandidate = [&listCandidates, nDeviceSize](qint64 nOffset) {
        if ((nOffset >= 0) && (nOffset <= nDeviceSize - 32) &&
            !listCandidates.contains(nOffset)) {
            listCandidates.append(nOffset);
        }
    };
    if ((baSearch.size() >= 4) &&
        (memcmp(baSearch.constData(), "MPQ\x1A", 4) == 0)) {
        addCandidate(0);
    }
    if ((baSearch.size() >= 16) &&
        (memcmp(baSearch.constData(), "MPQ\x1B", 4) == 0)) {
        const uchar *pUserData = reinterpret_cast<const uchar *>(
            baSearch.constData());
        addCandidate(mpqReadLE32(pUserData + 8));
    }
    for (qint64 nOffset = 0; nOffset <= nSearchSize - 4;
         nOffset += 0x200) {
        if (memcmp(baSearch.constData() + nOffset, "MPQ\x1A", 4) == 0)
            addCandidate(nOffset);
    }

    for (qint64 nCandidate : listCandidates) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nAvailable = nDeviceSize - nCandidate;
        const qint64 nProbeSize = qMin<qint64>(nAvailable, 44);
        const QByteArray baHeader = read_array_process(
            nCandidate, nProbeSize, pPdStruct);
        if (!guardedThis || (baHeader.size() != nProbeSize)) return false;
        if ((baHeader.size() < 32) ||
            (memcmp(baHeader.constData(), "MPQ\x1A", 4) != 0)) {
            continue;
        }

        const uchar *pRawHeader = reinterpret_cast<const uchar *>(
            baHeader.constData());
        MPQ_HEADER header = {};
        header.nHeaderOffset = nCandidate;
        header.nHeaderSize = mpqReadLE32(pRawHeader + 4);
        header.nArchiveSize = mpqReadLE32(pRawHeader + 8);
        header.nFormatVersion = mpqReadLE16(pRawHeader + 12);
        const quint16 nSectorShift = mpqReadLE16(pRawHeader + 14);
        header.nHashTableOffset = mpqReadLE32(pRawHeader + 16);
        header.nBlockTableOffset = mpqReadLE32(pRawHeader + 20);
        header.nHashTableEntries = mpqReadLE32(pRawHeader + 24);
        header.nBlockTableEntries = mpqReadLE32(pRawHeader + 28);

        if ((header.nFormatVersion > 1) ||
            (header.nHeaderSize < 32) ||
            (header.nHeaderSize > header.nArchiveSize) ||
            (header.nArchiveSize > static_cast<quint64>(nAvailable)) ||
            (nSectorShift > 15) ||
            (header.nHashTableEntries == 0) ||
            (header.nHashTableEntries > MPQ_MAX_TABLE_ENTRIES) ||
            ((header.nHashTableEntries &
              (header.nHashTableEntries - 1U)) != 0) ||
            (header.nBlockTableEntries > MPQ_MAX_TABLE_ENTRIES)) {
            continue;
        }
        header.nSectorSize = 512U << nSectorShift;

        if (header.nFormatVersion == 1) {
            if ((header.nHeaderSize < 44) || (baHeader.size() < 44))
                continue;
            header.nHiBlockTableOffset = mpqReadLE64(pRawHeader + 32);
            header.nHashTableOffset |=
                static_cast<quint64>(mpqReadLE16(pRawHeader + 40)) << 32;
            header.nBlockTableOffset |=
                static_cast<quint64>(mpqReadLE16(pRawHeader + 42)) << 32;
        }

        const quint64 nHashBytes =
            static_cast<quint64>(header.nHashTableEntries) * 16U;
        const quint64 nBlockBytes =
            static_cast<quint64>(header.nBlockTableEntries) * 16U;
        const quint64 nHiBlockBytes =
            static_cast<quint64>(header.nBlockTableEntries) * 2U;
        if (!mpqRangeWithin(header.nArchiveSize,
                            header.nHashTableOffset, nHashBytes) ||
            !mpqRangeWithin(header.nArchiveSize,
                            header.nBlockTableOffset, nBlockBytes) ||
            (header.nHiBlockTableOffset &&
             !mpqRangeWithin(header.nArchiveSize,
                              header.nHiBlockTableOffset,
                              nHiBlockBytes))) {
            continue;
        }

        qint64 nHashAbsolute = 0;
        qint64 nBlockAbsolute = 0;
        if (!mpqToQint64(static_cast<quint64>(nCandidate) +
                             header.nHashTableOffset,
                         &nHashAbsolute) ||
            !mpqToQint64(static_cast<quint64>(nCandidate) +
                             header.nBlockTableOffset,
                         &nBlockAbsolute)) {
            continue;
        }
        QByteArray baHashTable = read_array_process(
            nHashAbsolute, static_cast<qint64>(nHashBytes), pPdStruct);
        QByteArray baBlockTable = read_array_process(
            nBlockAbsolute, static_cast<qint64>(nBlockBytes), pPdStruct);
        if (!guardedThis ||
            (baHashTable.size() != static_cast<qint64>(nHashBytes)) ||
            (baBlockTable.size() != static_cast<qint64>(nBlockBytes)) ||
            !mpqDecryptBlock(&baHashTable, MPQ_HASH_TABLE_KEY) ||
            !mpqDecryptBlock(&baBlockTable, MPQ_BLOCK_TABLE_KEY)) {
            if (!guardedThis) return false;
            continue;
        }

        QByteArray baHiBlockTable;
        if (header.nHiBlockTableOffset) {
            qint64 nHiBlockAbsolute = 0;
            if (!mpqToQint64(static_cast<quint64>(nCandidate) +
                                 header.nHiBlockTableOffset,
                             &nHiBlockAbsolute)) {
                continue;
            }
            baHiBlockTable = read_array_process(
                nHiBlockAbsolute, static_cast<qint64>(nHiBlockBytes),
                pPdStruct);
            if (!guardedThis ||
                (baHiBlockTable.size() !=
                 static_cast<qint64>(nHiBlockBytes))) {
                if (!guardedThis) return false;
                continue;
            }
        }

        QVector<MPQ_HASH_ENTRY> vectorHashes;
        vectorHashes.resize(static_cast<int>(header.nHashTableEntries));
        for (quint32 i = 0; i < header.nHashTableEntries; ++i) {
            const uchar *pRaw = reinterpret_cast<const uchar *>(
                baHashTable.constData() + static_cast<qint64>(i) * 16);
            MPQ_HASH_ENTRY hashEntry = {};
            hashEntry.nHashA = mpqReadLE32(pRaw);
            hashEntry.nHashB = mpqReadLE32(pRaw + 4);
            hashEntry.nLocale = mpqReadLE16(pRaw + 8);
            hashEntry.nPlatform = pRaw[10];
            hashEntry.nReserved = pRaw[11];
            hashEntry.nBlockIndex = mpqReadLE32(pRaw + 12);
            vectorHashes[static_cast<int>(i)] = hashEntry;
        }

        QVector<MPQ_BLOCK_ENTRY> vectorBlocks;
        vectorBlocks.resize(static_cast<int>(header.nBlockTableEntries));
        for (quint32 i = 0; i < header.nBlockTableEntries; ++i) {
            const uchar *pRaw = reinterpret_cast<const uchar *>(
                baBlockTable.constData() + static_cast<qint64>(i) * 16);
            MPQ_BLOCK_ENTRY blockEntry = {};
            blockEntry.nFileOffset = mpqReadLE32(pRaw);
            if (!baHiBlockTable.isEmpty()) {
                const uchar *pHigh = reinterpret_cast<const uchar *>(
                    baHiBlockTable.constData() +
                    static_cast<qint64>(i) * 2);
                blockEntry.nFileOffset |=
                    static_cast<quint64>(mpqReadLE16(pHigh)) << 32;
            }
            blockEntry.nCompressedSize = mpqReadLE32(pRaw + 4);
            blockEntry.nUncompressedSize = mpqReadLE32(pRaw + 8);
            blockEntry.nFlags = mpqReadLE32(pRaw + 12);
            vectorBlocks[static_cast<int>(i)] = blockEntry;
        }

        QList<MPQ_ENTRY> listEntries;
        bool bTablesValid = true;
        for (quint32 i = 0; i < header.nHashTableEntries; ++i) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            const MPQ_HASH_ENTRY &hashEntry =
                vectorHashes.at(static_cast<int>(i));
            if ((hashEntry.nBlockIndex == MPQ_HASH_ENTRY_FREE) ||
                (hashEntry.nBlockIndex == MPQ_HASH_ENTRY_DELETED)) {
                continue;
            }
            if (hashEntry.nBlockIndex >= header.nBlockTableEntries) {
                bTablesValid = false;
                break;
            }
            const MPQ_BLOCK_ENTRY &blockEntry = vectorBlocks.at(
                static_cast<int>(hashEntry.nBlockIndex));
            if (!(blockEntry.nFlags & MPQ_FILE_EXISTS) ||
                (blockEntry.nFlags & MPQ_FILE_DELETE_MARKER) ||
                !mpqRangeWithin(header.nArchiveSize,
                                blockEntry.nFileOffset,
                                blockEntry.nCompressedSize) ||
                (blockEntry.nUncompressedSize &&
                 !blockEntry.nCompressedSize) ||
                (!(blockEntry.nFlags & MPQ_FILE_COMPRESS_MASK) &&
                 (blockEntry.nCompressedSize <
                  blockEntry.nUncompressedSize))) {
                bTablesValid = false;
                break;
            }

            if (pEntries) {
                MPQ_ENTRY entry = {};
                entry.block = blockEntry;
                entry.nHashEntryOffset = nHashAbsolute +
                    static_cast<qint64>(i) * 16;
                entry.nHashIndex = i;
                entry.nBlockIndex = hashEntry.nBlockIndex;
                entry.nLocale = hashEntry.nLocale;
                entry.nPlatform = hashEntry.nPlatform;
                entry.bFileNameKnown = false;
                listEntries.append(entry);
            }
        }
        if (!bTablesValid) continue;

        if (pEntries) {
            QHash<quint32, QString> mapNames;
            auto registerName = [&vectorHashes, &mapNames](
                                    const QByteArray &baName) {
                QString sSafeName;
                if (!mpqIsSafeName(baName, &sSafeName)) return;
                const quint32 nHashA = mpqHashString(baName, 1);
                const quint32 nHashB = mpqHashString(baName, 2);
                for (int i = 0; i < vectorHashes.size(); ++i) {
                    const MPQ_HASH_ENTRY &hashEntry = vectorHashes.at(i);
                    if ((hashEntry.nBlockIndex != MPQ_HASH_ENTRY_FREE) &&
                        (hashEntry.nBlockIndex != MPQ_HASH_ENTRY_DELETED) &&
                        (hashEntry.nHashA == nHashA) &&
                        (hashEntry.nHashB == nHashB) &&
                        !mapNames.contains(static_cast<quint32>(i))) {
                        mapNames.insert(static_cast<quint32>(i), sSafeName);
                    }
                }
            };
            registerName(QByteArrayLiteral("(listfile)"));
            registerName(QByteArrayLiteral("(attributes)"));
            registerName(QByteArrayLiteral("(signature)"));
            registerName(QByteArrayLiteral("(patch_metadata)"));

            MPQ_ENTRY listFileEntry = {};
            bool bListFilePresent = false;
            for (const MPQ_ENTRY &entry : listEntries) {
                if (mapNames.value(entry.nHashIndex) ==
                    QLatin1String("(listfile)")) {
                    listFileEntry = entry;
                    listFileEntry.sFileName = QStringLiteral("(listfile)");
                    listFileEntry.bFileNameKnown = true;
                    bListFilePresent = true;
                    break;
                }
            }
            if (bListFilePresent &&
                (listFileEntry.block.nUncompressedSize <=
                 static_cast<quint64>(MPQ_MAX_LISTFILE_SIZE))) {
                QByteArray baListFile;
                baListFile.reserve(static_cast<int>(
                    listFileEntry.block.nUncompressedSize));
                QBuffer listFileBuffer(&baListFile);
                if (listFileBuffer.open(QIODevice::ReadWrite)) {
                    QMap<UNPACK_PROP, QVariant> mapListLimits;
                    mapListLimits.insert(
                        UNPACK_PROP_MAX_OUTPUT_SIZE,
                        static_cast<qint64>(MPQ_MAX_LISTFILE_SIZE));
                    const bool bDecoded = decodeEntry(
                        header, listFileEntry, &listFileBuffer,
                        mapListLimits, pPdStruct);
                    if (!guardedThis) return false;
                    if (bDecoded) {
                        baListFile.replace('\0', '\n');
                        const QList<QByteArray> listLines =
                            baListFile.split('\n');
                        for (QByteArray baLine : listLines) {
                            if (baLine.endsWith('\r')) baLine.chop(1);
                            baLine = baLine.trimmed();
                            if (!baLine.isEmpty()) registerName(baLine);
                        }
                    }
                }
            }

            QSet<QString> stUsedNames;
            for (MPQ_ENTRY &entry : listEntries) {
                const QString sResolved = mapNames.value(entry.nHashIndex);
                entry.bFileNameKnown = !sResolved.isEmpty();
                QString sName = sResolved;
                if (sName.isEmpty()) {
                    sName = QStringLiteral("unknown/File%1_%2.bin")
                        .arg(entry.nBlockIndex, 8, 16, QLatin1Char('0'))
                        .arg(entry.nHashIndex, 8, 16, QLatin1Char('0'));
                }
                entry.sFileName = mpqUniqueName(sName, &stUsedNames);
                if (entry.sFileName.isEmpty()) {
                    bTablesValid = false;
                    break;
                }
            }
            if (!bTablesValid) continue;
            std::sort(listEntries.begin(), listEntries.end(),
                      [](const MPQ_ENTRY &a, const MPQ_ENTRY &b) {
                          if (a.nBlockIndex != b.nBlockIndex)
                              return a.nBlockIndex < b.nBlockIndex;
                          if (a.nLocale != b.nLocale)
                              return a.nLocale < b.nLocale;
                          return a.nHashIndex < b.nHashIndex;
                      });
        }

        const bool bRestored = positionGuard.restore();
        if (!guardedThis || !bRestored ||
            !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (pHeader) *pHeader = header;
        if (pEntries) *pEntries = listEntries;
        return true;
    }

    return guardedThis && positionGuard.restore() &&
           XBinary::isPdStructNotCanceled(pPdStruct) && false;
}

bool XMPQ::decodeEntry(
    const MPQ_HEADER &header, const MPQ_ENTRY &entry,
    QIODevice *pOutputDevice,
    const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    QPointer<XMPQ> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pOutputDevice);
    if (!guardedThis || !guardedOutput ||
        !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !XBinary::isUnpackOutputSizeAllowed(
            mapProperties, entry.block.nUncompressedSize) ||
        !(entry.block.nFlags & MPQ_FILE_EXISTS) ||
        (entry.block.nFlags & (MPQ_FILE_DELETE_MARKER |
                               MPQ_FILE_PATCH_FILE)) ||
        !mpqRangeWithin(header.nArchiveSize,
                        entry.block.nFileOffset,
                        entry.block.nCompressedSize)) {
        return false;
    }

    const quint32 nFileSize = entry.block.nUncompressedSize;
    const quint32 nCompressedSize = entry.block.nCompressedSize;
    const quint32 nFlags = entry.block.nFlags;
    const bool bEncrypted = (nFlags & MPQ_FILE_ENCRYPTED) != 0;
    const bool bCompressed = (nFlags & MPQ_FILE_COMPRESS_MASK) != 0;
    const bool bSingleUnit = (nFlags & MPQ_FILE_SINGLE_UNIT) != 0;
    qint64 nOutputOffset = 0;

    auto fail = [pPdStruct](const QString &sMessage) {
        XBinary::setPdStructErrorString(pPdStruct, sMessage);
        return false;
    };
    auto readBlockData = [this, &guardedThis, &header, &entry,
                          pPdStruct](quint64 nRelativeOffset,
                                     quint64 nSize,
                                     QByteArray *pData) {
        if (pData) pData->clear();
        if (!pData || !guardedThis ||
            !mpqRangeWithin(entry.block.nCompressedSize,
                            nRelativeOffset, nSize) ||
            (nSize > static_cast<quint64>(
                 (std::numeric_limits<int>::max)()))) {
            return false;
        }
        const quint64 nAbsolute =
            static_cast<quint64>(header.nHeaderOffset) +
            entry.block.nFileOffset + nRelativeOffset;
        qint64 nAbsoluteSigned = 0;
        if (!mpqToQint64(nAbsolute, &nAbsoluteSigned)) return false;
        *pData = read_array_process(
            nAbsoluteSigned, static_cast<qint64>(nSize), pPdStruct);
        return guardedThis &&
               (pData->size() == static_cast<qint64>(nSize));
    };
    auto writeOutput = [this, &guardedThis, &guardedOutput,
                        &nOutputOffset, pPdStruct](
                           const QByteArray &baData) {
        if (!guardedThis || !guardedOutput ||
            !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (baData.isEmpty()) return true;
        const qint64 nWritten = safeWriteData(
            guardedOutput.data(), nOutputOffset,
            baData.constData(), baData.size(), pPdStruct);
        if (!guardedThis || !guardedOutput ||
            (nWritten != baData.size())) return false;
        nOutputOffset += nWritten;
        return true;
    };

    if (nFileSize == 0) return nCompressedSize == 0;

    quint32 nFileKey = 0;
    bool bFileKeyKnown = !bEncrypted;
    if (bEncrypted && entry.bFileNameKnown) {
        nFileKey = mpqFileKey(entry.sFileName.toLatin1(),
                              entry.block.nFileOffset, nFileSize, nFlags);
        bFileKeyKnown = true;
    }

    if (bSingleUnit) {
        if (nCompressedSize > static_cast<quint32>(
                (std::numeric_limits<int>::max)())) {
            return fail(tr("MPQ single-unit member is too large"));
        }
        QByteArray baRaw;
        if (!readBlockData(0, nCompressedSize, &baRaw))
            return fail(tr("Cannot read MPQ member data"));
        if (bEncrypted) {
            if (!bFileKeyKnown) {
                return fail(tr("MPQ encrypted single-unit member needs "
                               "its original filename"));
            }
            if (!mpqDecryptBlock(&baRaw, nFileKey))
                return fail(tr("Cannot decrypt MPQ member"));
        }

        QByteArray baDecoded;
        if (bCompressed && (nCompressedSize < nFileSize)) {
            if (nFileSize > static_cast<quint32>(
                    (std::numeric_limits<int>::max)()) ||
                !mpqDecodeCompressedSector(
                    baRaw, static_cast<qint32>(nFileSize), nFlags,
                    mapProperties, pPdStruct, &baDecoded)) {
                return fail(tr("Unsupported or damaged MPQ compression"));
            }
        } else {
            if (baRaw.size() < static_cast<qint64>(nFileSize))
                return fail(tr("Truncated MPQ member"));
            baDecoded = baRaw.left(static_cast<int>(nFileSize));
        }
        if (!writeOutput(baDecoded))
            return fail(tr("Cannot write unpacked MPQ member"));
        return guardedThis && guardedOutput &&
               (nOutputOffset == nFileSize);
    }

    const quint32 nSectorSize = header.nSectorSize;
    const quint64 nSectorCount64 =
        (static_cast<quint64>(nFileSize) + nSectorSize - 1U) /
        nSectorSize;
    if ((nSectorSize == 0) ||
        (nSectorCount64 > (std::numeric_limits<quint32>::max)())) {
        return fail(tr("Invalid MPQ sector geometry"));
    }
    const quint32 nSectorCount = static_cast<quint32>(nSectorCount64);

    if (bCompressed) {
        const quint64 nBaseTableEntries =
            static_cast<quint64>(nSectorCount) + 1U;
        const quint64 nInitialTableEntries = nBaseTableEntries +
            ((nFlags & MPQ_FILE_SECTOR_CRC) ? 1U : 0U);
        const quint64 nInitialTableSize64 = nInitialTableEntries * 4U;
        if ((nInitialTableSize64 > nCompressedSize) ||
            (nInitialTableSize64 > static_cast<quint64>(
                 (std::numeric_limits<int>::max)()))) {
            return fail(tr("Invalid MPQ sector table"));
        }
        quint32 nTableSize = static_cast<quint32>(nInitialTableSize64);
        QByteArray baSectorTable;
        if (!readBlockData(0, nTableSize, &baSectorTable))
            return fail(tr("Cannot read MPQ sector table"));

        if (bEncrypted && !bFileKeyKnown) {
            if (!mpqDetectFileKeyBySectorTable(
                    baSectorTable, nSectorSize, nTableSize, &nFileKey)) {
                return fail(tr("Cannot recover MPQ sector encryption key"));
            }
            bFileKeyKnown = true;
        }
        if (bEncrypted &&
            (!bFileKeyKnown ||
             !mpqDecryptBlock(&baSectorTable, nFileKey - 1U))) {
            return fail(tr("Cannot decrypt MPQ sector table"));
        }

        quint32 nFirstOffset = mpqReadLE32(
            reinterpret_cast<const uchar *>(baSectorTable.constData()));
        const quint32 nAlignedFirstOffset = nFirstOffset & 0xFFFFFFFCU;
        if ((nAlignedFirstOffset > nTableSize) &&
            (nAlignedFirstOffset <= nTableSize + 0x400U) &&
            (nAlignedFirstOffset <= nCompressedSize)) {
            nTableSize = nAlignedFirstOffset;
            if (!readBlockData(0, nTableSize, &baSectorTable))
                return fail(tr("Cannot read extended MPQ sector table"));
            if (bEncrypted &&
                !mpqDecryptBlock(&baSectorTable, nFileKey - 1U)) {
                return fail(tr("Cannot decrypt extended MPQ sector table"));
            }
            nFirstOffset = mpqReadLE32(
                reinterpret_cast<const uchar *>(
                    baSectorTable.constData()));
        }

        const quint64 nRequiredTableSize = nBaseTableEntries * 4U;
        if ((baSectorTable.size() <
             static_cast<qint64>(nRequiredTableSize)) ||
            (nFirstOffset < nRequiredTableSize) ||
            (nFirstOffset > nCompressedSize)) {
            return fail(tr("Damaged MPQ sector table"));
        }

        QVector<quint32> vectorOffsets;
        vectorOffsets.resize(static_cast<int>(nSectorCount + 1U));
        const uchar *pOffsets = reinterpret_cast<const uchar *>(
            baSectorTable.constData());
        for (quint32 i = 0; i <= nSectorCount; ++i)
            vectorOffsets[static_cast<int>(i)] = mpqReadLE32(pOffsets + i * 4U);
        for (quint32 i = 0; i < nSectorCount; ++i) {
            const quint32 nBegin = vectorOffsets.at(static_cast<int>(i));
            const quint32 nEnd = vectorOffsets.at(static_cast<int>(i + 1U));
            if ((nEnd < nBegin) || (nEnd > nCompressedSize) ||
                (nEnd - nBegin > nSectorSize)) {
                return fail(tr("Damaged MPQ sector offsets"));
            }
        }

        quint32 nRemaining = nFileSize;
        for (quint32 i = 0; i < nSectorCount; ++i) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            const quint32 nBegin = vectorOffsets.at(static_cast<int>(i));
            const quint32 nEnd = vectorOffsets.at(static_cast<int>(i + 1U));
            const quint32 nRawSize = nEnd - nBegin;
            const quint32 nExpected = qMin(nSectorSize, nRemaining);
            QByteArray baRaw;
            if (!readBlockData(nBegin, nRawSize, &baRaw))
                return fail(tr("Cannot read MPQ sector"));
            if (bEncrypted && !mpqDecryptBlock(&baRaw, nFileKey + i))
                return fail(tr("Cannot decrypt MPQ sector"));

            QByteArray baDecoded;
            if (nRawSize < nExpected) {
                if (!mpqDecodeCompressedSector(
                        baRaw, static_cast<qint32>(nExpected), nFlags,
                        mapProperties, pPdStruct, &baDecoded)) {
                    return fail(tr("Unsupported or damaged MPQ sector "
                                   "compression"));
                }
            } else if (nRawSize == nExpected) {
                baDecoded = baRaw;
            } else {
                return fail(tr("Invalid MPQ sector size"));
            }
            if (!writeOutput(baDecoded))
                return fail(tr("Cannot write unpacked MPQ sector"));
            nRemaining -= nExpected;
        }
        return guardedThis && guardedOutput && (nRemaining == 0) &&
               (nOutputOffset == nFileSize);
    }

    if (nCompressedSize < nFileSize)
        return fail(tr("Truncated stored MPQ member"));
    if (bEncrypted && !bFileKeyKnown) {
        const quint32 nProbeSize = qMin(nSectorSize, nFileSize);
        QByteArray baProbe;
        if (!readBlockData(0, nProbeSize, &baProbe) ||
            !mpqDetectFileKeyByMagic(baProbe, nFileSize, &nFileKey)) {
            return fail(tr("MPQ encrypted stored member needs its original "
                           "filename"));
        }
        bFileKeyKnown = true;
    }

    quint32 nRemaining = nFileSize;
    quint64 nRawOffset = 0;
    quint32 nSectorIndex = 0;
    while (nRemaining) {
        const quint32 nChunkSize = qMin(nSectorSize, nRemaining);
        QByteArray baChunk;
        if (!readBlockData(nRawOffset, nChunkSize, &baChunk))
            return fail(tr("Cannot read stored MPQ sector"));
        if (bEncrypted &&
            (!bFileKeyKnown ||
             !mpqDecryptBlock(&baChunk, nFileKey + nSectorIndex))) {
            return fail(tr("Cannot decrypt stored MPQ sector"));
        }
        if (!writeOutput(baChunk))
            return fail(tr("Cannot write stored MPQ sector"));
        nRawOffset += nChunkSize;
        nRemaining -= nChunkSize;
        nSectorIndex++;
    }
    return guardedThis && guardedOutput &&
           (nOutputOffset == nFileSize);
}

XBinary::FT XMPQ::getFileType()
{
    return FT_MPQ;
}

XBinary::MODE XMPQ::getMode()
{
    return MODE_DATA;
}

qint32 XMPQ::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XMPQ::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XMPQ::getArch()
{
    return QString();
}

QString XMPQ::getFileFormatExt()
{
    return QStringLiteral("mpq");
}

QString XMPQ::getFileFormatExtsString()
{
    return QStringLiteral("Blizzard MPQ (*.mpq)");
}

QString XMPQ::getMIMEString()
{
    return QStringLiteral("application/x-mpq");
}

qint64 XMPQ::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    MPQ_HEADER header = {};
    if (!scanArchive(&header, nullptr, pPdStruct)) return 0;
    qint64 nResult = 0;
    if (!mpqToQint64(static_cast<quint64>(header.nHeaderOffset) +
                         header.nArchiveSize,
                     &nResult)) {
        return 0;
    }
    return nResult;
}

XBinary::OSNAME XMPQ::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QString XMPQ::getVersion()
{
    MPQ_HEADER header = {};
    if (!scanArchive(&header, nullptr, nullptr)) return QString();
    return QString::number(static_cast<quint32>(
        header.nFormatVersion) + 1U);
}

QList<QString> XMPQ::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'MPQ'1A"));
    listResult.append(QStringLiteral("'MPQ'1B"));
    return listResult;
}

XBinary *XMPQ::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XMPQ(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XMPQ::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XMPQ::initUnpack(
    UNPACK_STATE *pState,
    const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    QPointer<XMPQ> guardedThis(this);
    if (m_bUnpackOperationInProgress) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    MPQ_UNPACK_CONTEXT *pOldContext =
        static_cast<MPQ_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !isPdStructNotCanceled(pPdStruct)) return false;

    if (!bindUnpackSource(pState, pPdStruct) || !guardedThis) return false;

    MPQ_HEADER header = {};
    QList<MPQ_ENTRY> listEntries;
    const bool bScanned = scanArchive(&header, &listEntries, pPdStruct);
    if (!guardedThis || !bScanned ||
        !isPdStructNotCanceled(pPdStruct)) {
        if (guardedThis) {
            releaseUnpackSource(pState);
            *pState = UNPACK_STATE();
        }
        return false;
    }

    MPQ_UNPACK_CONTEXT *pContext =
        new (std::nothrow) MPQ_UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    pContext->header = header;
    pContext->listEntries = listEntries;
    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = listEntries.count();
    pState->nTotalSize = getSize();
    if (!guardedThis) return false;
    pState->nCurrentOffset = listEntries.isEmpty()
        ? static_cast<qint64>(header.nHeaderOffset + header.nArchiveSize)
        : listEntries.constFirst().nHashEntryOffset;
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

XBinary::ARCHIVERECORD XMPQ::infoCurrent(
    UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XMPQ> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext)
        return ARCHIVERECORD();
    if (!isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        !isPdStructNotCanceled(pPdStruct)) return ARCHIVERECORD();

    MPQ_UNPACK_CONTEXT *pContext =
        static_cast<MPQ_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nTotalSize != getSize()) || !guardedThis ||
        (pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pContext->listEntries.count())) {
        return ARCHIVERECORD();
    }
    const MPQ_ENTRY entry =
        pContext->listEntries.at(pState->nCurrentIndex);
    qint64 nStreamOffset = 0;
    if (!mpqToQint64(
            static_cast<quint64>(pContext->header.nHeaderOffset) +
                entry.block.nFileOffset,
            &nStreamOffset)) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = nStreamOffset;
    result.nStreamSize = entry.block.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME,
                                entry.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                static_cast<qint64>(
                                    entry.block.nUncompressedSize));
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                static_cast<qint64>(
                                    entry.block.nCompressedSize));
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                HANDLE_METHOD_UNKNOWN);
    result.mapProperties.insert(FPART_PROP_HEADER_OFFSET,
                                entry.nHashEntryOffset);
    result.mapProperties.insert(FPART_PROP_HEADER_SIZE,
                                static_cast<qint64>(16));
    result.mapProperties.insert(FPART_PROP_FILEMODE,
                                static_cast<quint32>(0644));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_ENCRYPTED,
                                (entry.block.nFlags &
                                 MPQ_FILE_ENCRYPTED) != 0);
    result.mapProperties.insert(FPART_PROP_TYPE,
                                entry.block.nFlags);

    QStringList listInfo;
    if (entry.block.nFlags & MPQ_FILE_IMPLODE)
        listInfo.append(QStringLiteral("PKWARE DCL Implode"));
    if (entry.block.nFlags & MPQ_FILE_COMPRESS)
        listInfo.append(QStringLiteral("Blizzard multi-compression"));
    if (entry.block.nFlags & MPQ_FILE_ENCRYPTED)
        listInfo.append(QStringLiteral("encrypted"));
    if (!entry.bFileNameKnown) {
        listInfo.append(QStringLiteral(
            "synthetic name (classic MPQ stores hashes only)"));
        if ((entry.block.nFlags & MPQ_FILE_ENCRYPTED) &&
            !(entry.block.nFlags & MPQ_FILE_COMPRESS_MASK)) {
            listInfo.append(QStringLiteral(
                "original filename may be required for extraction"));
        }
    }
    listInfo.append(QStringLiteral("block %1, locale 0x%2")
                        .arg(entry.nBlockIndex)
                        .arg(entry.nLocale, 4, 16, QLatin1Char('0')));
    result.mapProperties.insert(FPART_PROP_INFO,
                                listInfo.join(QStringLiteral(", ")));

    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex))
        return ARCHIVERECORD();
    return result;
}

bool XMPQ::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                         PDSTRUCT *pPdStruct)
{
    QPointer<XMPQ> guardedThis(this);
    MpqDevicePositionGuard positionGuard(getDevice());
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!positionGuard.isValid() || !operationGuard.isAcquired() ||
        !pState || !pState->pContext || !guardedOutput ||
        !guardedSource || !isUnpackOutputSupported(guardedOutput.data()) ||
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) ||
        !isPdStructNotCanceled(pPdStruct) ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis) {
        return false;
    }

    MPQ_UNPACK_CONTEXT *pContext =
        static_cast<MPQ_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pContext->listEntries.count()) ||
        (pState->nTotalSize != getSize()) || !guardedThis) {
        return false;
    }
    const MPQ_ENTRY entry =
        pContext->listEntries.at(pState->nCurrentIndex);
    if (!XBinary::isUnpackOutputSizeAllowed(
            pState->mapUnpackProperties,
            entry.block.nUncompressedSize)) {
        XBinary::setPdStructErrorString(
            pPdStruct, tr("Unpacked MPQ member exceeds the configured limit"));
        return false;
    }

    std::unique_ptr<QIODevice> pStage(XBinary::createFileBuffer(
        entry.block.nUncompressedSize, pPdStruct));
    if (!pStage || !guardedThis || !guardedOutput || !guardedSource)
        return false;
    const bool bDecoded = decodeEntry(
        pContext->header, entry, pStage.get(),
        pState->mapUnpackProperties, pPdStruct);
    if (!guardedThis || !guardedOutput || !guardedSource || !bDecoded ||
        !isPdStructNotCanceled(pPdStruct) ||
        (pStage->size() != entry.block.nUncompressedSize) ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis) {
        return false;
    }
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(
                pState->nCurrentIndex, entry.sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(
                    pPdStruct,
                    tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(
                pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(pStage->size())) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(
                    pPdStruct,
                    tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(
                pState->spOutputBudget.data());
        }
    }
    const bool bPublished = publishUnpackOutput(
        pStage.get(), guardedOutput.data(), pState, pPdStruct);
    if (!guardedThis || !guardedOutput || !guardedSource || !bPublished ||
        !positionGuard.restore()) {
        return false;
    }
    pState->nCurrentOffset = entry.block.nUncompressedSize;
    return true;
}

bool XMPQ::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XMPQ> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        !isPdStructNotCanceled(pPdStruct)) return false;
    MPQ_UNPACK_CONTEXT *pContext =
        static_cast<MPQ_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    pState->nCurrentIndex++;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listEntries.at(
            pState->nCurrentIndex).nHashEntryOffset;
        return true;
    }
    pState->nCurrentOffset = pState->nTotalSize;
    return false;
}

bool XMPQ::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    Q_UNUSED(pPdStruct)
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    MPQ_UNPACK_CONTEXT *pContext =
        static_cast<MPQ_UNPACK_CONTEXT *>(pState->pContext);
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

QList<XBinary::FPART_PROP> XMPQ::getAvailableFPARTProperties()
{
    QList<FPART_PROP> listResult;
    listResult.append(FPART_PROP_ORIGINALNAME);
    listResult.append(FPART_PROP_UNCOMPRESSEDSIZE);
    listResult.append(FPART_PROP_COMPRESSEDSIZE);
    listResult.append(FPART_PROP_HANDLEMETHOD);
    listResult.append(FPART_PROP_ENCRYPTED);
    listResult.append(FPART_PROP_TYPE);
    listResult.append(FPART_PROP_INFO);
    listResult.append(FPART_PROP_FILEMODE);
    return listResult;
}

bool XMPQ::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XMPQ> guardedThis(this);
    bool bResult = true;
    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo =
            static_cast<XArchive::INTERNAL_INFO *>(
                guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(
            guardedThis->m_internalInfo) = *pInfo;
    }
    return guardedThis && bResult;
}

void *XMPQ::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XMPQ> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;
    return &guardedThis->m_internalInfo;
}

void XMPQ::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(
            static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
