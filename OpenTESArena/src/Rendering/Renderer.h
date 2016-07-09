#ifndef RENDERER_H
#define RENDERER_H

#include <string>

// Acts as an SDL_Renderer wrapper.

// The format for all textures is ARGB8888.

class Color;
class Int2;
class Surface;
class TextureManager;

enum class TextureName;

struct SDL_PixelFormat;
struct SDL_Rect;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Window;

class Renderer
{
private:
	static const int DEFAULT_COLOR_BITS_PER_PIXEL;
	static const std::string DEFAULT_RENDER_SCALE_QUALITY;

	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *nativeTexture, *originalTexture; // Frame buffers.
	double letterboxAspect;

	// Helper method for making a renderer context.
	SDL_Renderer *createRenderer();

	// For use with window dimensions, etc.. Not for rendering.
	SDL_Surface *getWindowSurface() const;
public:
	Renderer(int width, int height, bool fullscreen, double letterboxAspect);
	Renderer(int width, int height, bool fullscreen);
	~Renderer();

	// Gets the width and height of the active window.
	Int2 getWindowDimensions() const;

	// For converting surfaces to the correct ARGB8888 format.
	SDL_PixelFormat *getFormat() const;

	// This is for the "letterbox" part of the screen, scaled to fit the window 
	// using the given letterbox aspect.
	SDL_Rect getLetterboxDimensions() const;

	unsigned int getFormattedARGB(const Color &color) const;

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
	SDL_Texture *createTextureFromSurface(const Surface &surface);

	void resize(int width, int height);
	void setWindowIcon(TextureName name, TextureManager &textureManager);

	// Tells the original frame buffer whether to blend with transparency when 
	// copying to the native frame buffer. This might be an expensive operation,
	// so only set it to true when the original frame buffer needs transparency.
	void useTransparencyBlending(bool blend);

	// Fills the desired frame buffer with the draw color, or default black/transparent.
	void clearNative(const Color &color);
	void clearNative();
	void clearOriginal(const Color &color);
	void clearOriginal();

	// Draw methods for the native and original frame buffers. Remove the SDL_Surface
	// methods once all panels are using textures exclusively.
	void drawToNative(SDL_Texture *texture, int x, int y, int w, int h);
	void drawToNative(SDL_Texture *texture, int x, int y);
	void drawToNative(SDL_Texture *texture);
	void drawToNative(SDL_Surface *surface, int x, int y, int w, int h);
	void drawToNative(SDL_Surface *surface, int x, int y);
	void drawToNative(SDL_Surface *surface);
	void drawToOriginal(SDL_Texture *texture, int x, int y, int w, int h);
	void drawToOriginal(SDL_Texture *texture, int x, int y);
	void drawToOriginal(SDL_Texture *texture);
	void drawToOriginal(SDL_Surface *surface, int x, int y, int w, int h);
	void drawToOriginal(SDL_Surface *surface, int x, int y);
	void drawToOriginal(SDL_Surface *surface);

	// Scales and copies the original frame buffer onto the native frame buffer.
	// If Renderer::useTransparencyBlending() is set to true, it also uses blending.
	void drawOriginalToNative();

	void present();
};

#endif
