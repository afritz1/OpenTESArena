#include <cassert>
#include <map>

#include "CharacterRace.h"

#include "CharacterRaceName.h"
#include "../World/ProvinceName.h"

const std::map<CharacterRaceName, std::string> CharacterRaceDisplayNames =
{
	{ CharacterRaceName::Argonian, "Argonian" },
	{ CharacterRaceName::Breton, "Breton" },
	{ CharacterRaceName::DarkElf, "Dark Elf" },
	{ CharacterRaceName::HighElf, "High Elf" },
	{ CharacterRaceName::Imperial, "Imperial" },
	{ CharacterRaceName::Khajiit, "Khajiit" },
	{ CharacterRaceName::Nord, "Nord" },
	{ CharacterRaceName::Redguard, "Redguard" },
	{ CharacterRaceName::WoodElf, "Wood Elf" }
};

const std::map<CharacterRaceName, ProvinceName> CharacterRaceHomeProvinces =
{
	{ CharacterRaceName::Argonian, ProvinceName::BlackMarsh },
	{ CharacterRaceName::Breton, ProvinceName::HighRock },
	{ CharacterRaceName::DarkElf, ProvinceName::Morrowind },
	{ CharacterRaceName::HighElf, ProvinceName::SummersetIsle },
	{ CharacterRaceName::Imperial, ProvinceName::ImperialProvince },
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

CharacterRaceName CharacterRace::getRaceName() const
{
	return this->raceName;
}

ProvinceName CharacterRace::getHomeProvinceName() const
{
	auto homeProvince = CharacterRaceHomeProvinces.at(this->getRaceName());
	return homeProvince;
}

std::string CharacterRace::toString() const
{
	auto displayName = CharacterRaceDisplayNames.at(this->getRaceName());
	return displayName;
}
