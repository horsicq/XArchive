/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTCRC16_P_H
#define XANCIENTCRC16_P_H

#include <cstdint>

#include "xancientbuffer_p.h"

namespace XAncientPrivate
{

// The most common CRC16

uint16_t CRC16(const ByteBuffer &buffer,size_t offset,size_t len,uint16_t accumulator);

uint16_t CRC16Byte(uint8_t ch,uint16_t accumulator) noexcept;

}

#endif
