/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTRNCDECODER_P_H
#define XANCIENTRNCDECODER_P_H

#include "xancientcodecbase_p.h"

namespace XAncientPrivate
{

class RncDecoder : public CodecDecoder
{
public:
	RncDecoder(const ByteBuffer &packedData,bool verify);
	~RncDecoder() noexcept=default;

	const std::string &getName() const noexcept final;
	size_t getPackedSize() const noexcept final;
	size_t getRawSize() const noexcept final;

	void decompressImpl(ByteBuffer &rawData,bool verify) final;

	static bool detectHeader(uint32_t hdr,uint32_t footer) noexcept;

	static std::shared_ptr<CodecDecoder> create(const ByteBuffer &packedData,bool exactSizeKnown,bool verify);

private:
	enum class Version
	{
		RNC1Old=0,
		RNC1New,
		RNC2Old,
		RNC2New
	};

	void RNCDecompressOld(ByteBuffer &rawData,bool verify,bool rnc2);
	void RNC1DecompressNew(ByteBuffer &rawData,bool verify);
	void RNC2DecompressNew(ByteBuffer &rawData,bool verify);

	const ByteBuffer	&_packedData;

	uint32_t	_rawSize{0};
	uint32_t	_packedSize{0};
	uint16_t	_rawCRC{0};
	uint8_t		_chunks{0};
	Version		_ver;
};

}

#endif
