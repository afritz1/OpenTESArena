#include "SDL.h"

#include "ArenaRenderUtils.h"
#include "Window.h"
#include "../Math/Constants.h"
#include "../Math/Rect.h"
#include "../UI/Surface.h"
#include "../Utilities/Platform.h"

#include "components/debug/Debug.h"

namespace
{
	constexpr bool UseVulkan = true; // @todo pass in a renderer type enum

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

	uint32_t GetSdlWindowFlags(RenderWindowMode windowMode, bool useVulkan)
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

		if (useVulkan)
		{
			flags |= SDL_WINDOW_VULKAN;
		}

		return flags;
	}

	const char *GetSdlWindowTitle()
	{
		return "OpenTESArena";
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
				DebugLogErrorFormat("Couldn't get desktop %d display mode, using given window dimensions \"%dx%d\" (%s).", displayIndex, fallbackWidth, fallbackHeight, SDL_GetError());
			}
		}

		return Int2(fallbackWidth, fallbackHeight);
	}
}

Window::Window()
{
	this->window = nullptr;
	this->letterboxMode = 0;
	this->fullGameWindow = false;
}

Window::~Window()
{
	SDL_DestroyWindow(this->window);
	SDL_Quit();
}

bool Window::init(int width, int height, RenderWindowMode windowMode, int letterboxMode, bool fullGameWindow)
{
	DebugLog("Initializing.");

	const int result = SDL_Init(SDL_INIT_VIDEO); // Required for SDL_GetDesktopDisplayMode() to work for exclusive fullscreen.
	if (result != 0)
	{
		DebugLogErrorFormat("Couldn't init SDL video subsystem (result: %d, %s).", result, SDL_GetError());
		return false;
	}

	if ((width <= 0) || (height <= 0))
	{
		DebugLogErrorFormat("Invalid window dimensions %dx%d.", width, height);
		return false;
	}

	const char *windowTitle = GetSdlWindowTitle();
	const int windowPosition = GetSdlWindowPosition(windowMode);
	const uint32_t windowFlags = GetSdlWindowFlags(windowMode, UseVulkan);
	const Int2 windowDims = GetWindowDimsForMode(windowMode, width, height);
	this->window = SDL_CreateWindow(windowTitle, windowPosition, windowPosition, windowDims.x, windowDims.y, windowFlags);
	if (this->window == nullptr)
	{
		DebugLogErrorFormat("Couldn't create SDL_Window (dimensions: %dx%d, window mode: %d, %s).", width, height, windowMode, SDL_GetError());
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

	this->letterboxMode = letterboxMode;
	this->fullGameWindow = fullGameWindow;

	return true;
}

Int2 Window::getDimensions() const
{
	int windowWidth, windowHeight;
	SDL_GetWindowSize(this->window, &windowWidth, &windowHeight);
	return Int2(windowWidth, windowHeight);
}

double Window::getAspectRatio() const
{
	const Int2 dims = this->getDimensions();
	return static_cast<double>(dims.x) / static_cast<double>(dims.y);
}

double Window::getLetterboxAspectRatio() const
{
	if (this->letterboxMode == 0)
	{
		return 16.0 / 10.0;
	}
	else if (this->letterboxMode == 1)
	{
		return 4.0 / 3.0;
	}
	else if (this->letterboxMode == 2)
	{
		// Stretch to fill.
		const Int2 windowDims = this->getDimensions();
		return static_cast<double>(windowDims.x) / static_cast<double>(windowDims.y);
	}
	else
	{
		DebugUnhandledReturnMsg(double, std::to_string(this->letterboxMode));
	}
}

double Window::getDpiScale() const
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
		DebugLogWarningFormat("Couldn't get DPI of display %d (%s).", displayIndex, SDL_GetError());
		return 1.0;
	}
}

Int2 Window::getViewDimensions() const
{
	constexpr int classicViewHeight = ArenaRenderUtils::SCREEN_HEIGHT - 53;
	constexpr double classicViewHeightRatio = static_cast<double>(classicViewHeight) / ArenaRenderUtils::SCREEN_HEIGHT_REAL;

	const Int2 windowDims = this->getDimensions();
	const int viewHeight = this->fullGameWindow ? windowDims.y : static_cast<int>(std::ceil(windowDims.y * classicViewHeightRatio));

	return Int2(windowDims.x, viewHeight);
}

double Window::getViewAspectRatio() const
{
	const Int2 viewDims = this->getViewDimensions();
	return static_cast<double>(viewDims.x) / static_cast<double>(viewDims.y);
}

SDL_Rect Window::getLetterboxDimensions() const
{
	const Int2 windowDims = this->getDimensions();
	const double nativeAspect = static_cast<double>(windowDims.x) / static_cast<double>(windowDims.y);
	const double letterboxAspect = this->getLetterboxAspectRatio();

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

Int2 Window::nativeToOriginal(const Int2 &nativePoint) const
{
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

Rect Window::nativeToOriginal(const Rect &nativeRect) const
{
	const Int2 newTopLeft = this->nativeToOriginal(nativeRect.getTopLeft());
	const Int2 newBottomRight = this->nativeToOriginal(nativeRect.getBottomRight());
	return Rect(
		newTopLeft.x,
		newTopLeft.y,
		newBottomRight.x - newTopLeft.x,
		newBottomRight.y - newTopLeft.y);
}

Int2 Window::originalToNative(const Int2 &originalPoint) const
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

Rect Window::originalToNative(const Rect &originalRect) const
{
	const Int2 newTopLeft = this->originalToNative(originalRect.getTopLeft());
	const Int2 newBottomRight = this->originalToNative(originalRect.getBottomRight());
	return Rect(
		newTopLeft.x,
		newTopLeft.y,
		newBottomRight.x - newTopLeft.x,
		newBottomRight.y - newTopLeft.y);
}

bool Window::letterboxContains(const Int2 &nativePoint) const
{
	const SDL_Rect letterbox = this->getLetterboxDimensions();
	const Rect rectangle(letterbox.x, letterbox.y, letterbox.w, letterbox.h);
	return rectangle.contains(nativePoint);
}

void Window::setMode(RenderWindowMode mode)
{
	int result = 0;
	if (mode == RenderWindowMode::ExclusiveFullscreen)
	{
		SDL_DisplayMode displayMode; // @todo: may consider changing this to some GetDisplayModeForWindowMode()
		result = SDL_GetDesktopDisplayMode(0, &displayMode);
		if (result != 0)
		{
			DebugLogErrorFormat("Couldn't get desktop display mode for exclusive fullscreen (%s).", SDL_GetError());
			return;
		}

		result = SDL_SetWindowDisplayMode(this->window, &displayMode);
		if (result != 0)
		{
			DebugLogErrorFormat("Couldn't set window display mode to %dx%d %dHz for exclusive fullscreen (%s).", displayMode.w, displayMode.h, displayMode.refresh_rate, SDL_GetError());
			return;
		}
	}

	const uint32_t flags = GetSdlWindowFlags(mode, UseVulkan);
	result = SDL_SetWindowFullscreen(this->window, flags);
	if (result != 0)
	{
		DebugLogErrorFormat("Couldn't set window fullscreen flags to 0x%X (%s).", flags, SDL_GetError());
		return;
	}
}

void Window::setIcon(const Surface &icon)
{
	SDL_SetWindowIcon(this->window, icon.get());
}

void Window::setTitle(const char *title)
{
	SDL_SetWindowTitle(this->window, title);
}

void Window::warpMouse(int x, int y)
{
	SDL_WarpMouseInWindow(this->window, x, y);
}
