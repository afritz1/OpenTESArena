#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include <memory>
#include <optional>
#include <vector>

#include "Chunk.h"
#include "ChunkUtils.h"
#include "VoxelUtils.h"
#include "../Entities/CitizenUtils.h"
#include "../Entities/EntityGeneration.h"

// Handles lifetimes of chunks. Does not store any entities. When freeing a chunk, it needs to tell
// the entity manager so the entities in it are handled correctly (marked for deletion one way or
// another).

class AudioManager;
class BinaryAssetLibrary;
class EntityDefinitionLibrary;
class EntityManager;
class Game;
class LevelDefinition;
class LevelInfoDefinition;
class MapDefinition;
class TextureManager;

enum class MapType;

class ChunkManager
{
private:
	using ChunkPtr = std::unique_ptr<Chunk>;

	std::vector<ChunkPtr> chunkPool;
	std::vector<ChunkPtr> activeChunks;
	ChunkInt2 centerChunkPos;

	template <typename VoxelIdType>
	using VoxelIdFunc = VoxelIdType(*)(const Chunk &chunk, const VoxelInt3 &voxel);

	// Gets the voxel definitions adjacent to a voxel. Useful with context-sensitive voxels like chasms.
	template <typename VoxelIdType>
	void getAdjacentVoxelIDsInternal(const CoordInt3 &coord, VoxelIdFunc<VoxelIdType> voxelIdFunc, VoxelIdType defaultID,
		std::optional<int> *outNorthChunkIndex, std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex,
		std::optional<int> *outWestChunkIndex, VoxelIdType *outNorthID, VoxelIdType *outEastID, VoxelIdType *outSouthID,
		VoxelIdType *outWestID);
	void getAdjacentVoxelMeshDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
		std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
		Chunk::VoxelMeshDefID *outNorthID, Chunk::VoxelMeshDefID *outEastID, Chunk::VoxelMeshDefID *outSouthID,
		Chunk::VoxelMeshDefID *outWestID);
	void getAdjacentVoxelTextureDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
		std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
		Chunk::VoxelTextureDefID *outNorthID, Chunk::VoxelTextureDefID *outEastID, Chunk::VoxelTextureDefID *outSouthID,
		Chunk::VoxelTextureDefID *outWestID);
	void getAdjacentVoxelTraitsDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
		std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
		Chunk::VoxelTraitsDefID *outNorthID, Chunk::VoxelTraitsDefID *outEastID, Chunk::VoxelTraitsDefID *outSouthID,
		Chunk::VoxelTraitsDefID *outWestID);

	// Takes a chunk from the chunk pool, moves it to the active chunks, and returns its index.
	int spawnChunk();

	// Clears the chunk and removes it from the active chunks.
	void recycleChunk(int index);

	// Helper function for setting the chunk's voxel definitions.
	void populateChunkVoxelDefs(Chunk &chunk, const LevelInfoDefinition &levelInfoDefinition);

	// Helper function for setting the chunk's voxels for the given level. This might not touch all voxels
	// in the chunk because it does not fully overlap the level.
	void populateChunkVoxels(Chunk &chunk, const LevelDefinition &levelDefinition,
		const LevelInt2 &levelOffset);

	// Helper function for setting the chunk's secondary voxel data (transitions, triggers, etc.).
	void populateChunkDecorators(Chunk &chunk, const LevelDefinition &levelDefinition,
		const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset);

	// Helper function for setting a wild chunk's building names.
	void populateWildChunkBuildingNames(Chunk &chunk, const MapGeneration::WildChunkBuildingNameInfo &buildingNameInfo,
		const LevelInfoDefinition &levelInfoDefinition);

	// Adds any voxel instances to a chunk that should exist at level generation time. Mostly intended for chasms.
	void populateChunkVoxelInsts(Chunk &chunk);

	// Adds entities from the level to the chunk.
	void populateChunkEntities(Chunk &chunk, const LevelDefinition &levelDefinition,
		const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset,
		const EntityGeneration::EntityGenInfo &entityGenInfo,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, EntityManager &entityManager);

	// Fills the chunk with the data required based on its position and the world type.
	void populateChunk(int index, const ChunkInt2 &chunkPos, const std::optional<int> &activeLevelIndex,
		const MapDefinition &mapDefinition, const EntityGeneration::EntityGenInfo &entityGenInfo,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, EntityManager &entityManager);

	// Updates context-sensitive voxels (such as chasms) on a chunk's perimeter that may be affected by
	// adjacent chunks.
	void updateChunkPerimeter(Chunk &chunk);
public:
	int getChunkCount() const;
	Chunk &getChunk(int index);
	const Chunk &getChunk(int index) const;
	std::optional<int> tryGetChunkIndex(const ChunkInt2 &position) const;

	// Convenience functions for attempting to obtain a chunk from the given chunk coordinate.
	Chunk *tryGetChunk(const ChunkInt2 &position);
	const Chunk *tryGetChunk(const ChunkInt2 &position) const;

	// Index of the chunk all other active chunks surround.
	int getCenterChunkIndex() const;

	// Updates the chunk manager with the given chunk as the current center of the game world. This invalidates
	// all active chunk references and they must be looked up again. The 'updateChunkStates' parameter tells
	// whether to update the real-time state of chunks; this should be false during the frame of a level's
	// initialization, and true for all other cases (otherwise the world would be one update step ahead of the
	// player, which isn't a big deal but is poor design).
	void update(double dt, const ChunkInt2 &centerChunkPos, const CoordDouble3 &playerCoord,
		const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
		const EntityGeneration::EntityGenInfo &entityGenInfo,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, double ceilingScale,
		int chunkDistance, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, AudioManager &audioManager,
		EntityManager &entityManager);

	// Run at the end of a frame to reset certain frame data like dirty voxels.
	void cleanUp();
};

#endif
