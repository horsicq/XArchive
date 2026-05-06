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

#include "xppmd7model.h"
#include "algo_utils.h"
#include "xalgo_local.h"

#include <cstring>

typedef Algo_utils::QIODeviceByteInStream XPPMd7InputStream;

// Private implementation using 7-Zip Ppmd7 model
struct XPPMd7ModelPrivate {
    CPpmd7 sPpmd;
    XPPMd7InputStream sInputStream;
    bool bAllocated;

    XPPMd7ModelPrivate() : bAllocated(false)
    {
        memset(&sPpmd, 0, sizeof(sPpmd));
        memset(&sInputStream, 0, sizeof(sInputStream));
    }
};

// ============================================================================
// XPPMd7Model public methods
// ============================================================================

XPPMd7Model::XPPMd7Model()
{
    m_pPrivate = new XPPMd7ModelPrivate();
    X_Ppmd7_Construct(&m_pPrivate->sPpmd);
}

XPPMd7Model::~XPPMd7Model()
{
    free();
    delete m_pPrivate;
}

bool XPPMd7Model::allocate(quint32 nMemorySize)
{
    if (m_pPrivate->bAllocated) {
        free();
    }

    m_pPrivate->bAllocated = (X_Ppmd7_Alloc(&m_pPrivate->sPpmd, nMemorySize, Algo_utils::ppmdAlloc()) != 0);
    return m_pPrivate->bAllocated;
}

void XPPMd7Model::init(quint8 nOrder)
{
    // Initialize the Ppmd7 model with the specified order
    X_Ppmd7_Init(&m_pPrivate->sPpmd, nOrder);
}

void XPPMd7Model::setInputStream(QIODevice *pDevice)
{
    // Set up input stream for 7-Zip's internal range decoder
    m_pPrivate->sInputStream.vt.Read = Algo_utils::readFromQIODeviceStream;
    m_pPrivate->sInputStream.pDevice = pDevice;
    m_pPrivate->sInputStream.bError = false;

    // Connect the stream to the Ppmd7 decoder
    m_pPrivate->sPpmd.rc.dec.Stream = &m_pPrivate->sInputStream.vt;

    // Initialize 7-Zip's internal range decoder for 7z format
    if (!X_Ppmd7z_RangeDec_Init(&m_pPrivate->sPpmd.rc.dec)) {
        // Range decoder initialization failed
        m_pPrivate->sInputStream.bError = true;
    }
}

qint32 XPPMd7Model::decodeSymbol()
{
    if (!m_pPrivate->bAllocated) {
        return -2;  // Error: model not allocated
    }

    // Use 7-Zip's Ppmd7z decoder (for 7z format)
    int nSymbol = X_Ppmd7z_DecodeSymbol(&m_pPrivate->sPpmd);

    return nSymbol;
}

void XPPMd7Model::free()
{
    if (m_pPrivate->bAllocated) {
        X_Ppmd7_Free(&m_pPrivate->sPpmd, Algo_utils::ppmdAlloc());
        m_pPrivate->bAllocated = false;
    }
}

bool XPPMd7Model::wasAllocated() const
{
    return m_pPrivate->bAllocated;
}
