/* Copyright (C) Teemu Suutari */

#include "xancientinputstream_p.h"
#include "xancientoverflow_p.h"
#include "xancientsubbuffer_p.h"

namespace XAncientPrivate
{

ForwardInputStream::ForwardInputStream(const ByteBuffer &buffer,size_t startOffset,size_t endOffset,size_t overrunAllowance) :
	_buffer{buffer},
	_currentOffset{startOffset},
	_endOffset{endOffset},
	_overrunAllowance{overrunAllowance}
{
	if (_currentOffset>_endOffset || _currentOffset>_buffer.size() || _endOffset>_buffer.size())
		throw CodecDecoder::DecompressionError();
}

void ForwardInputStream::reset(size_t startOffset,size_t endOffset)
{
	_currentOffset=startOffset;
	_endOffset=endOffset;
	if (_currentOffset>_endOffset || _currentOffset>_buffer.size() || _endOffset>_buffer.size())
		throw CodecDecoder::DecompressionError();

	if (_linkedInputStream) _linkedInputStream->setEndOffset(_currentOffset);
}

uint8_t ForwardInputStream::readByte()
{
	if (_currentOffset>=_endOffset)
	{
		if (_overrunAllowance)
		{
			_overrunAllowance--;
			return 0;
		}
		throw CodecDecoder::DecompressionError();
	}
	uint8_t ret{_buffer[_currentOffset++]};
	if (_linkedInputStream) _linkedInputStream->setEndOffset(_currentOffset);
	return ret;
}

uint16_t ForwardInputStream::readBE16()
{
	uint16_t b0{readByte()};
	uint16_t b1{readByte()};
	return (b0<<8)|b1;
}

uint32_t ForwardInputStream::readBE32()
{
	uint32_t b0{readByte()};
	uint32_t b1{readByte()};
	uint32_t b2{readByte()};
	uint32_t b3{readByte()};
	return (b0<<24)|(b1<<16)|(b2<<8)|b3;
}

uint16_t ForwardInputStream::readLE16()
{
	uint16_t b0{readByte()};
	uint16_t b1{readByte()};
	return (b1<<8)|b0;
}

uint32_t ForwardInputStream::readLE32()
{
	uint32_t b0{readByte()};
	uint32_t b1{readByte()};
	uint32_t b2{readByte()};
	uint32_t b3{readByte()};
	return (b3<<24)|(b2<<16)|(b1<<8)|b0;
}

std::shared_ptr<const ByteBuffer> ForwardInputStream::consume(size_t bytes)
{
	if (BoundsCheck::sum(_currentOffset,bytes)>_endOffset)
		throw CodecDecoder::DecompressionError();
	std::shared_ptr<ReadOnlySlice> ret{std::make_shared<ReadOnlySlice>(_buffer,_currentOffset,bytes)};
	_currentOffset+=bytes;
	if (_linkedInputStream) _linkedInputStream->setEndOffset(_currentOffset);
	return ret;
}

void ForwardInputStream::setOffset(size_t offset)
{
	if (offset>_endOffset)
		throw CodecDecoder::DecompressionError();
	_currentOffset=offset;
	if (_linkedInputStream) _linkedInputStream->setEndOffset(_currentOffset);
}

BackwardInputStream::BackwardInputStream(const ByteBuffer &buffer,size_t startOffset,size_t endOffset) :
	_buffer{buffer},
	_currentOffset{endOffset},
	_endOffset{startOffset}
{
	if (_currentOffset<_endOffset || _currentOffset>buffer.size() || _endOffset>buffer.size())
		throw CodecDecoder::DecompressionError();
}

uint8_t BackwardInputStream::readByte()
{
	if (_currentOffset<=_endOffset)
		throw CodecDecoder::DecompressionError();
	uint8_t ret=_buffer[--_currentOffset];
	if (_linkedInputStream) _linkedInputStream->setEndOffset(_currentOffset);
	return ret;
}

uint16_t BackwardInputStream::readBE16()
{
	uint16_t b0{readByte()};
	uint16_t b1{readByte()};
	return (b1<<8)|b0;
}

uint32_t BackwardInputStream::readBE32()
{
	uint32_t b0{readByte()};
	uint32_t b1{readByte()};
	uint32_t b2{readByte()};
	uint32_t b3{readByte()};
	return (b3<<24)|(b2<<16)|(b1<<8)|b0;
}

uint16_t BackwardInputStream::readLE16()
{
	uint16_t b0{readByte()};
	uint16_t b1{readByte()};
	return (b0<<8)|b1;
}

uint32_t BackwardInputStream::readLE32()
{
	uint32_t b0{readByte()};
	uint32_t b1{readByte()};
	uint32_t b2{readByte()};
	uint32_t b3{readByte()};
	return (b0<<24)|(b1<<16)|(b2<<8)|b3;
}

void BackwardInputStream::setOffset(size_t offset)
{
	if (offset<_endOffset)
		throw CodecDecoder::DecompressionError();
	_currentOffset=offset;
	if (_linkedInputStream) _linkedInputStream->setEndOffset(_currentOffset);
}

}
