#ifndef SCENE_GRAPH_H
#define SCENE_GRAPH_H

#include <vector>

#include "ChunkRenderDefinition.h"
#include "ChunkRenderInstance.h"
#include "EntityRenderDefinition.h"
#include "EntityRenderInstance.h"
#include "SceneGraphChunk.h"
#include "SkyObjectRenderDefinition.h"
#include "SkyObjectRenderInstance.h"
#include "VoxelRenderDefinition.h"
#include "../RenderTriangle.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"

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

class ChunkManager;
class EntityDefinitionLibrary;
class EntityManager;
class LevelInstance;
class SkyInstance;

struct RenderCamera;

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

	std::vector<SkyObjectRenderDefinition> skyObjectRenderDefs; // Shared render data for identical-looking sky objects.
	std::vector<SkyObjectRenderInstance> skyObjectRenderInsts; // Instances of every renderable sky object in the scene.

	// @todo: map EntityID to EntityRenderInstance for updating/freeing?
	// @todo: might need some key -> value structure for determining how voxels/entities/sky map to render defs

	// Chunks with data for geometry storage, visibility calculation, etc..
	std::vector<SceneGraphChunk> graphChunks;

	// @todo: buffer(s) of visible geometry, lights, indices, texture IDs, etc.
	// - sort of thinking like: static voxel geometry, dynamic voxel geometry, entity geometry, ...
	// - presumably each open door voxel, etc. would be in its own separate draw list (can't remember how OpenGL would handle something like this. Uniforms?)
	std::vector<RenderTriangle> opaqueVoxelTriangles;
	std::vector<RenderTriangle> alphaTestedVoxelTriangles;

	std::vector<RenderTriangle> entityTriangles;
	std::vector<RenderTriangle> skyTriangles; // @todo: will the main difference with opaque voxel geometry be the lack of depth writes?

	void clearVoxels(bool includeGraphChunks);
	void clearEntities();
	void clearSky();
public:
	// Visible geometry getters. These should only provide geometry that touch the view frustum.
	BufferView<const RenderTriangle> getVisibleOpaqueVoxelGeometry() const;
	BufferView<const RenderTriangle> getVisibleAlphaTestedVoxelGeometry() const;
	BufferView<const RenderTriangle> getVisibleEntityGeometry() const;

	void updateVoxels(const LevelInstance &levelInst, const RenderCamera &camera, double ceilingScale,
		double chasmAnimPercent, bool nightLightsAreActive);
	void updateEntities(const LevelInstance &levelInst, const CoordDouble3 &cameraPos, const VoxelDouble3 &cameraDir,
		const EntityDefinitionLibrary &entityDefLibrary, double ceilingScale, bool nightLightsAreActive, bool playerHasLight);
	void updateSky(const SkyInstance &skyInst, double daytimePercent, double latitude);

	// Evaluates the scene graph's internal representation of voxels/entities/sky/etc. to re-populate its draw call lists.
	//void updateVisibleGeometry(const RenderCamera &camera);

	// Clears all state from the game world (used on scene changes).
	void clear();
};

#endif
