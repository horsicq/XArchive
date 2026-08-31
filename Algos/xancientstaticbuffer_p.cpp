/* Copyright (C) Teemu Suutari */

#include "xancientstaticbuffer_p.h"


namespace XAncientPrivate
{

ReadOnlyBuffer::ReadOnlyBuffer(const uint8_t *data,size_t length) noexcept :
	_data{data},
	_length{length}
{
	// nothing needed
}

const uint8_t *ReadOnlyBuffer::data() const noexcept
{
	return _data;
}

uint8_t *ReadOnlyBuffer::data()
{
	throw InvalidOperationError();
}

size_t ReadOnlyBuffer::size() const noexcept
{
	return _length;
}

bool ReadOnlyBuffer::isResizable() const noexcept
{
	return false;
}

}
