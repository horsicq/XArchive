/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTFREEZEDECODER_P_H
#define XANCIENTFREEZEDECODER_P_H

#include "xancientcodecbase_p.h"

#include <array>

namespace XAncientPrivate
{

class FreezeDecoder : public CodecDecoder
{
public:
	FreezeDecoder(const ByteBuffer &packedData,bool exactSizeKnown,bool verify);
	~FreezeDecoder() noexcept=default;

	size_t getRawSize() const noexcept final;
	size_t getPackedSize() const noexcept final;

	const std::string &getName() const noexcept final;

	void decompressImpl(ByteBuffer &rawData,bool verify) final;

	static bool detectHeader(uint32_t hdr,uint32_t footer) noexcept;

	static std::shared_ptr<CodecDecoder> create(const ByteBuffer &packedData,bool exactSizeKnown,bool verify);

private:
	const ByteBuffer	&_packedData;

	size_t		_packedSize{0};
	size_t		_rawSize{0};
	bool		_isOldVersion;
	bool		_exactSizeKnown;

	std::array<uint8_t,8> _hufTable;
};

}

#endif
