/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTSUBBUFFER_P_H
#define XANCIENTSUBBUFFER_P_H

#include <cstddef>
#include <cstdint>

#include "xancientbuffer_p.h"
#include "xancientoverflow_p.h"

namespace XAncientPrivate
{

// helpers to splice ByteBuffer

template <typename T>
class BufferSlice : public ByteBuffer
{
public:
	BufferSlice(const BufferSlice&)=delete;
	BufferSlice& operator=(const BufferSlice&)=delete;

	BufferSlice(T &base,size_t start,size_t length) :
		_base{base},
		_start{start},
		_length{length}
	{
		if (BoundsCheck::sum(start,length)>_base.size())
			throw OutOfBoundsError();
	}
	
	~BufferSlice() noexcept=default;

	const uint8_t *data() const noexcept final
	{
		return _base.data()+_start;
	}

	uint8_t *data() final;

	size_t size() const noexcept final
	{
		return _length;
	}

	bool isResizable() const noexcept final
	{
		return false;
	}

	// can only make the buffer smaller, can't run away from the current bounds
	void adjust(size_t start,size_t length)
	{
		if (start<_start || BoundsCheck::sum(start,length)>_start+_length) throw OutOfBoundsError();
		_start=start;
		_length=length;
	}

private:
	T &_base;
	size_t	_start;
	size_t	_length;
};

typedef BufferSlice<ByteBuffer> MutableSlice;
typedef BufferSlice<const ByteBuffer> ReadOnlySlice;

}

#endif
