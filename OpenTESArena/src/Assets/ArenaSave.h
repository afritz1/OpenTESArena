#ifndef ARENA_SAVE_H
#define ARENA_SAVE_H

#include <string>

#include "ArenaTypes.h"

// Loads various files saved by the original game.

class ArenaSave
{
private:
	static const std::string AUTOMAP_FILENAME;
	static const std::string IN_FILENAME;
	static const std::string LOG_FILENAME;
	static const std::string NAMES_FILENAME;
	static const std::string RE_FILENAME;
	static const std::string SAVEENGN_FILENAME;
	static const std::string SAVEGAME_FILENAME;
	static const std::string SPELLS_FILENAME;
	static const std::string SPELLSG_FILENAME;
	static const std::string STATE_FILENAME;

	ArenaSave() = delete;
	~ArenaSave() = delete;
public:
	static ArenaTypes::Automap loadAUTOMAP(const std::string &savePath, int index);
	static ArenaTypes::Tavern loadIN(const std::string &savePath, int number, int index);
	static ArenaTypes::Log loadLOG(const std::string &savePath, int index);
	static ArenaTypes::Names loadNAMES(const std::string &savePath);
	static ArenaTypes::Repair loadRE(const std::string &savePath, int number, int index);
	static ArenaTypes::SaveEngine loadSAVEENGN(const std::string &savePath, int index);
	static ArenaTypes::SaveGame loadSAVEGAME(const std::string &savePath, int index);
	static ArenaTypes::Spells loadSPELLS(const std::string &savePath, int index);
	static ArenaTypes::Spellsg loadSPELLSG(const std::string &savePath, int index);
	static ArenaTypes::MQLevelState loadSTATE(const std::string &savePath, int index);
	// To do: load city data.
	// To do: load INN.0x.
	// To do: load wild 001, 002, 003, 004.

	static void saveAUTOMAP(const std::string &savePath, int index,
		const ArenaTypes::Automap &data);
	static void saveIN(const std::string &savePath, int number, int index,
		const ArenaTypes::Tavern &data);
	static void saveLOG(const std::string &savePath, int index,
		const ArenaTypes::Log &data);
	static void saveRE(const std::string &savePath, int number, int index,
		const ArenaTypes::Repair &data);
	static void saveSAVEENGN(const std::string &savePath, int index,
		const ArenaTypes::SaveEngine &data);
	static void saveSAVEGAME(const std::string &savePath, int index,
		const ArenaTypes::SaveGame &data);
	static void saveSPELLS(const std::string &savePath, int index,
		const ArenaTypes::Spells &data);
	static void saveSPELLSG(const std::string &savePath, int index,
		const ArenaTypes::Spellsg &data);
	static void saveSTATE(const std::string &savePath, int index,
		const ArenaTypes::MQLevelState &data);
	// To do: save city data.
	// To do: save INN.0x.
	// To do: save wild 001, 002, 003, 004.
};

#endif
