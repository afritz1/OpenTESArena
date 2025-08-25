#include <algorithm>

#include "RenderMaterialUtils.h"

#include "components/debug/Debug.h"

RenderMaterialKey::RenderMaterialKey()
{
	this->vertexShaderType = static_cast<VertexShaderType>(-1);
	this->pixelShaderType = static_cast<PixelShaderType>(-1);

	std::fill(std::begin(this->textureIDs), std::end(this->textureIDs), -1);
	this->textureCount = 0;

	this->lightingType = static_cast<RenderLightingType>(-1);

	this->enableBackFaceCulling = false;
	this->enableDepthRead = false;
	this->enableDepthWrite = false;
}

void RenderMaterialKey::init(VertexShaderType vertexShaderType, PixelShaderType pixelShaderType, Span<const ObjectTextureID> textureIDs,
	RenderLightingType lightingType, bool enableBackFaceCulling, bool enableDepthRead, bool enableDepthWrite)
{
	this->vertexShaderType = vertexShaderType;
	this->pixelShaderType = pixelShaderType;

	DebugAssert(textureIDs.getCount() <= RenderMaterialKey::MAX_TEXTURE_COUNT);
	std::copy(textureIDs.begin(), textureIDs.end(), std::begin(this->textureIDs));
	this->textureCount = textureIDs.getCount();

	this->lightingType = lightingType;

	this->enableBackFaceCulling = enableBackFaceCulling;
	this->enableDepthRead = enableDepthRead;
	this->enableDepthWrite = enableDepthWrite;
}
