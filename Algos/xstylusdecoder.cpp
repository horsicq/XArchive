/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xstylusdecoder.h"

#include "algo_utils.h"

#include <array>
#include <limits>

namespace {
const qint32 STYLUS_WINDOW_SIZE = 4096;
const qint32 STYLUS_WINDOW_MASK = STYLUS_WINDOW_SIZE - 1;
// The encoder's write cursor started at N - F, so every stored position has to
// be shifted by F to be read against a cursor that starts at 0.
const qint32 STYLUS_MATCH_BIAS = 18;
const qint32 STYLUS_MATCH_MIN_LENGTH = 3;
const quint8 STYLUS_XOR_KEY = 0xb5;
// Cancellation is polled on this many decoded items rather than on each one.
const qint32 STYLUS_CANCEL_INTERVAL = 0xffff;

// Byte source over the archive device.  A short read is a clean end of stream:
// the format has no terminator and stops when the member's bytes run out.
class StylusDeviceReader {
public:
    StylusDeviceReader(XBinary::DATAPROCESS_STATE *pState,
                       XBinary::PDSTRUCT *pPdStruct)
        : m_pState(pState), m_pPdStruct(pPdStruct), m_bError(false)
    {
    }

    qint32 get()
    {
        if (!m_pState || !XBinary::isPdStructNotCanceled(m_pPdStruct)) {
            m_bError = true;
            return -1;
        }
        if ((m_pState->nInputLimit != -1) &&
            (m_pState->nCountInput >= m_pState->nInputLimit)) {
            m_bError = (m_pState->nCountInput != m_pState->nInputLimit);
            return -1;
        }
        char cByte = 0;
        const qint32 nRead = XBinary::_readDevice(&cByte, 1, m_pState);
        if (nRead != 1) {
            if ((nRead < 0) || m_pState->bReadError) m_bError = true;
            return -1;
        }
        return static_cast<qint32>(
            static_cast<quint8>(static_cast<quint8>(cByte) ^ STYLUS_XOR_KEY));
    }

    bool hasError() const
    {
        return m_bError;
    }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    XBinary::PDSTRUCT *m_pPdStruct;
    bool m_bError;
};

// Byte source over a memory block, used by measure().
class StylusMemoryReader {
public:
    StylusMemoryReader(const quint8 *pData, qint64 nSize)
        : m_pData(pData), m_nSize(nSize), m_nPosition(0)
    {
    }

    qint32 get()
    {
        if (!m_pData || (m_nPosition >= m_nSize)) return -1;
        const quint8 nByte = m_pData[m_nPosition++];
        return static_cast<qint32>(
            static_cast<quint8>(nByte ^ STYLUS_XOR_KEY));
    }

    bool hasError() const
    {
        return false;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
};

class StylusDeviceWriter {
public:
    explicit StylusDeviceWriter(XBinary::DATAPROCESS_STATE *pState)
        : m_pState(pState), m_nProduced(0)
    {
    }

    bool write(const quint8 *pData, qint32 nSize)
    {
        if (!m_pState || (nSize < 0)) return false;
        if (nSize == 0) return true;
        // _writeDevice applies the caller's output window and the unpack
        // output limit itself and returns the full chunk size on success.
        if (XBinary::_writeDevice(reinterpret_cast<const char *>(pData), nSize,
                                  m_pState) != nSize) {
            return false;
        }
        m_nProduced += nSize;
        return true;
    }

    qint64 produced() const
    {
        return m_nProduced;
    }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    qint64 m_nProduced;
};

class StylusCountingWriter {
public:
    StylusCountingWriter() : m_nProduced(0)
    {
    }

    bool write(const quint8 *pData, qint32 nSize)
    {
        Q_UNUSED(pData)
        if (nSize < 0) return false;
        if (m_nProduced > ((std::numeric_limits<qint64>::max)() - nSize)) {
            return false;
        }
        m_nProduced += nSize;
        return true;
    }

    qint64 produced() const
    {
        return m_nProduced;
    }

private:
    qint64 m_nProduced;
};

// The ring doubles as the output buffer, exactly as the original does: it is
// handed over whenever the write cursor wraps, and its contents stay live for
// the back references that follow.
template <class READER, class WRITER>
bool stylusCore(READER &reader, WRITER &writer, XBinary::PDSTRUCT *pPdStruct)
{
    std::array<quint8, STYLUS_WINDOW_SIZE> abWindow;
    abWindow.fill(0);

    qint32 nWindowPosition = 0;
    quint32 nFlags = 0;
    qint32 nCancelCounter = 0;

    for (;;) {
        if (((++nCancelCounter & STYLUS_CANCEL_INTERVAL) == 0) &&
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        qint32 nByte = reader.get();
        if (nByte < 0) break;

        nFlags >>= 1;
        if ((nFlags & 0x100) == 0) {
            // The byte just read was the flag byte for the next eight items;
            // 0xFF00 marks the eight valid bit positions.
            nFlags = static_cast<quint32>(nByte) | 0xff00U;
            nByte = reader.get();
            if (nByte < 0) break;
        }

        if (nFlags & 1) {
            abWindow[nWindowPosition++] = static_cast<quint8>(nByte);
            if (nWindowPosition == STYLUS_WINDOW_SIZE) {
                if (!writer.write(abWindow.data(), STYLUS_WINDOW_SIZE)) {
                    return false;
                }
                nWindowPosition = 0;
            }
        } else {
            const qint32 nSecond = reader.get();
            if (nSecond < 0) break;
            qint32 nPosition =
                static_cast<qint32>(
                    ((static_cast<quint32>(nSecond) >> 4) << 8) |
                    static_cast<quint32>(nByte)) +
                STYLUS_MATCH_BIAS;
            const qint32 nLength =
                (nSecond & 0x0f) + STYLUS_MATCH_MIN_LENGTH;
            for (qint32 i = 0; i < nLength; i++) {
                const quint8 nCopied = abWindow[nPosition & STYLUS_WINDOW_MASK];
                nPosition = (nPosition & STYLUS_WINDOW_MASK) + 1;
                abWindow[nWindowPosition++] = nCopied;
                if (nWindowPosition == STYLUS_WINDOW_SIZE) {
                    if (!writer.write(abWindow.data(), STYLUS_WINDOW_SIZE)) {
                        return false;
                    }
                    nWindowPosition = 0;
                }
            }
        }
    }

    if (reader.hasError()) return false;
    if ((nWindowPosition > 0) &&
        !writer.write(abWindow.data(), nWindowPosition)) {
        return false;
    }
    return XBinary::isPdStructNotCanceled(pPdStruct);
}
}  // namespace

XStylusDecoder::XStylusDecoder(QObject *parent) : QObject(parent)
{
}

bool XStylusDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState,
                                XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput ||
        !pDecompressState->pDeviceOutput ||
        (pDecompressState->nInputOffset < 0) ||
        (pDecompressState->nInputLimit < -1)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    if ((pDecompressState->nProcessedOffset < 0) ||
        (pDecompressState->nProcessedLimit < -1)) {
        pDecompressState->bWriteError = true;
        return false;
    }

    StylusDeviceReader reader(pDecompressState, pPdStruct);
    StylusDeviceWriter writer(pDecompressState);
    return stylusCore(reader, writer, pPdStruct);
}

qint64 XStylusDecoder::measure(const quint8 *pData, qint64 nSize,
                               XBinary::PDSTRUCT *pPdStruct)
{
    if (!pData || (nSize < 0)) return -1;
    StylusMemoryReader reader(pData, nSize);
    StylusCountingWriter writer;
    if (!stylusCore(reader, writer, pPdStruct)) return -1;
    return writer.produced();
}
