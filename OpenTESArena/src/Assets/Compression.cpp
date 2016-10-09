#include "Compression.h"

void Compression::decodeRLE(const uint8_t *src, uint32_t stopCount, 
	std::vector<uint8_t> &out)
{
	// Adapted from WinArena.
	uint32_t i = 0;
	uint32_t o = 0;

	while (o < stopCount)
	{
		const uint8_t sample = src[i];
		src++;

		// Is the selected byte part of a compressed packet?
		if ((sample & 0x80) != 0)
		{
			const uint8_t value = src[i];
			src++;

			const uint32_t count = static_cast<uint32_t>(sample) - 0x7F;

			for (uint32_t j = 0; j < count; ++j)
			{
				out.at(o) = value;
				o++;
			}
		}
		else
		{
			const uint32_t count = static_cast<uint32_t>(sample) + 1;

			for (uint32_t j = 0; j < count; ++j)
			{
				out.at(o) = src[i];
				o++;
				i++;
			}
		}
	}
}
