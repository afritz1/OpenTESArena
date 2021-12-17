#include "RectangleRenderDefinition.h"

void RectangleRenderDefinition::init(const Quad &quad, ObjectTextureID textureID, AlphaType alphaType)
{
	this->quad = quad;
	this->textureID = textureID;
	this->alphaType = alphaType;
}

void RectangleRenderDefinition::init(const Quad &quad, ObjectTextureID textureID)
{
	this->init(quad, textureID, AlphaType::Opaque);
}
