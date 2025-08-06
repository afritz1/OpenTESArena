#ifndef WINDOW_H
#define WINDOW_H

#include <cstdint>
#include <vector>

#include "../Math/Vector2.h"

#include "components/utilities/Span.h"

class Surface;

struct Rect;
struct SDL_Window;

struct RenderDisplayMode
{
	int width, height, refreshRate;

	RenderDisplayMode(int width, int height, int refreshRate);
};

enum class RenderWindowMode
{
	Window,
	BorderlessFullscreen,
	ExclusiveFullscreen
};

// Thin wrapper over SDL_Window.
struct Window
{
	SDL_Window *window;
	uint32_t additionalFlags;
	std::vector<RenderDisplayMode> displayModes; // Supported fullscreen display modes.
	int letterboxMode; // Determines aspect ratio of the original UI (16:10, 4:3, etc.).
	bool fullGameWindow; // Determines height of 3D frame buffer.

	Window();
	~Window();

	bool init(int width, int height, RenderWindowMode windowMode, uint32_t additionalFlags, int letterboxMode, bool fullGameWindow);

	Int2 getDimensions() const;
	double getAspectRatio() const;

	// Gets the letterbox aspect associated with the current letterbox mode.
	double getLetterboxAspectRatio() const;

	// Gets the active window's pixels-per-inch scale divided by platform DPI.
	double getDpiScale() const;

	// The "view height" is the height in pixels for the visible game world. This depends on whether the whole screen
	// is rendered or just the portion above the interface. The game interface is 53 pixels tall in 320x200.
	Int2 getViewDimensions() const;
	double getViewAspectRatio() const;

	// For the "letterbox" part of the screen, scaled to fit the window using the given letterbox aspect.
	Rect getLetterboxRect() const;

	// Transforms a native window (i.e., 1920x1080) point or rectangle to an original (320x200) point or rectangle.
	// Points outside the letterbox will either be negative or outside the 320x200 limit when returned.
	Int2 nativeToOriginal(const Int2 &nativePoint) const;
	Rect nativeToOriginal(const Rect &nativeRect) const;

	// Does the opposite of nativeToOriginal().
	Int2 originalToNative(const Int2 &originalPoint) const;
	Rect originalToNative(const Rect &originalRect) const;

	// Returns true if the letterbox contains a native point.
	bool letterboxContains(const Int2 &nativePoint) const;

	void setMode(RenderWindowMode mode);
	void setIcon(const Surface &icon);
	void setTitle(const char *title);
	void warpMouse(int x, int y);
};

#endif
