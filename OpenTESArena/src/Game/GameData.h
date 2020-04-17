#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Clock.h"
#include "Date.h"
#include "../Assets/MiscAssets.h"
#include "../Entities/EntityManager.h"
#include "../Entities/Player.h"
#include "../Interface/TimedTextBox.h"
#include "../Math/Random.h"
#include "../Math/Vector2.h"
#include "../World/Location.h"
#include "../World/WorldData.h"
#include "../World/WorldMapInstance.h"

// Intended to be a container for the player and world data that is currently active 
// while a player is loaded (i.e., not in the main menu).

// The GameData object will be initialized only upon loading of the player, and 
// will be uninitialized when the player goes to the main menu (thus unloading
// the character resources). Whichever entry points into the "game" there are, they
// need to load data into the game data object.

class CharacterClass;
class CityDataFile;
class FontManager;
class INFFile;
class LocationDefinition;
class LocationInstance;
class MIFFile;
class ProvinceDefinition;
class Renderer;
class TextBox;
class Texture;
class TextureManager;

enum class GenderName;
enum class WeatherType;
enum class WorldType;

class GameData
{
private:
	// The time scale determines how long or short a real-time second is. If the time 
	// scale is 5.0, then each real-time second is five game seconds, etc..
	static const double TIME_SCALE;

	// Seconds per chasm animation loop.
	static const double CHASM_ANIM_PERIOD;

	// Arbitrary value for interior fog distance (mostly for testing purposes).
	static const double DEFAULT_INTERIOR_FOG_DIST;

	std::unordered_map<Int2, std::string> textTriggers, soundTriggers;

	// Game world interface display texts with their associated time remaining. These values 
	// are stored here so they are not destroyed when switching away from the game world panel.
	// - Trigger text: lore message from voxel trigger
	// - Action text: description of the player's current action
	// - Effect text: effect on the player (disease, drunk, silence, etc.)
	TimedTextBox triggerText, actionText, effectText;

	// One weather for each of the 36 province quadrants (updated hourly).
	std::array<WeatherType, 36> weathers;

	Player player;
	std::unique_ptr<WorldData> worldData;
	Location location;
	WorldMapInstance worldMapInst;
	Date date;
	Clock clock;
	ArenaRandom arenaRandom;
	double fogDistance;
	double chasmAnimSeconds;
	WeatherType weatherType;

	// Custom function for *LEVELUP voxel enter events. If no function is set, the default
	// behavior is to decrement the world's level index.
	std::function<void(Game&)> onLevelUpVoxelEnter;
public:
	// Clock times for when each time range begins.
	static const Clock Midnight;
	static const Clock Night1;
	static const Clock EarlyMorning;
	static const Clock Morning;
	static const Clock Noon;
	static const Clock Afternoon;
	static const Clock Evening;
	static const Clock Night2;

	// Clock times for changes in ambient lighting.
	static const Clock AmbientStartBrightening;
	static const Clock AmbientEndBrightening;
	static const Clock AmbientStartDimming;
	static const Clock AmbientEndDimming;

	// Clock times for lamppost activation.
	static const Clock LamppostActivate;
	static const Clock LamppostDeactivate;

	// Clock times for changes in music.
	static const Clock MusicSwitchToDay;
	static const Clock MusicSwitchToNight;

	// Creates incomplete game data with no active world, to be further initialized later.
	GameData(Player &&player, const MiscAssets &miscAssets);
	GameData(GameData&&) = default;
	~GameData();

	// Gets the date string for a given date, using strings from the executable data.
	static std::string getDateString(const Date &date, const ExeData &exeData);

	// Returns whether the current music should be for day or night.
	bool nightMusicIsActive() const;

	// Returns whether night lights (i.e., lampposts) should currently be active.
	bool nightLightsAreActive() const;

	// Reads in data from an interior .MIF file and writes it to the game data.
	void loadInterior(VoxelDefinition::WallData::MenuType interiorType, const MIFFile &mif,
		const Location &location, const MiscAssets &miscAssets, TextureManager &textureManager,
		Renderer &renderer);

	// Reads in data from an interior .MIF file and inserts it into the active exterior data.
	// Only call this method if the player is in an exterior location (city or wilderness).
	void enterInterior(VoxelDefinition::WallData::MenuType interiorType, const MIFFile &mif,
		const Int2 &returnVoxel, const MiscAssets &miscAssets, TextureManager &textureManager,
		Renderer &renderer);

	// Leaves the current interior and returns to the exterior. Only call this method if the
	// player is in an interior that has an outside area to return to.
	void leaveInterior(const MiscAssets &miscAssets, TextureManager &textureManager,
		Renderer &renderer);

	// Reads in data from RANDOM1.MIF based on the given dungeon ID and parameters and writes it
	// to the game data. This modifies the current map location.
	void loadNamedDungeon(int localDungeonID, int provinceID, const LocationDefinition &locationDef,
		const ProvinceDefinition &provinceDef, bool isArtifactDungeon,
		VoxelDefinition::WallData::MenuType interiorType, const MiscAssets &miscAssets,
		TextureManager &textureManager, Renderer &renderer);

	// Reads in data from RANDOM1.MIF based on the given location parameters and writes it to the
	// game data. This does not modify the current map location.
	void loadWildernessDungeon(int provinceID, int wildBlockX, int wildBlockY,
		VoxelDefinition::WallData::MenuType interiorType, const CityDataFile &cityData,
		const MiscAssets &miscAssets, TextureManager &textureManager, Renderer &renderer);

	// Reads in data from a city after determining its .MIF file, and writes it to the game data.
	void loadCity(int localCityID, int provinceID, const LocationDefinition &locationDef,
		const ProvinceDefinition &provinceDef, WeatherType weatherType, int starCount,
		const MiscAssets &miscAssets, TextureManager &textureManager, Renderer &renderer);

	// Reads in data from wilderness and writes it to the game data.
	void loadWilderness(int localCityID, int provinceID, const LocationDefinition &locationDef,
		const ProvinceDefinition &provinceDef, const Int2 &gatePos, const Int2 &transitionDir,
		bool debug_ignoreGatePos, WeatherType weatherType, int starCount, const MiscAssets &miscAssets,
		TextureManager &textureManager, Renderer &renderer);

	const std::array<WeatherType, 36> &getWeathersArray() const;

	Player &getPlayer();
	WorldData &getWorldData();
	Location &getLocation(); // @todo: deprecate and remove
	WorldMapInstance &getWorldMapInstance();
	ProvinceInstance &getProvinceInstance();
	const LocationDefinition &getLocationDefinition(const WorldMapDefinition &worldMapDef) const;
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
	void setTriggerText(const std::string &text, FontManager &fontManager, Renderer &renderer);
	void setActionText(const std::string &text, FontManager &fontManager, Renderer &renderer);
	void setEffectText(const std::string &text, FontManager &fontManager, Renderer &renderer);

	// Resets on-screen text boxes to empty and hidden.
	void resetTriggerText();
	void resetActionText();
	void resetEffectText();

	// Recalculates the weather for each global quarter (done hourly).
	void updateWeather(const ExeData &exeData);

	// Ticks the game clock (for the current time of day and date).
	void tickTime(double dt, Game &game);
};

#endif
