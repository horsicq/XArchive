/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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

/* This code is based on:
   Ppmd7.h -- Ppmd7 (PPMdH) compression codec
   2023-04-02 : Igor Pavlov : Public domain
   PPMd var.H (2001): Dmitry Shkarin : Public domain
*/

#ifndef XPPMD7MODEL_H
#define XPPMD7MODEL_H

#include <QIODevice>
#include "xbinary.h"

#include "Ppmd.h"
#include "Ppmd7.h"

// Forward declaration for internal implementation
struct XPPMd7ModelPrivate;

// PPMd7 Model wrapper class for 7z PPMd support
class XPPMd7Model {
public:
    static const quint8 MIN_ORDER = PPMD7_MIN_ORDER;
    static const quint8 MAX_ORDER = PPMD7_MAX_ORDER;

    XPPMd7Model();
    ~XPPMd7Model();

    // Allocate memory for the model
    bool allocate(quint32 nMemorySize);

    // Initialize the model with parameters
    void init(quint8 nOrder);

    // Set input device for stream reading
    void setInputStream(QIODevice *pDevice);

    // Decode a single symbol
    // Returns: >= 0 for decoded byte, -1 for end of stream, -2 for error
    qint32 decodeSymbol();

    // Free allocated resources
    void free();

    // Check if memory was allocated
    bool wasAllocated() const;

private:
    XPPMd7ModelPrivate *m_pPrivate;
};

#endif  // XPPMD7MODEL_H
