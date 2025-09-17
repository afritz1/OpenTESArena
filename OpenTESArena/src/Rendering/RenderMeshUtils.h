#ifndef RENDER_MESH_UTILS_H
#define RENDER_MESH_UTILS_H

#include "RenderShaderUtils.h"
#include "../Math/Matrix4.h"

#include "components/utilities/FixedPool.h"

// Unique ID for a mesh allocated in the renderer's internal format.
using VertexPositionBufferID = int;

// Unique ID for mesh attributes allocated in the renderer's internal format.
using VertexAttributeBufferID = int;

// Unique ID for a set of mesh indices allocated in the renderer's internal format.
using IndexBufferID = int;

// One per uniform buffer.
struct RenderTransformHeap
{
	static constexpr int MAX_TRANSFORMS = 8192;

	UniformBufferID uniformBufferID;
	FixedPool<Matrix4d, MAX_TRANSFORMS> pool; // Copied into uniform buffer every frame.

	RenderTransformHeap();

	int alloc();
	void free(int transformIndex);
	void clear();
};

#endif
