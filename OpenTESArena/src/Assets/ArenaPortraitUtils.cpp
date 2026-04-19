#include <array>
#include <cstdio>

#include "ArenaPortraitUtils.h"
#include "../Assets/ArenaTextureName.h"

#include "components/dos/DOSUtils.h"

std::string ArenaPortraitUtils::getHeads(bool male, int raceID, bool trimmed)
{
	char filename[DOSUtils::FilenameBufferSize];
	std::snprintf(filename, sizeof(filename), "FACES%s%d%d.CIF", male ? "" : "F", trimmed ? 0 : 1, raceID);
	return std::string(filename);
}

std::string ArenaPortraitUtils::getBody(bool male, int raceID)
{
	char filename[DOSUtils::FilenameBufferSize];
	std::snprintf(filename, sizeof(filename), "%s0%d.IMG", male ? "CHARBK" : "CHRBKF", raceID);
	return std::string(filename);
}

const std::string &ArenaPortraitUtils::getShirt(bool male, bool magic)
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

const std::string &ArenaPortraitUtils::getPants(bool male)
{
	return male ? ArenaTextureName::MalePants : ArenaTextureName::FemalePants;
}

const std::string &ArenaPortraitUtils::getEquipment(bool male)
{
	return male ? ArenaTextureName::MaleEquipment : ArenaTextureName::FemaleEquipment;
}

Int2 ArenaPortraitUtils::getShirtOffset(bool male, bool magic)
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

Int2 ArenaPortraitUtils::getPantsOffset(bool male)
{
	return male ? Int2(229, 82) : Int2(212, 74);
}
