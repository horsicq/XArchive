/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xbthpakdecoder.h"

#include <cstring>
#include <limits>

namespace {
// Gage's own BLOCKSIZE is 5000 and the corpus tops out at 4819 packed bytes,
// so a block length is always representable in the 16-bit field; the value is
// only used to keep the block counter honest.
const qint32 BTHPAK_MAX_BLOCKS = 1 << 20;
// Gage's expand() uses stack[30].  A crafted pair table can make a code expand
// into itself and grow the stack without bound, which in the original C is a
// straight buffer overflow.  The deepest genuine expansion in the whole family
// is 18, so this bound costs nothing and turns the overflow into a rejection.
const qint32 BTHPAK_MAX_STACK = 1024;
const qint32 BTHPAK_ALPHABET_SIZE = 256;
const qint32 BTHPAK_SKIP_BIAS = 127;

// Parses one block's pair table starting at *pnPosition.
//
// Layout: a run-length coded walk over the 256 code slots.  A control byte
// above 127 skips (count - 127) slots and leaves them as literals; otherwise it
// introduces (count + 1) explicit entries.  For each entry the stored "left"
// byte is read first, and the "right" byte follows ONLY when left != c -- an
// entry whose left byte equals its own index is the literal marker and carries
// no second byte.  Reading the right byte unconditionally desynchronises the
// table from its first literal entry onward, which then mis-frames the block
// length field and the whole chain; this rule fires in nearly every table.
//
// pLeft/pRight may be null when the caller only needs to skip the table.
bool bthpakParseTable(const uchar *pData, qint64 nSize, qint64 *pnPosition,
                      quint8 *pLeft, quint8 *pRight)
{
    if (!pData || !pnPosition) return false;

    if (pLeft) {
        for (qint32 i = 0; i < BTHPAK_ALPHABET_SIZE; ++i) {
            pLeft[i] = static_cast<quint8>(i);
        }
    }

    qint64 nPosition = *pnPosition;
    if (nPosition < 0 || nPosition >= nSize) return false;

    qint32 nCount = pData[nPosition++];
    qint32 nCode = 0;

    for (;;) {
        // The bias really is 127, not 128; verified against the real tables.
        if (nCount > 127) {
            nCode += nCount - BTHPAK_SKIP_BIAS;
            nCount = 0;
        }
        // The historical code would happily run past left[]/right[] here.  A
        // genuine table never pushes the index beyond 256.
        if (nCode > BTHPAK_ALPHABET_SIZE) return false;
        if (nCode == BTHPAK_ALPHABET_SIZE) break;

        bool bTableComplete = false;
        for (qint32 k = 0; k <= nCount; ++k) {
            if (nCode >= BTHPAK_ALPHABET_SIZE) {
                bTableComplete = true;
                break;
            }
            if (nPosition >= nSize) return false;
            const quint8 nLeft = pData[nPosition++];
            if (pLeft) pLeft[nCode] = nLeft;
            if (nLeft != static_cast<quint8>(nCode)) {
                if (nPosition >= nSize) return false;
                const quint8 nRight = pData[nPosition++];
                if (pRight) pRight[nCode] = nRight;
            }
            ++nCode;
        }
        // The completion test is deliberately made twice -- once right after
        // the skip and once after the entry run.  Both exits occur in the
        // corpus (1212 and 2546 times); implementing only one of them
        // mis-frames a third of the blocks.
        if (bTableComplete || (nCode >= BTHPAK_ALPHABET_SIZE)) break;

        if (nPosition >= nSize) return false;
        nCount = pData[nPosition++];
    }

    *pnPosition = nPosition;
    return true;
}

// Reads the block's packed length.  BIG-endian, inside an otherwise
// little-endian container.
bool bthpakReadBlockSize(const uchar *pData, qint64 nSize, qint64 *pnPosition,
                         qint32 *pnBlockSize)
{
    if (!pData || !pnPosition || !pnBlockSize) return false;
    if (*pnPosition < 0 || *pnPosition > nSize - 2) return false;

    const qint32 nBlockSize = (static_cast<qint32>(pData[*pnPosition]) << 8) |
                              static_cast<qint32>(pData[*pnPosition + 1]);
    *pnPosition += 2;
    // A zero-length block makes the original decoder spin forever; the genuine
    // minimum in the family is 5.
    if (nBlockSize == 0) return false;
    if (nBlockSize > nSize - *pnPosition) return false;

    *pnBlockSize = nBlockSize;
    return true;
}
}  // namespace

bool XBTHPAKDecoder::scanChain(const QByteArray &payload,
                               bool bRequireExactEnd, qint32 nBlockLimit,
                               qint64 *pnConsumed, qint32 *pnBlockCount,
                               XBinary::PDSTRUCT *pPdStruct)
{
    if (pnConsumed) *pnConsumed = 0;
    if (pnBlockCount) *pnBlockCount = 0;
    if (payload.isEmpty() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const uchar *pData = reinterpret_cast<const uchar *>(payload.constData());
    const qint64 nSize = payload.size();
    qint64 nPosition = 0;
    qint32 nBlockCount = 0;

    while (nPosition < nSize) {
        if (nBlockCount >= BTHPAK_MAX_BLOCKS) return false;
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (!bthpakParseTable(pData, nSize, &nPosition, nullptr, nullptr)) {
            return false;
        }
        qint32 nBlockSize = 0;
        if (!bthpakReadBlockSize(pData, nSize, &nPosition, &nBlockSize)) {
            return false;
        }
        nPosition += nBlockSize;
        ++nBlockCount;
        if ((nBlockLimit > 0) && (nBlockCount >= nBlockLimit)) break;
    }

    if (bRequireExactEnd && (nPosition != nSize)) return false;
    if (nBlockCount == 0) return false;

    if (pnConsumed) *pnConsumed = nPosition;
    if (pnBlockCount) *pnBlockCount = nBlockCount;
    return true;
}

bool XBTHPAKDecoder::decode(const QByteArray &packed, qint64 expectedSize,
                            QByteArray *output, XBinary::PDSTRUCT *pPdStruct)
{
    if (output) output->clear();
    if (!output || packed.isEmpty() || expectedSize <= 0 ||
        expectedSize > (std::numeric_limits<qint32>::max)() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const uchar *pData = reinterpret_cast<const uchar *>(packed.constData());
    const qint64 nSize = packed.size();

    QByteArray result;
    result.reserve(static_cast<qint32>(expectedSize));

    quint8 left[BTHPAK_ALPHABET_SIZE];
    quint8 right[BTHPAK_ALPHABET_SIZE];
    quint8 stack[BTHPAK_MAX_STACK];

    qint64 nPosition = 0;
    qint32 nBlockCount = 0;

    while (nPosition < nSize) {
        if (nBlockCount >= BTHPAK_MAX_BLOCKS) return false;
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        memset(right, 0, sizeof(right));
        if (!bthpakParseTable(pData, nSize, &nPosition, left, right)) {
            return false;
        }
        qint32 nBlockSize = 0;
        if (!bthpakReadBlockSize(pData, nSize, &nPosition, &nBlockSize)) {
            return false;
        }
        ++nBlockCount;

        qint32 nStackDepth = 0;
        qint32 nRemaining = nBlockSize;
        for (;;) {
            quint8 nCode = 0;
            if (nStackDepth > 0) {
                nCode = stack[--nStackDepth];
            } else {
                if (nRemaining == 0) break;
                --nRemaining;
                nCode = pData[nPosition++];
            }

            if (nCode == left[nCode]) {
                // Emitting past the declared size means the pair table encodes
                // a longer stream than the header promises: refuse instead of
                // growing the buffer on attacker-controlled input.
                if (result.size() >= expectedSize) return false;
                result.append(static_cast<char>(nCode));
            } else {
                if (nStackDepth > BTHPAK_MAX_STACK - 2) return false;
                stack[nStackDepth++] = right[nCode];
                stack[nStackDepth++] = left[nCode];
            }
        }
    }

    // The chain carries no end marker, so "consumed the payload exactly" and
    // "produced exactly the declared length" are the only completeness proofs
    // available.  Both hold on every genuine file.
    if ((nPosition != nSize) || (result.size() != expectedSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    *output = result;
    return true;
}
