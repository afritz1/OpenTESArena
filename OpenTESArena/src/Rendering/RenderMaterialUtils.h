#ifndef RENDER_MATERIAL_H
#define RENDER_MATERIAL_H

#include "RenderLightUtils.h"
#include "RenderShaderUtils.h"
#include "RenderTextureUtils.h"

#include "components/utilities/Span.h"

using RenderMaterialID = int;

struct RenderMaterialKey
{
	static constexpr int MAX_TEXTURE_COUNT = 2;

	VertexShaderType vertexShaderType;
	PixelShaderType pixelShaderType;

	ObjectTextureID textureIDs[MAX_TEXTURE_COUNT];
	int textureCount;

	RenderLightingType lightingType;

	bool enableBackFaceCulling;
	bool enableDepthRead;
	bool enableDepthWrite;

	RenderMaterialKey();

	void init(VertexShaderType vertexShaderType, PixelShaderType pixelShaderType, Span<const ObjectTextureID> textureIDs,
		RenderLightingType lightingType, bool enableBackFaceCulling, bool enableDepthRead, bool enableDepthWrite);
};

#endif
