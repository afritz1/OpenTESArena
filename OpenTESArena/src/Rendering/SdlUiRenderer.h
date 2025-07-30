#ifndef SDL_UI_RENDERER_H
#define SDL_UI_RENDERER_H

#include <functional>
#include <unordered_map>

#include "RendererSystem2D.h"

#include "components/utilities/Span2D.h"

struct Rect;
struct SDL_Renderer;
struct SDL_Texture;

class SdlUiRenderer : public RendererSystem2D
{
private:
	SDL_Renderer *renderer;
	std::unordered_map<UiTextureID, SDL_Texture*> textures;
	UiTextureID nextID;

	using TexelsInitFunc = std::function<void(Span2D<uint32_t>)>;
	UiTextureID createUiTextureInternal(int width, int height, const TexelsInitFunc &initFunc);
public:
	SdlUiRenderer();

	bool init(SDL_Window *window) override;
	void shutdown() override;

	UiTextureID createUiTexture(int width, int height) override;
	UiTextureID createUiTexture(Span2D<const uint32_t> texels) override;
	UiTextureID createUiTexture(Span2D<const uint8_t> texels, const Palette &palette) override;
	UiTextureID createUiTexture(TextureBuilderID textureBuilderID, PaletteID paletteID, const TextureManager &textureManager) override;

	uint32_t *lockUiTexture(UiTextureID textureID) override;
	void unlockUiTexture(UiTextureID textureID) override;

	void freeUiTexture(UiTextureID id) override;

	std::optional<Int2> tryGetTextureDims(UiTextureID id) const override;

	void draw(const RenderElement *elements, int count, RenderSpace renderSpace, const Rect &letterboxRect) override;
};

#endif
