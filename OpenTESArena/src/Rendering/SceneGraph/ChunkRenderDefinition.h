#ifndef CHUNK_RENDER_DEFINITION_H
#define CHUNK_RENDER_DEFINITION_H

#include <cstdint>

#include "VoxelRenderUtils.h"
#include "../../World/Coord.h"

#include "components/utilities/Buffer3D.h"

// This is most useful with large view distances where it's more likely there will be identical chunks
// being rendered. With smaller view distances, there will likely be no sharing between chunk instances.

struct ChunkRenderDefinition
{
	Buffer3D<VoxelRenderDefID> voxelRenderDefIDs; // Points into defs list.

	void init(SNInt width, int height, WEInt depth);
};

#endif
