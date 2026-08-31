/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTUNIXPACKDECODER_P_H
#define XANCIENTUNIXPACKDECODER_P_H

#include "xancientcodecbase_p.h"

namespace XAncientPrivate
{

class UnixPackDecoder : public CodecDecoder
{
public:
	UnixPackDecoder(const ByteBuffer &packedData,bool exactSizeKnown,bool verify);
	~UnixPackDecoder() noexcept=default;

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
};

}

#endif
