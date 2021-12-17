#ifndef RECTANGLE_RENDER_DEFINITION_H
#define RECTANGLE_RENDER_DEFINITION_H

#include <type_traits>

#include "../RenderTextureUtils.h"
#include "../../Math/Quad.h"

// Defines a rectangle whose rendering properties can be shared with any number of instances of geometry.

struct RectangleRenderDefinition
{
	enum class AlphaType
	{
		Opaque,
		AlphaTested
	};

	Quad quad; // Model-space rectangle.
	ObjectTextureID textureID;
	AlphaType alphaType;

	void init(const Quad &quad, ObjectTextureID textureID, AlphaType alphaType);
	void init(const Quad &quad, ObjectTextureID textureID);
};

#endif
