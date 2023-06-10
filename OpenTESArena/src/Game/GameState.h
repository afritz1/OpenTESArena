#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <functional>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <vector>

#include "Clock.h"
#include "Date.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Entities/Player.h"
#include "../Interface/ProvinceMapUiModel.h"
#include "../Math/Random.h"
#include "../Math/Vector2.h"
#include "../Weather/WeatherDefinition.h"
#include "../Weather/WeatherInstance.h"
#include "../World/MapDefinition.h"
#include "../WorldMap/WorldMapInstance.h"

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
class MusicDefinition;
class ProvinceDefinition;
class Renderer;
class TextAssetLibrary;
class Texture;
class TextureManager;

enum class MapType;

class GameState
{
public:
	using SceneChangeMusicFunc = std::function<const MusicDefinition*(Game&)>;

	// Used with the currently selected world map location.
	struct WorldMapLocationIDs
	{
		int provinceID;
		int locationID;

		WorldMapLocationIDs(int provinceID, int locationID);
	};
private:
	// Determines length of a real-time second in-game. For the original game, one real second is
	// twenty in-game seconds.
	static constexpr double GAME_TIME_SCALE = static_cast<double>(Clock::SECONDS_IN_A_DAY) / 4320.0;

	MapDefinition activeMapDef;
	int activeLevelIndex;
	MapDefinition prevMapDef; // Stored map definition when in an interior.
	std::optional<CoordInt3> prevMapReturnCoord; // Used when leaving interiors.

	// Scene transition request variables.
	MapDefinition nextMapDef;
	VoxelInt2 nextMapPlayerStartOffset; // Used with random dungeons.
	std::optional<WorldMapLocationIDs> nextMapDefLocationIDs;
	std::optional<WeatherDefinition> nextMapDefWeatherDef; // Used with fast travel, etc..
	bool nextMapClearsPrevious; // Clears any previously-loaded map defs (such as when fast travelling).
	int nextLevelIndex;
	SceneChangeMusicFunc nextMusicFunc, nextJingleMusicFunc; // Music changes after a map change.
	
	// Player's current world map location data.
	WorldMapDefinition worldMapDef;
	WorldMapInstance worldMapInst;
	int provinceIndex;
	int locationIndex;
	std::optional<ProvinceMapUiModel::TravelData> travelData; // Non-null when a destination is selected.

	// Game world interface display texts have an associated time remaining. These values are stored here so
	// they are not destroyed when switching away from the game world panel.
	// - Trigger text: lore message from voxel trigger
	// - Action text: description of the player's current action
	// - Effect text: effect on the player (disease, drunk, silence, etc.)
	double triggerTextRemainingSeconds, actionTextRemainingSeconds, effectTextRemainingSeconds;

	// One weather for each of the 36 province quadrants (updated hourly).
	static constexpr int WORLD_MAP_WEATHER_QUADRANT_COUNT = 36;
	ArenaTypes::WeatherType worldMapWeathers[WORLD_MAP_WEATHER_QUADRANT_COUNT];

	// Custom function for *LEVELUP voxel enter events. If no function is set, the default
	// behavior is to decrement the world's level index.
	std::function<void(Game&)> onLevelUpVoxelEnter;

	Date date;
	Clock clock;
	ArenaRandom arenaRandom;
	double chasmAnimSeconds;
	bool isCamping;

	WeatherDefinition weatherDef;
	WeatherInstance weatherInst;

	// Attempts to set the level active in the systems (i.e. renderer) that need its data.
	/*bool trySetLevelActive(LevelInstance &levelInst, const std::optional<int> &activeLevelIndex,
		Player &player, WeatherDefinition &&weatherDef, const CoordInt2 &startCoord,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		RenderChunkManager &renderChunkManager, TextureManager &textureManager, Renderer &renderer);*/

	// Attempts to set the sky active in the systems (i.e. renderer) that need its data. This must
	// be run after trySetLevelActive() (not sure that's a good idea though).
	/*bool trySetSkyActive(SkyInstance &skyInst, const std::optional<int> &activeLevelIndex,
		TextureManager &textureManager, Renderer &renderer);*/

	// Attempts to apply the map transition state saved from the previous frame to the current game state.
	/*bool tryApplyMapTransition(MapTransitionState &&transitionState, Player &player,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		RenderChunkManager &renderChunkManager, TextureManager &textureManager, Renderer &renderer);*/

	void clearMaps();
public:
	// Creates incomplete game state with no active world, to be further initialized later.
	GameState();
	GameState(GameState&&) = default;
	~GameState();

	void init(const BinaryAssetLibrary &binaryAssetLibrary);
	
	void clearSession();

	bool hasPendingLevelIndexChange() const;
	bool hasPendingMapDefChange() const;
	bool hasPendingSceneChange() const;

	void queueLevelIndexChange(int newLevelIndex);
	void queueMapDefChange(MapDefinition &&newMapDef, const std::optional<CoordInt3> &returnCoord = std::nullopt,
		const VoxelInt2 &playerStartOffset = VoxelInt2::Zero,
		const std::optional<WorldMapLocationIDs> &worldMapLocationIDs = std::nullopt,
		bool clearPreviousMap = false, const std::optional<WeatherDefinition> &weatherDef = std::nullopt);
	void queueMapDefPop();
	void queueMusicOnSceneChange(const SceneChangeMusicFunc &musicFunc, const SceneChangeMusicFunc &jingleMusicFunc = SceneChangeMusicFunc());

	// Attempts to generate an interior, add it to the map stack, and set it active based on the given generation
	// info. This preserves existing maps for later when the interior is exited.
	/*bool tryPushInterior(const MapGeneration::InteriorGenInfo &interiorGenInfo,
		const std::optional<CoordInt3> &returnCoord, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, Renderer &renderer);*/

	// Clears all maps and attempts to generate an interior and set it active based on the given generation info.
	// This is simpler than pushing an interior since there is no exterior to return to. Intended for world map
	// dungeons.
	/*bool trySetInterior(const MapGeneration::InteriorGenInfo &interiorGenInfo,
		const std::optional<VoxelInt2> &playerStartOffset, const WorldMapLocationIDs &worldMapLocationIDs,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, Renderer &renderer);*/

	// Clears all maps and attempts to generate a city and set it active based on the given generation info.
	/*bool trySetCity(const MapGeneration::CityGenInfo &cityGenInfo, const SkyGeneration::ExteriorSkyGenInfo &skyGenInfo,
		const std::optional<WeatherDefinition> &overrideWeather,
		const std::optional<WorldMapLocationIDs> &newWorldMapLocationIDs,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, const TextAssetLibrary &textAssetLibrary,
		TextureManager &textureManager, Renderer &renderer);*/

	// Clears all maps and attempts to generate a wilderness and set it active based on the given generation info.
	/*bool trySetWilderness(const MapGeneration::WildGenInfo &wildGenInfo,
		const SkyGeneration::ExteriorSkyGenInfo &skyGenInfo, const std::optional<WeatherDefinition> &overrideWeather,
		const std::optional<CoordInt3> &startCoord, const std::optional<WorldMapLocationIDs> &newWorldMapLocationIDs,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, Renderer &renderer);*/

	// Pops the top-most map from the stack and sets the next map active if there is one available.
	/*bool tryPopMap(Player &player, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, RenderChunkManager &renderChunkManager,
		TextureManager &textureManager, Renderer &renderer);*/

	MapType getActiveMapType() const;
	bool isActiveMapValid() const; // Basically "is there something we can populate the scene with?".
	int getActiveLevelIndex() const;
	const MapDefinition &getActiveMapDef() const; // @todo: this is bad practice since it becomes dangling when changing the active map.
	double getActiveCeilingScale() const;
	bool isActiveMapNested() const; // True if the active interior is inside an exterior.
	const WorldMapDefinition &getWorldMapDefinition() const;
	const ProvinceDefinition &getProvinceDefinition() const;
	const LocationDefinition &getLocationDefinition() const;
	WorldMapInstance &getWorldMapInstance();
	ProvinceInstance &getProvinceInstance();
	LocationInstance &getLocationInstance();
	const ProvinceMapUiModel::TravelData *getTravelData() const;
	BufferView<const ArenaTypes::WeatherType> getWorldMapWeathers() const;
	ArenaTypes::WeatherType getWeatherForLocation(int provinceIndex, int locationIndex) const;
	Date &getDate();
	Clock &getClock();

	// Gets a percentage representing how far along the current day is. 0.0 is 
	// 12:00am and 0.50 is noon.
	double getDaytimePercent() const;

	// Gets a percentage representing the current progress through the looping chasm animation.
	double getChasmAnimPercent() const;

	// Gets the currently selected weather and associated state.
	const WeatherDefinition &getWeatherDefinition() const;
	const WeatherInstance &getWeatherInstance() const;

	// Gets the custom function for the *LEVELUP voxel enter event.
	std::function<void(Game&)> &getOnLevelUpVoxelEnter();

	// On-screen text is visible if it has remaining duration.
	bool triggerTextIsVisible() const;
	bool actionTextIsVisible() const;
	bool effectTextIsVisible() const;

	// Sets whether the player is camping, which influences time passing and other things.
	void setIsCamping(bool isCamping);

	// Sets the player's world map travel data when they select a destination.
	void setTravelData(std::optional<ProvinceMapUiModel::TravelData> travelData);

	// Sets on-screen text duration for various types of in-game messages.
	void setTriggerTextDuration(const std::string_view &text);
	void setActionTextDuration(const std::string_view &text);
	void setEffectTextDuration(const std::string_view &text);

	// Resets on-screen text boxes to empty and hidden.
	void resetTriggerTextDuration();
	void resetActionTextDuration();
	void resetEffectTextDuration();

	// Recalculates the weather for each global quarter (done hourly).
	void updateWeatherList(const ExeData &exeData);

	// Checks if there is a map transition in progress and applies it if so, setting the new level active in
	// the game world and renderer.
	void tryUpdatePendingMapTransition(Game &game, double dt);

	// Ticks the game clock (for the current time of day and date).
	void tickGameClock(double dt, Game &game);
	void tickChasmAnimation(double dt);
	void tickWeather(double dt, Game &game);
	void tickUiMessages(double dt);
	void tickPlayer(double dt, Game &game);
	void tickVoxels(double dt, Game &game);
	void tickEntities(double dt, Game &game);
	void tickCollision(double dt, Game &game);
	void tickRendering(Game &game);
};

#endif
