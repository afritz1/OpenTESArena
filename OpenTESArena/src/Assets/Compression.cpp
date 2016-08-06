#include "Compression.h"

uint16_t Compression::getLE16(const uint8_t *buf)
{
	return buf[0] | (buf[1] << 8);
}

uint32_t Compression::getLE32(const uint8_t *buf)
{
	return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}
