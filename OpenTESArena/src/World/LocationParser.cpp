#include <cassert>
#include <map>
#include <sstream>

#include "LocationParser.h"

#include "ClimateName.h"
#include "Location.h"
#include "LocationType.h"
#include "ProvinceName.h"
#include "../Utilities/Debug.h"
#include "../Utilities/File.h"

// Text file string mappings. These strings are not considered display names.
const auto LocationParserProvinces = std::map<std::string, ProvinceName>
{
	{ "BlackMarsh", ProvinceName::BlackMarsh },
	{ "Elsweyr", ProvinceName::Elsweyr },
	{ "Hammerfell", ProvinceName::Hammerfell },
	{ "HighRock", ProvinceName::HighRock },
	{ "ImperialProvince", ProvinceName::ImperialProvince },
	{ "Morrowind", ProvinceName::Morrowind },
	{ "Skyrim", ProvinceName::Skyrim },
	{ "SummersetIsle", ProvinceName::SummersetIsle },
	{ "Valenwood", ProvinceName::Valenwood }
};

const auto LocationParserTypes = std::map<std::string, LocationType>
{
	{ "CityState", LocationType::CityState },
	{ "Dungeon", LocationType::Dungeon },
	{ "Town", LocationType::Town },
	{ "Unique", LocationType::Unique },
	{ "Village", LocationType::Village }
};

const auto LocationParserClimates = std::map<std::string, ClimateName>
{
	{ "Cold", ClimateName::Cold },
	{ "Desert", ClimateName::Desert },
	{ "Grassy", ClimateName::Grassy },
	{ "Interior", ClimateName::Interior },
	{ "Snowy", ClimateName::Snowy }
};

const std::string LocationParser::PATH = "data/text/";
const std::string LocationParser::FILENAME = "locations.txt";

std::vector<std::unique_ptr<Location>> LocationParser::parse()
{
	// This parser is very simple right now. All text must have the exact amount
	// of spacing and commas, and there must be a new line at the end of the file.
	// Comment lines must have the comment symbol in the first column.

	auto fullPath = LocationParser::PATH + LocationParser::FILENAME;

	// Read the locations file into a string.
	auto text = File::toString(fullPath);

	// Relevant parsing symbols.
	const char comment = '#';
	const char comma = ',';

	auto locations = std::vector<std::unique_ptr<Location>>();
	std::istringstream iss;
	iss.str(text);

	auto line = std::string();

	// For each line, get the substrings between commas.
	while (std::getline(iss, line))
	{
		// Ignore comments and blank lines.
		if ((line.at(0) == comment) || (line.at(0) == '\r') || (line.at(0) == '\n'))
		{
			continue;
		}

		// Get the display name.
		int index = 0;
		while (line.at(index) != comma)
		{
			++index;
		}

		auto displayName = line.substr(0, index);

		// Get the province name.
		index += 2;
		int oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		auto province = line.substr(oldIndex, index - oldIndex);

		// Get the location type.
		index += 2;
		oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		auto type = line.substr(oldIndex, index - oldIndex);

		// Get the climate type (read until the end of the line).
		index += 2;
		oldIndex = index;
		while ((line.at(index) != '\r') && (line.at(index) != '\n'))
		{
			++index;
		}

		auto climate = line.substr(oldIndex, index - oldIndex);

		// Verify that the strings each have a mapping.
		Debug::check(LocationParserProvinces.find(province) != LocationParserProvinces.end(),
			"Location Parser", "Invalid province \"" + province + "\".");
		Debug::check(LocationParserTypes.find(type) != LocationParserTypes.end(),
			"Location Parser", "Invalid location type \"" + type + "\".");
		Debug::check(LocationParserClimates.find(climate) != LocationParserClimates.end(),
			"Location Parser", "Invalid climate \"" + climate + "\".");

		// Convert the strings to recognized types.
		auto provinceName = LocationParserProvinces.at(province);
		auto locationType = LocationParserTypes.at(type);
		auto climateName = LocationParserClimates.at(climate);

		locations.push_back(std::unique_ptr<Location>(new Location(
			displayName, provinceName, locationType, climateName)));
	}

	return locations;
}
