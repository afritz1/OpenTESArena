#ifndef SDL_UI_RENDERER_H
#define SDL_UI_RENDERER_H

#include <unordered_map>

#include "RendererSystem2D.h"

struct SDL_Renderer;
struct SDL_Texture;

class SdlUiRenderer : public RendererSystem2D
{
private:
	SDL_Renderer *renderer;
	std::unordered_map<UiTextureID, SDL_Texture*> textures;
	UiTextureID nextID;

	template <typename TexelType>
	bool tryCreateUiTextureInternal(const BufferView2D<TexelType> &srcTexels, const Palette *palette,
		UiTextureID *outID);
public:
	SdlUiRenderer();

	bool init(SDL_Window *window) override;
	void shutdown() override;

	bool tryCreateUiTexture(const BufferView2D<const uint32_t> &texels, UiTextureID *outID) override;
	bool tryCreateUiTexture(const BufferView2D<const uint8_t> &texels, const Palette &palette, UiTextureID *outID) override;
	bool tryCreateUiTexture(TextureBuilderID textureBuilderID, PaletteID paletteID,
		const TextureManager &textureManager, UiTextureID *outID) override;
	void freeUiTexture(UiTextureID id) override;
	std::optional<Int2> tryGetTextureDims(UiTextureID id) const override;

	void draw(const RenderElement *elements, int count, RenderSpace renderSpace) override;
};

#endif
