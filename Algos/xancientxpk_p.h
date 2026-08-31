/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTXPK_P_H
#define XANCIENTXPK_P_H

#include <cstddef>
#include <cstdint>

#include <string>

#include "xancientcodecbase_p.h"

namespace XAncientPrivate
{

class XpkDecoder
{
public:
	class State
	{
	public:
		State(const State&)=delete;
		State& operator=(const State&)=delete;

		State() noexcept=default;
		virtual ~State() noexcept=default;
	};

	XpkDecoder(const XpkDecoder&)=delete;
	XpkDecoder& operator=(const XpkDecoder&)=delete;

	XpkDecoder(uint32_t recursionLevel=0);
	virtual ~XpkDecoder() noexcept=default;

	virtual const std::string &getSubName() const noexcept=0;

	// Actual decompression
	virtual void decompressImpl(ByteBuffer &rawData,const ByteBuffer &previousData,bool verify)=0;

protected:
	uint32_t	_recursionLevel;
};

}

#endif
