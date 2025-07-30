#ifndef RENDERER_SYSTEM_2D_H
#define RENDERER_SYSTEM_2D_H

#include <cstdint>
#include <optional>

#include "RenderTextureUtils.h"
#include "../Assets/TextureUtils.h"
#include "../Math/Vector2.h"
#include "../Utilities/Palette.h"

#include "components/utilities/Span2D.h"

class TextureManager;

enum class RenderSpace;

struct Rect;
struct SDL_Window;

// Abstract base class for UI renderer.
//
// @todo: finish designing Renderer/RendererSystem2D/RendererSystem3D interconnect.
// - Renderer has "the screen frame buffer" via SDL_Window probably.
// - RendererSystem3D has a "game world frame buffer" that's copied to "the screen" when done.
// - RendererSystem2D draws to "the screen frame buffer" after RendererSystem3D.
// - Move most of Renderer SDL drawing code to SdlRenderer2D and call renderer2D->draw(...) in Renderer functions.
// - Renderer::renderer could probably be moved to SdlRenderer2D. Renderer probably just needs the SDL_Window.
//
// @todo: might eventually need some "shared" struct for resources to talk between 2D and 3D renderer
// if it's the same backend.
class RendererSystem2D
{
public:
	struct RenderElement
	{
		UiTextureID id;
		double x, y; // X and Y percents across the render space.
		double width, height; // Percents of render space dimensions.
		// @todo: optional shading/blending parameters? SDL_BlendMode? Alpha percent?

		RenderElement(UiTextureID id, double x, double y, double width, double height);
	};

	virtual ~RendererSystem2D();

	virtual bool init(SDL_Window *window) = 0;
	virtual void shutdown() = 0;

	// Texture handle allocation functions for a UI texture. All UI textures are stored as 32-bit.
	virtual UiTextureID createUiTexture(int width, int height) = 0;
	virtual UiTextureID createUiTexture(Span2D<const uint32_t> texels) = 0;
	virtual UiTextureID createUiTexture(Span2D<const uint8_t> texels, const Palette &palette) = 0;
	virtual UiTextureID createUiTexture(TextureBuilderID textureBuilderID, PaletteID paletteID, const TextureManager &textureManager) = 0;

	virtual uint32_t *lockUiTexture(UiTextureID textureID) = 0;
	virtual void unlockUiTexture(UiTextureID textureID) = 0;

	// Texture handle freeing function for a UI texture.
	virtual void freeUiTexture(UiTextureID id) = 0;

	// Returns the texture's dimensions, if it exists.
	virtual std::optional<Int2> tryGetTextureDims(UiTextureID id) const = 0;

	// Drawing method for UI elements. Positions and sizes are in 0->1 vector space so that the caller's
	// data is resolution-independent.
	virtual void draw(const RenderElement *elements, int count, RenderSpace renderSpace, const Rect &letterboxRect) = 0;
};

#endif
