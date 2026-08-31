/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTVECTORBUFFER_P_H
#define XANCIENTVECTORBUFFER_P_H

#include <vector>

#include <cstddef>
#include <cstdint>

#include "xancientbuffer_p.h"

namespace XAncientPrivate
{

class VectorBuffer : public ByteBuffer
{
public:
	VectorBuffer(std::vector<uint8_t> &refdata) noexcept;
	~VectorBuffer() noexcept=default;

	const uint8_t *data() const noexcept final;
	uint8_t *data() final;
	size_t size() const noexcept final;

	bool isResizable() const noexcept final;
	void resize(size_t newSize) final;

private:
	std::vector<uint8_t> & _refdata;
};

}

#endif
