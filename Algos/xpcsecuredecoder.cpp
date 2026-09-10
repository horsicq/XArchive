/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xpcsecuredecoder.h"

#include <QVector>
#include <QtEndian>

namespace {
const qint64 PCS_MAX_OUTPUT = 0x20000000;  // 512 MiB sanity cap
const qint64 PCS_HEADER_SIZE = 68;

// Standard DES tables.  Bit 1 is the most significant bit of the value being
// permuted, which is also the most significant bit of its first byte.
const int PCS_PC1[56] = {57, 49, 41, 33, 25, 17, 9,  1,  58, 50, 42, 34, 26, 18,
                         10, 2,  59, 51, 43, 35, 27, 19, 11, 3,  60, 52, 44, 36,
                         63, 55, 47, 39, 31, 23, 15, 7,  62, 54, 46, 38, 30, 22,
                         14, 6,  61, 53, 45, 37, 29, 21, 13, 5,  28, 20, 12, 4};

const int PCS_PC2[48] = {14, 17, 11, 24, 1,  5,  3,  28, 15, 6,  21, 10,
                         23, 19, 12, 4,  26, 8,  16, 7,  27, 20, 13, 2,
                         41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48,
                         44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32};

const int PCS_IP[64] = {58, 50, 42, 34, 26, 18, 10, 2,  60, 52, 44, 36, 28,
                        20, 12, 4,  62, 54, 46, 38, 30, 22, 14, 6,  64, 56,
                        48, 40, 32, 24, 16, 8,  57, 49, 41, 33, 25, 17, 9,
                        1,  59, 51, 43, 35, 27, 19, 11, 3,  61, 53, 45, 37,
                        29, 21, 13, 5,  63, 55, 47, 39, 31, 23, 15, 7};

const int PCS_FP[64] = {40, 8,  48, 16, 56, 24, 64, 32, 39, 7,  47, 15, 55,
                        23, 63, 31, 38, 6,  46, 14, 54, 22, 62, 30, 37, 5,
                        45, 13, 53, 21, 61, 29, 36, 4,  44, 12, 52, 20, 60,
                        28, 35, 3,  43, 11, 51, 19, 59, 27, 34, 2,  42, 10,
                        50, 18, 58, 26, 33, 1,  41, 9,  49, 17, 57, 25};

const int PCS_E[48] = {32, 1,  2,  3,  4,  5,  4,  5,  6,  7,  8,  9,
                       8,  9,  10, 11, 12, 13, 12, 13, 14, 15, 16, 17,
                       16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25,
                       24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32, 1};

const int PCS_P[32] = {16, 7,  20, 21, 29, 12, 28, 17, 1,  15, 23,
                       26, 5,  18, 31, 10, 2,  8,  24, 14, 32, 27,
                       3,  9,  19, 13, 30, 6,  22, 11, 4,  25};

const int PCS_SHIFTS[16] = {1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};

const quint8 PCS_SBOX[8][64] = {
    {14, 4,  13, 1, 2,  15, 11, 8,  3,  10, 6,  12, 5,  9,  0, 7,
     0,  15, 7,  4, 14, 2,  13, 1,  10, 6,  12, 11, 9,  5,  3, 8,
     4,  1,  14, 8, 13, 6,  2,  11, 15, 12, 9,  7,  3,  10, 5, 0,
     15, 12, 8,  2, 4,  9,  1,  7,  5,  11, 3,  14, 10, 0,  6, 13},
    {15, 1,  8,  14, 6,  11, 3,  4,  9,  7, 2,  13, 12, 0, 5,  10,
     3,  13, 4,  7,  15, 2,  8,  14, 12, 0, 1,  10, 6,  9, 11, 5,
     0,  14, 7,  11, 10, 4,  13, 1,  5,  8, 12, 6,  9,  3, 2,  15,
     13, 8,  10, 1,  3,  15, 4,  2,  11, 6, 7,  12, 0,  5, 14, 9},
    {10, 0,  9,  14, 6, 3,  15, 5,  1,  13, 12, 7,  11, 4,  2,  8,
     13, 7,  0,  9,  3, 4,  6,  10, 2,  8,  5,  14, 12, 11, 15, 1,
     13, 6,  4,  9,  8, 15, 3,  0,  11, 1,  2,  12, 5,  10, 14, 7,
     1,  10, 13, 0,  6, 9,  8,  7,  4,  15, 14, 3,  11, 5,  2,  12},
    {7,  13, 14, 3, 0,  6,  9,  10, 1,  2, 8, 5,  11, 12, 4,  15,
     13, 8,  11, 5, 6,  15, 0,  3,  4,  7, 2, 12, 1,  10, 14, 9,
     10, 6,  9,  0, 12, 11, 7,  13, 15, 1, 3, 14, 5,  2,  8,  4,
     3,  15, 0,  6, 10, 1,  13, 8,  9,  4, 5, 11, 12, 7,  2,  14},
    {2,  12, 4,  1,  7,  10, 11, 6,  8,  5,  3,  15, 13, 0, 14, 9,
     14, 11, 2,  12, 4,  7,  13, 1,  5,  0,  15, 10, 3,  9, 8,  6,
     4,  2,  1,  11, 10, 13, 7,  8,  15, 9,  12, 5,  6,  3, 0,  14,
     11, 8,  12, 7,  1,  14, 2,  13, 6,  15, 0,  9,  10, 4, 5,  3},
    {12, 1,  10, 15, 9, 2,  6,  8,  0,  13, 3,  4,  14, 7,  5,  11,
     10, 15, 4,  2,  7, 12, 9,  5,  6,  1,  13, 14, 0,  11, 3,  8,
     9,  14, 15, 5,  2, 8,  12, 3,  7,  0,  4,  10, 1,  13, 11, 6,
     4,  3,  2,  12, 9, 5,  15, 10, 11, 14, 1,  7,  6,  0,  8,  13},
    {4,  11, 2,  14, 15, 0, 8,  13, 3,  12, 9,  7,  5,  10, 6, 1,
     13, 0,  11, 7,  4,  9, 1,  10, 14, 3,  5,  12, 2,  15, 8, 6,
     1,  4,  11, 13, 12, 3, 7,  14, 10, 15, 6,  8,  0,  5,  9, 2,
     6,  11, 13, 8,  1,  4, 10, 7,  9,  5,  0,  15, 14, 2,  3, 12},
    {13, 2,  8,  4,  6,  15, 11, 1,  10, 9,  3,  14, 5,  0,  12, 7,
     1,  15, 13, 8,  10, 3,  7,  4,  12, 5,  6,  11, 0,  14, 9,  2,
     7,  11, 4,  1,  9,  12, 14, 2,  0,  6,  10, 13, 15, 3,  5,  8,
     2,  1,  14, 7,  4,  10, 8,  13, 15, 12, 9,  0,  3,  5,  6,  11}};

// The four built-in product keys. The reference implementation holds them as little-endian quad words
// (0x0489cf09a84cb420, 0xf03606ff259275dd, 0xa9e9721989bca97c,
// 0x4f279ef1fad9666e); DES numbers key bit 1 as the most significant bit of the
// FIRST key BYTE, so the schedule has to see those quad words in memory order,
// which is the byte reverse of the value above.  Storing them pre-reversed keeps
// one convention - "byte 0 is the MSB of the pcsSubkeys() input" - for the
// built-in keys and for a key recovered from the password verifier alike.
// Transcribing the quad words verbatim is what made every header probe fail.
//   0x0489cf09a84cb420 -> bytes 20 b4 4c a8 09 cf 89 04
//   0xf03606ff259275dd -> bytes dd 75 92 25 ff 06 36 f0
//   0xa9e9721989bca97c -> bytes 7c a9 bc 89 19 72 e9 a9
//   0x4f279ef1fad9666e -> bytes 6e 66 d9 fa f1 9e 27 4f
const quint64 PCS_BUILTIN_KEYS[4] = {
    Q_UINT64_C(0x20b44ca809cf8904), Q_UINT64_C(0xdd759225ff0636f0),
    Q_UINT64_C(0x7ca9bc891972e9a9), Q_UINT64_C(0x6e66d9faf19e274f)};

quint64 pcsPermute(quint64 nValue, qint32 nInputBits, const int *pTable,
                   qint32 nOutputBits)
{
    quint64 nResult = 0;
    for (qint32 i = 0; i < nOutputBits; ++i) {
        const qint32 nFrom = pTable[i];
        const quint64 nBit =
            (nValue >> (nInputBits - nFrom)) & Q_UINT64_C(1);
        nResult |= nBit << (nOutputBits - 1 - i);
    }
    return nResult;
}

quint32 pcsRotate28(quint32 nValue, qint32 nCount)
{
    return ((nValue << nCount) | (nValue >> (28 - nCount))) & 0x0fffffffU;
}

void pcsSubkeys(quint64 nKey, quint64 *pSubkeys)
{
    const quint64 nPermuted = pcsPermute(nKey, 64, PCS_PC1, 56);
    quint32 nC = static_cast<quint32>((nPermuted >> 28) & 0x0fffffffU);
    quint32 nD = static_cast<quint32>(nPermuted & 0x0fffffffU);
    for (qint32 i = 0; i < 16; ++i) {
        nC = pcsRotate28(nC, PCS_SHIFTS[i]);
        nD = pcsRotate28(nD, PCS_SHIFTS[i]);
        const quint64 nCD =
            (static_cast<quint64>(nC) << 28) | static_cast<quint64>(nD);
        pSubkeys[i] = pcsPermute(nCD, 56, PCS_PC2, 48);
    }
}

quint32 pcsFeistel(quint32 nRight, quint64 nSubkey)
{
    const quint64 nExpanded = pcsPermute(nRight, 32, PCS_E, 48) ^ nSubkey;
    quint32 nMerged = 0;
    for (qint32 i = 0; i < 8; ++i) {
        const quint32 nSix =
            static_cast<quint32>((nExpanded >> (42 - i * 6)) & 0x3fU);
        const quint32 nRow = ((nSix >> 4) & 0x02U) | (nSix & 0x01U);
        const quint32 nColumn = (nSix >> 1) & 0x0fU;
        nMerged |= static_cast<quint32>(PCS_SBOX[i][nRow * 16 + nColumn])
                   << (28 - i * 4);
    }
    return static_cast<quint32>(pcsPermute(nMerged, 32, PCS_P, 32));
}

// One decrypting block pass. nRounds < 3 skips IP/FP, which is what the reference implementation's
// The reference implementation does; the halves are still swapped before the result is stored.
void pcsDecryptBlock(const quint8 *pIn, quint8 *pOut, const quint64 *pSubkeys,
                     qint32 nRounds)
{
    quint64 nBlock = 0;
    for (qint32 i = 0; i < 8; ++i) {
        nBlock = (nBlock << 8) | pIn[i];
    }
    if (nRounds >= 3) nBlock = pcsPermute(nBlock, 64, PCS_IP, 64);

    quint32 nLeft = static_cast<quint32>(nBlock >> 32);
    quint32 nRight = static_cast<quint32>(nBlock & 0xffffffffU);
    for (qint32 i = nRounds - 1; i >= 0; --i) {
        const quint32 nNext = nLeft ^ pcsFeistel(nRight, pSubkeys[i]);
        nLeft = nRight;
        nRight = nNext;
    }

    quint64 nResult = (static_cast<quint64>(nRight) << 32) |
                      static_cast<quint64>(nLeft);
    if (nRounds >= 3) nResult = pcsPermute(nResult, 64, PCS_FP, 64);
    for (qint32 i = 0; i < 8; ++i) {
        pOut[i] = static_cast<quint8>((nResult >> (56 - i * 8)) & 0xffU);
    }
}

quint32 pcsSwap32(quint32 nValue)
{
    return ((nValue & 0x000000ffU) << 24) | ((nValue & 0x0000ff00U) << 8) |
           ((nValue & 0x00ff0000U) >> 8) | ((nValue & 0xff000000U) >> 24);
}

void pcsFixHeaderByteOrder(QByteArray *pbaHeader)
{
    uchar *pData = reinterpret_cast<uchar *>(pbaHeader->data());
    const int nDwordOffsets[7] = {4, 8, 0x18, 0x1c, 0x20, 0x24, 0x38};
    for (qint32 i = 0; i < 7; ++i) {
        const quint32 nValue =
            pcsSwap32(qFromLittleEndian<quint32>(pData + nDwordOffsets[i]));
        qToLittleEndian<quint32>(nValue, pData + nDwordOffsets[i]);
    }
    const int nWordOffsets[2] = {0x0c, 0x16};
    for (qint32 i = 0; i < 2; ++i) {
        const quint16 nValue = qFromLittleEndian<quint16>(pData + nWordOffsets[i]);
        qToLittleEndian<quint16>(
            static_cast<quint16>(((nValue & 0x00ffU) << 8) | (nValue >> 8)),
            pData + nWordOffsets[i]);
    }
}

// The LZW engine EA shares, configured the way PCSECURE runs it: 14-bit codes,
// CLEAR only (no end code), first assignable code 0x101.
bool pcsLzw(const QByteArray &baPacked, qint64 nLimit, QByteArray *pOutput,
            XBinary::PDSTRUCT *pPdStruct)
{
    const qint32 nMaxBits = 14;
    const qint32 nMaxCodes = 1 << nMaxBits;
    const qint32 nFirstCode = 0x101;

    QVector<quint16> vectPrefix(nMaxCodes, 0);
    QVector<quint8> vectSuffix(nMaxCodes, 0);
    for (qint32 i = 0; i < 256; ++i) vectSuffix[i] = static_cast<quint8>(i);
    QVector<quint8> vectStack(nMaxCodes + 1, 0);

    const quint8 *pData = reinterpret_cast<const quint8 *>(baPacked.constData());
    const qint64 nSize = baPacked.size();
    qint64 nPos = 0;
    quint32 nBuffer = 0;
    qint32 nBits = 0;

    qint32 nWidth = 9;
    qint32 nMaxCode = 0x1ff;
    qint32 nNextFree = nFirstCode;
    qint32 nPrevCode = 0;
    qint32 nFirstChar = 0;
    qint64 nLeft = nLimit;
    qint32 nCounter = 0;
    bool bFirst = true;

    QByteArray baResult;
    baResult.reserve(static_cast<qint32>(qMin<qint64>(nLimit, PCS_MAX_OUTPUT)));

    while (nLeft > 0) {
        if ((++nCounter & 0xffff) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) break;
        }
        if (nMaxCode < nNextFree) {
            ++nWidth;
            nMaxCode = (nWidth == nMaxBits) ? nMaxCodes : ((1 << nWidth) - 1);
        }
        while (nBits < nWidth) {
            if (nPos >= nSize) break;
            nBuffer |= static_cast<quint32>(pData[nPos]) << nBits;
            ++nPos;
            nBits += 8;
        }
        if (nBits < nWidth) break;
        const qint32 nCode = static_cast<qint32>(nBuffer & ((1U << nWidth) - 1U));
        nBuffer >>= nWidth;
        nBits -= nWidth;

        if (bFirst) {
            // The opening code is emitted verbatim and seeds the previous code.
            bFirst = false;
            nPrevCode = nCode;
            nFirstChar = nCode & 0xff;
            baResult.append(static_cast<char>(static_cast<quint8>(nCode)));
            --nLeft;
            continue;
        }

        if (nCode == 0x100) {
            nWidth = 9;
            nMaxCode = 0x1ff;
            nNextFree = nFirstCode;
            if (nMaxCode < nNextFree) {
                ++nWidth;
                nMaxCode = (nWidth == nMaxBits) ? nMaxCodes : ((1 << nWidth) - 1);
            }
            while (nBits < nWidth) {
                if (nPos >= nSize) break;
                nBuffer |= static_cast<quint32>(pData[nPos]) << nBits;
                ++nPos;
                nBits += 8;
            }
            if (nBits < nWidth) break;
            const qint32 nSeed =
                static_cast<qint32>(nBuffer & ((1U << nWidth) - 1U));
            nBuffer >>= nWidth;
            nBits -= nWidth;
            if (nSeed > 0xff) break;  // 0x100 or a non-literal ends the stream
            nPrevCode = nSeed;
            nFirstChar = nSeed;
            baResult.append(static_cast<char>(static_cast<quint8>(nSeed)));
            --nLeft;
            continue;
        }

        qint32 nStackSize = 0;
        qint32 nCurrent = nCode;
        if (nCode >= nNextFree) {
            vectStack[nStackSize++] = static_cast<quint8>(nFirstChar);
            nCurrent = nPrevCode;
        }
        bool bOverflow = false;
        while (nCurrent > 0xff) {
            if (nStackSize >= nMaxCodes) {
                bOverflow = true;
                break;
            }
            vectStack[nStackSize++] = vectSuffix[nCurrent];
            nCurrent = vectPrefix[nCurrent];
        }
        if (bOverflow) break;
        vectStack[nStackSize++] = static_cast<quint8>(nCurrent & 0xff);
        nFirstChar = nCurrent & 0xff;

        while ((nStackSize > 0) && (nLeft > 0)) {
            --nStackSize;
            baResult.append(static_cast<char>(vectStack[nStackSize]));
            --nLeft;
        }
        if (nLeft == 0) break;

        if (nNextFree < nMaxCodes) {
            vectPrefix[nNextFree] = static_cast<quint16>(nPrevCode);
            vectSuffix[nNextFree] = static_cast<quint8>(nFirstChar);
            ++nNextFree;
        }
        nPrevCode = nCode;
    }

    *pOutput = baResult;
    return (baResult.size() == nLimit);
}
}  // namespace

const quint64 *XPCSecureDecoder::builtinKeys()
{
    return PCS_BUILTIN_KEYS;
}

qint32 XPCSecureDecoder::builtinKeyCount()
{
    return 4;
}

bool XPCSecureDecoder::tryHeader(const QByteArray &baHeader, quint64 nKey,
                                 qint32 nRounds, QByteArray *pbaHeader)
{
    if (!pbaHeader) return false;
    if (baHeader.size() < PCS_HEADER_SIZE) return false;
    if ((nRounds < 0) || (nRounds > PC_SECURE_MAX_ROUNDS)) return false;

    quint64 subkeys[16] = {};
    pcsSubkeys(nKey, subkeys);

    QByteArray baPlain = baHeader.left(PCS_HEADER_SIZE);
    const quint8 *pSource =
        reinterpret_cast<const quint8 *>(baHeader.constData());
    quint8 *pTarget = reinterpret_cast<quint8 *>(baPlain.data());
    // Bytes 4..59 are the encrypted region: seven blocks; 0..3 is the ASCII
    // signature and 60..67 the plaintext password verifier.
    for (qint32 i = 0; i < 7; ++i) {
        pcsDecryptBlock(pSource + 4 + i * 8, pTarget + 4 + i * 8, subkeys,
                        nRounds);
    }

    const uchar *pData = reinterpret_cast<const uchar *>(baPlain.constData());
    const quint32 nHigh = qFromLittleEndian<quint32>(pData + 0x28) ^
                          qFromLittleEndian<quint32>(pData + 0x30);
    const quint32 nLow = qFromLittleEndian<quint32>(pData + 0x2c) ^
                         qFromLittleEndian<quint32>(pData + 0x34);
    // "SeaHawks", split across two XOR pairs.
    if ((nHigh != 0x48616553U) || (nLow != 0x736b7761U)) return false;

    const quint16 nPayloadRounds = qFromLittleEndian<quint16>(pData + 0x0c);
    // The reference implementation requires the payload round count to be 0..16 before it will accept
    // the header, which is what stops a lucky verifier collision.
    const quint16 nSwapped =
        static_cast<quint16>(((nPayloadRounds & 0x00ffU) << 8) |
                             (nPayloadRounds >> 8));
    if (nSwapped > PC_SECURE_MAX_ROUNDS) return false;

    pcsFixHeaderByteOrder(&baPlain);
    *pbaHeader = baPlain;
    return true;
}

bool XPCSecureDecoder::decode(const QByteArray &baPacked,
                              qint64 nUncompressedSize,
                              const QByteArray &baProperty, QByteArray *pOutput,
                              XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput) return false;
    pOutput->clear();
    if (baProperty.size() != PC_SECURE_PROPERTY_SIZE) return false;
    if ((nUncompressedSize < 0) || (nUncompressedSize > PCS_MAX_OUTPUT)) {
        return false;
    }
    if (nUncompressedSize == 0) return true;

    const uchar *pProperty =
        reinterpret_cast<const uchar *>(baProperty.constData());
    quint64 nKey = 0;
    for (qint32 i = 0; i < 8; ++i) {
        nKey = (nKey << 8) | pProperty[i];
    }
    const qint32 nRounds = static_cast<qint32>(pProperty[8]);
    const quint8 nFlags = pProperty[9];
    const qint64 nCompressedSize =
        static_cast<qint64>(qFromLittleEndian<quint32>(pProperty + 10));
    if ((nRounds < 0) || (nRounds > PC_SECURE_MAX_ROUNDS)) return false;

    quint64 subkeys[16] = {};
    pcsSubkeys(nKey, subkeys);

    // The cipher is ECB over whole 8-byte blocks; a short tail is carried
    // through untouched, exactly as the reference implementation's block stream does.
    QByteArray baPlain = baPacked;
    const qint64 nBlocks = baPacked.size() / PC_SECURE_BLOCK_SIZE;
    const quint8 *pSource =
        reinterpret_cast<const quint8 *>(baPacked.constData());
    quint8 *pTarget = reinterpret_cast<quint8 *>(baPlain.data());
    for (qint64 i = 0; i < nBlocks; ++i) {
        if ((i & 0xffff) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }
        pcsDecryptBlock(pSource + i * PC_SECURE_BLOCK_SIZE,
                        pTarget + i * PC_SECURE_BLOCK_SIZE, subkeys, nRounds);
    }

    if (nFlags & 0x01U) {
        qint64 nInput = nCompressedSize;
        if ((nInput <= 0) || (nInput > baPlain.size())) nInput = baPlain.size();
        return pcsLzw(baPlain.left(static_cast<qint32>(nInput)),
                      nUncompressedSize, pOutput, pPdStruct);
    }

    if (baPlain.size() < nUncompressedSize) return false;
    *pOutput = baPlain.left(static_cast<qint32>(nUncompressedSize));
    return true;
}
