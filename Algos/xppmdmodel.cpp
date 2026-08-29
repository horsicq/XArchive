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

#include "xppmdmodel.h"
#include "algo_utils.h"
#include "xalgo_local.h"

#include <cstring>

typedef Algo_utils::QIODeviceByteInStream XPPMdInputStream;

// Private implementation using 7-Zip PPMd model
struct XPPMdModelPrivate {
    CPpmd8 sPpmd;
    XPPMdInputStream sInputStream;
    bool bAllocated;
    bool bGentee;

    XPPMdModelPrivate() : bAllocated(false), bGentee(false)
    {
        memset(&sPpmd, 0, sizeof(sPpmd));
        memset(&sInputStream, 0, sizeof(sInputStream));
    }
};

// ============================================================================
// XPPMdModel public methods
// ============================================================================

XPPMdModel::XPPMdModel()
{
    m_pPrivate = new XPPMdModelPrivate();
    X_Ppmd8_Construct(&m_pPrivate->sPpmd);
}

XPPMdModel::~XPPMdModel()
{
    free();
    delete m_pPrivate;
}

bool XPPMdModel::allocate(quint32 nMemorySize)
{
    if (m_pPrivate->bAllocated) {
        free();
    }

    m_pPrivate->bAllocated = (X_Ppmd8_Alloc(&m_pPrivate->sPpmd, nMemorySize, Algo_utils::ppmdAlloc()) != 0);
    return m_pPrivate->bAllocated;
}

void XPPMdModel::init(quint8 nOrder, quint8 nRestoreMethod)
{
    // Initialize the PPMd model with the specified parameters
    m_pPrivate->bGentee = false;
    X_Ppmd8_Init(&m_pPrivate->sPpmd, nOrder, nRestoreMethod);
}

void XPPMdModel::initGentee(quint8 nOrder)
{
    m_pPrivate->bGentee = true;
    X_Ppmd8g_Init(&m_pPrivate->sPpmd, nOrder, PPMD8_RESTORE_METHOD_RESTART);
}

bool XPPMdModel::lightweightResetGentee()
{
    if (!m_pPrivate->bAllocated || !m_pPrivate->bGentee || !m_pPrivate->sPpmd.MaxContext) return false;

    m_pPrivate->bGentee = true;
    X_Ppmd8g_LightweightReset(&m_pPrivate->sPpmd);
    return true;
}

void XPPMdModel::setInputStream(QIODevice *pDevice, qint64 nLimit)
{
    if (!pDevice) {
        m_pPrivate->sInputStream.pDevice = nullptr;
        m_pPrivate->sInputStream.bError = true;
        return;
    }

    // Set up input stream for 7-Zip's internal range decoder
    m_pPrivate->sInputStream.vt.Read = Algo_utils::readFromQIODeviceStream;
    m_pPrivate->sInputStream.pDevice = pDevice;
    m_pPrivate->sInputStream.bError = false;
    m_pPrivate->sInputStream.nBytesRead = 0;
    m_pPrivate->sInputStream.nLimit = nLimit;

    // Connect the stream to the PPMd decoder
    m_pPrivate->sPpmd.Stream.In = &m_pPrivate->sInputStream.vt;

    // Initialize 7-Zip's internal range decoder
    if (!X_Ppmd8_Init_RangeDec(&m_pPrivate->sPpmd)) {
        // Range decoder initialization failed
        m_pPrivate->sInputStream.bError = true;
    }
}

qint32 XPPMdModel::decodeSymbol()
{
    if (!m_pPrivate->bAllocated || m_pPrivate->sInputStream.bError) {
        return -2;  // Error: model not allocated
    }

    // Use 7-Zip's proven decoder
    int nSymbol = m_pPrivate->bGentee ? X_Ppmd8g_DecodeSymbol(&m_pPrivate->sPpmd) : X_Ppmd8_DecodeSymbol(&m_pPrivate->sPpmd);

    return m_pPrivate->sInputStream.bError ? -2 : nSymbol;
}

void XPPMdModel::free()
{
    if (m_pPrivate->bAllocated) {
        X_Ppmd8_Free(&m_pPrivate->sPpmd, Algo_utils::ppmdAlloc());
        m_pPrivate->bAllocated = false;
        m_pPrivate->bGentee = false;
    }
}

bool XPPMdModel::wasAllocated() const
{
    return m_pPrivate->bAllocated;
}

bool XPPMdModel::hasInputError() const
{
    return m_pPrivate->sInputStream.bError;
}

qint64 XPPMdModel::inputBytesRead() const
{
    return m_pPrivate->sInputStream.nBytesRead;
}
