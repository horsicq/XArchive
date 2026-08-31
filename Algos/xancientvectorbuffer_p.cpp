/* Copyright (C) Teemu Suutari */

#include "xancientvectorbuffer_p.h"


namespace XAncientPrivate
{

VectorBuffer::VectorBuffer(std::vector<uint8_t> &refdata) noexcept :
	_refdata{refdata}
{
	// nothing needed
}

const uint8_t *VectorBuffer::data() const noexcept
{
	return _refdata.data();
}

uint8_t *VectorBuffer::data()
{
	return _refdata.data();
}

size_t VectorBuffer::size() const noexcept
{
	return _refdata.size();
}

bool VectorBuffer::isResizable() const noexcept
{
	return true;
}

void VectorBuffer::resize(size_t newSize)
{
	_refdata.resize(newSize);
}

}
