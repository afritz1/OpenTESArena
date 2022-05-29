#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

#include "SDL.h"

#include "ArenaRenderUtils.h"
#include "RenderCamera.h"
#include "Renderer.h"
#include "RenderFrameSettings.h"
#include "RenderInitSettings.h"
#include "SdlUiRenderer.h"
#include "SoftwareRenderer.h"
#include "../Entities/EntityAnimationInstance.h"
#include "../Entities/EntityVisibilityState.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Rect.h"
#include "../Media/Color.h"
#include "../Media/TextureManager.h"
#include "../UI/CursorAlignment.h"
#include "../UI/RenderSpace.h"
#include "../UI/Surface.h"
#include "../Utilities/Platform.h"
#include "../World/LevelInstance.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	int GetSdlWindowPosition(Renderer::WindowMode windowMode)
	{
		switch (windowMode)
		{
		case Renderer::WindowMode::Window:
			return SDL_WINDOWPOS_CENTERED;
		case Renderer::WindowMode::BorderlessFullscreen:
		case Renderer::WindowMode::ExclusiveFullscreen:
			return SDL_WINDOWPOS_UNDEFINED;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(windowMode)));
		}
	}

	uint32_t GetSdlWindowFlags(Renderer::WindowMode windowMode)
	{
		uint32_t flags = SDL_WINDOW_ALLOW_HIGHDPI;
		if (windowMode == Renderer::WindowMode::Window)
		{
			flags |= SDL_WINDOW_RESIZABLE;
		}
		else if (windowMode == Renderer::WindowMode::BorderlessFullscreen)
		{
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		}
		else if (windowMode == Renderer::WindowMode::ExclusiveFullscreen)
		{
			flags |= SDL_WINDOW_FULLSCREEN;
		}

		return flags;
	}

	Int2 GetWindowDimsForMode(Renderer::WindowMode windowMode, int fallbackWidth, int fallbackHeight)
	{
		if (windowMode == Renderer::WindowMode::ExclusiveFullscreen)
		{
			// Use desktop resolution of the primary display device. In the future, the display index could be
			// an option in the options menu.
			constexpr int displayIndex = 0;
			SDL_DisplayMode displayMode;
			const int result = SDL_GetDesktopDisplayMode(displayIndex, &displayMode);
			if (result == 0)
			{
				return Int2(displayMode.w, displayMode.h);
			}
			else
			{
				DebugLogError("Couldn't get desktop " + std::to_string(displayIndex) + " display mode, using given window dimensions \"" +
					std::to_string(fallbackWidth) + "x" + std::to_string(fallbackHeight) + "\" (" + std::string(SDL_GetError()) + ").");
			}
		}

		return Int2(fallbackWidth, fallbackHeight);
	}
}

Renderer::DisplayMode::DisplayMode(int width, int height, int refreshRate)
{
	this->width = width;
	this->height = height;
	this->refreshRate = refreshRate;
}

Renderer::ProfilerData::ProfilerData()
{
	this->width = -1;
	this->height = -1;
	this->threadCount = -1;
	this->potentiallyVisTriangleCount = -1;
	this->visTriangleCount = -1;
	this->visLightCount = -1;
	this->frameTime = 0.0;
}

void Renderer::ProfilerData::init(int width, int height, int threadCount, int potentiallyVisTriangleCount,
	int visTriangleCount, int visLightCount, double frameTime)
{
	this->width = width;
	this->height = height;
	this->threadCount = threadCount;
	this->potentiallyVisTriangleCount = potentiallyVisTriangleCount;
	this->visTriangleCount = visTriangleCount;
	this->visLightCount = visLightCount;
	this->frameTime = frameTime;
}

const char *Renderer::DEFAULT_RENDER_SCALE_QUALITY = "nearest";
const char *Renderer::DEFAULT_TITLE = "OpenTESArena";
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

	if (this->renderer2D)
	{
		this->renderer2D->shutdown();
	}
	
	if (this->renderer3D)
	{
		this->renderer3D->shutdown();
	}

	SDL_DestroyWindow(this->window);

	// This also destroys the frame buffer textures.
	SDL_DestroyRenderer(this->renderer);

	SDL_Quit();
}

SDL_Renderer *Renderer::createRenderer(SDL_Window *window)
{
	// Automatically choose the best driver.
	constexpr int bestDriver = -1;

	SDL_Renderer *rendererContext = SDL_CreateRenderer(window, bestDriver, SDL_RENDERER_ACCELERATED);
	if (rendererContext == nullptr)
	{
		DebugLogError("Couldn't create SDL_Renderer with driver \"" + std::to_string(bestDriver) + "\".");
		return nullptr;
	}

	SDL_RendererInfo rendererInfo;
	if (SDL_GetRendererInfo(rendererContext, &rendererInfo) < 0)
	{
		DebugLogError("Couldn't get SDL_RendererInfo (error: " + std::string(SDL_GetError()) + ").");
		return nullptr;
	}

	const std::string rendererInfoFlags = String::toHexString(rendererInfo.flags);
	DebugLog("Created renderer \"" + std::string(rendererInfo.name) + "\" (flags: 0x" + rendererInfoFlags + ").");

	// Set pixel interpolation hint.
	const SDL_bool status = SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, Renderer::DEFAULT_RENDER_SCALE_QUALITY);
	if (status != SDL_TRUE)
	{
		DebugLogWarning("Couldn't set SDL rendering interpolation hint.");
	}

	// Set the size of the render texture to be the size of the whole screen
	// (it automatically scales otherwise).
	const SDL_Surface *nativeSurface = SDL_GetWindowSurface(window);

	// If this fails, we might not support hardware accelerated renderers for some reason
	// (such as with Linux), so we retry with software.
	if (nativeSurface == nullptr)
	{
		DebugLogWarning("Failed to init accelerated SDL_Renderer, trying software fallback.");
		SDL_DestroyRenderer(rendererContext);

		rendererContext = SDL_CreateRenderer(window, bestDriver, SDL_RENDERER_SOFTWARE);
		if (rendererContext == nullptr)
		{
			DebugLogError("Couldn't create software fallback SDL_Renderer.");
			return nullptr;
		}

		nativeSurface = SDL_GetWindowSurface(window);
		if (nativeSurface == nullptr)
		{
			DebugLogError("Couldn't get software fallback SDL_Window surface.");
			return nullptr;
		}
	}

	// Set the device-independent resolution for rendering (i.e., the "behind-the-scenes" resolution).
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

double Renderer::getWindowAspect() const
{
	const Int2 dims = this->getWindowDimensions();
	return static_cast<double>(dims.x) / static_cast<double>(dims.y);
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

Int2 Renderer::getViewDimensions() const
{
	const Int2 windowDims = this->getWindowDimensions();
	const int screenHeight = windowDims.y;

	// Ratio of the view height and window height in 320x200.
	const double viewWindowRatio = static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT - 53) /
		ArenaRenderUtils::SCREEN_HEIGHT_REAL;

	// Actual view height to use.
	const int viewHeight = this->fullGameWindow ? screenHeight :
		static_cast<int>(std::ceil(screenHeight * viewWindowRatio));

	return Int2(windowDims.x, viewHeight);
}

SDL_Rect Renderer::getLetterboxDimensions() const
{
	const Int2 windowDims = this->getWindowDimensions();
	const double nativeAspect = static_cast<double>(windowDims.x) /
		static_cast<double>(windowDims.y);
	const double letterboxAspect = this->getLetterboxAspect();

	// Compare the two aspects to decide what the letterbox dimensions are.
	if (std::abs(nativeAspect - letterboxAspect) < Constants::Epsilon)
	{
		// Equal aspects. The letterbox is equal to the screen size.
		SDL_Rect rect;
		rect.x = 0;
		rect.y = 0;
		rect.w = windowDims.x;
		rect.h = windowDims.y;
		return rect;
	}
	else if (nativeAspect > letterboxAspect)
	{
		// Native window is wider = empty left and right.
		const int subWidth = static_cast<int>(std::ceil(
			static_cast<double>(windowDims.y) * letterboxAspect));
		SDL_Rect rect;
		rect.x = (windowDims.x - subWidth) / 2;
		rect.y = 0;
		rect.w = subWidth;
		rect.h = windowDims.y;
		return rect;
	}
	else
	{
		// Native window is taller = empty top and bottom.
		const int subHeight = static_cast<int>(std::ceil(
			static_cast<double>(windowDims.x) / letterboxAspect));
		SDL_Rect rect;
		rect.x = 0;
		rect.y = (windowDims.y - subHeight) / 2;
		rect.w = windowDims.x;
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

bool Renderer::getEntityRayIntersection(const EntityVisibilityState3D &visState,
	const EntityDefinition &entityDef, const VoxelDouble3 &entityForward, const VoxelDouble3 &entityRight,
	const VoxelDouble3 &entityUp, double entityWidth, double entityHeight, const CoordDouble3 &rayPoint,
	const VoxelDouble3 &rayDirection, bool pixelPerfect, const Palette &palette, CoordDouble3 *outHitPoint) const
{
	DebugAssert(this->renderer3D->isInited());
	const Entity &entity = *visState.entity;

	// Do a ray test to see if the ray intersects.
	const NewDouble3 absoluteRayPoint = VoxelUtils::coordToNewPoint(rayPoint);
	const NewDouble3 absoluteFlatPosition = VoxelUtils::coordToNewPoint(visState.flatPosition);
	NewDouble3 absoluteHitPoint;
	if (MathUtils::rayPlaneIntersection(absoluteRayPoint, rayDirection, absoluteFlatPosition,
		entityForward, &absoluteHitPoint))
	{
		const NewDouble3 diff = absoluteHitPoint - absoluteFlatPosition;

		// Get the texture coordinates. It's okay if they are outside the entity.
		const Double2 uv(
			0.5 - (diff.dot(entityRight) / entityWidth),
			1.0 - (diff.dot(entityUp) / entityHeight));

		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
		const EntityAnimationDefinition::State &animState = animDef.getState(visState.stateIndex);
		const EntityAnimationDefinition::KeyframeList &animKeyframeList = animState.getKeyframeList(visState.angleIndex);
		const EntityAnimationDefinition::Keyframe &animKeyframe = animKeyframeList.getKeyframe(visState.keyframeIndex);
		const TextureAsset &textureAsset = animKeyframe.getTextureAsset();
		const bool flipped = animKeyframeList.isFlipped();
		const bool reflective = (entityDef.getType() == EntityDefinition::Type::Doodad) && entityDef.getDoodad().puddle;

		// See if the ray successfully hit a point on the entity, and that point is considered
		// selectable (i.e. it's not transparent).
		//bool isSelected;

		// @todo: get the entity anim inst's texture ID instead of asset ref. Can probably remove 'palette' too.
		DebugLogError("Not implemented: getEntityRayIntersection");
		return false;
		/*const bool withinEntity = this->renderer3D->tryGetEntitySelectionData(uv, textureAsset, flipped, reflective, pixelPerfect, palette, &isSelected);

		*outHitPoint = VoxelUtils::newPointToCoord(absoluteHitPoint);
		return withinEntity && isSelected;*/
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
	return this->renderer3D->screenPointToRay(xPercent, yPercent, cameraDirection, fovY, aspect);
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

	const double originalWidthReal = ArenaRenderUtils::SCREEN_WIDTH_REAL;
	const double originalHeightReal = ArenaRenderUtils::SCREEN_HEIGHT_REAL;

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
		ArenaRenderUtils::SCREEN_WIDTH_REAL;
	const double originalYPercent = static_cast<double>(originalPoint.y) /
		ArenaRenderUtils::SCREEN_HEIGHT_REAL;

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

bool Renderer::init(int width, int height, WindowMode windowMode, int letterboxMode,
	const ResolutionScaleFunc &resolutionScaleFunc, RendererSystemType2D systemType2D, RendererSystemType3D systemType3D,
	int renderThreadsMode)
{
	DebugLog("Initializing.");
	const int result = SDL_Init(SDL_INIT_VIDEO); // Required for SDL_GetDesktopDisplayMode() to work for exclusive fullscreen.
	if (result != 0)
	{
		DebugLogError("Couldn't init SDL video subsystem (result: " + std::to_string(result) + ", " + std::string(SDL_GetError()) + ").");
		return false;
	}

	if ((width <= 0) || (height <= 0))
	{
		DebugLogError("Invalid renderer dimensions \"" + std::to_string(width) + "x" + std::to_string(height) + "\"");
		return false;
	}

	this->letterboxMode = letterboxMode;
	this->resolutionScaleFunc = resolutionScaleFunc;

	// Initialize SDL window.
	const char *windowTitle = Renderer::DEFAULT_TITLE;
	const int windowPosition = GetSdlWindowPosition(windowMode);
	const uint32_t windowFlags = GetSdlWindowFlags(windowMode);
	const Int2 windowDims = GetWindowDimsForMode(windowMode, width, height);
	this->window = SDL_CreateWindow(windowTitle, windowPosition, windowPosition, windowDims.x, windowDims.y, windowFlags);
	if (this->window == nullptr)
	{
		DebugLogError("Couldn't create SDL_Window (dimensions: " + std::to_string(width) + "x" + std::to_string(height) +
			", window mode: " + std::to_string(static_cast<int>(windowMode)) + ").");
		return false;
	}

	// Initialize SDL renderer context.
	this->renderer = Renderer::createRenderer(this->window);
	if (this->renderer == nullptr)
	{
		DebugLogError("Couldn't create SDL_Renderer.");
		return false;
	}

	// Initialize display modes list for the current window.
	// @todo: these display modes will only work on the display device the window was initialized on
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
				this->displayModes.emplace_back(DisplayMode(mode.w, mode.h, mode.refresh_rate));
			}
		}
	}

	// Use window dimensions, just in case it's fullscreen and the given width and
	// height are ignored.
	const Int2 windowDimensions = this->getWindowDimensions();

	// Initialize native frame buffer.
	this->nativeTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_TARGET, windowDimensions.x, windowDimensions.y);
	if (this->nativeTexture.get() == nullptr)
	{
		DebugLogError("Couldn't create SDL_Texture frame buffer (error: " + std::string(SDL_GetError()) + ").");
		return false;
	}

	// Initialize 2D renderer.
	this->renderer2D = [systemType2D]() -> std::unique_ptr<RendererSystem2D>
	{
		if (systemType2D == RendererSystemType2D::SDL2)
		{
			return std::make_unique<SdlUiRenderer>();
		}
		else
		{
			DebugLogError("Unrecognized 2D renderer system type \"" +
				std::to_string(static_cast<int>(systemType2D)) + "\".");
			return nullptr;
		}
	}();

	if (!this->renderer2D->init(this->window))
	{
		DebugCrash("Couldn't init 2D renderer.");
	}

	// Initialize 3D renderer.
	this->renderer3D = [systemType3D]() -> std::unique_ptr<RendererSystem3D>
	{
		if (systemType3D == RendererSystemType3D::SoftwareClassic)
		{
			return std::make_unique<SoftwareRenderer>();
		}
		else
		{
			DebugLogError("Unrecognized 3D renderer system type \"" +
				std::to_string(static_cast<int>(systemType3D)) + "\".");
			return nullptr;
		}
	}();

	// Make sure render dimensions are at least 1x1.
	const Int2 viewDims = this->getViewDimensions();
	const double resolutionScale = resolutionScaleFunc();
	const int renderWidth = Renderer::makeRendererDimension(viewDims.x, resolutionScale);
	const int renderHeight = Renderer::makeRendererDimension(viewDims.y, resolutionScale);

	// Initialize game world destination frame buffer.
	this->gameWorldTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_STREAMING, renderWidth, renderHeight);
	DebugAssertMsg(this->gameWorldTexture.get() != nullptr,
		"Couldn't create game world texture (" + std::string(SDL_GetError()) + ").");

	RenderInitSettings initSettings;
	initSettings.init(renderWidth, renderHeight, renderThreadsMode);
	this->renderer3D->init(initSettings);
	this->fullGameWindow = fullGameWindow;

	return true;
}

void Renderer::resize(int width, int height, double resolutionScale, bool fullGameWindow)
{
	// The window's dimensions are resized automatically by SDL. The renderer's are not.
	const Int2 windowDims = this->getWindowDimensions();
	DebugAssertMsg(windowDims.x == width, "Mismatched resize widths.");
	DebugAssertMsg(windowDims.y == height, "Mismatched resize heights.");

	SDL_RenderSetLogicalSize(this->renderer, width, height);

	// Reinitialize native frame buffer.
	this->nativeTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_TARGET, width, height);
	DebugAssertMsg(this->nativeTexture.get() != nullptr,
		"Couldn't recreate native frame buffer (" + std::string(SDL_GetError()) + ").");

	this->fullGameWindow = fullGameWindow;

	// Rebuild the 3D renderer if initialized.
	if (this->renderer3D->isInited())
	{
		const Int2 viewDims = this->getViewDimensions();
		const int renderWidth = Renderer::makeRendererDimension(viewDims.x, resolutionScale);
		const int renderHeight = Renderer::makeRendererDimension(viewDims.y, resolutionScale);

		// Reinitialize the game world frame buffer.
		this->gameWorldTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
			SDL_TEXTUREACCESS_STREAMING, renderWidth, renderHeight);
		DebugAssertMsg(this->gameWorldTexture.get() != nullptr,
			"Couldn't recreate game world texture (" + std::string(SDL_GetError()) + ").");

		this->renderer3D->resize(renderWidth, renderHeight);
	}
}

void Renderer::setLetterboxMode(int letterboxMode)
{
	this->letterboxMode = letterboxMode;
}

void Renderer::setWindowMode(WindowMode mode)
{
	int result = 0;
	if (mode == WindowMode::ExclusiveFullscreen)
	{
		SDL_DisplayMode displayMode; // @todo: may consider changing this to some GetDisplayModeForWindowMode()
		result = SDL_GetDesktopDisplayMode(0, &displayMode);
		if (result != 0)
		{
			DebugLogError("Couldn't get desktop display mode for exclusive fullscreen (" + std::string(SDL_GetError()) + ").");
			return;
		}

		result = SDL_SetWindowDisplayMode(this->window, &displayMode);
		if (result != 0)
		{
			DebugLogError("Couldn't set window display mode to \"" + std::to_string(displayMode.w) + "x" +
				std::to_string(displayMode.h) + " " + std::to_string(displayMode.refresh_rate) +
				" Hz\" for exclusive fullscreen (" + std::string(SDL_GetError()) + ").");
			return;
		}
	}

	const uint32_t flags = GetSdlWindowFlags(mode);
	result = SDL_SetWindowFullscreen(this->window, flags);
	if (result != 0)
	{
		DebugLogError("Couldn't set window fullscreen flags to 0x" + String::toHexString(flags) +
			" (" + std::string(SDL_GetError()) + ").");
		return;
	}

	const Int2 windowDims = this->getWindowDimensions();
	const double resolutionScale = this->resolutionScaleFunc();
	this->resize(windowDims.x, windowDims.y, resolutionScale, this->fullGameWindow);

	// Reset the cursor to the center of the screen for consistency.
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
	if (rect != nullptr)
	{
		// @temp: assume in classic space
		Rect nativeRect = this->originalToNative(Rect(rect->x, rect->y, rect->w, rect->h));
		SDL_RenderSetClipRect(this->renderer, &nativeRect.getRect());
	}
	else
	{
		SDL_RenderSetClipRect(this->renderer, nullptr);
	}
}

void Renderer::setRenderThreadsMode(int mode)
{
	DebugAssert(this->renderer3D->isInited());
	DebugNotImplementedMsg("setRenderThreadsMode");
	//this->renderer3D->setRenderThreadsMode(mode); // @todo: figure out if this should be in RenderFrameSettings and obtained via Options
}

bool Renderer::tryCreateObjectTexture(int width, int height, bool isPalette, ObjectTextureID *outID)
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->tryCreateObjectTexture(width, height, isPalette, outID);
}

bool Renderer::tryCreateObjectTexture(const TextureBuilder &textureBuilder, ObjectTextureID *outID)
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->tryCreateObjectTexture(textureBuilder, outID);
}

bool Renderer::tryCreateUiTexture(int width, int height, UiTextureID *outID)
{
	return this->renderer2D->tryCreateUiTexture(width, height, outID);
}

bool Renderer::tryCreateUiTexture(const BufferView2D<const uint32_t> &texels, UiTextureID *outID)
{
	return this->renderer2D->tryCreateUiTexture(texels, outID);
}

bool Renderer::tryCreateUiTexture(const BufferView2D<const uint8_t> &texels, const Palette &palette, UiTextureID *outID)
{
	return this->renderer2D->tryCreateUiTexture(texels, palette, outID);
}

bool Renderer::tryCreateUiTexture(TextureBuilderID textureBuilderID, PaletteID paletteID,
	const TextureManager &textureManager, UiTextureID *outID)
{
	return this->renderer2D->tryCreateUiTexture(textureBuilderID, paletteID, textureManager, outID);
}

std::optional<Int2> Renderer::tryGetObjectTextureDims(ObjectTextureID id) const
{
	return this->renderer3D->tryGetObjectTextureDims(id);
}

std::optional<Int2> Renderer::tryGetUiTextureDims(UiTextureID id) const
{
	return this->renderer2D->tryGetTextureDims(id);
}

LockedTexture Renderer::lockObjectTexture(ObjectTextureID id)
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->lockObjectTexture(id);
}

uint32_t *Renderer::lockUiTexture(UiTextureID textureID)
{
	return this->renderer2D->lockUiTexture(textureID);
}

void Renderer::unlockObjectTexture(ObjectTextureID id)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->unlockObjectTexture(id);
}

void Renderer::unlockUiTexture(UiTextureID textureID)
{
	this->renderer2D->unlockUiTexture(textureID);
}

void Renderer::freeObjectTexture(ObjectTextureID id)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->freeObjectTexture(id);
}

void Renderer::freeUiTexture(UiTextureID id)
{
	this->renderer2D->freeUiTexture(id);
}

void Renderer::loadScene(const RenderCamera &camera, const LevelInstance &levelInst, const SkyInstance &skyInst,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, double daytimePercent, double latitude,
	double chasmAnimPercent, bool nightLightsAreActive, bool playerHasLight,
	const EntityDefinitionLibrary &entityDefLibrary, TextureManager &textureManager)
{
	DebugAssert(this->renderer3D != nullptr);
	this->sceneGraph.loadScene(levelInst, skyInst, activeLevelIndex, mapDefinition, citizenGenInfo, camera,
		chasmAnimPercent, nightLightsAreActive, playerHasLight, daytimePercent, latitude, entityDefLibrary,
		textureManager, *this, *this->renderer3D);
}

/*void Renderer::updateScene(const RenderCamera &camera, const LevelInstance &levelInst, const SkyInstance &skyInst,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, double daytimePercent, double latitude,
	double chasmAnimPercent, bool nightLightsAreActive, bool playerHasLight,
	const EntityDefinitionLibrary &entityDefLibrary, TextureManager &textureManager)
{
	DebugAssert(this->renderer3D != nullptr);
	const double ceilingScale = levelInst.getCeilingScale();
	this->sceneGraph.updateScene(levelInst, skyInst, activeLevelIndex, mapDefinition, citizenGenInfo, camera,
		ceilingScale, chasmAnimPercent, nightLightsAreActive, playerHasLight, daytimePercent, latitude,
		entityDefLibrary, textureManager, *this->renderer3D);
}*/

void Renderer::unloadScene()
{
	DebugAssert(this->renderer3D != nullptr);
	this->sceneGraph.unloadScene(*this->renderer3D);
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

void Renderer::submitFrame(const RenderCamera &camera, double ambientPercent, ObjectTextureID paletteTextureID,
	ObjectTextureID lightTableTextureID, ObjectTextureID skyColorsTextureID, ObjectTextureID thunderstormColorsTextureID,
	int renderThreadsMode)
{
	DebugAssert(this->renderer3D->isInited());

	const Int2 renderDims(this->gameWorldTexture.getWidth(), this->gameWorldTexture.getHeight());

	RenderFrameSettings renderFrameSettings;
	renderFrameSettings.init(ambientPercent, paletteTextureID, lightTableTextureID, skyColorsTextureID,
		thunderstormColorsTextureID, renderDims.x, renderDims.y, renderThreadsMode);

	uint32_t *outputBuffer;
	int gameWorldPitch;
	int status = SDL_LockTexture(this->gameWorldTexture.get(), nullptr,
		reinterpret_cast<void**>(&outputBuffer), &gameWorldPitch);
	DebugAssertMsg(status == 0, "Couldn't lock game world texture for scene rendering (" + std::string(SDL_GetError()) + ").");

	const BufferView<const RenderDrawCall> drawCalls = this->sceneGraph.getDrawCalls();

	// Render the game world (no UI).
	const auto startTime = std::chrono::high_resolution_clock::now();
	this->renderer3D->submitFrame(camera, drawCalls, renderFrameSettings, outputBuffer);
	const auto endTime = std::chrono::high_resolution_clock::now();
	const double frameTime = static_cast<double>((endTime - startTime).count()) / static_cast<double>(std::nano::den);

	// Update profiler stats.
	const RendererSystem3D::ProfilerData swProfilerData = this->renderer3D->getProfilerData();
	this->profilerData.init(swProfilerData.width, swProfilerData.height, swProfilerData.threadCount,
		swProfilerData.potentiallyVisTriangleCount, swProfilerData.visTriangleCount, swProfilerData.visLightCount,
		frameTime);

	// Update the game world texture with the new pixels and copy to the native frame buffer (stretching if needed).
	SDL_UnlockTexture(this->gameWorldTexture.get());

	const Int2 viewDims = this->getViewDimensions();
	this->draw(this->gameWorldTexture, 0, 0, viewDims.x, viewDims.y);
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

void Renderer::draw(const RendererSystem2D::RenderElement *renderElements, int count, RenderSpace renderSpace)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	const SDL_Rect letterboxRect = this->getLetterboxDimensions();
	this->renderer2D->draw(renderElements, count, renderSpace,
		Rect(letterboxRect.x, letterboxRect.y, letterboxRect.w, letterboxRect.h));
}

void Renderer::present()
{
	this->renderer3D->present(); // @todo: maybe this call will do the below at some point? Not sure

	SDL_SetRenderTarget(this->renderer, nullptr);
	SDL_RenderCopy(this->renderer, this->nativeTexture.get(), nullptr, nullptr);
	SDL_RenderPresent(this->renderer);
}
