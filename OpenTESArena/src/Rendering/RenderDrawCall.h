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
	IndexBufferID indexBufferID;
	AttribBufferID attribBufferID;
	std::optional<ObjectTextureID> textureIDs[MAX_TEXTURE_COUNT];
	VertexShaderType vertexShaderType;
	PixelShaderType pixelShaderType;
	Double3 position; // World space offset.

	RenderDrawCall();

	void clear();
};

#endif
