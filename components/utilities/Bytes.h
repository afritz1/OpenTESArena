#ifndef BYTES_H
#define BYTES_H

#include <climits>
#include <cstdint>
#include <type_traits>

// Namespace for interacting with bits and bytes.

namespace Bytes
{
	uint16_t getLE16(const uint8_t *buf);
	uint32_t getLE24(const uint8_t *buf);
	uint32_t getLE32(const uint8_t *buf);

	// Counts number of 1's in an integer's bits.
	template <typename T>
	constexpr int getSetBitCount(T value)
	{
		static_assert(std::is_integral_v<T>);
		constexpr int bitCount = CHAR_BIT * sizeof(value);
		int setBitCount = 0;
		for (int i = 0; i < bitCount; i++)
		{
			if (((value >> i) & 1) == 1)
			{
				setBitCount++;
			}
		}

		return setBitCount;
	}

	// Circular rotation of an integer to the right.
	template <typename T>
	T ror(T value, unsigned int count)
	{
		static_assert(std::is_integral_v<T>);
		constexpr unsigned int mask = (CHAR_BIT * sizeof(value)) - 1;
		count &= mask;
		return (value >> count) |
			(value << (static_cast<unsigned int>(-static_cast<int>(count)) & mask));
	}

	// Circular rotation of an integer to the left.
	template <typename T>
	T rol(T value, unsigned int count)
	{
		static_assert(std::is_integral_v<T>);
		constexpr unsigned int mask = (CHAR_BIT * sizeof(value)) - 1;
		count &= mask;
		return (value << count) |
			(value >> (static_cast<unsigned int>(-static_cast<int>(count)) & mask));
	}
}

#endif
