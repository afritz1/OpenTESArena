#ifndef ENTITY_CHUNK_MANAGER_H
#define ENTITY_CHUNK_MANAGER_H

#include "EntityAnimationDefinition.h"
#include "EntityAnimationInstance.h"
#include "EntityChunk.h"
#include "EntityInstance.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/BufferView.h"
#include "components/utilities/RecyclablePool.h"

class Renderer;
class TextureManager;
class VoxelChunk;
class VoxelChunkManager;

class EntityChunkManager final : public SpecializedChunkManager<EntityChunk>
{
private:
	using EntityPool = RecyclablePool<EntityInstance, EntityInstanceID>;
	using EntityPositionPool = RecyclablePool<CoordDouble3, EntityPositionID>;
	using EntityDirectionPool = RecyclablePool<VoxelDouble3, EntityDirectionID>;
	using EntityAnimationInstancePool = RecyclablePool<EntityAnimationInstance, EntityAnimationInstanceID>;

	EntityPool entities;
	EntityPositionPool positions;
	EntityDirectionPool directions;
	EntityAnimationInstancePool animInsts;
	
	// Entity definitions for this currently-active level. Their definition IDs CANNOT be assumed
	// to be zero-based because these are in addition to ones in the entity definition library.
	// @todo: separate EntityAnimationDefinition from EntityDefinition?
	std::unordered_map<EntityDefID, EntityDefinition> entityDefs;

	void populateChunk(EntityChunk &entityChunk, const VoxelChunk &voxelChunk, double ceilingScale, TextureManager &textureManager,
		Renderer &renderer);

	// Adds entities from the level to the chunk.
	/*void populateChunkEntities(VoxelChunk &chunk, const LevelDefinition &levelDefinition,
		const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset,
		const EntityGeneration::EntityGenInfo &entityGenInfo,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, EntityManager &entityManager);*/
public:
	void update(double dt, const BufferView<const ChunkInt2> &activeChunkPositions,
		const BufferView<const ChunkInt2> &newChunkPositions, const BufferView<const ChunkInt2> &freedChunkPositions,
		const CoordDouble3 &playerCoord, double ceilingScale, const VoxelChunkManager &voxelChunkManager,
		TextureManager &textureManager, Renderer &renderer);

	// @todo: support spawning an entity not from the level def

	/*void update(double dt, const CoordDouble3 &playerCoord, const std::optional<int> &activeLevelIndex,
		const MapDefinition &mapDefinition, const EntityGeneration::EntityGenInfo &entityGenInfo,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, double ceilingScale,
		int chunkDistance, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, AudioManager &audioManager,
		EntityManager &entityManager);*/
};

#endif
