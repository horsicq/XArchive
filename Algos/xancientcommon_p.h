/* Copyright (C) Teemu Suutari */

#ifndef XANCIENTCOMMON_P_H
#define XANCIENTCOMMON_P_H

#include <cstdint>

#include <string>
#include <vector>

namespace XAncientPrivate
{

constexpr uint16_t MultiChar2(const char (&cc)[3]) noexcept
{
	return static_cast<uint16_t>((static_cast<uint8_t>(cc[0]) << 8) | static_cast<uint8_t>(cc[1]));
}

constexpr uint32_t FourCC(const char (&cc)[5]) noexcept
{
	return static_cast<uint32_t>((static_cast<uint8_t>(cc[0]) << 24) | (static_cast<uint8_t>(cc[1]) << 16) | (static_cast<uint8_t>(cc[2]) << 8) | static_cast<uint8_t>(cc[3]));
}

uint32_t rotateBits(uint32_t value,uint32_t count) noexcept;

}

#endif
