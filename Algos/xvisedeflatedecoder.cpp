/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xvisedeflatedecoder.h"
#include "xdeflatedecoder.h"

#include <QBuffer>

namespace {
const qint64 MAX_VISE_OUTPUT = Q_INT64_C(512) * 1024 * 1024;

class XViseCountingDevice final : public QIODevice
{
public:
    explicit XViseCountingDevice(qint64 limit) : m_limit(limit) {}
    qint64 count() const { return m_count; }

protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *, qint64 size) override
    {
        if (size < 0 || m_count > m_limit - size) return -1;
        m_count += size;
        return size;
    }

private:
    qint64 m_limit = 0;
    qint64 m_count = 0;
};
}

bool XViseDeflateDecoder::decode(const QByteArray &packed,
                                 qint64 expectedSize,
                                 QByteArray *output, qint64 *rawSize,
                                 XBinary::PDSTRUCT *pPdStruct)
{
    if (packed.size() < 2 || (packed.size() & 1) || expectedSize < -1 ||
        expectedSize > MAX_VISE_OUTPUT ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    QByteArray input = packed;
    for (qint32 i = 0; i < input.size(); i += 2) {
        const char value = input.at(i);
        input[i] = input.at(i + 1);
        input[i + 1] = value;
    }

    // VISE writes an aligned stored block as 01 00 LEN NLEN DATA, followed
    // by its three-byte member trailer.  Removing the alignment byte yields
    // an ordinary RFC 1951 stored block.  Validate every redundant field
    // before using the direct path so this cannot turn a random object into a
    // convincing archive member.
    if (input.size() >= 9 && quint8(input.at(0)) == 0x01U &&
        quint8(input.at(1)) == 0x00U) {
        const quint16 storedSize =
            quint16(quint8(input.at(2))) |
            (quint16(quint8(input.at(3))) << 8);
        const quint16 invertedSize =
            quint16(quint8(input.at(4))) |
            (quint16(quint8(input.at(5))) << 8);
        if (quint16(storedSize ^ invertedSize) == 0xffffU &&
            input.size() == qint32(storedSize) + 9 &&
            quint8(input.at(input.size() - 2)) == 0 &&
            quint8(input.at(input.size() - 1)) == 0 &&
            (expectedSize < 0 || expectedSize == storedSize)) {
            if (output) *output = input.mid(6, storedSize);
            if (rawSize) *rawSize = storedSize;
            return XBinary::isPdStructNotCanceled(pPdStruct);
        }
    }

    QBuffer inputDevice(&input);
    QByteArray decoded;
    QBuffer outputDevice(&decoded);
    const qint64 limit = expectedSize >= 0 ? expectedSize : MAX_VISE_OUTPUT;
    XViseCountingDevice countingDevice(limit);
    QIODevice *sink = output ? static_cast<QIODevice *>(&outputDevice)
                             : static_cast<QIODevice *>(&countingDevice);
    if (!inputDevice.open(QIODevice::ReadOnly) ||
        !sink->open(QIODevice::WriteOnly))
        return false;

    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = &inputDevice;
    state.pDeviceOutput = sink;
    state.nInputOffset = 0;
    state.nInputLimit = input.size();
    state.nProcessedOffset = 0;
    state.nProcessedLimit = limit;
    state.mapUnpackProperties.insert(XBinary::UNPACK_PROP_MAX_OUTPUT_SIZE,
                                     limit);
    if (expectedSize >= 0)
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE,
                                   expectedSize);
    const bool ok = XDeflateDecoder::decompress(&state, pPdStruct);
    sink->close();
    inputDevice.close();

    const qint64 actualSize = output ? decoded.size() : countingDevice.count();
    // VISE rounds the byte-swapped storage to a two-byte boundary.  Depending
    // on the final Deflate code, the padding byte is either consumed as part
    // of the last storage pair or remains as one unused zero byte.
    const bool completeInput =
        state.nCountInput == input.size() ||
        (state.nCountInput == input.size() - 1 && input.endsWith('\0'));
    if (!ok || state.bReadError || state.bWriteError || !completeInput ||
        state.nCountOutput != actualSize ||
        (expectedSize >= 0 && actualSize != expectedSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;
    if (output) *output = decoded;
    if (rawSize) *rawSize = actualSize;
    return true;
}
