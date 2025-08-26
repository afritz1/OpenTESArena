#ifndef RENDER_DRAW_CALL_H
#define RENDER_DRAW_CALL_H

#include <optional>

#include "RenderMaterialUtils.h"
#include "RenderMeshUtils.h"
#include "RenderShaderUtils.h"
#include "RenderTextureUtils.h"

struct RenderDrawCall
{
	UniformBufferID transformBufferID; // Translation/rotation/scale of this model
	int transformIndex;
	
	UniformBufferID preScaleTranslationBufferID; // Extra translation for some vertex shaders (currently shared by all raised doors).

	VertexPositionBufferID positionBufferID;
	VertexAttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID indexBufferID;

	RenderMaterialID materialID;

	RenderDrawCall();

	void clear();
};

#endif
