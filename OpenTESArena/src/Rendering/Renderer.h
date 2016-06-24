#ifndef RENDERER_H
#define RENDERER_H

#include <string>

// The renderer is for sending the SDL_Surface to the GPU as an SDL_Texture. It isn't
// for the CLProgram functionality. The reason why this class exposes an SDL type to
// the user is because, although it'd be ideal to only give the dimensions and a pixel 
// pointer, I intend on doing SDL blits and fills to the screen, too.

// The CLProgram should be in the GameState, not the "GameWorldPanel", because if it 
// was in a panel, the kernel would be recompiled whenever the panel changes back
// (which takes a while). This reinforces my idea of having every panel get a reference 
// to the GameState object, instead of having to encase things in lambdas in the panel
// constructor.

// Interface surfaces should be scaled up by the UI scale and use panel methods for 
// getting screen dimensions when checking for mouse clicks, etc..

class Int2;
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
	static const int DEFAULT_COLOR_BITS;
	static const std::string DEFAULT_RENDER_SCALE_QUALITY;

	SDL_Window *window;
	SDL_Renderer *renderer;

	// This is private now, for use with window dimensions, etc.. Not for rendering.
	SDL_Surface *getWindowSurface() const;

	// Helper method for making a renderer context.
	SDL_Renderer *createRenderer();
public:
	Renderer(int width, int height, bool fullscreen, const std::string &title);
	~Renderer();
	
	// Get the dimensions of the render texture. This should always match the window
	// dimensions.
	Int2 getRenderDimensions() const;

	// For converting surfaces to the correct ARGB format.
	SDL_PixelFormat *getPixelFormat() const;

	// For hardware-accelerated rendering using SDL_Textures.
	SDL_Renderer *getRenderer() const;

	// This is for the "letterbox" part of the screen, scaled to fit while using
	// a constant aspect ratio.
	SDL_Rect getLetterboxDimensions() const;

	void resize(int width, int height);
	void setWindowIcon(TextureName name, TextureManager &textureManager);
	void present();
};

#endif
