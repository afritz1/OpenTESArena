#ifndef RENDER_DRAW_CALL_H
#define RENDER_DRAW_CALL_H

#include <optional>

#include "RenderGeometryUtils.h"
#include "RenderShaderUtils.h"
#include "RenderTextureUtils.h"
#include "../Math/Matrix4.h"
#include "../Math/Vector3.h"

struct RenderDrawCall
{
	static constexpr int MAX_TEXTURE_COUNT = 2; // For multi-texturing.

	Double3 position;
	Matrix4d rotation;
	VertexBufferID vertexBufferID;
	AttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID indexBufferID;
	std::optional<ObjectTextureID> textureIDs[MAX_TEXTURE_COUNT];
	TextureSamplingType textureSamplingType;
	VertexShaderType vertexShaderType;
	PixelShaderType pixelShaderType;

	RenderDrawCall();

	void clear();
};

#endif
