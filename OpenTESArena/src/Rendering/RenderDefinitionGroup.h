#ifndef RENDER_DEFINITION_GROUP_H
#define RENDER_DEFINITION_GROUP_H

#include "EntityRenderDefinition.h"
#include "SkyObjectRenderDefinition.h"
#include "VoxelRenderDefinition.h"

// Contains render definition data for shared voxel/entity/sky-object data.

// It's useful to generate more data than may seem useful in case of render features like shadows
// that frequently need off-screen data.

class RenderDefinitionGroup
{
private:
	// @todo: collections of voxel/entity/sky-object render definitions
	// - might have ChunkRenderDefinition for voxels, or not if an array is fine (indexable by
	//   voxel render instances).
public:

};

#endif
