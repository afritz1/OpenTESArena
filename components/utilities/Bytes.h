#ifndef BYTES_H
#define BYTES_H

#include <climits>
#include <cstddef>
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

	// Gets the index of the MSB or -1 if the value is 0.
	template<typename T>
	constexpr int findHighestSetBitIndex(T value)
	{
		static_assert(std::is_integral_v<T>);
		constexpr int bitCount = CHAR_BIT * sizeof(value);
		for (int i = bitCount - 1; i >= 0; i--)
		{
			if (((value >> i) & 1) == 1)
			{
				return i;
			}
		}

		return -1;
	}

	// Gets the exact number of bits the integer takes up.
	template<typename T>
	constexpr int getRequiredBitCount(T value)
	{
		static_assert(std::is_integral_v<T>);

		if (value == 0)
		{
			// 0 still needs one bit.
			return 1;
		}
		else if (value < 0)
		{
			// Assume that negative numbers need all the 1's (due to how right shifting works).
			return CHAR_BIT * sizeof(value);
		}

		int requiredBitCount = 0;
		while (value != 0)
		{
			value >>= 1;
			requiredBitCount++;
		}

		return requiredBitCount;
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
	int getBytesToNextAlignment(uintptr_t address, size_t alignment);

	// Gets the next aligned address given a potentially unaligned address for the type.
	// If the given address is aligned then the return value is unchanged.
	uintptr_t getAlignedAddress(uintptr_t address, size_t alignment);

	template<typename T>
	uintptr_t getAlignedAddress(uintptr_t address)
	{
		return Bytes::getAlignedAddress(address, alignof(T));
	}
}

#endif
