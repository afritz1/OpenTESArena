#include <fstream>
#include <iomanip>
#include <sstream>

#include "ArenaSave.h"
#include "../Utilities/Debug.h"

namespace
{
	// Makes a numbered extension for the given save index.
	std::string makeSaveExtension(int index)
	{
		std::stringstream ss;
		ss << '.' << std::setw(2) << std::setfill('0') << index;
		return ss.str();
	}

	// Convenience function for loading a binary save file and returning the initialized record.
	template <typename T>
	T loadBinary(const std::string &filename)
	{
		std::ifstream ifs(filename, std::ios::binary);

		if (ifs.is_open())
		{
			std::array<uint8_t, T::SIZE> buffer;
			ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

			T obj;
			obj.init(buffer.data());
			return obj;
		}
		else
		{
			throw DebugException("\"" + filename + "\" not found.");
		}
	}
}

const std::string ArenaSave::AUTOMAP_FILENAME = "AUTOMAP";
const std::string ArenaSave::IN_FILENAME = "IN";
const std::string ArenaSave::LOG_FILENAME = "LOG";
const std::string ArenaSave::NAMES_FILENAME = "NAMES.DAT";
const std::string ArenaSave::RE_FILENAME = "RE";
const std::string ArenaSave::SAVEENGN_FILENAME = "SAVEENGN";
const std::string ArenaSave::SAVEGAME_FILENAME = "SAVEGAME";
const std::string ArenaSave::SPELLS_FILENAME = "SPELLS";
const std::string ArenaSave::SPELLSG_FILENAME = "SPELLSG";
const std::string ArenaSave::STATE_FILENAME = "STATE";

ArenaTypes::Automap ArenaSave::loadAUTOMAP(const std::string &savePath, int index)
{
	return loadBinary<ArenaTypes::Automap>(
		savePath + ArenaSave::AUTOMAP_FILENAME + makeSaveExtension(index));
}

ArenaTypes::Tavern ArenaSave::loadIN(const std::string &savePath, int number, int index)
{
	const std::string innName = ArenaSave::IN_FILENAME + std::to_string(number);
	return loadBinary<ArenaTypes::Tavern>(savePath + innName + makeSaveExtension(index));
}

ArenaTypes::Log ArenaSave::loadLOG(const std::string &savePath, int index)
{
	const std::string filename = savePath +
		ArenaSave::LOG_FILENAME + makeSaveExtension(index);
	std::ifstream ifs(filename);

	if (ifs.is_open())
	{
		ifs.seekg(0, std::ios::end);
		std::string buffer(ifs.tellg(), '\0');
		ifs.seekg(0, std::ios::beg);
		ifs.read(&buffer.front(), buffer.size());

		ArenaTypes::Log log;
		log.init(buffer);
		return log;
	}
	else
	{
		throw DebugException("\"" + filename + "\" not found.");
	}
}

ArenaTypes::Names ArenaSave::loadNAMES(const std::string &savePath)
{
	return loadBinary<ArenaTypes::Names>(savePath + ArenaSave::NAMES_FILENAME);
}

ArenaTypes::Repair ArenaSave::loadRE(const std::string &savePath, int number, int index)
{
	const std::string repairName = ArenaSave::RE_FILENAME + std::to_string(number);
	return loadBinary<ArenaTypes::Repair>(savePath + repairName + makeSaveExtension(index));
}

ArenaTypes::SaveEngine ArenaSave::loadSAVEENGN(const std::string &savePath, int index)
{
	return loadBinary<ArenaTypes::SaveEngine>(
		savePath + ArenaSave::SAVEENGN_FILENAME + makeSaveExtension(index));
}

ArenaTypes::SaveGame ArenaSave::loadSAVEGAME(const std::string &savePath, int index)
{
	return loadBinary<ArenaTypes::SaveGame>(
		savePath + ArenaSave::SAVEGAME_FILENAME + makeSaveExtension(index));
}

ArenaTypes::Spells ArenaSave::loadSPELLS(const std::string &savePath, int index)
{
	const std::string filename = savePath +
		ArenaSave::SPELLS_FILENAME + makeSaveExtension(index);
	std::ifstream ifs(filename, std::ios::binary);

	if (ifs.is_open())
	{
		ArenaTypes::Spells spells;

		std::array<uint8_t, ArenaTypes::SpellData::SIZE * spells.size()> buffer;
		ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

		for (size_t i = 0; i < spells.size(); i++)
		{
			spells.at(i).init(buffer.data() + (ArenaTypes::SpellData::SIZE * i));
		}

		return spells;
	}
	else
	{
		throw DebugException("\"" + filename + "\" not found.");
	}
}

ArenaTypes::Spellsg ArenaSave::loadSPELLSG(const std::string &savePath, int index)
{
	const std::string filename = savePath +
		ArenaSave::SPELLSG_FILENAME + makeSaveExtension(index);
	std::ifstream ifs(filename, std::ios::binary);

	if (ifs.is_open())
	{
		ArenaTypes::Spellsg spellsg;

		std::array<uint8_t, ArenaTypes::SpellData::SIZE * spellsg.size()> buffer;
		ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

		for (size_t i = 0; i < spellsg.size(); i++)
		{
			spellsg.at(i).init(buffer.data() + (ArenaTypes::SpellData::SIZE * i));
		}

		return spellsg;
	}
	else
	{
		throw DebugException("\"" + filename + "\" not found.");
	}
}

ArenaTypes::MQLevelState ArenaSave::loadSTATE(const std::string &savePath, int index)
{
	return loadBinary<ArenaTypes::MQLevelState>(
		savePath + ArenaSave::STATE_FILENAME + makeSaveExtension(index));
}

void ArenaSave::saveAUTOMAP(const std::string &savePath, int index,
	const ArenaTypes::Automap &data)
{
	const std::string filename = savePath +
		ArenaSave::AUTOMAP_FILENAME + makeSaveExtension(index);

	DebugNotImplemented();
}

void ArenaSave::saveIN(const std::string &savePath, int number, int index,
	const ArenaTypes::Tavern &data)
{
	const std::string filename = savePath + ArenaSave::IN_FILENAME +
		std::to_string(number) + makeSaveExtension(index);

	DebugNotImplemented();
}

void ArenaSave::saveLOG(const std::string &savePath, int index,
	const ArenaTypes::Log &data)
{
	const std::string filename = savePath +
		ArenaSave::LOG_FILENAME + makeSaveExtension(index);

	DebugNotImplemented();
}

void ArenaSave::saveRE(const std::string &savePath, int number, int index,
	const ArenaTypes::Repair &data)
{
	const std::string filename = savePath + ArenaSave::RE_FILENAME +
		std::to_string(number) + makeSaveExtension(index);

	DebugNotImplemented();
}

void ArenaSave::saveSAVEENGN(const std::string &savePath, int index,
	const ArenaTypes::SaveEngine &data)
{
	const std::string filename = savePath +
		ArenaSave::SAVEENGN_FILENAME + makeSaveExtension(index);

	DebugNotImplemented();
}

void ArenaSave::saveSAVEGAME(const std::string &savePath, int index,
	const ArenaTypes::SaveGame &data)
{
	const std::string filename = savePath +
		ArenaSave::SAVEGAME_FILENAME + makeSaveExtension(index);

	DebugNotImplemented();
}

void ArenaSave::saveSPELLS(const std::string &savePath, int index,
	const ArenaTypes::Spells &data)
{
	const std::string filename = savePath +
		ArenaSave::SPELLS_FILENAME + makeSaveExtension(index);

	DebugNotImplemented();
}

void ArenaSave::saveSPELLSG(const std::string &savePath, int index,
	const ArenaTypes::Spellsg &data)
{
	const std::string filename = savePath +
		ArenaSave::SPELLSG_FILENAME + makeSaveExtension(index);

	DebugNotImplemented();
}

void ArenaSave::saveSTATE(const std::string &savePath, int index,
	const ArenaTypes::MQLevelState &data)
{
	const std::string filename = savePath +
		ArenaSave::STATE_FILENAME + makeSaveExtension(index);

	DebugNotImplemented();
}
