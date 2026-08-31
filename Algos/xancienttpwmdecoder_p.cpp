/* Copyright (C) Teemu Suutari */

#include "xancienttpwmdecoder_p.h"
#include "xancientinputstream_p.h"
#include "xancientoutputstream_p.h"
#include "xancientcommon_p.h"


namespace XAncientPrivate
{

bool TpwmDecoder::detectHeader(uint32_t hdr,uint32_t footer) noexcept
{
	return hdr==FourCC("TPWM");
}

std::shared_ptr<CodecDecoder> TpwmDecoder::create(const ByteBuffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_shared<TpwmDecoder>(packedData,verify);
}

TpwmDecoder::TpwmDecoder(const ByteBuffer &packedData,bool verify) :
	_packedData{packedData}
{
	uint32_t hdr{packedData.readBE32(0)};
	if (!detectHeader(hdr,0) || packedData.size()<12)
		throw InvalidFormatError();

	_rawSize=packedData.readBE32(4);
	if (!_rawSize || _rawSize>getMaxRawSize())
		throw InvalidFormatError();
}

const std::string &TpwmDecoder::getName() const noexcept
{
	static std::string name="TPWM: Turbo Packer";
	return name;
}

size_t TpwmDecoder::getPackedSize() const noexcept
{
	// No packed size in the stream :(
	// After decompression, we can tell how many bytes were actually used
	return _decompressedPackedSize;
}

size_t TpwmDecoder::getRawSize() const noexcept
{
	return _rawSize;
}

void TpwmDecoder::decompressImpl(ByteBuffer &rawData,bool verify)
{
	if (rawData.size()<_rawSize)
		throw DecompressionError();

	ForwardInputStream inputStream{_packedData,8U,_packedData.size()};
	MSBBitReader<ForwardInputStream> bitReader{inputStream};

	ForwardOutputStream outputStream{rawData,0,_rawSize};

	while (!outputStream.eof())
	{
		if (bitReader.readBits8(1U))
		{
			uint8_t byte1{inputStream.readByte()};
			uint8_t byte2{inputStream.readByte()};
			uint32_t distance{(uint32_t(byte1&0xf0U)<<4U)|byte2};
			if (!distance) distance=4096U;
			uint32_t count{uint32_t(byte1&0xfU)+3U};

			count=std::min(count,uint32_t(_rawSize-outputStream.getOffset()));
			outputStream.copy(distance,count,uint8_t(0));
		} else {
			outputStream.writeByte(inputStream.readByte());
		}
	}

	_decompressedPackedSize=inputStream.getOffset();
}

}
