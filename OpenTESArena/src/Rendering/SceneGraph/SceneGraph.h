#ifndef SCENE_GRAPH_H
#define SCENE_GRAPH_H

#include <vector>

#include "ChunkRenderDefinition.h"
#include "ChunkRenderInstance.h"
#include "EntityRenderDefinition.h"
#include "EntityRenderInstance.h"
#include "VoxelRenderDefinition.h"

#include "components/utilities/Buffer.h"

// Objectives:
// - efficient visible voxel determination (use quadtree per chunk to find ranges of visible voxel columns)
// - efficient visible entity determination (don't have notion of a 'chunk' for entities yet, might want bounding box hierarchy per chunk?)
// - efficient visible sky determination (simple dot product >0 against camera direction at first? or use +/- sign of each star direction? camera can specify which +/- octants are visible to it)
// - put voxel/entity/sky render defs into a "batchable" position, although not required (just want to go in that direction)
// - make a generateVisibleDrawList() function for obtaining the actual ordered draw list of geometry with texture IDs and params for the renderer.
//   This will be implemented by evaluating hundreds/thousands of render def + inst pairs and obtaining sets of renderable geometry.

// Render definitions + instances
// - render definitions can be created on chunk instantiation and during gameplay
// - thousands of render instances can reference a small handful of render definitions for memory savings w/ big draw distance
// - render definitions + instances can be 'evaluated' to generate world space geometry + material/shader params
// - this class is effectively a "SceneRenderDefinition + SceneRenderInstance"

// Considerations for batching/ordering draw list by texture? That's kind of an advanced optimization.
// - combined with a functioning occlusion culling system or hierarchical Z buffer, this could probably work.
// - if anything, would probably order by render def IDs, not necessarily texture IDs

class SceneGraph
{
private:
	std::vector<VoxelRenderDefinition> voxelRenderDefs; // Pointed to by voxels in active chunks. Air is index 0.

	std::vector<ChunkRenderDefinition> chunkRenderDefs; // Contains voxel IDs that point to voxel render defs.
	std::vector<ChunkRenderInstance> chunkRenderInsts; // Actual chunk positions and "unique" voxel instances.

	std::vector<EntityRenderDefinition> entityRenderDefs; // Shared render data for identical-looking entities.
	std::vector<EntityRenderInstance> entityRenderInsts; // Instances of every renderable entity in the scene.
	// @todo: might group entity render defs into citizens vs. non-citizens, or static and dynamic, since citizens
	// and enemies are going to be changing animation frames frequently.

	// @todo: map EntityID to EntityRenderInstance for updating/freeing?
	// @todo: might need some key -> value structure for determining how voxels/entities/sky map to render defs
};

#endif
