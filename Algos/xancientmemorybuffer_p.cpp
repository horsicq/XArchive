/* Copyright (C) Teemu Suutari */

#include <cstring>
#include <cstdlib>

#include <memory>
#include <new>

#include "xancientmemorybuffer_p.h"
#include "xancientoverflow_p.h"


namespace XAncientPrivate
{

OwnedBuffer::OwnedBuffer(size_t size) :
	_data{reinterpret_cast<uint8_t*>(std::malloc(size))},
	_size{size}
{
	if (!_data) throw std::bad_alloc();
}

OwnedBuffer::OwnedBuffer(const ByteBuffer &src,size_t offset,size_t size) :
	OwnedBuffer{size}
{
	if(BoundsCheck::sum(offset,size)>src.size()) throw InvalidOperationError();
	std::memcpy(_data,src.data()+offset,size);
}


OwnedBuffer::~OwnedBuffer() noexcept
{
	std::free(_data);
}

const uint8_t *OwnedBuffer::data() const noexcept
{
	return _data;
}

uint8_t *OwnedBuffer::data()
{
	return _data;
}

size_t OwnedBuffer::size() const noexcept
{
	return _size;
}

bool OwnedBuffer::isResizable() const noexcept
{
	return true;
}

void OwnedBuffer::resize(size_t newSize)
{
	if (!newSize)
	{
		std::free(_data);
		_data=nullptr;
		_size=0;
		return;
	}
	uint8_t *newData=reinterpret_cast<uint8_t*>(std::realloc(_data,newSize));
	if (!newData)
	{
		std::free(_data);
		_data=nullptr;
		_size=0;
		throw std::bad_alloc();
	}
	_data=newData;
	_size=newSize;
}

}
