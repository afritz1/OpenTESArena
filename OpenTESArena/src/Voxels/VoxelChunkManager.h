#ifndef VOXEL_CHUNK_MANAGER_H
#define VOXEL_CHUNK_MANAGER_H

#include <memory>
#include <optional>
#include <vector>

#include "VoxelChunk.h"
#include "../World/Coord.h"

#include "components/utilities/BufferView.h"

class MapDefinition;

// Handles the lifetimes of voxel chunks. Relies on the base chunk manager for the active chunk coordinates.
class VoxelChunkManager
{
private:
	using ChunkPtr = std::unique_ptr<VoxelChunk>;

	std::vector<ChunkPtr> chunkPool;
	std::vector<ChunkPtr> activeChunks;

	template <typename VoxelIdType>
	using VoxelIdFunc = VoxelIdType(*)(const VoxelChunk &chunk, const VoxelInt3 &voxel);

	std::optional<int> tryGetChunkIndex(const ChunkInt2 &position) const;
	int getChunkIndex(const ChunkInt2 &position) const;
	VoxelChunk &getChunkAtIndex(int index);
	const VoxelChunk &getChunkAtIndex(int index) const;

	// Gets the voxel definitions adjacent to a voxel. Useful with context-sensitive voxels like chasms.
	template <typename VoxelIdType>
	void getAdjacentVoxelIDsInternal(const CoordInt3 &coord, VoxelIdFunc<VoxelIdType> voxelIdFunc, VoxelIdType defaultID,
		std::optional<int> *outNorthChunkIndex, std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex,
		std::optional<int> *outWestChunkIndex, VoxelIdType *outNorthID, VoxelIdType *outEastID, VoxelIdType *outSouthID,
		VoxelIdType *outWestID);
	void getAdjacentVoxelMeshDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
		std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
		VoxelChunk::VoxelMeshDefID *outNorthID, VoxelChunk::VoxelMeshDefID *outEastID, VoxelChunk::VoxelMeshDefID *outSouthID,
		VoxelChunk::VoxelMeshDefID *outWestID);
	void getAdjacentVoxelTextureDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
		std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
		VoxelChunk::VoxelTextureDefID *outNorthID, VoxelChunk::VoxelTextureDefID *outEastID, VoxelChunk::VoxelTextureDefID *outSouthID,
		VoxelChunk::VoxelTextureDefID *outWestID);
	void getAdjacentVoxelTraitsDefIDs(const CoordInt3 &coord, std::optional<int> *outNorthChunkIndex,
		std::optional<int> *outEastChunkIndex, std::optional<int> *outSouthChunkIndex, std::optional<int> *outWestChunkIndex,
		VoxelChunk::VoxelTraitsDefID *outNorthID, VoxelChunk::VoxelTraitsDefID *outEastID, VoxelChunk::VoxelTraitsDefID *outSouthID,
		VoxelChunk::VoxelTraitsDefID *outWestID);

	// Takes a chunk from the chunk pool, moves it to the active chunks, and returns its index.
	int spawnChunk();

	// Clears the chunk and removes it from the active chunks.
	void recycleChunk(int index);

	// Helper function for setting the chunk's voxel definitions.
	void populateChunkVoxelDefs(VoxelChunk &chunk, const LevelInfoDefinition &levelInfoDefinition);

	// Helper function for setting the chunk's voxels for the given level. This might not touch all voxels
	// in the chunk because it does not fully overlap the level.
	void populateChunkVoxels(VoxelChunk &chunk, const LevelDefinition &levelDefinition,
		const LevelInt2 &levelOffset);

	// Helper function for setting the chunk's secondary voxel data (transitions, triggers, etc.).
	void populateChunkDecorators(VoxelChunk &chunk, const LevelDefinition &levelDefinition,
		const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset);

	// Helper function for setting a wild chunk's building names.
	void populateWildChunkBuildingNames(VoxelChunk &chunk, const MapGeneration::WildChunkBuildingNameInfo &buildingNameInfo,
		const LevelInfoDefinition &levelInfoDefinition);

	// Adds chasm instances to the chunk that should exist at level generation time. Chasms are context-sensitive
	// to adjacent voxels so this function also operates based on adjacent chunks (if any).
	void populateChunkChasmInsts(VoxelChunk &chunk);

	// Fills the chunk with the data required based on its position and the world type.
	void populateChunk(int index, const ChunkInt2 &chunkPos, const std::optional<int> &activeLevelIndex,
		const MapDefinition &mapDefinition);

	// Updates chasms (context-sensitive voxels) on a chunk's perimeter that may be affected by adjacent chunks.
	void updateChunkPerimeterChasmInsts(VoxelChunk &chunk);
public:
	VoxelChunk *tryGetChunkAtPosition(const ChunkInt2 &position);
	const VoxelChunk *tryGetChunkAtPosition(const ChunkInt2 &position) const;

	VoxelChunk &getChunkAtPosition(const ChunkInt2 &position);
	const VoxelChunk &getChunkAtPosition(const ChunkInt2 &position) const;

	void update(double dt, const BufferView<const ChunkInt2> &newChunkPositions,
		const BufferView<const ChunkInt2> &freedChunkPositions, const CoordDouble3 &playerCoord,
		const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition, double ceilingScale,
		AudioManager &audioManager);

	// Run at the end of a frame to reset certain frame data like dirty voxels.
	void cleanUp();
};

#endif
