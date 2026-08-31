/* Copyright (C) Teemu Suutari */

#include "xancientbuffer_p.h"
#include "xancientoverflow_p.h"


namespace XAncientPrivate
{

void ByteBuffer::resize(size_t newSize)
{
	throw InvalidOperationError();
}

uint8_t &ByteBuffer::operator[](size_t i)
{
	if (i>=size()) throw OutOfBoundsError();
	return data()[i];
}

const uint8_t &ByteBuffer::operator[](size_t i) const
{
	if (i>=size()) throw OutOfBoundsError();
	return data()[i];
}

uint32_t ByteBuffer::readBE32(size_t offset) const
{
	if (BoundsCheck::sum(offset,4U)>size()) throw OutOfBoundsError();
	const uint8_t *ptr=data()+offset;
	return (uint32_t(ptr[0])<<24)|(uint32_t(ptr[1])<<16)|(uint32_t(ptr[2])<<8)|uint32_t(ptr[3]);
}

uint16_t ByteBuffer::readBE16(size_t offset) const
{
	if (BoundsCheck::sum(offset,2U)>size()) throw OutOfBoundsError();
	const uint8_t *ptr=data()+offset;
	return (uint16_t(ptr[0])<<8)|uint16_t(ptr[1]);
}

uint64_t ByteBuffer::readLE64(size_t offset) const
{
	if (BoundsCheck::sum(offset,8U)>size()) throw OutOfBoundsError();
	const uint8_t *ptr=data()+offset;
	return (uint64_t(ptr[7])<<56)|(uint64_t(ptr[6])<<48)|(uint64_t(ptr[5])<<40)|(uint64_t(ptr[4])<<32)|
		(uint64_t(ptr[3])<<24)|(uint64_t(ptr[2])<<16)|(uint64_t(ptr[1])<<8)|uint64_t(ptr[0]);
}

uint32_t ByteBuffer::readLE32(size_t offset) const
{
	if (BoundsCheck::sum(offset,4U)>size()) throw OutOfBoundsError();
	const uint8_t *ptr=data()+offset;
	return (uint32_t(ptr[3])<<24)|(uint32_t(ptr[2])<<16)|(uint32_t(ptr[1])<<8)|uint32_t(ptr[0]);
}

uint16_t ByteBuffer::readLE16(size_t offset) const
{
	if (BoundsCheck::sum(offset,2U)>size()) throw OutOfBoundsError();
	const uint8_t *ptr=data()+offset;
	return (uint16_t(ptr[1])<<8)|uint16_t(ptr[0]);
}

uint8_t ByteBuffer::read8(size_t offset) const
{
	if (offset>=size()) throw OutOfBoundsError();
	const uint8_t *ptr=reinterpret_cast<const uint8_t*>(data())+offset;
	return ptr[0];
}

}
