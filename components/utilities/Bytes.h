#ifndef BYTES_H
#define BYTES_H

#include <climits>
#include <cstdint>

// Namespace for interacting with bits and bytes.

namespace Bytes
{
	uint16_t getLE16(const uint8_t *buf);
	uint32_t getLE24(const uint8_t *buf);
	uint32_t getLE32(const uint8_t *buf);

	// Circular rotation of an integer to the right.
	template <typename T>
	T ror(T value, unsigned int count)
	{
		constexpr unsigned int mask = (CHAR_BIT * sizeof(value)) - 1;
		count &= mask;
		return (value >> count) |
			(value << (static_cast<unsigned int>(-static_cast<int>(count)) & mask));
	}

	// Circular rotation of an integer to the left.
	template <typename T>
	T rol(T value, unsigned int count)
	{
		constexpr unsigned int mask = (CHAR_BIT * sizeof(value)) - 1;
		count &= mask;
		return (value << count) |
			(value >> (static_cast<unsigned int>(-static_cast<int>(count)) & mask));
	}
}

#endif
