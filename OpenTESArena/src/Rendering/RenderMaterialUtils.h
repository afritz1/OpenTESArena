#ifndef RENDER_MATERIAL_UTILS_H
#define RENDER_MATERIAL_UTILS_H

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

	bool operator==(const RenderMaterialKey &other) const;

	void init(VertexShaderType vertexShaderType, PixelShaderType pixelShaderType, Span<const ObjectTextureID> textureIDs,
		RenderLightingType lightingType, bool enableBackFaceCulling, bool enableDepthRead, bool enableDepthWrite);
};

struct RenderMaterial
{
	RenderMaterialKey key;
	RenderMaterialID id;

	RenderMaterial();

	void init(RenderMaterialKey key, RenderMaterialID id);
};

#endif
