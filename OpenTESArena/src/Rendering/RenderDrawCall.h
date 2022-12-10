#ifndef RENDER_DRAW_CALL_H
#define RENDER_DRAW_CALL_H

#include <optional>

#include "RenderGeometryUtils.h"
#include "RenderShaderUtils.h"
#include "RenderTextureUtils.h"
#include "../Math/Vector3.h"

struct RenderDrawCall
{
	static constexpr int MAX_TEXTURE_COUNT = 2; // For multi-texturing.

	VertexBufferID vertexBufferID;
	AttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID indexBufferID;
	std::optional<ObjectTextureID> textureIDs[MAX_TEXTURE_COUNT];
	TextureSamplingType textureSamplingType;
	VertexShaderType vertexShaderType;
	PixelShaderType pixelShaderType;
	Double3 worldSpaceOffset;

	RenderDrawCall();

	void clear();
};

#endif
