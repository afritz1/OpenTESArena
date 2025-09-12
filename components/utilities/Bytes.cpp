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

int Bytes::getBytesToNextAlignment(uintptr_t address, size_t alignment)
{
	const size_t modulo = address % alignment;
	if (modulo == 0)
	{
		return 0;
	}

	return static_cast<int>(alignment - modulo);
}

uintptr_t Bytes::getAlignedAddress(uintptr_t address, size_t alignment)
{
	return address + Bytes::getBytesToNextAlignment(address, alignment);
}
