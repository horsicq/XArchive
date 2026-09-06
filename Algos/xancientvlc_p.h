/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTVLC_P_H
#define XANCIENTVLC_P_H

#include <cstddef>
#include <cstdint>

#include <array>

// For exception
#include "xancientcodecbase_p.h"

namespace XAncientPrivate
{

template<size_t N>
class VariableLengthCodeDecoder
{
public:
	// negative lengths can be used to reset the offset.
	// N has to be given explicitly and to match the argument count; the
	// static_assert below is the C++11 stand-in for the deduction guide this
	// class used to carry.
	template<typename ...Args>
	VariableLengthCodeDecoder(Args ...args) noexcept :
		_bitLengths{{createBitLength(args)...}}
	{
		static_assert(sizeof...(Args)==N,"VariableLengthCodeDecoder needs exactly N bit lengths");

		// Probably this could be someway done as a nice constexpr initializer list, but I can't
		// see an easy way. Let it be for now
		uint32_t length{0};
		uint32_t i{0};

		// The C++11 stand-in for the comma fold expression this used to be: the
		// elements of a braced-init-list are evaluated left to right, so the
		// calls still run in argument order. The dummy array is optimized away.
		// (A plain index loop over a local values[N] array would be simpler, but
		// it makes MSVC 14.44's x64 optimizer ICE -- C1001 in p2/main.cpp -- when
		// the constructor is inlined into the RNC and DMS decoders.)
		const int expander[]={0,(foldOffset(int32_t(args),length,i),0)...};
		(void)expander;
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
	// A negative length resets the offset; the magnitude is the bit count.
	void foldOffset(int32_t value,uint32_t &length,uint32_t &i) noexcept
	{
		if (value<0)
		{
			_offsets[i]=0;
			length=1U<<uint32_t(-value);
			i++;
			return;
		}
		_offsets[i]=length;
		length+=1U<<uint32_t(value);
		i++;
	}

	// Both the signed and the unsigned case of the original: negative values
	// carry the reset flag in their sign, the bit length is the magnitude.
	template<typename T>
	static uint8_t createBitLength(T value) noexcept
	{
		const int32_t signedValue{int32_t(value)};
		return uint8_t(signedValue>=0?signedValue:-signedValue);
	}

	const std::array<uint8_t,N>	_bitLengths;
	std::array<uint32_t,N>		_offsets;
};

}

#endif
