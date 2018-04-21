#include "ArenaSave.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"
#include "../Utilities/File.h"

void ArenaSave::SaveEngine::unscramble()
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
	scramble(reinterpret_cast<uint8_t*>(&this->player), 1054);
	scramble(reinterpret_cast<uint8_t*>(&this->playerData), 2609);
}

ArenaSave::Automap ArenaSave::loadAutomap(int index)
{
	ArenaSave::Automap automap;
	DebugNotImplemented();
	return automap;
}

ArenaSave::Log ArenaSave::loadLog(int index)
{
	ArenaSave::Log log;
	DebugNotImplemented();
	return log;
}

ArenaSave::SaveEngine ArenaSave::loadSaveEngn(int index)
{
	ArenaSave::SaveEngine saveEngine;
	DebugNotImplemented();
	return saveEngine;
}

ArenaSave::SaveGame ArenaSave::loadSaveGame(int index)
{
	ArenaSave::SaveGame saveGame;
	DebugNotImplemented();
	return saveGame;
}

ArenaSave::SpellData ArenaSave::loadSpells(int index)
{
	ArenaSave::SpellData spellData;
	DebugNotImplemented();
	return spellData;
}

ArenaSave::SpellData ArenaSave::loadSpellsg(int index)
{
	ArenaSave::SpellData spellData;
	DebugNotImplemented();
	return spellData;
}

ArenaSave::MQLevelState ArenaSave::loadState(int index)
{
	ArenaSave::MQLevelState state;
	DebugNotImplemented();
	return state;
}
