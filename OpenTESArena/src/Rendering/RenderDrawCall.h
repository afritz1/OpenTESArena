#ifndef RENDER_DRAW_CALL_H
#define RENDER_DRAW_CALL_H

#include <optional>

#include "RenderGeometryUtils.h"
#include "RenderLightUtils.h"
#include "RenderShaderUtils.h"
#include "RenderTextureUtils.h"
#include "../Math/Matrix4.h"
#include "../Math/Vector3.h"

struct RenderDrawCall
{
	static constexpr int MAX_TEXTURE_COUNT = 2; // For multi-texturing.

	UniformBufferID transformBufferID; // Translation/rotation/scale of this model
	int transformIndex;
	
	UniformBufferID preScaleTranslationBufferID; // Extra translation for some vertex shaders (currently shared by all raised doors).

	VertexBufferID vertexBufferID;
	AttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID indexBufferID;
	ObjectTextureID textureIDs[MAX_TEXTURE_COUNT];
	
	RenderLightingType lightingType;
	double lightPercent; // For per-mesh lighting.
	RenderLightID lightIDs[RenderLightIdList::MAX_LIGHTS]; // For per-pixel lighting.
	int lightIdCount;

	VertexShaderType vertexShaderType;
	PixelShaderType pixelShaderType;
	double pixelShaderParam0; // For specialized values like texture coordinate manipulation.

	bool enableDepthRead;
	bool enableDepthWrite;

	RenderDrawCall();

	void clear();
};

#endif
