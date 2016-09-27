#include "Compression.h"

void Compression::decodeRLE(const std::vector<uint8_t>::iterator src, 
	const std::vector<uint8_t>::iterator srcEnd, std::vector<uint8_t> &out)
{
	// Adapted from WinArena.
	uint32_t i = 0;
	uint32_t o = 0;
	const uint32_t stopCount = static_cast<uint32_t>(srcEnd - src);

	while (o < stopCount)
	{
		const uint8_t sample = src[i];
		i++;

		if ((sample & 0x80) > 0)
		{
			// Compressed packet.
			const uint8_t value = src[i];
			i++;

			const uint32_t count = static_cast<uint32_t>(sample) - 0x7F;
			for (uint32_t j = 0; j < count; ++j)
			{
				out.at(o) = value;
				o++;
			}
		}
		else
		{
			// Uncompressed packet.
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
