/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTOUTPUTSTREAM_P_H
#define XANCIENTOUTPUTSTREAM_P_H

#include <cstddef>
#include <cstdint>

#include "xancientbuffer_p.h"

namespace XAncientPrivate
{

class ForwardOutputStreamBase
{
public:
	ForwardOutputStreamBase(ByteBuffer &buffer,size_t startOffset);
	virtual ~ForwardOutputStreamBase() noexcept=default;

	void writeByte(uint8_t value);

	uint8_t copy(size_t distance,size_t count);
	uint8_t copy(size_t distance,size_t count,const ByteBuffer &prevBuffer);
	uint8_t copy(size_t distance,size_t count,uint8_t defaultChar);
	uint8_t *history(size_t distance);
	const uint8_t *history(size_t distance) const;
	void produce(const ByteBuffer &src);

	size_t getOffset() const { return _currentOffset; }

protected:
	virtual void ensureSize(size_t offset)=0;

	ByteBuffer		&_buffer;
	size_t		_startOffset;
	size_t		_currentOffset;
};

class ForwardOutputStream : public ForwardOutputStreamBase
{
public:
	ForwardOutputStream(ByteBuffer &buffer,size_t startOffset,size_t endOffset);
	~ForwardOutputStream() noexcept=default;

	void reset(size_t startOffset,size_t endOffset);

	bool eof() const { return _currentOffset==_endOffset; }
	size_t getEndOffset() const { return _endOffset; }

protected:
	void ensureSize(size_t offset) final;

private:
	size_t		_endOffset;
};

class AutoExpandingForwardOutputStream : public ForwardOutputStreamBase
{
public:
	AutoExpandingForwardOutputStream(ByteBuffer &buffer);
	~AutoExpandingForwardOutputStream() noexcept;

protected:
	void ensureSize(size_t offset) final;

private:
	static constexpr size_t _advance{65536U};

	bool		_hasExpanded=false;
};

class BackwardOutputStream
{
public:
	BackwardOutputStream(ByteBuffer &buffer,size_t startOffset,size_t endOffset);
	~BackwardOutputStream() noexcept=default;

	void writeByte(uint8_t value);

	uint8_t copy(size_t distance,size_t count);
	uint8_t copy(size_t distance,size_t count,uint8_t defaultChar);

	bool eof() const { return _currentOffset==_startOffset; }
	size_t getOffset() const { return _currentOffset; }

private:
	ByteBuffer		&_buffer;
	size_t		_startOffset;
	size_t		_currentOffset;
	size_t		_endOffset;
};

}

#endif
