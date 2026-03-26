#include <algorithm>
#include <cmath>
#include <functional>
#include <type_traits>

#include "SDL_render.h"

#include "ArenaRenderUtils.h"
#include "RenderBackend.h"
#include "Renderer.h"
#include "RendererUtils.h"
#include "SdlUiRenderer.h"
#include "../Assets/TextureBuilder.h"
#include "../Assets/TextureManager.h"
#include "../Math/Constants.h"
#include "../Math/Rect.h"
#include "../UI/UiRenderSpace.h"

#include "components/debug/Debug.h"

namespace
{
	using TexelsInitFunc = std::function<void(Span2D<uint32_t>)>;

	UiTextureID CreateUiTexture(int width, int height, const TexelsInitFunc &initFunc, SdlUiTexturePool &pool, SDL_Renderer *renderer)
	{
		const UiTextureID textureID = pool.alloc();
		if (textureID < 0)
		{
			DebugLogErrorFormat("Couldn't allocate texture ID from pool for SDL_Texture with dims %dx%d.", width, height);
			return -1;
		}

		SDL_Texture *&texture = pool.get(textureID);
		texture = SDL_CreateTexture(renderer, RendererUtils::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, width, height);
		if (texture == nullptr)
		{
			DebugLogErrorFormat("Couldn't allocate SDL_Texture with dims %dx%d (%s).", width, height, SDL_GetError());
			pool.free(textureID);
			return -1;
		}

		if (SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND) != 0)
		{
			DebugLogErrorFormat("Couldn't set SDL_Texture blend mode with dims %dx%d (%s).", width, height, SDL_GetError());
			pool.free(textureID);
			return -1;
		}

		uint32_t *dstTexels;
		int pitch;
		if (SDL_LockTexture(texture, nullptr, reinterpret_cast<void**>(&dstTexels), &pitch) != 0)
		{
			DebugLogErrorFormat("Couldn't lock SDL_Texture for writing with dims %dx%d (%s).", width, height, SDL_GetError());
			pool.free(textureID);
			return -1;
		}

		Span2D<uint32_t> dstTexelsView(dstTexels, width, height);
		initFunc(dstTexelsView);
		SDL_UnlockTexture(texture);

		return textureID;
	}
}

SdlUiRenderer::SdlUiRenderer()
{
	this->renderer = nullptr;
}

bool SdlUiRenderer::init(SDL_Window *window)
{
	this->renderer = SDL_GetRenderer(window);
	if (this->renderer == nullptr)
	{
		DebugLogError("Couldn't get SDL_Renderer from window.");
		return false;
	}

	return true;
}

void SdlUiRenderer::shutdown()
{
	for (SDL_Texture *texture : this->texturePool.values)
	{
		SDL_DestroyTexture(texture);
	}

	this->texturePool.clear();
	this->renderer = nullptr;
}

RendererProfilerData2D SdlUiRenderer::getProfilerData() const
{
	RendererProfilerData2D profilerData;
	profilerData.drawCallCount = 0;
	profilerData.uiTextureCount = static_cast<int>(this->texturePool.values.size());
	for (SDL_Texture *texture : this->texturePool.values)
	{
		constexpr int bytesPerTexel = 4;

		int width, height;
		const int result = SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
		if (result != 0)
		{
			continue;
		}

		profilerData.uiTextureByteCount += width * height * bytesPerTexel;
	}

	return profilerData;
}

UiTextureID SdlUiRenderer::createTexture(int width, int height)
{
	TexelsInitFunc initFunc = [](Span2D<uint32_t> dstTexels)
	{
		dstTexels.fill(Colors::MagentaRGBA);
	};

	return CreateUiTexture(width, height, initFunc, this->texturePool, this->renderer);
}

void SdlUiRenderer::freeTexture(UiTextureID textureID)
{
	SDL_Texture **texture = this->texturePool.tryGet(textureID);
	if (texture == nullptr)
	{
		DebugLogWarningFormat("No SDL_Texture to free at ID %d.", textureID);
		return;
	}

	SDL_DestroyTexture(*texture);
	this->texturePool.free(textureID);
}

std::optional<Int2> SdlUiRenderer::tryGetTextureDims(UiTextureID textureID) const
{
	SDL_Texture *const *texture = this->texturePool.tryGet(textureID);
	if (texture == nullptr)
	{
		DebugLogWarningFormat("No SDL_Texture registered for ID %d.", textureID);
		return std::nullopt;
	}

	int width, height;
	const int result = SDL_QueryTexture(*texture, nullptr, nullptr, &width, &height);
	if (result != 0)
	{
		DebugLogWarningFormat("Couldn't query SDL_Texture %d dimensions (%s).", textureID, SDL_GetError());
		return std::nullopt;
	}

	return Int2(width, height);
}

LockedTexture SdlUiRenderer::lockTexture(UiTextureID textureID)
{
	SDL_Texture **texture = this->texturePool.tryGet(textureID);
	if (texture == nullptr)
	{
		DebugLogWarningFormat("No SDL_Texture to lock at ID %d.", textureID);
		return LockedTexture();
	}

	int width, height;
	if (SDL_QueryTexture(*texture, nullptr, nullptr, &width, &height) != 0)
	{
		DebugLogErrorFormat("Couldn't query SDL_Texture dimensions for ID %d (%s).", textureID, SDL_GetError());
		return LockedTexture();
	}

	uint32_t *dstTexels;
	int pitch;
	if (SDL_LockTexture(*texture, nullptr, reinterpret_cast<void**>(&dstTexels), &pitch) != 0)
	{
		DebugLogErrorFormat("Couldn't lock SDL_Texture for updating (ID %d, dims %dx%d, %s).", textureID, width, height, SDL_GetError());
		return LockedTexture();
	}

	const int bytesPerElement = sizeof(*dstTexels);
	const int byteCount = width * height * bytesPerElement;
	return LockedTexture(Span<std::byte>(reinterpret_cast<std::byte*>(dstTexels), byteCount), width, height, bytesPerElement);
}

void SdlUiRenderer::unlockTexture(UiTextureID textureID)
{
	SDL_Texture **texture = this->texturePool.tryGet(textureID);
	if (texture == nullptr)
	{
		DebugLogWarningFormat("No SDL_Texture to unlock at ID %d.", textureID);
		return;
	}

	SDL_UnlockTexture(*texture);
}

void SdlUiRenderer::draw(Span<const RenderElement2D> elements)
{
	for (const RenderElement2D &element : elements)
	{
		const Rect clipRect = element.clipRect;
		if (!clipRect.isEmpty())
		{
			const SDL_Rect sdlClipRect = clipRect.getSdlRect();
			SDL_RenderSetClipRect(this->renderer, &sdlClipRect);
		}

		SDL_Texture *texture = this->texturePool.get(element.id);

		SDL_Rect rect;
		rect.x = element.rect.x;
		rect.y = element.rect.y;
		rect.w = element.rect.width;
		rect.h = element.rect.height;

		SDL_RenderCopy(this->renderer, texture, nullptr, &rect);

		if (!clipRect.isEmpty())
		{
			SDL_RenderSetClipRect(this->renderer, nullptr);
		}
	}
}
