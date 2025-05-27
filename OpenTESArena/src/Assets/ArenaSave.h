#ifndef ARENA_SAVE_H
#define ARENA_SAVE_H

#include <memory>
#include <string>

#include "ArenaTypes.h"

// Loads various files saved by the original game.
namespace ArenaSave
{
	// Using heap-allocated pointers to avoid stack overflow warnings.
	std::unique_ptr<ArenaTypes::Automap> loadAUTOMAP(const std::string &savePath, int index);
	std::unique_ptr<ArenaTypes::Tavern> loadIN(const std::string &savePath, int number, int index);
	std::unique_ptr<ArenaTypes::Log> loadLOG(const std::string &savePath, int index);
	std::unique_ptr<ArenaTypes::Names> loadNAMES(const std::string &savePath);
	std::unique_ptr<ArenaTypes::Repair> loadRE(const std::string &savePath, int number, int index);
	std::unique_ptr<ArenaTypes::SaveEngine> loadSAVEENGN(const std::string &savePath, int index);
	std::unique_ptr<ArenaTypes::SaveGame> loadSAVEGAME(const std::string &savePath, int index);
	std::unique_ptr<ArenaTypes::Spells> loadSPELLS(const std::string &savePath, int index);
	std::unique_ptr<ArenaTypes::Spellsg> loadSPELLSG(const std::string &savePath, int index);
	std::unique_ptr<ArenaTypes::MQLevelState> loadSTATE(const std::string &savePath, int index);
	// @todo: load city data.
	// @todo: load INN.0x.
	// @todo: load wild 001, 002, 003, 004.

	void saveAUTOMAP(const std::string &savePath, int index, const ArenaTypes::Automap &data);
	void saveIN(const std::string &savePath, int number, int index, const ArenaTypes::Tavern &data);
	void saveLOG(const std::string &savePath, int index, const ArenaTypes::Log &data);
	void saveRE(const std::string &savePath, int number, int index, const ArenaTypes::Repair &data);
	void saveSAVEENGN(const std::string &savePath, int index, const ArenaTypes::SaveEngine &data);
	void saveSAVEGAME(const std::string &savePath, int index, const ArenaTypes::SaveGame &data);
	void saveSPELLS(const std::string &savePath, int index, const ArenaTypes::Spells &data);
	void saveSPELLSG(const std::string &savePath, int index, const ArenaTypes::Spellsg &data);
	void saveSTATE(const std::string &savePath, int index, const ArenaTypes::MQLevelState &data);
	// @todo: save city data.
	// @todo: save INN.0x.
	// @todo: save wild 001, 002, 003, 004.
}

#endif
