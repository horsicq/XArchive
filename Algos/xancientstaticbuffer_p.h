/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTSTATICBUFFER_P_H
#define XANCIENTSTATICBUFFER_P_H

#include <cstddef>
#include <cstdint>

#include "xancientbuffer_p.h"

namespace XAncientPrivate
{

template<size_t N>
class FixedBuffer : public ByteBuffer
{
public:
	FixedBuffer(const FixedBuffer&)=delete;
	FixedBuffer& operator=(const FixedBuffer&)=delete;

	FixedBuffer() noexcept=default;
	~FixedBuffer() noexcept=default;

	const uint8_t *data() const noexcept final
	{
		return _data;
	}

	uint8_t *data() final
	{
		return _data;
	}

	size_t size() const noexcept final
	{
		return N;
	}

	bool isResizable() const noexcept final
	{
		return false;
	}

private:
	uint8_t 	_data[N];
};


class ReadOnlyBuffer : public ByteBuffer
{
public:
	ReadOnlyBuffer(const ReadOnlyBuffer&)=delete;
	ReadOnlyBuffer& operator=(const ReadOnlyBuffer&)=delete;

	ReadOnlyBuffer(const uint8_t *data,size_t length) noexcept;
	~ReadOnlyBuffer() noexcept=default;

	const uint8_t *data() const noexcept final;
	uint8_t *data() final;

	size_t size() const noexcept final;
	bool isResizable() const noexcept final;

private:
	const uint8_t 	*_data;
	size_t		_length;
};

}

#endif
