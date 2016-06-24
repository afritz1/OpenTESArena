#ifndef LOAD_GAME_PANEL_H
#define LOAD_GAME_PANEL_H

#include "Panel.h"

class Button;
class Surface;
class TextBox;

class LoadGamePanel : public Panel
{
private:
	// up/down arrow buttons, saved game buttons...
	std::unique_ptr<Button> backButton;
	std::unique_ptr<TextBox> underConstructionTextBox;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	// This could get the game data boolean from the game state itself to determine
	// whether to return to the main menu or the pause menu.
	LoadGamePanel(GameState *gameState);
	virtual ~LoadGamePanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Renderer *renderer, const SDL_Rect *letterbox) override;
};

#endif
