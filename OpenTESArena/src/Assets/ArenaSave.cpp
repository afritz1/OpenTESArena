#include "ArenaSave.h"
#include "../Utilities/Debug.h"
#include "../Utilities/File.h"

ArenaTypes::Automap ArenaSave::loadAUTOMAP(const std::string &filename, int index)
{
	ArenaTypes::Automap automap;
	DebugNotImplemented();
	return automap;
}

ArenaTypes::Log ArenaSave::loadLOG(const std::string &filename, int index)
{
	ArenaTypes::Log log;
	DebugNotImplemented();
	return log;
}

ArenaTypes::SaveEngine ArenaSave::loadSAVEENGN(const std::string &filename, int index)
{
	ArenaTypes::SaveEngine saveEngine;
	DebugNotImplemented();
	return saveEngine;
}

ArenaTypes::SaveGame ArenaSave::loadSAVEGAME(const std::string &filename, int index)
{
	ArenaTypes::SaveGame saveGame;
	DebugNotImplemented();
	return saveGame;
}

ArenaTypes::Spells ArenaSave::loadSPELLS(const std::string &filename, int index)
{
	ArenaTypes::Spells spells;
	DebugNotImplemented();
	return spells;
}

ArenaTypes::Spells ArenaSave::loadSPELLSG(const std::string &filename, int index)
{
	ArenaTypes::Spells spells;
	DebugNotImplemented();
	return spells;
}

ArenaTypes::MQLevelState ArenaSave::loadSTATE(const std::string &filename, int index)
{
	ArenaTypes::MQLevelState state;
	DebugNotImplemented();
	return state;
}

void ArenaSave::saveAUTOMAP(const std::string &filename, int index,
	const ArenaTypes::Automap &data)
{
	DebugNotImplemented();
}

void ArenaSave::saveLOG(const std::string &filename, int index,
	const ArenaTypes::Log &data)
{
	DebugNotImplemented();
}

void ArenaSave::saveSAVEENGN(const std::string &filename, int index,
	const ArenaTypes::SaveEngine &data)
{
	DebugNotImplemented();
}

void ArenaSave::saveSAVEGAME(const std::string &filename, int index,
	const ArenaTypes::SaveGame &data)
{
	DebugNotImplemented();
}

void ArenaSave::saveSPELLS(const std::string &filename, int index,
	const ArenaTypes::Spells &data)
{
	DebugNotImplemented();
}

void ArenaSave::saveSPELLSG(const std::string &filename, int index,
	const ArenaTypes::Spells &data)
{
	DebugNotImplemented();
}

void ArenaSave::saveSTATE(const std::string &filename, int index,
	const ArenaTypes::MQLevelState &data)
{
	DebugNotImplemented();
}
