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
#include "Algos/xzstddecoder.h"

#include <QPointer>

#include <cstring>
#include <limits>

namespace {

const qint64 TAR_ZSTD_PROBE_MAX_INPUT = Q_INT64_C(1024) * 1024;
const size_t TAR_ZSTD_PROBE_MAX_WINDOW = size_t(16) * 1024 * 1024;
const qint32 TAR_ZSTD_HEADER_SIZE = 512;

bool parseTarNumber(const char *pData, qint32 nSize, qint64 *pValue)
{
    if (!pData || !pValue || (nSize <= 0)) return false;

    const quint8 *pBytes = reinterpret_cast<const quint8 *>(pData);
    const quint64 nMaximum = static_cast<quint64>((std::numeric_limits<qint64>::max)());

    if (pBytes[0] & 0x80) {
        if (pBytes[0] & 0x40) return false;
        quint64 nValue = pBytes[0] & 0x7f;
        for (qint32 i = 1; i < nSize; i++) {
            if (nValue > ((nMaximum - pBytes[i]) / 256)) return false;
            nValue = (nValue * 256) + pBytes[i];
        }
        *pValue = static_cast<qint64>(nValue);
        return true;
    }

    qint32 nIndex = 0;
    while ((nIndex < nSize) && ((pBytes[nIndex] == 0) || (pBytes[nIndex] == ' '))) {
        nIndex++;
    }

    bool bHasDigit = false;
    quint64 nValue = 0;
    for (; nIndex < nSize; nIndex++) {
        const quint8 nByte = pBytes[nIndex];
        if ((nByte == 0) || (nByte == ' ')) {
            for (; nIndex < nSize; nIndex++) {
                if ((pBytes[nIndex] != 0) && (pBytes[nIndex] != ' ')) {
                    return false;
                }
            }
            break;
        }
        if ((nByte < '0') || (nByte > '7')) return false;
        const quint64 nDigit = nByte - '0';
        if (nValue > ((nMaximum - nDigit) / 8)) return false;
        nValue = (nValue * 8) + nDigit;
        bHasDigit = true;
    }

    if (!bHasDigit) return false;
    *pValue = static_cast<qint64>(nValue);
    return true;
}

bool isValidTarHeader(const QByteArray &baHeader)
{
    if (baHeader.size() != TAR_ZSTD_HEADER_SIZE || (baHeader.at(0) == 0) || (memcmp(baHeader.constData() + 257, "ustar", 5) != 0)) {
        return false;
    }

    qint64 nStoredChecksum = 0;
    qint64 nFileSize = 0;
    if (!parseTarNumber(baHeader.constData() + 148, 8, &nStoredChecksum) || !parseTarNumber(baHeader.constData() + 124, 12, &nFileSize)) {
        return false;
    }

    quint64 nUnsignedChecksum = 0;
    qint64 nSignedChecksum = 0;
    for (qint32 i = 0; i < baHeader.size(); i++) {
        const quint8 nUnsignedByte = ((i >= 148) && (i < 156)) ? static_cast<quint8>(' ') : static_cast<quint8>(baHeader.at(i));
        const qint8 nSignedByte = ((i >= 148) && (i < 156)) ? static_cast<qint8>(' ') : static_cast<qint8>(baHeader.at(i));
        nUnsignedChecksum += nUnsignedByte;
        nSignedChecksum += nSignedByte;
    }

    return (nFileSize >= 0) && ((static_cast<quint64>(nStoredChecksum) == nUnsignedChecksum) || ((nSignedChecksum >= 0) && (nStoredChecksum == nSignedChecksum)));
}

}  // namespace

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

bool XTAR_ZSTD::isValidPrefix(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice || guardedDevice->isSequential() || !guardedDevice->isOpen() || !guardedDevice->isReadable() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint64 nOriginalPosition = guardedDevice->pos();
    const qint64 nFileSize = guardedDevice->size();
    if (!guardedDevice || (nOriginalPosition < 0) || (nFileSize < 8) || !guardedDevice->seek(0)) {
        return false;
    }

    const qint64 nProbeSize = qMin(nFileSize, TAR_ZSTD_PROBE_MAX_INPUT);
    QByteArray baInput;
    baInput.reserve(static_cast<qint32>(nProbeSize));
    while (guardedDevice && (baInput.size() < 4) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const QByteArray baChunk = guardedDevice->read(4 - baInput.size());
        if (!guardedDevice || baChunk.isEmpty()) break;
        baInput.append(baChunk);
    }

    const bool bHasZstdMagic = (baInput.size() == 4) && (static_cast<quint8>(baInput.at(0)) == 0x28) && (static_cast<quint8>(baInput.at(1)) == 0xB5) &&
                               (static_cast<quint8>(baInput.at(2)) == 0x2F) && (static_cast<quint8>(baInput.at(3)) == 0xFD);
    while (bHasZstdMagic && guardedDevice && (baInput.size() < nProbeSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nRemaining = nProbeSize - baInput.size();
        const QByteArray baChunk = guardedDevice->read(qMin<qint64>(65536, nRemaining));
        if (!guardedDevice || baChunk.isEmpty()) break;
        baInput.append(baChunk);
    }

    bool bResult = false;
    if (bHasZstdMagic && guardedDevice && (baInput.size() >= 8) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        ZSTD_DStream *pStream = ZSTD_createDStream();
        if (pStream) {
            const size_t nWindowResult = ZSTD_DCtx_setMaxWindowSize(pStream, TAR_ZSTD_PROBE_MAX_WINDOW);
            const size_t nInitResult = ZSTD_initDStream(pStream);
            if (!ZSTD_isError(nWindowResult) && !ZSTD_isError(nInitResult)) {
                QByteArray baHeader(TAR_ZSTD_HEADER_SIZE, 0);
                ZSTD_inBuffer input = {baInput.constData(), static_cast<size_t>(baInput.size()), 0};
                ZSTD_outBuffer output = {baHeader.data(), static_cast<size_t>(baHeader.size()), 0};

                while ((output.pos < output.size) && (input.pos < input.size) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                    const size_t nInputBefore = input.pos;
                    const size_t nOutputBefore = output.pos;
                    const size_t nDecodeResult = ZSTD_decompressStream(pStream, &output, &input);
                    if (ZSTD_isError(nDecodeResult) || ((input.pos == nInputBefore) && (output.pos == nOutputBefore))) {
                        break;
                    }
                    // A Zstandard stream can concatenate frames.  Continue
                    // across a frame boundary so the first TAR header may be
                    // split between frames; the input/output caps still apply.
                }

                bResult = (output.pos == output.size) && isValidTarHeader(baHeader) && XBinary::isPdStructNotCanceled(pPdStruct);
            }
            ZSTD_freeDStream(pStream);
        }
    }

    if (!guardedDevice || !guardedDevice->seek(nOriginalPosition)) {
        return false;
    }
    return bResult;
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
        XTARCOMPRESSED::INTERNAL_INFO *pInfo = static_cast<XTARCOMPRESSED::INTERNAL_INFO *>(guardedThis->XTARCOMPRESSED::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XTARCOMPRESSED::INTERNAL_INFO &>(guardedThis->m_internalInfo) = *pInfo;
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
