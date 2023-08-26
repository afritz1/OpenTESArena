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

SdlUiRenderer::SdlUiRenderer()
{
	this->renderer = nullptr;
	this->nextID = -1;
}

bool SdlUiRenderer::init(SDL_Window *window)
{
	this->renderer = SDL_GetRenderer(window);
	if (this->renderer == nullptr)
	{
		DebugLogError("Couldn't get SDL renderer from window.");
		return false;
	}

	this->nextID = 0;
	return true;
}

void SdlUiRenderer::shutdown()
{
	for (auto &pair : this->textures)
	{
		SDL_Texture *texture = pair.second;
		SDL_DestroyTexture(texture);
	}

	this->textures.clear();

	this->renderer = nullptr;
	this->nextID = -1;
}

bool SdlUiRenderer::tryCreateUiTextureInternal(int width, int height, const TexelsInitFunc &initFunc, UiTextureID *outID)
{
	SDL_Texture *texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	if (texture == nullptr)
	{
		DebugLogError("Couldn't allocate SDL_Texture (dims: " + std::to_string(width) + "x" + std::to_string(height) +
			", " + std::string(SDL_GetError()) + ").");
		return false;
	}

	if (SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND) != 0)
	{
		DebugLogError("Couldn't set SDL_Texture blend mode to blend (dims: " + std::to_string(width) + "x" +
			std::to_string(height) + ", " + std::string(SDL_GetError()) + ").");
		return false;
	}

	uint32_t *dstTexels;
	int pitch;
	if (SDL_LockTexture(texture, nullptr, reinterpret_cast<void**>(&dstTexels), &pitch) != 0)
	{
		DebugLogError("Couldn't lock SDL_Texture for writing (dims: " + std::to_string(width) + "x" + std::to_string(height) +
			", " + std::string(SDL_GetError()) + ").");
		return false;
	}

	BufferView2D<uint32_t> dstTexelsView(dstTexels, width, height);
	initFunc(dstTexelsView);
	SDL_UnlockTexture(texture);

	*outID = this->nextID;
	this->nextID++;
	this->textures.emplace(*outID, texture);
	return true;
}

bool SdlUiRenderer::tryCreateUiTexture(int width, int height, UiTextureID *outID)
{
	TexelsInitFunc initFunc = [](BufferView2D<uint32_t> dstTexels)
	{
		const Color &debugColor = Color::Magenta;
		dstTexels.fill(debugColor.toARGB());
	};

	return this->tryCreateUiTextureInternal(width, height, initFunc, outID);
}

bool SdlUiRenderer::tryCreateUiTexture(BufferView2D<const uint32_t> texels, UiTextureID *outID)
{
	TexelsInitFunc initFunc = [&texels](BufferView2D<uint32_t> dstTexels)
	{
		std::copy(texels.begin(), texels.end(), dstTexels.begin());
	};

	return this->tryCreateUiTextureInternal(texels.getWidth(), texels.getHeight(), initFunc, outID);
}

bool SdlUiRenderer::tryCreateUiTexture(BufferView2D<const uint8_t> texels, const Palette &palette, UiTextureID *outID)
{
	TexelsInitFunc initFunc = [&texels, &palette](BufferView2D<uint32_t> dstTexels)
	{
		std::transform(texels.begin(), texels.end(), dstTexels.begin(),
			[&palette](uint8_t texel)
		{
			return palette[texel].toARGB();
		});
	};

	return this->tryCreateUiTextureInternal(texels.getWidth(), texels.getHeight(), initFunc, outID);
}

bool SdlUiRenderer::tryCreateUiTexture(TextureBuilderID textureBuilderID, PaletteID paletteID,
	const TextureManager &textureManager, UiTextureID *outID)
{
	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(textureBuilderID);
	const TextureBuilderType type = textureBuilder.getType();
	if (type == TextureBuilderType::Paletted)
	{
		const TextureBuilder::PalettedTexture &palettedTexture = textureBuilder.getPaletted();
		const Buffer2D<uint8_t> &texels = palettedTexture.texels;
		const BufferView2D<const uint8_t> texelsView(texels);
		const Palette &palette = textureManager.getPaletteHandle(paletteID);
		return this->tryCreateUiTexture(texelsView, palette, outID);
	}
	else if (type == TextureBuilderType::TrueColor)
	{
		const TextureBuilder::TrueColorTexture &trueColorTexture = textureBuilder.getTrueColor();
		const Buffer2D<uint32_t> &texels = trueColorTexture.texels;
		const BufferView2D<const uint32_t> texelsView(texels);
		return this->tryCreateUiTexture(texelsView, outID);
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(type)));
	}
}

uint32_t *SdlUiRenderer::lockUiTexture(UiTextureID textureID)
{
	const auto iter = this->textures.find(textureID);
	if (iter == this->textures.end())
	{
		DebugLogError("Couldn't get SDL_Texture from ID " + std::to_string(textureID) + " to lock.");
		return nullptr;
	}

	SDL_Texture *texture = iter->second;
	int width, height;
	SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

	uint32_t *dstTexels;
	int pitch;
	if (SDL_LockTexture(texture, nullptr, reinterpret_cast<void**>(&dstTexels), &pitch) != 0)
	{
		DebugLogError("Couldn't lock SDL_Texture for updating (ID " + std::to_string(textureID) + ", dims: " +
			std::to_string(width) + "x" + std::to_string(height) + ", " + std::string(SDL_GetError()) + ").");
		return nullptr;
	}

	return dstTexels;
}

void SdlUiRenderer::unlockUiTexture(UiTextureID textureID)
{
	const auto iter = this->textures.find(textureID);
	if (iter == this->textures.end())
	{
		DebugLogError("Couldn't get SDL_Texture from ID " + std::to_string(textureID) + " to unlock.");
		return;
	}

	SDL_Texture *texture = iter->second;
	SDL_UnlockTexture(texture);
}

void SdlUiRenderer::freeUiTexture(UiTextureID id)
{
	const auto iter = this->textures.find(id);
	if (iter == this->textures.end())
	{
		DebugLogWarning("No UI texture to free for ID \"" + std::to_string(id) + "\".");
		return;
	}

	SDL_Texture *texture = iter->second;
	SDL_DestroyTexture(texture);
	this->textures.erase(iter);
}

std::optional<Int2> SdlUiRenderer::tryGetTextureDims(UiTextureID id) const
{
	const auto iter = this->textures.find(id);
	if (iter == this->textures.end())
	{
		DebugLogWarning("No UI texture registered for ID \"" + std::to_string(id) + "\".");
		return std::nullopt;
	}

	SDL_Texture *texture = iter->second;
	int width, height;
	const int result = SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
	if (result != 0)
	{
		DebugLogWarning("Couldn't query UI texture \"" + std::to_string(id) + "\" dimensions (" + std::to_string(result) + ").");
		return std::nullopt;
	}

	return Int2(width, height);
}

void SdlUiRenderer::draw(const RenderElement *elements, int count, RenderSpace renderSpace, const Rect &letterboxRect)
{
	auto originalPointToNative = [&letterboxRect](const Int2 &point)
	{
		const double originalXPercent = static_cast<double>(point.x) /
			ArenaRenderUtils::SCREEN_WIDTH_REAL;
		const double originalYPercent = static_cast<double>(point.y) /
			ArenaRenderUtils::SCREEN_HEIGHT_REAL;

		const double letterboxWidthReal = static_cast<double>(letterboxRect.getWidth());
		const double letterboxHeightReal = static_cast<double>(letterboxRect.getHeight());
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

	for (int i = 0; i < count; i++)
	{
		const RenderElement &element = elements[i];

		const auto textureIter = this->textures.find(element.id);
		DebugAssert(textureIter != this->textures.end());
		SDL_Texture *texture = textureIter->second;

		SDL_Rect nativeRect;
		if (renderSpace == RenderSpace::Classic)
		{
			constexpr double screenWidthReal = ArenaRenderUtils::SCREEN_WIDTH_REAL;
			constexpr double screenHeightReal = ArenaRenderUtils::SCREEN_HEIGHT_REAL;

			// Rect in classic 320x200 space.
			SDL_Rect classicRect;
			classicRect.x = static_cast<int>(std::round(static_cast<double>(element.x) * screenWidthReal));
			classicRect.y = static_cast<int>(std::round(static_cast<double>(element.y) * screenHeightReal));
			classicRect.w = static_cast<int>(std::round(static_cast<double>(element.width) * screenWidthReal)); // @todo: dimensions should honor pixel centers, right?
			classicRect.h = static_cast<int>(std::round(static_cast<double>(element.height) * screenHeightReal));

			nativeRect = originalRectToNative(classicRect);
		}
		else if (renderSpace == RenderSpace::Native)
		{
			int renderWidth, renderHeight;
			if (SDL_GetRendererOutputSize(this->renderer, &renderWidth, &renderHeight) != 0)
			{
				DebugCrash("Couldn't get renderer output size.");
			}

			nativeRect.x = static_cast<int>(std::round(static_cast<double>(element.x) * renderWidth));
			nativeRect.y = static_cast<int>(std::round(static_cast<double>(element.y) * renderHeight));
			nativeRect.w = static_cast<int>(std::round(static_cast<double>(element.width) * renderWidth)); // @todo: dimensions should honor pixel centers, right?
			nativeRect.h = static_cast<int>(std::round(static_cast<double>(element.height) * renderHeight));
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(renderSpace)));
		}

		SDL_RenderCopy(this->renderer, texture, nullptr, &nativeRect);
	}
}
