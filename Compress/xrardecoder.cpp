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
#include "xrardecoder.h"

XRar1Decoder::XRar1Decoder()
{

}

XRar1Decoder::~XRar1Decoder()
{

}

int XRar1Decoder::decodeInit(RAR1_stream *strm)
{
    // Initialize the RAR1 stream structure
    strm->next_in = nullptr;
    strm->avail_in = 0;
    strm->total_in = 0;
    strm->next_out = nullptr;
    strm->avail_out = 0;
    strm->total_out = 0;

    return 0; // Return success
}

int XRar1Decoder::decode(RAR1_stream *strm)
{
    // Placeholder for decoding logic
    // This should contain the actual decoding algorithm for RAR1 format

    // For now, just simulate a successful decode operation
    strm->total_out += strm->avail_in; // Simulate output size
    strm->avail_out = 0; // Reset output availability

    return 0; // Indicate end of stream
}

int XRar1Decoder::decodeEnd(RAR1_stream *strm)
{
    // Clean up the RAR1 stream structure if necessary
    strm->next_in = nullptr;
    strm->avail_in = 0;
    strm->total_in = 0;
    strm->next_out = nullptr;
    strm->avail_out = 0;
    strm->total_out = 0;

    return 0; // Return success
}


