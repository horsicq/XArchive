/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 *
 * Visioneer PaperPort page renderer.  See xpaperportdecoder.h for the object
 * layout; this file carries the tile codec and the BMP writer.
 *
 * The run-length and mode tables are the public CCITT T.4 / T.6 code tables,
 * written out here as bit strings and expanded into flat lookup tables the way
 * the reference extractor does (place each code at index code << (bits - len),
 * then propagate each entry forward over the slots that no code claimed).
 */
#include "xpaperportdecoder.h"

#include <QtEndian>

#include <limits>

namespace {
const qint32 PP_MODE_BITS = 7;
const qint32 PP_RUN_BITS = 13;
const qint32 PP_MODE_SIZE = 1 << PP_MODE_BITS;
const qint32 PP_RUN_SIZE = 1 << PP_RUN_BITS;
const qint32 PP_OBJECT_HEADER_SIZE = 0x88;
const qint32 PP_TILE_HEADER_SIZE = 14;
const qint32 PP_TILE_DIR_SIZE = 10;
const qint32 PP_BMP_HEADER_SIZE = 14 + 40 + 8;
const qint32 PP_MAX_DIMENSION = 65535;
// The renderer never needs more than a page of canvas; 256 MiB is a hard stop
// for a corrupt geometry.
const qint64 PP_MAX_CANVAS = 256LL * 1024 * 1024;

struct PPCODE {
    quint8 nLength;
    quint16 nValue;
};

const char *const PP_WHITE_TERM[64] = {
    "00110101", "000111",   "0111",     "1000",     "1011",     "1100",     "1110",     "1111",     "10011",    "10100",    "00111",
    "01000",    "001000",   "000011",   "110100",   "110101",   "101010",   "101011",   "0100111",  "0001100",  "0001000",  "0010111",
    "0000011",  "0000100",  "0101000",  "0101011",  "0010011",  "0100100",  "0011000",  "00000010", "00000011", "00011010", "00011011",
    "00010010", "00010011", "00010100", "00010101", "00010110", "00010111", "00101000", "00101001", "00101010", "00101011", "00101100",
    "00101101", "00000100", "00000101", "00001010", "00001011", "01010010", "01010011", "01010100", "01010101", "00100100", "00100101",
    "01011000", "01011001", "01011010", "01011011", "01001010", "01001011", "00110010", "00110011", "00110100"};

const char *const PP_WHITE_MAKEUP[27] = {"11011",     "10010",     "010111",    "0110111",   "00110110",  "00110111",  "01100100",
                                         "01100101",  "01101000",  "01100111",  "011001100", "011001101", "011010010", "011010011",
                                         "011010100", "011010101", "011010110", "011010111", "011011000", "011011001", "011011010",
                                         "011011011", "010011000", "010011001", "010011010", "011000",    "010011011"};

const char *const PP_BLACK_TERM[64] = {
    "0000110111",   "010",          "11",           "10",           "011",          "0011",         "0010",         "00011",
    "000101",       "000100",       "0000100",      "0000101",      "0000111",      "00000100",     "00000111",     "000011000",
    "0000010111",   "0000011000",   "0000001000",   "00001100111",  "00001101000",  "00001101100",  "00000110111",  "00000101000",
    "00000010111",  "00000011000",  "000011001010", "000011001011", "000011001100", "000011001101", "000001101000", "000001101001",
    "000001101010", "000001101011", "000011010010", "000011010011", "000011010100", "000011010101", "000011010110", "000011010111",
    "000001101100", "000001101101", "000011011010", "000011011011", "000001010100", "000001010101", "000001010110", "000001010111",
    "000001100100", "000001100101", "000001010010", "000001010011", "000000100100", "000000110111", "000000111000", "000000100111",
    "000000101000", "000001011000", "000001011001", "000000101011", "000000101100", "000001011010", "000001100110", "000001100111"};

const char *const PP_BLACK_MAKEUP[27] = {"0000001111",    "000011001000",  "000011001001",  "000001011011",  "000000110011",
                                         "000000110100",  "000000110101",  "0000001101100", "0000001101101", "0000001001010",
                                         "0000001001011", "0000001001100", "0000001001101", "0000001110010", "0000001110011",
                                         "0000001110100", "0000001110101", "0000001110110", "0000001110111", "0000001010010",
                                         "0000001010011", "0000001010100", "0000001010101", "0000001011010", "0000001011011",
                                         "0000001100100", "0000001100101"};

const char *const PP_EXT_MAKEUP[13] = {"00000001000",  "00000001100",  "00000001101",  "000000010010", "000000010011",
                                       "000000010100", "000000010101", "000000010110", "000000010111", "000000011100",
                                       "000000011101", "000000011110", "000000011111"};

// Vertical -3..+3, then horizontal, then pass: the reference table stores the
// index, and the decoder reads it as (index - 3).
const char *const PP_MODE_CODES[9] = {"0000010", "000010", "010", "1", "011", "000011", "0000011", "001", "0001"};

void ppPlace(QVector<PPCODE> *pTable, qint32 nBits, const char *pCode, quint16 nValue)
{
    qint32 nLength = 0;
    quint32 nCode = 0;
    while (pCode[nLength] != '\0') {
        nCode = (nCode << 1) | static_cast<quint32>(pCode[nLength] == '1' ? 1 : 0);
        nLength++;
    }
    if ((nLength < 1) || (nLength > nBits)) return;
    const qint32 nIndex = static_cast<qint32>(nCode << (nBits - nLength));
    if ((nIndex < 0) || (nIndex >= pTable->size())) return;
    PPCODE code;
    code.nLength = static_cast<quint8>(nLength);
    code.nValue = nValue;
    (*pTable)[nIndex] = code;
}

void ppExpand(QVector<PPCODE> *pTable)
{
    for (qint32 i = 1; i < pTable->size(); i++) {
        if (pTable->at(i).nLength == 0) (*pTable)[i] = pTable->at(i - 1);
    }
}

void ppBuildRunTable(QVector<PPCODE> *pTable, const char *const *ppTerm, const char *const *ppMakeup)
{
    pTable->fill(PPCODE{0, 0}, PP_RUN_SIZE);
    for (qint32 i = 0; i < 64; i++) ppPlace(pTable, PP_RUN_BITS, ppTerm[i], static_cast<quint16>(i));
    for (qint32 i = 0; i < 27; i++) ppPlace(pTable, PP_RUN_BITS, ppMakeup[i], static_cast<quint16>((i + 1) << 6));
    for (qint32 i = 0; i < 13; i++) ppPlace(pTable, PP_RUN_BITS, PP_EXT_MAKEUP[i], static_cast<quint16>((i + 28) << 6));
    ppExpand(pTable);
}

void ppBuildModeTable(QVector<PPCODE> *pTable)
{
    pTable->fill(PPCODE{0, 0}, PP_MODE_SIZE);
    for (qint32 i = 0; i < 9; i++) ppPlace(pTable, PP_MODE_BITS, PP_MODE_CODES[i], static_cast<quint16>(i));
    ppExpand(pTable);
}

// The whole codec runs against one padded copy of the tile payload, so every
// read is bounds checked once, here.
class PPInput {
public:
    PPInput(const quint8 *pData, qint32 nSize) : m_pData(pData), m_nSize(nSize)
    {
    }

    bool byteAt(qint32 nOffset, quint8 *pValue) const
    {
        if ((nOffset < 0) || (nOffset >= m_nSize)) return false;
        *pValue = m_pData[nOffset];
        return true;
    }

    bool be16At(qint32 nOffset, quint32 *pValue) const
    {
        if ((nOffset < 0) || (nOffset > m_nSize - 2)) return false;
        *pValue = (static_cast<quint32>(m_pData[nOffset]) << 8) | m_pData[nOffset + 1];
        return true;
    }

    bool be32At(qint32 nOffset, quint32 *pValue) const
    {
        if ((nOffset < 0) || (nOffset > m_nSize - 4)) return false;
        *pValue = (static_cast<quint32>(m_pData[nOffset]) << 24) | (static_cast<quint32>(m_pData[nOffset + 1]) << 16) |
                  (static_cast<quint32>(m_pData[nOffset + 2]) << 8) | m_pData[nOffset + 3];
        return true;
    }

    bool copyOut(qint32 nOffset, qint32 nCount, quint8 *pDestination) const
    {
        if ((nOffset < 0) || (nCount < 0) || (nOffset > m_nSize - nCount)) return false;
        for (qint32 i = 0; i < nCount; i++) pDestination[i] = m_pData[nOffset + i];
        return true;
    }

    const quint8 *raw(qint32 nOffset, qint32 nCount) const
    {
        if ((nOffset < 0) || (nCount < 0) || (nOffset > m_nSize - nCount)) return nullptr;
        return m_pData + nOffset;
    }

private:
    const quint8 *m_pData;
    qint32 m_nSize;
};

struct PPCanvas {
    quint8 *pData;
    qint64 nSize;
    qint32 nStride;
};

bool ppCanvasFill(PPCanvas *pCanvas, qint64 nOffset, quint8 nValue)
{
    if ((nOffset < 0) || (nOffset >= pCanvas->nSize)) return false;
    pCanvas->pData[nOffset] = nValue;
    return true;
}

bool ppCanvasOr(PPCanvas *pCanvas, qint64 nOffset, quint8 nValue)
{
    if ((nOffset < 0) || (nOffset >= pCanvas->nSize)) return false;
    pCanvas->pData[nOffset] |= nValue;
    return true;
}

bool ppCanvasAnd(PPCanvas *pCanvas, qint64 nOffset, quint8 nValue)
{
    if ((nOffset < 0) || (nOffset >= pCanvas->nSize)) return false;
    pCanvas->pData[nOffset] &= nValue;
    return true;
}

// Changing elements of a raw 1 bpp row, white first.
bool ppBuildChangingElements(const PPInput &input, qint32 nOffset, qint32 nWidth, QVector<qint32> *pBuffer, qint32 nBase)
{
    const qint32 nRowBytes = (nWidth + 7) >> 3;
    qint32 p = nBase;
    qint32 nColor = 0;
    qint32 nPosition = 0;
    for (qint32 k = 0; k < nRowBytes; k++) {
        quint8 nByte = 0;
        if (!input.byteAt(nOffset + k, &nByte)) return false;
        for (qint32 nBit = 0; nBit < 8; nBit++) {
            const qint32 nValue = (nByte >> (7 - nBit)) & 1;
            if (nValue != nColor) {
                if ((p < 0) || (p >= pBuffer->size())) return false;
                (*pBuffer)[p] = nPosition + nBit;
                p++;
                nColor = nValue;
            }
        }
        nPosition += 8;
    }
    if ((p < 0) || (p + 2 >= pBuffer->size())) return false;
    (*pBuffer)[p] = nWidth;
    (*pBuffer)[p + 1] = nWidth;
    (*pBuffer)[p + 2] = nWidth;
    return true;
}

// Paints the black runs described by a changing element list into one row.
bool ppRenderRow(const QVector<qint32> &buffer, qint32 nBase, PPCanvas *pCanvas, qint64 nRowOffset, qint32 nWidth)
{
    qint32 p = nBase;
    if ((p < 0) || (p >= buffer.size())) return false;
    qint32 a = buffer.at(p);
    if (a == nWidth) return true;
    p++;
    while (a < nWidth) {
        if ((p < 0) || (p >= buffer.size())) return false;
        const qint32 b = buffer.at(p);
        p++;
        if (a < 0) return false;
        qint64 i = nRowOffset + (a >> 3);
        if (!ppCanvasOr(pCanvas, i, static_cast<quint8>(0xFFU >> (a & 7)))) return false;
        a = a - (a & 7);
        while (true) {
            a += 8;
            if (b <= a) break;
            if (!ppCanvasFill(pCanvas, i + 1, 0xFF)) return false;
            i++;
        }
        if (a != b) {
            const qint32 nShift = (8 - (b & 7)) & 0x1f;
            const quint8 nMask = (nShift >= 8) ? 0 : static_cast<quint8>(0xFFU << nShift);
            if (!ppCanvasAnd(pCanvas, i, nMask)) return false;
        }
        if (b >= nWidth) return true;
        if ((p < 0) || (p >= buffer.size())) return false;
        a = buffer.at(p);
        p++;
    }
    return true;
}

// One G4 two-dimensional coded row.  Returns the number of bits consumed in
// *pnConsumed.
bool ppDecode2D(const PPInput &input, qint32 p, qint32 nBitOffset, qint32 nWidth, QVector<qint32> *pBuffer, qint32 nReference, qint32 nCurrent,
                const QVector<PPCODE> &white, const QVector<PPCODE> &black, const QVector<PPCODE> &mode, qint32 *pnConsumed)
{
    if ((nCurrent < 0) || (nCurrent >= pBuffer->size())) return false;
    (*pBuffer)[nCurrent] = -1;
    qint32 p5 = nCurrent + 1;

    quint32 nAccumulator = 0;
    if (!input.be16At(p, &nAccumulator)) return false;
    qint32 pb = p + 2;
    qint32 nColor = 0;
    qint32 a0 = -1;
    qint32 j = nReference + 1;
    qint32 nBits = 16 - nBitOffset;
    bool bEnded = false;

    while (!bEnded) {
        while (true) {
            while (true) {
                if ((j < 0) || (j >= pBuffer->size())) return false;
                if (pBuffer->at(j) > a0) break;
                j += 2;
            }
            if (nBits < 16) {
                quint32 nWord = 0;
                if (!input.be16At(pb, &nWord)) return false;
                nAccumulator = ((nAccumulator << 16) | nWord) & 0xFFFFFFFFU;
                pb += 2;
                nBits += 16;
            }
            if ((nBits < PP_MODE_BITS) || (nBits > 32)) return false;
            const PPCODE &modeCode = mode.at(static_cast<qint32>((nAccumulator >> (nBits - PP_MODE_BITS)) & 0x7fU));
            if (modeCode.nLength == 0) return false;
            nBits -= modeCode.nLength;
            const qint32 k = static_cast<qint32>(modeCode.nValue) - 3;
            if (k == 4) break;  // horizontal
            if (k == 5) {
                if ((j + 1 < 0) || (j + 1 >= pBuffer->size())) return false;
                a0 = pBuffer->at(j + 1);
                if (a0 >= nWidth) {
                    bEnded = true;
                    break;
                }
            } else {
                a0 = pBuffer->at(j) + k;
                if ((p5 < 0) || (p5 >= pBuffer->size())) return false;
                (*pBuffer)[p5] = a0;
                p5++;
                if (a0 >= nWidth) {
                    bEnded = true;
                    break;
                }
                nColor = ~nColor;
                j--;
            }
        }
        if (bEnded) break;

        for (qint32 nRun = 0; nRun < 2; nRun++) {
            while (true) {
                if (nBits < 16) {
                    quint32 nWord = 0;
                    if (!input.be16At(pb, &nWord)) return false;
                    nAccumulator = ((nAccumulator << 16) | nWord) & 0xFFFFFFFFU;
                    pb += 2;
                    nBits += 16;
                }
                if ((nBits < PP_RUN_BITS) || (nBits > 32)) return false;
                const QVector<PPCODE> &table = (nColor != 0) ? black : white;
                const PPCODE &runCode = table.at(static_cast<qint32>((nAccumulator >> (nBits - PP_RUN_BITS)) & 0x1fffU));
                if (runCode.nLength == 0) return false;
                nBits -= runCode.nLength;
                a0 += static_cast<qint32>(runCode.nValue);
                if (a0 > nWidth + PP_MAX_DIMENSION) return false;
                if (runCode.nValue < 0x40) break;
            }
            nColor = ~nColor;
            if ((p5 < 0) || (p5 >= pBuffer->size())) return false;
            (*pBuffer)[p5] = a0;
            p5++;
        }
        if (a0 >= nWidth) break;
    }

    if ((p5 - 1 < 0) || (p5 + 1 >= pBuffer->size())) return false;
    (*pBuffer)[p5 + 1] = nWidth;
    (*pBuffer)[p5] = nWidth;
    (*pBuffer)[p5 - 1] = nWidth;
    *pnConsumed = (pb - p) * 8 - nBitOffset - nBits;
    return true;
}

// One compression-2 tile: nWidth is already rounded up to a multiple of eight.
bool ppDecodeTile(const PPInput &input, qint32 nAvailable, qint32 nWidth, qint32 nHeight, PPCanvas *pCanvas, qint64 nRowOffset,
                  const QVector<PPCODE> &white, const QVector<PPCODE> &black, const QVector<PPCODE> &mode, XBinary::PDSTRUCT *pPdStruct)
{
    if ((nWidth <= 0) || (nHeight <= 0) || (nWidth > PP_MAX_DIMENSION)) return false;
    const qint32 nRowBytes = (nWidth + 7) >> 3;
    const qint32 nSpan = nWidth + 4;
    QVector<qint32> buffer(2 * nSpan + 8, 0);
    buffer[nSpan] = -1;
    buffer[0] = -1;
    buffer[1] = nWidth;
    buffer[2] = nWidth;
    buffer[3] = nWidth;

    qint32 nNext = nSpan;
    qint32 nReference = 0;
    qint32 p = 0;
    qint32 q = 0;
    qint32 nAvail = nAvailable;
    qint64 nRow = nRowOffset;
    qint32 nRowsLeft = nHeight;

    while (true) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint32 nCurrent = nNext;
        if (q > 15) {
            q -= 16;
            p += 2;
            nAvail -= 2;
        }
        if (nAvail < 4) return false;
        quint32 nWord = 0;
        if (!input.be32At(p, &nWord)) return false;
        const qint32 nMode = static_cast<qint32>((nWord >> (30 - q)) & 3U);
        qint32 nBitPosition = q + 2;

        if (nMode == 0) {
            const qint32 nAdvance = (q + 9) >> 3;
            const qint32 lp = p + nAdvance;
            nAvail -= nAdvance;
            if (nAvail < nRowBytes) return false;
            if ((nRow < 0) || (nRow > pCanvas->nSize - nRowBytes)) return false;
            if (!input.copyOut(lp, nRowBytes, pCanvas->pData + nRow)) return false;
            if (!ppBuildChangingElements(input, lp, nWidth, &buffer, nCurrent + 1)) return false;
            p = lp + nRowBytes;
            nAvail -= nRowBytes;
            q = 0;
        } else if (nMode == 1) {
            if ((nCurrent < 0) || (nCurrent >= buffer.size())) return false;
            buffer[nCurrent] = -1;
            const qint32 nAdvance = (q + 9) >> 3;
            const qint32 lp = p + nAdvance;
            nAvail -= nAdvance;
            if (nAvail < 2) return false;
            quint32 nValue = 0;
            if (!input.be16At(lp, &nValue)) return false;
            if (static_cast<qint32>(nValue) > nWidth) return false;
            if (nCurrent + 1 >= buffer.size()) return false;
            buffer[nCurrent + 1] = static_cast<qint32>(nValue);
            qint32 nLast = nCurrent + 1;
            if (static_cast<qint32>(nValue) != nWidth) {
                qint32 k = 0;
                while (true) {
                    if (nAvail < k * 2 + 2) return false;
                    if (!input.be16At(lp + (k + 1) * 2, &nValue)) return false;
                    if (static_cast<qint32>(nValue) > nWidth) return false;
                    nLast++;
                    k++;
                    if (k > nWidth) return false;
                    if ((nLast < 0) || (nLast >= buffer.size())) return false;
                    buffer[nLast] = static_cast<qint32>(nValue);
                    if (static_cast<qint32>(nValue) == nWidth) break;
                }
            }
            if (nLast + 2 >= buffer.size()) return false;
            buffer[nLast + 1] = nWidth;
            buffer[nLast + 2] = nWidth;
            if (!ppRenderRow(buffer, nCurrent + 1, pCanvas, nRow, nWidth)) return false;
            const qint32 nBytes = (nLast - nCurrent) * 2;
            p = lp + nBytes;
            nAvail -= nBytes;
            q = 0;
        } else if (nMode == 2) {
            qint32 nConsumed = 0;
            if (!ppDecode2D(input, p, nBitPosition, nWidth, &buffer, nReference, nCurrent, white, black, mode, &nConsumed)) return false;
            const qint32 nTotal = nConsumed + nBitPosition;
            if (nTotal < 0) return false;
            const qint32 nAdvance = (nTotal >> 3) & ~1;
            q = nTotal & 0xf;
            p += nAdvance;
            nAvail -= nAdvance;
            if (!ppRenderRow(buffer, nCurrent + 1, pCanvas, nRow, nWidth)) return false;
        } else {
            if (nCurrent + 3 >= buffer.size()) return false;
            buffer[nCurrent + 1] = nWidth;
            buffer[nCurrent + 2] = nWidth;
            buffer[nCurrent + 3] = nWidth;
            if (nBitPosition > 15) {
                nBitPosition = q - 14;
                p += 2;
                nAvail -= 2;
            }
            if (nAvail < 4) return false;
            if (!input.be32At(p, &nWord)) return false;
            if ((nBitPosition < 0) || (nBitPosition > 26)) return false;
            const qint32 nSkip = static_cast<qint32>((nWord >> (26 - nBitPosition)) & 0x3fU);
            q = nBitPosition + 6;
            if (nSkip < 1) return false;
            nRow += static_cast<qint64>(pCanvas->nStride) * (nSkip - 1);
            nRowsLeft = nRowsLeft - nSkip + 1;
        }

        nRow += pCanvas->nStride;
        nRowsLeft--;
        nNext = nReference;
        nReference = nCurrent;
        if (nRowsLeft <= 0) break;
        if (nRow < 0) return false;
    }
    return true;
}

bool ppReadGeometry(const QByteArray &object, qint32 *pnWidth, qint32 *pnHeight, qint32 *pnBitsPerPixel, qint32 *pnTileHeaderOffset,
                    qint32 *pnTileDataOffset)
{
    if (object.size() < PP_OBJECT_HEADER_SIZE) return false;
    const uchar *p = reinterpret_cast<const uchar *>(object.constData());
    const qint32 nWidth = static_cast<qint32>(qFromLittleEndian<quint16>(p + 10));
    const qint32 nHeight = static_cast<qint32>(qFromLittleEndian<quint16>(p + 12));
    const qint32 nBitsPerPixel = static_cast<qint32>(qFromLittleEndian<quint16>(p + 20));
    const qint32 nTag = static_cast<qint32>(qFromLittleEndian<qint16>(p + 34));
    const qint32 nTileHeaderOffset = qFromLittleEndian<qint32>(p + 60);
    const qint32 nTileDataOffset = qFromLittleEndian<qint32>(p + 68);
    if (nTag != 5) return false;
    if ((nWidth < 1) || (nHeight < 1)) return false;
    if ((nBitsPerPixel != 1) && (nBitsPerPixel != 8) && (nBitsPerPixel != 24)) return false;
    if ((nTileHeaderOffset < PP_OBJECT_HEADER_SIZE) || (nTileDataOffset < PP_OBJECT_HEADER_SIZE)) return false;
    *pnWidth = nWidth;
    *pnHeight = nHeight;
    *pnBitsPerPixel = nBitsPerPixel;
    *pnTileHeaderOffset = nTileHeaderOffset;
    *pnTileDataOffset = nTileDataOffset;
    return true;
}
}  // namespace

bool XPaperPortDecoder::probeObjectHeader(const QByteArray &objectHeader, qint32 *pnTileHeaderOffset)
{
    if (!pnTileHeaderOffset) return false;
    qint32 nWidth = 0;
    qint32 nHeight = 0;
    qint32 nBitsPerPixel = 0;
    qint32 nTileHeaderOffset = 0;
    qint32 nTileDataOffset = 0;
    if (!ppReadGeometry(objectHeader, &nWidth, &nHeight, &nBitsPerPixel, &nTileHeaderOffset, &nTileDataOffset)) return false;
    *pnTileHeaderOffset = nTileHeaderOffset;
    return true;
}

bool XPaperPortDecoder::probeGeometry(const QByteArray &objectHeader, const QByteArray &tileHeader, IMAGEINFO *pInfo)
{
    if (!pInfo || (tileHeader.size() < PP_TILE_HEADER_SIZE)) return false;
    qint32 nWidth = 0;
    qint32 nHeight = 0;
    qint32 nBitsPerPixel = 0;
    qint32 nTileHeaderOffset = 0;
    qint32 nTileDataOffset = 0;
    if (!ppReadGeometry(objectHeader, &nWidth, &nHeight, &nBitsPerPixel, &nTileHeaderOffset, &nTileDataOffset)) return false;

    const uchar *pTile = reinterpret_cast<const uchar *>(tileHeader.constData());
    if (qFromLittleEndian<quint16>(pTile) != 0x000EU) return false;
    const qint32 nTileWidth = static_cast<qint32>(qFromLittleEndian<quint16>(pTile + 4));
    const qint32 nTileHeight = static_cast<qint32>(qFromLittleEndian<quint16>(pTile + 6));
    const qint32 nAcross = static_cast<qint32>(qFromLittleEndian<quint16>(pTile + 8));
    const qint32 nDown = static_cast<qint32>(qFromLittleEndian<quint16>(pTile + 10));
    if ((nTileWidth < 1) || (nTileHeight < 1) || (nAcross < 1) || (nDown < 1)) return false;

    const qint64 nStride = ((static_cast<qint64>(nWidth) * nBitsPerPixel + 31) / 32) * 4;
    if (nStride * nHeight > PP_MAX_CANVAS) return false;

    pInfo->nWidth = nWidth;
    pInfo->nHeight = nHeight;
    pInfo->nBitsPerPixel = nBitsPerPixel;
    pInfo->nStride = static_cast<qint32>(nStride);
    pInfo->nOutputSize = PP_BMP_HEADER_SIZE + nStride * nHeight;
    return true;
}

bool XPaperPortDecoder::probe(const QByteArray &object, IMAGEINFO *pInfo)
{
    qint32 nTileHeaderOffset = 0;
    if (!probeObjectHeader(object, &nTileHeaderOffset)) return false;
    if ((nTileHeaderOffset < 0) || (nTileHeaderOffset > object.size() - PP_TILE_HEADER_SIZE)) return false;
    return probeGeometry(object, object.mid(nTileHeaderOffset, PP_TILE_HEADER_SIZE), pInfo);
}

bool XPaperPortDecoder::decode(const QByteArray &packed, qint64 nUncompressedSize, QByteArray *pOutput, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput || (nUncompressedSize < PP_BMP_HEADER_SIZE) || (nUncompressedSize > static_cast<qint64>((std::numeric_limits<qint32>::max)()))) {
        return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    IMAGEINFO info = {};
    if (!probe(packed, &info)) return false;
    // Only 1 bpp pages are renderable; 8 and 24 bpp pages carry JPEG tiles.
    if (info.nBitsPerPixel != 1) return false;
    if (info.nOutputSize != nUncompressedSize) return false;

    qint32 nWidth = 0;
    qint32 nHeight = 0;
    qint32 nBitsPerPixel = 0;
    qint32 nTileHeaderOffset = 0;
    qint32 nTileDataOffset = 0;
    if (!ppReadGeometry(packed, &nWidth, &nHeight, &nBitsPerPixel, &nTileHeaderOffset, &nTileDataOffset)) return false;

    const uchar *pTile = reinterpret_cast<const uchar *>(packed.constData()) + nTileHeaderOffset;
    const qint32 nTileWidthRaw = static_cast<qint32>(qFromLittleEndian<quint16>(pTile + 4));
    const qint32 nTileHeight = static_cast<qint32>(qFromLittleEndian<quint16>(pTile + 6));
    const qint32 nAcross = static_cast<qint32>(qFromLittleEndian<quint16>(pTile + 8));
    const qint32 nDown = static_cast<qint32>(qFromLittleEndian<quint16>(pTile + 10));
    const qint32 nTileWidth = nTileWidthRaw * 8;
    if ((nTileWidth < 8) || (nTileWidth > PP_MAX_DIMENSION)) return false;
    if (static_cast<qint64>(nTileWidth) * (nAcross - 1) >= nWidth) return false;
    if (static_cast<qint64>(nTileHeight) * (nDown - 1) >= nHeight) return false;

    const qint64 nDirectorySize = static_cast<qint64>(nAcross) * nDown * PP_TILE_DIR_SIZE;
    if (nDirectorySize > packed.size() - nTileHeaderOffset - PP_TILE_HEADER_SIZE) return false;

    QVector<PPCODE> white;
    QVector<PPCODE> black;
    QVector<PPCODE> mode;
    ppBuildRunTable(&white, PP_WHITE_TERM, PP_WHITE_MAKEUP);
    ppBuildRunTable(&black, PP_BLACK_TERM, PP_BLACK_MAKEUP);
    ppBuildModeTable(&mode);

    QByteArray baCanvas(static_cast<qint32>(static_cast<qint64>(info.nStride) * nHeight), 0);
    PPCanvas canvas;
    canvas.pData = reinterpret_cast<quint8 *>(baCanvas.data());
    canvas.nSize = baCanvas.size();
    canvas.nStride = info.nStride;

    qint64 nDirectory = static_cast<qint64>(nTileHeaderOffset) + PP_TILE_HEADER_SIZE;
    qint64 nData = nTileDataOffset;
    qint32 nIndex = 0;
    const uchar *pBase = reinterpret_cast<const uchar *>(packed.constData());

    for (qint32 r = 0; r < nDown; r++) {
        const qint32 nCurrentHeight = (r == nDown - 1) ? (nHeight - nTileHeight * (nDown - 1)) : nTileHeight;
        for (qint32 c = 0; c < nAcross; c++) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            qint32 nCurrentWidth = (c == nAcross - 1) ? (nWidth - nTileWidth * (nAcross - 1)) : nTileWidth;
            if ((nCurrentWidth < 1) || (nCurrentHeight < 1)) return false;

            if (nDirectory > packed.size() - PP_TILE_DIR_SIZE) return false;
            qint32 nCompression = static_cast<qint32>(qFromLittleEndian<quint16>(pBase + nDirectory));
            const qint32 nRecordSize = qFromLittleEndian<qint32>(pBase + nDirectory + 2);
            const qint32 nRawSize = qFromLittleEndian<qint32>(pBase + nDirectory + 6);
            nDirectory += PP_TILE_DIR_SIZE;

            if ((nRecordSize < 4) || (nData < 0) || (nData > packed.size() - nRecordSize)) return false;
            const qint32 nRecordIndex = static_cast<qint32>(qFromLittleEndian<quint16>(pBase + nData));
            const qint32 nRecordTag = static_cast<qint32>(qFromLittleEndian<quint16>(pBase + nData + 2));
            if (nRecordIndex != nIndex) return false;
            if ((nRecordTag != 0x43) && (nRecordTag != 0x44)) return false;
            if (nRecordTag == 0x44) {
                if (nRecordSize != nRawSize) return false;
                nCompression = 1;
                nCurrentWidth = nTileWidth;
            }

            const qint32 nPayloadSize = nRecordSize - 4;
            const qint64 nX = static_cast<qint64>(c) * nTileWidth;
            const qint64 nY = static_cast<qint64>(r) * nTileHeight;
            const qint64 nRowOffset = nY * info.nStride + (nX >> 3);
            if ((nRowOffset < 0) || (nRowOffset >= canvas.nSize)) return false;

            if (nCompression == 1) {
                if ((nX & 7) != 0) return false;
                const qint32 nBytes = nCurrentWidth >> 3;
                if ((nBytes < 1) || (static_cast<qint64>(nBytes) * nCurrentHeight > nPayloadSize)) return false;
                for (qint32 k = 0; k < nCurrentHeight; k++) {
                    const qint64 nTarget = nRowOffset + static_cast<qint64>(k) * info.nStride;
                    if ((nTarget < 0) || (nTarget > canvas.nSize - nBytes)) return false;
                    for (qint32 m = 0; m < nBytes; m++) {
                        canvas.pData[nTarget + m] = pBase[nData + 4 + static_cast<qint64>(k) * nBytes + m];
                    }
                }
            } else if (nCompression == 2) {
                // The reference extractor hands the codec a buffer four bytes
                // longer than the payload it reads, and counts those four
                // bytes as available input, so the trailing lookahead checks
                // still pass on the last row.
                QByteArray baPayload = packed.mid(static_cast<qint32>(nData) + 4, nPayloadSize);
                if (baPayload.size() != nPayloadSize) return false;
                baPayload.append(QByteArray(16, '\0'));
                const PPInput input(reinterpret_cast<const quint8 *>(baPayload.constData()), baPayload.size());
                if (!ppDecodeTile(input, nRecordSize, (nCurrentWidth + 7) & ~7, nCurrentHeight, &canvas, nRowOffset, white, black, mode, pPdStruct)) {
                    return false;
                }
            } else {
                // Compression 4 is a JPEG tile; not implemented.
                return false;
            }

            nData += nRecordSize;
            nIndex++;
        }
    }

    QByteArray baOutput;
    baOutput.reserve(static_cast<qint32>(nUncompressedSize));
    const quint32 nImageSize = static_cast<quint32>(static_cast<qint64>(info.nStride) * nHeight);
    QByteArray baHeader(PP_BMP_HEADER_SIZE, 0);
    uchar *pHeader = reinterpret_cast<uchar *>(baHeader.data());
    pHeader[0] = 'B';
    pHeader[1] = 'M';
    qToLittleEndian<quint32>(static_cast<quint32>(PP_BMP_HEADER_SIZE + nImageSize), pHeader + 2);
    qToLittleEndian<quint32>(static_cast<quint32>(PP_BMP_HEADER_SIZE), pHeader + 10);
    qToLittleEndian<quint32>(40U, pHeader + 14);
    qToLittleEndian<qint32>(nWidth, pHeader + 18);
    qToLittleEndian<qint32>(nHeight, pHeader + 22);
    qToLittleEndian<quint16>(1U, pHeader + 26);
    qToLittleEndian<quint16>(static_cast<quint16>(nBitsPerPixel), pHeader + 28);
    qToLittleEndian<quint32>(nImageSize, pHeader + 34);
    qToLittleEndian<quint32>(2U, pHeader + 46);
    qToLittleEndian<quint32>(2U, pHeader + 50);
    qToLittleEndian<quint32>(0x00FFFFFFU, pHeader + 54);
    qToLittleEndian<quint32>(0x00000000U, pHeader + 58);
    baOutput.append(baHeader);

    for (qint32 y = nHeight - 1; y >= 0; y--) {
        baOutput.append(baCanvas.constData() + static_cast<qint64>(y) * info.nStride, info.nStride);
    }

    if (baOutput.size() != nUncompressedSize) return false;
    *pOutput = baOutput;
    return true;
}
