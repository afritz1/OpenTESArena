#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <functional>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "Clock.h"
#include "Date.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Entities/CitizenManager.h"
#include "../Entities/EntityManager.h"
#include "../Entities/Player.h"
#include "../Interface/TimedTextBox.h"
#include "../Math/Random.h"
#include "../Math/Vector2.h"
#include "../World/WorldData.h"
#include "../World/WorldMapInstance.h"

// Intended to be a container for the player and world data that is currently active 
// while a player is loaded (i.e., not in the main menu).

// The GameData object will be initialized only upon loading of the player, and 
// will be uninitialized when the player goes to the main menu (thus unloading
// the character resources). Whichever entry points into the "game" there are, they
// need to load data into the game data object.

class BinaryAssetLibrary;
class CharacterClassLibrary;
class CityDataFile;
class EntityDefinitionLibrary;
class FontLibrary;
class INFFile;
class LocationDefinition;
class LocationInstance;
class MIFFile;
class ProvinceDefinition;
class Renderer;
class TextAssetLibrary;
class TextBox;
class Texture;
class TextureManager;

enum class MapType;
enum class WeatherType;

class GameData
{
public:
	// One weather for each of the 36 province quadrants (updated hourly).
	using WeatherList = std::array<WeatherType, 36>;
private:
	// Determines length of a real-time second in-game. For the original game, one real
	// second is twenty in-game seconds.
	static constexpr double TIME_SCALE = static_cast<double>(Clock::SECONDS_IN_A_DAY) / 4320.0;

	// Game world interface display texts with their associated time remaining. These values 
	// are stored here so they are not destroyed when switching away from the game world panel.
	// - Trigger text: lore message from voxel trigger
	// - Action text: description of the player's current action
	// - Effect text: effect on the player (disease, drunk, silence, etc.)
	TimedTextBox triggerText, actionText, effectText;

	WeatherList weathers;

	Player player;

	// Stack of world data instances. Multiple ones can exist at the same time when the player is inside
	// an interior in a city or wilderness, but ultimately the size should never exceed 2.
	std::stack<std::unique_ptr<WorldData>> worldDatas;
	std::optional<NewInt2> returnVoxel; // Available if in an interior that's in an exterior.

	CitizenManager citizenManager; // Tracks active citizens and spawning.
	
	// Player's current world map location data.
	WorldMapDefinition worldMapDef;
	WorldMapInstance worldMapInst;
	int provinceIndex;
	int locationIndex;

	Date date;
	Clock clock;
	ArenaRandom arenaRandom;
	double fogDistance;
	double chasmAnimSeconds;
	WeatherType weatherType;

	// Custom function for *LEVELUP voxel enter events. If no function is set, the default
	// behavior is to decrement the world's level index.
	std::function<void(Game&)> onLevelUpVoxelEnter;

	void setTransitionedPlayerPosition(const NewDouble3 &position);
	void clearWorldDatas();
public:
	// Creates incomplete game data with no active world, to be further initialized later.
	GameData(Player &&player, const BinaryAssetLibrary &binaryAssetLibrary);
	GameData(GameData&&) = default;
	~GameData();

	// Returns whether the current music should be for day or night.
	bool nightMusicIsActive() const;

	// Returns whether night lights (i.e., lampposts) should currently be active.
	bool nightLightsAreActive() const;

	// Reads in data from an interior .MIF file and writes it to the game data.
	bool loadInterior(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
		ArenaTypes::InteriorType interiorType, const MIFFile &mif,
		const EntityDefinitionLibrary &entityDefLibrary, const CharacterClassLibrary &charClassLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, Random &random, TextureManager &textureManager,
		Renderer &renderer);

	// Reads in data from an interior .MIF file and inserts it into the active exterior data.
	// Only call this method if the player is in an exterior location (city or wilderness).
	void enterInterior(ArenaTypes::InteriorType interiorType, const MIFFile &mif,
		const Int2 &returnVoxel, const EntityDefinitionLibrary &entityDefLibrary,
		const CharacterClassLibrary &charClassLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		Random &random, TextureManager &textureManager, Renderer &renderer);

	// Leaves the current interior and returns to the exterior. Only call this method if the
	// player is in an interior that has an outside area to return to.
	void leaveInterior(const EntityDefinitionLibrary &entityDefLibrary,
		const CharacterClassLibrary &charClassLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		Random &random, TextureManager &textureManager, Renderer &renderer);

	// Reads in data from RANDOM1.MIF based on the given dungeon ID and parameters and writes it
	// to the game data. This modifies the current map location.
	bool loadNamedDungeon(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
		bool isArtifactDungeon, const EntityDefinitionLibrary &entityDefLibrary,
		const CharacterClassLibrary &charClassLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		Random &random, TextureManager &textureManager, Renderer &renderer);

	// Reads in data from RANDOM1.MIF based on the given location parameters and writes it to the
	// game data. This does not modify the current map location.
	bool loadWildernessDungeon(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
		int wildBlockX, int wildBlockY, const CityDataFile &cityData,
		const EntityDefinitionLibrary &entityDefLibrary, const CharacterClassLibrary &charClassLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, Random &random, TextureManager &textureManager,
		Renderer &renderer);

	// Reads in data from a city after determining its .MIF file, and writes it to the game data.
	bool loadCity(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
		WeatherType weatherType, int starCount, const EntityDefinitionLibrary &entityDefLibrary,
		const CharacterClassLibrary &charClassLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		const TextAssetLibrary &textAssetLibrary, Random &random, TextureManager &textureManager,
		Renderer &renderer);

	// Reads in data from wilderness and writes it to the game data.
	bool loadWilderness(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
		const NewInt2 &gatePos, const NewInt2 &transitionDir, bool debug_ignoreGatePos,
		WeatherType weatherType, int starCount, const EntityDefinitionLibrary &entityDefLibrary, 
		const CharacterClassLibrary &charClassLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		Random &random, TextureManager &textureManager, Renderer &renderer);

	const WeatherList &getWeathersArray() const;

	Player &getPlayer();
	WorldData &getActiveWorld(); // @todo: this is bad practice since leaveInterior() can delete the active world.
	bool isActiveWorldNested() const; // True if the active interior is inside an exterior.
	CitizenManager &getCitizenManager();
	const WorldMapDefinition &getWorldMapDefinition() const;
	const ProvinceDefinition &getProvinceDefinition() const;
	const LocationDefinition &getLocationDefinition() const;
	WorldMapInstance &getWorldMapInstance();
	ProvinceInstance &getProvinceInstance();
	LocationInstance &getLocationInstance();
	Date &getDate();
	Clock &getClock();
	ArenaRandom &getRandom();

	// Gets a percentage representing how far along the current day is. 0.0 is 
	// 12:00am and 0.50 is noon.
	double getDaytimePercent() const;

	// Gets a percentage representing the current progress through the looping chasm animation.
	double getChasmAnimPercent() const;

	double getFogDistance() const;
	WeatherType getWeatherType() const;

	// Gets the current ambient light percent, based on the current clock time and 
	// the player's location (interior/exterior). This function is intended to match
	// the actual calculation done in Arena.
	double getAmbientPercent() const;

	// A more gradual ambient percent function (maybe useful on the side sometime).
	double getBetterAmbientPercent() const;

	// Gets the custom function for the *LEVELUP voxel enter event.
	std::function<void(Game&)> &getOnLevelUpVoxelEnter();

	// On-screen text is visible if it has remaining duration.
	bool triggerTextIsVisible() const;
	bool actionTextIsVisible() const;
	bool effectTextIsVisible() const;

	// On-screen text render info for the game world.
	void getTriggerTextRenderInfo(const Texture **outTexture) const;
	void getActionTextRenderInfo(const Texture **outTexture) const;
	void getEffectTextRenderInfo(const Texture **outTexture) const;

	// Sets on-screen text for various types of in-game messages.
	void setTriggerText(const std::string &text, FontLibrary &fontLibrary, Renderer &renderer);
	void setActionText(const std::string &text, FontLibrary &fontLibrary, Renderer &renderer);
	void setEffectText(const std::string &text, FontLibrary &fontLibrary, Renderer &renderer);

	// Resets on-screen text boxes to empty and hidden.
	void resetTriggerText();
	void resetActionText();
	void resetEffectText();

	// Recalculates the weather for each global quarter (done hourly).
	void updateWeather(const ExeData &exeData);

	// Ticks the game clock (for the current time of day and date).
	void tick(double dt, Game &game);
};

#endif
