#ifndef ENTITY_CHUNK_MANAGER_H
#define ENTITY_CHUNK_MANAGER_H

#include "CitizenUtils.h"
#include "EntityAnimationDefinition.h"
#include "EntityAnimationInstance.h"
#include "EntityChunk.h"
#include "EntityGeneration.h"
#include "EntityInstance.h"
#include "EntityUtils.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/RecyclablePool.h"

class AudioManager;
class BinaryAssetLibrary;
class EntityDefinitionLibrary;
class LevelInfoDefinition;
class MapDefinition;
class Player;
class Renderer;
class TextureManager;
class VoxelChunk;
class VoxelChunkManager;

struct EntityVisibilityState2D;
struct EntityVisibilityState3D;

class EntityChunkManager final : public SpecializedChunkManager<EntityChunk>
{
private:
	using EntityPool = RecyclablePool<EntityInstance, EntityInstanceID>;
	using EntityPositionPool = RecyclablePool<CoordDouble2, EntityPositionID>;
	using EntityBoundingBoxPool = RecyclablePool<double, EntityBoundingBoxID>;
	using EntityDirectionPool = RecyclablePool<VoxelDouble2, EntityDirectionID>;
	using EntityAnimationInstancePool = RecyclablePool<EntityAnimationInstance, EntityAnimationInstanceID>;
	using EntityCreatureSoundPool = RecyclablePool<double, EntityCreatureSoundInstanceID>;
	using EntityPaletteInstancePool = RecyclablePool<Palette, EntityPaletteInstanceID>;

	EntityPool entities;
	EntityPositionPool positions;
	EntityBoundingBoxPool boundingBoxes;
	EntityDirectionPool directions;
	EntityAnimationInstancePool animInsts;
	EntityCreatureSoundPool creatureSoundInsts;

	// Each citizen has a unique palette in place of unique textures for memory savings. It was found
	// that hardly any citizen instances share textures due to variations in their random palette. As
	// a result, citizen textures will need to be 8-bit.
	EntityPaletteInstancePool palettes;

	// Entity definitions for this currently-active level. Their definition IDs CANNOT be assumed
	// to be zero-based because these are in addition to ones in the entity definition library.
	// @todo: separate EntityAnimationDefinition from EntityDefinition?
	std::unordered_map<EntityDefID, EntityDefinition> entityDefs;

	EntityDefID addEntityDef(EntityDefinition &&def, const EntityDefinitionLibrary &defLibrary);
	EntityDefID getOrAddEntityDefID(const EntityDefinition &def, const EntityDefinitionLibrary &defLibrary);

	EntityInstanceID spawnEntity();

	void populateChunkEntities(EntityChunk &entityChunk, const VoxelChunk &chunk, const LevelDefinition &levelDefinition,
		const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset,
		const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
		Random &random, const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, Renderer &renderer);
	void populateChunk(EntityChunk &entityChunk, const VoxelChunk &voxelChunk, const std::optional<int> &activeLevelIndex,
		const MapDefinition &mapDefinition, const EntityGeneration::EntityGenInfo &entityGenInfo,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, double ceilingScale,
		Random &random, const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, Renderer &renderer);

	void updateCitizenStates(double dt, EntityChunk &entityChunk, const CoordDouble2 &playerCoordXZ, bool isPlayerMoving,
		bool isPlayerWeaponSheathed, Random &random, const VoxelChunkManager &voxelChunkManager,
		const EntityDefinitionLibrary &entityDefLibrary);

	std::string getCreatureSoundFilename(const EntityDefID defID, const EntityDefinitionLibrary &entityDefLibrary) const;
	void updateCreatureSounds(double dt, EntityChunk &entityChunk, const CoordDouble3 &playerCoord,
		double ceilingScale, Random &random, const EntityDefinitionLibrary &entityDefLibrary, AudioManager &audioManager);
public:
	const EntityDefinition &getEntityDef(EntityDefID defID, const EntityDefinitionLibrary &defLibrary) const;
	const EntityInstance &getEntity(EntityInstanceID id) const;
	const CoordDouble2 &getEntityPosition(EntityPositionID id) const;
	double getEntityBoundingBox(EntityBoundingBoxID id) const;
	const VoxelDouble2 &getEntityDirection(EntityDirectionID id) const;
	const EntityAnimationInstance &getEntityAnimationInstance(EntityAnimationInstanceID id) const;
	const Palette &getEntityPalette(EntityPaletteInstanceID id) const;

	// Count functions for specialized entities.
	int getCountInChunkWithDirection(const ChunkInt2 &chunkPos) const;
	int getCountInChunkWithCreatureSound(const ChunkInt2 &chunkPos) const;
	int getCountInChunkWithPalette(const ChunkInt2 &chunkPos) const;

	// Gets the entity visibility data necessary for rendering and ray cast selection.
	void getEntityVisibilityState2D(EntityInstanceID id, const CoordDouble2 &eye2D,
		const EntityDefinitionLibrary &entityDefLibrary, EntityVisibilityState2D &outVisState) const;
	void getEntityVisibilityState3D(EntityInstanceID id, const CoordDouble2 &eye2D,
		double ceilingScale, const VoxelChunkManager &voxelChunkManager,
		const EntityDefinitionLibrary &entityDefLibrary, EntityVisibilityState3D &outVisState) const;

	void update(double dt, const BufferView<const ChunkInt2> &activeChunkPositions,
		const BufferView<const ChunkInt2> &newChunkPositions, const BufferView<const ChunkInt2> &freedChunkPositions,
		const Player &player, const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
		const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
		double ceilingScale, Random &random, const VoxelChunkManager &voxelChunkManager, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, AudioManager &audioManager, TextureManager &textureManager, Renderer &renderer);

	// @todo: support spawning an entity not from the level def
};

#endif
