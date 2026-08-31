/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include <limits>

#include "xancientdecoder.h"

#include "xancientdmsdecoder_p.h"
#include "xancientfreezedecoder_p.h"
#include "xancientpowerpackerdecoder_p.h"
#include "xancientunixpackdecoder_p.h"
#include "xancientrncdecoder_p.h"
#include "xancienttpwmdecoder_p.h"
#include "xancientstaticbuffer_p.h"
#include "xancientvectorbuffer_p.h"

#include <cstring>
#include <memory>
#include <new>
#include <vector>

namespace {
using CodecBase = XAncientPrivate::CodecDecoder;
using CodecBuffer = XAncientPrivate::ByteBuffer;

bool fourCC(const QByteArray &data, const char *value)
{
    return data.size() >= 4 && !std::memcmp(data.constData(), value, 4);
}

std::shared_ptr<CodecBase> createDecoder(const CodecBuffer &packed,
                                         XAncientDecoder::TYPE type,
                                         bool verify)
{
    using namespace XAncientPrivate;
    switch (type) {
        case XAncientDecoder::TYPE_DMS:
            return std::make_shared<DmsDecoder>(packed, verify);
        case XAncientDecoder::TYPE_POWERPACKER:
            return std::make_shared<PowerPackerDecoder>(packed, true, verify);
        case XAncientDecoder::TYPE_RNC:
            return std::make_shared<RncDecoder>(packed, verify);
        case XAncientDecoder::TYPE_TPWM:
            return std::make_shared<TpwmDecoder>(packed, verify);
        case XAncientDecoder::TYPE_FREEZE:
            return std::make_shared<FreezeDecoder>(packed, true, verify);
        case XAncientDecoder::TYPE_UNIX_PACK:
            return std::make_shared<UnixPackDecoder>(packed, true, verify);
        default: throw XAncientPrivate::InvalidFormatError();
    }
}

void copyInfo(const std::shared_ptr<CodecBase> &decoder,
              XAncientDecoder::INFO *info)
{
    if (!info) return;
    info->method = QString::fromStdString(decoder->getName());
    info->packedSize = decoder->getPackedSize()
                           ? qint64(decoder->getPackedSize())
                           : -1;
    info->rawSize = decoder->getRawSize() ? qint64(decoder->getRawSize()) : -1;
    const qint64 imageSize = qint64(decoder->getImageSize());
    const qint64 imageOffset = qint64(decoder->getImageOffset());
    info->imageSize = (imageSize || imageOffset) ? imageSize : info->rawSize;
    info->imageOffset = imageOffset;
}
}  // namespace

XAncientDecoder::TYPE XAncientDecoder::identify(const QByteArray &data)
{
    if (fourCC(data, "DMS!")) return TYPE_DMS;
    if (fourCC(data, "RNC\001") || fourCC(data, "RNC\002") ||
        fourCC(data, "...\001"))
        return TYPE_RNC;
    if (fourCC(data, "TPWM")) return TYPE_TPWM;
    static const char *const powerPackerIds[] = {
        "PP11", "PP20", "PX20", "CHFC", "DEN!", "DXS9", "H.D.", "RVV!"};
    for (const char *id : powerPackerIds)
        if (fourCC(data, id)) return TYPE_POWERPACKER;
    if (data.size() >= 2) {
        const quint16 signature = (quint16(quint8(data.at(0))) << 8) |
                                  quint16(quint8(data.at(1)));
        if (signature == 0x1f9e || signature == 0x1f9f) return TYPE_FREEZE;
        if (signature == 0x1f1e || signature == 0x1f1f)
            return TYPE_UNIX_PACK;
    }
    return TYPE_UNKNOWN;
}

bool XAncientDecoder::describe(const QByteArray &data, TYPE type, INFO *info)
{
    if (!info || data.size() < 2 || data.size() > MAX_PACKED_SIZE ||
        identify(data) != type)
        return false;
    try {
        XAncientPrivate::ReadOnlyBuffer packed(
            reinterpret_cast<const uint8_t *>(data.constData()),
            size_t(data.size()));
        const std::shared_ptr<CodecBase> decoder = createDecoder(packed, type,
                                                                 true);
        copyInfo(decoder, info);
        return !info->method.isEmpty() && info->packedSize <= data.size() &&
               info->rawSize <= MAX_RAW_SIZE && info->imageSize <= MAX_RAW_SIZE;
    } catch (const XAncientPrivate::CodecError &) {
        return false;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

bool XAncientDecoder::decode(const QByteArray &data, TYPE type, QByteArray *raw,
                             INFO *info, DECODE_ERROR *error, bool verify)
{
    if (error) *error = ERROR_NONE;
    if (!raw || data.size() < 2 || data.size() > MAX_PACKED_SIZE ||
        identify(data) != type) {
        if (error) *error = ERROR_INVALID_FORMAT;
        return false;
    }
    raw->clear();
    try {
        XAncientPrivate::ReadOnlyBuffer packed(
            reinterpret_cast<const uint8_t *>(data.constData()),
            size_t(data.size()));
        const std::shared_ptr<CodecBase> decoder = createDecoder(packed, type,
                                                                 verify);
        const size_t reportedSize = decoder->getRawSize();
        if (reportedSize > size_t(MAX_RAW_SIZE)) return false;
        std::vector<uint8_t> output(reportedSize ? reportedSize : 0);
        {
            XAncientPrivate::VectorBuffer outputBuffer(output);
            decoder->decompress(outputBuffer, verify);
        }
        if (output.size() > size_t(MAX_RAW_SIZE) ||
            output.size() > size_t((std::numeric_limits<int>::max)()))
            return false;
        raw->resize(int(output.size()));
        if (!output.empty())
            std::memcpy(raw->data(), output.data(), output.size());
        copyInfo(decoder, info);
        return true;
    } catch (const XAncientPrivate::VerificationError &) {
        if (error) *error = ERROR_VERIFICATION;
    } catch (const XAncientPrivate::InvalidFormatError &) {
        if (error) *error = ERROR_INVALID_FORMAT;
    } catch (const XAncientPrivate::CodecError &) {
        if (error) *error = ERROR_DECOMPRESSION;
    } catch (const std::bad_alloc &) {
        if (error) *error = ERROR_MEMORY;
    }
    raw->clear();
    return false;
}
