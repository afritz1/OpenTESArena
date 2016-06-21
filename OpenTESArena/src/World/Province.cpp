#include <cassert>
#include <map>

#include "Province.h"

#include "ProvinceName.h"
#include "../Entities/CharacterRaceName.h"
#include "../Math/Rect.h"

namespace
{
	const std::map<ProvinceName, std::string> ProvinceDisplayNames =
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

	const std::map<ProvinceName, std::string> ProvinceSingularRaceDisplayNames =
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

	const std::map<ProvinceName, std::string> ProvincePluralRaceDisplayNames =
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

	const std::map<ProvinceName, CharacterRaceName> ProvinceRaceNames =
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

	// Mouse click areas for the world map.
	const std::map<ProvinceName, Rect> ProvinceClickAreas =
	{
		{ ProvinceName::BlackMarsh, Rect(216, 144, 55, 12) },
		{ ProvinceName::Elsweyr, Rect(148, 127, 37, 11) },
		{ ProvinceName::Hammerfell, Rect(72, 75, 50, 11) },
		{ ProvinceName::HighRock, Rect(52, 51, 44, 11) },
		{ ProvinceName::ImperialProvince, Rect(133, 105, 83, 11) },
		{ ProvinceName::Morrowind, Rect(222, 84, 52, 11) },
		{ ProvinceName::Skyrim, Rect(142, 44, 34, 11) },
		{ ProvinceName::SummersetIsle, Rect(37, 149, 49, 19) },
		{ ProvinceName::Valenwood, Rect(106, 147, 49, 10) }
	};
}

Province::Province(ProvinceName provinceName)
{
	this->locations = std::vector<Location>();
	this->provinceName = provinceName;
}

Province::~Province()
{

}

std::vector<ProvinceName> Province::getAllProvinceNames()
{
	std::vector<ProvinceName> provinceNames;

	for (const auto &item : ProvinceDisplayNames)
	{
		provinceNames.push_back(item.first);
	}

	return provinceNames;
}

ProvinceName Province::getProvinceName() const
{
	return this->provinceName;
}

CharacterRaceName Province::getRaceName() const
{
	auto raceName = ProvinceRaceNames.at(this->getProvinceName());
	return raceName;
}

const Rect &Province::getWorldMapClickArea() const
{
	const Rect &provinceArea = ProvinceClickAreas.at(this->getProvinceName());
	return provinceArea;
}

const std::vector<Location> &Province::getLocations() const
{
	return this->locations;
}

std::string Province::toString() const
{
	auto displayName = ProvinceDisplayNames.at(this->getProvinceName());
	return displayName;
}

std::string Province::getRaceDisplayName(bool plural) const
{
	auto raceDisplayName = plural ?
		ProvincePluralRaceDisplayNames.at(this->getProvinceName()) :
		ProvinceSingularRaceDisplayNames.at(this->getProvinceName());
	return raceDisplayName;
}

void Province::addLocation(const Location &location)
{
	this->locations.push_back(location);
}
