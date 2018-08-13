#ifndef BYTES_H
#define BYTES_H

#include <climits>
#include <cstdint>

// Static class for interacting with bits and bytes.

class Bytes
{
private:
	Bytes() = delete;
	~Bytes() = delete;
public:
	static uint16_t getLE16(const uint8_t *buf);
	static uint32_t getLE24(const uint8_t *buf);
	static uint32_t getLE32(const uint8_t *buf);

	// Circular rotation of an integer to the right.
	template <typename T>
	static T ror(T value, unsigned int count)
	{
		const unsigned int mask = (CHAR_BIT * sizeof(value)) - 1;
		count &= mask;
		return (value >> count) |
			(value << (static_cast<unsigned int>(-static_cast<int>(count)) & mask));
	}

	// Circular rotation of an integer to the left.
	template <typename T>
	static T rol(T value, unsigned int count)
	{
		const unsigned int mask = (CHAR_BIT * sizeof(value)) - 1;
		count &= mask;
		return (value << count) |
			(value >> (static_cast<unsigned int>(-static_cast<int>(count)) & mask));
	}
};

#endif
