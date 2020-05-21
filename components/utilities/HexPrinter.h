#ifndef HEX_PRINTER_H
#define HEX_PRINTER_H

#include <cstdint>

namespace HexPrinter
{
	// Writes to the given file.
	void print(const uint8_t *data, int count, const char *filename);

	// Writes to stdout by default.
	void print(const uint8_t *data, int count);
}

#endif
