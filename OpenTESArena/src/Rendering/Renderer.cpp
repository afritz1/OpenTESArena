#include <algorithm>
#include <cassert>
#include <cmath>

#include "SDL.h"

#include "Renderer.h"

#include "SoftwareRenderer.h"
#include "Surface.h"
#include "../Math/Constants.h"
#include "../Math/Rect.h"
#include "../Media/Color.h"
#include "../Utilities/Debug.h"

const char *Renderer::DEFAULT_RENDER_SCALE_QUALITY = "nearest";
const std::string Renderer::DEFAULT_TITLE = "OpenTESArena";
const int Renderer::ORIGINAL_WIDTH = 320;
const int Renderer::ORIGINAL_HEIGHT = 200;
const int Renderer::DEFAULT_BPP = 32;
const uint32_t Renderer::DEFAULT_PIXELFORMAT = SDL_PIXELFORMAT_ARGB8888;

Renderer::Renderer(int width, int height, bool fullscreen, double letterboxAspect)
{
	Debug::mention("Renderer", "Initializing.");

	assert(width > 0);
	assert(height > 0);

	this->letterboxAspect = letterboxAspect;

	// Initialize window. The SDL_Surface is obtained from this window.
	this->window = [width, height, fullscreen]()
	{
		const std::string &title = Renderer::DEFAULT_TITLE;
		return fullscreen ?
			SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP) :
			SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE);
	}();
	Debug::check(this->window != nullptr, "Renderer", "SDL_CreateWindow");

	// Initialize renderer context.
	this->renderer = this->createRenderer();

	// Use window dimensions, just in case it's fullscreen and the given width and
	// height are ignored.
	Int2 windowDimensions = this->getWindowDimensions();

	// Initialize native frame buffer.
	this->nativeTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_TARGET, windowDimensions.x, windowDimensions.y);
	Debug::check(this->nativeTexture != nullptr, "Renderer",
		"Couldn't create native frame buffer, " + std::string(SDL_GetError()));

	// Initialize 320x200 frame buffer.
	this->originalTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_TARGET, Renderer::ORIGINAL_WIDTH, Renderer::ORIGINAL_HEIGHT);

	// Don't initialize the game world buffer until the 3D renderer is initialized.
	this->gameWorldTexture = nullptr;
	this->softwareRenderer = nullptr;
	this->fullGameWindow = false;

	// Set the original frame buffer to not use transparency by default.
	this->useTransparencyBlending(false);
}

Renderer::Renderer(int width, int height, bool fullscreen)
	: Renderer(width, height, fullscreen, static_cast<double>(Renderer::ORIGINAL_WIDTH) /
		static_cast<double>(Renderer::ORIGINAL_HEIGHT)) { }

Renderer::~Renderer()
{
	Debug::mention("Renderer", "Closing.");

	SDL_DestroyWindow(this->window);

	// This also destroys the frame buffer textures.
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
		Renderer::DEFAULT_RENDER_SCALE_QUALITY);
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
		Debug::mention("Renderer", "Failed to initialize accelerated SDL_Renderer.");
		Debug::mention("Renderer", "Trying software fallback.");

		SDL_DestroyRenderer(rendererContext);

		rendererContext = SDL_CreateRenderer(this->window, bestDriver, SDL_RENDERER_SOFTWARE);
		Debug::check(rendererContext != nullptr, "Renderer", "SDL_CreateRenderer software");

		nativeSurface = this->getWindowSurface();
	}

	Debug::check(nativeSurface != nullptr, "Renderer", "SDL_GetWindowSurface");

	// Set the device-independent resolution for rendering (i.e., the 
	// "behind-the-scenes" resolution).
	SDL_RenderSetLogicalSize(rendererContext, nativeSurface->w, nativeSurface->h);

	return rendererContext;
}

SDL_Surface *Renderer::getWindowSurface() const
{
	return SDL_GetWindowSurface(this->window);
}

Int2 Renderer::getWindowDimensions() const
{
	const SDL_Surface *nativeSurface = this->getWindowSurface();
	return Int2(nativeSurface->w, nativeSurface->h);
}

int Renderer::getViewHeight() const
{
	const int screenHeight = this->getWindowDimensions().y;

	// Ratio of the view height and window height in 320x200.
	const double viewWindowRatio = static_cast<double>(ORIGINAL_HEIGHT - 53) /
		static_cast<double>(ORIGINAL_HEIGHT);

	// Actual view height to use.
	const int viewHeight = this->fullGameWindow ? screenHeight :
		static_cast<int>(std::ceil(screenHeight * viewWindowRatio));

	return viewHeight;
}

SDL_Rect Renderer::getLetterboxDimensions() const
{
	const auto *nativeSurface = this->getWindowSurface();
	double nativeAspect = static_cast<double>(nativeSurface->w) /
		static_cast<double>(nativeSurface->h);

	// Compare the two aspects to decide what the letterbox dimensions are.
	if (std::abs(nativeAspect - this->letterboxAspect) < EPSILON)
	{
		// Equal aspects. The letterbox is equal to the screen size.
		SDL_Rect rect;
		rect.x = 0;
		rect.y = 0;
		rect.w = nativeSurface->w;
		rect.h = nativeSurface->h;
		return rect;
	}
	else if (nativeAspect > this->letterboxAspect)
	{
		// Native window is wider = empty left and right.
		int subWidth = static_cast<int>(std::ceil(
			static_cast<double>(nativeSurface->h) * this->letterboxAspect));
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
			static_cast<double>(nativeSurface->w) / this->letterboxAspect));
		SDL_Rect rect;
		rect.x = 0;
		rect.y = (nativeSurface->h - subHeight) / 2;
		rect.w = nativeSurface->w;
		rect.h = subHeight;
		return rect;
	}
}

SDL_Surface *Renderer::getScreenshot() const
{
	const Int2 dimensions = this->getWindowDimensions();
	SDL_Surface *screenshot = Surface::createSurfaceWithFormat(
		dimensions.x, dimensions.y,
		Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);

	int status = SDL_RenderReadPixels(this->renderer, nullptr,
		screenshot->format->format, screenshot->pixels, screenshot->pitch);

	if (status == 0)
	{
		Debug::mention("Renderer", "Screenshot taken.");
	}
	else
	{
		Debug::crash("Renderer", "Couldn't take screenshot, " +
			std::string(SDL_GetError()));
	}

	return screenshot;
}

Int2 Renderer::nativePointToOriginal(const Int2 &nativePoint) const
{
	// From native point to letterbox point.
	const Int2 windowDimensions = this->getWindowDimensions();
	const SDL_Rect letterbox = this->getLetterboxDimensions();

	const Int2 letterboxPoint(
		nativePoint.x - letterbox.x,
		nativePoint.y - letterbox.y);

	// Then from letterbox point to original point.
	const double letterboxXPercent = static_cast<double>(letterboxPoint.x) /
		static_cast<double>(letterbox.w);
	const double letterboxYPercent = static_cast<double>(letterboxPoint.y) /
		static_cast<double>(letterbox.h);

	const double originalWidthReal = static_cast<double>(Renderer::ORIGINAL_WIDTH);
	const double originalHeightReal = static_cast<double>(Renderer::ORIGINAL_HEIGHT);

	const Int2 originalPoint(
		static_cast<int>(originalWidthReal * letterboxXPercent),
		static_cast<int>(originalHeightReal * letterboxYPercent));

	return originalPoint;
}

Int2 Renderer::originalPointToNative(const Int2 &originalPoint) const
{
	// From original point to letterbox point.
	const double originalXPercent = static_cast<double>(originalPoint.x) /
		static_cast<double>(Renderer::ORIGINAL_WIDTH);
	const double originalYPercent = static_cast<double>(originalPoint.y) /
		static_cast<double>(Renderer::ORIGINAL_HEIGHT);

	const SDL_Rect letterbox = this->getLetterboxDimensions();

	const double letterboxWidthReal = static_cast<double>(letterbox.w);
	const double letterboxHeightReal = static_cast<double>(letterbox.h);

	const Int2 letterboxPoint(
		static_cast<int>(letterboxWidthReal * originalXPercent),
		static_cast<int>(letterboxHeightReal * originalYPercent));

	// Then from letterbox point to native point.
	const Int2 nativePoint(
		letterboxPoint.x + letterbox.x,
		letterboxPoint.y + letterbox.y);

	return nativePoint;
}

bool Renderer::letterboxContains(const Int2 &nativePoint) const
{
	const SDL_Rect letterbox = this->getLetterboxDimensions();
	const Rect rectangle(letterbox.x, letterbox.y,
		letterbox.w, letterbox.h);
	return rectangle.contains(nativePoint);
}

SDL_Texture *Renderer::createTexture(uint32_t format, int access, int w, int h)
{
	return SDL_CreateTexture(this->renderer, format, access, w, h);
}

SDL_Texture *Renderer::createTextureFromSurface(SDL_Surface *surface)
{
	return SDL_CreateTextureFromSurface(this->renderer, surface);
}

void Renderer::resize(int width, int height, double resolutionScale)
{
	// The window's dimensions are resized automatically. The renderer's are not.
	const auto *nativeSurface = this->getWindowSurface();
	Debug::check(nativeSurface->w == width, "Renderer", "Mismatched resize widths.");
	Debug::check(nativeSurface->h == height, "Renderer", "Mismatched resize heights.");

	SDL_RenderSetLogicalSize(this->renderer, width, height);

	// Reinitialize native frame buffer.
	SDL_DestroyTexture(this->nativeTexture);
	this->nativeTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_TARGET, width, height);
	Debug::check(this->nativeTexture != nullptr, "Renderer",
		"Couldn't recreate native frame buffer, " + std::string(SDL_GetError()));

	// Rebuild the 3D renderer if initialized.
	if (this->softwareRenderer.get() != nullptr)
	{
		// Height of the game world view in pixels. Determined by whether the game 
		// interface is visible or not.
		const int viewHeight = this->getViewHeight();

		// Make sure render dimensions are at least 1x1.
		const int renderWidth = std::max(static_cast<int>(width * resolutionScale), 1);
		const int renderHeight = std::max(static_cast<int>(viewHeight * resolutionScale), 1);

		// Reinitialize the game world frame buffer.
		SDL_DestroyTexture(this->gameWorldTexture);
		this->gameWorldTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
			SDL_TEXTUREACCESS_STREAMING, renderWidth, renderHeight);
		Debug::check(this->gameWorldTexture != nullptr, "Renderer",
			"Couldn't recreate game world texture, " + std::string(SDL_GetError()));

		// Resize 3D renderer.
		this->softwareRenderer->resize(renderWidth, renderHeight);
	}
}

void Renderer::setWindowIcon(SDL_Surface *icon)
{
	SDL_SetWindowIcon(this->window, icon);
}

void Renderer::setWindowTitle(const std::string &title)
{
	SDL_SetWindowTitle(this->window, title.c_str());
}

void Renderer::warpMouse(int x, int y)
{
	SDL_WarpMouseInWindow(this->window, x, y);
}

void Renderer::useTransparencyBlending(bool blend)
{
	int status = SDL_SetTextureBlendMode(this->originalTexture,
		blend ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
	Debug::check(status == 0, "Renderer", "Couldn't set blending mode, " +
		std::string(SDL_GetError()));
}

void Renderer::initializeWorldRendering(double resolutionScale, bool fullGameWindow)
{
	this->fullGameWindow = fullGameWindow;

	const int screenWidth = this->getWindowDimensions().x;
	const int screenHeight = this->getWindowDimensions().y;

	// Height of the game world view in pixels. Determined by whether the game 
	// interface is visible or not.
	const int viewHeight = this->getViewHeight();

	// Make sure render dimensions are at least 1x1.
	const int renderWidth = std::max(static_cast<int>(screenWidth * resolutionScale), 1);
	const int renderHeight = std::max(static_cast<int>(viewHeight * resolutionScale), 1);

	// Remove any previous game world frame buffer.
	if (this->softwareRenderer.get() != nullptr)
	{
		SDL_DestroyTexture(this->gameWorldTexture);
	}

	// Initialize a new game world frame buffer.
	this->gameWorldTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_STREAMING, renderWidth, renderHeight);
	Debug::check(this->gameWorldTexture != nullptr, "Renderer",
		"Couldn't create game world texture, " + std::string(SDL_GetError()));

	// Initialize 3D rendering program.
	this->softwareRenderer = std::unique_ptr<SoftwareRenderer>(new SoftwareRenderer(
		renderWidth, renderHeight));
}

void Renderer::updateCamera(const Double3 &eye, const Double3 &direction, double fovY)
{
	assert(this->softwareRenderer.get() != nullptr);
	this->softwareRenderer->setEye(eye);
	this->softwareRenderer->setForward(direction);
	this->softwareRenderer->setFovY(fovY);
}

void Renderer::updateGameTime(double gameTime)
{
	//assert(this->clProgram.get() != nullptr);
	//this->clProgram->updateGameTime(gameTime);
	Debug::crash("Renderer", "updateGameTime() not implemented.");
}

void Renderer::updateViewDistance(double viewDistance)
{
	assert(this->softwareRenderer.get() != nullptr);
	this->softwareRenderer->setViewDistance(viewDistance);
}

int Renderer::addTexture(const uint32_t *pixels, int width, int height)
{
	assert(this->softwareRenderer.get() != nullptr);
	return this->softwareRenderer->addTexture(pixels, width, height);
}

void Renderer::updateVoxel(int x, int y, int z, const std::vector<Rect3D> &rects,
	const std::vector<int> &textureIndices)
{
	//assert(this->clProgram.get() != nullptr);
	//this->clProgram->queueVoxelUpdate(x, y, z, rects, textureIndices);
	Debug::crash("Renderer", "updateVoxel() not implemented.");
}

void Renderer::updateVoxel(int x, int y, int z,
	const std::vector<Rect3D> &rects, int textureIndex)
{
	//assert(this->clProgram.get() != nullptr);
	//this->clProgram->queueVoxelUpdate(x, y, z, rects, textureIndex);
	Debug::crash("Renderer", "updateVoxel() not implemented.");
}

void Renderer::updateSprite(int spriteID, const Rect3D &rect, int textureIndex)
{
	//assert(this->clProgram.get() != nullptr);
	//this->clProgram->queueSpriteUpdate(spriteID, rect, textureIndex);
	Debug::crash("Renderer", "updateSprite() not implemented.");
}

void Renderer::removeSprite(int spriteID)
{
	//assert(this->clProgram.get() != nullptr);
	//this->clProgram->queueSpriteRemoval(spriteID);
	Debug::crash("Renderer", "removeSprite() not implemented.");
}

void Renderer::updateLight(int lightID, const Double3 &point,
	const Double3 &color, double intensity)
{
	//assert(this->clProgram.get() != nullptr);
	//this->clProgram->queueLightUpdate(lightID, point, color, intensity);
	Debug::crash("Renderer", "updateLight() not implemented.");
}

void Renderer::removeLight(int lightID)
{
	//assert(this->clProgram.get() != nullptr);
	//this->clProgram->queueLightRemoval(lightID);
	Debug::crash("Renderer", "removeLight() not implemented.");
}

void Renderer::clearNative(const Color &color)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture);
	SDL_SetRenderDrawColor(this->renderer, color.getR(), color.getG(),
		color.getB(), color.getA());
	SDL_RenderClear(this->renderer);
}

void Renderer::clearNative()
{
	this->clearNative(Color::Black);
}

void Renderer::clearOriginal(const Color &color)
{
	SDL_SetRenderTarget(this->renderer, this->originalTexture);
	SDL_SetRenderDrawColor(this->renderer, color.getR(), color.getG(),
		color.getB(), color.getA());
	SDL_RenderClear(this->renderer);
}

void Renderer::clearOriginal()
{
	this->clearOriginal(Color::Transparent);
}

void Renderer::drawNativePixel(const Color &color, int x, int y)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture);
	SDL_SetRenderDrawColor(this->renderer, color.getR(), color.getG(),
		color.getB(), color.getA());
	SDL_RenderDrawPoint(this->renderer, x, y);
}

void Renderer::drawNativeLine(const Color &color, int x1, int y1, int x2, int y2)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture);
	SDL_SetRenderDrawColor(this->renderer, color.getR(), color.getG(),
		color.getB(), color.getA());
	SDL_RenderDrawLine(this->renderer, x1, y1, x2, y2);
}

void Renderer::drawNativeRect(const Color &color, int x, int y, int w, int h)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture);
	SDL_SetRenderDrawColor(this->renderer, color.getR(), color.getG(),
		color.getB(), color.getA());

	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_RenderDrawRect(this->renderer, &rect);
}

void Renderer::drawOriginalPixel(const Color &color, int x, int y)
{
	SDL_SetRenderTarget(this->renderer, this->originalTexture);
	SDL_SetRenderDrawColor(this->renderer, color.getR(), color.getG(),
		color.getB(), color.getA());
	SDL_RenderDrawPoint(this->renderer, x, y);
}

void Renderer::drawOriginalLine(const Color &color, int x1, int y1, int x2, int y2)
{
	SDL_SetRenderTarget(this->renderer, this->originalTexture);
	SDL_SetRenderDrawColor(this->renderer, color.getR(), color.getG(),
		color.getB(), color.getA());
	SDL_RenderDrawLine(this->renderer, x1, y1, x2, y2);
}

void Renderer::drawOriginalRect(const Color &color, int x, int y, int w, int h)
{
	SDL_SetRenderTarget(this->renderer, this->originalTexture);
	SDL_SetRenderDrawColor(this->renderer, color.getR(), color.getG(),
		color.getB(), color.getA());

	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_RenderDrawRect(this->renderer, &rect);
}

void Renderer::fillNativeRect(const Color &color, int x, int y, int w, int h)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture);
	SDL_SetRenderDrawColor(this->renderer, color.getR(), color.getG(),
		color.getB(), color.getA());

	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_RenderFillRect(this->renderer, &rect);
}

void Renderer::fillOriginalRect(const Color &color, int x, int y, int w, int h)
{
	SDL_SetRenderTarget(this->renderer, this->originalTexture);
	SDL_SetRenderDrawColor(this->renderer, color.getR(), color.getG(),
		color.getB(), color.getA());

	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_RenderFillRect(this->renderer, &rect);
}

void Renderer::renderWorld(const std::vector<char> &voxelGrid, int gridWidth,
	int gridHeight, int gridDepth)
{
	// The 3D renderer must be initialized.
	assert(this->softwareRenderer.get() != nullptr);

	// Render the game world to a frame buffer.
	this->softwareRenderer->render(voxelGrid, gridWidth, gridHeight, gridDepth);

	int renderWidth;
	SDL_QueryTexture(this->gameWorldTexture, nullptr, nullptr, &renderWidth, nullptr);

	// Send the ARGB8888 pixels to the game world texture. Later, this step can be 
	// skipped once using a graphics API.
	const uint32_t *pixels = this->softwareRenderer->getPixels();
	const int pitch = renderWidth * sizeof(*pixels);
	SDL_UpdateTexture(this->gameWorldTexture, nullptr,
		static_cast<const void*>(pixels), pitch);

	// Now copy to the native frame buffer (stretching if needed).
	const int screenWidth = this->getWindowDimensions().x;
	const int viewHeight = this->getViewHeight();
	this->drawToNative(this->gameWorldTexture, 0, 0, screenWidth, viewHeight);
}

void Renderer::drawToNative(SDL_Texture *texture, int x, int y, int w, int h)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture);

	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_RenderCopy(this->renderer, texture, nullptr, &rect);
}

void Renderer::drawToNative(SDL_Texture *texture, int x, int y)
{
	int width, height;
	SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

	this->drawToNative(texture, x, y, width, height);
}

void Renderer::drawToNative(SDL_Texture *texture)
{
	this->drawToNative(texture, 0, 0);
}

void Renderer::drawToOriginal(SDL_Texture *texture, int x, int y, int w, int h)
{
	SDL_SetRenderTarget(this->renderer, this->originalTexture);

	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_RenderCopy(this->renderer, texture, nullptr, &rect);
}

void Renderer::drawToOriginal(SDL_Texture *texture, int x, int y)
{
	int width, height;
	SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

	this->drawToOriginal(texture, x, y, width, height);
}

void Renderer::drawToOriginal(SDL_Texture *texture)
{
	this->drawToOriginal(texture, 0, 0);
}

void Renderer::fillNative(SDL_Texture *texture)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture);
	SDL_RenderCopy(this->renderer, texture, nullptr, nullptr);
}

void Renderer::drawOriginalToNative()
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture);

	// The original frame buffer should always be cleared with a fully transparent 
	// color, not just black.

	SDL_Rect rect = this->getLetterboxDimensions();
	SDL_RenderCopy(this->renderer, this->originalTexture, nullptr, &rect);
}

void Renderer::present()
{
	SDL_SetRenderTarget(this->renderer, nullptr);
	SDL_RenderCopy(this->renderer, this->nativeTexture, nullptr, nullptr);
	SDL_RenderPresent(this->renderer);
}
