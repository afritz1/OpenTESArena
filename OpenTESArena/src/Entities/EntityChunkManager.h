#ifndef ENTITY_CHUNK_MANAGER_H
#define ENTITY_CHUNK_MANAGER_H

#include "components/utilities/BufferView.h"

#include "EntityChunk.h"
#include "EntityInstance.h"
#include "../World/SpecializedChunkManager.h"

class Renderer;
class TextureManager;
class VoxelChunk;
class VoxelChunkManager;

class EntityChunkManager final : public SpecializedChunkManager<EntityChunk>
{
private:
	// @todo: entity instance data for all entities
	// @todo: chunks that reference those entities

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

	/*void update(double dt, const CoordDouble3 &playerCoord, const std::optional<int> &activeLevelIndex,
		const MapDefinition &mapDefinition, const EntityGeneration::EntityGenInfo &entityGenInfo,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, double ceilingScale,
		int chunkDistance, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, AudioManager &audioManager,
		EntityManager &entityManager);*/
};

#endif
