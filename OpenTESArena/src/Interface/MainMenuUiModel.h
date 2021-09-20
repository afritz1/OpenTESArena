#ifndef MAIN_MENU_UI_MODEL_H
#define MAIN_MENU_UI_MODEL_H

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Assets/ArenaTypes.h"

class ExeData;
class Game;
class ProvinceDefinition;

enum class MapType;

namespace MainMenuUiModel
{
	constexpr int MaxTestTypes = 5;
	constexpr int TestType_MainQuest = 0;
	constexpr int TestType_Interior = 1;
	constexpr int TestType_City = 2;
	constexpr int TestType_Wilderness = 3;
	constexpr int TestType_Dungeon = 4;

	// Main quest locations. There are eight map dungeons and eight staff dungeons.
	// The special cases are the start dungeon and the final dungeon.
	constexpr int MainQuestLocationCount = 18;

	// Small hack for main menu testing.
	enum class SpecialCaseType { None, StartDungeon };

	// Prefixes for some .MIF files, with an inclusive min/max range of ID suffixes.
	// These also need ".MIF" appended at the end.
	const std::vector<std::tuple<std::string, std::pair<int, int>, ArenaTypes::InteriorType>> InteriorLocations =
	{
		{ "BS", { 1, 8 }, ArenaTypes::InteriorType::House },
		{ "EQUIP", { 1, 8 }, ArenaTypes::InteriorType::Equipment },
		{ "MAGE", { 1, 8 }, ArenaTypes::InteriorType::MagesGuild },
		{ "NOBLE", { 1, 8 }, ArenaTypes::InteriorType::Noble },
		{ "PALACE", { 1, 5 }, ArenaTypes::InteriorType::Palace },
		{ "TAVERN", { 1, 8 }, ArenaTypes::InteriorType::Tavern },
		{ "TEMPLE", { 1, 8 }, ArenaTypes::InteriorType::Temple },
		{ "TOWER", { 1, 8 }, ArenaTypes::InteriorType::Tower },
		{ "TOWNPAL", { 1, 3 }, ArenaTypes::InteriorType::Palace },
		{ "VILPAL", { 1, 3 }, ArenaTypes::InteriorType::Palace },
		{ "WCRYPT", { 1, 8 }, ArenaTypes::InteriorType::Crypt }
	};

	const std::string ImperialMIF = "IMPERIAL.MIF";
	const std::string RandomCity = "Random City";
	const std::string RandomTown = "Random Town";
	const std::string RandomVillage = "Random Village";
	const std::vector<std::string> CityLocations =
	{
		ImperialMIF,
		RandomCity,
		RandomTown,
		RandomVillage
	};

	const std::string WildPlaceholderName = "WILD";
	const std::vector<std::string> WildernessLocations =
	{
		WildPlaceholderName
	};

	const std::string RandomNamedDungeon = "Random Named";
	const std::string RandomWildDungeon = "Random Wild";
	const std::vector<std::string> DungeonLocations =
	{
		RandomNamedDungeon,
		RandomWildDungeon
	};

	// Values for testing.
	const std::vector<ArenaTypes::WeatherType> Weathers =
	{
		ArenaTypes::WeatherType::Clear,
		ArenaTypes::WeatherType::Overcast,
		ArenaTypes::WeatherType::Rain,
		ArenaTypes::WeatherType::Snow,
		ArenaTypes::WeatherType::SnowOvercast,
		ArenaTypes::WeatherType::Rain2,
		ArenaTypes::WeatherType::Overcast2,
		ArenaTypes::WeatherType::SnowOvercast2
	};

	const std::unordered_map<ArenaTypes::WeatherType, std::string> WeatherTypeNames =
	{
		{ ArenaTypes::WeatherType::Clear, "Clear" },
		{ ArenaTypes::WeatherType::Overcast, "Overcast" },
		{ ArenaTypes::WeatherType::Rain, "Rain" },
		{ ArenaTypes::WeatherType::Snow, "Snow" },
		{ ArenaTypes::WeatherType::SnowOvercast, "Snow Overcast" },
		{ ArenaTypes::WeatherType::Rain2, "Rain 2" },
		{ ArenaTypes::WeatherType::Overcast2, "Overcast 2" },
		{ ArenaTypes::WeatherType::SnowOvercast2, "Snow Overcast 2" }
	};

	std::string getTestButtonText();
	std::string getTestTypeName(int type);

	std::string getSelectedTestName(Game &game, int testType, int testIndex, int testIndex2);
	std::optional<ArenaTypes::InteriorType> getSelectedTestInteriorType(int testType, int testIndex);
	ArenaTypes::WeatherType getSelectedTestWeatherType(int testWeather);
	MapType getSelectedTestMapType(int testType);

	void getMainQuestLocationFromIndex(int testIndex, const ExeData &exeData,
		int *outLocationID, int *outProvinceID, SpecialCaseType *outSpecialCaseType);
	std::vector<int> makeShuffledLocationIndices(const ProvinceDefinition &provinceDef);
	std::optional<int> getRandomCityLocationDefIndexIfType(const ProvinceDefinition &provinceDef,
		ArenaTypes::CityType cityType);
	int getRandomCityLocationIndex(const ProvinceDefinition &provinceDef);
	std::optional<int> getRandomDungeonLocationDefIndex(const ProvinceDefinition &provinceDef);
}

#endif
