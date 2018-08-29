#ifndef ARENA_SAVE_H
#define ARENA_SAVE_H

#include <memory>
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
	// Using heap-allocated pointers to avoid stack overflow warnings.
	static std::unique_ptr<ArenaTypes::Automap> loadAUTOMAP(const std::string &savePath, int index);
	static std::unique_ptr<ArenaTypes::Tavern> loadIN(const std::string &savePath, int number, int index);
	static std::unique_ptr<ArenaTypes::Log> loadLOG(const std::string &savePath, int index);
	static std::unique_ptr<ArenaTypes::Names> loadNAMES(const std::string &savePath);
	static std::unique_ptr<ArenaTypes::Repair> loadRE(const std::string &savePath, int number, int index);
	static std::unique_ptr<ArenaTypes::SaveEngine> loadSAVEENGN(const std::string &savePath, int index);
	static std::unique_ptr<ArenaTypes::SaveGame> loadSAVEGAME(const std::string &savePath, int index);
	static std::unique_ptr<ArenaTypes::Spells> loadSPELLS(const std::string &savePath, int index);
	static std::unique_ptr<ArenaTypes::Spellsg> loadSPELLSG(const std::string &savePath, int index);
	static std::unique_ptr<ArenaTypes::MQLevelState> loadSTATE(const std::string &savePath, int index);
	// @todo: load city data.
	// @todo: load INN.0x.
	// @todo: load wild 001, 002, 003, 004.

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
	// @todo: save city data.
	// @todo: save INN.0x.
	// @todo: save wild 001, 002, 003, 004.
};

#endif
