/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTMEMORYBUFFER_P_H
#define XANCIENTMEMORYBUFFER_P_H

#include <memory>

#include "xancientbuffer_p.h"

namespace XAncientPrivate
{

class OwnedBuffer : public ByteBuffer
{
public:
	OwnedBuffer(size_t size);
	OwnedBuffer(const ByteBuffer &src,size_t offset,size_t size);
	~OwnedBuffer() noexcept;

	const uint8_t *data() const noexcept final;
	uint8_t *data() final;
	size_t size() const noexcept final;

	bool isResizable() const noexcept final;
	void resize(size_t newSize) final;

private:
	uint8_t*			_data;
	size_t				_size;
};

}

#endif
