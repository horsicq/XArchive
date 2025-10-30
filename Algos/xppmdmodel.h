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

/* This code is based on:
   Ppmd8.h -- Ppmd8 (PPMdI) compression codec
   2023-04-02 : Igor Pavlov : Public domain
   PPMd var.I (2002): Dmitry Shkarin : Public domain
   Carryless rangecoder (1999): Dmitry Subbotin : Public domain
*/

#ifndef XPPMDMODEL_H
#define XPPMDMODEL_H

#include <QIODevice>
#include "xbinary.h"

extern "C" {
#include "../3rdparty/ppmd/src/Ppmd.h"
#include "../3rdparty/ppmd/src/Ppmd8.h"
}

// Forward declaration for internal implementation
struct XPPMdModelPrivate;

// PPMd Model wrapper class
class XPPMdModel {
public:
    static const quint8 MIN_ORDER = PPMD8_MIN_ORDER;
    static const quint8 MAX_ORDER = PPMD8_MAX_ORDER;
    
    enum RESTORE_METHOD {
        RESTORE_METHOD_RESTART = PPMD8_RESTORE_METHOD_RESTART,
        RESTORE_METHOD_CUT_OFF = PPMD8_RESTORE_METHOD_CUT_OFF,
        RESTORE_METHOD_UNSUPPORTED = PPMD8_RESTORE_METHOD_UNSUPPPORTED
    };
    
    XPPMdModel();
    ~XPPMdModel();
    
    // Allocate memory for the model
    bool allocate(quint32 nMemorySize);
    
    // Initialize the model with parameters
    void init(quint8 nOrder, quint8 nRestoreMethod);
    
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
    XPPMdModelPrivate *m_pPrivate;
};

#endif  // XPPMDMODEL_H
