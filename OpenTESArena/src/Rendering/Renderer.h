#ifndef RENDERER_H
#define RENDERER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Rect3D.h"
#include "../Math/Float3.h"

// Acts as a wrapper for SDL_Renderer operations as well as 3D rendering operations.

// The format for all textures is ARGB8888.

class CLProgram;
class Color;
class Int2;

enum class TextureName;

struct SDL_PixelFormat;
struct SDL_Rect;
struct SDL_Renderer;
struct SDL_Surface;
struct SDL_Texture;
struct SDL_Window;

class Renderer
{
private:
	static const std::string DEFAULT_RENDER_SCALE_QUALITY;

	std::unique_ptr<CLProgram> clProgram;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *nativeTexture, *originalTexture, *gameWorldTexture; // Frame buffers.
	double letterboxAspect;
	bool fullGameWindow; // Determines height of 3D frame buffer.

	// Helper method for making a renderer context.
	SDL_Renderer *createRenderer();

	// For use with window dimensions, etc.. No longer used for rendering.
	SDL_Surface *getWindowSurface() const;
public:
	Renderer(int width, int height, bool fullscreen, double letterboxAspect);
	Renderer(int width, int height, bool fullscreen);
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

	// Returns true if the letterbox contains a native point.
	bool letterboxContains(const Int2 &nativePoint) const;

	// Wrapper methods for SDL_CreateTexture.
	SDL_Texture *createTexture(unsigned int format, int access, int w, int h);
	SDL_Texture *createTextureFromSurface(SDL_Surface *surface);

	// Resizes the renderer dimensions, and rebuilds the render program if it is
	// initialized.
	void resize(int width, int height, double resolutionScale);

	// Sets the window icon to be the given surface.
	void setWindowIcon(SDL_Surface *icon);

	// Teleports the mouse to a location in the window.
	void warpMouse(int x, int y);

	// Tells the original frame buffer whether to blend with transparency when 
	// copying to the native frame buffer. This might be an expensive operation,
	// so only set it to true when the original frame buffer needs transparency.
	void useTransparencyBlending(bool blend);

	// Initialize the OpenCL program for rendering the game world. The "renderFullWindow"
	// argument determines whether to render a "fullscreen" 3D image or just the part
	// above the game interface. If there is an existing OpenCL program in memory, it 
	// will be overwritten with the new one.
	void initializeWorldRendering(int worldWidth, int worldHeight, int worldDepth,
		double resolutionScale, bool renderFullWindow);

	// Helper methods for interacting with OpenCL device memory.
	void updateCamera(const Float3d &eye, const Float3d &direction, double fovY);
	void updateGameTime(double gameTime);
	int addTexture(uint32_t *pixels, int width, int height);
	void updateVoxel(int x, int y, int z, const std::vector<Rect3D> &rects,
		const std::vector<int> &textureIndices);
	void updateVoxel(int x, int y, int z, const std::vector<Rect3D> &rects,
		int textureIndex);
	void updateSprite(int spriteID, const Rect3D &rect, int textureIndex);
	void removeSprite(int spriteID);

	// Fills the desired frame buffer with the draw color, or default black/transparent.
	void clearNative(const Color &color);
	void clearNative();
	void clearOriginal(const Color &color);
	void clearOriginal();

	// Wrapper methods for some SDL draw functions. These draw onto the native texture.
	void drawPixel(const Color &color, int x, int y);
	void drawLine(const Color &color, int x1, int y1, int x2, int y2);
	void drawRect(const Color &color, int x, int y, int w, int h);

	// Runs the rendering program which draws the world onto the native frame buffer.
	// If the rendering program is uninitialized, this causes a crash.
	void renderWorld();

	// Draw methods for the native and original frame buffers.
	void drawToNative(SDL_Texture *texture, int x, int y, int w, int h);
	void drawToNative(SDL_Texture *texture, int x, int y);
	void drawToNative(SDL_Texture *texture);
	void drawToOriginal(SDL_Texture *texture, int x, int y, int w, int h);
	void drawToOriginal(SDL_Texture *texture, int x, int y);
	void drawToOriginal(SDL_Texture *texture);

	// Stretches a texture over the entire native frame buffer.
	void fillNative(SDL_Texture *texture);

	// Scales and copies the original frame buffer onto the native frame buffer.
	// If Renderer::useTransparencyBlending() is set to true, it also uses blending.
	void drawOriginalToNative();

	// Refreshes the displayed frame buffer.
	void present();
};

#endif
