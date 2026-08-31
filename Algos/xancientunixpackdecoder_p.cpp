/* Copyright (C) Teemu Suutari */

#include <cstdint>
#include <cstring>
#include <array>

#include "xancientunixpackdecoder_p.h"
#include "xancienthuffman_p.h"
#include "xancientdynamichuffman_p.h"
#include "xancientinputstream_p.h"
#include "xancientoutputstream_p.h"
#include "xancientcommon_p.h"

namespace XAncientPrivate
{

namespace
{

void buildOldHuffmanDecoder(const std::array<uint16_t,1024> &tree,uint32_t count,HuffmanDecoder<uint8_t> &decoder,uint32_t node,uint32_t length,uint32_t bits)
{
	if (node>=count)
		throw CodecDecoder::DecompressionError();
	if (tree[node])
	{
		length++;
		bits<<=1U;
		if (length>24U)
			throw CodecDecoder::DecompressionError();
		buildOldHuffmanDecoder(tree,count,decoder,node+tree[node],length,bits);
		if (node+1>=count)
			throw CodecDecoder::DecompressionError();
		buildOldHuffmanDecoder(tree,count,decoder,node+tree[node+1],length,bits|1U);
	} else {
		if (!length)
			throw CodecDecoder::DecompressionError();
		decoder.insert(HuffmanCode{length,bits,uint8_t(tree[node+1])});
	}
}

class UnixPackLE16BitReader
{
public:
	explicit UnixPackLE16BitReader(MSBBitReader<ForwardInputStream> &bitReader) noexcept :
		_bitReader{bitReader}
	{
	}

	uint32_t operator()()
	{
		return _bitReader.readBitsLE16(1);
	}

private:
	MSBBitReader<ForwardInputStream> &_bitReader;
};

class UnixPackByteBitReader
{
public:
	explicit UnixPackByteBitReader(MSBBitReader<ForwardInputStream> &bitReader) noexcept :
		_bitReader{bitReader}
	{
	}

	uint32_t operator()()
	{
		return _bitReader.readBits8(1);
	}

private:
	MSBBitReader<ForwardInputStream> &_bitReader;
};

}

bool UnixPackDecoder::detectHeader(uint32_t hdr,uint32_t footer) noexcept
{
	return ((hdr>>16)==0x1f1eU||(hdr>>16)==0x1f1fU);
}

std::shared_ptr<CodecDecoder> UnixPackDecoder::create(const ByteBuffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_shared<UnixPackDecoder>(packedData,exactSizeKnown,verify);
}

UnixPackDecoder::UnixPackDecoder(const ByteBuffer &packedData,bool exactSizeKnown,bool verify) :
	_packedData{packedData}
{
	if (_packedData.size()<6U)
		throw InvalidFormatError();
	uint32_t hdr{_packedData.readBE16(0)};
	if (!detectHeader(hdr<<16U,0))
		throw InvalidFormatError();
	_isOldVersion=hdr==0x1f1fU;
	if (exactSizeKnown) _packedSize=packedData.size();
	if (_isOldVersion)
	{
		// PDP endian!!!
		_rawSize=(uint32_t(_packedData.readLE16(2U))<<16U)|_packedData.readLE16(4);
	} else {
		_rawSize=_packedData.readBE32(2U);
	}
	if (_rawSize>getMaxRawSize() || (_isOldVersion && !_rawSize))
		throw InvalidFormatError();
}

const std::string &UnixPackDecoder::getName() const noexcept
{
	static std::string names[2]{
		"z: Pack (Old)",
		"z: Pack"};
	return names[_isOldVersion?0:1U];
}

size_t UnixPackDecoder::getPackedSize() const noexcept
{
	// no way to know before decompressing
	return _packedSize;
}


size_t UnixPackDecoder::getRawSize() const noexcept
{
	return _rawSize;
}

void UnixPackDecoder::decompressImpl(ByteBuffer &rawData,bool verify)
{
	ForwardInputStream inputStream{_packedData,6,_packedSize?_packedSize:_packedData.size()};
	ForwardOutputStream outputStream{rawData,0,rawData.size()};
	MSBBitReader<ForwardInputStream> bitReader{inputStream};

	if (_isOldVersion)
	{
		HuffmanDecoder<uint8_t> decoder;
		{
			std::array<uint16_t,1024> tree;
			uint32_t count=inputStream.readLE16();
			if (count>=1024U)
				throw DecompressionError();
			for (uint32_t i=0;i<count;i++)
			{
				uint8_t tmp{inputStream.readByte()};
				if (tmp<255U) tree[i]=tmp;
					else tree[i]=inputStream.readLE16();
			}

			buildOldHuffmanDecoder(tree,count,decoder,0,0,0);
		}

		UnixPackLE16BitReader readBit{bitReader};

		while (outputStream.getOffset()!=_rawSize)
			outputStream.writeByte(decoder.decode(readBit));
	} else {
		HuffmanDecoder<uint16_t> decoder;
		// interesting ordering...
		{
			uint32_t maxLevel{inputStream.readByte()};
			if (!maxLevel || maxLevel>24U)
				throw DecompressionError();
			std::array<uint16_t,24> levelCounts;
			for (uint32_t i=0;i<maxLevel;i++)
				levelCounts[i]=inputStream.readByte();
			levelCounts[maxLevel-1U]+=2U;
			uint32_t code{0x100'0000U};
			for (uint32_t i=0;i<maxLevel;i++)
			{
				code-=levelCounts[i]<<(23U-i);
				for (uint32_t j=0;j<levelCounts[i];j++)
				{
					uint16_t symbol{(i==maxLevel-1&&j==levelCounts[i]-1U)?uint16_t(256U):uint16_t(inputStream.readByte())};
					decoder.insert(HuffmanCode{i+1U,code>>(23U-i),symbol});
					code+=1U<<(23U-i);
				}
				code-=levelCounts[i]<<(23U-i);
			}
		}

		UnixPackByteBitReader readBit{bitReader};

		while (outputStream.getOffset()!=_rawSize)
		{
			uint16_t code{decoder.decode(readBit)};
			if (code==0x100U)
			{
				if (outputStream.getOffset()!=_rawSize)
					throw DecompressionError();
				break;
			}
			outputStream.writeByte(uint8_t(code));
		}
	}

	// we do not verify the exact packed length here since official encoder
	// tends to add few bytes at the end
	_packedSize=inputStream.getOffset();
}

}
