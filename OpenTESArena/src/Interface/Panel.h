#ifndef PANEL_H
#define PANEL_H

#include <memory>
#include <string>

#include "../Math/Vector2.h"

// Each panel interprets user input and draws to the screen. There is only one panel 
// active at a time, and it is owned by the Game.

// How might "continued" text boxes work? Arena has some pop-up text boxes that have
// multiple screens based on the amount of text, and even some buttons like "yes/no" on
// the last screen. I think I'll just replace them with scrolled text boxes. The buttons
// can be separate interface objects (no need for a "ScrollableButtonedTextBox").

class Color;
class Font;
class Game;
class Renderer;

enum class CursorAlignment;

struct SDL_Texture;

union SDL_Event;

class Panel
{
private:
	Game *game;
protected:
	// Generates a tooltip texture with the default white foreground and gray
	// background with alpha blending.
	static SDL_Texture *createTooltip(const std::string &text,
		const Font &font, Renderer &renderer);

	Game *getGame() const;
public:
	Panel(Game *game);
	virtual ~Panel();

	static std::unique_ptr<Panel> defaultPanel(Game *game);

	// Gets the panel's active mouse cursor and alignment. Override this method if
	// the panel has at least one cursor defined. The texture must be supplied by 
	// the texture manager.
	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const;

	// Handles panel-specific events. Application events like closing and resizing
	// are handled by the game loop.
	virtual void handleEvent(const SDL_Event &e) = 0;

	// Animates the panel by delta time. Override this method if a panel animates
	// in some form each frame without user input, or depends on things like a key
	// or a mouse button being held down.
	virtual void tick(double dt);

	// Draws the panel's contents onto the display.
	virtual void render(Renderer &renderer) = 0;
};

#endif
