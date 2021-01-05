#ifndef RECTANGLE_RENDER_DEFINITION_H
#define RECTANGLE_RENDER_DEFINITION_H

#include <type_traits>

#include "RenderTextureUtils.h"

// Defines a rectangle and how to render it.

template <typename T>
class RectangleRenderDefinition
{
public:
	enum class AlphaType
	{
		Opaque,
		AlphaTested
	};
private:
	static_assert(std::is_integral_v<T>);

	// @todo: model space rectangle, independent of chunk space or ceiling height
	T textureID;
	AlphaType alphaType;
public:

};

using VoxelRectangleRenderDefinition = RectangleRenderDefinition<VoxelTextureID>;
using EntityRectangleRenderDefinition = RectangleRenderDefinition<EntityTextureID>;

#endif
