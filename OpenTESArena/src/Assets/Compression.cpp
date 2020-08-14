#include "Compression.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"

void Compression::decodeRLE(const uint8_t *src, int stopCount, uint8_t *dst, int dstSize)
{
	// Adapted from WinArena.
	int i = 0;
	int o = 0;

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

			DebugAssert(o >= 0);
			DebugAssert((o + static_cast<int>(count)) <= dstSize);
			for (uint32_t j = 0; j < count; j++)
			{
				dst[o] = value;
				o++;
			}
		}
		else
		{
			const uint32_t count = static_cast<uint32_t>(sample) + 1;

			DebugAssert(o >= 0);
			DebugAssert((o + static_cast<int>(count)) <= dstSize);
			for (uint32_t j = 0; j < count; j++)
			{
				dst[o] = src[i];
				o++;
				i++;
			}
		}
	}
}

void Compression::decodeRLEWords(const uint8_t *src, int stopCount, 
	std::vector<uint8_t> &out)
{
	int i = 0;
	int o = 0;

	while (o < stopCount)
	{
		const int16_t sample = Bytes::getLE16(src + i);
		i += 2;

		// If "sample" is positive, then "sample" literal words follow. Otherwise,
		// repeat the next word "sample" times.
		if (sample > 0)
		{
			for (int16_t j = 0; j < sample; j++)
			{
				const uint16_t value = Bytes::getLE16(src + i);
				i += 2;

				out.at(o * 2) = value & 0x00FF;
				out.at((o * 2) + 1) = (value & 0xFF00) >> 8;
				o++;
			}
		}
		else
		{
			const uint16_t value = Bytes::getLE16(src + i);
			i += 2;

			const uint16_t count = -sample;

			for (uint16_t j = 0; j < count; j++)
			{
				out.at(o * 2) = value & 0x00FF;
				out.at((o * 2) + 1) = (value & 0xFF00) >> 8;
				o++;
			}
		}
	}
}
