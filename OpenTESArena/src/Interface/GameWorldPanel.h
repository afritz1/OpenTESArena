#ifndef GAME_WORLD_PANEL_H
#define GAME_WORLD_PANEL_H

#include "Panel.h"

// There should be hotkeys only in the game world panel. There isn't going to be
// a game world interface to click in this project. Just stat bars and compass.
// Maybe some options will be available later on.

class Button;

class GameWorldPanel : public Panel
{
private:
	std::unique_ptr<Button> pauseButton;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	GameWorldPanel(GameState *gameState);
	virtual ~GameWorldPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Surface *dst, const SDL_Rect *letterbox) override;
};

#endif
