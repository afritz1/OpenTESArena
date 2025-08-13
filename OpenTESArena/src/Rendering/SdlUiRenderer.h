#ifndef SDL_UI_RENDERER_H
#define SDL_UI_RENDERER_H

#include <optional>

#include "RenderTextureAllocator.h"

#include "components/utilities/RecyclablePool.h"
#include "components/utilities/Span2D.h"

enum class RenderSpace;

struct Rect;
struct RenderElement2D;
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
	void free(UiTextureID textureID) override;

	std::optional<Int2> tryGetDimensions(UiTextureID textureID) const override;

	LockedTexture lock(UiTextureID textureID) override;
	void unlock(UiTextureID textureID) override;
};

class SdlUiRenderer
{
private:
	SDL_Renderer *renderer;
	SdlUiTexturePool texturePool;
	SdlUiTextureAllocator textureAllocator;
public:
	SdlUiRenderer();

	bool init(SDL_Window *window);
	void shutdown();
	
	UiTextureAllocator *getTextureAllocator();

	void draw(const RenderElement2D *elements, int count, RenderSpace renderSpace, const Rect &letterboxRect);
};

#endif
