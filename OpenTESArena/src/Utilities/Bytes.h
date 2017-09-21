#ifndef BYTES_H
#define BYTES_H

#include <cstdint>

// Static class for interacting with bits and bytes. For example, reading bytes from a 
// byte buffer given certain constraints, such as the buffer being in little endian format.

class Bytes
{
private:
	Bytes() = delete;
	Bytes(const Bytes&) = delete;
	~Bytes() = delete;
public:
	static uint16_t getLE16(const uint8_t *buf);
	static uint32_t getLE24(const uint8_t *buf);
	static uint32_t getLE32(const uint8_t *buf);

	// Circular rotation of a 16-bit integer to the right.
	static uint16_t ror16(uint16_t value, unsigned int count);
};

#endif
