#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

#include "ArenaRenderUtils.h"
#include "RenderCamera.h"
#include "Renderer.h"
#include "RendererUtils.h"
#include "RenderFrameSettings.h"
#include "RenderInitSettings.h"
#include "SdlUiRenderer.h"
#include "SoftwareRenderer.h"
#include "../Assets/TextureManager.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Rect.h"
#include "../UI/CursorAlignment.h"
#include "../UI/RenderSpace.h"
#include "../UI/Surface.h"
#include "../Utilities/Color.h"
#include "../Utilities/Platform.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	RenderCamera g_physicsDebugCamera; // Cached every frame for Jolt Physics debug renderer.
	constexpr double PHYSICS_DEBUG_MAX_DISTANCE = 3.0;
	constexpr double PHYSICS_DEBUG_MAX_DISTANCE_SQR = PHYSICS_DEBUG_MAX_DISTANCE * PHYSICS_DEBUG_MAX_DISTANCE;

	int GetSdlWindowPosition(RenderWindowMode windowMode)
	{
		switch (windowMode)
		{
		case RenderWindowMode::Window:
			return SDL_WINDOWPOS_CENTERED;
		case RenderWindowMode::BorderlessFullscreen:
		case RenderWindowMode::ExclusiveFullscreen:
			return SDL_WINDOWPOS_UNDEFINED;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(windowMode)));
		}
	}

	uint32_t GetSdlWindowFlags(RenderWindowMode windowMode)
	{
		uint32_t flags = SDL_WINDOW_ALLOW_HIGHDPI;
		if (windowMode == RenderWindowMode::Window)
		{
			flags |= SDL_WINDOW_RESIZABLE;
		}
		else if (windowMode == RenderWindowMode::BorderlessFullscreen)
		{
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		}
		else if (windowMode == RenderWindowMode::ExclusiveFullscreen)
		{
			flags |= SDL_WINDOW_FULLSCREEN;
		}

		return flags;
	}

	const char *GetSdlWindowTitle()
	{
		return "OpenTESArena";
	}

	const char *GetSdlRenderScaleQuality()
	{
		return "nearest";
	}

	Int2 GetWindowDimsForMode(RenderWindowMode windowMode, int fallbackWidth, int fallbackHeight)
	{
		if (windowMode == RenderWindowMode::ExclusiveFullscreen)
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

	// Helper method for making a renderer context.
	SDL_Renderer *CreateSdlRendererForWindow(SDL_Window *window)
	{
		// Automatically choose the best driver.
		constexpr int bestDriver = -1;

		SDL_Renderer *rendererContext = SDL_CreateRenderer(window, bestDriver, SDL_RENDERER_ACCELERATED);
		if (rendererContext == nullptr)
		{
			DebugLogError("Couldn't create SDL_Renderer with driver \"" + std::to_string(bestDriver) + "\" (" + std::string(SDL_GetError()) + ").");
			return nullptr;
		}

		SDL_RendererInfo rendererInfo;
		if (SDL_GetRendererInfo(rendererContext, &rendererInfo) < 0)
		{
			DebugLogError("Couldn't get SDL_RendererInfo (" + std::string(SDL_GetError()) + ").");
			return nullptr;
		}

		const std::string rendererInfoFlags = String::toHexString(rendererInfo.flags);
		DebugLog("Created renderer \"" + std::string(rendererInfo.name) + "\" (flags: 0x" + rendererInfoFlags + ").");

		// Set pixel interpolation hint.
		const SDL_bool status = SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, GetSdlRenderScaleQuality());
		if (status != SDL_TRUE)
		{
			DebugLogWarning("Couldn't set SDL rendering interpolation hint (" + std::string(SDL_GetError()) + ").");
		}

		int windowWidth, windowHeight;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);

		auto isValidWindowSize = [](int width, int height)
		{
			return (width > 0) && (height > 0);
		};

		// Set the size of the render texture to be the size of the whole screen (it automatically scales otherwise).
		// If this fails, the OS might not support hardware accelerated renderers for some reason (such as with Linux),
		// so retry with software.
		if (!isValidWindowSize(windowWidth, windowHeight))
		{
			DebugLogWarning("Failed to init accelerated SDL_Renderer, trying software fallback (" + std::string(SDL_GetError()) + ").");
			SDL_DestroyRenderer(rendererContext);

			rendererContext = SDL_CreateRenderer(window, bestDriver, SDL_RENDERER_SOFTWARE);
			if (rendererContext == nullptr)
			{
				DebugLogError("Couldn't create software fallback SDL_Renderer (" + std::string(SDL_GetError()) + ").");
				return nullptr;
			}

			SDL_GetWindowSize(window, &windowWidth, &windowHeight);
			if (!isValidWindowSize(windowWidth, windowHeight))
			{
				DebugLogError("Couldn't get software fallback SDL_Window dimensions (" + std::string(SDL_GetError()) + ").");
				return nullptr;
			}
		}

		// Set the device-independent resolution for rendering (i.e., the "behind-the-scenes" resolution).
		SDL_RenderSetLogicalSize(rendererContext, windowWidth, windowHeight);

		return rendererContext;
	}

	Int2 MakeInternalRendererDimensions(const Int2 &dimensions, double resolutionScale)
	{
		// Make sure dimensions are at least 1x1, and round to avoid off-by-one resolutions like 1079p.
		const int renderWidth = std::max(static_cast<int>(std::round(static_cast<double>(dimensions.x) * resolutionScale)), 1);
		const int renderHeight = std::max(static_cast<int>(std::round(static_cast<double>(dimensions.y) * resolutionScale)), 1);
		return Int2(renderWidth, renderHeight);
	}
}

RenderDisplayMode::RenderDisplayMode(int width, int height, int refreshRate)
{
	this->width = width;
	this->height = height;
	this->refreshRate = refreshRate;
}

RendererProfilerData::RendererProfilerData()
{
	this->width = -1;
	this->height = -1;
	this->pixelCount = -1;
	this->threadCount = -1;
	this->drawCallCount = -1;
	this->presentedTriangleCount = -1;
	this->objectTextureCount = -1;
	this->objectTextureByteCount = -1;
	this->totalLightCount = -1;
	this->totalCoverageTests = -1;
	this->totalDepthTests = -1;
	this->totalColorWrites = -1;
	this->renderTime = 0.0;
	this->presentTime = 0.0;
}

void RendererProfilerData::init(int width, int height, int threadCount, int drawCallCount, int presentedTriangleCount,
	int objectTextureCount, int64_t objectTextureByteCount, int totalLightCount, int totalCoverageTests, int totalDepthTests,
	int totalColorWrites, double renderTime, double presentTime)
{
	this->width = width;
	this->height = height;
	this->pixelCount = width * height;
	this->threadCount = threadCount;
	this->drawCallCount = drawCallCount;
	this->presentedTriangleCount = presentedTriangleCount;
	this->objectTextureCount = objectTextureCount;
	this->objectTextureByteCount = objectTextureByteCount;
	this->totalLightCount = totalLightCount;
	this->totalCoverageTests = totalCoverageTests;
	this->totalDepthTests = totalDepthTests;
	this->totalColorWrites = totalColorWrites;
	this->renderTime = renderTime;
	this->presentTime = presentTime;
}

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
	int windowWidth, windowHeight;
	SDL_GetWindowSize(this->window, &windowWidth, &windowHeight);
	return Int2(windowWidth, windowHeight);
}

double Renderer::getWindowAspect() const
{
	const Int2 dims = this->getWindowDimensions();
	return static_cast<double>(dims.x) / static_cast<double>(dims.y);
}

Span<const RenderDisplayMode> Renderer::getDisplayModes() const
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
		DebugLogWarning("Couldn't get DPI of display \"" + std::to_string(displayIndex) + "\" (" + std::string(SDL_GetError()) + ").");
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

double Renderer::getViewAspect() const
{
	const Int2 viewDims = this->getViewDimensions();
	return static_cast<double>(viewDims.x) / static_cast<double>(viewDims.y);
}

SDL_Rect Renderer::getLetterboxDimensions() const
{
	const Int2 windowDims = this->getWindowDimensions();
	const double nativeAspect = static_cast<double>(windowDims.x) / static_cast<double>(windowDims.y);
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
		const int subWidth = static_cast<int>(std::ceil(static_cast<double>(windowDims.y) * letterboxAspect));
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
		const int subHeight = static_cast<int>(std::ceil(static_cast<double>(windowDims.x) / letterboxAspect));
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
		DebugCrash("Couldn't take screenshot (" + std::string(SDL_GetError()) + ").");
	}

	return screenshot;
}

const RendererProfilerData &Renderer::getProfilerData() const
{
	return this->profilerData;
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
	const double letterboxXPercent = static_cast<double>(letterboxPoint.x) / static_cast<double>(letterbox.w);
	const double letterboxYPercent = static_cast<double>(letterboxPoint.y) / static_cast<double>(letterbox.h);

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
	const double originalXPercent = static_cast<double>(originalPoint.x) / ArenaRenderUtils::SCREEN_WIDTH_REAL;
	const double originalYPercent = static_cast<double>(originalPoint.y) / ArenaRenderUtils::SCREEN_HEIGHT_REAL;

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
		DebugLogError("Couldn't create SDL_Texture (" + std::string(SDL_GetError()) + ").");
	}

	Texture texture;
	texture.init(tex);
	return texture;
}

bool Renderer::init(int width, int height, RenderWindowMode windowMode, int letterboxMode, bool fullGameWindow,
	const RenderResolutionScaleFunc &resolutionScaleFunc, RendererSystemType2D systemType2D, RendererSystemType3D systemType3D,
	int renderThreadsMode, DitheringMode ditheringMode)
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
	this->fullGameWindow = fullGameWindow;
	this->resolutionScaleFunc = resolutionScaleFunc;

	// Initialize SDL window.
	const char *windowTitle = GetSdlWindowTitle();
	const int windowPosition = GetSdlWindowPosition(windowMode);
	const uint32_t windowFlags = GetSdlWindowFlags(windowMode);
	const Int2 windowDims = GetWindowDimsForMode(windowMode, width, height);
	this->window = SDL_CreateWindow(windowTitle, windowPosition, windowPosition, windowDims.x, windowDims.y, windowFlags);
	if (this->window == nullptr)
	{
		DebugLogError("Couldn't create SDL_Window (dimensions: " + std::to_string(width) + "x" + std::to_string(height) +
			", window mode: " + std::to_string(static_cast<int>(windowMode)) + ", " + std::string(SDL_GetError()) + ").");
		return false;
	}

	// Initialize SDL renderer context.
	this->renderer = CreateSdlRendererForWindow(this->window);
	if (this->renderer == nullptr)
	{
		DebugLogError("Couldn't create SDL_Renderer (" + std::string(SDL_GetError()) + ").");
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
				this->displayModes.emplace_back(RenderDisplayMode(mode.w, mode.h, mode.refresh_rate));
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
		DebugLogError("Couldn't create SDL_Texture frame buffer (" + std::string(SDL_GetError()) + ").");
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
			DebugLogError("Unrecognized 2D renderer system type \"" + std::to_string(static_cast<int>(systemType2D)) + "\".");
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
			DebugLogError("Unrecognized 3D renderer system type \"" + std::to_string(static_cast<int>(systemType3D)) + "\".");
			return nullptr;
		}
	}();

	const Int2 viewDims = this->getViewDimensions();
	const double resolutionScale = resolutionScaleFunc();
	const Int2 renderDims = MakeInternalRendererDimensions(viewDims, resolutionScale);

	// Initialize game world destination frame buffer.
	this->gameWorldTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, renderDims.x, renderDims.y);
	if (this->gameWorldTexture.get() == nullptr)
	{
		DebugLogError("Couldn't create game world texture at " + std::to_string(renderDims.x) + "x" +
			std::to_string(renderDims.y) + " (" + std::string(SDL_GetError()) + ").");
	}

	RenderInitSettings initSettings;
	initSettings.init(renderDims.x, renderDims.y, renderThreadsMode, ditheringMode);
	this->renderer3D->init(initSettings);

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
	this->nativeTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_TARGET, width, height);
	if (this->nativeTexture.get() == nullptr)
	{
		DebugLogError("Couldn't recreate native frame buffer for resize to " + std::to_string(width) + "x" +
			std::to_string(height) + " (" + std::string(SDL_GetError()) + ").");
	}

	this->fullGameWindow = fullGameWindow;

	// Rebuild the 3D renderer if initialized.
	if (this->renderer3D->isInited())
	{
		const Int2 viewDims = this->getViewDimensions();
		const Int2 renderDims = MakeInternalRendererDimensions(viewDims, resolutionScale);

		// Reinitialize the game world frame buffer.
		this->gameWorldTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, renderDims.x, renderDims.y);
		if (this->gameWorldTexture.get() == nullptr)
		{
			DebugLogError("Couldn't recreate game world texture for resize to " + std::to_string(renderDims.x) + "x" +
				std::to_string(renderDims.y) + " (" + std::string(SDL_GetError()) + ").");
		}

		this->renderer3D->resize(renderDims.x, renderDims.y);
	}
}

void Renderer::handleRenderTargetsReset()
{
	if (this->window == nullptr)
	{
		DebugLogError("Missing SDL_Window for render targets reset.");
		return;
	}

	if (this->renderer == nullptr)
	{
		DebugLogError("Missing SDL_Renderer for render targets reset.");
		return;
	}

	const Int2 windowDims = this->getWindowDimensions();
	this->nativeTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_TARGET, windowDims.x, windowDims.y);
	if (this->nativeTexture.get() == nullptr)
	{
		DebugLogError("Couldn't recreate native frame buffer for render targets reset to " + std::to_string(windowDims.x) + "x" +
			std::to_string(windowDims.y) + " (" + std::string(SDL_GetError()) + ").");
	}

	if (this->renderer3D->isInited())
	{
		const Int2 viewDims = this->getViewDimensions();
		const double resolutionScale = this->resolutionScaleFunc();
		const Int2 renderDims = MakeInternalRendererDimensions(viewDims, resolutionScale);

		this->gameWorldTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, renderDims.x, renderDims.y);
		if (this->gameWorldTexture.get() == nullptr)
		{
			DebugLogError("Couldn't recreate game world texture for render targets reset to " + std::to_string(renderDims.x) + "x" +
				std::to_string(renderDims.y) + " (" + std::string(SDL_GetError()) + ").");
		}

		this->renderer3D->resize(renderDims.x, renderDims.y);
	}
}

void Renderer::setLetterboxMode(int letterboxMode)
{
	this->letterboxMode = letterboxMode;
}

void Renderer::setWindowMode(RenderWindowMode mode)
{
	int result = 0;
	if (mode == RenderWindowMode::ExclusiveFullscreen)
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
		const Rect nativeRect = this->originalToNative(Rect(rect->x, rect->y, rect->w, rect->h));
		const SDL_Rect nativeRectSdl = nativeRect.getSdlRect();
		SDL_RenderSetClipRect(this->renderer, &nativeRectSdl);
	}
	else
	{
		SDL_RenderSetClipRect(this->renderer, nullptr);
	}
}

VertexPositionBufferID Renderer::createVertexPositionBuffer(int vertexCount, int componentsPerVertex)
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->createVertexPositionBuffer(vertexCount, componentsPerVertex);
}

VertexAttributeBufferID Renderer::createVertexAttributeBuffer(int vertexCount, int componentsPerVertex)
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->createVertexAttributeBuffer(vertexCount, componentsPerVertex);
}

IndexBufferID Renderer::createIndexBuffer(int indexCount)
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->createIndexBuffer(indexCount);
}

void Renderer::populateVertexPositionBuffer(VertexPositionBufferID id, Span<const double> positions)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->populateVertexPositionBuffer(id, positions);
}

void Renderer::populateVertexAttributeBuffer(VertexAttributeBufferID id, Span<const double> attributes)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->populateVertexAttributeBuffer(id, attributes);
}

void Renderer::populateIndexBuffer(IndexBufferID id, Span<const int32_t> indices)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->populateIndexBuffer(id, indices);
}

void Renderer::freeVertexPositionBuffer(VertexPositionBufferID id)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->freeVertexPositionBuffer(id);
}

void Renderer::freeVertexAttributeBuffer(VertexAttributeBufferID id)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->freeVertexAttributeBuffer(id);
}

void Renderer::freeIndexBuffer(IndexBufferID id)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->freeIndexBuffer(id);
}

ObjectTextureID Renderer::createObjectTexture(int width, int height, int bytesPerTexel)
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->createObjectTexture(width, height, bytesPerTexel);
}

ObjectTextureID Renderer::createObjectTexture(const TextureBuilder &textureBuilder)
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->createObjectTexture(textureBuilder);
}

bool Renderer::tryCreateUiTexture(int width, int height, UiTextureID *outID)
{
	return this->renderer2D->tryCreateUiTexture(width, height, outID);
}

bool Renderer::tryCreateUiTexture(Span2D<const uint32_t> texels, UiTextureID *outID)
{
	return this->renderer2D->tryCreateUiTexture(texels, outID);
}

bool Renderer::tryCreateUiTexture(Span2D<const uint8_t> texels, const Palette &palette, UiTextureID *outID)
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
	DebugAssert(this->renderer3D->isInited());
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

UniformBufferID Renderer::createUniformBuffer(int elementCount, size_t sizeOfElement, size_t alignmentOfElement)
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->createUniformBuffer(elementCount, sizeOfElement, alignmentOfElement);
}

void Renderer::populateUniformBuffer(UniformBufferID id, Span<const std::byte> data)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->populateUniformBuffer(id, data);
}

void Renderer::populateUniformAtIndex(UniformBufferID id, int uniformIndex, Span<const std::byte> uniformData)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->populateUniformAtIndex(id, uniformIndex, uniformData);
}

void Renderer::freeUniformBuffer(UniformBufferID id)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->freeUniformBuffer(id);
}

RenderLightID Renderer::createLight()
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->createLight();
}

void Renderer::setLightPosition(RenderLightID id, const Double3 &worldPoint)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->setLightPosition(id, worldPoint);
}

void Renderer::setLightRadius(RenderLightID id, double startRadius, double endRadius)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->setLightRadius(id, startRadius, endRadius);
}

void Renderer::freeLight(RenderLightID id)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->freeLight(id);
}

void Renderer::clear(const Color &color)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderClear(this->renderer);
}

void Renderer::clear()
{
	this->clear(Colors::Black);
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
	this->clearOriginal(Colors::Black);
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
	const SDL_Rect rectSdl = rect.getSdlRect();
	SDL_RenderFillRect(this->renderer, &rectSdl);
}

void Renderer::DrawLine(JPH::RVec3Arg src, JPH::RVec3Arg dst, JPH::ColorArg color)
{
	const RenderCamera &camera = g_physicsDebugCamera;
	const Double3 worldPoint0(static_cast<double>(src.GetX()), static_cast<double>(src.GetY()), static_cast<double>(src.GetZ()));
	const Double3 worldPoint1(static_cast<double>(dst.GetX()), static_cast<double>(dst.GetY()), static_cast<double>(dst.GetZ()));
	const double distSqr0 = (camera.worldPoint - worldPoint0).lengthSquared();
	const double distSqr1 = (camera.worldPoint - worldPoint1).lengthSquared();
	if ((distSqr0 > PHYSICS_DEBUG_MAX_DISTANCE_SQR) || (distSqr1 > PHYSICS_DEBUG_MAX_DISTANCE_SQR))
	{
		return;
	}

	const Double4 clipPoint0 = RendererUtils::worldSpaceToClipSpace(Double4(worldPoint0, 1.0), camera.viewProjMatrix);
	const Double4 clipPoint1 = RendererUtils::worldSpaceToClipSpace(Double4(worldPoint1, 1.0), camera.viewProjMatrix);
	if ((clipPoint0.w <= 0.0) || (clipPoint1.w <= 0.0))
	{
		return;
	}

	const Int2 viewDims = this->getViewDimensions();
	const Double3 ndc0 = RendererUtils::clipSpaceToNDC(clipPoint0);
	const Double3 ndc1 = RendererUtils::clipSpaceToNDC(clipPoint1);
	const Double2 screenSpace0 = RendererUtils::ndcToScreenSpace(ndc0, viewDims.x, viewDims.y);
	const Double2 screenSpace1 = RendererUtils::ndcToScreenSpace(ndc1, viewDims.x, viewDims.y);
	const Int2 pixelSpace0(static_cast<int>(screenSpace0.x), static_cast<int>(screenSpace0.y));
	const Int2 pixelSpace1(static_cast<int>(screenSpace1.x), static_cast<int>(screenSpace1.y));

	const double distanceRatio = std::max(distSqr0, distSqr1) / PHYSICS_DEBUG_MAX_DISTANCE_SQR;
	const double intensityPercent = std::clamp(1.0 - (distanceRatio * distanceRatio * distanceRatio), 0.0, 1.0);
	const ColorReal multipliedColor = ColorReal::fromARGB(color.GetUInt32()) * intensityPercent;
	const Color presentedColor = Color::fromARGB(multipliedColor.toARGB());
	this->drawLine(presentedColor, pixelSpace0.x, pixelSpace0.y, pixelSpace1.x, pixelSpace1.y);
}

void Renderer::DrawTriangle(JPH::RVec3Arg v1, JPH::RVec3Arg v2, JPH::RVec3Arg v3, JPH::ColorArg color, ECastShadow castShadow)
{
	const RenderCamera &camera = g_physicsDebugCamera;
	const Double3 worldPoint0(static_cast<double>(v1.GetX()), static_cast<double>(v1.GetY()), static_cast<double>(v1.GetZ()));
	const Double3 worldPoint1(static_cast<double>(v2.GetX()), static_cast<double>(v2.GetY()), static_cast<double>(v2.GetZ()));
	const Double3 worldPoint2(static_cast<double>(v3.GetX()), static_cast<double>(v3.GetY()), static_cast<double>(v3.GetZ()));
	const double distSqr0 = (camera.worldPoint - worldPoint0).lengthSquared();
	const double distSqr1 = (camera.worldPoint - worldPoint1).lengthSquared();
	const double distSqr2 = (camera.worldPoint - worldPoint2).lengthSquared();
	if ((distSqr0 > PHYSICS_DEBUG_MAX_DISTANCE_SQR) || (distSqr1 > PHYSICS_DEBUG_MAX_DISTANCE_SQR) || (distSqr2 > PHYSICS_DEBUG_MAX_DISTANCE_SQR))
	{
		return;
	}

	const Double4 clipPoint0 = RendererUtils::worldSpaceToClipSpace(Double4(worldPoint0, 1.0), camera.viewProjMatrix);
	const Double4 clipPoint1 = RendererUtils::worldSpaceToClipSpace(Double4(worldPoint1, 1.0), camera.viewProjMatrix);
	const Double4 clipPoint2 = RendererUtils::worldSpaceToClipSpace(Double4(worldPoint2, 1.0), camera.viewProjMatrix);
	if ((clipPoint0.w <= 0.0) || (clipPoint1.w <= 0.0) || (clipPoint2.w <= 0.0))
	{
		return;
	}

	const Int2 viewDims = this->getViewDimensions();
	const Double3 ndc0 = RendererUtils::clipSpaceToNDC(clipPoint0);
	const Double3 ndc1 = RendererUtils::clipSpaceToNDC(clipPoint1);
	const Double3 ndc2 = RendererUtils::clipSpaceToNDC(clipPoint2);
	const Double2 screenSpace0 = RendererUtils::ndcToScreenSpace(ndc0, viewDims.x, viewDims.y);
	const Double2 screenSpace1 = RendererUtils::ndcToScreenSpace(ndc1, viewDims.x, viewDims.y);
	const Double2 screenSpace2 = RendererUtils::ndcToScreenSpace(ndc2, viewDims.x, viewDims.y);
	const Double2 screenSpace01 = screenSpace1 - screenSpace0;
	const Double2 screenSpace12 = screenSpace2 - screenSpace1;
	const Double2 screenSpace20 = screenSpace0 - screenSpace2;
	const double screenSpace01Cross12 = screenSpace12.cross(screenSpace01);
	const double screenSpace12Cross20 = screenSpace20.cross(screenSpace12);
	const double screenSpace20Cross01 = screenSpace01.cross(screenSpace20);

	// Discard back-facing.
	const bool isFrontFacing = (screenSpace01Cross12 + screenSpace12Cross20 + screenSpace20Cross01) > 0.0;
	if (!isFrontFacing)
	{
		return;
	}

	this->DrawLine(v1, v2, color);
	this->DrawLine(v2, v3, color);
	this->DrawLine(v3, v1, color);
}

void Renderer::DrawText3D(JPH::RVec3Arg position, const std::string_view &str, JPH::ColorArg color, float height)
{
	// Do nothing.
}

void Renderer::submitFrame(const RenderCamera &camera, const RenderCommandBuffer &commandBuffer, double ambientPercent,
	double screenSpaceAnimPercent, ObjectTextureID paletteTextureID, ObjectTextureID lightTableTextureID, ObjectTextureID skyBgTextureID,
	int renderThreadsMode, DitheringMode ditheringMode)
{
	DebugAssert(this->renderer3D->isInited());

	g_physicsDebugCamera = camera;

	const Int2 renderDims(this->gameWorldTexture.getWidth(), this->gameWorldTexture.getHeight());

	RenderFrameSettings renderFrameSettings;
	renderFrameSettings.init(ambientPercent, screenSpaceAnimPercent, paletteTextureID, lightTableTextureID, skyBgTextureID,
		renderDims.x, renderDims.y, renderThreadsMode, ditheringMode);

	uint32_t *outputBuffer;
	int gameWorldPitch;
	int status = SDL_LockTexture(this->gameWorldTexture.get(), nullptr, reinterpret_cast<void**>(&outputBuffer), &gameWorldPitch);
	if (status != 0)
	{
		DebugLogError("Couldn't lock game world SDL_Texture for scene rendering (" + std::string(SDL_GetError()) + ").");
		return;
	}

	// Render the game world (no UI).
	const auto renderStartTime = std::chrono::high_resolution_clock::now();
	this->renderer3D->submitFrame(camera, renderFrameSettings, commandBuffer, outputBuffer);
	const auto renderEndTime = std::chrono::high_resolution_clock::now();
	const double renderTotalTime = static_cast<double>((renderEndTime - renderStartTime).count()) / static_cast<double>(std::nano::den);

	// Update the game world texture with the new pixels and copy to the native frame buffer (stretching if needed).
	const auto presentStartTime = std::chrono::high_resolution_clock::now();
	SDL_UnlockTexture(this->gameWorldTexture.get());

	const Int2 viewDims = this->getViewDimensions();
	this->draw(this->gameWorldTexture, 0, 0, viewDims.x, viewDims.y);
	const auto presentEndTime = std::chrono::high_resolution_clock::now();
	const double presentTotalTime = static_cast<double>((presentEndTime - presentStartTime).count()) / static_cast<double>(std::nano::den);

	// Update profiler stats.
	const Renderer3DProfilerData swProfilerData = this->renderer3D->getProfilerData();
	this->profilerData.init(swProfilerData.width, swProfilerData.height, swProfilerData.threadCount,
		swProfilerData.drawCallCount, swProfilerData.presentedTriangleCount, swProfilerData.textureCount,
		swProfilerData.textureByteCount, swProfilerData.totalLightCount, swProfilerData.totalCoverageTests,
		swProfilerData.totalDepthTests, swProfilerData.totalColorWrites, renderTotalTime, presentTotalTime);
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
