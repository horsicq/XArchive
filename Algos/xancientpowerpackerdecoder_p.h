/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTPOWERPACKERDECODER_P_H
#define XANCIENTPOWERPACKERDECODER_P_H

#include "xancientcodecbase_p.h"
#include "xancientxpk_p.h"
#include "xancientinputstream_p.h"

#include <array>

namespace XAncientPrivate
{

class PowerPackerDecoder : public CodecDecoder, public XpkDecoder
{
private:
	class PPState : public XpkDecoder::State
	{
	public:
		PPState(uint32_t mode) noexcept;
		~PPState() noexcept=default;

		uint32_t _cachedMode;
	};

public:
	PowerPackerDecoder(const ByteBuffer &packedData,bool exactSizeKnown,bool verify);
	PowerPackerDecoder(uint32_t hdr,uint32_t recursionLevel,const ByteBuffer &packedData,std::shared_ptr<XpkDecoder::State> &state,bool verify);
	~PowerPackerDecoder() noexcept=default;

	const std::string &getName() const noexcept final;
	const std::string &getSubName() const noexcept final;

	size_t getPackedSize() const noexcept final;
	size_t getRawSize() const noexcept final;

	void decompressImpl(ByteBuffer &rawData,bool verify) final;
	void decompressImpl(ByteBuffer &rawData,const ByteBuffer &previousData,bool verify) final;

	static bool detectHeader(uint32_t hdr,uint32_t footer) noexcept;
	static bool detectHeaderXPK(uint32_t hdr) noexcept;

	static std::shared_ptr<CodecDecoder> create(const ByteBuffer &packedData,bool exactSizeKnown,bool verify);
	static std::shared_ptr<XpkDecoder> create(uint32_t hdr,uint32_t recursionLevel,const ByteBuffer &packedData,std::shared_ptr<XpkDecoder::State> &state,bool verify);

private:
	class DoneException : public std::exception
	{
	public:
		DoneException(uint32_t key) noexcept : _key(key) {}
		~DoneException() noexcept=default;

		uint32_t getKey() const noexcept { return _key; }

	private:
		uint32_t	_key;
	};

	void findKeyRound(BackwardInputStream &inputStream,LSBBitReader<BackwardInputStream> &bitReader,uint32_t keyBits,uint32_t keyMask,uint32_t outputPosition,uint32_t &iterCount);
	void findKey(uint32_t keyBits,uint32_t keyMask);

	const ByteBuffer		&_packedData;

	size_t			_dataStart{0};
	size_t			_rawSize{0};
	uint8_t			_startShift{0};
	std::array<uint8_t,4>	_modeTable;
	bool			_isObsfuscated{false};
	bool			_isXPK{false};
};

}

#endif
