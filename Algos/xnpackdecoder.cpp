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
#include "xnpackdecoder.h"

#include "algo_utils.h"

#include <new>

namespace {

const qint32 NPACK_OUTPUT_FLUSH_SIZE = 0x10000;
// A single match may not exceed this.  The largest seen in the corpus is 60466;
// the cap only stops a corrupt run of 0xf nibbles from spinning forever.
const qint64 NPACK_MAX_MATCH_LENGTH = 0x4000000;  // 64 MiB

// The match-length prefix code, expressed as the two branches the reader takes.
const quint32 NPACK_LEN_LONG_CODE = 6U;

/*--
   Bit reader over an in-memory payload, MSB first.  Running out of input is a
   sticky error rather than a source of zero bits: an LZS block is defined to
   end with its stop code, so a token that needs bits past EOF is by definition
   a broken stream.
--*/
class NPackMemBitReader {
public:
    NPackMemBitReader(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPos(0), m_nBitBuffer(0), m_nBitsLive(0), m_bEof(false)
    {
    }

    bool isEof() const
    {
        return m_bEof;
    }

    // Payload bytes consumed so far, i.e. the byte-aligned end of the block.
    qint64 consumed() const
    {
        return m_nPos;
    }

    quint32 getBits(qint32 nCount)
    {
        quint32 nResult = 0;

        for (qint32 i = 0; i < nCount; i++) {
            if (m_nBitsLive == 0) {
                if (m_nPos >= m_nSize) {
                    m_bEof = true;
                    return 0;
                }

                m_nBitBuffer = m_pData[m_nPos];
                m_nPos++;
                m_nBitsLive = 8;
            }

            m_nBitsLive--;
            nResult = (nResult << 1) | ((m_nBitBuffer >> m_nBitsLive) & 1U);
        }

        return nResult;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPos;
    quint32 m_nBitBuffer;
    qint32 m_nBitsLive;
    bool m_bEof;
};

// Shared by the probe and the real decoder: reads the match-length prefix code.
// Returns false when the length runs away (only reachable on corrupt input).
template <class BITREADER>
bool npackReadMatchLength(BITREADER *pReader, qint64 *pnLength)
{
    const quint32 nShort = pReader->getBits(2);
    quint32 nCode = nShort;

    if (nShort == 3U) {
        nCode = 3U + pReader->getBits(2);
    }

    if (nCode < NPACK_LEN_LONG_CODE) {
        *pnLength = (qint64)nCode + 2;
        return true;
    }

    qint64 nLength = 8;

    for (;;) {
        const quint32 nExtra = pReader->getBits(4);
        nLength += (qint64)nExtra;

        if (nLength > NPACK_MAX_MATCH_LENGTH) {
            return false;
        }

        if (nExtra < 15U) {
            break;
        }
    }

    *pnLength = nLength;

    return true;
}

/*--
   Bit reader over the DATAPROCESS_STATE input device, MSB first, chunked so a
   large container is never resident in full.  Same sticky-EOF rule as the
   in-memory reader above.
--*/
class NPackStreamBitReader {
public:
    NPackStreamBitReader(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct, qint32 nBufferSize)
        : m_pState(pState), m_pPdStruct(pPdStruct), m_nAvailable(0), m_nPosition(0), m_nBitBuffer(0), m_nBitsLive(0), m_bEof(false), m_bFailed(false)
    {
        m_baInput.resize(nBufferSize);
    }

    bool isEof() const
    {
        return m_bEof;
    }

    bool isFailed() const
    {
        return m_bFailed;
    }

    bool isAllocated() const
    {
        return !m_baInput.isEmpty();
    }

    quint32 getBits(qint32 nCount)
    {
        quint32 nResult = 0;

        for (qint32 i = 0; i < nCount; i++) {
            if (m_nBitsLive == 0) {
                quint8 nByte = 0;

                if (!nextByte(&nByte)) {
                    m_bEof = true;
                    return 0;
                }

                m_nBitBuffer = nByte;
                m_nBitsLive = 8;
            }

            m_nBitsLive--;
            nResult = (nResult << 1) | ((m_nBitBuffer >> m_nBitsLive) & 1U);
        }

        return nResult;
    }

private:
    bool nextByte(quint8 *pnByte)
    {
        if (m_nPosition >= m_nAvailable) {
            if (!XBinary::isPdStructNotCanceled(m_pPdStruct)) {
                m_bFailed = true;
                return false;
            }

            const qint32 nChunkSize = Algo_utils::getReadChunkSize(m_pState, m_baInput.size());

            if (nChunkSize <= 0) {
                return false;  // the declared input is exhausted
            }

            const qint32 nRead = XBinary::_readDevice(m_baInput.data(), nChunkSize, m_pState);

            if (nRead <= 0) {
                if (m_pState->bReadError || (nRead < 0)) {
                    m_bFailed = true;
                }
                return false;
            }

            m_nAvailable = nRead;
            m_nPosition = 0;
        }

        *pnByte = (quint8)m_baInput.at(m_nPosition);
        m_nPosition++;

        return true;
    }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    XBinary::PDSTRUCT *m_pPdStruct;
    QByteArray m_baInput;
    qint32 m_nAvailable;
    qint32 m_nPosition;
    quint32 m_nBitBuffer;
    qint32 m_nBitsLive;
    bool m_bEof;
    bool m_bFailed;
};

}  // namespace

bool XNPackDecoder::probeStream(const char *pPayload, qint64 nPayloadSize, qint64 nMaxOutput, PROBE_RESULT *pResult)
{
    if (!pPayload || (nPayloadSize <= 0) || (nMaxOutput <= 0) || !pResult) {
        return false;
    }

    NPackMemBitReader reader((const quint8 *)pPayload, nPayloadSize);
    qint64 nProduced = 0;

    PROBE_RESULT result = {};

    for (;;) {
        if (nProduced >= nMaxOutput) {
            result.bOutputCapped = true;
            break;
        }

        const quint32 nIsMatch = reader.getBits(1);
        if (reader.isEof()) return false;

        if (nIsMatch == 0) {
            reader.getBits(8);
            if (reader.isEof()) return false;
            nProduced++;
            continue;
        }

        quint32 nDistance = 0;

        if (reader.getBits(1) == 0) {
            nDistance = reader.getBits(11);
            if (reader.isEof()) return false;
            // The 11-bit form never encodes distance 0; the writer uses the
            // 7-bit form for the short distances, so this is a hard error.
            if (nDistance == 0) return false;
        } else {
            nDistance = reader.getBits(7);
            if (reader.isEof()) return false;
            if (nDistance == 0) {
                result.bStopCode = true;
                break;
            }
        }

        // No real NPack stream reads the zero pre-fill of the window; treating
        // that as an error is the probe's main discriminator.
        if ((qint64)nDistance > nProduced) return false;

        qint64 nLength = 0;
        if (!npackReadMatchLength(&reader, &nLength)) return false;
        if (reader.isEof()) return false;

        nProduced += nLength;
    }

    result.nConsumed = reader.consumed();
    result.nProduced = nProduced;

    *pResult = result;

    return true;
}

bool XNPackDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput || (pDecompressState->nInputOffset < 0) ||
        (pDecompressState->nInputLimit < -1) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint32 nRequestedBufferSize = XBinary::getBufferSize(pPdStruct);
    if (nRequestedBufferSize <= 0) return false;
    const qint32 nBufferSize = qBound((qint32)0x1000, nRequestedBufferSize, (qint32)0x100000);

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError) return false;

    NPackStreamBitReader reader(pDecompressState, pPdStruct, nBufferSize);
    if (!reader.isAllocated()) return false;

    QByteArray baWindow(NPACK_WINDOW_SIZE, char(0));
    if (baWindow.size() != NPACK_WINDOW_SIZE) return false;
    quint8 *pWindow = (quint8 *)baWindow.data();
    qint32 nWindowPos = 0;

    QByteArray baOutput;
    baOutput.reserve(NPACK_OUTPUT_FLUSH_SIZE + 1);

    qint64 nProduced = 0;
    bool bStopCode = false;
    bool bFailed = false;

    while (!bFailed) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            bFailed = true;
            break;
        }

        const quint32 nIsMatch = reader.getBits(1);
        if (reader.isEof()) {
            bFailed = true;
            break;
        }

        qint64 nLength = 0;
        qint32 nDistance = 0;

        if (nIsMatch == 0) {
            const quint32 nLiteral = reader.getBits(8);
            if (reader.isEof()) {
                bFailed = true;
                break;
            }

            const quint8 nByte = (quint8)nLiteral;
            pWindow[nWindowPos] = nByte;
            nWindowPos = (nWindowPos + 1) & (NPACK_WINDOW_SIZE - 1);
            baOutput.append((char)nByte);
            nProduced++;

            if (baOutput.size() >= NPACK_OUTPUT_FLUSH_SIZE) {
                if (XBinary::_writeDevice(baOutput.constData(), baOutput.size(), pDecompressState) != baOutput.size()) {
                    bFailed = true;
                    break;
                }
                baOutput.resize(0);
            }

            continue;
        }

        if (reader.getBits(1) == 0) {
            const quint32 nCode = reader.getBits(11);
            if (reader.isEof() || (nCode == 0)) {
                bFailed = true;
                break;
            }
            nDistance = (qint32)nCode;
        } else {
            const quint32 nCode = reader.getBits(7);
            if (reader.isEof()) {
                bFailed = true;
                break;
            }
            if (nCode == 0) {
                bStopCode = true;
                break;
            }
            nDistance = (qint32)nCode;
        }

        if ((qint64)nDistance > nProduced) {
            // Same rule as probeStream(): the window pre-fill is never read by
            // a genuine stream, so this can only be corruption.
            bFailed = true;
            break;
        }

        if (!npackReadMatchLength(&reader, &nLength) || reader.isEof()) {
            bFailed = true;
            break;
        }

        for (qint64 i = 0; i < nLength; i++) {
            const qint32 nSource = (nWindowPos - nDistance) & (NPACK_WINDOW_SIZE - 1);
            const quint8 nByte = pWindow[nSource];

            pWindow[nWindowPos] = nByte;
            nWindowPos = (nWindowPos + 1) & (NPACK_WINDOW_SIZE - 1);
            baOutput.append((char)nByte);

            if (baOutput.size() >= NPACK_OUTPUT_FLUSH_SIZE) {
                if (XBinary::_writeDevice(baOutput.constData(), baOutput.size(), pDecompressState) != baOutput.size()) {
                    bFailed = true;
                    break;
                }
                baOutput.resize(0);
            }
        }

        if (bFailed) break;

        nProduced += nLength;
    }

    if (bFailed || reader.isFailed()) return false;
    if (!bStopCode) return false;

    if (!baOutput.isEmpty()) {
        if (XBinary::_writeDevice(baOutput.constData(), baOutput.size(), pDecompressState) != baOutput.size()) {
            return false;
        }
        baOutput.resize(0);
    }

    // When the caller knows the unpacked size (it does not for a bare NPack
    // container, but a wrapper could), hold the decode to it.
    if (pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        bool bConverted = false;
        const qint64 nDeclared = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bConverted);
        if (!bConverted || (nDeclared < 0) || (nDeclared != nProduced)) return false;
    }

    return (pDecompressState->nCountOutput == nProduced) && !pDecompressState->bReadError && !pDecompressState->bWriteError &&
           XBinary::isPdStructNotCanceled(pPdStruct);
}
