#include <cassert>
#include <map>

#include "Province.h"

#include "../Entities/CharacterRaceName.h"

const auto ProvinceDisplayNames = std::map<ProvinceName, std::string>
{
	{ ProvinceName::BlackMarsh, "Black Marsh" },
	{ ProvinceName::Elsweyr, "Elsweyr" },
	{ ProvinceName::Hammerfell, "Hammerfell" },
	{ ProvinceName::HighRock, "High Rock" },
	{ ProvinceName::ImperialProvince, "Imperial Province" },
	{ ProvinceName::Morrowind, "Morrowind" },
	{ ProvinceName::Skyrim, "Skyrim" },
	{ ProvinceName::SummersetIsle, "Summerset Isle" },
	{ ProvinceName::Valenwood, "Valenwood" }
};

const auto ProvinceSingularRaceDisplayNames = std::map<ProvinceName, std::string>
{
	{ ProvinceName::BlackMarsh, "Argonian" },
	{ ProvinceName::Elsweyr, "Khajiit" },
	{ ProvinceName::Hammerfell, "Redguard" },
	{ ProvinceName::HighRock, "Breton" },
	{ ProvinceName::ImperialProvince, "Imperial" },
	{ ProvinceName::Morrowind, "Dark Elf" },
	{ ProvinceName::Skyrim, "Nord" },
	{ ProvinceName::SummersetIsle, "High Elf" },
	{ ProvinceName::Valenwood, "Wood Elf" }
};

const auto ProvincePluralRaceDisplayNames = std::map<ProvinceName, std::string>
{
	{ ProvinceName::BlackMarsh, "Argonians" },
	{ ProvinceName::Elsweyr, "Khajiit" },
	{ ProvinceName::Hammerfell, "Redguards" },
	{ ProvinceName::HighRock, "Bretons" },
	{ ProvinceName::ImperialProvince, "Imperials" },
	{ ProvinceName::Morrowind, "Dark Elves" },
	{ ProvinceName::Skyrim, "Nords" },
	{ ProvinceName::SummersetIsle, "High Elves" },
	{ ProvinceName::Valenwood, "Wood Elves" }
};

const auto ProvinceRaceNames = std::map<ProvinceName, CharacterRaceName>
{
	{ ProvinceName::BlackMarsh, CharacterRaceName::Argonian },
	{ ProvinceName::Elsweyr, CharacterRaceName::Khajiit },
	{ ProvinceName::Hammerfell, CharacterRaceName::Redguard },
	{ ProvinceName::HighRock, CharacterRaceName::Breton },
	{ ProvinceName::ImperialProvince, CharacterRaceName::Imperial },
	{ ProvinceName::Morrowind, CharacterRaceName::DarkElf },
	{ ProvinceName::Skyrim, CharacterRaceName::Nord },
	{ ProvinceName::SummersetIsle, CharacterRaceName::HighElf },
	{ ProvinceName::Valenwood, CharacterRaceName::WoodElf }
};

Province::Province(ProvinceName provinceName)
{
	this->provinceName = provinceName;
}

Province::~Province()
{

}

const ProvinceName &Province::getProvinceName() const
{
	return this->provinceName;
}

const std::vector<Location> &Province::getLocations() const
{
	return this->locations;
}

std::string Province::toString() const
{
	auto displayName = ProvinceDisplayNames.at(this->getProvinceName());
	assert(displayName.size() > 0);
	return displayName;
}

std::string Province::getRaceDisplayName(bool plural) const
{
	auto raceDisplayName = plural ?
		ProvincePluralRaceDisplayNames.at(this->getProvinceName()) :
		ProvinceSingularRaceDisplayNames.at(this->getProvinceName());
	assert(raceDisplayName.size() > 0);
	return raceDisplayName;
}

CharacterRaceName Province::getRaceName() const
{
	auto raceName = ProvinceRaceNames.at(this->getProvinceName());
	return raceName;
}

void Province::addLocation(const Location &location)
{
	this->locations.push_back(location);
}
