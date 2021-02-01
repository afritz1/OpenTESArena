#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "ArenaLevelUtils.h"
#include "DistantSky.h"
#include "VoxelGrid.h"
#include "VoxelInstance.h"
#include "VoxelUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Entities/EntityManager.h"
#include "../Math/Vector2.h"

#include "components/utilities/BufferView.h"
#include "components/utilities/BufferView2D.h"

// Base class for each active "space" in the game. Exteriors only have one level, but
// interiors can have several.

// Arena's level origins start at the top-right corner of the map, so X increases 
// going to the left, and Z increases going down. The wilderness uses this same 
// pattern. Each chunk looks like this:
// +++++++ <- Origin (0, 0)
// +++++++
// +++++++
// +++++++
// ^
// |
// Max (mapWidth - 1, mapDepth - 1)

class ArenaRandom;
class BinaryAssetLibrary;
class CharacterClassLibrary;
class CitizenManager;
class EntityDefinitionLibrary;
class ExeData;
class Game;
class LocationDefinition;
class ProvinceDefinition;
class Random;
class Renderer;
class TextAssetLibrary;
class TextureManager;

enum class MapType;
enum class WeatherType;

class LevelData
{
public:
	class Lock
	{
	private:
		NewInt2 position;
		int lockLevel;
	public:
		Lock(const NewInt2 &position, int lockLevel);

		const NewInt2 &getPosition() const;
		int getLockLevel() const;
	};

	// Each text trigger is paired with a boolean telling whether it should be displayed once.
	class TextTrigger
	{
	private:
		std::string text;
		bool displayedOnce, previouslyDisplayed;
	public:
		TextTrigger(const std::string &text, bool displayedOnce);

		const std::string &getText() const;
		bool isSingleDisplay() const;
		bool hasBeenDisplayed() const;
		void setPreviouslyDisplayed(bool previouslyDisplayed);
	};

	// @temp: this is just a convenience class for converting voxel definition menus to the new level definition design.
	class Transition
	{
	public:
		enum class Type { LevelUp, LevelDown, Menu };

		struct Menu
		{
			int id;
		};
	private:
		NewInt2 voxel;
		Type type;
		
		union
		{
			Menu menu;
		};

		void init(const NewInt2 &voxel, Type type);
	public:
		static Transition makeLevelUp(const NewInt2 &voxel);
		static Transition makeLevelDown(const NewInt2 &voxel);
		static Transition makeMenu(const NewInt2 &voxel, int id);

		const NewInt2 &getVoxel() const;
		Type getType() const;
		const Transition::Menu &getMenu() const;
	};

	// @temp change to hash table for wild chunk name generation performance.
	using Transitions = std::unordered_map<NewInt2, Transition>;

	// Interior-specific data.
	struct Interior
	{
		std::unordered_map<NewInt2, LevelData::TextTrigger> textTriggers;
		std::unordered_map<NewInt2, std::string> soundTriggers;

		// Exteriors have dynamic sky palettes, so sky color can only be stored by interiors (for the
		// purposes of background fill, fog, etc.).
		uint32_t skyColor;

		bool outdoorDungeon;

		void init(uint32_t skyColor, bool outdoorDungeon);
	};

	// Exterior-specific data.
	struct Exterior
	{
		DistantSky distantSky;
		ArenaLevelUtils::MenuNamesList menuNames;
	};

	// One group per chunk. Needs to be a hash table for chasm rendering performance.
	using VoxelInstanceGroup = std::unordered_map<NewInt3, std::vector<VoxelInstance>>;
private:
	// Mapping of .INF flat index to instances in the game world.
	class FlatDef
	{
	private:
		ArenaTypes::FlatIndex flatIndex; // Index in .INF file flats and flat textures.
		std::vector<NewInt2> positions;
	public:
		FlatDef(ArenaTypes::FlatIndex flatIndex);

		ArenaTypes::FlatIndex getFlatIndex() const;
		const std::vector<NewInt2> &getPositions() const;

		void addPosition(const NewInt2 &position);
	};

	// Mappings of IDs to voxel data indices. These maps are stored here because they might be
	// shared between multiple calls to read{FLOR,MAP1,MAP2}().
	std::vector<std::pair<uint16_t, int>> wallDataMappings, floorDataMappings, map2DataMappings;

	VoxelGrid voxelGrid;
	EntityManager entityManager;
	INFFile inf;
	std::vector<FlatDef> flatsLists;
	std::unordered_map<NewInt2, Lock> locks;
	std::unordered_map<ChunkInt2, VoxelInstanceGroup> voxelInstMap; // @temp interim solution until using chunk system.
	Transitions transitions;
	std::string name;

	// Level-type-specific data.
	bool isInterior;
	LevelData::Interior interior;
	LevelData::Exterior exterior;

	// Used by derived LevelData load methods.
	LevelData(SNInt gridWidth, int gridHeight, WEInt gridDepth, const std::string &infName,
		const std::string &name, bool isInterior);

	void setVoxel(SNInt x, int y, WEInt z, uint16_t id);

	void addFlatInstance(ArenaTypes::FlatIndex flatIndex, const NewInt2 &flatPosition);

	void readFLOR(const BufferView2D<const ArenaTypes::VoxelID> &flor, const INFFile &inf,
		MapType mapType);
	void readMAP1(const BufferView2D<const ArenaTypes::VoxelID> &map1, const INFFile &inf,
		MapType mapType, const ExeData &exeData);
	void readMAP2(const BufferView2D<const ArenaTypes::VoxelID> &map2, const INFFile &inf);
	void readCeiling(const INFFile &inf);
	void readLocks(const BufferView<const ArenaTypes::MIFLock> &locks);
	void readTriggers(const BufferView<const ArenaTypes::MIFTrigger> &triggers, const INFFile &inf);

	// Gets voxel IDs surrounding the given voxel. If one of the IDs would point to a voxel
	// outside the grid, it is air.
	void getAdjacentVoxelIDs(const NewInt3 &voxel, uint16_t *outNorthID, uint16_t *outSouthID,
		uint16_t *outEastID, uint16_t *outWestID) const;

	// Creates mappings of *MENU voxel coordinates to *MENU names. Call this after voxels have
	// been loaded into the voxel grid so that voxel bits don't have to be decoded twice.
	static ArenaLevelUtils::MenuNamesList generateBuildingNames(const LocationDefinition &locationDef,
		const ProvinceDefinition &provinceDef, ArenaRandom &random, const VoxelGrid &voxelGrid,
		const LevelData::Transitions &transitions, const BinaryAssetLibrary &binaryAssetLibrary,
		const TextAssetLibrary &textAssetLibrary);

	// Creates mappings of wilderness *MENU voxel coordinates to *MENU names.
	static ArenaLevelUtils::MenuNamesList generateWildChunkBuildingNames(const VoxelGrid &voxelGrid,
		const LevelData::Transitions &transitions, const ExeData &exeData);

	// Refreshes a chasm voxel, after one of its neighbors presumably just changed from a
	// floor voxel.
	void tryUpdateChasmVoxel(const NewInt3 &voxel);

	// Gets the new voxel ID of a floor voxel after figuring out what chasm it would be.
	uint16_t getChasmIdFromFadedFloorVoxel(const NewInt3 &voxel);

	// Updates fading voxels in the given chunk range (interim solution to using the chunk system).
	void updateFadingVoxels(const ChunkInt2 &minChunk, const ChunkInt2 &maxChunk, double dt);
public:
	LevelData(LevelData&&) = default;

	// Interior level. The .INF is obtained from the level's info member.
	static LevelData loadInterior(const MIFFile::Level &level, SNInt gridWidth,
		WEInt gridDepth, const ExeData &exeData);

	// Interior dungeon level. Each chunk is determined by an "inner seed" which depends on the
	// dungeon level count being calculated beforehand.
	static LevelData loadDungeon(ArenaRandom &random, const MIFFile &mif,
		int levelUpBlock, const int *levelDownBlock, int widthChunks, int depthChunks,
		const std::string &infName, SNInt gridWidth, WEInt gridDepth, const ExeData &exeData);

	// Exterior level with a pre-defined .INF file. If premade, this loads the premade city. Otherwise,
	// this loads the skeleton of the level (city walls, etc.), and fills in the rest by generating
	// the required chunks.
	static LevelData loadCity(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
		const MIFFile::Level &level, WeatherType weatherType, int currentDay, int starCount,
		const std::string &infName, SNInt gridWidth, WEInt gridDepth, const BinaryAssetLibrary &binaryAssetLibrary,
		const TextAssetLibrary &textAssetLibrary, TextureManager &textureManager);

	// Exterior wilderness level with a pre-defined .INF file. This loads the skeleton of the wilderness
	// and fills in the rest by loading the required .RMD chunks.
	static LevelData loadWilderness(const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef,
		WeatherType weatherType, int currentDay, int starCount, const std::string &infName,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager);

	const std::string &getName() const;
	double getCeilingHeight() const;
	std::vector<FlatDef> &getFlats();
	const std::vector<FlatDef> &getFlats() const;
	VoxelInstanceGroup *tryGetVoxelInstances(const ChunkInt2 &chunk);
	const VoxelInstanceGroup *tryGetVoxelInstances(const ChunkInt2 &chunk) const;

	// Convenience function that does the chunk look-up internally.
	VoxelInstance *tryGetVoxelInstance(const NewInt3 &voxel, VoxelInstance::Type type);
	const VoxelInstance *tryGetVoxelInstance(const NewInt3 &voxel, VoxelInstance::Type type) const;

	Transitions &getTransitions();
	const Transitions &getTransitions() const;
	const INFFile &getInfFile() const;
	EntityManager &getEntityManager();
	const EntityManager &getEntityManager() const;
	VoxelGrid &getVoxelGrid();
	const VoxelGrid &getVoxelGrid() const;

	// Returns a pointer to some lock if the given voxel has a lock, or null if it doesn't.
	const Lock *getLock(const NewInt2 &voxel) const;

	// Returns a pointer to some trigger text if the given voxel has a text trigger, or null if it doesn't.
		// Also returns a pointer to one-shot text triggers that have been activated previously (use another
		// function to check activation).
	LevelData::TextTrigger *getTextTrigger(const NewInt2 &voxel);

	// Returns a pointer to a sound filename if the given voxel has a sound trigger, or null if it doesn't.
	const std::string *getSoundTrigger(const NewInt2 &voxel) const;

	// Only for exterior levels. Gets the mappings of voxel coordinates to *MENU display names.
	const ArenaLevelUtils::MenuNamesList &getMenuNames() const;

	// Returns whether a level is considered an outdoor dungeon. Only true for some interiors.
	bool isOutdoorDungeon() const;

	void addVoxelInstance(VoxelInstance &&voxelInst);

	// Removes all voxel instances not stored between level transitions (open doors, fading voxels).
	void clearTemporaryVoxelInstances();

	// Sets this level active in the renderer.
	void setActive(bool nightLightsAreActive, const WorldData &worldData,
		const ProvinceDefinition &provinceDef, const LocationDefinition &locationDef,
		const EntityDefinitionLibrary &entityDefLibrary, const CharacterClassLibrary &charClassLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, Random &random, CitizenManager &citizenManager,
		TextureManager &textureManager, Renderer &renderer);

	// Ticks the level data by delta time. Does nothing by default.
	void tick(Game &game, double dt);
};

#endif
