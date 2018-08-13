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
