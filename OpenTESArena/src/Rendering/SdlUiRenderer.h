#ifndef SDL_UI_RENDERER_H
#define SDL_UI_RENDERER_H

#include <optional>

#include "components/utilities/KeyValuePool.h"
#include "components/utilities/Span.h"
#include "components/utilities/Span2D.h"

struct Rect;
struct RenderElement2D;
struct RendererProfilerData2D;
struct SDL_Renderer;
struct SDL_Texture;

using SdlUiTexturePool = KeyValuePool<UiTextureID, SDL_Texture*>;

class SdlUiRenderer
{
private:
	SDL_Renderer *renderer;
	SdlUiTexturePool texturePool;
public:
	SdlUiRenderer();

	bool init(SDL_Window *window);
	void shutdown();

	RendererProfilerData2D getProfilerData() const;

	UiTextureID createTexture(int width, int height);
	void freeTexture(UiTextureID textureID);
	std::optional<Int2> tryGetTextureDims(UiTextureID id) const;
	LockedTexture lockTexture(UiTextureID textureID);
	void unlockTexture(UiTextureID textureID);

	void draw(Span<const RenderElement2D> elements);
};

#endif
