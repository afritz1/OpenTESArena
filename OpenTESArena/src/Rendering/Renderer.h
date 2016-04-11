#ifndef RENDERER_H
#define RENDERER_H

#include <memory>
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

class TextureManager;

enum class TextureName;

struct SDL_Rect;
struct SDL_Renderer;
struct SDL_Surface;
struct SDL_Texture;
struct SDL_Window;

class Renderer
{
private:
	static const int DEFAULT_COLOR_BITS;
	static const std::string DEFAULT_RENDER_SCALE_QUALITY;

	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;

	void checkSuccess(bool success, const std::string &name);
public:
	Renderer(int width, int height, bool fullscreen, const std::string &title);
	~Renderer();

	SDL_Surface *getWindowSurface() const;

	// This is for the "letterbox" part of the screen. It is scaled and maintains 
	// the original 1.6:1 (320x200) aspect ratio for fullscreen interface items.
	std::unique_ptr<SDL_Rect> getLetterboxDimensions() const;

	void resize(int width, int height);
	void setWindowIcon(TextureName name, TextureManager &textureManager);
	void present();
};

#endif
