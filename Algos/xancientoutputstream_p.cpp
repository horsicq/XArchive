/* Copyright (C) Teemu Suutari */

#include <cstring>

#include <algorithm>

#include "xancientoutputstream_p.h"
// for exceptions
#include "xancientcodecbase_p.h"
#include "xancientcommon_p.h"
#include "xancientoverflow_p.h"


namespace XAncientPrivate
{

ForwardOutputStreamBase::ForwardOutputStreamBase(ByteBuffer &buffer,size_t startOffset) :
	_buffer{buffer},
	_startOffset{startOffset},
	_currentOffset{startOffset}
{
	// should call ensureSize but can't
}

void ForwardOutputStreamBase::writeByte(uint8_t value)
{
	ensureSize(_currentOffset+1);
	_buffer[_currentOffset++]=value;
}

uint8_t ForwardOutputStreamBase::copy(size_t distance,size_t count)
{
	ensureSize(BoundsCheck::sum(_currentOffset,count));
	if (!distance || BoundsCheck::sum(_startOffset,distance)>_currentOffset)
		throw CodecDecoder::DecompressionError();
	uint8_t ret{0};
	for (size_t i=0;i<count;i++,_currentOffset++)
		ret=_buffer[_currentOffset]=_buffer[_currentOffset-distance];
	return ret;
}

uint8_t ForwardOutputStreamBase::copy(size_t distance,size_t count,const ByteBuffer &prevBuffer)
{
	ensureSize(BoundsCheck::sum(_currentOffset,count));
	if (!distance)
		throw CodecDecoder::DecompressionError();
	size_t prevCount{0};
	uint8_t ret{0};
	if (BoundsCheck::sum(_startOffset,distance)>_currentOffset)
	{
		size_t prevSize{prevBuffer.size()};
		if (_startOffset+distance>_currentOffset+prevSize)
			throw CodecDecoder::DecompressionError(); 
		size_t prevDist{_startOffset+distance-_currentOffset};
		prevCount=std::min(count,prevDist);
		const uint8_t *prev{&prevBuffer[prevSize-prevDist]};
		for (size_t i=0;i<prevCount;i++,_currentOffset++)
			ret=_buffer[_currentOffset]=prev[i];
	}
	for (size_t i=prevCount;i<count;i++,_currentOffset++)
		ret=_buffer[_currentOffset]=_buffer[_currentOffset-distance];
	return ret;
}

uint8_t ForwardOutputStreamBase::copy(size_t distance,size_t count,uint8_t defaultChar)
{
	ensureSize(BoundsCheck::sum(_currentOffset,count));
	if (!distance)
		throw CodecDecoder::DecompressionError();
	size_t prevCount{0};
	uint8_t ret{0};
	if (BoundsCheck::sum(_startOffset,distance)>_currentOffset)
	{
		prevCount=std::min(count,_startOffset+distance-_currentOffset);
		for (size_t i=0;i<prevCount;i++,_currentOffset++)
			ret=_buffer[_currentOffset]=defaultChar;
	}
	for (size_t i=prevCount;i<count;i++,_currentOffset++)
		ret=_buffer[_currentOffset]=_buffer[_currentOffset-distance];
	return ret;
}

const uint8_t *ForwardOutputStreamBase::history(size_t distance) const
{
	if (BoundsCheck::sum(distance,_startOffset)>_currentOffset)
		throw CodecDecoder::DecompressionError();
	return &_buffer[_currentOffset-distance];
}

uint8_t *ForwardOutputStreamBase::history(size_t distance)
{
	if (BoundsCheck::sum(distance,_startOffset)>_currentOffset)
		throw CodecDecoder::DecompressionError();
	return &_buffer[_currentOffset-distance];
}

void ForwardOutputStreamBase::produce(const ByteBuffer &src)
{
	ensureSize(BoundsCheck::sum(_currentOffset,src.size()));
	std::memcpy(&_buffer[_currentOffset],src.data(),src.size());
	_currentOffset+=src.size();
}

// ---

ForwardOutputStream::ForwardOutputStream(ByteBuffer &buffer,size_t startOffset,size_t endOffset) :
	ForwardOutputStreamBase{buffer,startOffset},
	_endOffset{endOffset}
{
	if (_startOffset>_endOffset || _endOffset>_buffer.size())
		throw CodecDecoder::DecompressionError();
}

void ForwardOutputStream::reset(size_t startOffset,size_t endOffset)
{
	_currentOffset=_startOffset=startOffset;
	_endOffset=endOffset;
	if (_startOffset>_endOffset || _endOffset>_buffer.size())
		throw CodecDecoder::DecompressionError();
}

void ForwardOutputStream::ensureSize(size_t offset)
{
	if (offset>_endOffset)
		throw CodecDecoder::DecompressionError();
}

// ---

AutoExpandingForwardOutputStream::AutoExpandingForwardOutputStream(ByteBuffer &buffer) :
	ForwardOutputStreamBase{buffer,0}
{
	// nothing needed
}

AutoExpandingForwardOutputStream::~AutoExpandingForwardOutputStream() noexcept
{
	// trim
	if (_hasExpanded && _currentOffset!=_buffer.size())
		_buffer.resize(_currentOffset);
}

void AutoExpandingForwardOutputStream::ensureSize(size_t offset)
{
	if (offset>CodecDecoder::getMaxRawSize())
		throw CodecDecoder::DecompressionError();
	if (offset>_buffer.size())
	{
		_buffer.resize(offset+_advance);
		_hasExpanded=true;
	}
}

// ---

BackwardOutputStream::BackwardOutputStream(ByteBuffer &buffer,size_t startOffset,size_t endOffset) :
	_buffer{buffer},
	_startOffset{startOffset},
	_currentOffset{endOffset},
	_endOffset{endOffset}
{
	if (_startOffset>_endOffset || _currentOffset>buffer.size() || _endOffset>buffer.size())
		throw CodecDecoder::DecompressionError();
}

void BackwardOutputStream::writeByte(uint8_t value)
{
	if (_currentOffset<=_startOffset)
		throw CodecDecoder::DecompressionError();
	_buffer[--_currentOffset]=value;
}

uint8_t BackwardOutputStream::copy(size_t distance,size_t count)
{
	if (!distance || BoundsCheck::sum(_startOffset,count)>_currentOffset || BoundsCheck::sum(_currentOffset,distance)>_endOffset)
		throw CodecDecoder::DecompressionError();
	uint8_t ret{0};
	for (size_t i=0;i<count;i++,--_currentOffset)
		ret=_buffer[_currentOffset-1]=_buffer[_currentOffset+distance-1];
	return ret;
}

uint8_t BackwardOutputStream::copy(size_t distance,size_t count,uint8_t defaultChar)
{
	if (!distance || BoundsCheck::sum(_startOffset,count)>_currentOffset)
		throw CodecDecoder::DecompressionError();
	size_t prevCount{0};
	uint8_t ret{0};
	if (BoundsCheck::sum(_currentOffset,distance)>_endOffset)
	{
		prevCount=std::min(count,_currentOffset+distance-_endOffset);
		for (size_t i=0;i<prevCount;i++,--_currentOffset)
			ret=_buffer[_currentOffset-1]=defaultChar;
	}
	for (size_t i=prevCount;i<count;i++,--_currentOffset)
		ret=_buffer[_currentOffset-1]=_buffer[_currentOffset+distance-1];
	return ret;
}

}
