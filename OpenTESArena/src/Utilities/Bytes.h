#ifndef BYTES_H
#define BYTES_H

#include <cstdint>

// Static class for reading bytes from a byte buffer given certain constraints,
// such as the buffer being in little endian format.

class Bytes
{
private:
	Bytes() = delete;
	Bytes(const Bytes&) = delete;
	~Bytes() = delete;
public:
	static uint16_t getLE16(const uint8_t *buf);
	static uint32_t getLE32(const uint8_t *buf);
};

#endif
