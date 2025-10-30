/* Copyright (c) 2025 hors<horsicq@gmail.com>
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
#include "xppmdrangedecoder.h"

XPPMdRangeDecoder::XPPMdRangeDecoder()
{
    m_pDevice = nullptr;
    m_nRange = 0;
    m_nCode = 0;
    m_bError = false;
}

XPPMdRangeDecoder::~XPPMdRangeDecoder()
{
}

bool XPPMdRangeDecoder::init(QIODevice *pDevice)
{
    m_pDevice = pDevice;
    m_bError = false;
    
    if (!m_pDevice) {
        m_bError = true;
        return false;
    }
    
    // Initialize range decoder
    m_nRange = 0xFFFFFFFF;
    m_nCode = 0;
    
    // Read initial 4 bytes for Code value (matching 7-Zip's Ppmd8_Init_RangeDec)
    for (qint32 i = 0; i < 4; i++) {
        m_nCode = (m_nCode << 8) | readByte();
    }
    
    if (m_bError) {
        return false;
    }
    
    return true;
}

quint32 XPPMdRangeDecoder::getThreshold(quint32 nTotal)
{
    // Calculate threshold = (Code - Low) * Total / Range
    // In 7-Zip's implementation: (Code / (Range / Total))
    return m_nCode / (m_nRange / nTotal);
}

void XPPMdRangeDecoder::decode(quint32 nStart, quint32 nSize, quint32 nTotal)
{
    // Update range decoder state after decoding a symbol
    // Range = Range / Total
    quint32 nNewRange = m_nRange / nTotal;
    
    // Low = Low + Start * NewRange
    // Code = Code - Start * NewRange
    m_nCode -= nStart * nNewRange;
    
    // Range = Size * NewRange
    m_nRange = nSize * nNewRange;
    
    // Normalize if needed
    normalize();
}

void XPPMdRangeDecoder::normalize()
{
    // Normalize range when it becomes too small
    // This matches 7-Zip's range decoder normalization
    while (m_nRange < 0x01000000) {
        m_nRange <<= 8;
        m_nCode = (m_nCode << 8) | readByte();
    }
}

bool XPPMdRangeDecoder::isFinishedOK() const
{
    // Stream is finished correctly if Code equals 0
    return (m_nCode == 0) && !m_bError;
}

quint8 XPPMdRangeDecoder::readByte()
{
    if (m_bError || !m_pDevice) {
        m_bError = true;
        return 0;
    }
    
    char cByte = 0;
    qint64 nRead = m_pDevice->read(&cByte, 1);
    
    if (nRead != 1) {
        m_bError = true;
        return 0;
    }
    
    return (quint8)cByte;
}
