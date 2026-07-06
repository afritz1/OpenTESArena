#pragma once

#include <optional>
#include <stack>
#include <string>
#include <vector>

#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"

#include "../Entities/EntityDefinitionLibrary.h"
#include "../Interface/ProvinceMapUiMVC.h"
#include "../Math/Random.h"
#include "../Math/Vector2.h"
#include "../Player/Player.h"
#include "../Time/Clock.h"
#include "../Time/Date.h"
#include "../Weather/WeatherDefinition.h"
#include "../Weather/WeatherInstance.h"
#include "../World/MapDefinition.h"
#include "../WorldMap/WorldMapInstance.h"

class LocationDefinition;
class LocationInstance;
class ProvinceDefinition;

enum class MapType;

struct MusicDefinition;
struct RenderCamera;

struct GuardSpawnState
{
	double secondsTillSpawn;
	std::function<void()> spawnFunc;

	// @todo maybe want to store spawned guard EntityInstanceIDs so we know when to add citizens back

	GuardSpawnState();

	bool isQueued() const;

	void clearQueue();
};

struct EntityEncounterSpawnInfo
{
	int guardType; // Non-negative if for city guards.
	int level;
	int count;
	EntityDefinitionPredicate entityDefPredicate; // Filters to matching definitions from library.

	EntityEncounterSpawnInfo();

	void initCreaturesOrHumans(int spawnID, int level, int count);
	void initCityGuards(int guardType, int level, int count);

	bool isCityGuards() const;
};

struct CampingState
{
	int manualHoursRemaining;

	bool isCampingUntilHealed;
	int untilHealedHoursAccumulated;

	CampingState();

	bool isCamping() const;

	void setManualHours(int hours);
	void setUntilHealed();

	void clear();
};

// Container for currently loaded game/world data.
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
	MapDefinition activeMapDef;
	int activeLevelIndex;
	MapDefinition prevMapDef; // Stored map definition when in an interior.
	std::optional<CoordInt3> prevMapReturnCoord; // Used when leaving interiors.

	// Scene transition request variables.
	MapDefinition nextMapDef;
	std::optional<CoordInt2> nextMapStartCoord; // Overrides map definition start points; used with city -> wilderness transition.
	VoxelInt2 nextMapPlayerStartOffset; // Used with exterior scene transitions and random dungeons.
	VoxelInt2 nextMapLevelTransitionVoxel; // Used with interior level changes.
	std::optional<WorldMapLocationIDs> nextMapDefLocationIDs;
	std::optional<WeatherDefinition> nextMapDefWeatherDef; // Used with fast travel, etc..
	bool nextMapClearsPrevious; // Clears any previously-loaded map defs (such as when fast travelling or leaving wild dungeon).
	int nextLevelIndex;
	SceneChangeMusicFunc nextMusicFunc, nextJingleMusicFunc; // Music changes after a map change.

	// Level transition calculation, stored during physics contact w/ transition voxel and applied afterwards to avoid player physics deadlock.
	CoordInt3 levelTransitionCalculationPlayerCoord;
	CoordInt3 levelTransitionCalculationTransitionCoord;
	bool isLevelTransitionCalculationPending;
	
	// Player's current world map location data.
	WorldMapInstance worldMapInst;
	int provinceIndex;
	int locationIndex;
	std::optional<ProvinceMapUiModel::TravelData> travelData; // Non-null when a destination is selected.

	// One weather for each of the 36 province quadrants (updated hourly).
	static constexpr int WORLD_MAP_WEATHER_QUADRANT_COUNT = 36;
	ArenaWeatherType worldMapWeathers[WORLD_MAP_WEATHER_QUADRANT_COUNT];

	Date date;
	Clock clock;
	double chasmAnimSeconds;

	WeatherDefinition weatherDef;
	WeatherInstance weatherInst;

	CampingState campingState;

	GuardSpawnState guardSpawnState;

	void clearMaps();
public:
	// Creates incomplete game state with no active world, to be further initialized later.
	GameState();
	GameState(GameState&&) = default;
	~GameState();

	void init(ArenaRandom &random);
	
	void clearSession();

	bool hasPendingLevelIndexChange() const;
	bool hasPendingMapDefChange() const;
	bool hasPendingSceneChange() const;

	void queueLevelIndexChange(int newLevelIndex, const VoxelInt2 &transitionVoxel, const VoxelInt2 &playerStartOffset);
	void queueMapDefChange(MapDefinition &&newMapDef, const std::optional<CoordInt2> &startCoord = std::nullopt,
		const std::optional<CoordInt3> &returnCoord = std::nullopt, const VoxelInt2 &playerStartOffset = VoxelInt2::Zero,
		const std::optional<WorldMapLocationIDs> &worldMapLocationIDs = std::nullopt,
		bool clearPreviousMap = false, const std::optional<WeatherDefinition> &weatherDef = std::nullopt);
	void queueMapDefPop();
	void queueMusicOnSceneChange(const SceneChangeMusicFunc &musicFunc, const SceneChangeMusicFunc &jingleMusicFunc = SceneChangeMusicFunc());

	bool hasPendingLevelTransitionCalculation() const;
	const CoordInt3 &getLevelTransitionCalculationPlayerCoord() const;
	const CoordInt3 &getLevelTransitionCalculationTransitionCoord() const;
	void queueLevelTransitionCalculation(const CoordInt3 &playerCoord, const CoordInt3 &transitionCoord);
	void clearLevelTransitionCalculation();

	MapType getActiveMapType() const;
	bool isActiveMapValid() const; // Basically "is there something we can populate the scene with?".
	int getActiveLevelIndex() const;
	int getActiveSkyIndex() const;
	const MapDefinition &getActiveMapDef() const;
	double getActiveCeilingScale() const;
	bool isActiveMapNested() const; // True if the active interior is inside an exterior.
	ArenaEnvironmentType getEnvironmentType() const; // For enemy encounters.
	ArenaBuildingType getBuildingType() const; // For enemy encounters.
	const ProvinceDefinition &getProvinceDefinition() const;
	const LocationDefinition &getLocationDefinition() const;
	WorldMapInstance &getWorldMapInstance();
	ProvinceInstance &getProvinceInstance();
	LocationInstance &getLocationInstance();
	const ProvinceMapUiModel::TravelData *getTravelData() const;
	Span<const ArenaWeatherType> getWorldMapWeathers() const;
	ArenaWeatherType getWeatherForLocation(int provinceIndex, int locationIndex) const;
	Date &getDate();
	Clock &getClock();
	const Clock &getClock() const;
	double getDayPercent() const;

	// Gets a percentage representing the current progress through the looping chasm animation.
	double getChasmAnimPercent() const;

	// Gets the currently selected weather and associated state.
	const WeatherDefinition &getWeatherDefinition() const;
	const WeatherInstance &getWeatherInstance() const;

	// Refers to fog in outdoor dungeons and daytime fog, not the heavy fog screen effect.
	bool isFogActive() const;

	// Sets the player's world map travel data when they select a destination.
	void setTravelData(std::optional<ProvinceMapUiModel::TravelData> travelData);

	// Recalculates the weather for each global quarter (done hourly).
	void updateWeatherList(ArenaRandom &random, const ExeData &exeData);

	// Sets whether the player is camping, which influences time passing and other things.
	bool isCamping() const;
	void setCampingManualHours(int hours);
	void setCampingUntilHealed();

	void spawnEncounterEnemies(Game &game, const EntityEncounterSpawnInfo &spawnInfo) const;
	void queueCityGuardEncounter(Game &game);

	// Applies any pending scene transition, setting the new level active in the game world and renderer.
	void applyPendingSceneChange(Game &game, JPH::PhysicsSystem &physicsSystem, double dt);

	void tickGameClock(double dt, Game &game);
	void tickChasmAnimation(double dt);
	void tickSky(double dt, Game &game);
	void tickWeather(double dt, Game &game);
	void tickUiMessages(double dt);
	void tickPlayerHealth(double dt, Game &game);
	void tickPlayerStamina(double dt, Game &game);
	void tickPlayerEffects(double dt, Game &game);
	void tickPlayerEffectChanges(const PlayerEffectsState &currentEffectsState, const PlayerEffectsState &prevEffectsState);
	void tickPlayerAttack(double dt, Game &game);
	void tickPlayerLevel(Game &game);
	void tickVoxels(double dt, Game &game);
	void tickEntitiesPrePhysicsStep(double dt, Game &game);
	void tickEntitiesPostPhysicsStep(Game &game);
	void tickCollision(double dt, JPH::PhysicsSystem &physicsSystem, Game &game);
	void tickVisibility(const RenderCamera &renderCamera, Game &game);
	void tickRendering(double dt, const RenderCamera &renderCamera, bool isFloatingOriginChanged, Game &game);
};
