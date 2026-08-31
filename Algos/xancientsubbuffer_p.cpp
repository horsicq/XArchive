/* Copyright (C) Teemu Suutari */

#include "xancientsubbuffer_p.h"


namespace XAncientPrivate
{

template <>
uint8_t *BufferSlice<ByteBuffer>::data()
{
	return _base.data()+_start;
}

template <>
uint8_t *BufferSlice<const ByteBuffer>::data()
{
	throw InvalidOperationError();
}

}
