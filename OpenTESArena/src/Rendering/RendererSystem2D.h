#ifndef RENDERER_SYSTEM_2D_H
#define RENDERER_SYSTEM_2D_H

#include <optional>

#include "RenderTextureUtils.h"
#include "../Math/Vector2.h"

// Abstract base class for UI renderer.

// @todo: might eventually need some "shared" struct for resources to talk between 2D and 3D renderer
// if it's the same backend.

class TextureBuilder;

struct SDL_Window;

class RendererSystem2D
{
public:
	enum class RenderSpace
	{
		Native, // Relative to the native window itself.
		Classic // Occupies a fixed-aspect-ratio portion of the native window.
	};

	struct RenderElement
	{
		UiTextureID id;
		double x, y; // X and Y percents across the render space.
		double width, height; // Percents of render space dimensions.
		// @todo: optional shading/blending parameters? SDL_BlendMode? Alpha percent?

		RenderElement(UiTextureID id, double x, double y, double width, double height);
	};

	virtual void init(SDL_Window *window) = 0;
	virtual void shutdown() = 0;

	// Texture handle allocation function for a UI texture.
	virtual std::optional<UiTextureID> tryCreateUiTexture(const TextureBuilder &textureBuilder) = 0;

	// Texture handle freeing function for a UI texture.
	virtual void freeUiTexture(UiTextureID id) = 0;

	// Returns the texture's dimensions, if it exists.
	virtual std::optional<Int2> tryGetTextureDims(UiTextureID id) const = 0;

	// Bulk drawing method for handling several UI elements.
	virtual void draw(const RenderElement *elements, int count, RenderSpace renderSpace) = 0;

	// Convenience methods for drawing a single texture.
	void draw(UiTextureID id, double x, double y, double width, double height, RenderSpace renderSpace);
	void draw(UiTextureID id, double x, double y, RenderSpace renderSpace);
};

#endif
