#include <cassert>
#include <cmath>

#include "SDL.h"

#include "Renderer.h"
#include "../Interface/Surface.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
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
	this->renderer = this->createRenderer();
}

Renderer::~Renderer()
{
	Debug::mention("Renderer", "Closing.");

	SDL_DestroyWindow(this->window);
	SDL_DestroyRenderer(this->renderer);
}

SDL_Renderer *Renderer::createRenderer()
{
	// Automatically choose the best driver.
	const int bestDriver = -1;

	SDL_Renderer *rendererContext = SDL_CreateRenderer(
		this->window, bestDriver, SDL_RENDERER_ACCELERATED);
	Debug::check(rendererContext != nullptr, "Renderer", "SDL_CreateRenderer");

	// Set pixel interpolation hint.
	SDL_bool status = SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,
		Renderer::DEFAULT_RENDER_SCALE_QUALITY.c_str());
	if (status != SDL_TRUE)
	{
		Debug::mention("Renderer", "Could not set interpolation hint.");
	}

	// Set the size of the render texture to be the size of the whole screen
	// (it automatically scales otherwise).
	auto *nativeSurface = this->getWindowSurface();

	// If this fails, we might not support hardware accelerated renderers for some reason
	// (such as with Linux), so we retry with software.
	if (!nativeSurface)
	{
		Debug::mention("Renderer",
			"Failed to initialize with hardware accelerated renderer, trying software.");

		SDL_DestroyRenderer(rendererContext);

		rendererContext = SDL_CreateRenderer(this->window, bestDriver, SDL_RENDERER_SOFTWARE);
		Debug::check(rendererContext != nullptr, "Renderer", "SDL_CreateRenderer software");

		nativeSurface = this->getWindowSurface();
	}

	Debug::check(nativeSurface != nullptr, "Renderer", "SDL_GetWindowSurface");

	// Set the device-independent resolution for rendering.
	SDL_RenderSetLogicalSize(rendererContext, nativeSurface->w, nativeSurface->h);

	// Set the clear color of the renderer.
	SDL_SetRenderDrawColor(rendererContext, 0, 0, 0, 255);

	return rendererContext;
}

SDL_Surface *Renderer::getWindowSurface() const
{
	return SDL_GetWindowSurface(this->window);
}

Int2 Renderer::getRenderDimensions() const
{
	int width, height;
	SDL_GetRendererOutputSize(this->renderer, &width, &height);
	return Int2(width, height);
}

SDL_PixelFormat *Renderer::getPixelFormat() const
{
	return SDL_GetWindowSurface(this->window)->format;
}

SDL_Renderer *Renderer::getRenderer() const
{
	return this->renderer;
}

SDL_Rect Renderer::getLetterboxDimensions() const
{
	// Letterbox width and height always maintain the original aspect ratio.
	// Now using corrected 4:3 aspect ratio.
	const double originalAspect = static_cast<double>(ORIGINAL_WIDTH) /
		static_cast<double>(ORIGINAL_HEIGHT);

	// Native window aspect can be anything finite.
	const auto *nativeSurface = this->getWindowSurface();
	double nativeAspect = static_cast<double>(nativeSurface->w) /
		static_cast<double>(nativeSurface->h);

	assert(std::isfinite(originalAspect));
	assert(std::isfinite(nativeAspect));

	// Compare the two aspects to decide what the letterbox dimensions are.
	if (std::abs(nativeAspect - originalAspect) < EPSILON)
	{
		// Equal aspects. The letterbox is equal to the screen size.
		SDL_Rect rect;
		rect.x = 0;
		rect.y = 0;
		rect.w = nativeSurface->w;
		rect.h = nativeSurface->h;
		return rect;
	}
	else if (nativeAspect > originalAspect)
	{
		// Native window is wider = empty left and right.
		int subWidth = static_cast<int>(std::ceil(
			static_cast<double>(nativeSurface->h) * originalAspect));
		SDL_Rect rect;
		rect.x = (nativeSurface->w - subWidth) / 2;
		rect.y = 0;
		rect.w = subWidth;
		rect.h = nativeSurface->h;
		return rect;
	}
	else
	{
		// Native window is taller = empty top and bottom.
		int subHeight = static_cast<int>(std::ceil(
			static_cast<double>(nativeSurface->w) / originalAspect));
		SDL_Rect rect;
		rect.x = 0;
		rect.y = (nativeSurface->h - subHeight) / 2;
		rect.w = nativeSurface->w;
		rect.h = subHeight;
		return rect;
	}
}

void Renderer::resize(int width, int height)
{
	// No need to set dimensions on a resize event; the window does that
	// automatically now. The arguments should be kept for when the options menu
	// is implemented.
	static_cast<void>(width);
	static_cast<void>(height);

	// Reset the size of the render texture.
	const auto *nativeSurface = this->getWindowSurface();
	SDL_RenderSetLogicalSize(this->renderer, nativeSurface->w, nativeSurface->h);
}

void Renderer::setWindowIcon(TextureName name, TextureManager &textureManager)
{
	const auto &icon = textureManager.getSurface(TextureFile::fromName(name));
	SDL_SetWindowIcon(this->window, icon.getSurface());
}

void Renderer::present()
{
	SDL_RenderPresent(this->renderer);
}
