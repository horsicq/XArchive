/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTTPWMDECODER_P_H
#define XANCIENTTPWMDECODER_P_H

#include "xancientcodecbase_p.h"

namespace XAncientPrivate
{

class TpwmDecoder : public CodecDecoder
{
public:
	TpwmDecoder(const ByteBuffer &packedData,bool verify);
	~TpwmDecoder() noexcept=default;

	const std::string &getName() const noexcept final;
	size_t getPackedSize() const noexcept final;
	size_t getRawSize() const noexcept final;

	void decompressImpl(ByteBuffer &rawData,bool verify) final;

	static bool detectHeader(uint32_t hdr,uint32_t footer) noexcept;
	static std::shared_ptr<CodecDecoder> create(const ByteBuffer &packedData,bool exactSizeKnown,bool verify);

private:
	const ByteBuffer	&_packedData;

	uint32_t	_rawSize{0};
	size_t		_decompressedPackedSize{0};
};

}

#endif
