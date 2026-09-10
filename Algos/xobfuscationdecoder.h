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
#ifndef XOBFUSCATIONDECODER_H
#define XOBFUSCATIONDECODER_H

#include "../xbinary.h"

// Whole-file byte obfuscations that wrap an otherwise ordinary archive.  These
// are not compression: the output is the same length as the input and every
// byte is an independent, involutive-or-invertible transform of its input
// byte, so the decoder is a single pass with no state.
//
// The point of having them at all is that the archive underneath is a perfectly
// ordinary ZIP or ARJ.  Recovering the plaintext hands the file to a reader
// that already exists rather than needing a new format.
//
// Each transform is recognised from KNOWN PLAINTEXT: the first bytes of the
// archive underneath are fixed ("PK" 03 04, or 60 EA), so the transform - and
// for the XOR family the key itself - falls out of the header alone.  That is
// also why the key must be recovered per file and never assumed: the reference
// corpus holds three different XOR keys, and a hard-coded majority key silently
// mis-decodes the rest.
class XObfuscationDecoder {
public:
    enum TRANSFORM {
        TRANSFORM_UNKNOWN = 0,
        TRANSFORM_XOR,       // b ^ key
        TRANSFORM_ROL3_XOR,  // rol(b, 3) ^ key
        TRANSFORM_SWAP_XOR   // (b >> 4 | b << 4) ^ key
    };

    struct METHOD {
        TRANSFORM transform;
        quint8 nKey;
    };

    // Look at the first bytes and work out which transform, if any, turns them
    // into the given plaintext.  Returns TRANSFORM_UNKNOWN when none does.
    static METHOD detect(const QByteArray &baHeader, const QByteArray &baPlainText);

    static QByteArray methodToProperty(const METHOD &method);
    static bool propertyToMethod(const QByteArray &baProperty, METHOD *pMethod);

    static bool decode(const QByteArray &baPacked, const METHOD &method, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static QString methodToString(const METHOD &method);
};

#endif  // XOBFUSCATIONDECODER_H
