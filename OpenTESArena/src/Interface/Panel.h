#ifndef PANEL_H
#define PANEL_H

#include <memory>

// Each panel interprets user input and draws to the screen. There is only one panel 
// active at a time, and it is owned by the GameState.

// How might "continued" text boxes work? Arena has some pop-up text boxes that have
// multiple screens based on the amount of text, and even some buttons like "yes/no" on
// the last screen. I think I'll just replace them with scrolled text boxes. The buttons
// can be separate interface objects (no need for a "ScrollableButtonedTextBox").

class Color;
class GameState;
class Int2;
class Surface;

struct SDL_PixelFormat;
struct SDL_Rect;
struct SDL_Renderer;
struct SDL_Surface;
struct SDL_Texture;

class Panel
{
private:
	GameState *gameStatePtr;
protected:
	virtual void handleEvents(bool &running) = 0;
	virtual void handleMouse(double dt) = 0;
	virtual void handleKeyboard(double dt) = 0;

	double getCursorScale() const; // Should eventually be in the options.
	unsigned int getFormattedRGB(const Color &color, const SDL_PixelFormat *format) const;

	void clearScreen(SDL_Renderer *renderer);
	void drawCursor(const Surface &cursor, int x, int y, SDL_Renderer *renderer);
	void drawCursor(const Surface &cursor, SDL_Renderer *renderer);
	void drawScaledToNative(const SDL_Texture *texture, int x, int y, int w, int h, 
		SDL_Renderer *renderer);
	void drawScaledToNative(const SDL_Texture *texture, int x, int y, SDL_Renderer *renderer);
	void drawScaledToNative(const SDL_Texture *texture, SDL_Renderer *renderer);

	// Compatibility with software rendering. These methods are sloooow. Remove these
	// once all panels are using SDL_Textures for rendering.
	void drawScaledToNative(const Surface &surface, int x, int y, int w, int h, 
		SDL_Renderer *renderer);
	void drawScaledToNative(const Surface &surface, int x, int y, SDL_Renderer *renderer);
	void drawScaledToNative(const Surface &surface, SDL_Renderer *renderer);

	void drawLetterbox(const SDL_Texture *texture, SDL_Renderer *renderer,
		const SDL_Rect *letterbox);
public:
	Panel(GameState *gameState);
	virtual ~Panel();

	static std::unique_ptr<Panel> defaultPanel(GameState *gameState);

	GameState *getGameState() const;
	double getDrawScale() const;
	Int2 getMousePosition() const;
	bool letterboxContains(const Int2 &point) const;

	// Transforms a native window (i.e., 1920x1080) point to an original (320x200) 
	// point. Points outside the letterbox will either be negative or outside the 
	// 320x200 limit when returned.
	Int2 nativePointToOriginal(const Int2 &point) const;

	// Does the opposite of nativePointToOriginal().
	Int2 originalPointToNative(const Int2 &point) const;

	virtual void tick(double dt, bool &running) = 0;
	virtual void render(SDL_Renderer *renderer, const SDL_Rect *letterbox) = 0;
};

#endif
