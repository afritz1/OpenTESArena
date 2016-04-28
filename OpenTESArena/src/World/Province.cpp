#include <cassert>
#include <map>

#include "Province.h"

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

const auto ProvinceSingularRaceNames = std::map<ProvinceName, std::string>
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

const auto ProvincePluralRaceNames = std::map<ProvinceName, std::string>
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

std::string Province::getRaceName(bool plural) const
{
	auto raceName = plural ?
		ProvincePluralRaceNames.at(this->getProvinceName()) :
		ProvinceSingularRaceNames.at(this->getProvinceName());
	assert(raceName.size() > 0);
	return raceName;
}

void Province::addLocation(const Location &location)
{
	this->locations.push_back(location);
}
