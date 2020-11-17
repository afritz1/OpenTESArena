#ifndef VOXEL_RENDER_DEFINITION_H
#define VOXEL_RENDER_DEFINITION_H

// Common voxel render data usable by all renderers. Can be pointed to by multiple voxel
// render instances. Each voxel render definition's coordinate is implicitly defined by its
// XYZ grid position in a chunk.

class VoxelRenderDefinition
{
private:
	// @todo: probably do want ChunkRenderDefinition/Instance after all for ChunkInt2 at least.

	// @todo: shared voxel render data a renderer would care about
	// - RectangleRenderDefinitions and indices arrays for which front faces are visible from
	//   each voxel face.
	// - Make a render utils function for converting +/- {x,y,z} face/enum to index (like sky octants).
public:

};

#endif
