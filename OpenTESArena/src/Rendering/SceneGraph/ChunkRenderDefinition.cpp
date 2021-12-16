#include "ChunkRenderDefinition.h"

void ChunkRenderDefinition::init(SNInt width, int height, WEInt depth)
{
	this->voxelRenderDefIDs.init(width, height, depth);
	this->voxelRenderDefIDs.fill(0);
}
