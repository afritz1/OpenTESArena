#ifndef GAME_WORLD_PANEL_H
#define GAME_WORLD_PANEL_H

#include "Panel.h"

// When the GameWorldPanel is active, the game world is ticking.

// There are two desired kinds of interfaces:
// - The original: compass, portrait, stat bars, and buttons.
// - A modern version: only compass and stat bars.

// The original interface needs the mouse cursor visible.
// The modern interface does not need the mouse cursor visible.

class Button;
class TextBox;

class GameWorldPanel : public Panel
{
private:
	std::unique_ptr<TextBox> playerNameTextBox;
	std::unique_ptr<Button> automapButton, characterSheetButton, logbookButton, 
		pauseButton, worldMapButton;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	// Constructs the game world panel. The GameData object in GameState must be
	// initialized.
	GameWorldPanel(GameState *gameState);
	virtual ~GameWorldPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Renderer *renderer, const SDL_Rect *letterbox) override;
};

#endif
