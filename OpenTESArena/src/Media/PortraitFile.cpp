#include <array>
#include <cstdio>

#include "PortraitFile.h"
#include "../Assets/ArenaTextureName.h"

#include "components/dos/DOSUtils.h"

std::string PortraitFile::getHeads(bool male, int raceID, bool trimmed)
{
	DOSUtils::FilenameBuffer filename;
	std::snprintf(filename.data(), filename.size(), "FACES%s%d%d.CIF",
		male ? "" : "F", trimmed ? 0 : 1, raceID);

	return std::string(filename.data());
}

std::string PortraitFile::getBody(bool male, int raceID)
{
	DOSUtils::FilenameBuffer filename;
	std::snprintf(filename.data(), filename.size(), "%s0%d.IMG",
		male ? "CHARBK" : "CHRBKF", raceID);

	return std::string(filename.data());
}

const std::string &PortraitFile::getShirt(bool male, bool magic)
{
	if (male)
	{
		return magic ? ArenaTextureName::MaleMagicShirt : ArenaTextureName::MaleNonMagicShirt;
	}
	else
	{
		return magic ? ArenaTextureName::FemaleMagicShirt : ArenaTextureName::FemaleNonMagicShirt;
	}
}

const std::string &PortraitFile::getPants(bool male)
{
	return male ? ArenaTextureName::MalePants : ArenaTextureName::FemalePants;
}

const std::string &PortraitFile::getEquipment(bool male)
{
	return male ? ArenaTextureName::MaleEquipment : ArenaTextureName::FemaleEquipment;
}

Int2 PortraitFile::getShirtOffset(bool male, bool magic)
{
	if (male)
	{
		return magic ? Int2(215, 35) : Int2(186, 12);
	}
	else
	{
		return magic ? Int2(220, 33) : Int2(220, 35);
	}
}

Int2 PortraitFile::getPantsOffset(bool male)
{
	return male ? Int2(229, 82) : Int2(212, 74);
}
