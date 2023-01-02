#include "RenderDrawCall.h"

RenderDrawCall::RenderDrawCall()
{
	this->vertexBufferID = -1;
	this->normalBufferID = -1;
	this->texCoordBufferID = -1;
	this->indexBufferID = -1;
	this->textureSamplingType = static_cast<TextureSamplingType>(-1);
	this->vertexShaderType = static_cast<VertexShaderType>(-1);
	this->pixelShaderType = static_cast<PixelShaderType>(-1);
	this->pixelShaderParam0 = 0.0;
}

void RenderDrawCall::clear()
{
	this->position = Double3::Zero;
	this->preScaleTranslation = Double3::Zero;
	this->rotation = Matrix4d();
	this->scale = Matrix4d();
	this->vertexBufferID = -1;
	this->normalBufferID = -1;
	this->texCoordBufferID = -1;
	this->indexBufferID = -1;
	for (std::optional<ObjectTextureID> &textureID : this->textureIDs)
	{
		textureID = std::nullopt;
	}

	this->textureSamplingType = static_cast<TextureSamplingType>(-1);
	this->vertexShaderType = static_cast<VertexShaderType>(-1);
	this->pixelShaderType = static_cast<PixelShaderType>(-1);
	this->pixelShaderParam0 = 0.0;
}
