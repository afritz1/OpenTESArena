#include <cassert>
#include <iostream>
#include <cmath>

#include <SDL2/SDL.h>

#include "Renderer.h"
#include "../Interface/Surface.h"
#include "../Math/Constants.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Utilities/Debug.h"

const int Renderer::DEFAULT_COLOR_BITS = 32;
const std::string Renderer::DEFAULT_RENDER_SCALE_QUALITY = "nearest";

Renderer::Renderer(int width, int height, bool fullscreen, const std::string &title)
{
	Debug::mention("Renderer", "Initializing.");

	assert(width > 0);
	assert(height > 0);

	this->window = nullptr;
	this->renderer = nullptr;
	this->texture = nullptr;

	// Initialize window. The SDL_Surface is obtained from this window.
	this->window = [width, height, fullscreen, &title]()
	{
		return fullscreen ?
			SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP) :
			SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE);
	}();
	Debug::check(this->window != nullptr, "Renderer", "SDL_CreateWindow");

	// Initialize renderer context.
	this->renderer = [this]()
	{
		const int bestDriver = -1;
		return SDL_CreateRenderer(this->window, bestDriver, SDL_RENDERER_ACCELERATED);
	}();
	Debug::check(this->renderer != nullptr, "Renderer", "SDL_CreateRenderer");

	// Set pixel interpolation hint.
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, Renderer::DEFAULT_RENDER_SCALE_QUALITY.c_str());

	// Set the size of the render texture to be the size of the whole screen
	// (it scales otherwise).
	const auto *nativeSurface = this->getWindowSurface();
	SDL_RenderSetLogicalSize(this->renderer, nativeSurface->w, nativeSurface->h);

	// Set the clear color of the renderer.
	SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);

	// Create SDL_Texture on the GPU. The surface updates this every frame.
	this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, nativeSurface->w, nativeSurface->h);
	Debug::check(this->texture != nullptr, "Renderer", "SDL_CreateTexture");

	assert(this->window != nullptr);
	assert(this->renderer != nullptr);
	assert(this->texture != nullptr);
}

Renderer::~Renderer()
{
	assert(this->window != nullptr);
	assert(this->renderer != nullptr);
	assert(this->texture != nullptr);

	SDL_DestroyWindow(this->window);
	SDL_DestroyRenderer(this->renderer);
	SDL_DestroyTexture(this->texture);
}

SDL_Surface *Renderer::getWindowSurface() const
{
	assert(this->window != nullptr);

	return SDL_GetWindowSurface(this->window);
}

std::unique_ptr<SDL_Rect> Renderer::getLetterboxDimensions() const
{
	// Letterbox width and height always maintain the 1.6:1 aspect ratio.
	const auto originalAspect = static_cast<double>(ORIGINAL_WIDTH) /
		static_cast<double>(ORIGINAL_HEIGHT);

	// Native window aspect can be anything finite.
	const auto *nativeSurface = this->getWindowSurface();
	auto nativeAspect = static_cast<double>(nativeSurface->w) /
		static_cast<double>(nativeSurface->h);

	assert(std::isfinite(originalAspect));
	assert(std::isfinite(nativeAspect));

	// Compare the two aspects to decide what the letterbox dimensions are.
	if (std::abs(nativeAspect - originalAspect) < EPSILON)
	{
		// Equal aspects. The letterbox is equal to the screen size.
		auto rect = SDL_Rect();
		rect.x = 0;
		rect.y = 0;
		rect.w = nativeSurface->w;
		rect.h = nativeSurface->h;
		return std::unique_ptr<SDL_Rect>(new SDL_Rect(rect));
	}
	else if (nativeAspect > originalAspect)
	{
		// Native window is wider = empty left and right.
		int subWidth = static_cast<int>(std::ceil(
			static_cast<double>(nativeSurface->h) * originalAspect));
		auto rect = SDL_Rect();
		rect.x = (nativeSurface->w - subWidth) / 2;
		rect.y = 0;
		rect.w = subWidth;
		rect.h = nativeSurface->h;
		return std::unique_ptr<SDL_Rect>(new SDL_Rect(rect));
	}
	else
	{
		// Native window is taller = empty top and bottom.
		int subHeight = static_cast<int>(std::ceil(
			static_cast<double>(nativeSurface->w) / originalAspect));
		auto rect = SDL_Rect();
		rect.x = 0;
		rect.y = (nativeSurface->h - subHeight) / 2;
		rect.w = nativeSurface->w;
		rect.h = subHeight;
		return std::unique_ptr<SDL_Rect>(new SDL_Rect(rect));
	}
}

void Renderer::resize(int width, int height)
{
	// No need to set dimensions on a resize event; the window does that
	// automatically now. The arguments should be kept for when the options menu
	// is implemented.
	static_cast<void>(width);
	static_cast<void>(height);

	// Release old SDL things.
	SDL_DestroyTexture(this->texture);

	// Reset the size of the render texture.
	const auto *nativeSurface = this->getWindowSurface();
	SDL_RenderSetLogicalSize(this->renderer, nativeSurface->w, nativeSurface->h);

	// Recreate SDL_Texture for the GPU.
	this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, nativeSurface->w, nativeSurface->h);
	Debug::check(this->texture != nullptr, "Renderer", "SDL_CreateTexture resize");
}

void Renderer::setWindowIcon(TextureName name, TextureManager &textureManager)
{
	const auto &icon = textureManager.getSurface(TextureFile::fromName(name));
	auto *iconSurface = icon.getSurface();
	SDL_SetWindowIcon(this->window, iconSurface);
}

void Renderer::present()
{
	const auto *nativeSurface = this->getWindowSurface();
	SDL_UpdateTexture(this->texture, nullptr, nativeSurface->pixels,
		nativeSurface->pitch);
	SDL_RenderClear(this->renderer);
	SDL_RenderCopy(this->renderer, this->texture, nullptr, nullptr);
	SDL_RenderPresent(this->renderer);
}
