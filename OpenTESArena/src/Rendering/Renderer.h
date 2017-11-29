#ifndef RENDERER_H
#define RENDERER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"

// Acts as a wrapper for SDL_Renderer operations as well as 3D rendering operations.

// The format for all textures is ARGB8888.

class Color;
class Rect;
class SoftwareRenderer;
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
	std::unique_ptr<SoftwareRenderer> softwareRenderer; // 3D renderer.
	double letterboxAspect;
	bool fullGameWindow; // Determines height of 3D frame buffer.

	// Helper method for making a renderer context.
	SDL_Renderer *createRenderer();

	// For use with window dimensions, etc.. No longer used for rendering.
	SDL_Surface *getWindowSurface() const;
public:
	Renderer(int width, int height, bool fullscreen, double letterboxAspect);
	~Renderer();

	// Original screen dimensions.
	static const int ORIGINAL_WIDTH;
	static const int ORIGINAL_HEIGHT;

	// The default pixel format for all software surfaces, ARGB8888.
	static const uint32_t DEFAULT_PIXELFORMAT;

	// Default bits per pixel.
	static const int DEFAULT_BPP;

	// Gets the width and height of the active window.
	Int2 getWindowDimensions() const;

	// The "view height" is the height in pixels for the visible game world. This 
	// depends on whether the whole screen is rendered or just the portion above 
	// the interface. The game interface is 53 pixels tall in 320x200.
	int getViewHeight() const;

	// This is for the "letterbox" part of the screen, scaled to fit the window 
	// using the given letterbox aspect.
	SDL_Rect getLetterboxDimensions() const;

	// Gets a screenshot of the current window. The returned surface should be freed
	// by the caller with SDL_FreeSurface() when finished.
	SDL_Surface *getScreenshot() const;

	// Transforms a native window (i.e., 1920x1080) point to an original (320x200) 
	// point. Points outside the letterbox will either be negative or outside the 
	// 320x200 limit when returned.
	Int2 nativePointToOriginal(const Int2 &nativePoint) const;

	// Does the opposite of nativePointToOriginal().
	Int2 originalPointToNative(const Int2 &originalPoint) const;

	// Same as nativePointToOriginal() but for rectangles.
	Rect nativeRectToOriginal(const Rect &nativeRect) const;

	// Same as originalPointToNative() but for rectangles.
	Rect originalRectToNative(const Rect &originalRect) const;

	// Returns true if the letterbox contains a native point.
	bool letterboxContains(const Int2 &nativePoint) const;

	// Wrapper methods for SDL_CreateTexture.
	SDL_Texture *createTexture(uint32_t format, int access, int w, int h);
	SDL_Texture *createTextureFromSurface(SDL_Surface *surface);

	// Resizes the renderer dimensions.
	void resize(int width, int height, double resolutionScale, bool fullGameWindow);

	// Sets the letterbox aspect. 1.60 is the default, and 1.33 is the "stretched"
	// aspect for simulating tall pixels on a 640x480 display.
	void setLetterboxAspect(double letterboxAspect);

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
	void initializeWorldRendering(double resolutionScale, bool fullGameWindow);

	// Helper methods for changing data in the 3D renderer. Some data, like the voxel
	// grid, are passed each frame by reference.
	// - Some 'add' methods take a unique ID and parameters to create a new object.
	// - 'update' methods take optional parameters for updating, ignoring null ones.
	// - 'remove' methods delete an object from renderer memory if it exists.
	void addFlat(int id, const Double3 &position, double width, double height, int textureID);
	void addLight(int id, const Double3 &point, const Double3 &color, double intensity);
	int addTexture(const uint32_t *pixels, int width, int height);
	void updateFlat(int id, const Double3 *position, const double *width, 
		const double *height, const int *textureID, const bool *flipped);
	void updateLight(int id, const Double3 *point, const Double3 *color,
		const double *intensity);
	void setFogDistance(double fogDistance);
	void setSkyPalette(const uint32_t *colors, int count);
	void removeFlat(int id);
	void removeLight(int id);
	void removeAllWorldTextures();

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
		double ambient, double daytimePercent, const VoxelGrid &voxelGrid);

	// Draws the given cursor texture to the native frame buffer. The exact position 
	// of the cursor is modified by the cursor alignment.
	void drawCursor(SDL_Texture *texture, CursorAlignment alignment, 
		const Int2 &mousePosition, double scale);

	// Draw methods for the native and original frame buffers.
	void draw(SDL_Texture *texture, int x, int y, int w, int h);
	void draw(SDL_Texture *texture, int x, int y);
	void draw(SDL_Texture *texture);
	void drawOriginal(SDL_Texture *texture, int x, int y, int w, int h);
	void drawOriginal(SDL_Texture *texture, int x, int y);
	void drawOriginal(SDL_Texture *texture);

	// Stretches a texture over the entire native frame buffer.
	void fill(SDL_Texture *texture);

	// Refreshes the displayed frame buffer.
	void present();
};

#endif
