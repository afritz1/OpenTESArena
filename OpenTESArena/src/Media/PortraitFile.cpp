#include <cassert>
#include <cstdint>
#include <map>

#include "PortraitFile.h"

#include "../Entities/CharacterGenderName.h"
#include "../Entities/CharacterRaceName.h"
#include "../Utilities/Debug.h"

const std::map<CharacterGenderName, std::string> PortraitFileGenderNames =
{
	{ CharacterGenderName::Female, "female" },
	{ CharacterGenderName::Male, "male" }
};

const std::map<CharacterRaceName, std::string> PortraitFileRaceNames =
{
	{ CharacterRaceName::Argonian, "argonian" },
	{ CharacterRaceName::Breton, "breton" },
	{ CharacterRaceName::DarkElf, "dark_elf" },
	{ CharacterRaceName::HighElf, "high_elf" },
	{ CharacterRaceName::Imperial, "imperial" },
	{ CharacterRaceName::Khajiit, "khajiit" },
	{ CharacterRaceName::Nord, "nord" },
	{ CharacterRaceName::Redguard, "redguard" },
	{ CharacterRaceName::WoodElf,  "wood_elf" }
};

const std::map<bool, std::string> PortraitFileMagicNames =
{
	{ false, "non_magic" },
	{ true, "magic" }
};

// This path might be obsolete soon.
const std::string PortraitFile::PATH = "interface/character_images/";

std::vector<std::string> PortraitFile::getGroup(CharacterGenderName gender, 
	CharacterRaceName race, bool isMagic)
{
	// Remove this once the Imperial race pictures are added.
	Debug::check(race != CharacterRaceName::Imperial, "Portrait File", 
		"Imperial race portraits not implemented.");

	const auto &genderString = PortraitFileGenderNames.at(gender);
	const auto &raceString = PortraitFileRaceNames.at(race);
	const auto &magicString = PortraitFileMagicNames.at(isMagic);

	std::string fileString = PortraitFile::PATH + raceString + "/" + raceString + "_" + 
		genderString + "_" + magicString + "_";

	std::vector<std::string> fileStrings;

	// Currently only ten images can be used for each triplet of options.
	for (int32_t i = 0; i < 10; ++i)
	{
		fileStrings.push_back(fileString + std::to_string(i + 1));
	}
	
	return fileStrings;
}
