#ifndef MAIN_MENU_UI_MVC_H
#define MAIN_MENU_UI_MVC_H

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Assets/ArenaTypes.h"
#include "../Assets/TextureAsset.h"
#include "../Assets/TextureUtils.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../Utilities/Color.h"

class Game;
class ProvinceDefinition;

enum class MapType;

struct ExeData;

#include <optional>
#include <string>

#include "../Assets/ArenaTypes.h"

class Game;

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
	const std::vector<std::tuple<std::string, std::pair<int, int>, ArenaInteriorType>> InteriorLocations =
	{
		{ "BS", { 1, 8 }, ArenaInteriorType::House },
		{ "EQUIP", { 1, 8 }, ArenaInteriorType::Equipment },
		{ "MAGE", { 1, 8 }, ArenaInteriorType::MagesGuild },
		{ "NOBLE", { 1, 8 }, ArenaInteriorType::Noble },
		{ "PALACE", { 1, 5 }, ArenaInteriorType::Palace },
		{ "TAVERN", { 1, 8 }, ArenaInteriorType::Tavern },
		{ "TEMPLE", { 1, 8 }, ArenaInteriorType::Temple },
		{ "TOWER", { 1, 8 }, ArenaInteriorType::Tower },
		{ "TOWNPAL", { 1, 3 }, ArenaInteriorType::Palace },
		{ "VILPAL", { 1, 3 }, ArenaInteriorType::Palace },
		{ "WCRYPT", { 1, 8 }, ArenaInteriorType::Crypt }
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
	const std::vector<ArenaWeatherType> Weathers =
	{
		ArenaWeatherType::Clear,
		ArenaWeatherType::Overcast,
		ArenaWeatherType::Rain,
		ArenaWeatherType::Snow,
		ArenaWeatherType::SnowOvercast,
		ArenaWeatherType::Rain2,
		ArenaWeatherType::Overcast2,
		ArenaWeatherType::SnowOvercast2
	};

	const std::unordered_map<ArenaWeatherType, std::string> WeatherTypeNames =
	{
		{ ArenaWeatherType::Clear, "Clear" },
		{ ArenaWeatherType::Overcast, "Overcast" },
		{ ArenaWeatherType::Rain, "Rain" },
		{ ArenaWeatherType::Snow, "Snow" },
		{ ArenaWeatherType::SnowOvercast, "Snow Overcast" },
		{ ArenaWeatherType::Rain2, "Rain 2" },
		{ ArenaWeatherType::Overcast2, "Overcast 2" },
		{ ArenaWeatherType::SnowOvercast2, "Snow Overcast 2" }
	};

	std::string getTestButtonText();
	std::string getTestTypeName(int type);

	std::string getSelectedTestName(Game &game, int testType, int testIndex, int testIndex2);
	std::optional<ArenaInteriorType> getSelectedTestInteriorType(int testType, int testIndex);
	ArenaWeatherType getSelectedTestWeatherType(int testWeather);
	MapType getSelectedTestMapType(int testType);

	void getMainQuestLocationFromIndex(int testIndex, const ExeData &exeData,
		int *outLocationID, int *outProvinceID, SpecialCaseType *outSpecialCaseType);
	std::vector<int> makeShuffledLocationIndices(const ProvinceDefinition &provinceDef);
	std::optional<int> getRandomCityLocationDefIndexIfType(const ProvinceDefinition &provinceDef, ArenaCityType cityType);
	int getRandomCityLocationIndex(const ProvinceDefinition &provinceDef);
	std::optional<int> getRandomDungeonLocationDefIndex(const ProvinceDefinition &provinceDef);
}

namespace MainMenuUiView
{
	Rect getLoadButtonRect();
	Rect getNewGameButtonRect();
	Rect getExitButtonRect();

	// Test values.
	Rect getTestButtonRect();
	constexpr UiTexturePatternType TestButtonPatternType = UiTexturePatternType::Custom1;
	const std::string TestButtonFontName = ArenaFontName::Arena;
	constexpr TextAlignment TestButtonTextAlignment = TextAlignment::MiddleCenter;

	Color getTestButtonTextColor();

	Rect getTestTypeUpButtonRect();
	Rect getTestTypeDownButtonRect();
	Rect getTestIndexUpButtonRect();
	Rect getTestIndexDownButtonRect();
	Rect getTestIndex2UpButtonRect();
	Rect getTestIndex2DownButtonRect();
	Rect getTestWeatherUpButtonRect();
	Rect getTestWeatherDownButtonRect();

	TextureAsset getBackgroundTextureAsset();
	TextureAsset getPaletteTextureAsset();
	TextureAsset getTestArrowsTextureAsset();
	TextureAsset getTestArrowsPaletteTextureAsset();

	UiTextureID allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocTestArrowsTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocTestButtonTexture(TextureManager &textureManager, Renderer &renderer);
}

namespace MainMenuUiController
{
	void onLoadGameButtonSelected(Game &game);
	void onNewGameButtonSelected(Game &game);
	void onExitGameButtonSelected();

	void onQuickStartButtonSelected(Game &game, int testType, int testIndex, const std::string &mifName,
		const std::optional<ArenaInteriorType> &optInteriorType, ArenaWeatherType weatherType, MapType mapType);

	void onTestTypeUpButtonSelected(int *testType, int *testIndex, int *testIndex2, int *testWeather);
	void onTestTypeDownButtonSelected(int *testType, int *testIndex, int *testIndex2, int *testWeather);
	void onTestIndexUpButtonSelected(int *testType, int *testIndex, int *testIndex2);
	void onTestIndexDownButtonSelected(int *testType, int *testIndex, int *testIndex2);
	void onTestIndex2UpButtonSelected(int testType, int testIndex, int *testIndex2);
	void onTestIndex2DownButtonSelected(int testType, int testIndex, int *testIndex2);
	void onTestWeatherUpButtonSelected(int testType, int *testWeather);
	void onTestWeatherDownButtonSelected(int testType, int *testWeather);
}

#endif
