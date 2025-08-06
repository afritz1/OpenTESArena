#ifndef RENDERER_SYSTEM_2D_H
#define RENDERER_SYSTEM_2D_H

#include <optional>

#include "RenderTextureUtils.h"
#include "../Math/Vector2.h"

#include "components/utilities/Span2D.h"

class TextureManager;

enum class RenderSpace;

struct Rect;
struct SDL_Window;
struct UiTextureAllocator;

struct RenderElement2D
{
	UiTextureID id;
	double x, y; // X and Y percents across the render space.
	double width, height; // Percents of render space dimensions.
	// @todo: optional shading/blending parameters? SDL_BlendMode? Alpha percent?

	RenderElement2D(UiTextureID id, double x, double y, double width, double height);
};

// Abstract base class for UI renderer.
class RendererSystem2D
{
public:
	virtual ~RendererSystem2D();

	virtual bool init(SDL_Window *window) = 0;
	virtual void shutdown() = 0;

	// Texture management functions. All UI textures are stored as 32-bit.
	virtual UiTextureAllocator *getTextureAllocator() = 0;

	// Returns the texture's dimensions, if it exists.
	virtual std::optional<Int2> tryGetTextureDims(UiTextureID id) const = 0;

	// Drawing method for UI elements. Positions and sizes are in 0->1 vector space so that the caller's
	// data is resolution-independent.
	virtual void draw(const RenderElement2D *elements, int count, RenderSpace renderSpace, const Rect &letterboxRect) = 0;
};

#endif
