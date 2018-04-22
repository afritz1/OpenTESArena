#ifndef ARENA_SAVE_H
#define ARENA_SAVE_H

#include <string>

#include "ArenaTypes.h"

// Loads various files saved by the original game.

class ArenaSave
{
private:
	ArenaSave() = delete;
	~ArenaSave() = delete;
public:
	static ArenaTypes::Automap loadAUTOMAP(const std::string &filename, int index);
	static ArenaTypes::Log loadLOG(const std::string &filename, int index);
	static ArenaTypes::SaveEngine loadSAVEENGN(const std::string &filename, int index);
	static ArenaTypes::SaveGame loadSAVEGAME(const std::string &filename, int index);
	static ArenaTypes::Spells loadSPELLS(const std::string &filename, int index);
	static ArenaTypes::Spells loadSPELLSG(const std::string &filename, int index);
	static ArenaTypes::MQLevelState loadSTATE(const std::string &filename, int index);
	// To do: load city data.
	// To do: load INN.0x.
	// To do: load wild 001, 002, 003, 004.

	static void saveAUTOMAP(const std::string &filename, int index,
		const ArenaTypes::Automap &data);
	static void saveLOG(const std::string &filename, int index,
		const ArenaTypes::Log &data);
	static void saveSAVEENGN(const std::string &filename, int index,
		const ArenaTypes::SaveEngine &data);
	static void saveSAVEGAME(const std::string &filename, int index,
		const ArenaTypes::SaveGame &data);
	static void saveSPELLS(const std::string &filename, int index,
		const ArenaTypes::Spells &data);
	static void saveSPELLSG(const std::string &filename, int index,
		const ArenaTypes::Spells &data);
	static void saveSTATE(const std::string &filename, int index,
		const ArenaTypes::MQLevelState &data);
	// To do: save city data.
	// To do: save INN.0x.
	// To do: save wild 001, 002, 003, 004.
};

#endif
