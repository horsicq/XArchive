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
#ifndef XQUANTUMDECODER_H
#define XQUANTUMDECODER_H

#include <QList>

#include "xbinary.h"

// Quantum (CAB compression type 2) decoder.
//
// EXPERIMENTAL / THINLY VERIFIED. Implemented clean-room from Matthew Russotto's
// public reverse-engineering of Quantum (russotto.net/quantumcomp.html); no
// libmspack (qtmd.c/.h) or other decoder source was consulted. It is a faithful
// port of a Python prototype that was validated two ways: (1) it decodes the one
// genuine local Quantum vector - the 59-byte qtm.txt inside libmspack's
// mixed.cab (window order 18) - to the exact expected bytes, and a single-bit
// flip anywhere in that stream perturbs the output, proving it tracks the
// arithmetic coder rather than returning a constant; (2) a matching clean-room
// encoder round-trips varied inputs. Real-Quantum conformance therefore rests on
// ONE small vector: the selector model, all four literal banks, and one short
// match are exercised, but the length model, the rescale/reorder paths, larger
// windows and multi-block coder re-init are covered only by the self-consistent
// round-trip, NOT by real compressed data. Any divergence from real data is
// this decoder's bug.
class XQuantumDecoder {
public:
    // Decode ordered, independently framed CAB CFDATA payloads. The two lists
    // must have the same non-zero size, and every uncompressed size must be the
    // exact positive cbUncomp value for the corresponding payload. The 9 adaptive
    // models and the LZ history persist across entries; the arithmetic coder is
    // re-primed (16 fresh MSB-first bits) at each block. nWindowBits is the
    // Quantum window order (10..21) from CFFOLDER.typeCompress.
    static bool decompressCABDataBlocks(const QList<QByteArray> &listCompressedBlocks,
                                        const QList<qint32> &listUncompressedSizes,
                                        QByteArray *pbaUncompressed, qint32 nWindowBits,
                                        XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XQUANTUMDECODER_H
