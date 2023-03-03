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
	Double3 preScaleTranslation; // For scaling around arbitrary point.
	Matrix4d rotation, scale;
	VertexBufferID vertexBufferID;
	AttributeBufferID normalBufferID, texCoordBufferID;
	IndexBufferID indexBufferID;
	std::optional<ObjectTextureID> textureIDs[MAX_TEXTURE_COUNT];
	TextureSamplingType textureSamplingType0, textureSamplingType1;
	VertexShaderType vertexShaderType;
	PixelShaderType pixelShaderType;
	double pixelShaderParam0;

	RenderDrawCall();

	void clear();
};

#endif
