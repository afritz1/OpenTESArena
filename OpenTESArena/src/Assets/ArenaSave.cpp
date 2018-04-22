#include <iomanip>
#include <sstream>

#include "ArenaSave.h"
#include "../Utilities/Debug.h"
#include "../Utilities/File.h"

const std::string ArenaSave::AUTOMAP_FILENAME = "AUTOMAP";
const std::string ArenaSave::LOG_FILENAME = "LOG";
const std::string ArenaSave::SAVEENGN_FILENAME = "SAVEENGN";
const std::string ArenaSave::SAVEGAME_FILENAME = "SAVEGAME";
const std::string ArenaSave::SPELLS_FILENAME = "SPELLS";
const std::string ArenaSave::SPELLSG_FILENAME = "SPELLSG";
const std::string ArenaSave::STATE_FILENAME = "STATE";

std::string ArenaSave::makeSaveExtension(int index)
{
	std::stringstream ss;
	ss << '.' << std::setw(2) << std::setfill('0') << index;
	return ss.str();
}

ArenaTypes::Automap ArenaSave::loadAUTOMAP(const std::string &savePath, int index)
{
	const std::string filename = savePath + ArenaSave::AUTOMAP_FILENAME +
		ArenaSave::makeSaveExtension(index);

	ArenaTypes::Automap automap;
	DebugNotImplemented();
	return automap;
}

ArenaTypes::Log ArenaSave::loadLOG(const std::string &savePath, int index)
{
	const std::string filename = savePath + ArenaSave::LOG_FILENAME +
		ArenaSave::makeSaveExtension(index);

	ArenaTypes::Log log;
	DebugNotImplemented();
	return log;
}

ArenaTypes::SaveEngine ArenaSave::loadSAVEENGN(const std::string &savePath, int index)
{
	const std::string filename = savePath + ArenaSave::SAVEENGN_FILENAME +
		ArenaSave::makeSaveExtension(index);

	ArenaTypes::SaveEngine saveEngine;
	DebugNotImplemented();
	return saveEngine;
}

ArenaTypes::SaveGame ArenaSave::loadSAVEGAME(const std::string &savePath, int index)
{
	const std::string filename = savePath + ArenaSave::SAVEGAME_FILENAME +
		ArenaSave::makeSaveExtension(index);

	ArenaTypes::SaveGame saveGame;
	DebugNotImplemented();
	return saveGame;
}

ArenaTypes::Spells ArenaSave::loadSPELLS(const std::string &savePath, int index)
{
	const std::string filename = savePath + ArenaSave::SPELLS_FILENAME +
		ArenaSave::makeSaveExtension(index);

	ArenaTypes::Spells spells;
	DebugNotImplemented();
	return spells;
}

ArenaTypes::Spellsg ArenaSave::loadSPELLSG(const std::string &savePath, int index)
{
	const std::string filename = savePath + ArenaSave::SPELLSG_FILENAME +
		ArenaSave::makeSaveExtension(index);

	ArenaTypes::Spellsg spellsg;
	DebugNotImplemented();
	return spellsg;
}

ArenaTypes::MQLevelState ArenaSave::loadSTATE(const std::string &savePath, int index)
{
	const std::string filename = savePath + ArenaSave::STATE_FILENAME +
		ArenaSave::makeSaveExtension(index);

	ArenaTypes::MQLevelState state;
	DebugNotImplemented();
	return state;
}

void ArenaSave::saveAUTOMAP(const std::string &savePath, int index,
	const ArenaTypes::Automap &data)
{
	const std::string filename = savePath + ArenaSave::AUTOMAP_FILENAME +
		ArenaSave::makeSaveExtension(index);

	DebugNotImplemented();
}

void ArenaSave::saveLOG(const std::string &savePath, int index,
	const ArenaTypes::Log &data)
{
	const std::string filename = savePath + ArenaSave::LOG_FILENAME +
		ArenaSave::makeSaveExtension(index);

	DebugNotImplemented();
}

void ArenaSave::saveSAVEENGN(const std::string &savePath, int index,
	const ArenaTypes::SaveEngine &data)
{
	const std::string filename = savePath + ArenaSave::SAVEENGN_FILENAME +
		ArenaSave::makeSaveExtension(index);

	DebugNotImplemented();
}

void ArenaSave::saveSAVEGAME(const std::string &savePath, int index,
	const ArenaTypes::SaveGame &data)
{
	const std::string filename = savePath + ArenaSave::SAVEGAME_FILENAME +
		ArenaSave::makeSaveExtension(index);

	DebugNotImplemented();
}

void ArenaSave::saveSPELLS(const std::string &savePath, int index,
	const ArenaTypes::Spells &data)
{
	const std::string filename = savePath + ArenaSave::SPELLS_FILENAME +
		ArenaSave::makeSaveExtension(index);

	DebugNotImplemented();
}

void ArenaSave::saveSPELLSG(const std::string &savePath, int index,
	const ArenaTypes::Spellsg &data)
{
	const std::string filename = savePath + ArenaSave::SPELLSG_FILENAME +
		ArenaSave::makeSaveExtension(index);

	DebugNotImplemented();
}

void ArenaSave::saveSTATE(const std::string &savePath, int index,
	const ArenaTypes::MQLevelState &data)
{
	const std::string filename = savePath + ArenaSave::STATE_FILENAME +
		ArenaSave::makeSaveExtension(index);

	DebugNotImplemented();
}
