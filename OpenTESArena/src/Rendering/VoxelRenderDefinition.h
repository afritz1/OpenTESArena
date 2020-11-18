#ifndef VOXEL_RENDER_DEFINITION_H
#define VOXEL_RENDER_DEFINITION_H

#include <array>

#include "RectangleRenderDefinition.h"

// Common voxel render data usable by all renderers. Can be pointed to by multiple voxel
// render instances. Each voxel render definition's coordinate is implicitly defined by its
// XYZ grid position in a chunk.

class VoxelRenderDefinition
{
public:
	static constexpr int MAX_RECTS = 8; // Max number of rectangles in the voxel.
	static constexpr int FACES = 6; // Number of faces on the voxel.

	// Indices to front-facing rectangles relative to this face of the voxel.
	struct FaceIndicesDef
	{
		std::array<int, MAX_RECTS> indices;
		int count;
	};
private:
	// @todo: shared voxel render data a renderer would care about
	// - Make a render utils function for converting +/- {x,y,z} face/enum to index (like sky octants).
	std::array<VoxelRectangleRenderDefinition, MAX_RECTS> rects;
	std::array<FaceIndicesDef, FACES> faceIndices; // X: 0, 1; Y: 2, 3; Z: 4, 5.
public:

};

#endif
