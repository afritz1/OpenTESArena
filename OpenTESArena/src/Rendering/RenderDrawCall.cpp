#include "RenderDrawCall.h"

RenderDrawCall::RenderDrawCall()
{
	this->vertexBufferID = -1;
	this->normalBufferID = -1;
	this->texCoordBufferID = -1;
	this->indexBufferID = -1;
	this->vertexShaderType = static_cast<VertexShaderType>(-1);
	this->pixelShaderType = static_cast<PixelShaderType>(-1);
	this->allowBackFaces = false;
}

void RenderDrawCall::clear()
{
	this->vertexBufferID = -1;
	this->normalBufferID = -1;
	this->texCoordBufferID = -1;
	this->indexBufferID = -1;
	for (std::optional<ObjectTextureID> &textureID : this->textureIDs)
	{
		textureID = std::nullopt;
	}

	this->vertexShaderType = static_cast<VertexShaderType>(-1);
	this->pixelShaderType = static_cast<PixelShaderType>(-1);
	this->worldSpaceOffset = Double3::Zero;
	this->allowBackFaces = false;
}
