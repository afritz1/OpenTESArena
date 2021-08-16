#ifndef RENDERER_SYSTEM_2D_H
#define RENDERER_SYSTEM_2D_H

#include <cstdint>
#include <optional>

#include "RenderTextureUtils.h"
#include "../Math/Vector2.h"
#include "../Media/Palette.h"
#include "../Media/TextureUtils.h"

#include "components/utilities/BufferView2D.h"

// Abstract base class for UI renderer.

// @todo: finish designing Renderer/RendererSystem2D/RendererSystem3D interconnect.
// - Renderer has "the screen frame buffer" via SDL_Window probably.
// - RendererSystem3D has a "game world frame buffer" that's copied to "the screen" when done.
// - RendererSystem2D draws to "the screen frame buffer" after RendererSystem3D.
// - Move most of Renderer SDL drawing code to SdlRenderer2D and call renderer2D->draw(...) in Renderer functions.
// - Renderer::renderer could probably be moved to SdlRenderer2D. Renderer probably just needs the SDL_Window.

// @todo: might eventually need some "shared" struct for resources to talk between 2D and 3D renderer
// if it's the same backend.

class TextureManager;

enum class RenderSpace;

struct SDL_Window;

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
	virtual bool tryCreateUiTexture(const BufferView2D<const uint32_t> &texels, UiTextureID *outID) = 0;
	virtual bool tryCreateUiTexture(const BufferView2D<const uint8_t> &texels, const Palette &palette, UiTextureID *outID) = 0;
	virtual bool tryCreateUiTexture(TextureBuilderID textureBuilderID, PaletteID paletteID,
		const TextureManager &textureManager, UiTextureID *outID) = 0;

	// Texture handle freeing function for a UI texture.
	virtual void freeUiTexture(UiTextureID id) = 0;

	// Returns the texture's dimensions, if it exists.
	virtual std::optional<Int2> tryGetTextureDims(UiTextureID id) const = 0;

	// Drawing method for UI elements. Positions and sizes are in 0->1 vector space so that the caller's
	// data is resolution-independent.
	virtual void draw(const RenderElement *elements, int count, RenderSpace renderSpace) = 0;
};

#endif
