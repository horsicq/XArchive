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
#include "xtar_lzip.h"

XTAR_LZIP::XTAR_LZIP(QIODevice *pDevice) : XTARCOMPRESSED(pDevice)
{
    m_compressionType = COMPRESSION_LZIP;
}

XTAR_LZIP::~XTAR_LZIP()
{
}

bool XTAR_LZIP::isValid(PDSTRUCT *pPdStruct)
{
    return XTARCOMPRESSED::isValid(pPdStruct);
}

bool XTAR_LZIP::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pDevice) {
        return false;
    }

    if (detectCompressionType(pDevice) != COMPRESSION_LZIP) return false;
    XTAR_LZIP archive(pDevice);
    return archive.XTARCOMPRESSED::isValid(pPdStruct);
}

XBinary::FT XTAR_LZIP::getFileType()
{
    return FT_TAR_LZIP;
}

QString XTAR_LZIP::getFileFormatExt()
{
    return "tar.lz";
}

QString XTAR_LZIP::getFileFormatExtsString()
{
    return "*.tar.lz";
}

QString XTAR_LZIP::getMIMEString()
{
    return "application/x-lzip";
}

QIODevice *XTAR_LZIP::decompressData(PDSTRUCT *pPdStruct)
{
    return decompressByMethod(HANDLE_METHOD_LZIP, 0, -1, pPdStruct);
}

QList<QString> XTAR_LZIP::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("'LZIP'");

    return listResult;
}

XBinary *XTAR_LZIP::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XTAR_LZIP(pDevice);
}

bool XTAR_LZIP::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XTAR_LZIP> guardedThis(this);
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

void *XTAR_LZIP::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XTAR_LZIP> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XTAR_LZIP::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XTARCOMPRESSED::setInternalInfo(static_cast<XTARCOMPRESSED::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XTARCOMPRESSED::setInternalInfo(nullptr);
    }
}
