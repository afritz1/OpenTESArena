#ifndef VOXEL_CHUNK_MANAGER_H
#define VOXEL_CHUNK_MANAGER_H

#include <memory>
#include <optional>
#include <vector>

#include "VoxelChunk.h"
#include "../World/Coord.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/BufferView.h"

struct MapSubDefinition;

// Handles the lifetimes of voxel chunks. Relies on the base chunk manager for the active chunk coordinates.
class VoxelChunkManager final : public SpecializedChunkManager<VoxelChunk>
{
private:
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

	// Helper function for setting the chunk's voxel definitions.
	void populateChunkVoxelDefs(VoxelChunk &chunk, const LevelDefinition &levelDefinition,
		const LevelInfoDefinition &levelInfoDefinition);

	// Helper function for setting the chunk's voxels for the given level. This might not touch all voxels
	// in the chunk because it does not fully overlap the level.
	void populateChunkVoxels(VoxelChunk &chunk, const LevelDefinition &levelDefinition,
		const WorldInt2 &levelOffset);

	// Helper function for setting the chunk's secondary voxel data (transitions, triggers, etc.).
	void populateChunkDecorators(VoxelChunk &chunk, const LevelDefinition &levelDefinition,
		const LevelInfoDefinition &levelInfoDefinition, const WorldInt2 &levelOffset);

	// Helper function for setting a wild chunk's building names.
	void populateWildChunkBuildingNames(VoxelChunk &chunk, const MapGeneration::WildChunkBuildingNameInfo &buildingNameInfo,
		const LevelInfoDefinition &levelInfoDefinition);

	// Adds chasm instances to the chunk that should exist at level generation time. Chasms are context-sensitive
	// to adjacent voxels so this function also operates based on adjacent chunks (if any).
	void populateChunkChasmInsts(VoxelChunk &chunk);

	// Adds door visibility instances to the chunk for determining which faces to render.
	void populateChunkDoorVisibilityInsts(VoxelChunk &chunk);

	// Fills the chunk with the data required based on its position and the world type.
	void populateChunk(int index, const ChunkInt2 &chunkPos, const LevelDefinition &levelDef, const LevelInfoDefinition &levelInfoDef,
		const MapSubDefinition &mapSubDef);

	// Updates a chasm (context-sensitive voxel) that may be affected by adjacent chunks.
	void updateChasmWallInst(VoxelChunk &chunk, SNInt x, int y, WEInt z);

	// Updates door visibilities for a chunk; some of which might be on the chunk's perimeter that are affected by adjacent chunks.
	void updateChunkDoorVisibilityInsts(VoxelChunk &chunk, const CoordDouble3 &playerCoord);
public:
	void update(double dt, BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
		const CoordDouble3 &playerCoord, const LevelDefinition *activeLevelDef, const LevelInfoDefinition *activeLevelInfoDef,
		const MapSubDefinition &mapSubDef, BufferView<const LevelDefinition> levelDefs,
		BufferView<const int> levelInfoDefIndices, BufferView<const LevelInfoDefinition> levelInfoDefs,
		double ceilingScale, AudioManager &audioManager);

	// Run at the end of a frame to reset certain frame data like dirty voxels.
	void cleanUp();
};

#endif
