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
#ifndef XPPMDRANGEDECODER_H
#define XPPMDRANGEDECODER_H

#include <QIODevice>
#include "xbinary.h"

// Range decoder for PPMd - wraps arithmetic coding
class XPPMdRangeDecoder {
public:
    XPPMdRangeDecoder();
    ~XPPMdRangeDecoder();

    // Initialize range decoder with input stream
    bool init(QIODevice *pDevice);

    // Get current bit from range
    quint32 getThreshold(quint32 nTotal);

    // Decode using range
    void decode(quint32 nStart, quint32 nSize, quint32 nTotal);

    // Normalize range
    void normalize();

    // Check if stream is finished correctly
    bool isFinishedOK() const;

    // Get range and code values (for debugging)
    quint32 getRange() const
    {
        return m_nRange;
    }
    quint32 getCode() const
    {
        return m_nCode;
    }

private:
    QIODevice *m_pDevice;
    quint32 m_nRange;
    quint32 m_nCode;
    bool m_bError;

    quint8 readByte();
};

#endif  // XPPMDRANGEDECODER_H
