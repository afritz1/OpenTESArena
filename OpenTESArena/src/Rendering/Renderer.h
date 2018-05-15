#ifndef RENDERER_H
#define RENDERER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "SoftwareRenderer.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"

// Acts as a wrapper for SDL_Renderer operations as well as 3D rendering operations.

// The format for all textures is ARGB8888.

class Color;
class Rect;
class Surface;
class VoxelGrid;

enum class CursorAlignment;

struct SDL_Rect;
struct SDL_Renderer;
struct SDL_Surface;
struct SDL_Texture;
struct SDL_Window;

class Renderer
{
private:
	static const char *DEFAULT_RENDER_SCALE_QUALITY;
	static const std::string DEFAULT_TITLE;

	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *nativeTexture, *gameWorldTexture; // Frame buffers.
	SoftwareRenderer softwareRenderer; // Game world renderer.
	int letterboxMode; // Determines aspect ratio of the original UI (16:10, 4:3, etc.).
	bool fullGameWindow; // Determines height of 3D frame buffer.

	// Helper method for making a renderer context.
	SDL_Renderer *createRenderer();

	// For use with window dimensions, etc.. No longer used for rendering.
	SDL_Surface *getWindowSurface() const;
public:
	// Only defined so members are initialized for Game ctor exception handling.
	Renderer();
	~Renderer();

	// Original screen dimensions.
	static const int ORIGINAL_WIDTH;
	static const int ORIGINAL_HEIGHT;

	// The default pixel format for all software surfaces, ARGB8888.
	static const uint32_t DEFAULT_PIXELFORMAT;

	// Default bits per pixel.
	static const int DEFAULT_BPP;

	// Gets the letterbox aspect associated with the current letterbox mode.
	double getLetterboxAspect() const;

	// Gets the width and height of the active window.
	Int2 getWindowDimensions() const;

	// The "view height" is the height in pixels for the visible game world. This 
	// depends on whether the whole screen is rendered or just the portion above 
	// the interface. The game interface is 53 pixels tall in 320x200.
	int getViewHeight() const;

	// This is for the "letterbox" part of the screen, scaled to fit the window 
	// using the given letterbox aspect.
	SDL_Rect getLetterboxDimensions() const;

	// Gets a screenshot of the current window.
	Surface getScreenshot() const;

	// Transforms a native window (i.e., 1920x1080) point or rectangle to an original 
	// (320x200) point or rectangle. Points outside the letterbox will either be negative 
	// or outside the 320x200 limit when returned.
	Int2 nativeToOriginal(const Int2 &nativePoint) const;
	Rect nativeToOriginal(const Rect &nativeRect) const;

	// Does the opposite of nativeToOriginal().
	Int2 originalToNative(const Int2 &originalPoint) const;
	Rect originalToNative(const Rect &originalRect) const;

	// Returns true if the letterbox contains a native point.
	bool letterboxContains(const Int2 &nativePoint) const;

	// Wrapper methods for SDL_CreateTexture.
	SDL_Texture *createTexture(uint32_t format, int access, int w, int h);
	SDL_Texture *createTextureFromSurface(SDL_Surface *surface);

	void init(int width, int height, bool fullscreen, int letterboxMode);

	// Resizes the renderer dimensions.
	void resize(int width, int height, double resolutionScale, bool fullGameWindow);

	// Sets the letterbox mode.
	void setLetterboxMode(int letterboxMode);

	// Sets whether the program is windowed or fullscreen.
	void setFullscreen(bool fullscreen);

	// Sets the window icon to be the given surface.
	void setWindowIcon(SDL_Surface *icon);

	// Sets the window title.
	void setWindowTitle(const std::string &title);

	// Teleports the mouse to a location in the window.
	void warpMouse(int x, int y);

	// Sets the clip rectangle of the renderer so that pixels outside the specified area
	// will not be rendered. If rect is null, then clipping is disabled.
	void setClipRect(const SDL_Rect *rect);

	// Initialize the renderer for the game world. The "fullGameWindow" argument 
	// determines whether to render a "fullscreen" 3D image or just the part above 
	// the game interface. If there is an existing renderer in memory, it will be 
	// overwritten with the new one.
	void initializeWorldRendering(double resolutionScale, bool fullGameWindow,
		int renderThreadsMode);

	// Sets which mode to use for software render threads (low, medium, high, etc.).
	void setRenderThreadsMode(int mode);

	// Helper methods for changing data in the 3D renderer. Some data, like the voxel
	// grid, are passed each frame by reference.
	// - Some 'add' methods take a unique ID and parameters to create a new object.
	// - 'update' methods take optional parameters for updating, ignoring null ones.
	// - 'remove' methods delete an object from renderer memory if it exists.
	void addFlat(int id, const Double3 &position, double width, double height, int textureID);
	void addLight(int id, const Double3 &point, const Double3 &color, double intensity);
	void updateFlat(int id, const Double3 *position, const double *width, 
		const double *height, const int *textureID, const bool *flipped);
	void updateLight(int id, const Double3 *point, const Double3 *color,
		const double *intensity);
	void setFogDistance(double fogDistance);
	void setVoxelTexture(int id, const uint32_t *srcTexels);
	void setFlatTexture(int id, const uint32_t *srcTexels, int width, int height);
	void setSkyPalette(const uint32_t *colors, int count);
	void setNightLightsActive(bool active);
	void removeFlat(int id);
	void removeLight(int id);
	void clearTextures();

	// Fills the native frame buffer with the draw color, or default black/transparent.
	void clear(const Color &color);
	void clear();
	void clearOriginal(const Color &color);
	void clearOriginal();

	// Wrapper methods for some SDL draw functions.
	void drawPixel(const Color &color, int x, int y);
	void drawLine(const Color &color, int x1, int y1, int x2, int y2);
	void drawRect(const Color &color, int x, int y, int w, int h);

	// Wrapper methods for some SDL fill functions.
	void fillRect(const Color &color, int x, int y, int w, int h);
	void fillOriginalRect(const Color &color, int x, int y, int w, int h);

	// Runs the 3D renderer which draws the world onto the native frame buffer.
	// If the renderer is uninitialized, this causes a crash.
	void renderWorld(const Double3 &eye, const Double3 &forward, double fovY, 
		double ambient, double daytimePercent, double ceilingHeight, 
		const VoxelGrid &voxelGrid);

	// Draws the given cursor texture to the native frame buffer. The exact position 
	// of the cursor is modified by the cursor alignment.
	void drawCursor(SDL_Texture *texture, CursorAlignment alignment, 
		const Int2 &mousePosition, double scale);

	// Draw methods for the native and original frame buffers.
	void draw(SDL_Texture *texture, int x, int y, int w, int h);
	void draw(SDL_Texture *texture, int x, int y);
	void draw(SDL_Texture *texture);
	void drawClipped(SDL_Texture *texture, const Rect &srcRect, const Rect &dstRect);
	void drawClipped(SDL_Texture *texture, const Rect &srcRect, int x, int y);
	void drawOriginal(SDL_Texture *texture, int x, int y, int w, int h);
	void drawOriginal(SDL_Texture *texture, int x, int y);
	void drawOriginal(SDL_Texture *texture);
	void drawOriginalClipped(SDL_Texture *texture, const Rect &srcRect, const Rect &dstRect);
	void drawOriginalClipped(SDL_Texture *texture, const Rect &srcRect, int x, int y);

	// Stretches a texture over the entire native frame buffer.
	void fill(SDL_Texture *texture);

	// Refreshes the displayed frame buffer.
	void present();
};

#endif
