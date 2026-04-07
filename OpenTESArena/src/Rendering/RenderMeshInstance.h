#pragma once

#include "RenderMeshUtils.h"

class Renderer;

using RenderMeshInstID = int;

struct RenderMeshInstance
{
	VertexPositionBufferID positionBufferID;
	VertexAttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID indexBufferID;

	RenderMeshInstance();

	void freeBuffers(Renderer &renderer);
};
