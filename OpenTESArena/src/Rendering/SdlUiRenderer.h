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
public:
	void init(SDL_Window *window) override; // @todo: might also need target SDL_Texture*
	void shutdown() override;

	bool tryCreateUiTexture(const TextureAssetReference &textureAssetRef, TextureManager &textureManager) override;
	void freeUiTexture(const TextureAssetReference &textureAssetRef) override;
	std::optional<Int2> tryGetTextureDims(UiTextureID id) const override;

	void draw(const RenderElement *elements, int count, RenderSpace renderSpace) override;
};

#endif
