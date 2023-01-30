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

#include "components/utilities/BufferView.h"
#include "components/utilities/RecyclablePool.h"

class AudioManager;
class BinaryAssetLibrary;
class EntityDefinitionLibrary;
class LevelInfoDefinition;
class MapDefinition;
class Renderer;
class TextureManager;
class VoxelChunk;
class VoxelChunkManager;

class EntityChunkManager final : public SpecializedChunkManager<EntityChunk>
{
private:
	using EntityPool = RecyclablePool<EntityInstance, EntityInstanceID>;
	using EntityPositionPool = RecyclablePool<CoordDouble2, EntityPositionID>;
	using EntityDirectionPool = RecyclablePool<VoxelDouble2, EntityDirectionID>;
	using EntityAnimationInstancePool = RecyclablePool<EntityAnimationInstanceA, EntityAnimationInstanceID>;
	using EntityCreatureSoundPool = RecyclablePool<double, EntityCreatureSoundInstanceID>;
	using EntityPaletteInstancePool = RecyclablePool<Palette, EntityPaletteInstanceID>;

	EntityPool entities;
	EntityPositionPool positions;
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

	// Allocated textures for each entity definition's animations.
	std::unordered_map<EntityDefID, std::vector<ScopedObjectTextureRef>> animTextureRefs;

	const EntityDefinition &getEntityDef(EntityDefID defID, const EntityDefinitionLibrary &defLibrary) const;
	EntityDefID addEntityDef(EntityDefinition &&def, const EntityDefinitionLibrary &defLibrary);

	EntityInstanceID spawnEntity();

	void populateChunkEntities(EntityChunk &entityChunk, const VoxelChunk &chunk, const LevelDefinition &levelDefinition,
		const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset,
		const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
		Random &random, const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager);
	void populateChunk(EntityChunk &entityChunk, const VoxelChunk &voxelChunk, const std::optional<int> &activeLevelIndex,
		const MapDefinition &mapDefinition, const EntityGeneration::EntityGenInfo &entityGenInfo,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, double ceilingScale,
		Random &random, const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, Renderer &renderer);

	std::string getCreatureSoundFilename(const EntityDefID defID, const EntityDefinitionLibrary &entityDefLibrary) const;
	void updateCreatureSounds(double dt, EntityChunk &entityChunk, const CoordDouble3 &playerCoord,
		double ceilingScale, Random &random, const EntityDefinitionLibrary &entityDefLibrary, AudioManager &audioManager);
public:
	void update(double dt, const BufferView<const ChunkInt2> &activeChunkPositions,
		const BufferView<const ChunkInt2> &newChunkPositions, const BufferView<const ChunkInt2> &freedChunkPositions,
		const CoordDouble3 &playerCoord, const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
		const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
		double ceilingScale, Random &random, const VoxelChunkManager &voxelChunkManager, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, AudioManager &audioManager, TextureManager &textureManager, Renderer &renderer);

	// @todo: support spawning an entity not from the level def
};

#endif
