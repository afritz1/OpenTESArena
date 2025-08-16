#ifndef SDL_UI_RENDERER_H
#define SDL_UI_RENDERER_H

#include <optional>

#include "components/utilities/RecyclablePool.h"
#include "components/utilities/Span2D.h"

enum class RenderSpace;

struct Rect;
struct RenderElement2D;
struct SDL_Renderer;
struct SDL_Texture;

using SdlUiTexturePool = RecyclablePool<UiTextureID, SDL_Texture*>;

class SdlUiRenderer
{
private:
	SDL_Renderer *renderer;
	SdlUiTexturePool texturePool;
public:
	SdlUiRenderer();

	bool init(SDL_Window *window);
	void shutdown();

	UiTextureID createTexture(int width, int height);
	void freeTexture(UiTextureID textureID);
	std::optional<Int2> tryGetTextureDims(UiTextureID id) const;
	LockedTexture lockTexture(UiTextureID textureID);
	void unlockTexture(UiTextureID textureID);

	void draw(const RenderElement2D *elements, int count, RenderSpace renderSpace, const Rect &letterboxRect);
};

#endif
