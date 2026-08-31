/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * Copyright (C) Teemu Suutari
 *
 * See xancientdecoder.LICENSE for the upstream codec license.
 */
#include "xancientcodecbase_p.h"

#include "xancientxpk_p.h"

namespace XAncientPrivate
{

void CodecDecoder::decompress(ByteBuffer &rawData, bool verify)
{
    try {
        decompressImpl(rawData, verify);
    } catch (const ByteBuffer::Error &) {
        throw DecompressionError();
    }
}

size_t CodecDecoder::getImageSize() const noexcept
{
    return 0;
}

size_t CodecDecoder::getImageOffset() const noexcept
{
    return 0;
}

size_t CodecDecoder::getMaxPackedSize() noexcept
{
    return 0x8000000U;
}

size_t CodecDecoder::getMaxRawSize() noexcept
{
    return 0x8000000U;
}

XpkDecoder::XpkDecoder(uint32_t recursionLevel)
    : _recursionLevel(recursionLevel)
{
}

}  // namespace XAncientPrivate
