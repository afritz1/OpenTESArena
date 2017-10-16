#include <climits>

#include "Bytes.h"

uint16_t Bytes::getLE16(const uint8_t *buf)
{
	return buf[0] | (buf[1] << 8);
}

uint32_t Bytes::getLE24(const uint8_t *buf)
{
	return buf[0] | (buf[1] << 8) | (buf[2] << 16);
}

uint32_t Bytes::getLE32(const uint8_t *buf)
{
	return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

uint16_t Bytes::ror16(uint16_t value, unsigned int count)
{
	const unsigned int mask = (CHAR_BIT * sizeof(value)) - 1;
	count &= mask;
	return (value >> count) | 
		(value << (static_cast<unsigned int>(-static_cast<int>(count)) & mask));
}

uint32_t Bytes::rol32(uint32_t value, unsigned int count)
{
	const unsigned int mask = (CHAR_BIT * sizeof(value)) - 1;
	count &= mask;
	return (value << count) |
		(value >> (static_cast<unsigned int>(-static_cast<int>(count)) & mask));
}
