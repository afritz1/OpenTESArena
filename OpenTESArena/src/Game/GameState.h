#ifndef GAME_STATE_H
#define GAME_STATE_H

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
#include "../World/MapDefinition.h"
#include "../World/MapInstance.h"
#include "../World/WorldMapInstance.h"

// Intended to be a container for the player and world data that is currently active 
// while a player is loaded (i.e., not in the main menu).

// The GameState object will be initialized only upon loading of the player, and 
// will be uninitialized when the player goes to the main menu (thus unloading
// the character resources). Whichever entry points into the "game" there are, they
// need to load data into the game state object.

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

class GameState
{
public:
	// One weather for each of the 36 province quadrants (updated hourly).
	using WeatherList = std::array<WeatherType, 36>;
private:
	struct MapState
	{
		MapDefinition definition;
		MapInstance instance;
		std::optional<CoordInt3> returnVoxel; // Available when returning from inside an interior.
		
		void init(MapDefinition &&mapDefinition, MapInstance &&mapInstance, const std::optional<CoordInt3> &returnVoxel);
	};

	// Determines length of a real-time second in-game. For the original game, one real
	// second is twenty in-game seconds.
	static constexpr double TIME_SCALE = static_cast<double>(Clock::SECONDS_IN_A_DAY) / 4320.0;

	Player player;

	// Stack of map definitions and instances. Multiple ones can exist at the same time when the player is
	// inside an interior in a city or wilderness, but ultimately the size should never exceed 2.
	std::stack<MapState> maps;

	CitizenManager citizenManager; // Tracks active citizens and spawning.
	
	// Player's current world map location data.
	WorldMapDefinition worldMapDef;
	WorldMapInstance worldMapInst;
	int provinceIndex;
	int locationIndex;

	// Game world interface display texts with their associated time remaining. These values 
	// are stored here so they are not destroyed when switching away from the game world panel.
	// - Trigger text: lore message from voxel trigger
	// - Action text: description of the player's current action
	// - Effect text: effect on the player (disease, drunk, silence, etc.)
	TimedTextBox triggerText, actionText, effectText;

	WeatherList weathers;

	// Custom function for *LEVELUP voxel enter events. If no function is set, the default
	// behavior is to decrement the world's level index.
	std::function<void(Game&)> onLevelUpVoxelEnter;

	Date date;
	Clock clock;
	ArenaRandom arenaRandom;
	double chasmAnimSeconds;
	WeatherType weatherType;

	// Helper function for generating a map definition and instance from the given world map location.
	static bool tryMakeMapFromLocation(const LocationDefinition &locationDef, int raceID, WeatherType weatherType,
		int currentDay, int starCount, bool provinceHasAnimatedLand, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		const TextAssetLibrary &textAssetLibrary, TextureManager &textureManager, MapState *outMapState);

	void setTransitionedPlayerPosition(const NewDouble3 &position);

	// Attempts to set the level active in the systems (i.e. renderer) that need its data.
	bool trySetLevelActive(LevelInstance &levelInst, WeatherType weatherType, const LevelDouble2 &startPoint,
		TextureManager &textureManager, Renderer &renderer);

	void clearMaps();
public:
	// Creates incomplete game state with no active world, to be further initialized later.
	GameState(Player &&player, const BinaryAssetLibrary &binaryAssetLibrary);
	GameState(GameState&&) = default;
	~GameState();

	// Clears all maps and attempts to generate one and set it active based on the given province + location pair.
	// The map type can only be an interior (world map dungeon, etc.) or a city, as viewed from the world map.
	bool trySetFromWorldMap(int provinceID, int locationID, int currentDay, int starCount,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, const TextAssetLibrary &textAssetLibrary,
		TextureManager &textureManager, Renderer &renderer);

	// Attempts to generate an interior, add it to the map stack, and set it active based on the given generation
	// info. This preserves existing maps for later when the interior is exited.
	bool tryPushInterior(const MapGeneration::InteriorGenInfo &interiorGenInfo,
		const std::optional<CoordInt3> &returnVoxel, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, Renderer &renderer);

	// Clears all maps and attempts to generate a city and set it active based on the given generation info.
	bool trySetCity(const MapGeneration::CityGenInfo &cityGenInfo);

	// Clears all maps and attempts to generate a wilderness and set it active based on the given generation info.
	bool trySetWilderness(const MapGeneration::WildGenInfo &wildGenInfo);

	// Pops the top-most map from the stack and sets the next map active. This fails if there would be no available map
	// to switch to (meaning that there must always be an active map).
	bool tryPopMap();

	Player &getPlayer();
	const MapDefinition &getActiveMapDef() const; // @todo: this is bad practice since it becomes dangling when changing the active map.
	MapInstance &getActiveMapInst(); // @todo: this is bad practice since it becomes dangling when changing the active map.
	const MapInstance &getActiveMapInst() const; // @todo: this is bad practice since it becomes dangling when changing the active map.
	bool isActiveMapNested() const; // True if the active interior is inside an exterior.
	CitizenManager &getCitizenManager();
	const WorldMapDefinition &getWorldMapDefinition() const;
	const ProvinceDefinition &getProvinceDefinition() const;
	const LocationDefinition &getLocationDefinition() const;
	WorldMapInstance &getWorldMapInstance();
	ProvinceInstance &getProvinceInstance();
	LocationInstance &getLocationInstance();
	const WeatherList &getWeathersArray() const;
	Date &getDate();
	Clock &getClock();
	ArenaRandom &getRandom();

	// Gets a percentage representing how far along the current day is. 0.0 is 
	// 12:00am and 0.50 is noon.
	double getDaytimePercent() const;

	// Gets a percentage representing the current progress through the looping chasm animation.
	double getChasmAnimPercent() const;

	WeatherType getWeatherType() const;

	// Gets the current ambient light percent, based on the current clock time and 
	// the player's location (interior/exterior). This function is intended to match
	// the actual calculation done in Arena.
	double getAmbientPercent() const;

	// A more gradual ambient percent function (maybe useful on the side sometime).
	double getBetterAmbientPercent() const;

	// Returns whether the current music should be for day or night.
	bool nightMusicIsActive() const;

	// Returns whether night lights (i.e., lampposts) should currently be active.
	bool nightLightsAreActive() const;

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
