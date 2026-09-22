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
#include "xtar_bzip2.h"
#include "xbzip2decoder.h"

#include <QBuffer>

namespace {
class BzipTarBuffer : public QBuffer {
public:
    BzipTarBuffer(QByteArray *data, qint64 limit) : QBuffer(data), m_limit(limit) {}
protected:
    qint64 writeData(const char *data, qint64 size) override {
        if (size < 0 || pos() < 0 || pos() > m_limit || size > m_limit - pos()) return -1;
        return QBuffer::writeData(data, size);
    }
private:
    qint64 m_limit;
};
}

XTAR_BZIP2::XTAR_BZIP2(QIODevice *pDevice) : XTARCOMPRESSED(pDevice)
{
    m_compressionType = COMPRESSION_BZIP2;
}

bool XTAR_BZIP2::isValid(PDSTRUCT *pPdStruct)
{
    return XTARCOMPRESSED::isValid(pPdStruct);
}

bool XTAR_BZIP2::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pDevice) {
        return false;
    }

    if (detectCompressionType(pDevice) != COMPRESSION_BZIP2) return false;
    XTAR_BZIP2 archive(pDevice);
    return archive.XTARCOMPRESSED::isValid(pPdStruct);
}

XBinary::FT XTAR_BZIP2::getFileType()
{
    return FT_TAR_BZIP2;
}

QString XTAR_BZIP2::getFileFormatExt()
{
    return "tar.bz2";
}

QString XTAR_BZIP2::getFileFormatExtsString()
{
    return "*.tar.bz2;*.tbz;*.tbz2;*.tb2;*.tz2";
}

QString XTAR_BZIP2::getMIMEString()
{
    return "application/x-bzip2";
}

QIODevice *XTAR_BZIP2::decompressData(PDSTRUCT *pPdStruct)
{
    QIODevice *source = getDevice();
    m_nTrailingOffset = m_nTrailingSize = 0;
    if (!source || !isPdStructNotCanceled(pPdStruct)) return nullptr;
    qint64 size = source->size();
    if (!source || size <= 0 || m_nMaterializedOutputLimit < 0) return nullptr;
    const qint64 limit = m_nMaterializedOutputLimit;
    QByteArray data;
    BzipTarBuffer output(&data, limit);
    if (!output.open(QIODevice::ReadWrite)) return nullptr;
    DATAPROCESS_STATE state = {};
    state.pDeviceInput = source;
    state.pDeviceOutput = &output;
    state.nInputOffset = 0;
    state.nInputLimit = size;
    state.nProcessedLimit = -1;
    state.mapUnpackProperties.insert(UNPACK_PROP_MAX_OUTPUT_SIZE, limit);
    const bool decoded = XBZIP2Decoder::decompressPrefix(&state, pPdStruct);
    if (!source || !decoded || state.bReadError || state.bWriteError || !isPdStructNotCanceled(pPdStruct) ||
        state.nCountInput <= 0 || state.nCountInput > size || state.nCountOutput <= 0 || state.nCountOutput > limit || state.nCountOutput != data.size()) return nullptr;
    // Only a complete, checksummed sequence reaches this point. The exact
    // trailing extent is descriptive metadata, never a decoder input range.
    m_nTrailingOffset = state.nCountInput;
    m_nTrailingSize = size - state.nCountInput;
    return createMemoryBuffer(data);
}

XBinary::ARCHIVERECORD XTAR_BZIP2::infoCurrent(UNPACK_STATE *state, PDSTRUCT *pd)
{
    ARCHIVERECORD result = XTARCOMPRESSED::infoCurrent(state, pd);
    if (result.mapProperties.isEmpty()) return ARCHIVERECORD();
    if (m_nTrailingSize > 0) {
        result.mapProperties.insert(FPART_PROP_INFO, tr("BZip2 transport; %1 trailing bytes outside the stream, at offset %2")
                                                        .arg(m_nTrailingSize).arg(m_nTrailingOffset));
    }
    return result;
}

QList<QString> XTAR_BZIP2::getSearchSignatures()
{
    return {"'BZh'"};
}

XBinary *XTAR_BZIP2::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XTAR_BZIP2(pDevice);
}

bool XTAR_BZIP2::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XTARCOMPRESSED::handleInternalInfo(pPdStruct);
        if (!bResult) return false;
        XTARCOMPRESSED::INTERNAL_INFO *pInfo = static_cast<XTARCOMPRESSED::INTERNAL_INFO *>(XTARCOMPRESSED::getInternalInfo(pPdStruct));
        if (!pInfo) return false;
        static_cast<XTARCOMPRESSED::INTERNAL_INFO &>(m_internalInfo) = *pInfo;
    }

    return bResult;
}

void *XTAR_BZIP2::getInternalInfo(PDSTRUCT *pPdStruct)
{
    const bool bHandled = handleInternalInfo(pPdStruct);
    if (!bHandled) return nullptr;

    return &m_internalInfo;
}

void XTAR_BZIP2::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XTARCOMPRESSED::setInternalInfo(static_cast<XTARCOMPRESSED::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XTARCOMPRESSED::setInternalInfo(nullptr);
    }
}
