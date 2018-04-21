#ifndef ARENA_SAVE_H
#define ARENA_SAVE_H

#include "ArenaTypes.h"

// Loads various files saved by the original game.

class ArenaSave
{
private:
	ArenaSave() = delete;
	~ArenaSave() = delete;
public:
	static ArenaTypes::Automap loadAUTOMAP(int index);
	static ArenaTypes::Log loadLOG(int index);
	static ArenaTypes::SaveEngine loadSAVEENGN(int index);
	static ArenaTypes::SaveGame loadSAVEGAME(int index);
	static ArenaTypes::SpellData loadSPELLS(int index);
	static ArenaTypes::SpellData loadSPELLSG(int index);
	static ArenaTypes::MQLevelState loadSTATE(int index);
	// To do: load city data.
	// To do: load INN.0x.
	// To do: load wild 001, 002, 003, 004.
};

#endif
