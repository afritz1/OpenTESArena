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

class Panel
{
private:
	GameState *gameStatePtr;
protected:
	virtual void handleEvents(bool &running) = 0;
	virtual void handleMouse(double dt) = 0;
	virtual void handleKeyboard(double dt) = 0;

	GameState *getGameState() const;
	double getCursorScale() const; // Should eventually be in the options.
	Int2 getMousePosition() const;
public:
	Panel(GameState *gameState);
	virtual ~Panel();

	static std::unique_ptr<Panel> defaultPanel(GameState *gameState);

	// Sets whether the mouse should move during motion events (for player camera).
	void setRelativeMouseMode(bool active);

	virtual void tick(double dt, bool &running) = 0;
	virtual void render(Renderer &renderer) = 0;
};

#endif
