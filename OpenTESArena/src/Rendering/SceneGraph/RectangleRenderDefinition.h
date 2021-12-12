#ifndef RECTANGLE_RENDER_DEFINITION_H
#define RECTANGLE_RENDER_DEFINITION_H

#include <type_traits>

#include "../RenderTextureUtils.h"

// Defines a rectangle and how to render it.

class RectangleRenderDefinition
{
public:
	enum class AlphaType
	{
		Opaque,
		AlphaTested
	};
private:
	// @todo: model space rectangle, independent of chunk space or ceiling height
	ObjectTextureID textureID;
	AlphaType alphaType;
public:

};

#endif
