#include "RenderDrawCall.h"

RenderDrawCall::RenderDrawCall()
{
	this->transformBufferID = -1;
	this->transformIndex = -1;
	this->preScaleTranslationBufferID = -1;
	this->vertexBufferID = -1;
	this->normalBufferID = -1;
	this->texCoordBufferID = -1;
	this->indexBufferID = -1;
	
	for (ObjectTextureID &textureID : this->textureIDs)
	{
		textureID = -1;
	}

	for (auto &ptr : this->varyingTextures)
	{
		ptr = nullptr;
	}

	for (TextureSamplingType &samplingType : this->textureSamplingTypes)
	{
		samplingType = static_cast<TextureSamplingType>(-1);
	}

	this->lightingType = static_cast<RenderLightingType>(-1);
	for (RenderLightID &lightID : this->lightIDs)
	{
		lightID = -1;
	}

	this->lightIdCount = 0;
	this->lightPercent = 0.0;
	this->vertexShaderType = static_cast<VertexShaderType>(-1);
	this->pixelShaderType = static_cast<PixelShaderType>(-1);
	this->pixelShaderParam0 = 0.0;
	this->enableDepthRead = false;
	this->enableDepthWrite = false;
}

void RenderDrawCall::clear()
{
	this->transformBufferID = -1;
	this->transformIndex = -1;
	this->preScaleTranslationBufferID = -1;
	this->vertexBufferID = -1;
	this->normalBufferID = -1;
	this->texCoordBufferID = -1;
	this->indexBufferID = -1;
	
	for (ObjectTextureID &textureID : this->textureIDs)
	{
		textureID = -1;
	}

	for (auto &ptr : this->varyingTextures)
	{
		ptr = nullptr;
	}

	for (TextureSamplingType &samplingType : this->textureSamplingTypes)
	{
		samplingType = static_cast<TextureSamplingType>(-1);
	}
	
	this->lightingType = static_cast<RenderLightingType>(-1);
	for (RenderLightID &lightID : this->lightIDs)
	{
		lightID = -1;
	}

	this->lightIdCount = 0;
	this->lightPercent = 0.0;
	this->vertexShaderType = static_cast<VertexShaderType>(-1);
	this->pixelShaderType = static_cast<PixelShaderType>(-1);
	this->pixelShaderParam0 = 0.0;
	this->enableDepthRead = false;
	this->enableDepthWrite = false;
}
