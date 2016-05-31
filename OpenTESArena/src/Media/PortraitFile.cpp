#include <cassert>
#include <map>

#include "PortraitFile.h"

#include "../Entities/CharacterGenderName.h"
#include "../Entities/CharacterRaceName.h"
#include "../Utilities/Debug.h"

const auto PortraitFileGenderNames = std::map<CharacterGenderName, std::string>
{
	{ CharacterGenderName::Female, "female" },
	{ CharacterGenderName::Male, "male" }
};

const auto PortraitFileRaceNames = std::map<CharacterRaceName, std::string>
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

const auto PortraitFileMagicNames = std::map<bool, std::string>
{
	{ false, "non_magic" },
	{ true, "magic" }
};

const std::string PortraitFile::PATH = "interface/character_images/";

std::vector<std::string> PortraitFile::getGroup(CharacterGenderName gender, 
	CharacterRaceName race, bool isMagic)
{
	// Remove this once the Imperial race pictures are added.
	Debug::check(race != CharacterRaceName::Imperial, "Portrait File", 
		"Imperial race portraits not implemented.");

	auto genderString = PortraitFileGenderNames.at(gender);
	auto raceString = PortraitFileRaceNames.at(race);
	auto magicString = PortraitFileMagicNames.at(isMagic);

	auto fileString = PortraitFile::PATH + raceString + "/" + raceString + "_" + 
		genderString + "_" + magicString + "_";
	auto fileStrings = std::vector<std::string>();

	// Currently only ten images can be used for each triplet of options.
	for (int i = 0; i < 10; ++i)
	{
		fileStrings.push_back(fileString + std::to_string(i + 1));
	}
	
	return fileStrings;
}
