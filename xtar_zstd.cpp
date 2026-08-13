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
#include "xtar_zstd.h"

XTAR_ZSTD::XTAR_ZSTD(QIODevice *pDevice) : XTARCOMPRESSED(pDevice)
{
    m_compressionType = COMPRESSION_ZSTD;
}

XTAR_ZSTD::~XTAR_ZSTD()
{
}

bool XTAR_ZSTD::isValid(PDSTRUCT *pPdStruct)
{
    return XTARCOMPRESSED::isValid(pPdStruct);
}

bool XTAR_ZSTD::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pDevice) {
        return false;
    }

    if (detectCompressionType(pDevice) != COMPRESSION_ZSTD) return false;
    XTAR_ZSTD archive(pDevice);
    return archive.XTARCOMPRESSED::isValid(pPdStruct);
}

XBinary::FT XTAR_ZSTD::getFileType()
{
    return FT_TAR_ZSTD;
}

QString XTAR_ZSTD::getFileFormatExt()
{
    return "tar.zst";
}

QString XTAR_ZSTD::getFileFormatExtsString()
{
    return "*.tar.zst;*.tzst";
}

QString XTAR_ZSTD::getMIMEString()
{
    return "application/zstd";
}

QIODevice *XTAR_ZSTD::decompressData(PDSTRUCT *pPdStruct)
{
    return decompressByMethod(HANDLE_METHOD_ZSTD, 0, -1, pPdStruct);
}

QList<QString> XTAR_ZSTD::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("28B52FFD");

    return listResult;
}

XBinary *XTAR_ZSTD::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XTAR_ZSTD(pDevice);
}

bool XTAR_ZSTD::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XTAR_ZSTD> guardedThis(this);
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XTARCOMPRESSED::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XTARCOMPRESSED::INTERNAL_INFO *pInfo =
            static_cast<XTARCOMPRESSED::INTERNAL_INFO *>(
                guardedThis->XTARCOMPRESSED::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XTARCOMPRESSED::INTERNAL_INFO &>(
            guardedThis->m_internalInfo) = *pInfo;
    }

    return guardedThis && bResult;
}

void *XTAR_ZSTD::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XTAR_ZSTD> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XTAR_ZSTD::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XTARCOMPRESSED::setInternalInfo(static_cast<XTARCOMPRESSED::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XTARCOMPRESSED::setInternalInfo(nullptr);
    }
}
