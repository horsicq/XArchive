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
#ifndef XRAR1DECODER_H
#define XRAR1DECODER_H

#include <QtCore>
#include <QIODevice>

// RAR1 stream structure
struct RAR1_stream {
    char *next_in;
    qint64 avail_in;
    qint64 total_in;
    char *next_out;
    qint64 avail_out;
    qint64 total_out;
};

class XRar1Decoder
{
public:
    XRar1Decoder();
    ~XRar1Decoder();

    // Initialize the decoder
    int decodeInit(RAR1_stream *strm);

    // Decode data from the stream
    int decode(RAR1_stream *strm);

    // End the decoding process
    int decodeEnd(RAR1_stream *strm);
};

#endif // XRAR1DECODER_H
