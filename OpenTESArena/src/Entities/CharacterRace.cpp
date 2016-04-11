#include <cassert>
#include <map>

#include "CharacterRace.h"

#include "../World/ProvinceName.h"

const auto CharacterRaceDisplayNames = std::map<CharacterRaceName, std::string>
{
	{ CharacterRaceName::Argonian, "Argonian" },
	{ CharacterRaceName::Breton, "Breton" },
	{ CharacterRaceName::DarkElf, "Dark Elf" },
	{ CharacterRaceName::HighElf, "High Elf" },
	{ CharacterRaceName::Khajiit, "Khajiit" },
	{ CharacterRaceName::Nord, "Nord" },
	{ CharacterRaceName::Redguard, "Redguard" },
	{ CharacterRaceName::WoodElf, "Wood Elf" }
};

const auto CharacterRaceHomeProvinces = std::map<CharacterRaceName, ProvinceName>
{
	{ CharacterRaceName::Argonian, ProvinceName::BlackMarsh },
	{ CharacterRaceName::Breton, ProvinceName::HighRock },
	{ CharacterRaceName::DarkElf, ProvinceName::Morrowind },
	{ CharacterRaceName::HighElf, ProvinceName::SummersetIsle },
	{ CharacterRaceName::Khajiit, ProvinceName::Elsweyr },
	{ CharacterRaceName::Nord, ProvinceName::Skyrim },
	{ CharacterRaceName::Redguard, ProvinceName::Hammerfell },
	{ CharacterRaceName::WoodElf, ProvinceName::Valenwood }
};

CharacterRace::CharacterRace(CharacterRaceName raceName)
{
	this->raceName = raceName;
}

CharacterRace::~CharacterRace()
{

}

const CharacterRaceName &CharacterRace::getRaceName() const
{
	return this->raceName;
}

std::string CharacterRace::toString() const
{
	auto displayName = CharacterRaceDisplayNames.at(this->getRaceName());
	assert(displayName.size() > 0);
	return displayName;
}

ProvinceName CharacterRace::getHomeProvinceName() const
{
	auto homeProvince = CharacterRaceHomeProvinces.at(this->getRaceName());
	return homeProvince;
}
