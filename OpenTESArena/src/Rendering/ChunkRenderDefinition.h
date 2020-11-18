#ifndef CHUNK_RENDER_DEFINITION_H
#define CHUNK_RENDER_DEFINITION_H

#include <cstdint>
#include <type_traits>
#include <vector>

#include "VoxelRenderDefinition.h"
#include "../World/ChunkUtils.h"
#include "../World/VoxelUtils.h"

#include "components/utilities/Buffer3D.h"

using VoxelRenderDefID = int16_t;

class ChunkRenderDefinition
{
private:
	static_assert(std::is_signed_v<VoxelRenderDefID>);

	std::vector<VoxelRenderDefinition> voxelRenderDefs;
	Buffer3D<VoxelRenderDefID> voxelRenderDefIDs; // Points into defs list.
	ChunkInt2 coord;
public:
	// Indicates nothing to render in a voxel.
	static constexpr VoxelRenderDefID NO_VOXEL_ID = -1;

	void init(SNInt width, int height, WEInt depth, const ChunkInt2 &coord);

	const ChunkInt2 &getCoord() const;
	const VoxelRenderDefinition &getVoxelRenderDef(VoxelRenderDefID id) const;

	SNInt getWidth() const;
	int getHeight() const;
	WEInt getDepth() const;
	VoxelRenderDefID getVoxelRenderDefID(SNInt x, int y, WEInt z) const;

	VoxelRenderDefID addVoxelRenderDef(VoxelRenderDefinition &&def);
	void clear();
};

#endif
