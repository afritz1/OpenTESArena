#include "ArenaTypes.h"
#include "../Utilities/Bytes.h"

void ArenaTypes::SaveEngine::unscramble()
{
	auto scramble = [](uint8_t *data, uint16_t length)
	{
		uint16_t buffer = length;
		for (uint16_t i = 0; i < length; i++)
		{
			const uint8_t key = Bytes::ror16(buffer, buffer & 0xF) & 0xFF;
			data[i] ^= key;
			buffer--;
		}
	};

	// Unscramble first two members (player and player data).
	// - To do: make sure the byte lengths are within struct bounds.
	scramble(reinterpret_cast<uint8_t*>(&this->player), 1054);
	scramble(reinterpret_cast<uint8_t*>(&this->playerData), 2609);
}
