/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTVLC_P_H
#define XANCIENTVLC_P_H

#include <cstddef>
#include <cstdint>

#include <array>
#include <type_traits>

// For exception
#include "xancientcodecbase_p.h"

namespace XAncientPrivate
{

template<size_t N>
class VariableLengthCodeDecoder
{
public:
	// negative lengths can be used to reset the offset
	template<typename ...Args>
	VariableLengthCodeDecoder(Args ...args) noexcept :
		_bitLengths{createBitLength(args)...}
	{
		// Probably this could be someway done as a nice constexpr initializer list, but I can't
		// see an easy way. Let it be for now
		uint32_t length{0};
		uint32_t i{0};

		(foldOffset(args,length,i),...);
	}
	~VariableLengthCodeDecoder() noexcept=default;

	template<typename F>
	uint32_t decode(F bitReader,uint32_t base) const
	{
		if (base>=N)
			throw CodecDecoder::DecompressionError();
		return _offsets[base]+bitReader(_bitLengths[base]);
	}

	template<typename F>
	uint32_t decodeCascade(F bitReader) const
	{
		for (uint32_t i=0;i<N;i++)
		{
			if (!_bitLengths[i])		// not valid in this context
				throw CodecDecoder::DecompressionError();
			uint32_t tmp{bitReader(_bitLengths[i])};
			if (i==N-1U || tmp!=(1U<<_bitLengths[i])-1U)
				return _offsets[i]-i+tmp;
		}
		throw CodecDecoder::DecompressionError();
	}

private:
	template<typename T>
	void foldOffset(T value,uint32_t &length,uint32_t &i) noexcept
	{
		if constexpr (std::is_signed_v<T>) if (value<0)
		{
			_offsets[i]=0;
			length=1U<<-value;
			i++;
			return;
		}
		_offsets[i]=length;
		length+=1U<<value;
		i++;
	}

	template<typename T>
	static constexpr uint8_t createBitLength(T value) noexcept
	{
		if constexpr (std::is_signed_v<T>) return uint8_t(value>=0?value:-value);
			return uint8_t(value);
	}

	const std::array<uint8_t,N>	_bitLengths;
	std::array<uint32_t,N>		_offsets;
};

template<typename ...Args>
VariableLengthCodeDecoder(Args...args)->VariableLengthCodeDecoder<sizeof...(args)>;

}

#endif
