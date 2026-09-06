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
#include "xbzip1decoder.h"

#include "algo_utils.h"

#include <QVector>

#include <new>

namespace {

/*--
   Arithmetic coder geometry.  b = 26 is the code register width; the coder is
   the binary-renormalising variant, so the only two thresholds that matter are
   2^(b-1) (the initial range, and the midpoint) and 2^(b-2) (the value R must
   stay above).  These are the values the format was written with - changing
   either one changes the bit stream, they are not tuning knobs.
--*/
const qint32 BZIP1_SMALL_B = 26;
const quint32 BZIP1_R_INITIAL = 1U << (BZIP1_SMALL_B - 1);
const quint32 BZIP1_R_FLOOR = 1U << (BZIP1_SMALL_B - 2);

const qint32 BZIP1_MAX_MODEL_SYMBOLS = 256;

// The coder's flush is always inside the file, so a real stream never needs a
// bit past EOF.  A truncated one does, and this cap stops it spinning; the
// trailing CRC turns the truncation into a clean failure either way.
const qint32 BZIP1_MAX_PAD_BITS = 64;

const qint32 BZIP1_SPOT_BASIS_STEP = 8000;

// Every stream's final block ends with this one extra byte, which marks
// end-of-input and is never written out.  A wrong value means the block was
// mis-decoded, so it is checked.
const quint8 BZIP1_RLE_SENTINEL = 42U;

// Values getMTFVal() can return outside the 1..255 move-to-front range.
const qint32 BZIP1_SYM_RUNA = 257;
const qint32 BZIP1_SYM_RUNB = 258;
const qint32 BZIP1_SYM_EOB = 259;

const qint32 BZIP1_MODEL_COUNT = 8;
const qint32 BZIP1_OUTPUT_FLUSH_SIZE = 0x10000;

/*--
   CRC-32 with the AUTODIN-II/Ethernet polynomial 0x04C11DB7 in its
   NON-reflected, MSB-first form: the same function bzip2 later kept.  Built at
   first use rather than pasted as a 256-entry literal.
--*/
struct Bzip1CrcTable {
    Bzip1CrcTable()
    {
        for (quint32 i = 0; i < 256; i++) {
            quint32 nValue = i << 24;
            for (qint32 j = 0; j < 8; j++) {
                nValue = (nValue & 0x80000000U) ? ((nValue << 1) ^ 0x04c11db7U) : (nValue << 1);
            }
            nTable[i] = nValue;
        }
    }
    quint32 nTable[256];
};

const Bzip1CrcTable &bzip1CrcTable()
{
    static const Bzip1CrcTable table;
    return table;
}

/*--
   One adaptive frequency table.  Counts live at freq[1 .. nNumSymbols]; index 0
   is unused so the cumulative-frequency scan can start from an empty prefix.
   nIncValue == 0 means "never adapt", which is how the fixed uniform byte model
   used for origPtr and the stream CRC is expressed.
--*/
struct Bzip1Model {
    quint32 nTotFreq;
    quint32 nNumSymbols;
    quint32 nIncValue;
    quint32 nNoExceed;
    quint32 freq[BZIP1_MAX_MODEL_SYMBOLS + 2];
};

void bzip1InitModel(Bzip1Model *pModel, quint32 nNumSymbols, quint32 nIncValue, quint32 nNoExceed)
{
    const quint32 nInitial = (nIncValue == 0) ? 1U : nIncValue;

    pModel->nNumSymbols = nNumSymbols;
    pModel->nIncValue = nIncValue;
    pModel->nNoExceed = nNoExceed;
    pModel->nTotFreq = nNumSymbols * nInitial;

    for (quint32 i = 0; i <= nNumSymbols + 1; i++) {
        pModel->freq[i] = nInitial;
    }
    pModel->freq[0] = 0;
    pModel->freq[nNumSymbols + 1] = 0;
}

void bzip1UpdateModel(Bzip1Model *pModel, quint32 nSymbol)
{
    pModel->nTotFreq += pModel->nIncValue;
    pModel->freq[nSymbol] += pModel->nIncValue;

    if (pModel->nTotFreq > pModel->nNoExceed) {
        pModel->nTotFreq = 0;
        for (quint32 i = 1; i <= pModel->nNumSymbols; i++) {
            // The +1 before the shift is what keeps every count at 1 or above,
            // so no symbol can ever be scaled out of the alphabet.
            pModel->freq[i] = (pModel->freq[i] + 1) >> 1;
            pModel->nTotFreq += pModel->freq[i];
        }
    }
}

class Bzip1Decoder {
public:
    Bzip1Decoder(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct, qint32 nBufferSize)
        : m_pState(pState),
          m_pPdStruct(pPdStruct),
          m_baInput(nBufferSize, char(0)),
          m_nInputAvailable(0),
          m_nInputPosition(0),
          m_nBitBuffer(0),
          m_nBitsLive(0),
          m_nPadBits(0),
          m_bFailed(false),
          m_nBigR(BZIP1_R_INITIAL),
          m_nBigD(0),
          m_nBlockLimit(0),
          m_nLast(-1),
          m_nOrigPtr(0),
          m_nCrc(0xffffffffU)
    {
        bzip1InitModel(&m_byteModel, 256, 0, 256);
        for (qint32 i = 0; i < BZIP1_MODEL_COUNT; i++) {
            bzip1InitModel(&m_models[i], 1, 1, 1);
        }
    }

    bool run();

private:
    bool readBit(quint32 *pnBit);
    bool nextInputByte(quint8 *pnByte);
    bool readRawByte(quint32 *pnByte);
    bool decodeSymbol(Bzip1Model *pModel, quint32 *pnSymbol);
    bool decodeByte(quint32 *pnByte);
    bool decodeUInt32(quint32 *pnValue);
    void initStructuredModels();
    bool decodeMTFValue(qint32 *pnValue);
    bool decodeBlockSymbols(bool *pbLastBlock);
    bool undoTransform();
    void applySpotTransform();
    bool unRleAndEmit(bool bLastBlock);
    bool emitByte(quint8 nByte);
    bool flushOutput();

    XBinary::DATAPROCESS_STATE *m_pState;
    XBinary::PDSTRUCT *m_pPdStruct;

    QByteArray m_baInput;
    qint32 m_nInputAvailable;
    qint32 m_nInputPosition;
    quint32 m_nBitBuffer;
    qint32 m_nBitsLive;
    qint32 m_nPadBits;
    bool m_bFailed;

    quint32 m_nBigR;
    quint32 m_nBigD;

    Bzip1Model m_byteModel;
    Bzip1Model m_models[BZIP1_MODEL_COUNT];

    qint32 m_nBlockLimit;
    qint32 m_nLast;
    qint32 m_nOrigPtr;

    QByteArray m_baMtfBlock;   // the BWT output, "ll" in the original description
    QByteArray m_baBlock;      // the block after the inverse transform
    QVector<qint32> m_vecNext;  // the inverse-BWT successor vector

    QByteArray m_baOutput;
    quint32 m_nCrc;
};

bool Bzip1Decoder::nextInputByte(quint8 *pnByte)
{
    if (m_nInputPosition >= m_nInputAvailable) {
        const qint32 nChunkSize = Algo_utils::getReadChunkSize(m_pState, m_baInput.size());
        qint32 nRead = 0;

        if (nChunkSize > 0) {
            nRead = XBinary::_readDevice(m_baInput.data(), nChunkSize, m_pState);
        }

        if (nRead <= 0) {
            if (m_pState->bReadError || (nRead < 0) || (m_nPadBits >= BZIP1_MAX_PAD_BITS)) {
                m_bFailed = true;
                return false;
            }
            // Past the end of a truncated stream: feed zeroes for a bounded
            // number of bits so the coder unwinds instead of spinning, and let
            // the sentinel/CRC checks reject the result.
            m_nPadBits += 8;
            *pnByte = 0;
            return true;
        }

        m_nInputAvailable = nRead;
        m_nInputPosition = 0;
    }

    *pnByte = quint8(m_baInput.at(m_nInputPosition));
    m_nInputPosition++;

    return true;
}

bool Bzip1Decoder::readBit(quint32 *pnBit)
{
    if (m_bFailed) return false;

    if (m_nBitsLive == 0) {
        quint8 nByte = 0;
        if (!nextInputByte(&nByte)) return false;
        m_nBitBuffer = nByte;
        m_nBitsLive = 8;
    }

    m_nBitsLive--;
    *pnBit = (m_nBitBuffer >> m_nBitsLive) & 1U;

    return true;
}

bool Bzip1Decoder::readRawByte(quint32 *pnByte)
{
    quint32 nValue = 0;

    for (qint32 i = 0; i < 8; i++) {
        quint32 nBit = 0;
        if (!readBit(&nBit)) return false;
        nValue = (nValue << 1) | nBit;
    }

    *pnByte = nValue;

    return true;
}

bool Bzip1Decoder::decodeSymbol(Bzip1Model *pModel, quint32 *pnSymbol)
{
    if (m_bFailed) return false;

    const quint32 nTotal = pModel->nTotFreq;
    if (nTotal == 0) {
        m_bFailed = true;
        return false;
    }

    const quint32 nScale = m_nBigR / nTotal;
    if (nScale == 0) {
        m_bFailed = true;
        return false;
    }

    const quint32 nTarget = qMin(nTotal - 1, m_nBigD / nScale);

    quint32 nSymbol = 0;
    quint32 nHigh = 0;
    while (nHigh <= nTarget) {
        nSymbol++;
        if (nSymbol > pModel->nNumSymbols) {
            m_bFailed = true;
            return false;
        }
        nHigh += pModel->freq[nSymbol];
    }
    const quint32 nLow = nHigh - pModel->freq[nSymbol];

    const quint32 nScaledLow = nScale * nLow;
    m_nBigD -= nScaledLow;

    // The top interval keeps the rounding remainder R - scale*low, which is the
    // whole reason this coder needs no division on the encode side either.
    if (nHigh < nTotal) {
        m_nBigR = nScale * (nHigh - nLow);
    } else {
        m_nBigR -= nScaledLow;
    }

    if (m_nBigR == 0) {
        m_bFailed = true;
        return false;
    }

    while (m_nBigR <= BZIP1_R_FLOOR) {
        quint32 nBit = 0;
        if (!readBit(&nBit)) return false;
        m_nBigR <<= 1;
        m_nBigD = (m_nBigD << 1) + nBit;
    }

    bzip1UpdateModel(pModel, nSymbol);
    *pnSymbol = nSymbol;

    return true;
}

bool Bzip1Decoder::decodeByte(quint32 *pnByte)
{
    quint32 nSymbol = 0;
    if (!decodeSymbol(&m_byteModel, &nSymbol)) return false;
    *pnByte = nSymbol - 1;
    return true;
}

bool Bzip1Decoder::decodeUInt32(quint32 *pnValue)
{
    quint32 nValue = 0;

    for (qint32 i = 0; i < 4; i++) {
        quint32 nByte = 0;
        if (!decodeByte(&nByte)) return false;
        nValue = (nValue << 8) | (nByte & 0xffU);
    }

    *pnValue = nValue;

    return true;
}

/*--
   Fenwick's structured model over the move-to-front alphabet: a first-level
   model of 11 symbols (RUNA, RUNB, the literal 1, six escapes naming a
   power-of-two bucket, and EOB) with one sub-model per bucket.  Rarer buckets
   get a smaller increment so they adapt more slowly.  Every one of these
   numbers is part of the bit stream definition.
--*/
void Bzip1Decoder::initStructuredModels()
{
    bzip1InitModel(&m_models[0], 11, 12, 1000);   // first level
    bzip1InitModel(&m_models[1], 2, 4, 1000);     // 2..3
    bzip1InitModel(&m_models[2], 4, 3, 1000);     // 4..7
    bzip1InitModel(&m_models[3], 8, 3, 1000);     // 8..15
    bzip1InitModel(&m_models[4], 16, 3, 1000);    // 16..31
    bzip1InitModel(&m_models[5], 32, 3, 1000);    // 32..63
    bzip1InitModel(&m_models[6], 64, 2, 1000);    // 64..127
    bzip1InitModel(&m_models[7], 128, 1, 1000);   // 128..255
}

bool Bzip1Decoder::decodeMTFValue(qint32 *pnValue)
{
    quint32 nSymbol = 0;
    if (!decodeSymbol(&m_models[0], &nSymbol)) return false;

    if (nSymbol == 1) {
        *pnValue = BZIP1_SYM_RUNA;
        return true;
    }
    if (nSymbol == 2) {
        *pnValue = BZIP1_SYM_RUNB;
        return true;
    }
    if (nSymbol == 3) {
        *pnValue = 1;
        return true;
    }
    if (nSymbol == 11) {
        *pnValue = BZIP1_SYM_EOB;
        return true;
    }

    // Symbols 4..10 are escapes into the 2-3, 4-7, ... 128-255 buckets.  Both
    // the sub-model index and the bucket's base exponent are the escape minus
    // three: escape 4 selects model 1 and base 2^1 = 2, escape 10 selects model
    // 7 and base 2^7 = 128.  The sub-model then codes 1..bucketSize, so the
    // value is base + offset - 1.
    const qint32 nModelIndex = qint32(nSymbol) - 3;
    if ((nModelIndex < 1) || (nModelIndex >= BZIP1_MODEL_COUNT)) {
        m_bFailed = true;
        return false;
    }

    quint32 nOffset = 0;
    if (!decodeSymbol(&m_models[nModelIndex], &nOffset)) return false;

    *pnValue = qint32((1U << (nSymbol - 3)) + nOffset - 1);

    return true;
}

bool Bzip1Decoder::decodeBlockSymbols(bool *pbLastBlock)
{
    // The signed origPtr comes first and it is sent through the fixed byte
    // model, BEFORE the structured models are (re)initialised: the sign bit is
    // the only end-of-stream marker the format has.
    quint32 nRawOrigPtr = 0;
    if (!decodeUInt32(&nRawOrigPtr)) return false;

    // Negated rather than sign-extended, so the magnitude is taken on the
    // unsigned value: negating 0x80000000 as a signed int would be undefined.
    *pbLastBlock = ((nRawOrigPtr & 0x80000000U) != 0);
    const quint32 nMagnitude = (*pbLastBlock) ? ((~nRawOrigPtr) + 1U) : nRawOrigPtr;
    if ((nMagnitude == 0) || (nMagnitude > quint32(m_nBlockLimit))) {
        m_bFailed = true;
        return false;
    }
    m_nOrigPtr = qint32(nMagnitude) - 1;

    initStructuredModels();

    quint8 mtfTable[256];
    for (qint32 i = 0; i < 256; i++) {
        mtfTable[i] = quint8(i);
    }

    // The buffer stays at its full block size for the life of the decoder;
    // nCount is the only cursor, so no resize() invalidates pBlock mid-block.
    qint32 nCount = 0;
    char *pBlock = m_baMtfBlock.data();

    qint32 nSymbol = 0;
    if (!decodeMTFValue(&nSymbol)) return false;

    while (nSymbol != BZIP1_SYM_EOB) {
        if (!XBinary::isPdStructNotCanceled(m_pPdStruct)) {
            m_bFailed = true;
            return false;
        }

        if ((nSymbol == BZIP1_SYM_RUNA) || (nSymbol == BZIP1_SYM_RUNB)) {
            // Bijective base-2 run length: each step doubles, adds the RUNA/RUNB
            // bit and then adds one, so no run length has two encodings.
            qint64 nRunLength = 0;
            do {
                nRunLength = (nRunLength << 1) + ((nSymbol == BZIP1_SYM_RUNA) ? 1 : 0) + 1;
                if (nRunLength > m_nBlockLimit) {
                    m_bFailed = true;
                    return false;
                }
                if (!decodeMTFValue(&nSymbol)) return false;
            } while ((nSymbol == BZIP1_SYM_RUNA) || (nSymbol == BZIP1_SYM_RUNB));

            if (nRunLength > (qint64)(m_nBlockLimit - nCount)) {
                m_bFailed = true;
                return false;
            }

            const quint8 nRunByte = mtfTable[0];
            for (qint64 i = 0; i < nRunLength; i++) {
                pBlock[nCount] = char(nRunByte);
                nCount++;
            }
            continue;
        }

        if ((nSymbol < 1) || (nSymbol > 255)) {
            m_bFailed = true;
            return false;
        }
        if (nCount >= m_nBlockLimit) {
            m_bFailed = true;
            return false;
        }

        const quint8 nByte = mtfTable[nSymbol];
        for (qint32 j = nSymbol; j > 0; j--) {
            mtfTable[j] = mtfTable[j - 1];
        }
        mtfTable[0] = nByte;

        pBlock[nCount] = char(nByte);
        nCount++;

        if (!decodeMTFValue(&nSymbol)) return false;
    }

    m_nLast = nCount - 1;

    // Every real block has at least the RLE end marker in it, and origPtr has to
    // address a byte that exists; both are cheap fail-closed checks on a stream
    // that is otherwise fully entropy-coded.
    if ((m_nLast < 0) || (m_nOrigPtr < 0) || (m_nOrigPtr > m_nLast)) {
        m_bFailed = true;
        return false;
    }

    return true;
}

bool Bzip1Decoder::undoTransform()
{
    qint32 counts[256];
    for (qint32 i = 0; i < 256; i++) {
        counts[i] = 0;
    }

    const qint32 nSize = m_nLast + 1;
    const uchar *pMtf = reinterpret_cast<const uchar *>(m_baMtfBlock.constData());
    qint32 *pNext = m_vecNext.data();

    for (qint32 i = 0; i < nSize; i++) {
        const uchar nByte = pMtf[i];
        pNext[i] = counts[nByte];
        counts[nByte]++;
    }

    qint32 nSum = 0;
    for (qint32 nChar = 0; nChar < 256; nChar++) {
        nSum += counts[nChar];
        counts[nChar] = nSum - counts[nChar];
    }

    char *pOut = m_baBlock.data();
    qint32 nIndex = m_nOrigPtr;
    for (qint32 j = m_nLast; j >= 0; j--) {
        if ((nIndex < 0) || (nIndex >= nSize)) {
            m_bFailed = true;
            return false;
        }
        const uchar nByte = pMtf[nIndex];
        pOut[j] = char(nByte);
        nIndex = pNext[nIndex] + counts[nByte];
    }

    return true;
}

/*--
   The "spot" transform: bzip 0.21 decrements one byte in every ~8000 at a
   fixed, data-independent set of positions (the compressor incremented them
   before sorting).  It carries no information and cannot be skipped - leaving
   it out corrupts roughly one byte per 8 KB, which is exactly the kind of
   damage a CRC catches but an eyeball does not.
--*/
void Bzip1Decoder::applySpotTransform()
{
    uchar *pBlock = reinterpret_cast<uchar *>(m_baBlock.data());
    qint32 nPos = BZIP1_SPOT_BASIS_STEP;
    qint32 nDelta = 1;

    while (nPos < m_nLast) {
        pBlock[nPos] = quint8((pBlock[nPos] + 255U) & 0xffU);

        qint32 nNewDelta = 1;
        switch (nDelta) {
            case 1: nNewDelta = 4; break;
            case 2: nNewDelta = 6; break;
            case 3: nNewDelta = 1; break;
            case 4: nNewDelta = 5; break;
            case 5: nNewDelta = 9; break;
            case 6: nNewDelta = 7; break;
            case 7: nNewDelta = 3; break;
            case 8: nNewDelta = 8; break;
            case 9: nNewDelta = 2; break;
            default: nNewDelta = 1; break;
        }
        nDelta = nNewDelta;

        nPos += BZIP1_SPOT_BASIS_STEP + 17 * (nNewDelta - 5);
    }
}

bool Bzip1Decoder::emitByte(quint8 nByte)
{
    m_baOutput.append(char(nByte));
    m_nCrc = (m_nCrc << 8) ^ bzip1CrcTable().nTable[((m_nCrc >> 24) ^ nByte) & 0xffU];

    if (m_baOutput.size() >= BZIP1_OUTPUT_FLUSH_SIZE) {
        return flushOutput();
    }

    return true;
}

bool Bzip1Decoder::flushOutput()
{
    if (m_baOutput.isEmpty()) return true;

    const qint32 nSize = m_baOutput.size();
    if (XBinary::_writeDevice(m_baOutput.constData(), nSize, m_pState) != nSize) {
        m_bFailed = true;
        return false;
    }
    m_baOutput.resize(0);

    return true;
}

/*--
   Inverse RLE1: any run of four identical bytes is followed by one extra byte
   holding 0..255 further repeats.  On the final block the last byte is the
   sentinel and is not part of the data.
--*/
bool Bzip1Decoder::unRleAndEmit(bool bLastBlock)
{
    const uchar *pBlock = reinterpret_cast<const uchar *>(m_baBlock.constData());
    const qint32 nLastToEmit = bLastBlock ? (m_nLast - 1) : m_nLast;

    qint32 nCount = 0;
    qint32 i = 0;
    qint32 nPrevious = 256;  // neither a byte value nor the end marker

    while (i <= nLastToEmit) {
        if (!XBinary::isPdStructNotCanceled(m_pPdStruct)) {
            m_bFailed = true;
            return false;
        }

        const qint32 nCurrent = pBlock[i];
        i++;

        if (!emitByte(quint8(nCurrent))) return false;

        if (nCurrent != nPrevious) {
            nCount = 1;
        } else {
            nCount++;
            if (nCount >= 4) {
                if (i > m_nLast) {
                    // The repeat count byte has to be inside the block.
                    m_bFailed = true;
                    return false;
                }
                const qint32 nRepeat = pBlock[i];
                for (qint32 j = 0; j < nRepeat; j++) {
                    if (!emitByte(quint8(nCurrent))) return false;
                }
                i++;
                nCount = 0;
            }
        }
        nPrevious = nCurrent;
    }

    if (bLastBlock && (pBlock[m_nLast] != BZIP1_RLE_SENTINEL)) {
        m_bFailed = true;
        return false;
    }

    return true;
}

bool Bzip1Decoder::run()
{
    // The four header bytes are written raw, MSB-first, before the coder starts.
    quint32 nMagic[4] = {0, 0, 0, 0};
    for (qint32 i = 0; i < 4; i++) {
        if (!readRawByte(&nMagic[i])) return false;
    }

    if ((nMagic[0] != quint32('B')) || (nMagic[1] != quint32('Z')) || (nMagic[2] != quint32('0')) || (nMagic[3] < quint32('1')) ||
        (nMagic[3] > quint32('9'))) {
        return false;
    }

    const qint32 nBlockSize100k = qint32(nMagic[3] - quint32('0'));
    m_nBlockLimit = 100000 * nBlockSize100k;

    m_baMtfBlock.resize(m_nBlockLimit);
    m_baBlock.resize(m_nBlockLimit);
    m_vecNext.resize(m_nBlockLimit);
    if ((m_baMtfBlock.size() != m_nBlockLimit) || (m_baBlock.size() != m_nBlockLimit) || (m_vecNext.size() != m_nBlockLimit)) {
        return false;
    }

    // The coder is started once for the whole stream, not once per block.
    m_nBigR = BZIP1_R_INITIAL;
    m_nBigD = 0;
    for (qint32 i = 0; i < BZIP1_SMALL_B; i++) {
        quint32 nBit = 0;
        if (!readBit(&nBit)) return false;
        m_nBigD = (m_nBigD << 1) + nBit;
    }

    m_nCrc = 0xffffffffU;

    bool bLastBlock = false;
    while (!bLastBlock) {
        if (!XBinary::isPdStructNotCanceled(m_pPdStruct)) return false;
        if (!decodeBlockSymbols(&bLastBlock)) return false;
        if (!undoTransform()) return false;
        applySpotTransform();
        if (!unRleAndEmit(bLastBlock)) return false;
    }

    quint32 nStoredCrc = 0;
    if (!decodeUInt32(&nStoredCrc)) return false;

    // Fail closed.  The whole-file CRC is the only thing that certifies an
    // arithmetic decode - there is no other redundancy in the format - so a
    // mismatch returns false and never reports success.  Output is streamed, so
    // by this point some bytes may already have reached the output device; the
    // false return is the contract that tells the caller to discard them, the
    // same contract XBZIP2Decoder and every other streaming decoder here uses.
    if (nStoredCrc != (~m_nCrc)) return false;

    if (!flushOutput()) return false;

    return !m_bFailed;
}

}  // namespace

bool XBZIP1Decoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pDecompressState && pDecompressState->pDeviceInput && pDecompressState->pDeviceOutput && (pDecompressState->nInputOffset >= 0) &&
        (pDecompressState->nInputLimit >= -1) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nRequestedBufferSize = XBinary::getBufferSize(pPdStruct);
        if (nRequestedBufferSize <= 0) return false;
        const qint32 nBufferSize = qBound((qint32)0x1000, nRequestedBufferSize, (qint32)0x100000);

        Algo_utils::prepareState(pDecompressState);
        if (pDecompressState->bReadError || pDecompressState->bWriteError) return false;

        Bzip1Decoder *pDecoder = new (std::nothrow) Bzip1Decoder(pDecompressState, pPdStruct, nBufferSize);
        if (!pDecoder) return false;

        const bool bDecoded = pDecoder->run();
        delete pDecoder;

        const bool bExpectedOutput =
            !pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE) ||
            ((pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong() >= 0) &&
             (pDecompressState->nCountOutput == pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong()));

        bResult = bDecoded && bExpectedOutput && !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
    }

    return bResult;
}
