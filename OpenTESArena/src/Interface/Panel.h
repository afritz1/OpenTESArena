#ifndef PANEL_H
#define PANEL_H

#include <memory>

// Each panel interprets user input and draws to the screen. There is only one panel 
// active at a time, and it is owned by the GameState.

// How might "continued" text boxes work? Arena has some pop-up text boxes that have
// multiple screens based on the amount of text, and even some buttons like "yes/no" on
// the last screen. I think I'll just replace them with scrolled text boxes. The buttons
// can be separate interface objects (no need for a "ScrollableButtonedTextBox").

class GameState;
class Int2;
class Renderer;

union SDL_Event;

class Panel
{
private:
	GameState *gameState;
protected:
	GameState *getGameState() const;
	double getCursorScale() const;
	Int2 getMousePosition() const;
public:
	Panel(GameState *gameState);
	virtual ~Panel();

	static std::unique_ptr<Panel> defaultPanel(GameState *gameState);

	// Sets whether the mouse should move during motion events (for player camera).
	void setRelativeMouseMode(bool active);

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
