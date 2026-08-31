/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTDMSDECODER_P_H
#define XANCIENTDMSDECODER_P_H

#include "xancientcodecbase_p.h"

namespace XAncientPrivate
{


class DmsDecoder : public CodecDecoder
{
public:
	DmsDecoder(const ByteBuffer &packedData,bool verify);
	~DmsDecoder() noexcept=default;

	const std::string &getName() const noexcept final;
	size_t getPackedSize() const noexcept final;
	size_t getRawSize() const noexcept final;

	size_t getImageSize() const noexcept final;
	size_t getImageOffset() const noexcept final;

	void decompressImpl(ByteBuffer &rawData,bool verify) final;

	static bool detectHeader(uint32_t hdr,uint32_t footer) noexcept;
	static std::shared_ptr<CodecDecoder> create(const ByteBuffer &packedData,bool exactSizeKnown,bool verify);

private:
	void decompressImpl(ByteBuffer &rawData,bool verify,uint32_t &restartPosition);

	const ByteBuffer	&_packedData;

	uint32_t	_packedSize{0};
	uint32_t	_rawSize{0};
	uint32_t	_contextBufferSize{0};
	uint32_t	_tmpBufferSize{0};
	uint32_t	_imageSize;
	uint32_t	_rawOffset;
	uint32_t	_minTrack;
	bool		_isHD;
	bool		_isObsfuscated;
};

}

#endif
