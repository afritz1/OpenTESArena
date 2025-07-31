#include <algorithm>
#include <cmath>
#include <type_traits>

#include "SDL_render.h"

#include "ArenaRenderUtils.h"
#include "SdlUiRenderer.h"
#include "../Assets/TextureBuilder.h"
#include "../Assets/TextureManager.h"
#include "../Math/Constants.h"
#include "../Math/Rect.h"
#include "../UI/RenderSpace.h"

#include "components/debug/Debug.h"

namespace
{
	using TexelsInitFunc = std::function<void(Span2D<uint32_t>)>;

	UiTextureID CreateUiTexture(int width, int height, const TexelsInitFunc &initFunc, SdlUiTexturePool *pool, SDL_Renderer *renderer)
	{
		const UiTextureID textureID = pool->alloc();
		if (textureID < 0)
		{
			DebugLogErrorFormat("Couldn't allocate texture ID from pool for SDL_Texture with dims %dx%d.", width, height);
			return -1;
		}

		SDL_Texture *&texture = pool->get(textureID);
		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
		if (texture == nullptr)
		{
			DebugLogErrorFormat("Couldn't allocate SDL_Texture with dims %dx%d (%s).", width, height, SDL_GetError());
			pool->free(textureID);
			return -1;
		}

		if (SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND) != 0)
		{
			DebugLogErrorFormat("Couldn't set SDL_Texture blend mode with dims %dx%d (%s).", width, height, SDL_GetError());
			pool->free(textureID);
			return -1;
		}

		uint32_t *dstTexels;
		int pitch;
		if (SDL_LockTexture(texture, nullptr, reinterpret_cast<void**>(&dstTexels), &pitch) != 0)
		{
			DebugLogErrorFormat("Couldn't lock SDL_Texture for writing with dims %dx%d (%s).", width, height, SDL_GetError());
			pool->free(textureID);
			return -1;
		}

		Span2D<uint32_t> dstTexelsView(dstTexels, width, height);
		initFunc(dstTexelsView);
		SDL_UnlockTexture(texture);

		return textureID;
	}
}

SdlUiTextureAllocator::SdlUiTextureAllocator()
{
	this->pool = nullptr;
	this->renderer = nullptr;
}

void SdlUiTextureAllocator::init(SdlUiTexturePool *pool, SDL_Renderer *renderer)
{
	this->pool = pool;
	this->renderer = renderer;
}

UiTextureID SdlUiTextureAllocator::create(int width, int height)
{
	TexelsInitFunc initFunc = [](Span2D<uint32_t> dstTexels)
	{
		dstTexels.fill(Colors::MagentaARGB);
	};

	return CreateUiTexture(width, height, initFunc, this->pool, this->renderer);
}

UiTextureID SdlUiTextureAllocator::create(Span2D<const uint32_t> texels)
{
	TexelsInitFunc initFunc = [&texels](Span2D<uint32_t> dstTexels)
	{
		std::copy(texels.begin(), texels.end(), dstTexels.begin());
	};

	return CreateUiTexture(texels.getWidth(), texels.getHeight(), initFunc, this->pool, this->renderer);
}

UiTextureID SdlUiTextureAllocator::create(Span2D<const uint8_t> texels, const Palette &palette)
{
	TexelsInitFunc initFunc = [&texels, &palette](Span2D<uint32_t> dstTexels)
	{
		std::transform(texels.begin(), texels.end(), dstTexels.begin(),
			[&palette](uint8_t texel)
		{
			return palette[texel].toARGB();
		});
	};

	return CreateUiTexture(texels.getWidth(), texels.getHeight(), initFunc, this->pool, this->renderer);
}

UiTextureID SdlUiTextureAllocator::create(TextureBuilderID textureBuilderID, PaletteID paletteID, const TextureManager &textureManager)
{
	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(textureBuilderID);
	const TextureBuilderType type = textureBuilder.type;
	if (type == TextureBuilderType::Paletted)
	{
		const TextureBuilderPalettedTexture &palettedTexture = textureBuilder.paletteTexture;
		const Buffer2D<uint8_t> &texels = palettedTexture.texels;
		const Span2D<const uint8_t> texelsView(texels);
		const Palette &palette = textureManager.getPaletteHandle(paletteID);
		return this->create(texelsView, palette);
	}
	else if (type == TextureBuilderType::TrueColor)
	{
		const TextureBuilderTrueColorTexture &trueColorTexture = textureBuilder.trueColorTexture;
		const Buffer2D<uint32_t> &texels = trueColorTexture.texels;
		const Span2D<const uint32_t> texelsView(texels);
		return this->create(texelsView);
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(type)));
	}
}

void SdlUiTextureAllocator::free(UiTextureID textureID)
{
	SDL_Texture **texture = this->pool->tryGet(textureID);
	if (texture == nullptr)
	{
		DebugLogWarningFormat("No SDL_Texture to free at ID %d.", textureID);
		return;
	}

	SDL_DestroyTexture(*texture);
	this->pool->free(textureID);
}

uint32_t *SdlUiTextureAllocator::lock(UiTextureID textureID)
{
	SDL_Texture **texture = this->pool->tryGet(textureID);
	if (texture == nullptr)
	{
		DebugLogWarningFormat("No SDL_Texture to lock at ID %d.", textureID);
		return nullptr;
	}

	int width, height;
	if (SDL_QueryTexture(*texture, nullptr, nullptr, &width, &height) != 0)
	{
		DebugLogErrorFormat("Couldn't query SDL_Texture dimensions for ID %d (%s).", textureID, SDL_GetError());
		return nullptr;
	}

	uint32_t *dstTexels;
	int pitch;
	if (SDL_LockTexture(*texture, nullptr, reinterpret_cast<void**>(&dstTexels), &pitch) != 0)
	{
		DebugLogErrorFormat("Couldn't lock SDL_Texture for updating (ID %d, dims %dx%d, %s).", textureID, width, height, SDL_GetError());
		return nullptr;
	}

	return dstTexels;
}

void SdlUiTextureAllocator::unlock(UiTextureID textureID)
{
	SDL_Texture **texture = this->pool->tryGet(textureID);
	if (texture == nullptr)
	{
		DebugLogWarningFormat("No SDL_Texture to unlock at ID %d.", textureID);
		return;
	}

	SDL_UnlockTexture(*texture);
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

	this->textureAllocator.init(&this->texturePool, this->renderer);
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

UiTextureAllocator *SdlUiRenderer::getTextureAllocator()
{
	return &this->textureAllocator;
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

void SdlUiRenderer::draw(const RenderElement *elements, int count, RenderSpace renderSpace, const Rect &letterboxRect)
{
	auto originalPointToNative = [&letterboxRect](const Int2 &point)
	{
		const double originalXPercent = static_cast<double>(point.x) / ArenaRenderUtils::SCREEN_WIDTH_REAL;
		const double originalYPercent = static_cast<double>(point.y) / ArenaRenderUtils::SCREEN_HEIGHT_REAL;

		const double letterboxWidthReal = static_cast<double>(letterboxRect.width);
		const double letterboxHeightReal = static_cast<double>(letterboxRect.height);
		const Int2 letterboxPoint(
			static_cast<int>(std::round(letterboxWidthReal * originalXPercent)),
			static_cast<int>(std::round(letterboxHeightReal * originalYPercent)));
		const Int2 nativePoint(
			letterboxPoint.x + letterboxRect.getLeft(),
			letterboxPoint.y + letterboxRect.getTop());
		return nativePoint;
	};

	auto originalRectToNative = [&originalPointToNative](const SDL_Rect &rect)
	{
		const Int2 oldTopLeft(rect.x, rect.y);
		const Int2 oldBottomRight(rect.x + rect.w, rect.y + rect.h);
		const Int2 newTopLeft = originalPointToNative(oldTopLeft);
		const Int2 newBottomRight = originalPointToNative(oldBottomRight);

		SDL_Rect newRect;
		newRect.x = newTopLeft.x;
		newRect.y = newTopLeft.y;
		newRect.w = newBottomRight.x - newTopLeft.x;
		newRect.h = newBottomRight.y - newTopLeft.y;
		return newRect;
	};

	int renderWidth, renderHeight;
	if (SDL_GetRendererOutputSize(this->renderer, &renderWidth, &renderHeight) != 0)
	{
		DebugCrash("Couldn't get renderer output size.");
	}

	const double renderWidthReal = static_cast<double>(renderWidth);
	const double renderHeightReal = static_cast<double>(renderHeight);

	for (int i = 0; i < count; i++)
	{
		const RenderElement &element = elements[i];
		SDL_Texture *texture = this->texturePool.get(element.id);

		SDL_Rect nativeRect;
		if (renderSpace == RenderSpace::Classic)
		{
			constexpr double screenWidthReal = ArenaRenderUtils::SCREEN_WIDTH_REAL;
			constexpr double screenHeightReal = ArenaRenderUtils::SCREEN_HEIGHT_REAL;

			// Rect in classic 320x200 space.
			// @todo I wonder if the tiny cracks between things is because we round + truncate, then round + truncate again above?
			SDL_Rect classicRect;
			classicRect.x = static_cast<int>(std::round(static_cast<double>(element.x) * screenWidthReal));
			classicRect.y = static_cast<int>(std::round(static_cast<double>(element.y) * screenHeightReal));
			classicRect.w = static_cast<int>(std::round(static_cast<double>(element.width) * screenWidthReal)); // @todo: dimensions should honor pixel centers, right?
			classicRect.h = static_cast<int>(std::round(static_cast<double>(element.height) * screenHeightReal));

			nativeRect = originalRectToNative(classicRect);
		}
		else if (renderSpace == RenderSpace::Native)
		{
			nativeRect.x = static_cast<int>(std::round(static_cast<double>(element.x) * renderWidthReal));
			nativeRect.y = static_cast<int>(std::round(static_cast<double>(element.y) * renderHeightReal));
			nativeRect.w = static_cast<int>(std::round(static_cast<double>(element.width) * renderWidthReal)); // @todo: dimensions should honor pixel centers, right?
			nativeRect.h = static_cast<int>(std::round(static_cast<double>(element.height) * renderHeightReal));
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(renderSpace)));
		}

		SDL_RenderCopy(this->renderer, texture, nullptr, &nativeRect);
	}
}
