#ifndef SDL_UI_RENDERER_H
#define SDL_UI_RENDERER_H

#include <functional>
#include <unordered_map>

#include "RendererSystem2D.h"
#include "RenderTextureAllocator.h"

#include "components/utilities/RecyclablePool.h"
#include "components/utilities/Span2D.h"

struct Rect;
struct SDL_Renderer;
struct SDL_Texture;

using SdlUiTexturePool = RecyclablePool<UiTextureID, SDL_Texture*>;

struct SdlUiTextureAllocator final : public UiTextureAllocator
{
	SdlUiTexturePool *pool;
	SDL_Renderer *renderer;

	SdlUiTextureAllocator();

	void init(SdlUiTexturePool *pool, SDL_Renderer *renderer);

	UiTextureID create(int width, int height) override;
	UiTextureID create(Span2D<const uint32_t> texels) override;
	UiTextureID create(Span2D<const uint8_t> texels, const Palette &palette) override;
	UiTextureID create(TextureBuilderID textureBuilderID, PaletteID paletteID, const TextureManager &textureManager) override;

	void free(UiTextureID textureID) override;

	uint32_t *lock(UiTextureID textureID) override;
	void unlock(UiTextureID textureID) override;
};

class SdlUiRenderer final : public RendererSystem2D
{
private:
	SDL_Renderer *renderer;
	SdlUiTexturePool texturePool;
	SdlUiTextureAllocator textureAllocator;
public:
	SdlUiRenderer();

	bool init(SDL_Window *window) override;
	void shutdown() override;
	
	UiTextureAllocator *getTextureAllocator() override;
	std::optional<Int2> tryGetTextureDims(UiTextureID id) const override;

	void draw(const RenderElement *elements, int count, RenderSpace renderSpace, const Rect &letterboxRect) override;
};

#endif
