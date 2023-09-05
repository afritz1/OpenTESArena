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
	template<typename T>
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
	template<typename T>
	T ror(T value, unsigned int count)
	{
		static_assert(std::is_integral_v<T>);
		constexpr unsigned int mask = (CHAR_BIT * sizeof(value)) - 1;
		count &= mask;
		return (value >> count) | (value << (static_cast<unsigned int>(-static_cast<int>(count)) & mask));
	}

	// Circular rotation of an integer to the left.
	template<typename T>
	T rol(T value, unsigned int count)
	{
		static_assert(std::is_integral_v<T>);
		constexpr unsigned int mask = (CHAR_BIT * sizeof(value)) - 1;
		count &= mask;
		return (value << count) | (value >> (static_cast<unsigned int>(-static_cast<int>(count)) & mask));
	}

	// Number of bytes to increment the address by to get a valid aligned address for the type.
	int getBytesToNextAlignment(uintptr_t address, size_t alignment)
	{
		const size_t modulo = address % alignment;
		return (modulo != 0) ? static_cast<int>(alignment - modulo) : 0;
	}

	// Gets the next aligned address given a potentially unaligned address for the type.
	// If the given address is aligned then the return value is unchanged.
	uintptr_t getAlignedAddress(uintptr_t address, size_t alignment)
	{
		return address + Bytes::getBytesToNextAlignment(address, alignment);
	}

	template<typename T>
	uintptr_t getAlignedAddress(uintptr_t address)
	{
		return Bytes::getAlignedAddress(address, alignof(T));
	}
}

#endif
