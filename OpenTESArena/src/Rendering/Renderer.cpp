#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

#include "SDL.h"

#include "Renderer.h"
#include "Surface.h"
#include "../Interface/CursorAlignment.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Rect.h"
#include "../Media/Color.h"
#include "../Utilities/Platform.h"
#include "../World/VoxelGrid.h"

#include "components/debug/Debug.h"

Renderer::DisplayMode::DisplayMode(int width, int height, int refreshRate)
{
	this->width = width;
	this->height = height;
	this->refreshRate = refreshRate;
}

Renderer::ProfilerData::ProfilerData()
{
	this->width = 0;
	this->height = 0;
	this->frameTime = 0.0;
}

const char *Renderer::DEFAULT_RENDER_SCALE_QUALITY = "nearest";
const char *Renderer::DEFAULT_TITLE = "OpenTESArena";
const int Renderer::ORIGINAL_WIDTH = 320;
const int Renderer::ORIGINAL_HEIGHT = 200;
const int Renderer::DEFAULT_BPP = 32;
const uint32_t Renderer::DEFAULT_PIXELFORMAT = SDL_PIXELFORMAT_ARGB8888;

Renderer::Renderer()
{
	DebugAssert(this->nativeTexture.get() == nullptr);
	DebugAssert(this->gameWorldTexture.get() == nullptr);
	this->window = nullptr;
	this->renderer = nullptr;
	this->letterboxMode = 0;
	this->fullGameWindow = false;
}

Renderer::~Renderer()
{
	DebugLog("Closing.");

	SDL_DestroyWindow(this->window);

	// This also destroys the frame buffer textures.
	SDL_DestroyRenderer(this->renderer);
}

SDL_Renderer *Renderer::createRenderer(SDL_Window *window)
{
	// Automatically choose the best driver.
	const int bestDriver = -1;

	SDL_Renderer *rendererContext = SDL_CreateRenderer(
		window, bestDriver, SDL_RENDERER_ACCELERATED);
	DebugAssertMsg(rendererContext != nullptr, "SDL_CreateRenderer");

	// Set pixel interpolation hint.
	SDL_bool status = SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,
		Renderer::DEFAULT_RENDER_SCALE_QUALITY);
	if (status != SDL_TRUE)
	{
		DebugLogWarning("Could not set interpolation hint.");
	}

	// Set the size of the render texture to be the size of the whole screen
	// (it automatically scales otherwise).
	SDL_Surface *nativeSurface = SDL_GetWindowSurface(window);

	// If this fails, we might not support hardware accelerated renderers for some reason
	// (such as with Linux), so we retry with software.
	if (!nativeSurface)
	{
		DebugLogWarning("Failed to init accelerated SDL_Renderer, trying software fallback.");

		SDL_DestroyRenderer(rendererContext);

		rendererContext = SDL_CreateRenderer(window, bestDriver, SDL_RENDERER_SOFTWARE);
		DebugAssertMsg(rendererContext != nullptr, "SDL_CreateRenderer software");

		nativeSurface = SDL_GetWindowSurface(window);
	}

	DebugAssertMsg(nativeSurface != nullptr, "SDL_GetWindowSurface");

	// Set the device-independent resolution for rendering (i.e., the 
	// "behind-the-scenes" resolution).
	SDL_RenderSetLogicalSize(rendererContext, nativeSurface->w, nativeSurface->h);

	return rendererContext;
}

int Renderer::makeRendererDimension(int value, double resolutionScale)
{
	// Make sure renderer dimensions are at least 1x1, and round to make sure an
	// imprecise resolution scale doesn't result in off-by-one resolutions (like 1079p).
	return std::max(static_cast<int>(
		std::round(static_cast<double>(value) * resolutionScale)), 1);
}

double Renderer::getLetterboxAspect() const
{
	if (this->letterboxMode == 0)
	{
		// 16:10.
		return 16.0 / 10.0;
	}
	else if (this->letterboxMode == 1)
	{
		// 4:3.
		return 4.0 / 3.0;
	}
	else if (this->letterboxMode == 2)
	{
		// Stretch to fill.
		const Int2 windowDims = this->getWindowDimensions();
		return static_cast<double>(windowDims.x) / static_cast<double>(windowDims.y);
	}
	else
	{
		DebugUnhandledReturnMsg(double, std::to_string(this->letterboxMode));
	}
}

Int2 Renderer::getWindowDimensions() const
{
	const SDL_Surface *nativeSurface = SDL_GetWindowSurface(this->window);
	return Int2(nativeSurface->w, nativeSurface->h);
}

const std::vector<Renderer::DisplayMode> &Renderer::getDisplayModes() const
{
	return this->displayModes;
}

double Renderer::getDpiScale() const
{
	const double platformDpi = Platform::getDefaultDPI();	
	const int displayIndex = SDL_GetWindowDisplayIndex(this->window);

	float hdpi;
	if (SDL_GetDisplayDPI(displayIndex, nullptr, &hdpi, nullptr) == 0)
	{
		return static_cast<double>(hdpi) / platformDpi;
	}
	else
	{
		DebugLogWarning("Couldn't get DPI of display \"" + std::to_string(displayIndex) + "\".");
		return 1.0;
	}
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
	const SDL_Surface *nativeSurface = SDL_GetWindowSurface(this->window);
	const double nativeAspect = static_cast<double>(nativeSurface->w) /
		static_cast<double>(nativeSurface->h);
	const double letterboxAspect = this->getLetterboxAspect();

	// Compare the two aspects to decide what the letterbox dimensions are.
	if (std::abs(nativeAspect - letterboxAspect) < Constants::Epsilon)
	{
		// Equal aspects. The letterbox is equal to the screen size.
		SDL_Rect rect;
		rect.x = 0;
		rect.y = 0;
		rect.w = nativeSurface->w;
		rect.h = nativeSurface->h;
		return rect;
	}
	else if (nativeAspect > letterboxAspect)
	{
		// Native window is wider = empty left and right.
		int subWidth = static_cast<int>(std::ceil(
			static_cast<double>(nativeSurface->h) * letterboxAspect));
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
			static_cast<double>(nativeSurface->w) / letterboxAspect));
		SDL_Rect rect;
		rect.x = 0;
		rect.y = (nativeSurface->h - subHeight) / 2;
		rect.w = nativeSurface->w;
		rect.h = subHeight;
		return rect;
	}
}

Surface Renderer::getScreenshot() const
{
	const Int2 dimensions = this->getWindowDimensions();
	Surface screenshot = Surface::createWithFormat(dimensions.x, dimensions.y,
		Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);

	const int status = SDL_RenderReadPixels(this->renderer, nullptr,
		screenshot.get()->format->format, screenshot.get()->pixels, screenshot.get()->pitch);

	if (status != 0)
	{
		DebugCrash("Couldn't take screenshot, " + std::string(SDL_GetError()));
	}

	return screenshot;
}

const Renderer::ProfilerData &Renderer::getProfilerData() const
{
	return this->profilerData;
}

bool Renderer::getEntityRayIntersection(const EntityManager::EntityVisibilityData &visData,
	int flatIndex, const Double3 &entityForward, const Double3 &entityRight,
	const Double3 &entityUp, double entityWidth, double entityHeight, const Double3 &rayPoint,
	const Double3 &rayDirection, bool pixelPerfect, Double3 *outHitPoint) const
{
	DebugAssert(this->softwareRenderer.isInited());
	const Entity &entity = *visData.entity;

	// Do a ray test to see if the ray intersects.
	if (MathUtils::rayPlaneIntersection(rayPoint, rayDirection, visData.flatPosition,
		entityForward, outHitPoint))
	{
		const Double3 diff = (*outHitPoint) - visData.flatPosition;

		// Get the texture coordinates. It's okay if they are outside the entity.
		const Double2 uv(
			0.5 - (diff.dot(entityRight) / entityWidth),
			1.0 - (diff.dot(entityUp) / entityHeight));

		// See if the ray successfully hit a point on the entity, and that point is considered
		// selectable (i.e. it's not transparent).
		bool isSelected;
		const bool withinEntity = this->softwareRenderer.tryGetEntitySelectionData(uv, flatIndex,
			visData.keyframe.getTextureID(), visData.anglePercent, visData.stateType,
			pixelPerfect, &isSelected);

		return withinEntity && isSelected;
	}
	else
	{
		// Did not intersect the entity's plane.
		return false;
	}
}

Double3 Renderer::screenPointToRay(double xPercent, double yPercent, const Double3 &cameraDirection,
	double fovY, double aspect) const
{
	return SoftwareRenderer::screenPointToRay(xPercent, yPercent, cameraDirection, fovY, aspect);
}

Int2 Renderer::nativeToOriginal(const Int2 &nativePoint) const
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

Rect Renderer::nativeToOriginal(const Rect &nativeRect) const
{
	const Int2 newTopLeft = this->nativeToOriginal(nativeRect.getTopLeft());
	const Int2 newBottomRight = this->nativeToOriginal(nativeRect.getBottomRight());
	return Rect(
		newTopLeft.x,
		newTopLeft.y,
		newBottomRight.x - newTopLeft.x,
		newBottomRight.y - newTopLeft.y);
}

Int2 Renderer::originalToNative(const Int2 &originalPoint) const
{
	// From original point to letterbox point.
	const double originalXPercent = static_cast<double>(originalPoint.x) /
		static_cast<double>(Renderer::ORIGINAL_WIDTH);
	const double originalYPercent = static_cast<double>(originalPoint.y) /
		static_cast<double>(Renderer::ORIGINAL_HEIGHT);

	const SDL_Rect letterbox = this->getLetterboxDimensions();

	const double letterboxWidthReal = static_cast<double>(letterbox.w);
	const double letterboxHeightReal = static_cast<double>(letterbox.h);

	// Convert to letterbox point. Round to avoid off-by-one errors.
	const Int2 letterboxPoint(
		static_cast<int>(std::round(letterboxWidthReal * originalXPercent)),
		static_cast<int>(std::round(letterboxHeightReal * originalYPercent)));

	// Then from letterbox point to native point.
	const Int2 nativePoint(
		letterboxPoint.x + letterbox.x,
		letterboxPoint.y + letterbox.y);

	return nativePoint;
}

Rect Renderer::originalToNative(const Rect &originalRect) const
{
	const Int2 newTopLeft = this->originalToNative(originalRect.getTopLeft());
	const Int2 newBottomRight = this->originalToNative(originalRect.getBottomRight());
	return Rect(
		newTopLeft.x,
		newTopLeft.y,
		newBottomRight.x - newTopLeft.x,
		newBottomRight.y - newTopLeft.y);
}

bool Renderer::letterboxContains(const Int2 &nativePoint) const
{
	const SDL_Rect letterbox = this->getLetterboxDimensions();
	const Rect rectangle(letterbox.x, letterbox.y,
		letterbox.w, letterbox.h);
	return rectangle.contains(nativePoint);
}

Texture Renderer::createTexture(uint32_t format, int access, int w, int h)
{
	SDL_Texture *tex = SDL_CreateTexture(this->renderer, format, access, w, h);
	if (tex == nullptr)
	{
		DebugLogError("Could not create SDL_Texture.");
	}

	Texture texture;
	texture.init(tex);
	return texture;
}

Texture Renderer::createTextureFromSurface(const Surface &surface)
{
	SDL_Texture *tex = SDL_CreateTextureFromSurface(this->renderer, surface.get());
	if (tex == nullptr)
	{
		DebugLogError("Could not create SDL_Texture from surface.");
	}

	Texture texture;
	texture.init(tex);
	return texture;
}

void Renderer::init(int width, int height, WindowMode windowMode, int letterboxMode)
{
	DebugLog("Initializing.");

	DebugAssert(width > 0);
	DebugAssert(height > 0);

	this->letterboxMode = letterboxMode;

	// Initialize window. The SDL_Surface is obtained from this window.
	this->window = [width, height, windowMode]()
	{
		const char *title = Renderer::DEFAULT_TITLE;
		const int position = [windowMode]() -> int
		{
			switch (windowMode)
			{
			case WindowMode::Window:
				return SDL_WINDOWPOS_CENTERED;
			case WindowMode::BorderlessFull:
				return SDL_WINDOWPOS_UNDEFINED;
			default:
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(windowMode)));
			}
		}();

		const uint32_t flags = SDL_WINDOW_RESIZABLE |
			((windowMode == WindowMode::BorderlessFull) ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) |
			SDL_WINDOW_ALLOW_HIGHDPI;

		// If fullscreen is true, then width and height are ignored. They are stored
		// behind the scenes for when the user changes to windowed mode, however.
		return SDL_CreateWindow(title, position, position, width, height, flags);
	}();

	DebugAssertMsg(this->window != nullptr, "SDL_CreateWindow");

	// Initialize renderer context.
	this->renderer = Renderer::createRenderer(this->window);

	// Initialize display modes list for the current window.
	const int displayIndex = SDL_GetWindowDisplayIndex(this->window);
	const int displayModeCount = SDL_GetNumDisplayModes(displayIndex);
	for (int i = 0; i < displayModeCount; i++)
	{
		// Convert SDL display mode to our display mode.
		SDL_DisplayMode mode;
		if (SDL_GetDisplayMode(displayIndex, i, &mode) == 0)
		{
			// Filter away non-24-bit displays. Perhaps this could be handled better, but I don't
			// know how to do that for all possible displays out there.
			if (mode.format == SDL_PIXELFORMAT_RGB888)
			{
				this->displayModes.push_back(DisplayMode(mode.w, mode.h, mode.refresh_rate));
			}
		}
	}

	// Use window dimensions, just in case it's fullscreen and the given width and
	// height are ignored.
	Int2 windowDimensions = this->getWindowDimensions();

	// Initialize native frame buffer.
	this->nativeTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_TARGET, windowDimensions.x, windowDimensions.y);
	DebugAssertMsg(this->nativeTexture.get() != nullptr,
		"Couldn't create native frame buffer, " + std::string(SDL_GetError()));

	// Don't initialize the game world buffer until the 3D renderer is initialized.
	DebugAssert(this->gameWorldTexture.get() == nullptr);
	this->fullGameWindow = false;
}

void Renderer::resize(int width, int height, double resolutionScale, bool fullGameWindow)
{
	// The window's dimensions are resized automatically. The renderer's are not.
	const SDL_Surface *nativeSurface = SDL_GetWindowSurface(this->window);
	DebugAssertMsg(nativeSurface->w == width, "Mismatched resize widths.");
	DebugAssertMsg(nativeSurface->h == height, "Mismatched resize heights.");

	SDL_RenderSetLogicalSize(this->renderer, width, height);

	// Reinitialize native frame buffer.
	this->nativeTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_TARGET, width, height);
	DebugAssertMsg(this->nativeTexture.get() != nullptr,
		"Couldn't recreate native frame buffer, " + std::string(SDL_GetError()));

	this->fullGameWindow = fullGameWindow;

	// Rebuild the 3D renderer if initialized.
	if (this->softwareRenderer.isInited())
	{
		// Height of the game world view in pixels. Determined by whether the game 
		// interface is visible or not.
		const int viewHeight = this->getViewHeight();

		// Calculate renderer dimensions.
		const int renderWidth = Renderer::makeRendererDimension(width, resolutionScale);
		const int renderHeight = Renderer::makeRendererDimension(viewHeight, resolutionScale);

		// Reinitialize the game world frame buffer.
		this->gameWorldTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
			SDL_TEXTUREACCESS_STREAMING, renderWidth, renderHeight);
		DebugAssertMsg(this->gameWorldTexture.get() != nullptr,
			"Couldn't recreate game world texture, " + std::string(SDL_GetError()));

		// Resize 3D renderer.
		this->softwareRenderer.resize(renderWidth, renderHeight);
	}
}

void Renderer::setLetterboxMode(int letterboxMode)
{
	this->letterboxMode = letterboxMode;
}

void Renderer::setWindowMode(WindowMode mode)
{
	const uint32_t flags = [mode]() -> uint32_t
	{
		// Use fake fullscreen for now.
		switch (mode)
		{
		case WindowMode::Window:
			return 0;
		case WindowMode::BorderlessFull:
			return SDL_WINDOW_FULLSCREEN_DESKTOP;
		default:
			DebugUnhandledReturnMsg(uint32_t, std::to_string(static_cast<int>(mode)));
		}
	}();

	SDL_SetWindowFullscreen(this->window, flags);

	// Reset the cursor to the center of the screen for consistency.
	const Int2 windowDims = this->getWindowDimensions();
	this->warpMouse(windowDims.x / 2, windowDims.y / 2);
}

void Renderer::setWindowIcon(const Surface &icon)
{
	SDL_SetWindowIcon(this->window, icon.get());
}

void Renderer::setWindowTitle(const char *title)
{
	SDL_SetWindowTitle(this->window, title);
}

void Renderer::warpMouse(int x, int y)
{
	SDL_WarpMouseInWindow(this->window, x, y);
}

void Renderer::setClipRect(const SDL_Rect *rect)
{
	SDL_RenderSetClipRect(this->renderer, rect);
}

void Renderer::initializeWorldRendering(double resolutionScale, bool fullGameWindow,
	int renderThreadsMode)
{
	this->fullGameWindow = fullGameWindow;

	const int screenWidth = this->getWindowDimensions().x;

	// Height of the game world view in pixels, used in place of the screen height.
	// Its value is a function of whether the game interface is visible or not.
	const int viewHeight = this->getViewHeight();

	// Make sure render dimensions are at least 1x1.
	const int renderWidth = Renderer::makeRendererDimension(screenWidth, resolutionScale);
	const int renderHeight = Renderer::makeRendererDimension(viewHeight, resolutionScale);

	// Initialize a new game world frame buffer, removing any previous game world frame buffer.
	this->gameWorldTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_STREAMING, renderWidth, renderHeight);
	DebugAssertMsg(this->gameWorldTexture.get() != nullptr,
		"Couldn't create game world texture, " + std::string(SDL_GetError()));

	// Initialize 3D rendering.
	this->softwareRenderer.init(renderWidth, renderHeight, renderThreadsMode);
}

void Renderer::setRenderThreadsMode(int mode)
{
	DebugAssert(this->softwareRenderer.isInited());
	this->softwareRenderer.setRenderThreadsMode(mode);
}

void Renderer::addLight(int id, const Double3 &point, const Double3 &color, double intensity)
{
	DebugAssert(this->softwareRenderer.isInited());
	this->softwareRenderer.addLight(id, point, color, intensity);
}

void Renderer::updateLight(int id, const Double3 *point, const Double3 *color, 
	const double *intensity)
{
	DebugAssert(this->softwareRenderer.isInited());
	this->softwareRenderer.updateLight(id, point, color, intensity);
}

void Renderer::setFogDistance(double fogDistance)
{
	DebugAssert(this->softwareRenderer.isInited());
	this->softwareRenderer.setFogDistance(fogDistance);
}

void Renderer::setVoxelTexture(int id, const uint8_t *srcTexels, const Palette &palette)
{
	DebugAssert(this->softwareRenderer.isInited());
	this->softwareRenderer.setVoxelTexture(id, srcTexels, palette);
}

void Renderer::addFlatTexture(int flatIndex, EntityAnimationData::StateType stateType,
	int angleID, bool flipped, const uint8_t *srcTexels, int width, int height,
	const Palette &palette)
{
	DebugAssert(this->softwareRenderer.isInited());
	this->softwareRenderer.addFlatTexture(flatIndex, stateType, angleID, flipped,
		srcTexels, width, height, palette);
}

void Renderer::addChasmTexture(VoxelDefinition::ChasmData::Type chasmType, const uint8_t *colors,
	int width, int height, const Palette &palette)
{
	DebugAssert(this->softwareRenderer.isInited());
	this->softwareRenderer.addChasmTexture(chasmType, colors, width, height, palette);
}

void Renderer::setDistantSky(const DistantSky &distantSky, const Palette &palette)
{
	DebugAssert(this->softwareRenderer.isInited());
	this->softwareRenderer.setDistantSky(distantSky, palette);
}

void Renderer::setSkyPalette(const uint32_t *colors, int count)
{
	DebugAssert(this->softwareRenderer.isInited());
	this->softwareRenderer.setSkyPalette(colors, count);
}

void Renderer::setNightLightsActive(bool active)
{
	DebugAssert(this->softwareRenderer.isInited());
	this->softwareRenderer.setNightLightsActive(active);
}

void Renderer::removeLight(int id)
{
	DebugAssert(this->softwareRenderer.isInited());
	this->softwareRenderer.removeLight(id);
}

void Renderer::clearTextures()
{
	DebugAssert(this->softwareRenderer.isInited());
	this->softwareRenderer.clearTextures();
}

void Renderer::clearDistantSky()
{
	DebugAssert(this->softwareRenderer.isInited());
	this->softwareRenderer.clearDistantSky();
}

void Renderer::clear(const Color &color)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderClear(this->renderer);
}

void Renderer::clear()
{
	this->clear(Color::Black);
}

void Renderer::clearOriginal(const Color &color)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);

	const SDL_Rect rect = this->getLetterboxDimensions();
	SDL_RenderFillRect(this->renderer, &rect);
}

void Renderer::clearOriginal()
{
	this->clearOriginal(Color::Black);
}

void Renderer::drawPixel(const Color &color, int x, int y)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawPoint(this->renderer, x, y);
}

void Renderer::drawLine(const Color &color, int x1, int y1, int x2, int y2)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawLine(this->renderer, x1, y1, x2, y2);
}

void Renderer::drawRect(const Color &color, int x, int y, int w, int h)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);

	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_RenderDrawRect(this->renderer, &rect);
}

void Renderer::fillRect(const Color &color, int x, int y, int w, int h)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);

	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_RenderFillRect(this->renderer, &rect);
}

void Renderer::fillOriginalRect(const Color &color, int x, int y, int w, int h)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);

	const Rect rect = this->originalToNative(Rect(x, y, w, h));
	SDL_RenderFillRect(this->renderer, &rect.getRect());
}

void Renderer::renderWorld(const Double3 &eye, const Double3 &forward, double fovY,
	double ambient, double daytimePercent, double chasmAnimPercent, double latitude, bool parallaxSky,
	double ceilingHeight, const std::vector<LevelData::DoorState> &openDoors,
	const std::vector<LevelData::FadeState> &fadingVoxels, const VoxelGrid &voxelGrid,
	const EntityManager &entityManager)
{
	// The 3D renderer must be initialized.
	DebugAssert(this->softwareRenderer.isInited());
	
	// Lock the game world texture and give the pixel pointer to the software renderer.
	// - Supposedly this is faster than SDL_UpdateTexture(). In any case, there's one
	//   less frame buffer to take care of.
	uint32_t *gameWorldPixels;
	int gameWorldPitch;
	int status = SDL_LockTexture(this->gameWorldTexture.get(), nullptr,
		reinterpret_cast<void**>(&gameWorldPixels), &gameWorldPitch);
	DebugAssertMsg(status == 0, "Couldn't lock game world texture, " +
		std::string(SDL_GetError()));

	// Render the game world to the game world frame buffer.
	const auto startTime = std::chrono::high_resolution_clock::now();
	this->softwareRenderer.render(eye, forward, fovY, ambient, daytimePercent, chasmAnimPercent,
		latitude, parallaxSky, ceilingHeight, openDoors, fadingVoxels, voxelGrid,
		entityManager, gameWorldPixels);
	const auto endTime = std::chrono::high_resolution_clock::now();

	// Update profiler stats.
	const SoftwareRenderer::ProfilerData swProfilerData = this->softwareRenderer.getProfilerData();
	this->profilerData.width = swProfilerData.width;
	this->profilerData.height = swProfilerData.height;
	this->profilerData.frameTime = static_cast<double>((endTime - startTime).count()) /
		static_cast<double>(std::nano::den);

	// Update the game world texture with the new ARGB8888 pixels.
	SDL_UnlockTexture(this->gameWorldTexture.get());

	// Now copy to the native frame buffer (stretching if needed).
	const int screenWidth = this->getWindowDimensions().x;
	const int viewHeight = this->getViewHeight();
	this->draw(this->gameWorldTexture, 0, 0, screenWidth, viewHeight);
}

void Renderer::drawCursor(const Texture &cursor, CursorAlignment alignment,
	const Int2 &mousePosition, double scale)
{
	// The caller should check for any null textures.
	DebugAssert(cursor.get() != nullptr);

	const int scaledWidth = static_cast<int>(std::round(cursor.getWidth() * scale));
	const int scaledHeight = static_cast<int>(std::round(cursor.getHeight() * scale));

	// Get the magnitude to offset the cursor's coordinates by.
	const Int2 cursorOffset = [alignment, scaledWidth, scaledHeight]()
	{
		const int xOffset = [alignment, scaledWidth]()
		{
			if ((alignment == CursorAlignment::TopLeft) ||
				(alignment == CursorAlignment::Left) ||
				(alignment == CursorAlignment::BottomLeft))
			{
				return 0;
			}
			else if ((alignment == CursorAlignment::Top) ||
				(alignment == CursorAlignment::Middle) ||
				(alignment == CursorAlignment::Bottom))
			{
				return scaledWidth / 2;
			}
			else
			{
				return scaledWidth - 1;
			}
		}();

		const int yOffset = [alignment, scaledHeight]()
		{
			if ((alignment == CursorAlignment::TopLeft) ||
				(alignment == CursorAlignment::Top) ||
				(alignment == CursorAlignment::TopRight))
			{
				return 0;
			}
			else if ((alignment == CursorAlignment::Left) ||
				(alignment == CursorAlignment::Middle) ||
				(alignment == CursorAlignment::Right))
			{
				return scaledHeight / 2;
			}
			else
			{
				return scaledHeight - 1;
			}
		}();

		return Int2(xOffset, yOffset);
	}();

	this->draw(cursor,
		mousePosition.x - cursorOffset.x,
		mousePosition.y - cursorOffset.y,
		scaledWidth,
		scaledHeight);
}

void Renderer::draw(const Texture &texture, int x, int y, int w, int h)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());

	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_RenderCopy(this->renderer, texture.get(), nullptr, &rect);
}

void Renderer::draw(const Texture &texture, int x, int y)
{
	int width, height;
	SDL_QueryTexture(texture.get(), nullptr, nullptr, &width, &height);

	this->draw(texture, x, y, width, height);
}

void Renderer::draw(const Texture &texture)
{
	this->draw(texture, 0, 0);
}

void Renderer::drawClipped(const Texture &texture, const Rect &srcRect, const Rect &dstRect)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_RenderCopy(this->renderer, texture.get(), &srcRect.getRect(), &dstRect.getRect());
}

void Renderer::drawClipped(const Texture &texture, const Rect &srcRect, int x, int y)
{
	this->drawClipped(texture, srcRect, Rect(x, y, srcRect.getWidth(), srcRect.getHeight()));
}

void Renderer::drawOriginal(const Texture &texture, int x, int y, int w, int h)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	
	// The given coordinates and dimensions are in 320x200 space, so transform them
	// to native space.
	const Rect rect = this->originalToNative(Rect(x, y, w, h));

	SDL_RenderCopy(this->renderer, texture.get(), nullptr, &rect.getRect());
}

void Renderer::drawOriginal(const Texture &texture, int x, int y)
{
	int width, height;
	SDL_QueryTexture(texture.get(), nullptr, nullptr, &width, &height);

	this->drawOriginal(texture, x, y, width, height);
}

void Renderer::drawOriginal(const Texture &texture)
{
	this->drawOriginal(texture, 0, 0);
}

void Renderer::drawOriginalClipped(const Texture &texture, const Rect &srcRect, const Rect &dstRect)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());

	// The destination coordinates and dimensions are in 320x200 space, so transform 
	// them to native space.
	const Rect rect = this->originalToNative(dstRect);

	SDL_RenderCopy(this->renderer, texture.get(), &srcRect.getRect(), &rect.getRect());
}

void Renderer::drawOriginalClipped(const Texture &texture, const Rect &srcRect, int x, int y)
{
	this->drawOriginalClipped(texture, srcRect, 
		Rect(x, y, srcRect.getWidth(), srcRect.getHeight()));
}

void Renderer::fill(const Texture &texture)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_RenderCopy(this->renderer, texture.get(), nullptr, nullptr);
}

void Renderer::present()
{
	SDL_SetRenderTarget(this->renderer, nullptr);
	SDL_RenderCopy(this->renderer, this->nativeTexture.get(), nullptr, nullptr);
	SDL_RenderPresent(this->renderer);
}
