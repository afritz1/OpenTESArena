#ifndef RENDER_DRAW_CALL_H
#define RENDER_DRAW_CALL_H

#include "RenderMaterialUtils.h"
#include "RenderMeshUtils.h"
#include "RenderShaderUtils.h"
#include "RenderTextureUtils.h"

struct RenderDrawCall
{
	UniformBufferID transformBufferID; // Translation/rotation/scale of this model.
	int transformIndex;

	VertexPositionBufferID positionBufferID;
	VertexAttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID indexBufferID;

	RenderMaterialID materialID;
	RenderMaterialInstanceID materialInstID;

	RenderMultipassType multipassType;

	RenderDrawCall();

	void clear();
};

#endif
